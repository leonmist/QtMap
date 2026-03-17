cmake_minimum_required(VERSION 3.16)

project(ViewDemo3 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt5 REQUIRED COMPONENTS
  Core
  Gui
  Widgets
  Network
  3DCore
  3DRender
  3DExtras
  3DInput
  3DLogic
)

add_executable(${PROJECT_NAME}
  controlpanel.cpp
  coordinateconverter.cpp
  customlineentity.cpp
  main.cpp
  mainwindow.cpp
  mapentity.cpp
  targetentity.cpp
  view3d.cpp

  controlpanel.h
  coordinateconverter.h
  customlineentity.h
  mainwindow.h
  mapentity.h
  targetentity.h
  view3d.h

  mainwindow.ui
)

target_compile_definitions(${PROJECT_NAME} PRIVATE QT_DEPRECATED_WARNINGS)

target_link_libraries(${PROJECT_NAME} PRIVATE
  Qt5::Core
  Qt5::Gui
  Qt5::Widgets
  Qt5::Network
  Qt5::3DCore
  Qt5::3DRender
  Qt5::3DExtras
  Qt5::3DInput
  Qt5::3DLogic
)

