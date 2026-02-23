cmake_minimum_required(VERSION 3.20)
project(lab3 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(raft)
add_subdirectory(rpc)

add_library(KvServer
    KvServer.cpp
)
target_link_libraries(KvServer PRIVATE proto_kvService kvRpc raft)
target_include_directories(KvServer PUBLIC .)

add_executable(main main.cpp)
target_link_libraries(main PRIVATE raft)