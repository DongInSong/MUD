#include "network/file.hpp"
#include <iomanip>
#include <iostream>


namespace mud {
file::file(boost::asio::ip::tcp::socket &socket) : socket_(socket) {}

file::~file() {}

void file::send_file(const std::string &file_path,
                     std::function<void(bool)> callback) {
  input_file_.open(file_path, std::ios_base::binary | std::ios_base::ate);
  if (!input_file_.is_open()) {
    std::cerr << "Failed to open file: " << file_path << std::endl;
    callback(false);
    return;
  }

  std::streamsize file_size = input_file_.tellg();
  input_file_.seekg(0, std::ios_base::beg);

  std::string header =
      "file:" + file_path + ":" + std::to_string(file_size) + "\n";
  boost::asio::async_write(
      socket_, boost::asio::buffer(header),
      [this, callback](const boost::system::error_code &error,
                       std::size_t /*bytes_transferred*/) {
        if (!error) {
          do_write_file_content(callback);
        } else {
          std::cerr << "Send file header error: " << error.message()
                    << std::endl;
          callback(false);
        }
      });
}

void file::receive_file(const std::string &file_name, std::size_t file_size,
                        std::function<void(bool)> callback) {
  output_file_.open(file_name, std::ios_base::binary);
  if (!output_file_.is_open()) {
    std::cerr << "Failed to open file for writing: " << file_name << std::endl;
    callback(false);
    return;
  }
  do_read_file_content(file_size, callback);
}

void file::do_write_file_content(std::function<void(bool)> callback) {
  input_file_.read(buffer_.data(), buffer_.size());
  std::size_t bytes_read = input_file_.gcount();

  if (bytes_read > 0) {
    boost::asio::async_write(
        socket_, boost::asio::buffer(buffer_.data(), bytes_read),
        [this, callback](const boost::system::error_code &error,
                         std::size_t /*bytes_transferred*/) {
          if (!error) {
            do_write_file_content(callback);
          } else {
            std::cerr << "Send file content error: " << error.message()
                      << std::endl;
            callback(false);
          }
        });
  } else {
    callback(true);
  }
}

void file::do_read_file_content(std::size_t remaining_size,
                                std::function<void(bool)> callback) {
  if (remaining_size == 0) {
    callback(true);
    return;
  }

  socket_.async_read_some(
      boost::asio::buffer(buffer_),
      [this, remaining_size, callback](const boost::system::error_code &error,
                                       std::size_t bytes_transferred) {
        if (!error) {
          output_file_.write(buffer_.data(), bytes_transferred);

          std::cout << "\rDownloading... " << std::fixed << std::setprecision(2)
                    << (double)(remaining_size - bytes_transferred) /
                           remaining_size * 100
                    << "%" << std::flush;

          do_read_file_content(remaining_size - bytes_transferred, callback);
        } else {
          std::cerr << "Receive file content error: " << error.message()
                    << std::endl;
          callback(false);
        }
      });
}
} // namespace mud
