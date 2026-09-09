cmake_minimum_required(VERSION 3.22)
project(archtitan-settings VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Qt6 6.4 REQUIRED COMPONENTS
    Core
    Quick
    QuickControls2
    DBus
    Network
    Concurrent
)

qt_add_executable(archtitan-settings
    src/main.cpp
    src/settingsbackend.cpp
    src/settingsbackend.h
    src/networkmanager.cpp
    src/networkmanager.h
    src/systeminfo.cpp
    src/systeminfo.h
    src/displaymanager.cpp
    src/displaymanager.h
    src/audiobackend.cpp
    src/audiobackend.h
    src/wallpapermanager.cpp
    src/wallpapermanager.h
)

qt_add_qml_module(archtitan-settings
    URI ArchTitanSettings
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/components/SidebarItem.qml
        qml/components/SettingsCard.qml
        qml/components/TitanSwitch.qml
        qml/components/TitanSlider.qml
        qml/components/TitanButton.qml
        qml/components/SectionHeader.qml
        qml/components/StatusBadge.qml
        qml/pages/AppearancePage.qml
        qml/pages/NetworkPage.qml
        qml/pages/DisplayPage.qml
        qml/pages/AudioPage.qml
        qml/pages/SystemPage.qml
        qml/pages/SecurityPage.qml
        qml/pages/PowerPage.qml
        qml/pages/AboutPage.qml
    RESOURCES
        assets/icons/appearance.svg
        assets/icons/network.svg
        assets/icons/display.svg
        assets/icons/audio.svg
        assets/icons/system.svg
        assets/icons/security.svg
        assets/icons/power.svg
        assets/icons/about.svg
        assets/icons/cpu.svg
        assets/icons/ram.svg
        assets/icons/disk.svg
        assets/icons/gpu.svg
        assets/icons/hostname.svg
        assets/icons/kernel.svg
        assets/icons/uptime.svg
        assets/icons/os.svg
        assets/icons/LOGO.png
        assets/icons/powersaving.png
        assets/icons/balanced.png
        assets/icons/performance_nobg.png
)

target_link_libraries(archtitan-settings PRIVATE
    Qt6::Core
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::DBus
    Qt6::Network
    Qt6::Concurrent
)

target_include_directories(archtitan-settings PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

install(TARGETS archtitan-settings
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
