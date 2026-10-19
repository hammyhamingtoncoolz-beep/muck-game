#include <cstdlib>
#include <ctime>

#include "game.hpp"
#include "platform.hpp"

int main(int argc, char** argv) {
    unsigned seed = argc > 1 ? static_cast<unsigned>(std::atoi(argv[1]))
                             : static_cast<unsigned>(std::time(nullptr));
    plat::init();
    std::atexit(plat::shutdown);
    muck::Game game(seed);
    game.run();
    return 0;
}
