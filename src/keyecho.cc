#include <fmt/format.h>
#include <fmt/ranges.h>
#include <boost/asio/io_context.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/write.hpp>

#include <cstdint>
#include <span>
#include <stdexcept>
#include <vector>

#include <sys/ioctl.h>
#include <termios.h>

namespace asio = boost::asio;
using asio::posix::stream_descriptor;
using boost::system::error_code;

using namespace std::string_view_literals;

struct KeyCode {
  static constexpr uint32_t kCodepointMask = 0x1FFFFF;

  enum Function : uint32_t {
    Insert = 0x110000,
    Delete,
    PageUp,
    PageDown,
    Up,
    Down,
    Right,
    Left,
    Home,
    End,
    F1,
    F2,
    F3,
    F4,
    F5,
    F6,
    F7,
    F8,
    F9,
    F10,
    F11,
    F12,
    Enter,
    Escape,
    Backspace,
    Tab,
    Space,
    FocusIn,
    FocusOut,
    PasteStart,
    PasteEnd,
    Invalid,
  };

  enum Modifier : uint32_t {
    Shift = 1 << 21,
    Alt = 1 << 22,
    Ctrl = 1 << 23,
    Super = 1 << 24,
  };

  uint32_t c;

  constexpr KeyCode(uint32_t c) : c(c) {}

  bool is_modifier(uint32_t m) const {
    return (c & m) != 0;
  }

  uint32_t get_codepoint() const;
};

constexpr KeyCode shift(KeyCode c) {
  return {c.c | KeyCode::Shift};
}

constexpr KeyCode alt(KeyCode c) {
  return {c.c | KeyCode::Alt};
}

constexpr KeyCode ctrl(KeyCode c) {
  return {c.c | KeyCode::Ctrl};
}

constexpr KeyCode super(KeyCode c) {
  return {c.c | KeyCode::Super};
}

class KeyCodeParser {
public:
  explicit KeyCodeParser(uint8_t backspace) : backspace_(backspace) {}

  using Buffer = std::vector<uint8_t>;
  using BufferIter = Buffer::iterator;

  std::vector<KeyCode> parse(std::span<uint8_t> input) {
    buf_.insert(buf_.end(), input.begin(), input.end());
    return parse(/*flush=*/false);
  }

  std::vector<KeyCode> flush() {
    return parse(/*flush=*/true);
  }

private:
  std::vector<KeyCode> parse(bool flush);

  struct Result {
    enum Status { Success, Partial, Invalid };  // NOLINT

    KeyCode c;
    Status status;

    static Result success(KeyCode c) {
      return {.c = c, .status = Success};
    }

    static Result partial() {
      return {.c = 0, .status = Partial};
    }

    static Result invalid() {
      return {.c = 0, .status = Invalid};
    }
  };

  Result parse_key(BufferIter &it);

  Result parse_csi(BufferIter &it);

  Result parse_ss3(BufferIter &it);

  Buffer buf_;
  const uint8_t backspace_;
};

template <>
struct fmt::formatter<KeyCode> : fmt::formatter<std::string_view> {
  auto format(const KeyCode &key, fmt::format_context &ctx) const  // NOLINT
      -> fmt::format_context::iterator {
    if (key.is_modifier(KeyCode::Super)) {
      fmt::format_to(ctx.out(), "s-");
    }
    if (key.is_modifier(KeyCode::Ctrl)) {
      fmt::format_to(ctx.out(), "C-");
    }
    if (key.is_modifier(KeyCode::Alt)) {
      fmt::format_to(ctx.out(), "A-");
    }
    if (key.is_modifier(KeyCode::Shift)) {
      fmt::format_to(ctx.out(), "S-");
    }
    auto c = key.get_codepoint();
    if (c == '\t') {
      fmt::format_to(ctx.out(), "Tab");
    } else if (c == '\n') {
      fmt::format_to(ctx.out(), "Enter");
    } else if (c == ' ') {
      fmt::format_to(ctx.out(), "Space");
    } else if (c == 0x1b) {
      fmt::format_to(ctx.out(), "Esc");
    } else if ('!' <= c && c <= '~') {
      fmt::format_to(ctx.out(), "{}", char(c));
    } else if (c == KeyCode::Insert) {
      fmt::format_to(ctx.out(), "Insert");
    } else if (c == KeyCode::Delete) {
      fmt::format_to(ctx.out(), "Delete");
    } else if (c == KeyCode::PageUp) {
      fmt::format_to(ctx.out(), "PageUp");
    } else if (c == KeyCode::PageDown) {
      fmt::format_to(ctx.out(), "PageDown");
    } else if (c == KeyCode::Up) {
      fmt::format_to(ctx.out(), "Up");
    } else if (c == KeyCode::Down) {
      fmt::format_to(ctx.out(), "Down");
    } else if (c == KeyCode::Right) {
      fmt::format_to(ctx.out(), "Right");
    } else if (c == KeyCode::Left) {
      fmt::format_to(ctx.out(), "Left");
    } else if (c == KeyCode::Home) {
      fmt::format_to(ctx.out(), "Home");
    } else if (c == KeyCode::End) {
      fmt::format_to(ctx.out(), "End");
    } else if (c == KeyCode::F1) {
      fmt::format_to(ctx.out(), "F1");
    } else if (c == KeyCode::F2) {
      fmt::format_to(ctx.out(), "F2");
    } else if (c == KeyCode::F3) {
      fmt::format_to(ctx.out(), "F3");
    } else if (c == KeyCode::F4) {
      fmt::format_to(ctx.out(), "F4");
    } else if (c == KeyCode::F5) {
      fmt::format_to(ctx.out(), "F5");
    } else if (c == KeyCode::F6) {
      fmt::format_to(ctx.out(), "F6");
    } else if (c == KeyCode::F7) {
      fmt::format_to(ctx.out(), "F7");
    } else if (c == KeyCode::F8) {
      fmt::format_to(ctx.out(), "F8");
    } else if (c == KeyCode::F9) {
      fmt::format_to(ctx.out(), "F9");
    } else if (c == KeyCode::F10) {
      fmt::format_to(ctx.out(), "F10");
    } else if (c == KeyCode::F11) {
      fmt::format_to(ctx.out(), "F11");
    } else if (c == KeyCode::F12) {
      fmt::format_to(ctx.out(), "F12");
    } else if (c == KeyCode::Backspace) {
      fmt::format_to(ctx.out(), "Backspace");
    } else if (c == KeyCode::FocusIn) {
      fmt::format_to(ctx.out(), "FocusIn");
    } else if (c == KeyCode::FocusOut) {
      fmt::format_to(ctx.out(), "FocusOut");
    } else if (c == KeyCode::PasteStart) {
      fmt::format_to(ctx.out(), "PasteStart");
    } else if (c == KeyCode::PasteEnd) {
      fmt::format_to(ctx.out(), "PasteEnd");
    } else if (c == KeyCode::Invalid) {
      fmt::format_to(ctx.out(), "Invalid");
    } else {
      fmt::format_to(ctx.out(), "0x{:x}", c);
    }
    return ctx.out();
  }
};

using BufferIter = KeyCodeParser::BufferIter;

namespace {

bool match(BufferIter &it, uint8_t value) {
  if (*it == value) {
    it++;
    return true;
  }
  return false;
}

std::pair<uint8_t, bool> match(BufferIter &it, uint8_t start, uint8_t end) {
  int c = *it;
  if (start <= c && c <= end) {
    it++;
    return std::make_pair(c, true);
  }
  return std::make_pair(c, false);
}

uint32_t parse_kitty_mask(uint32_t m) {
  uint32_t ret = 0;
  if ((m & 1) != 0) {
    ret |= KeyCode::Shift;
  }
  if ((m & 2) != 0) {
    ret |= KeyCode::Alt;
  }
  if ((m & 4) != 0) {
    ret |= KeyCode::Ctrl;
  }
  if ((m & 8) != 0) {
    ret |= KeyCode::Super;
  }
  return ret;
}

}  // namespace

uint32_t KeyCode::get_codepoint() const {
  auto v = c & kCodepointMask;
  if (v == Enter) {
    return '\n';
  }
  if (v == Escape) {
    return 0x1b;
  }
  if (v == Tab) {
    return '\t';
  }
  return v;
}

std::vector<KeyCode> KeyCodeParser::parse(bool flush) {  // NOLINT
  std::vector<KeyCode> ret;
  auto it = buf_.begin();
  auto anchor = it;

  auto reach_end = [this, flush, &ret, &it,
                    &anchor](KeyCode alternative) -> bool {
    if (it == buf_.end()) {
      if (flush) {
        ret.push_back(alternative);
      } else {
        it = anchor;
      }
      return true;
    }
    return false;
  };

  while (it != buf_.end()) {
    anchor = it;

    if (*it != 27) {
      auto res = parse_key(it);
      if (res.status == Result::Success) {
        ret.push_back(res.c);
      } else if (res.status == Result::Partial) {
        it = anchor;
        break;
      } else {
        ret.emplace_back(KeyCode::Invalid);
      }
      continue;
    }

    // consume ESC
    it++;

    if (reach_end(KeyCode::Escape)) {
      break;
    }

    Result res{.c = 0, .status = Result::Invalid};

    if (match(it, '[')) {
      if (reach_end(alt('['))) {
        break;
      }
      res = parse_csi(it);

    } else if (match(it, 'O')) {
      if (reach_end(alt('O'))) {
        break;
      }
      res = parse_ss3(it);

    } else {
      // alt modified sequence, the following part should not be UTF-8 sequence
      auto res = parse_key(it);
      if (res.status == Result::Success) {
        ret.push_back(alt(res.c));
      } else {
        ret.push_back(KeyCode::Invalid);
      }
      continue;
    }

    switch (res.status) {
    case Result::Success:
      ret.push_back(res.c);
      break;
    case Result::Partial:
      it = anchor;
      break;
    case Result::Invalid:
      ret.push_back(KeyCode::Invalid);
      break;
    default:
      __builtin_unreachable();
    }
  }

  // remove processed bytes from buffer
  buf_.erase(buf_.begin(), it);
  return ret;
}

KeyCodeParser::Result KeyCodeParser::parse_key(BufferIter &it) {
  if (match(it, 0)) {
    return Result::success(ctrl(KeyCode::Space));
  }

  if (auto [c, s] = match(it, 1, 26); s) {
    if (c == ('m' & 0x1f)) {
      return Result::success(KeyCode::Enter);
    }
    if (c == ('i' & 0x1f)) {
      return Result::success(KeyCode::Tab);
    }
    // use lower case
    return Result::success(ctrl(c | 0x60));
  }

  if (match(it, 27)) {
    return Result::success(KeyCode::Escape);
  }

  if (auto [c, s] = match(it, 28, 31); s) {
    // 28 C-backslash C-4
    // 29 C-] C-5
    // 30 C-^ C-6
    // 31 C-_ C-7
    return Result::success(ctrl(c | 0x40));
  }

  if (match(it, ' ')) {  // 32
    return Result::success(KeyCode::Space);
  }

  if (auto [c, s] = match(it, '!', '~'); s) {  // [33, 126]
    return Result::success(c);
  }

  if (match(it, backspace_)) {  // 127
    return Result::success(KeyCode::Backspace);
  }

  // [128, 255] UTF-8 sequences
  uint32_t cp = *it++;
  int rest = 0;

  if ((cp & 0xe0) == 0xc0) {
    // 2-byte sequence
    rest = 1;
    cp &= 0x1f;
  } else if ((cp & 0xf0) == 0xe0) {
    // 3-byte sequence
    rest = 2;
    cp &= 0x0f;
  } else if ((cp & 0xf8) == 0xf0) {
    // 4-byte sequence
    rest = 3;
    cp &= 0x07;
  } else {
    // invalid UTF-8 start byte, consume it and return invalid
    return Result::invalid();
  }

  for (int i = 0; i < rest; i++) {
    if (it == buf_.end()) {
      // incomplete UTF-8 sequence
      return Result::partial();
    }
    auto c = *it++;
    if ((c & 0xc0) != 0x80) {
      return Result::invalid();
    }
    cp = (cp << 6) | (c & 0x3f);
  }

  return Result::success(cp);
}

KeyCodeParser::Result KeyCodeParser::parse_csi(BufferIter &it) {
  [[maybe_unused]] char private_marker = 0;
  // ? < = >
  if (auto [c, m] = match(it, 0x3c, 0x3f); m) {
    private_marker = static_cast<char>(c);
  }

  if (it == buf_.end()) {
    return Result::partial();
  }

  int params[16][4] = {};  // NOLINT
  auto c = *it++;
  for (int count = 0, subcount = 0; count < 16 && (0x30 <= c && c <= 0x3f);) {
    if (std::isdigit(c) != 0) {
      params[count][subcount] *= 10;       // NOLINT
      params[count][subcount] += c - '0';  // NOLINT
    } else if (c == ':' && subcount < 3) {
      subcount++;
    } else if (c == ';') {
      count++;
      subcount = 0;
    } else {  // ? < = >
      return Result::invalid();
    }
    if (it == buf_.end()) {
      return Result::partial();
    }
    c = *it++;
  }

  // consume and ignore intermediate bytes
  while (0x20 <= c && c <= 0x2f) {
    if (it == buf_.end()) {
      return Result::partial();
    }
    c = *it++;
  }

  // check final byte
  if (c != '$' && (c < 0x40 || 0x7e < c)) {
    return Result::invalid();
  }

  auto masked_key = [&](uint32_t key, uint32_t shifted_key = 0) {
    auto m = std::max(params[1][0] - 1, 0);  // NOLINT
    auto mask = parse_kitty_mask(m);
    if (shifted_key != 0 && (mask & KeyCode::Shift) != 0) {
      mask &= ~KeyCode::Shift;
      key = shifted_key;
    }
    return KeyCode(mask | key);
  };

  switch (c) {
  case 'A':
    return Result::success(masked_key(KeyCode::Up));
  case 'B':
    return Result::success(masked_key(KeyCode::Down));
  case 'C':
    return Result::success(masked_key(KeyCode::Right));
  case 'D':
    return Result::success(masked_key(KeyCode::Left));
  case 'F':
    return Result::success(masked_key(KeyCode::End));
  case 'H':
    return Result::success(masked_key(KeyCode::Home));
  case 'P':
    return Result::success(masked_key(KeyCode::F1));
  case 'Q':
    return Result::success(masked_key(KeyCode::F2));
  case 'R':
    return Result::success(masked_key(KeyCode::F3));
  case 'S':
    return Result::success(masked_key(KeyCode::F4));
  case 'I':
    return Result::success(KeyCode::FocusIn);
  case 'O':
    return Result::success(KeyCode::FocusOut);
  case 'u':
    return Result::success(masked_key(params[0][0], params[0][1]));
  case '~':
    switch (params[0][0]) {
    case 1:
      return Result::success(masked_key(KeyCode::Home));
    case 2:
      return Result::success(masked_key(KeyCode::Insert));
    case 3:
      return Result::success(masked_key(KeyCode::Delete));
    case 4:
      return Result::success(masked_key(KeyCode::End));
    case 5:
      return Result::success(masked_key(KeyCode::PageUp));
    case 6:
      return Result::success(masked_key(KeyCode::PageDown));
    case 15:
      return Result::success(masked_key(KeyCode::F5));
    case 17:
      return Result::success(masked_key(KeyCode::F6));
    case 18:
      return Result::success(masked_key(KeyCode::F7));
    case 19:
      return Result::success(masked_key(KeyCode::F8));
    case 20:
      return Result::success(masked_key(KeyCode::F9));
    case 21:
      return Result::success(masked_key(KeyCode::F10));
    case 23:
      return Result::success(masked_key(KeyCode::F11));
    case 24:
      return Result::success(masked_key(KeyCode::F12));
    case 200:
      return Result::success(KeyCode::PasteStart);
    case 201:
      return Result::success(KeyCode::PasteEnd);
    default:
      return Result::invalid();
    }
  default:
    return Result::invalid();
  }
}

KeyCodeParser::Result KeyCodeParser::parse_ss3(BufferIter &it) {  // NOLINT
  auto c = *it++;

  switch (c) {
  case 'P':
    return Result::success(KeyCode::F1);
  case 'Q':
    return Result::success(KeyCode::F2);
  case 'R':
    return Result::success(KeyCode::F3);
  case 'S':
    return Result::success(KeyCode::F4);
  case 'H':
    return Result::success(KeyCode::Home);
  case 'F':
    return Result::success(KeyCode::End);
  case 'A':
    return Result::success(KeyCode::Up);
  case 'B':
    return Result::success(KeyCode::Down);
  case 'C':
    return Result::success(KeyCode::Right);
  case 'D':
    return Result::success(KeyCode::Left);
  default:
    return Result::invalid();
  }
}

struct input_printer_t {
  std::span<uint8_t> input;
};

template <>
struct fmt::formatter<input_printer_t> : fmt::formatter<std::string_view> {
  auto format(const input_printer_t &i, auto &ctx) const {  // NOLINT
    fmt::format_to(ctx.out(), "[");
    bool start = true;
    for (auto c : i.input) {
      if (!start) {
        fmt::format_to(ctx.out(), ", 0x{:x}", int(c));
      } else {
        fmt::format_to(ctx.out(), "0x{:x}", int(c));
        start = false;
      }
    }
    return fmt::format_to(ctx.out(), "]");
  }
};

class AsioCtx;

class EchoState {
public:
  EchoState(AsioCtx &display);
  EchoState(const EchoState &) = delete;
  EchoState(EchoState &&) = delete;
  EchoState &operator=(const EchoState &) = delete;
  EchoState &operator=(EchoState &&) = delete;
  ~EchoState() = default;

  void setup(int /*unused*/, int /*unused*/);

  void tick(int d);

  void resize(int /*unused*/, int /*unused*/) {}

  void emit(std::span<uint8_t> input);

  void prepare_exit();

private:
  AsioCtx &ctx_;
  KeyCodeParser parser_;
  int duration_ = 0;
};

std::unique_ptr<EchoState> create_echo_state(AsioCtx &ctx) {
  return std::make_unique<EchoState>(ctx);
}

class AsioCtx {
public:
  AsioCtx()
      : sigs_(ctx_, SIGINT, SIGTERM, SIGWINCH),
        timer_(ctx_),
        in_(ctx_, ::dup(STDIN_FILENO)),
        out_(ctx_, ::dup(STDOUT_FILENO)) {
    if (isatty(STDIN_FILENO) != 1 || isatty(STDOUT_FILENO) != 1) {
      throw std::runtime_error("stdin/stderr is not a tty");
    }
    tcgetattr(STDIN_FILENO, &orig_);
    auto t = orig_;
    cfmakeraw(&t);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &t);
  }

  AsioCtx(const AsioCtx &) = delete;
  AsioCtx(AsioCtx &&) = delete;
  AsioCtx &operator=(const AsioCtx &) = delete;
  AsioCtx &operator=(AsioCtx &&) = delete;

  ~AsioCtx() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_);
  }

  void exit() {
    ctx_.stop();
  }

  void post(std::function<void()> cb) {
    asio::post(ctx_.get_executor(), std::move(cb));
  }

  std::pair<int, int> get_window_size() {  // NOLINT
    winsize ws{};
    ioctl(STDIN_FILENO, TIOCGWINSZ, &ws);  // NOLINT
    return std::make_pair<int>(ws.ws_row, ws.ws_col);
  }

  void sync_draw(std::string_view sv) {
    asio::write(out_, asio::buffer(sv));
  }

  uint8_t get_erase_key() const {
    return orig_.c_cc[VERASE];
  }

  void run(std::unique_ptr<EchoState> s) {
    state_ = std::move(s);
    setup_signal();
    setup_timer();
    handle_read();
    asio::post(ctx_.get_executor(), [this] {
      auto [row, col] = get_window_size();
      state_->setup(row, col);
    });
    ctx_.run();
  }

private:
  void setup_timer() {
    constexpr int kTickInterval = 25;  // milliseconds
    timer_.expires_after(std::chrono::milliseconds(kTickInterval));
    timer_.async_wait([this](error_code ec) {
      if (ec) {
        state_->prepare_exit();
        return;
      }
      state_->tick(kTickInterval);
      setup_timer();
    });
  }

  void setup_signal() {
    sigs_.async_wait([this](error_code, int s) {
      if (s == SIGWINCH) {
        auto [row, col] = get_window_size();
        state_->resize(row, col);
        setup_signal();
      } else {
        state_->prepare_exit();
      }
    });
  }

  void handle_read() {
    in_.async_read_some(asio::buffer(buf_),
                        [this](error_code ec, std::size_t length) {
                          if (ec) {
                            state_->prepare_exit();
                            return;
                          }
                          state_->emit({buf_.data(), length});
                          handle_read();
                        });
  }

  asio::io_context ctx_;
  asio::signal_set sigs_;
  asio::steady_timer timer_;
  stream_descriptor in_;
  stream_descriptor out_;
  std::array<uint8_t, 128> buf_{};
  termios orig_{};
  std::unique_ptr<EchoState> state_;
};

EchoState::EchoState(AsioCtx &display)
    : ctx_(display), parser_(ctx_.get_erase_key()) {}

void EchoState::setup(int /*unused*/, int /*unused*/) {
  ctx_.sync_draw("\x1b[?1004h\x1b[?2004hPress Ctrl-Q to exit\r\n");
}

void EchoState::tick(int d) {
  duration_ += d;
  if (duration_ > 50) {
    duration_ = 0;
    auto ret = parser_.flush();
    if (ret.empty()) {
      return;
    }
    auto output = fmt::format("output: {}\r\n", ret);
    ctx_.sync_draw(output);
    for (auto &key : ret) {
      if (key.c == ctrl('q').c) {
        prepare_exit();
      }
    }
  }
}

void EchoState::emit(std::span<uint8_t> input) {
  auto ret = parser_.parse(input);
  auto output =
      fmt::format("input: {}\r\noutput: {}\r\n", input_printer_t{input}, ret);
  ctx_.sync_draw(output);
  for (auto &key : ret) {
    if (key.c == ctrl('q').c) {
      prepare_exit();
    }
  }
  duration_ = 0;
}

void EchoState::prepare_exit() {
  ctx_.sync_draw("\x1b[?2004l\x1b[?1004l");
  ctx_.exit();
}

std::unique_ptr<AsioCtx> create_context() {
  return std::make_unique<AsioCtx>();
}

int main() {
  try {
    auto ctx = create_context();
    ctx->run(create_echo_state(*ctx));
    return 0;
  } catch (const std::exception &ex) {
    fmt::println(stderr, "{}\n", ex.what());
    return 1;
  }
}
