cmake_minimum_required(VERSION 3.22)
project(titanbrowser VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS
    Core Widgets WebEngineWidgets WebEngineCore Svg)

qt_add_executable(titanbrowser
    src/main.cpp
    src/browser.cpp
    src/browser.h
    src/tabwidget.cpp
    src/tabwidget.h
    src/addressbar.cpp
    src/addressbar.h
    resources/resources.qrc
)

target_link_libraries(titanbrowser PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::WebEngineWidgets
    Qt6::WebEngineCore
    Qt6::Svg
)

install(TARGETS titanbrowser DESTINATION /usr/local/bin)
install(FILES resources/titanbrowser.desktop DESTINATION /usr/share/applications)
