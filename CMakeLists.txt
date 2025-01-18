cmake_minimum_required(VERSION 3.15)
project(HIDeous)

# Enable Unicode in Windows
add_definitions(-DUNICODE -D_UNICODE)

# Add subdirectories
add_subdirectory(src/dll)
add_subdirectory(src/app)
