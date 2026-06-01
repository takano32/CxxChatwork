cmake_minimum_required(VERSION 3.20)

project(CxxChatwork LANGUAGES CXX)

find_package(OpenSSL REQUIRED)
find_package(CURL REQUIRED)

add_executable(cxx_chatwork
    src/main.cpp
    src/http.cpp
    src/json.cpp
    src/server.cpp
    src/uri.cpp
    src/hatena_client.cpp
    src/hatena_bookmark_client.cpp
    src/oauth.cpp
)

target_link_libraries(cxx_chatwork PRIVATE OpenSSL::Crypto CURL::libcurl)

target_compile_features(cxx_chatwork PRIVATE cxx_std_23)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(cxx_chatwork PRIVATE -Wall -Wextra -Wpedantic)
endif()
