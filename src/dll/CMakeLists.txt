cmake_minimum_required(VERSION 3.15)

add_library(hideous_hook SHARED
    hideous_hook.cpp
    hideous_hook.h
    hideous_hook.def
    ../common/constants.h
    ../common/settings.h
    ../common/settings.cpp
    ../common/logging.h
    ../common/logging.cpp
)

target_compile_definitions(hideous_hook PRIVATE
    HIDEOUS_HOOK_EXPORTS
    UNICODE
    _UNICODE
)

set_target_properties(hideous_hook PROPERTIES
    LINK_FLAGS "/SUBSYSTEM:WINDOWS"
)
