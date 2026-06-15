cmake_minimum_required(VERSION 3.16)
project(titanfetch VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

add_executable(titanfetch
    main.cpp
    cli.cpp
    gui.cpp
    sysinfo.cpp
)

target_link_libraries(titanfetch PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets)

