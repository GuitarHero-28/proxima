cmake_minimum_required(VERSION 3.15)
project(proxima)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(proxima SHARED
    src/addon.cpp
    src/Orderbook.cpp
)

# DIRECT PATH FIX:
target_include_directories(proxima PRIVATE
    src
    ${CMAKE_JS_INC}
    "${CMAKE_SOURCE_DIR}/node_modules/node-addon-api"
)

# NEW: Force the output file to be "proxima.node", not "proxima.dll"
set_target_properties(proxima PROPERTIES PREFIX "" SUFFIX ".node")

target_link_libraries(proxima ${CMAKE_JS_LIB})

target_compile_definitions(proxima PRIVATE NAPI_DISABLE_CPP_EXCEPTIONS)

if(WIN32)
    target_compile_definitions(proxima PRIVATE NOMINMAX)
endif()