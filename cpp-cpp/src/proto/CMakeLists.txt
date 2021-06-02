cmake_minimum_required(VERSION 3.14)
project(protos)
conan_basic_setup(TARGETS)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(${PROJECT_NAME}_DIR ${CMAKE_CURRENT_BINARY_DIR} CACHE PATH "GRPC generated files" FORCE)

get_filename_component(msg_proto "${CMAKE_CURRENT_SOURCE_DIR}/msg.proto" ABSOLUTE)
get_filename_component(my_msg_proto "${CMAKE_CURRENT_SOURCE_DIR}/my_msg.proto" ABSOLUTE)

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${msg_proto})
add_library(msg_${PROJECT_NAME} ${PROTO_HDRS} ${PROTO_SRCS})
target_compile_definitions(msg_${PROJECT_NAME} PUBLIC _GLIBCXX_USE_CXX11_ABI=0)
target_link_libraries(msg_${PROJECT_NAME}
  PUBLIC CONAN_PKG::protobuf
  PUBLIC CONAN_PKG::zeromq
  )
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${my_msg_proto})
add_library(my_msg_${PROJECT_NAME} ${PROTO_HDRS} ${PROTO_SRCS})
target_compile_definitions(my_msg_${PROJECT_NAME} PUBLIC _GLIBCXX_USE_CXX11_ABI=0)
target_link_libraries(my_msg_${PROJECT_NAME}
  PUBLIC CONAN_PKG::protobuf
  PUBLIC CONAN_PKG::zeromq
  )

#PROTOBUF_GENERATE_CPP(MY_PROTO_SRC MY_PROTO_HEADER my_msg.proto)
#ADD_LIBRARY(my_proto ${MY_PROTO_HEADER} ${MY_PROTO_SRC})
