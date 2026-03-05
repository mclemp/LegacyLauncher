cmake_minimum_required(VERSION 3.21)

project(MinecraftLegacyLauncher VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Network)

add_executable(MinecraftLegacyLauncher
    src/main.cpp
    src/MainWindow.h
    src/MainWindow.cpp
    src/services/ProfileStore.h
    src/services/ProfileStore.cpp
    src/services/InstallService.h
    src/services/InstallService.cpp
    src/services/ReleaseClient.h
    src/services/ReleaseClient.cpp
    resources/resources.qrc
)

target_link_libraries(MinecraftLegacyLauncher PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Network)

if(WIN32)
    set_target_properties(MinecraftLegacyLauncher PROPERTIES WIN32_EXECUTABLE TRUE)

    if(QT_VERSION_MAJOR EQUAL 6)
        set(_qt_qmake_target Qt6::qmake)
    else()
        set(_qt_qmake_target Qt5::qmake)
    endif()

    if(TARGET ${_qt_qmake_target})
        get_target_property(_qt_qmake_executable ${_qt_qmake_target} IMPORTED_LOCATION)
        get_filename_component(_qt_bin_dir "${_qt_qmake_executable}" DIRECTORY)
        find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${_qt_bin_dir}")

        if(WINDEPLOYQT_EXECUTABLE)
            add_custom_command(TARGET MinecraftLegacyLauncher POST_BUILD
                COMMAND "${WINDEPLOYQT_EXECUTABLE}"
                    "$<IF:$<CONFIG:Debug>,--debug,--release>"
                    --compiler-runtime
                    --no-translations
                    "$<TARGET_FILE:MinecraftLegacyLauncher>"
                COMMENT "Deploying Qt runtime dependencies with windeployqt"
                VERBATIM
            )
        else()
            message(WARNING "windeployqt not found. Automatic Qt runtime deployment is disabled.")
        endif()
    endif()
endif()
