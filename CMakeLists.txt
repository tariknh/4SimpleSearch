cmake_minimum_required(VERSION 3.20)

project(Submission05 LANGUAGES CXX)

add_executable(Submission-05
    main.cpp
    main.h
)

find_package(CURL QUIET)
if(CURL_FOUND)
    target_link_libraries(Submission-05 PRIVATE CURL::libcurl)
    target_compile_definitions(Submission-05 PRIVATE HAVE_CURL)
    message(STATUS "CURL found - online name fetching enabled")
else()
    message(WARNING "CURL not found - will use offline fallback for name generation")
endif()

target_compile_features(Submission-05 PUBLIC cxx_std_20)

# Copy resources directory to build directory so the executable can find it
add_custom_command(TARGET Submission-05 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/resources
    $<TARGET_FILE_DIR:Submission-05>/resources
    COMMENT "Copying resources directory to build output"
)

# TODO: Add tests and install targets if needed.