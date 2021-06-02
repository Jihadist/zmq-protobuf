cmake_minimum_required(VERSION 3.14)
project(cpp)

add_subdirectory(proto)

add_executable(pub "publisher.cpp")
target_include_directories(pub PUBLIC ${protos_DIR})
target_link_libraries(pub  msg_protos)

add_executable(sub "subscriber.cpp")
target_include_directories(sub PUBLIC ${protos_DIR})
target_link_libraries(sub  msg_protos)

add_executable(my_pub "my_publisher.cpp")
target_include_directories(my_pub PUBLIC ${protos_DIR})
target_link_libraries(my_pub my_msg_protos)

add_executable(my_sub "my_subscriber.cpp")
target_include_directories(my_sub PUBLIC ${protos_DIR})
target_link_libraries(my_sub my_msg_protos)
