cmake_minimum_required(VERSION 3.20)

project(Submission05 LANGUAGES CXX)

add_executable(Submission-05
    main.cpp
    main.h
)

find_package(CURL REQUIRED)
target_link_libraries(Submission-05 PRIVATE CURL::libcurl)

target_compile_features(Submission-05 PUBLIC cxx_std_20)

# TODO: Add tests and install targets if needed.