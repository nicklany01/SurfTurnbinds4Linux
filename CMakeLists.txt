cmake_minimum_required(VERSION 3.16)
project(SurfTurnbinds VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable Qt Meta-Object Compiler
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find Qt6, fallback to Qt5 if Qt6 is not present
find_package(Qt6 COMPONENTS Widgets QUIET)
if(NOT Qt6_FOUND)
    find_package(Qt5 COMPONENTS Widgets REQUIRED)
    set(QT_LIBRARIES Qt5::Widgets)
    message(STATUS "Using Qt5")
else()
    set(QT_LIBRARIES Qt6::Widgets)
    message(STATUS "Using Qt6")
endif()

set(SOURCES
    main.cpp
    MainWindow.cpp
    TurnEngine.cpp
)

set(HEADERS
    MainWindow.hpp
    TurnEngine.hpp
    VirtualMouse.hpp
    KeyNames.hpp
)

add_executable(surfturnbinds ${SOURCES} ${HEADERS})
target_link_libraries(surfturnbinds PRIVATE ${QT_LIBRARIES} pthread)
