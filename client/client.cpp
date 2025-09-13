#include <boost/asio.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/streambuf.hpp>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <fstream>
#include <array>
#include <functional>
#include <iomanip>
#include <atomic>
#include <sstream>

using boost::asio::ip::tcp;

std::string hex_encode(const std::string &input)
{
  std::stringstream ss;
  ss << std::hex << std::setfill('0');
  for (unsigned char c : input)
  {
    ss << std::setw(2) << static_cast<unsigned int>(c);
  }
  return ss.str();
}

std::string hex_decode(const std::string &input)
{
  std::string output;
  if (input.length() % 2 != 0)
  {
    return "";
  }
  output.reserve(input.length() / 2);
  for (size_t i = 0; i < input.length(); i += 2)
  {
    std::string byte_str = input.substr(i, 2);
    char byte = static_cast<char>(std::stoul(byte_str, nullptr, 16));
    output.push_back(byte);
  }
  return output;
}

std::mutex cout_mutex;
std::atomic<bool> running = true;
std::atomic<bool> file_mode = false;
std::atomic<bool> file_offer_pending = false;
std::string pending_sender_endpoint;
std::string current_file_path;
std::string target_recipient_endpoint;
std::ofstream receiving_file;
std::size_t received_file_size = 0;
std::size_t total_file_size = 0;
std::string receiving_file_name;

void send_file_data(tcp::socket &socket, const std::string &file_path, const std::string &recipient_endpoint);

void read_handler(tcp::socket &socket)
{
  try
  {
    boost::asio::streambuf buffer;
    boost::system::error_code error;

    while (running)
    {
      size_t n = boost::asio::read_until(socket, buffer, '\n', error);
      if (error == boost::asio::error::eof)
        break;
      else if (error)
        throw boost::system::system_error(error);

      std::istream is(&buffer);
      std::string line;
      std::getline(is, line);

      if (line.rfind("file_offer:", 0) == 0)
      {
        std::stringstream ss(line);
        std::string command, file_name, file_size_str, sender_endpoint;
        std::getline(ss, command, ':');
        std::getline(ss, file_name, ':');
        std::getline(ss, file_size_str, ':');
        std::getline(ss, sender_endpoint, '\n');

        pending_sender_endpoint = sender_endpoint;
        file_offer_pending = true;

        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cout << "\r\033[K" << "Incoming file transfer offer for '" << file_name << "' (" << file_size_str << " bytes) from " << sender_endpoint << ". Accept? (yes/no)\nfile > " << std::flush;
      }
      else if (line.rfind("file_accepted:", 0) == 0)
      {
        std::stringstream ss(line);
        std::string command, recipient_endpoint;
        std::getline(ss, command, ':');
        std::getline(ss, recipient_endpoint, '\n');
        std::cout << "\r\033[K" << "File transfer accepted by " << recipient_endpoint << ". Starting transfer...\nfile > " << std::flush;
        send_file_data(socket, current_file_path, recipient_endpoint);
      }
      else if (line.rfind("file_declined:", 0) == 0)
      {
        std::stringstream ss(line);
        std::string command, recipient_endpoint;
        std::getline(ss, command, ':');
        std::getline(ss, recipient_endpoint, '\n');
        std::cout << "\r\033[K" << "File transfer declined by " << recipient_endpoint << ".\nfile > " << std::flush;
        file_mode = false;
      }
      else if (line.rfind("file_begin_transfer:", 0) == 0)
      {
        std::stringstream ss(line);
        std::string command, file_name, file_size_str;
        std::getline(ss, command, ':');
        std::getline(ss, file_name, ':');
        std::getline(ss, file_size_str, '\n');
        std::cout << "\r\033[K" << "File transfer starting. Receiving '" << file_name << "'.\nfile > " << std::flush;
        receiving_file_name = file_name;
        receiving_file.open(receiving_file_name, std::ios_base::binary);
        if (!receiving_file.is_open())
        {
          std::cerr << "Failed to create file for writing: " << receiving_file_name << std::endl;
          return;
        }
        total_file_size = std::stoull(file_size_str);
        received_file_size = 0;
      }
      else if (line.rfind("file_data:", 0) == 0)
      {
        if (receiving_file.is_open())
        {
          std::string hex_data = line.substr(std::string("file_data:").length());
          // std::cout << "\r\033[K" << "Receiving data chunk of size " << hex_data.length() / 2 << " bytes.\nfile > " << std::flush;
          // std::cout << "\r\033[K" << "Receiving hex data " << hex_data << "\nfile > " << std::flush;
          if (!hex_data.empty())
          {
            std::string decoded_data = hex_decode(hex_data);
            receiving_file.write(decoded_data.c_str(), decoded_data.length());
            received_file_size += decoded_data.length();
            if (total_file_size > 0)
            {
              double progress = (double)received_file_size / total_file_size * 100;
              std::lock_guard<std::mutex> lock(cout_mutex);
              std::cout << "\rDownloading " << receiving_file_name << "... " << std::fixed << std::setprecision(2) << progress << "%" << std::flush;
            }
          }
        }
      }
      else if (line.rfind("file_end:", 0) == 0)
      {
        if (receiving_file.is_open())
        {
          receiving_file.close();
          std::cout << "\nFile transfer complete.\nfile > " << std::flush;
        }
        file_mode = false;
        std::cout << "\r\033[K" << "message > " << std::flush;
      }
      else
      {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cout << "\r\033[K" << line << "\n"
                  << (file_mode ? "file > " : "message > ") << std::flush;
      }
    }
  }
  catch (std::exception &e)
  {
    std::cerr << "Exception in read thread: " << e.what() << "\n";
  }
}

void input_thread(tcp::socket &socket)
{
  try
  {
    std::string line;
    while (running)
    {
      {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cout << (file_mode ? "file > " : "message > ") << std::flush;
      }

      if (!std::getline(std::cin, line))
      {
        running = false;
        break;
      }

      if (file_offer_pending)
      {
        if (line == "yes")
        {
          boost::asio::write(socket, boost::asio::buffer("file_accept:" + pending_sender_endpoint + "\n"));
        }
        else
        {
          boost::asio::write(socket, boost::asio::buffer("file_decline:" + pending_sender_endpoint + "\n"));
        }
        file_offer_pending = false;
        file_mode = false;
        continue;
      }

      if (line == "file")
      {
        file_mode = true;
        continue;
      }

      if (file_mode)
      {
        if (line.rfind("send ", 0) == 0)
        {
          std::string temp = line.substr(5);
          size_t last_space = temp.rfind(' ');
          if (last_space != std::string::npos)
          {
            std::string file_path = temp.substr(0, last_space);
            std::string target_endpoint = temp.substr(last_space + 1);

            std::ifstream file(file_path, std::ios_base::binary | std::ios_base::ate);
            if (file.is_open())
            {
              std::streamsize file_size = file.tellg();
              std::string file_name = file_path.substr(file_path.find_last_of("/\\") + 1);
              current_file_path = file_path;
              target_recipient_endpoint = target_endpoint;
              boost::asio::write(socket, boost::asio::buffer("file_offer:" + target_endpoint + ":" + file_name + ":" + std::to_string(file_size) + "\n"));
            }
            else
            {
              std::lock_guard<std::mutex> lock(cout_mutex);
              std::cerr << "Failed to open file: " << file_path << std::endl;
            }
          }
          else
          {
            std::cout << "Usage: send <file_path> <target_endpoint>\n";
          }
          continue;
        }
      }

      if (line == "exit")
      {
        if (file_mode)
        {
          file_mode = false;
          std::lock_guard<std::mutex> lock(cout_mutex);
          std::cout << "Exiting file mode.\n";
          continue;
        }

        running = false;
        socket.close();
        break;
      }

      if (line == "clear")
      {
        std::cout << "\033[2J\033[H" << std::flush;
        continue;
      }

      if (!file_mode)
      {
        boost::asio::write(socket, boost::asio::buffer(line + "\n"));

        {
          std::lock_guard<std::mutex> lock(cout_mutex);
          std::cout << "\033[F\r\033[K";
          std::cout << "You: " << line << "\n"
                    << std::flush;
        }
      }
      else
      {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cout << "\r\033[K" << "Invalid command in file mode. Use 'send <file_path> <target_endpoint>' or 'exit' to leave file mode.\n"
                  << std::flush;
      }
    }
  }
  catch (std::exception &e)
  {
    if (running)
    {
      std::cerr << "Exception in input thread: " << e.what() << "\n";
    }
  }
}

void send_file_data(tcp::socket &socket, const std::string &file_path, const std::string &recipient_endpoint)
{
  std::ifstream file(file_path, std::ios_base::binary);
  if (!file.is_open())
  {
    std::lock_guard<std::mutex> lock(cout_mutex);
    std::cerr << "Failed to open file for sending: " << file_path << std::endl;
    return;
  }

  std::array<char, 2048> buffer;
  while (!file.eof())
  {
    file.read(buffer.data(), buffer.size());
    std::streamsize bytes_read = file.gcount();
    if (bytes_read > 0)
    {
      std::string raw_data(buffer.data(), bytes_read);
      std::string hex_data = hex_encode(raw_data);
      boost::asio::write(socket, boost::asio::buffer("file_data:" + recipient_endpoint + ":" + hex_data + "\n"));
    }
  }
  boost::asio::write(socket, boost::asio::buffer("file_end:" + recipient_endpoint + "\n"));
}

int main(int argc, char *argv[])
{
  if (argc != 3)
  {
    std::cerr << "Usage: client <host> <port>\n";
    return 1;
  }

  try
  {
    boost::asio::io_context io_context;

    tcp::resolver resolver(io_context);
    tcp::socket socket(io_context);
    boost::asio::connect(socket, resolver.resolve(argv[1], argv[2]));

    std::cout << "Connected to " << argv[1] << ":" << argv[2] << "\n";

    std::thread reader(read_handler, std::ref(socket));
    std::thread inputter(input_thread, std::ref(socket));

    reader.join();
    inputter.join();
  }
  catch (std::exception &e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
