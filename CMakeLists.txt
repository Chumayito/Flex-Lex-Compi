cmake_minimum_required(VERSION 3.16)
project(RegexScannerGenerator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Intenta primero Qt6 y, si no está disponible, usa Qt5.
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

add_executable(RegexScannerGenerator
    src/main.cpp
    src/mainwindow.h
    src/mainwindow.cpp
    src/regexengine.h
    src/regexengine.cpp
    src/token.h
    src/scanner.h
)

target_link_libraries(RegexScannerGenerator PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

# --- Objetivo opcional: pruebas del motor sin Qt ---
add_executable(test_engine EXCLUDE_FROM_ALL
    src/test_engine.cpp
    src/regexengine.h
    src/regexengine.cpp
)
