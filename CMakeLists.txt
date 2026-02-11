cmake_minimum_required(VERSION 3.20)
project(kvCache LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS True)
# 引入自定义 cmake 模块
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(protobuf_grpc)

# 生成 proto/grpc 代码并建库

# lab1
# add_proto_library(
#   TARGET proto_lab1
#   PROTO_DIR ${CMAKE_CURRENT_SOURCE_DIR}/proto
#   PROTOS
#     lab1/lab1.proto
#   OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated
# )
# add_subdirectory(lab1)

# lab2
add_proto_library(
  TARGET proto_lab2
  PROTO_DIR ${CMAKE_CURRENT_SOURCE_DIR}/proto
  PROTOS
    lab2/lab2.proto
  OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated
)
add_subdirectory(lab2)


# add_executable(server server.cc)
# target_link_libraries(server PRIVATE proto_generated)

# add_executable(client client.cc)
# target_link_libraries(client PRIVATE proto_generated)
