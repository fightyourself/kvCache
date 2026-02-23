cmake_minimum_required(VERSION 3.20)
project(lab2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


add_library(rpc_server
    AppendEntriesCall.cpp
    RequestVoteCall.cpp
    RaftRpcServer.cpp
)
# 链接 proto_generated 库
target_link_libraries(rpc_server PUBLIC proto_raftService)

target_link_libraries(rpc_server PRIVATE raft)
target_include_directories(rpc_server PUBLIC .)


add_library(rpc_client
    AppendEntriesClientCall.cpp
    RequestVoteClientCall.cpp
    RaftRpcClient.cpp
    PeerClient.hpp
)

target_link_libraries(rpc_client PUBLIC proto_raftService)

target_link_libraries(rpc_client PRIVATE raft)
target_include_directories(rpc_client PUBLIC .)
