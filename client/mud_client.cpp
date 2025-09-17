#include <boost/asio.hpp>
#include <iostream>
// #include <ncurses.h>
#include <deque>
#include <mutex>
#include <string>
#include <vector>
#include <windows.h>


// --- Console UI Handling ---
std::mutex console_mutex;
std::vector<std::string> message_log;
std::string current_input_global; // 전역 변수로 변경
std::atomic<bool> connected{false};
const int MAX_LOG_SIZE = 20;

void clear_screen() {
  HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
  COORD coordScreen = {0, 0};
  DWORD cCharsWritten;
  CONSOLE_SCREEN_BUFFER_INFO csbi;
  DWORD dwConSize;

  GetConsoleScreenBufferInfo(hConsole, &csbi);
  dwConSize = csbi.dwSize.X * csbi.dwSize.Y;
  FillConsoleOutputCharacter(hConsole, (TCHAR)' ', dwConSize, coordScreen,
                             &cCharsWritten);
  GetConsoleScreenBufferInfo(hConsole, &csbi);
  FillConsoleOutputAttribute(hConsole, csbi.wAttributes, dwConSize, coordScreen,
                             &cCharsWritten);
  SetConsoleCursorPosition(hConsole, coordScreen);
}

void redraw_screen() {
  std::lock_guard<std::mutex> lock(console_mutex);
  clear_screen();

  HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
  CONSOLE_SCREEN_BUFFER_INFO csbi;
  GetConsoleScreenBufferInfo(hConsole, &csbi);

  int log_start_line = 0;
  for (const auto &msg : message_log) {
    COORD pos = {0, (SHORT)log_start_line++};
    SetConsoleCursorPosition(hConsole, pos);
    // 각 메시지를 별도의 줄에 출력
    std::cout << msg << std::endl;
  }

  COORD input_pos = {0, (SHORT)(csbi.dwSize.Y - 1)};
  SetConsoleCursorPosition(hConsole, input_pos);
  std::cout << "> " << current_input_global;
}

void add_message(const std::string &msg) {
  std::lock_guard<std::mutex> lock(console_mutex);
  message_log.push_back(msg);
  if (message_log.size() > MAX_LOG_SIZE) {
    message_log.erase(message_log.begin());
  }
}

// --- Network Client ---
class client {
public:
  client(boost::asio::io_context &io_context, const std::string &host,
         const std::string &port)
      : io_context_(io_context), socket_(io_context) {
    boost::asio::ip::tcp::resolver resolver(io_context_);
    endpoints_ = resolver.resolve(host, port);
  }

  void start() {
    boost::asio::async_connect(
        socket_, endpoints_,
        [this](const boost::system::error_code &ec,
               const boost::asio::ip::tcp::endpoint &endpoint) {
          if (!ec) {
            do_read();
          } else {
            add_message("Connect error: " + ec.message());
          }
        });
  }

  void write(const std::string &msg) {
    boost::asio::post(io_context_, [this, msg]() {
      bool write_in_progress = !write_msgs_.empty();
      write_msgs_.push_back(msg + "\n");
      if (!write_in_progress) {
        do_write();
      }
    });
  }

  void close() {
    boost::asio::post(io_context_, [this]() { socket_.close(); });
  }

private:
  void do_read() {

    boost::asio::async_read_until(
        socket_, read_buffer_, '\n',
        [this](const boost::system::error_code &ec, std::size_t length) {
          if (ec == boost::asio::error::eof ||
              ec == boost::asio::error::connection_reset) {
            add_message("\033[1;31mDisconnected from server. (/quit) \033[0m");
            redraw_screen();
            connected = false;
            return;
          }
          if (!ec) {
            std::istream is(&read_buffer_);
            std::string line;
            std::getline(is, line);
            if (!line.empty() && line.back() == '\r') {
              line.pop_back();
            }
            add_message(line);
            redraw_screen(); // 현재 입력 내용 유지하며 다시 그리기
            do_read();
          } else {
            if (ec != boost::asio::error::eof) {
              add_message("Read error: " + ec.message());
            }
            close();
          }
        });
  }

  void do_write() {
    boost::asio::async_write(
        socket_, boost::asio::buffer(write_msgs_.front()),
        [this](const boost::system::error_code &ec, std::size_t /*length*/) {
          if (!ec) {
            write_msgs_.pop_front();
            if (!write_msgs_.empty()) {
              do_write();
            }
          } else {
            add_message("Write error: " + ec.message());
            close();
          }
        });
  }

  boost::asio::io_context &io_context_;
  boost::asio::ip::tcp::socket socket_;
  boost::asio::ip::tcp::resolver::results_type endpoints_;
  boost::asio::streambuf read_buffer_;
  std::deque<std::string> write_msgs_;
};

int main(int argc, char *argv[]) {
  SetConsoleOutputCP(CP_UTF8);
  SetConsoleCP(CP_UTF8);
  try {
    if (argc != 3) {
      std::cerr << "Usage: mud_client <host> <port>\n";
      return 1;
    }

    boost::asio::io_context io_context;
    client c(io_context, argv[1], argv[2]);

    std::thread t([&io_context]() {
      try {
        io_context.run();
      } catch (const std::exception &e) {
        add_message("Exception in thread: " + std::string(e.what()));
      }
    });

    c.start();

    redraw_screen();

    while (true) {
      char ch = std::cin.get();
      {
        std::lock_guard<std::mutex> lock(console_mutex);
        if (ch == '\n' || ch == '\r') {
          if (current_input_global == "/quit") {
            break;
          }
          c.write(current_input_global);
          current_input_global.clear();
        } else if (ch == '\b' && !current_input_global.empty()) {
          current_input_global.pop_back();
        } else {
          current_input_global += ch;
        }
      }
      redraw_screen();
    }

    c.close();
    t.join();
  } catch (std::exception &e) {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
