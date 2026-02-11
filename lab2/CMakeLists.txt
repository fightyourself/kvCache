cmake_minimum_required(VERSION 3.20)
project(lab2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


add_library(raft 
    raft.cpp
    raftRole.hpp
    PeerClient.hpp
)
target_link_libraries(raft PRIVATE proto_lab2)

add_subdirectory(rpc)

target_include_directories(raft PUBLIC .)


add_executable(main main.cpp)
target_link_libraries(main PRIVATE raft rpc)

