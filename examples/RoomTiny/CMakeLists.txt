cmake_minimum_required(VERSION 3.0)

project(TinyRoom LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt5 REQUIRED COMPONENTS Quick Core Qml)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

qt5_add_resources(RESOURCES qml.qrc)

add_executable(${PROJECT_NAME} WIN32 main.cpp ${RESOURCES})

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt5::Core
    Qt5::Quick
    Qt5::Qml
)
