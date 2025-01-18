cmake_minimum_required(VERSION 3.15)

add_library(hideous_hook SHARED
    hideous_hook.cpp
    hideous_hook.h
)

target_compile_definitions(hideous_hook PRIVATE
    HIDEOUS_HOOK_EXPORTS
    UNICODE
    _UNICODE
)

target_include_directories(hideous_hook PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
