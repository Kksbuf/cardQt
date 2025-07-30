cmake_minimum_required(VERSION 3.16)
project(CardQt)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 COMPONENTS
    Core
    Gui
    Widgets
    Network
    SerialPort
    REQUIRED
)

add_executable(CardQt
    main.cpp
    welcomewindow.cpp
    welcomewindow.h
    mainwindow.cpp
    mainwindow.h
    capturewindow.cpp
    capturewindow.h
    motorizedcapturewindow.cpp
    motorizedcapturewindow.h
    motorizedcapturesettingsdialog.cpp
    motorizedcapturesettingsdialog.h
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

target_link_libraries(CardQt PRIVATE
    Qt::Core
    Qt::Gui
    Qt::Widgets
    Qt::Network
    Qt::SerialPort
)

# Copy Python script and model to build directory
file(COPY ${CMAKE_SOURCE_DIR}/scripts DESTINATION ${CMAKE_BINARY_DIR})
file(COPY ${CMAKE_SOURCE_DIR}/model DESTINATION ${CMAKE_BINARY_DIR}) 