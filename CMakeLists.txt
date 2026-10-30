cmake_minimum_required(VERSION 3.21)

project(Orbit VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.4 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
)

add_subdirectory(apps/shell)

# Add these once their CMake targets exist:
# add_subdirectory(apps/filesystem)
# add_subdirectory(apps/compiler)