cmake_minimum_required(VERSION 3.16)
project(QtVSCodeApp VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

add_executable(QtVSCodeApp 
    clajm.cpp
    ../src/calendar.h
    ../src/calender.cpp
    ../src/eventClass.h
)

target_link_libraries(QtVSCodeApp PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets)
target_include_directories(QtVSCodeApp PRIVATE ../src)
