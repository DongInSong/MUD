#include <boost/asio.hpp>
#include <memory>
#include <thread>
#include "network/session.hpp"

namespace mud {
    class server {
    public:
        server(const std::string& host, short port);
        void run();
        void stop();

    private:
        void start_accept();
        void handle_accept(boost::asio::ip::tcp::socket &&new_connection, const boost::system::error_code &error);
        boost::asio::io_context io_context_;
        boost::asio::ip::tcp::acceptor acceptor_;
        std::vector<std::thread> thread_pool_;
    };
}
