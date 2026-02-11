cmake_minimum_required(VERSION 3.20)
project(lab2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


add_library(rpc
    AppendEntriesCall.cpp
    RequestVoteCall.cpp
    RaftRpcServer.cpp
)
# 链接 proto_generated 库
target_link_libraries(rpc PUBLIC proto_lab2)

target_link_libraries(rpc PRIVATE raft)
target_include_directories(rpc PUBLIC .)
