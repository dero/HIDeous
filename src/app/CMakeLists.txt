cmake_minimum_required(VERSION 3.15)

add_executable(hideous_app WIN32
    ../common/shared_data.h
    ../common/shared_data.cpp
    hideous_app.h
    hideous_app.cpp
    ../common/log_util.h
    ../common/log_util.cpp
)

target_link_libraries(hideous_app
    hideous_hook
)
