cmake_minimum_required(VERSION 3.15)

project(TaskManager LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_BUILD_TYPE Debug)

add_executable(TaskManager 
main.cpp
processParser.cpp
ServiceManager.cpp
StartupManager.cpp
)



