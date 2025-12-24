add_library(common STATIC
    config/settings_manager.cpp
    input/key_mapping.cpp
    utils/logging.cpp
    utils/crypto.cpp
    utils/string_utils.cpp
)

# Allow linking targets to find headers relative to 'src/common' if they add it to includepath,
# but our code uses relative paths mostly.
# We'll just expose the source dir.
target_include_directories(common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
