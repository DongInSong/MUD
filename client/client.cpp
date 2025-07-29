#include <iostream>
#include <boost/asio.hpp>

using boost::asio::ip::tcp;

int main(int argc, char* argv[])
{
  try
  {
    // if (argc != 3)
    // {
    //   std::cerr << "Usage: client <host> <port>\n";
    //   return 1;
    // }

    boost::asio::io_context io_context;

    tcp::socket s(io_context);
    tcp::resolver resolver(io_context);
    // boost::asio::connect(s, resolver.resolve(argv[1], argv[2]));

    // std::cout << "Connected to " << argv[1] << ":" << argv[2] << std::endl;

    boost::asio::connect(s, resolver.resolve("localhost", "8080"));
    std::cout << "Connected to localhost:8080" << std::endl;
    for (;;)
    {
      char request[1024];
      std::cout << "Enter message: ";
      std::cin.getline(request, 1024);
      size_t request_length = std::strlen(request);
      boost::asio::write(s, boost::asio::buffer(request, request_length));

      char reply[1024];
      size_t reply_length = boost::asio::read(s,
          boost::asio::buffer(reply, request_length));
      std::cout << "Reply: ";
      std::cout.write(reply, reply_length);
      std::cout << "\n";
    }
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
