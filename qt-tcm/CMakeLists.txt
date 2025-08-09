cmake_minimum_required(VERSION 3.21)
project(qt_tcm_prescription VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Sql)

add_executable(qt_tcm
    src/main.cpp
    src/MainWindow.h
    src/MainWindow.cpp
)

target_link_libraries(qt_tcm PRIVATE Qt6::Widgets Qt6::Sql)

if(WIN32)
  add_compile_definitions(NOMINMAX)
endif()