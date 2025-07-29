#pragma once

#include <boost/asio.hpp>
#include <memory>

namespace mud {
    class session : public std::enable_shared_from_this<session> {
    public:
        session(boost::asio::ip::tcp::socket socket);
        void start();
        ~session();

    private:
        void do_read();
        void do_write(std::size_t length);

        boost::asio::ip::tcp::socket socket_;
        enum { max_length = 1024 };
        char data_[max_length];
    };
}
