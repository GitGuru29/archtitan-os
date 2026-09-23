cmake_minimum_required(VERSION 3.16)
project(titan-quicksettings VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick Qml DBus Network Widgets)

set(SOURCES
    src/main.cpp
    src/ipcserver.cpp
    src/systemcontroller.cpp
    src/networkcontroller.cpp
    src/bluetoothcontroller.cpp
    src/audiocontroller.cpp
    src/nightlightcontroller.cpp
    src/notificationserver.cpp
)

set(HEADERS
    src/ipcserver.h
    src/systemcontroller.h
    src/networkcontroller.h
    src/bluetoothcontroller.h
    src/audiocontroller.h
    src/nightlightcontroller.h
    src/notificationserver.h
)

set(RESOURCES
    resources.qrc
)

add_executable(titan-quicksettings
    ${SOURCES}
    ${HEADERS}
    ${RESOURCES}
)

target_link_libraries(titan-quicksettings PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::Qml
    Qt6::DBus
    Qt6::Network
    Qt6::Widgets
)

install(TARGETS titan-quicksettings DESTINATION /usr/local/bin)
