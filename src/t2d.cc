#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <array>
#include <boost/asio.hpp>
#include <csignal>
#include <cstring>
#include <iostream>
#include <optional>

namespace asio = boost::asio;

using Buffer = std::array<char, 4096>;
using boost::asio::local::stream_protocol;
using boost::system::error_code;

class Proxy {
public:
  explicit Proxy(std::string_view address)
      : address(address),
        acceptor(ctx),
        in(ctx, ::dup(STDIN_FILENO)),
        out(ctx, ::dup(STDOUT_FILENO)),
        sig_set(ctx, SIGINT, SIGTERM) {}

  Proxy(Proxy &&) = delete;
  Proxy(const Proxy &) = delete;
  Proxy &operator=(Proxy &&) = delete;
  Proxy &operator=(const Proxy &) = delete;

  void read_stdin() {
    in.async_read_some(
        asio::buffer(tty_buffer), [this](error_code ec, std::size_t length) {
          if (ec || length == 0) {
            if (client) {
              disconnect_client();
            }
            return;
          }

          if (client) {
            asio::async_write(*client, asio::buffer(tty_buffer, length),
                              [this](error_code ec, std::size_t) {
                                if (ec) {
                                  client.reset();
                                }
                                read_stdin();
                              });
          } else {
            read_stdin();
          }
        });
  }

  void read_client() {
    if (!client) {
      return;
    }
    client->async_read_some(
        asio::buffer(client_buffer), [this](error_code ec, std::size_t length) {
          if (ec || length == 0) {
            disconnect_client();
            return;
          }

          asio::async_write(out, asio::buffer(client_buffer, length),
                            [this](error_code ec, std::size_t) {
                              if (ec) {
                                disconnect_client();
                              } else {
                                read_client();
                              }
                            });
        });
  }

  void do_accept() {
    acceptor.async_accept(
        [this](error_code ec, stream_protocol::socket socket) {
          if (ec) {
            return;
          }

          if (client) {
            socket.close();
          } else {
            client.emplace(std::move(socket));
            read_client();
          }

          do_accept();
        });
  }

  void disconnect_client() {
    if (client) {
      client->cancel();
      client->close();
      client.reset();
    }
  }

  void setup_signal_handler() {
    sig_set.async_wait([this](error_code ec, int) {
      if (ec) {
        return;
      }
      if (acceptor.is_open()) {
        acceptor.close();
      }
      disconnect_client();
      if (in.is_open()) {
        in.close();
      }
      if (out.is_open()) {
        out.close();
      }
      ctx.stop();
    });
  }

  int run() {
    error_code ec;

    ::unlink(address.data());

    acceptor.open(stream_protocol(), ec);  // NOLINT
    if (ec) {
      std::cerr << "Failed to open acceptor: " << ec.message() << '\n';
      return 1;
    }

    acceptor.bind(address, ec);  // NOLINT
    if (ec) {
      std::cerr << "Failed to bind: " << ec.message() << '\n';
      return 1;
    }

    auto backlog = stream_protocol::socket::max_listen_connections;
    acceptor.listen(backlog, ec);  // NOLINT
    if (ec) {
      std::cerr << "Failed to listen: " << ec.message() << '\n';
      return 1;
    }

    setup_signal_handler();
    do_accept();
    read_stdin();
    ctx.run();
    return 0;
  }

  ~Proxy() {
    ::unlink(address.data());
  }

private:
  std::string address;
  asio::io_context ctx;
  stream_protocol::acceptor acceptor;
  asio::posix::stream_descriptor in;
  asio::posix::stream_descriptor out;
  asio::signal_set sig_set;
  std::optional<stream_protocol::socket> client;
  Buffer client_buffer{};
  Buffer tty_buffer{};
};

struct RawTerminal {
  RawTerminal() = default;
  RawTerminal(const RawTerminal &) = default;
  RawTerminal(RawTerminal &&) = delete;
  RawTerminal &operator=(const RawTerminal &) = default;
  RawTerminal &operator=(RawTerminal &&) = delete;

  struct termios orig;
  bool raw;

  void enable_raw() {
    if (raw) {
      return;
    }

    raw = true;
    struct termios t{};
    auto ret = tcgetattr(STDIN_FILENO, &t);
    if (ret == -1) {
      throw std::runtime_error(strerror(errno));
    }

    orig = t;
    cfmakeraw(&t);
    ret = tcsetattr(STDIN_FILENO, TCSANOW, &t);
    if (ret == -1) {
      throw std::runtime_error(strerror(errno));
    }
  }

  void disable_raw() {
    if (raw) {
      tcsetattr(STDIN_FILENO, TCSANOW, &orig);
      raw = false;
    }
  }

  ~RawTerminal() {
    disable_raw();
  }
};

int main(int argc, char **argv) {
  if (argc != 2 || strcmp(argv[1], "-h") == 0 ||  // NOLINT
      strcmp(argv[1], "--help") == 0) {           // NOLINT
    std::cerr << "Usage: t2d <socket-path>\n";
    return 1;
  }

  try {
    RawTerminal t{};
    t.enable_raw();

    Proxy proxy(argv[1]);  // NOLINT
    return proxy.run();
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << '\n';
    return 1;
  }
}
