cmake_minimum_required(VERSION 3.16)
project(StudentWorkspace VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 COMPONENTS Core Gui Widgets Sql Charts REQUIRED)

add_executable(StudentWorkspace
    main.cpp
    databasemanager.h databasemanager.cpp
    mainwindow.h mainwindow.cpp
    mainpagewidget.h mainpagewidget.cpp
    schedulewidget.h schedulewidget.cpp
    noteswidget.h noteswidget.cpp
    pomodorowidget.h pomodorowidget.cpp
    statisticswidget.h statisticswidget.cpp
    settingswidget.h settingswidget.cpp
    resources.qrc
    pomodoroglwidget.h pomodoroglwidget.cpp
)

target_link_libraries(StudentWorkspace PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Sql
    Qt6::Charts
)

if(MSVC)
    target_compile_options(StudentWorkspace PRIVATE /W4 /WX-)
endif()