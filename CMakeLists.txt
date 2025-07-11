cmake_minimum_required(VERSION 3.16)
project(CardQt VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS 
    Widgets 
    Network 
    Core
    Gui
)

set(PROJECT_SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.h
    welcomewindow.cpp
    welcomewindow.h
    capturewindow.cpp
    capturewindow.h
    capturesettingsdialog.cpp
    capturesettingsdialog.h
    dimensionsdialog.cpp
    dimensionsdialog.h
    imagestitcher.cpp
    imagestitcher.h
    defectdetector.cpp
    defectdetector.h
    cuttingconfigdialog.cpp
    cuttingconfigdialog.h
    cuttingwindow.cpp
    cuttingwindow.h
    cuttinganalyzer.cpp
    cuttinganalyzer.h
)

qt_add_executable(CardQt
    ${PROJECT_SOURCES}
)

target_link_libraries(CardQt PRIVATE
    Qt6::Widgets
    Qt6::Network
    Qt6::Core
    Qt6::Gui
)

set_target_properties(CardQt PROPERTIES
    WIN32_EXECUTABLE ON
    MACOSX_BUNDLE ON)

# Copy Python script to build directory
file(COPY ${CMAKE_SOURCE_DIR}/scripts/defect_detector.py
     DESTINATION ${CMAKE_BINARY_DIR}/scripts)

# Create model directory in build directory
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/model)

# Copy model file to build directory if it exists
if(EXISTS "${CMAKE_SOURCE_DIR}/model/best.pt")
    file(COPY ${CMAKE_SOURCE_DIR}/model/best.pt
         DESTINATION ${CMAKE_BINARY_DIR}/model)
else()
    message(WARNING "Model file not found at ${CMAKE_SOURCE_DIR}/model/best.pt")
endif() 