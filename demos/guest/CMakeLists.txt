
if (UNIX)
    add_executable(udp-ping-pong udp-ping-pong.cpp)
    target_link_libraries(udp-ping-pong "-static")
endif()
