#pragma once

#include <boost/asio.hpp>
#include <string>
#include <fstream>
#include <array>
#include <functional>

namespace mud
{
    class file {
    public:
        file(boost::asio::ip::tcp::socket& socket);
        ~file();

        void send_file(const std::string& file_path, std::function<void(bool)> callback);
        void receive_file(const std::string& file_name, std::size_t file_size, std::function<void(bool)> callback);

    private:
        void do_write_file_content(std::function<void(bool)> callback);
        void do_read_file_content(std::size_t remaining_size, std::function<void(bool)> callback);

        boost::asio::ip::tcp::socket& socket_;
        std::ifstream input_file_;
        std::ofstream output_file_;
        std::array<char, 4096> buffer_;
    };
} // namespace mud
