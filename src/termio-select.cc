#include <termios.h>
#include <unistd.h>
#include <iostream>
#include <vector>

void enableRawMode() {
  termios term{};
  tcgetattr(STDIN_FILENO, &term);
  // disable line buffer and echo
  term.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &term);
}

void disableRawMode() {
  termios term{};
  tcgetattr(STDIN_FILENO, &term);
  term.c_lflag |= ICANON | ECHO;
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &term);
}

int main() {
  enableRawMode();
  std::vector<std::string> options{"option 0", "option 1", "option 2"};
  int selected = 0;
  char c{};

  for (int i = 0; i < options.size(); i++) {
    std::cout << (i == selected ? "\033[7m" : "")  // reverse color fg/bg
              << options[i] << "\033[0m\n";        // reset
  }

  while (read(STDIN_FILENO, &c, 1) == 1) {
    if (c == '\x1B') {
      char seq[2]{};  // NOLINT
      read(STDIN_FILENO, &seq[0], 1);
      read(STDIN_FILENO, &seq[1], 1);
      if (seq[0] == '[') {
        if (seq[1] == 'A' && selected > 0) {
          selected--;
        } else if (seq[1] == 'B' && selected < options.size() - 1) {
          selected++;
        }
      }
    } else if (c == '\n') {
      break;
    }

    // move cursor
    std::cout << "\033[" << options.size() << "A";
    for (int i = 0; i < options.size(); i++) {
      std::cout << (i == selected ? "\033[7m" : "\033[0m") << options[i]
                << "\033[0m\n";
    }
  }

  disableRawMode();
  std::cout << "selected: " << options[selected] << '\n';
  return 0;
}
