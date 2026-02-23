cmake_minimum_required(VERSION 3.20)
project(lab2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


add_library(persistence
    Persister.cpp
)
# 链接 proto_generated 库
target_link_libraries(persistence PUBLIC proto_raftService)
target_include_directories(persistence PUBLIC .)