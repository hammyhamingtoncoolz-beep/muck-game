#pragma once
#include <deque>
#include <random>
#include <string>
#include <vector>

namespace muck {

enum class Tile { Grass, Tree, Rock, Ore, Water };

struct Mob {
    int x, y, hp, dmg;
    char glyph;
    bool boss;
};

class Game {
public:
    explicit Game(unsigned seed);
    void run();

private:
    static constexpr int W = 64, H = 32;   // world size
    static constexpr int VW = 41, VH = 19; // viewport size

    std::mt19937 rng;
    std::vector<Tile> tiles;
    std::vector<Mob> mobs;
    std::deque<std::string> msgs;

    int px = W / 2, py = H / 2, hp = 20, maxHp = 20;
    int wood = 0, stone = 0, iron = 0, apples = 0;
    int pick = 0;   // 0 none, 1 wooden, 2 stone
    int sword = 1;  // damage per hit (1 = fists)
    int armor = 0;
    int turn = 0, hitX = -1, hitY = -1, hits = 0;
    bool menu = false, over = false, won = false, quit = false, bossSpawned = false;

    int rnd(int a, int b);
    Tile& at(int x, int y) { return tiles[y * W + x]; }
    static bool inBounds(int x, int y) { return x >= 0 && y >= 0 && x < W && y < H; }
    bool isNight() const;
    int day() const;
    void log(const std::string& m);
    Mob* mobAt(int x, int y);

    void generate();
    void handle(int key);
    bool tryMove(int dx, int dy);
    bool harvest(int x, int y);
    void attack(Mob& m);
    void craft(int idx);
    void eat();
    void step();
    void spawnMob(bool boss);
    void mobsAct();
    void render() const;
};

}  // namespace muck
