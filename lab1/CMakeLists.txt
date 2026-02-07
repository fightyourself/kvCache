cmake_minimum_required(VERSION 3.20)
project(lab1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(map_reduce map_reduceFunc.cpp)

# 添加源文件
add_executable(master master.cpp) 
add_executable(worker worker.cpp)
# 链接 proto_generated 库
target_link_libraries(master PRIVATE proto_lab1)
target_link_libraries(worker PRIVATE proto_lab1 map_reduce)
