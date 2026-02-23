cmake_minimum_required(VERSION 3.20)
project(lab3 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


add_library(raft 
    raft.cpp
    raftRole.hpp
)

add_subdirectory(rpc)
add_subdirectory(persistence)

target_link_libraries(raft PRIVATE proto_raftService)
target_link_libraries(raft PUBLIC rpc_client persistence rpc_server)

target_include_directories(raft PUBLIC .)


