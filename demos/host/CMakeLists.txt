
add_executable(qemu-ethernet-socket qemu-ethernet-socket.cpp)
target_link_libraries(qemu-ethernet-socket PRIVATE asio)

