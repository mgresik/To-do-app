cmake_minimum_required(VERSION 3.14)
project(TodoTracker VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(todo
    main.cpp
    task.cpp
    task_manager.cpp
)

target_include_directories(todo PRIVATE src)