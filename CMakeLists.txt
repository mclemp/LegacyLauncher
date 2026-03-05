cmake_minimum_required(VERSION 3.21)

project(MinecraftLegacyLauncher VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

add_executable(MinecraftLegacyLauncher
    src/main.cpp
    src/MainWindow.h
    src/MainWindow.cpp
    resources/resources.qrc
)

target_link_libraries(MinecraftLegacyLauncher PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

if(WIN32)
    set_target_properties(MinecraftLegacyLauncher PROPERTIES WIN32_EXECUTABLE TRUE)
endif()
