cmake_minimum_required(VERSION 3.15)

add_executable(Hideous WIN32
    resource.h
    hideous_app.h
    hideous_app.cpp
    gui.h
    gui.cpp
    intercept.h
    intercept.cpp
    ../common/constants.h
    ../common/crypto.h
    ../common/crypto.cpp
    ../common/logging.h
    ../common/logging.cpp
    ../common/settings.h
    ../common/settings.cpp
    resources.rc
)

target_link_libraries(Hideous
    hideous_hook
)
