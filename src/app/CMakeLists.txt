cmake_minimum_required(VERSION 3.15)

add_executable(hideous_app WIN32
    ../common/shared.h
    ../common/shared.cpp
    settings.h
    settings.cpp
    hideous_app.h
    hideous_app.cpp
    ../common/crypto.h
    ../common/crypto.cpp
    ../common/logging.h
    ../common/logging.cpp
)

target_link_libraries(hideous_app
    hideous_hook
)
