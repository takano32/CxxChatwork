#include "hatena_bookmark_client.hpp"

#include "json.hpp"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string_view>

#include <curl/curl.h>

namespace chatwork {
namespace {

std::string percent_encode(std::string_view s) {
    std::ostringstream out;
    for (unsigned char c : s) {
        if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            out << c;
        } else {
            out << '%' << std::uppercase << std::hex << std::setw(2) << std::setfill('0')
                << static_cast<int>(c);
        }
    }
    return out.str();
}

std::string form_encode(const std::map<std::string, std::string>& params) {
    std::string result;
    for (const auto& [k, v] : params) {
        if (!result.empty()) result += '&';
        result += percent_encode(k) + "=" + percent_encode(v);
    }
    return result;
}

std::size_t write_callback(char* ptr, std::size_t size, std::size_t nmemb, void* userdata) {
    static_cast<std::string*>(userdata)->append(ptr, size * nmemb);
    return size * nmemb;
}

std::string format_bookmark_line(std::string_view label,
                                 const std::string& url,
                                 const std::string& comment,
                                 const std::vector<std::string>& tags) {
    std::string joined_tags;
    for (std::size_t i = 0; i < tags.size(); ++i) {
        if (i > 0) joined_tags += "][";
        joined_tags += tags[i];
    }
    return std::string(label) + " [" + joined_tags + "], " + url + ", " + comment;
}

} // namespace

void HatenaBookmarkClient::process(const std::string& url) {
    const auto result = post_bookmark(url);
    const auto label = (result == BookmarkResult::Added) ? "[ADD]" : "[UPDATE]";
    std::cout << format_bookmark_line(label, url, {}, {}) << std::endl;
}

BookmarkResult HatenaBookmarkClient::post_bookmark(const std::string& url,
                                                    const std::string& comment,
                                                    const std::vector<std::string>& tags) {
    constexpr std::string_view endpoint =
        "https://bookmark.hatenaapis.com/rest/1/my/bookmark";

    std::map<std::string, std::string> body_params;
    body_params["url"] = url;
    if (!comment.empty()) {
        body_params["comment"] = comment;
    }
    if (!tags.empty()) {
        std::string tags_str;
        for (const auto& tag : tags) {
            if (!tags_str.empty()) tags_str += ' ';
            tags_str += tag;
        }
        body_params["tags"] = tags_str;
    }

    const std::string auth_header =
        _oauth.authorization_header("POST", endpoint, body_params);
    const std::string body = form_encode(body_params);

    CURL* curl = curl_easy_init();
    if (curl == nullptr) {
        throw std::runtime_error("curl_easy_init failed");
    }

    std::string response_body;
    curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, ("Authorization: " + auth_header).c_str());
    headers = curl_slist_append(headers, "Content-Type: application/x-www-form-urlencoded");

    curl_easy_setopt(curl, CURLOPT_URL, std::string(endpoint).c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_body);

    const CURLcode res = curl_easy_perform(curl);
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        throw std::runtime_error(std::string("curl error: ") + curl_easy_strerror(res));
    }
    if (http_code != 200 && http_code != 201) {
        throw std::runtime_error("bookmark API error: HTTP " + std::to_string(http_code)
                                 + " " + response_body);
    }

    return http_code == 201 ? BookmarkResult::Added : BookmarkResult::Updated;
}

HatenaBookmark HatenaBookmarkClient::get_bookmark(const std::string& url) {
    constexpr std::string_view endpoint =
        "https://bookmark.hatenaapis.com/rest/1/my/bookmark";

    const std::map<std::string, std::string> query_params{{"url", url}};
    const std::string auth_header =
        _oauth.authorization_header("GET", endpoint, query_params);
    const std::string request_url =
        std::string(endpoint) + "?url=" + percent_encode(url);

    CURL* curl = curl_easy_init();
    if (curl == nullptr) {
        throw std::runtime_error("curl_easy_init failed");
    }

    std::string response_body;
    curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, ("Authorization: " + auth_header).c_str());

    curl_easy_setopt(curl, CURLOPT_URL, request_url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_body);

    const CURLcode res = curl_easy_perform(curl);
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        throw std::runtime_error(std::string("curl error: ") + curl_easy_strerror(res));
    }
    if (http_code != 200) {
        throw std::runtime_error("bookmark API error: HTTP " + std::to_string(http_code)
                                 + " " + response_body);
    }

    const JSON json = JSON::parse(response_body);

    std::string comment;
    if (json.contains("comment") && json.at("comment").is_string()) {
        comment = json.at("comment").string();
    }

    std::vector<std::string> tags;
    if (json.contains("tags") && json.at("tags").is_array()) {
        const auto& array = json.at("tags").array();
        const bool all_strings = std::all_of(array.begin(), array.end(),
                                              [](const JSON& tag) { return tag.is_string(); });
        if (all_strings) {
            for (const auto& tag : array) {
                tags.push_back(tag.string());
            }
        }
    }

    std::cout << format_bookmark_line("[GET]", url, comment, tags) << std::endl;

    return HatenaBookmark(url, std::move(comment), std::move(tags));
}

} // namespace chatwork
