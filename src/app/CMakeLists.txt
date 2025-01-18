cmake_minimum_required(VERSION 3.15)

add_executable(hideous_app WIN32
    hideous_app.cpp
)

target_link_libraries(hideous_app
    hideous_hook
)
