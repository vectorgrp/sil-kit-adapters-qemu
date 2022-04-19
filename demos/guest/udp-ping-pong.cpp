
#include <array>

#include <cerrno>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <cinttypes>

extern "C" {
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
} // extern "C"

namespace {

} // namespace

int main() {
  int socket_fd = ::socket(AF_INET, SOCK_DGRAM, 0);
  if (socket_fd < 0) {
    std::fprintf(stderr, "could not open socket (%d): %s\n", errno, std::strerror(errno));
    std::exit(EXIT_FAILURE);
  }

  struct sockaddr_in server_address = {};
  server_address.sin_family = AF_INET;
  server_address.sin_addr.s_addr = INADDR_ANY;
  server_address.sin_port = htons(12345);

  if (::bind(socket_fd, (const struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
    std::fprintf(stderr, "could not bind socket (%d): %s\n", errno, std::strerror(errno));
    std::exit(EXIT_FAILURE);
  }

  std::array<std::uint8_t, 4096> buffer = {};

  while (true) {
    struct sockaddr_in client_address = {};
    socklen_t client_address_len = sizeof(client_address);

    ssize_t recv_byte_count = ::recvfrom(socket_fd, buffer.data(), buffer.size(), 0, (struct sockaddr *)&client_address, &client_address_len);
    if (recv_byte_count < 0) {
      std::fprintf(stderr, "error during recvfrom (%d): %s\n", errno, std::strerror(errno));
      std::exit(EXIT_FAILURE);
    }

    std::fprintf(stdout, "received %" PRIiMAX " bytes\n", static_cast<std::intmax_t>(recv_byte_count));

    ssize_t send_byte_count = ::sendto(socket_fd, buffer.data(), recv_byte_count, 0, (struct sockaddr *)&client_address, client_address_len);
    if (send_byte_count < 0) {
      std::fprintf(stderr, "error during sendto (%d): %s\n", errno, std::strerror(errno));
      std::exit(EXIT_FAILURE);
    }

    std::fprintf(stdout, "sent %" PRIiMAX " bytes\n", static_cast<std::intmax_t>(send_byte_count));
  }

  ::close(socket_fd);
}
