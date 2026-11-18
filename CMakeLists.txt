cmake_minimum_required(VERSION 3.16)

project(CASHScheduler VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

add_executable(CASHScheduler
    qt/clajm.cpp
    src/calender.cpp
    src/Notification.cpp
    src/calendar.h
    src/eventClass.h
    src/Notification.h
)

target_include_directories(CASHScheduler PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

target_link_libraries(CASHScheduler PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/qt/styles.qss"
    "${CMAKE_CURRENT_BINARY_DIR}/styles.qss"
    COPYONLY
)
