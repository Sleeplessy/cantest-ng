#include <algorithm>
#include <boost/asio.hpp>
#include <boost/asio/buffer.hpp>
#include <ccel/canbus.hpp>
#include <iostream>
#include <rang.hpp>
#include <string>
#include <vector>

void connect_handler() { std::cout << "Client connected." << std::endl; }
int main(int argc, const char **argv) {
  try {
    boost::asio::io_service io;
    std::vector<std::string> __args(argv, argv + argc);
    int port = 7889; // The default port
    bool silent = false;
    ccel::canbus::canbus_handler *local_transfer = nullptr;
    if (argc > 2) {
      auto _silent_option = std::find(__args.begin(), __args.end(), "-s");
      if (_silent_option != __args.end())
        silent = true;
      auto status_p = std::find(__args.begin(), __args.end(), "-p");
      if (status_p != __args.end()) {
        if (status_p++ == __args.end()) {
          std::cout << "Need a param to define port!" << std::endl;
          return 0;
        } else {
          port = std::stoi(*status_p);
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
    std::string if_name;
    if (__args.size() > 1)
      if_name = __args[1];
    else
      if_name = "can0";
    boost::asio::ip::tcp::acceptor acc(
        io, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port));
    std::cout << "Start listening canbus interface " << if_name << " on port "
              << acc.local_endpoint().address() << ":"
              << acc.local_endpoint().port() << std::endl;
    acc.non_blocking(true);
    ccel::canbus::canbus_handler can(if_name.c_str(), io);
    // can.loopback(true);
    boost::asio::signal_set sig(io, SIGINT, SIGUSR1);
    for (;;) {
      can_frame __frame;
      boost::asio::ip::tcp::socket sock(io);
      boost::system::error_code ec;
      while (!ec || ec == boost::asio::error::eof ||
             ec == boost::asio::error::connection_reset) {
        acc.accept(sock, ec);
        __frame = can.read_sock();
        if (!silent) {
          bool ext = ccel::canbus::ext_frame_test(__frame);
          std::cout << std::hex << std::uppercase << std::setfill('0')
                    << std::setw(8);
          if (!ext)
            std::cout << std::setfill(' ');
          std::cout << static_cast<unsigned>(__frame.can_id & CAN_EFF_MASK)
                    << " " << (ext ? "EXT" : "STD");
          std::cout << "[" << static_cast<unsigned>(__frame.can_dlc) << "]"
                    << "\t";
          for (auto i = 0; i < __frame.can_dlc; i++)
            std::cout << std::setw(2) << std::setfill('0')
                      << static_cast<unsigned>(__frame.data[i]) << " ";
          if (local_transfer)
            std::cout << "\t" << local_transfer->interface_name();
          auto __ep = sock.remote_endpoint(ec);
          if ((ec != boost::asio::error::eof ||
               ec != boost::asio::error::connection_reset)&&__ep.address().to_string() != "0.0.0.0") {
            for (auto i = __frame.can_dlc; i < 8; i++)
              std::cout << "   ";
            std::cout << "\t" << sock.remote_endpoint().address();
          }
          std::cout << std::endl;
        }
        if (sock.is_open()) {
          sock.write_some(boost::asio::buffer(&__frame, sizeof(__frame)), ec);
        }
      }
      if (local_transfer) {
        try {
          local_transfer->write_sock(__frame);
        } catch (...) {
          throw;
        }
      }
      io.run();
    }
  } catch (std::exception &e) {
    std::cout << e.what() << std::endl;
    return -1;
  }
}
