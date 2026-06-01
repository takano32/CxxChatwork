#include "hatena_client.hpp"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <map>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string_view>

#include <curl/curl.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>

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

std::string base64_encode(const unsigned char* data, std::size_t len) {
    constexpr std::string_view table =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string result;
    result.reserve((len + 2) / 3 * 4);
    for (std::size_t i = 0; i < len; i += 3) {
        const unsigned int b0 = data[i];
        const unsigned int b1 = (i + 1 < len) ? data[i + 1] : 0u;
        const unsigned int b2 = (i + 2 < len) ? data[i + 2] : 0u;
        result += table[(b0 >> 2) & 0x3f];
        result += table[((b0 << 4) | (b1 >> 4)) & 0x3f];
        result += (i + 1 < len) ? table[((b1 << 2) | (b2 >> 6)) & 0x3f] : '=';
        result += (i + 2 < len) ? table[b2 & 0x3f] : '=';
    }
    return result;
}

std::string hmac_sha1(std::string_view key, std::string_view data) {
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digest_len = 0;
    HMAC(EVP_sha1(),
         key.data(), static_cast<int>(key.size()),
         reinterpret_cast<const unsigned char*>(data.data()), data.size(),
         digest, &digest_len);
    return base64_encode(digest, digest_len);
}

std::string generate_nonce() {
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<std::uint64_t> dist;
    std::ostringstream oss;
    oss << std::hex << dist(gen) << dist(gen);
    return oss.str();
}

std::string current_timestamp() {
    const auto now = std::chrono::system_clock::now();
    const auto secs =
        std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
    return std::to_string(secs);
}

std::size_t write_callback(char* ptr, std::size_t size, std::size_t nmemb, void* userdata) {
    static_cast<std::string*>(userdata)->append(ptr, size * nmemb);
    return size * nmemb;
}

std::string parse_oauth_param(std::string_view response, std::string_view key) {
    const std::string needle = std::string(key) + "=";
    const auto pos = response.find(needle);
    if (pos == std::string_view::npos) {
        return {};
    }
    const auto start = pos + needle.size();
    const auto end = response.find('&', start);
    return std::string(response.substr(
        start, end == std::string_view::npos ? std::string_view::npos : end - start));
}

std::string form_encode(const std::map<std::string, std::string>& params) {
    std::string result;
    for (const auto& [k, v] : params) {
        if (!result.empty()) result += '&';
        result += percent_encode(k) + "=" + percent_encode(v);
    }
    return result;
}

} // namespace

HatenaClient::HatenaClient(const std::string& consumer_key,
                           const std::string& consumer_secret)
    : consumer_key(consumer_key), consumer_secret(consumer_secret) {}

std::string HatenaClient::oauth_authorization_header(
    std::string_view method,
    std::string_view url,
    const std::map<std::string, std::string>& body_params,
    const std::map<std::string, std::string>& extra_oauth_params) const {

    const std::string nonce = generate_nonce();
    const std::string timestamp = current_timestamp();

    std::map<std::string, std::string> oauth_params = {
        {"oauth_consumer_key",     consumer_key},
        {"oauth_nonce",            nonce},
        {"oauth_signature_method", "HMAC-SHA1"},
        {"oauth_timestamp",        timestamp},
        {"oauth_version",          "1.0"},
    };
    if (!request_token.empty()) {
        oauth_params["oauth_token"] = request_token;
    }
    for (const auto& [k, v] : extra_oauth_params) {
        oauth_params[k] = v;
    }

    // oauth params + body params merged and sorted for signature base string
    std::map<std::string, std::string> all_params = oauth_params;
    all_params.insert(body_params.begin(), body_params.end());

    std::string param_string;
    for (const auto& [k, v] : all_params) {
        if (!param_string.empty()) param_string += '&';
        param_string += percent_encode(k) + "=" + percent_encode(v);
    }

    const std::string base_string =
        std::string(method) + "&" + percent_encode(url) + "&" + percent_encode(param_string);
    const std::string signing_key =
        percent_encode(consumer_secret) + "&" + percent_encode(request_token_secret);
    const std::string signature = hmac_sha1(signing_key, base_string);

    std::string auth_header = "OAuth ";
    for (const auto& [k, v] : oauth_params) {
        auth_header += percent_encode(k) + "=\"" + percent_encode(v) + "\", ";
    }
    auth_header += "oauth_signature=\"" + percent_encode(signature) + "\"";

    return auth_header;
}

void HatenaClient::oauth() {
    constexpr std::string_view url = "https://www.hatena.com/oauth/initiate";

    const std::string auth_header = oauth_authorization_header(
        "POST", url, {}, {{"oauth_callback", "oob"}});

    CURL* curl = curl_easy_init();
    if (curl == nullptr) {
        throw std::runtime_error("curl_easy_init failed");
    }

    std::string response_body;
    curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, ("Authorization: " + auth_header).c_str());

    curl_easy_setopt(curl, CURLOPT_URL, std::string(url).c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_body);

    const CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        throw std::runtime_error(std::string("curl error: ") + curl_easy_strerror(res));
    }

    request_token = parse_oauth_param(response_body, "oauth_token");
    request_token_secret = parse_oauth_param(response_body, "oauth_token_secret");

    if (request_token.empty()) {
        throw std::runtime_error("failed to get request token: " + response_body);
    }

    std::cout << "Authorize at: https://www.hatena.com/oauth/authorize?oauth_token="
              << request_token << "\n";
}

void HatenaClient::post_bookmark(const std::string& url,
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
        oauth_authorization_header("POST", endpoint, body_params, {});
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
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        throw std::runtime_error(std::string("curl error: ") + curl_easy_strerror(res));
    }
}

} // namespace chatwork
