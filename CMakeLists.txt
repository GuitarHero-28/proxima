cmake_minimum_required(VERSION 3.15)
project(proxima)

# Use C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Load Node.js Build Tools
find_package(cmake-js REQUIRED)

# Define the library
add_library(proxima SHARED
    src/addon.cpp
    src/Orderbook.cpp
)

# Include directories
target_include_directories(proxima PRIVATE src)

# Link Libraries
target_link_libraries(proxima ${CMAKE_JS_LIB})

# N-API Flags (Disable exceptions for speed)
target_compile_definitions(proxima PRIVATE NAPI_DISABLE_CPP_EXCEPTIONS)

# Windows Compatibility
if(WIN32)
    target_compile_definitions(proxima PRIVATE NOMINMAX)
endif()