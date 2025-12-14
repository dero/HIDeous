cmake_minimum_required(VERSION 3.15)
project(HIDeous VERSION 0.12.0)

# Set output directory for executables and DLLs
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)


# Enable Unicode in Windows
add_definitions(-DUNICODE -D_UNICODE)

# Add version definitions that can be used in code
add_definitions(
    -DPROJECT_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    -DPROJECT_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    -DPROJECT_VERSION_PATCH=${PROJECT_VERSION_PATCH}
)

# Add subdirectories
add_subdirectory(src/dll)
add_subdirectory(src/app)
