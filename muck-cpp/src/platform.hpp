// Tiny cross-platform terminal layer: raw single-key input + ANSI colors.
#pragma once
#ifdef _WIN32
  #ifndef NOMINMAX
    #define NOMINMAX
  #endif
  #include <conio.h>
  #include <windows.h>
  namespace plat {
  inline void init() {
      HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
      DWORD mode = 0;
      if (GetConsoleMode(h, &mode))
          SetConsoleMode(h, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
  }
  inline void shutdown() {}
  inline int getKey() {
      int c = _getch();
      if (c == 0 || c == 224) { _getch(); return 0; }  // ignore arrows/function keys
      return c;
  }
  }
#else
  #include <termios.h>
  #include <unistd.h>
  namespace plat {
  inline termios saved;
  inline bool raw = false;
  inline void shutdown() { if (raw) tcsetattr(STDIN_FILENO, TCSANOW, &saved); }
  inline void init() {
      if (tcgetattr(STDIN_FILENO, &saved) == 0) {
          termios t = saved;
          t.c_lflag &= ~(ICANON | ECHO);
          t.c_cc[VMIN] = 1;
          t.c_cc[VTIME] = 0;
          tcsetattr(STDIN_FILENO, TCSANOW, &t);
          raw = true;
      }
  }
  inline int getKey() {
      unsigned char c;
      return read(STDIN_FILENO, &c, 1) == 1 ? c : -1;
  }
  }
#endif
