# muck-cpp

A tiny terminal survival game in C++17, inspired by *Muck*: gather, craft, survive the nights, and slay the Muck Chief.

## Build and run

VS Code: open the folder, press **Ctrl+Shift+B** to build, then run the "Run muck (external terminal)" launch config (or run `build/muck` in a terminal).

Command line:

    cmake -S . -B build && cmake --build build
    ./build/muck [seed]

No CMake? `g++ -std=c++17 -O2 src/*.cpp -o muck`

(On Windows with MSVC the binary is `build\Debug\muck.exe`. Windows 10+ terminals are supported.)

## How to play

- `WASD` move; walking into trees, rocks, ore or monsters gathers or attacks
- `c` craft (pickaxes, swords, armor), `e` eat an apple, `space` wait, `q` quit
- Stone needs a pickaxe, ore needs a stone pickaxe
- Nights spawn goblins `g` and brutes `G`; they flee at dawn
- On the night of day 4 the Muck Chief `B` arrives. Kill him to win.

## Layout

- `src/platform.hpp`: raw keyboard input + ANSI setup (Windows/POSIX)
- `src/game.hpp/.cpp`: world generation, crafting, day/night, mobs, rendering
- `src/main.cpp`: entry point
