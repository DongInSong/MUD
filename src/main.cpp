#include "network/server.hpp"
#include <boost/asio.hpp>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <windows.h>

using boost::asio::ip::tcp;

void enable_ansi_escape_codes() {
  HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
  if (hOut == INVALID_HANDLE_VALUE) {
    return;
  }
  DWORD dwMode = 0;
  if (!GetConsoleMode(hOut, &dwMode)) {
    return;
  }
  dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
  if (!SetConsoleMode(hOut, dwMode)) {
    return;
  }
}

int main(int argc, char *argv[]) {
  enable_ansi_escape_codes();
  try {
    if (argc != 2) {
      std::cerr << "Usage: mud_server <port>\n";
      return 1;
    }

    std::filesystem::path exe_path(argv[0]);
    std::filesystem::path data_path = exe_path.parent_path() / "data";

    boost::asio::io_context io_context;
    tcp::endpoint endpoint(tcp::v4(), std::atoi(argv[1]));
    mud::server s(io_context, endpoint, data_path.string());
    std::cout << "\033[1;32mServer started " << endpoint.address().to_string() << ":" << endpoint.port() << "\033[0m" << std::endl;
    s.run();
  } catch (const std::exception &e) {
    std::cerr << "Exception: " << e.what() << "\n";
  }
  return 0;
}
