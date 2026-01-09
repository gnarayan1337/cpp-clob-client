cmake_minimum_required(VERSION 3.15)
project(cpp-clob-client VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Options
option(BUILD_EXAMPLES "Build example programs" ON)
option(BUILD_TESTS "Build tests" OFF)

# Find required packages
find_package(CURL REQUIRED)
find_package(OpenSSL REQUIRED)

# Find secp256k1 (installed via homebrew on macOS)
find_path(SECP256K1_INCLUDE_DIR secp256k1.h
    PATHS
    /opt/homebrew/include
    /usr/local/include
    /usr/include
)

find_library(SECP256K1_LIBRARY
    NAMES secp256k1 libsecp256k1
    PATHS
    /opt/homebrew/lib
    /usr/local/lib
    /usr/lib
)

if(NOT SECP256K1_INCLUDE_DIR OR NOT SECP256K1_LIBRARY)
    message(FATAL_ERROR "secp256k1 not found. Install with: brew install secp256k1")
endif()

message(STATUS "Found secp256k1: ${SECP256K1_LIBRARY}")
message(STATUS "secp256k1 include: ${SECP256K1_INCLUDE_DIR}")

# Include FetchContent for dependencies
include(FetchContent)

# simdjson - High performance SIMD JSON parser
FetchContent_Declare(
    simdjson
    GIT_REPOSITORY https://github.com/simdjson/simdjson.git
    GIT_TAG v3.11.5
)
FetchContent_MakeAvailable(simdjson)

# nlohmann/json - Keep for types serialization (to_json/from_json)
FetchContent_Declare(
    json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
)
FetchContent_MakeAvailable(json)

# cpp-httplib
FetchContent_Declare(
    httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG v0.18.2
)
FetchContent_MakeAvailable(httplib)

# secp256k1 (you may need to install this separately)
# On macOS: brew install secp256k1
# On Ubuntu: sudo apt-get install libsecp256k1-dev

# Library sources
set(CLOB_SOURCES
    src/client.cpp
    src/signer.cpp
    src/order_builder.cpp
    src/http_client.cpp
    src/eip712.cpp
    src/utilities.cpp
    src/constants.cpp
    src/eth_rpc.cpp
)

# Create library
add_library(clob_client STATIC ${CLOB_SOURCES})

target_include_directories(clob_client
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        ${SECP256K1_INCLUDE_DIR}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(clob_client
    PUBLIC
        simdjson::simdjson
        nlohmann_json::nlohmann_json
        httplib::httplib
        OpenSSL::Crypto
        CURL::libcurl
        ${SECP256K1_LIBRARY}
)

# Examples
if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Tests
if(BUILD_TESTS)
    include(CTest)
    enable_testing()
    add_subdirectory(tests)
endif()
