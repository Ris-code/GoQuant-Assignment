cmake_minimum_required(VERSION 3.10)
project(GoQuant-Assignment)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Include directories
include_directories(include)
include_directories(libs/websocketpp)
include_directories(libs/json/include)

# Find required packages
find_package(Threads REQUIRED)
find_package(CURL REQUIRED)

# Find OpenSSL
find_package(OpenSSL REQUIRED)

if (CURL_FOUND)
    message(STATUS "CURL library found: ${CURL_LIBRARIES}")
    message(STATUS "CURL include dirs: ${CURL_INCLUDE_DIRS}")
else()
    message(FATAL_ERROR "CURL library not found")
endif()

# Source files
file(GLOB SOURCES "src/*.cpp")

# Executable
add_executable(GoQuant-Assignment ${SOURCES})

# Link libraries
target_link_libraries(GoQuant-Assignment PRIVATE Threads::Threads CURL::libcurl OpenSSL::Crypto OpenSSL::SSL)

# Copy config.json to build directory after build
add_custom_command(TARGET GoQuant-Assignment POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/config.json"
        $<TARGET_FILE_DIR:GoQuant-Assignment>)