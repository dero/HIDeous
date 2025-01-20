cmake_minimum_required(VERSION 3.15)

add_executable(Hideous WIN32
    ../common/shared.h
    ../common/shared.cpp
    resource.h
    settings.h
    settings.cpp
    hideous_app.h
    hideous_app.cpp
    gui.h
    gui.cpp
    ../common/crypto.h
    ../common/crypto.cpp
    ../common/logging.h
    ../common/logging.cpp
    resources.rc
)

target_link_libraries(Hideous
    hideous_hook
)
