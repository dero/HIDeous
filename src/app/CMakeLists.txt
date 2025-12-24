cmake_minimum_required(VERSION 3.15)

add_executable(Hideous WIN32
    resource.h
    entry_point.cpp
    ui/gui_types.h
    ui/main_window.h
    ui/main_window.cpp
    ui/taskbar_icon.h
    ui/taskbar_icon.cpp
    ui/components/device_list.h
    ui/components/device_list.cpp
    ui/components/profile_selector.h
    ui/components/profile_selector.cpp
    processing/raw_input_handler.h
    processing/raw_input_handler.cpp
    processing/macro_executor.h
    processing/macro_executor.cpp
    processing/commands/keys_command.h
    processing/commands/keys_command.cpp
    processing/commands/text_command.h
    processing/commands/text_command.cpp
    processing/commands/run_command.h
    processing/commands/run_command.cpp
    processing/commands/profile_command.h
    processing/commands/profile_command.cpp
    ../common/utils/constants.h
    resources.rc
)

target_link_libraries(Hideous
    hideous_hook
    common
)

# Copy settings.ini to the output directory
add_custom_command(TARGET Hideous POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_SOURCE_DIR}/src/assets/settings.ini"
    "$<TARGET_FILE_DIR:Hideous>/settings.ini"
)
