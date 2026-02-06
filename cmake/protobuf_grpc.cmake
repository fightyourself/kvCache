function(add_proto_library)
  set(options)
  set(oneValueArgs TARGET PROTO_DIR OUT_DIR)
  set(multiValueArgs PROTOS)
  cmake_parse_arguments(APL "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if(NOT APL_TARGET)
    message(FATAL_ERROR "add_proto_library: TARGET is required")
  endif()

  if(NOT APL_PROTO_DIR)
    message(FATAL_ERROR "add_proto_library: PROTO_DIR is required")
  endif()

  if(NOT APL_PROTOS)
    message(FATAL_ERROR "add_proto_library: PROTOS is required")
  endif()

  find_package(Protobuf CONFIG REQUIRED)
  find_package(gRPC CONFIG REQUIRED)
  find_package(Threads REQUIRED)

  set(_PROTOC $<TARGET_FILE:protobuf::protoc>)
  set(_GRPC_CPP_PLUGIN $<TARGET_FILE:gRPC::grpc_cpp_plugin>)

  if(APL_OUT_DIR)
    set(_OUT_DIR "${APL_OUT_DIR}")
  else()
    set(_OUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
  endif()
  file(MAKE_DIRECTORY "${_OUT_DIR}")

  set(_GEN_SRCS "")
  set(_GEN_HDRS "")

  foreach(_proto_rel IN LISTS APL_PROTOS)
    # 强制 proto 使用“相对 PROTO_DIR 的路径”
    set(_proto_abs "${APL_PROTO_DIR}/${_proto_rel}")

    get_filename_component(_proto_rel_dir "${_proto_rel}" DIRECTORY)
    get_filename_component(_proto_name_we "${_proto_rel}" NAME_WE)

    if(_proto_rel_dir STREQUAL "")
      set(_out_subdir "${_OUT_DIR}")
    else()
      set(_out_subdir "${_OUT_DIR}/${_proto_rel_dir}")
      file(MAKE_DIRECTORY "${_out_subdir}")
    endif()

    list(APPEND _GEN_SRCS
      "${_out_subdir}/${_proto_name_we}.pb.cc"
      "${_out_subdir}/${_proto_name_we}.grpc.pb.cc"
    )
    list(APPEND _GEN_HDRS
      "${_out_subdir}/${_proto_name_we}.pb.h"
      "${_out_subdir}/${_proto_name_we}.grpc.pb.h"
    )

    add_custom_command(
      OUTPUT
        ${_out_subdir}/${_proto_name_we}.pb.cc
        ${_out_subdir}/${_proto_name_we}.pb.h
        ${_out_subdir}/${_proto_name_we}.grpc.pb.cc
        ${_out_subdir}/${_proto_name_we}.grpc.pb.h
      COMMAND "${_PROTOC}"
      ARGS
        --proto_path=${APL_PROTO_DIR}
        --cpp_out=${_OUT_DIR}
        --grpc_out=${_OUT_DIR}
        --plugin=protoc-gen-grpc=${_GRPC_CPP_PLUGIN}
        ${_proto_rel}
      DEPENDS ${_proto_abs}
      COMMENT "protoc: ${_proto_rel}"
      VERBATIM
    )
  endforeach()

  add_library(${APL_TARGET} STATIC ${_GEN_SRCS} ${_GEN_HDRS})

  target_include_directories(${APL_TARGET} PUBLIC "${_OUT_DIR}")

  target_link_libraries(${APL_TARGET}
    PUBLIC
      protobuf::libprotobuf
      gRPC::grpc++
      Threads::Threads
  )

  set(${APL_TARGET}_GENERATED_DIR "${_OUT_DIR}" PARENT_SCOPE)
endfunction()
