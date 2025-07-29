
#include <iostream>
#include <network/server.hpp>

int main() {
  try {
    mud::server s("127.0.0.1", 8080);
    std::cout << "Server created with host: localhost and port: 8080"
              << std::endl;
    s.run();
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
  }
}