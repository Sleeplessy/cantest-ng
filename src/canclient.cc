#include <ccel/canbus.hpp>

#include <algorithm>
#include <vector>
#include <boost/asio.hpp>
#include <iostream>
#include <rang.hpp>
#include <string>
int main(int argc, char **argv) {
  if (argc < 2) {
    std::cout << "USAGE: canclient IP [-p port] [-l local_can_if] [-s]"
              << std::endl;
    return -1;
  }
  std::vector<std::string> __args(argv, argv + argc);
  using namespace boost::asio;
  io_service io;
  ip::tcp::socket sock(io);
  int port = 7889;
  bool silent = false;
  bool color = false; // output color?
  ccel::canbus::canbus_handler *local_transfer = nullptr;
  ip::tcp::endpoint ep(ip::address::from_string(__args[1]), port);
  try {
    sock.connect(ep);
  } catch (...) {
    std::cout << "Connection failed!" << std::endl;
    return -1;
  }
  if (argc > 2) {
    auto _silent_option = std::find(__args.begin(), __args.end(), "-s");
    if (_silent_option != __args.end())
      silent = true;
    auto _color_option = std::find(__args.begin(), __args.end(), "-s");
    if (_color_option != __args.end())
      color = true;
    auto _port_option = std::find(__args.begin(), __args.end(), "-p");

    if (_port_option != __args.end() && (_port_option + 1 != __args.end())) {
      try {
        port = std::stoi(*(_port_option + 1));
      } catch (...) {
        std::cout << "port must be a number!" << std::endl;
        return -1;
      }
    }
    auto status_l = std::find(__args.begin(), __args.end(), "-l");
    if (status_l != __args.end()) {
      if ((status_l + 1) == __args.end()) {
        std::cout << "Need a param to define can interface!" << std::endl;
        return 0;
      } else {
        try {
          local_transfer =
              new ccel::canbus::canbus_handler((*(status_l + 1)).c_str(), io);
        } catch (...) {
          std::cout << "bind can interface failed!" << std::endl;
          return -1;
        }
      }
    }
  }

  can_frame buff;
  boost::system::error_code ec;
  for (long long i = 0;; i++) {
    sock.read_some(buffer(&buff, sizeof(buff)), ec);
    if (!ec) {
      auto __frame = buff;
      if (!silent) {
        bool ext = ccel::canbus::ext_frame_test(__frame);
        std::cout << std::hex << std::uppercase << std::setfill('0')
                  << std::setw(8);
        if (!ext)
          std::cout << std::setfill(' ');
        std::cout << static_cast<unsigned>(__frame.can_id & CAN_EFF_MASK)
                  << " ";
        if (color)
          std::cout << rang::control::forceColor << rang::fg::blue;
        std::cout << (ext ? "EXT" : "STD");
        if (color)
          std::cout << rang::fg::green;
        std::cout << "[" << static_cast<unsigned>(__frame.can_dlc) << "]"
                  << "\t";
        if (color)
          std::cout << rang::fg::yellow;
        for (auto i = 0; i < __frame.can_dlc; i++)
          std::cout << std::setw(2) << std::setfill('0')
                    << static_cast<unsigned>(__frame.data[i]) << " ";
        for (auto i = __frame.can_dlc; i < 8; i++)
          std::cout << "   ";
        if (color)
          std::cout << rang::fg::red;
        std::cout << "\tFROM\t" << sock.remote_endpoint() << std::endl;
        if (color)
          std::cout << rang::style::reset;
      }
      if (local_transfer)
        local_transfer->write_sock(buff);
      io.run();
    } else {
      if (ec == boost::asio::error::eof) {
        std::cout << "Remote server closed" << std::endl;
        break;
      }
      std::cout << ec.message() << std::endl;
      break;
    }
  }
}
