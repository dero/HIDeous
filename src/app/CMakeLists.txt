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

# Copy settings.ini to the output directory
add_custom_command(TARGET Hideous POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_SOURCE_DIR}/src/assets/settings.ini"
    "$<TARGET_FILE_DIR:Hideous>/settings.ini"
)
