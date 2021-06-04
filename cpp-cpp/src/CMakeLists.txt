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

add_executable(router "router.cpp")
target_include_directories(router PUBLIC ${protos_DIR})
target_link_libraries(router msg_protos)

add_executable(dealer "dealer.cpp")
target_include_directories(dealer PUBLIC ${protos_DIR})
target_link_libraries(dealer msg_protos)

add_executable(router_dealer "rtrdlr.cpp")
target_include_directories(router_dealer PUBLIC ${protos_DIR})
target_link_libraries(router_dealer msg_protos)

add_executable(pubsub_multithread "pubsub_multithread_inproc.cpp")
target_link_libraries(pubsub_multithread my_msg_protos)
