cmake_minimum_required(VERSION 3.20)

project(CxxChatwork LANGUAGES CXX)

add_executable(cxx_chatwork
    src/main.cpp
)

target_compile_features(cxx_chatwork PRIVATE cxx_std_23)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(cxx_chatwork PRIVATE -Wall -Wextra -Wpedantic)
endif()
