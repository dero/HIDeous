cmake_minimum_required(VERSION 3.15)

add_library(hideous_hook SHARED
    dll_main.cpp
    hideous_hook.h
    hideous_hook.def
    ../common/utils/constants.h
)

target_compile_definitions(hideous_hook PRIVATE
    HIDEOUS_HOOK_EXPORTS
    UNICODE
    _UNICODE
)

set_target_properties(hideous_hook PROPERTIES
    LINK_FLAGS "/SUBSYSTEM:WINDOWS"
)

target_link_libraries(hideous_hook PRIVATE common)
