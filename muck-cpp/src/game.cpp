#include "game.hpp"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <iostream>

#include "platform.hpp"

namespace muck {

namespace {
constexpr int DAY_LEN = 80, NIGHT_LEN = 50, CYCLE = DAY_LEN + NIGHT_LEN;
constexpr int BOSS_DAY = 4;

struct Recipe { const char* name; int wood, stone, iron; };
const Recipe RECIPES[] = {
    {"Wooden pickaxe", 5, 0, 0},
    {"Stone pickaxe", 3, 5, 0},
    {"Wooden sword (dmg 3)", 4, 0, 0},
    {"Stone sword (dmg 5)", 2, 5, 0},
    {"Iron sword (dmg 8)", 2, 0, 5},
    {"Iron armor (-2 dmg taken)", 0, 0, 6},
};
constexpr int NUM_RECIPES = sizeof(RECIPES) / sizeof(RECIPES[0]);
}  // namespace

Game::Game(unsigned seed) : rng(seed) {
    generate();
    log("You wake in the muck. Chop trees (T) to begin.");
}

int Game::rnd(int a, int b) { return std::uniform_int_distribution<int>(a, b)(rng); }
bool Game::isNight() const { return turn % CYCLE >= DAY_LEN; }
int Game::day() const { return turn / CYCLE + 1; }

void Game::log(const std::string& m) {
    msgs.push_back(m);
    while (msgs.size() > 4) msgs.pop_front();
}

Mob* Game::mobAt(int x, int y) {
    for (auto& m : mobs) if (m.x == x && m.y == y) return &m;
    return nullptr;
}

void Game::generate() {
    tiles.assign(W * H, Tile::Grass);
    auto blob = [&](int cx, int cy, int r, Tile t, int pct) {
        for (int y = cy - r; y <= cy + r; ++y)
            for (int x = cx - r; x <= cx + r; ++x)
                if (inBounds(x, y) && (x - cx) * (x - cx) + (y - cy) * (y - cy) <= r * r &&
                    rnd(1, 100) <= pct)
                    at(x, y) = t;
    };
    for (int i = 0; i < 7; ++i) blob(rnd(3, W - 4), rnd(3, H - 4), rnd(2, 4), Tile::Water, 100);
    for (int i = 0; i < 10; ++i) {
        int cx = rnd(3, W - 4), cy = rnd(3, H - 4);
        blob(cx, cy, rnd(2, 3), Tile::Rock, 85);
        blob(cx, cy, 1, Tile::Ore, 45);
    }
    for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x) {
            if (at(x, y) == Tile::Grass && rnd(1, 100) <= 10) at(x, y) = Tile::Tree;
            if (x == 0 || y == 0 || x == W - 1 || y == H - 1) at(x, y) = Tile::Water;
        }
    for (int y = py - 2; y <= py + 2; ++y)
        for (int x = px - 2; x <= px + 2; ++x) at(x, y) = Tile::Grass;
}

bool Game::harvest(int x, int y) {
    Tile t = at(x, y);
    int need = t == Tile::Tree ? 0 : (t == Tile::Rock ? 1 : 2);
    int total = t == Tile::Tree ? 2 : (t == Tile::Rock ? 3 : 4);
    if (pick < need) {
        log(need == 1 ? "You need a pickaxe to mine stone." : "You need a stone pickaxe for ore.");
        return false;
    }
    if (hitX != x || hitY != y) { hitX = x; hitY = y; hits = 0; }
    if (++hits < total) {
        log("You strike it... (" + std::to_string(hits) + "/" + std::to_string(total) + ")");
        return true;
    }
    hits = 0; hitX = -1;
    if (t == Tile::Tree) {
        int w = rnd(2, 3); wood += w;
        std::string s = "Timber! +" + std::to_string(w) + " wood.";
        if (rnd(1, 100) <= 30) { ++apples; s += " An apple falls."; }
        log(s);
    } else if (t == Tile::Rock) {
        int s = rnd(2, 3); stone += s;
        log("Rock splits. +" + std::to_string(s) + " stone.");
    } else {
        int o = rnd(1, 2); iron += o;
        log("Glittering vein! +" + std::to_string(o) + " iron.");
    }
    at(x, y) = Tile::Grass;
    return true;
}

void Game::attack(Mob& m) {
    m.hp -= sword;
    if (m.hp > 0) {
        log("You hit it for " + std::to_string(sword) + " (" + std::to_string(m.hp) + " hp left).");
        return;
    }
    if (m.boss) { won = true; over = true; log("The Muck Chief collapses!"); }
    else log("You slay the creature.");
    mobs.erase(std::remove_if(mobs.begin(), mobs.end(), [&](const Mob& o) { return &o == &m; }),
               mobs.end());
}

bool Game::tryMove(int dx, int dy) {
    int nx = px + dx, ny = py + dy;
    if (!inBounds(nx, ny)) return false;
    if (Mob* m = mobAt(nx, ny)) { attack(*m); return true; }
    switch (at(nx, ny)) {
        case Tile::Grass: px = nx; py = ny; return true;
        case Tile::Water: log("Too deep to wade."); return false;
        default: return harvest(nx, ny);
    }
}

void Game::craft(int idx) {
    if (idx < 0 || idx >= NUM_RECIPES) return;
    const Recipe& r = RECIPES[idx];
    bool owned = (idx == 0 && pick >= 1) || (idx == 1 && pick >= 2) || (idx == 2 && sword >= 3) ||
                 (idx == 3 && sword >= 5) || (idx == 4 && sword >= 8) || (idx == 5 && armor >= 2);
    if (owned) { log("You already have that or better."); return; }
    if (wood < r.wood || stone < r.stone || iron < r.iron) { log("Not enough materials."); return; }
    wood -= r.wood; stone -= r.stone; iron -= r.iron;
    switch (idx) {
        case 0: pick = 1; break;
        case 1: pick = 2; break;
        case 2: sword = 3; break;
        case 3: sword = 5; break;
        case 4: sword = 8; break;
        case 5: armor = 2; break;
    }
    log(std::string("Crafted: ") + r.name + ".");
}

void Game::eat() {
    if (apples <= 0) { log("No apples."); return; }
    --apples;
    hp = std::min(maxHp, hp + 6);
    log("Crunch. +6 hp.");
}

void Game::spawnMob(bool boss) {
    for (int tries = 0; tries < 40; ++tries) {
        int x = px + rnd(-14, 14), y = py + rnd(-9, 9);
        if (!inBounds(x, y) || at(x, y) != Tile::Grass || mobAt(x, y)) continue;
        if (std::abs(x - px) + std::abs(y - py) < 8) continue;
        int d = day();
        if (boss) mobs.push_back({x, y, 60, 6, 'B', true});
        else if (d >= 3 && rnd(1, 100) <= 35) mobs.push_back({x, y, 10 + d * 3, 3 + d / 2, 'G', false});
        else mobs.push_back({x, y, 4 + d * 2, 1 + d / 2, 'g', false});
        return;
    }
}

void Game::mobsAct() {
    for (auto& m : mobs) {
        int dx = px - m.x, dy = py - m.y;
        if (std::abs(dx) + std::abs(dy) == 1) {
            int dmg = std::max(1, m.dmg - armor);
            hp -= dmg;
            log(std::string(m.boss ? "The Muck Chief" : "A creature") + " hits you for " +
                std::to_string(dmg) + ".");
            if (hp <= 0) { hp = 0; over = true; }
            continue;
        }
        if (!m.boss && rnd(1, 3) == 3) continue;  // regular mobs are a bit slower
        int sx = dx > 0 ? 1 : (dx < 0 ? -1 : 0), sy = dy > 0 ? 1 : (dy < 0 ? -1 : 0);
        int opts[2][2] = {{sx, 0}, {0, sy}};
        if (std::abs(dy) > std::abs(dx)) { std::swap(opts[0][0], opts[1][0]); std::swap(opts[0][1], opts[1][1]); }
        for (auto& o : opts) {
            if (!o[0] && !o[1]) continue;
            int nx = m.x + o[0], ny = m.y + o[1];
            if (inBounds(nx, ny) && at(nx, ny) == Tile::Grass && !mobAt(nx, ny) &&
                !(nx == px && ny == py)) { m.x = nx; m.y = ny; break; }
        }
    }
}

void Game::step() {
    ++turn;
    int phase = turn % CYCLE;
    if (phase == 0) {
        mobs.erase(std::remove_if(mobs.begin(), mobs.end(), [](const Mob& m) { return !m.boss; }), mobs.end());
        log("Dawn breaks. The creatures flee the light. Day " + std::to_string(day()) + ".");
    } else if (phase == DAY_LEN) {
        log("Night falls. Things stir in the muck...");
        if (day() == BOSS_DAY && !bossSpawned) {
            bossSpawned = true;
            spawnMob(true);
            log("The ground shakes: the MUCK CHIEF has arrived!");
        }
    }
    if (!isNight() && turn % 10 == 0 && hp < maxHp) ++hp;
    if (isNight() && static_cast<int>(mobs.size()) < 3 + day() * 2 && rnd(1, 100) <= 30) spawnMob(false);
    mobsAct();
}

void Game::handle(int key) {
    if (menu) {
        menu = false;
        if (key >= '1' && key <= '0' + NUM_RECIPES) craft(key - '1');
        return;
    }
    bool acted = false;
    switch (key) {
        case 'w': case 'W': acted = tryMove(0, -1); break;
        case 's': case 'S': acted = tryMove(0, 1); break;
        case 'a': case 'A': acted = tryMove(-1, 0); break;
        case 'd': case 'D': acted = tryMove(1, 0); break;
        case ' ': case '.': acted = true; break;
        case 'e': case 'E': eat(); acted = true; break;
        case 'c': case 'C': menu = true; break;
        case 'q': case 'Q': quit = true; break;
        default: break;
    }
    if (acted && !over) step();
}

void Game::render() const {
    auto col = [](int c) { return "\x1b[" + std::to_string(c) + "m"; };
    const std::string reset = "\x1b[0m", eol = "\x1b[K\n";
    bool night = isNight();
    int vx = std::clamp(px - VW / 2, 0, W - VW), vy = std::clamp(py - VH / 2, 0, H - VH);
    std::string out = "\x1b[H";
    out += col(1) + "MUCK-CPP" + reset + "  Day " + std::to_string(day()) + (night ? col(35) + " [NIGHT]" : col(33) + " [DAY]") +
           reset + "  turn " + std::to_string(turn) + eol;
    for (int y = vy; y < vy + VH; ++y) {
        for (int x = vx; x < vx + VW; ++x) {
            std::string s;
            const Mob* mob = nullptr;
            for (const auto& m : mobs) if (m.x == x && m.y == y) { mob = &m; break; }
            if (x == px && y == py) s = col(1) + col(97) + "@";
            else if (mob) s = col(mob->boss ? 95 : 91) + std::string(1, mob->glyph);
            else {
                if (night) s = col(2);
                switch (tiles[y * W + x]) {
                    case Tile::Grass: s += col(32) + "."; break;
                    case Tile::Tree: s += col(92) + "T"; break;
                    case Tile::Rock: s += col(37) + "o"; break;
                    case Tile::Ore: s += col(93) + "*"; break;
                    case Tile::Water: s += col(34) + "~"; break;
                }
            }
            out += s + reset;
        }
        out += eol;
    }
    std::string bar;
    for (int i = 0; i < 20; ++i) bar += (i * maxHp < hp * 20) ? '#' : '-';
    out += "HP [" + col(91) + bar + reset + "] " + std::to_string(hp) + "/" + std::to_string(maxHp) + eol;
    out += "Wood " + std::to_string(wood) + "  Stone " + std::to_string(stone) + "  Iron " + std::to_string(iron) +
           "  Apples " + std::to_string(apples) + eol;
    const char* picks[] = {"fists", "wooden pickaxe", "stone pickaxe"};
    out += std::string("Pick: ") + picks[pick] + "  Sword dmg: " + std::to_string(sword) +
           "  Armor: " + std::to_string(armor) + eol;
    for (size_t i = 0; i < 4; ++i)
        out += (i < msgs.size() ? "> " + msgs[i] : std::string()) + eol;
    if (menu) {
        out += col(1) + "CRAFT (press number, any other key cancels)" + reset + eol;
        for (int i = 0; i < NUM_RECIPES; ++i) {
            const Recipe& r = RECIPES[i];
            out += " " + std::to_string(i + 1) + ") " + r.name + " - wood " + std::to_string(r.wood) + ", stone " +
                   std::to_string(r.stone) + ", iron " + std::to_string(r.iron) + eol;
        }
    } else {
        out += "WASD move/attack/gather  c craft  e eat  space wait  q quit" + eol;
        out += std::string("Goal: survive and slay the Muck Chief (appears night of day ") + std::to_string(BOSS_DAY) + ")" + eol;
        out += eol;
        for (int i = 0; i < 4; ++i) out += eol;
    }
    out += "\x1b[J";
    std::fputs(out.c_str(), stdout);
    std::fflush(stdout);
}

void Game::run() {
    std::fputs("\x1b[2J", stdout);
    while (!over && !quit) {
        render();
        int key = plat::getKey();
        if (key < 0) break;
        handle(key);
    }
    render();
    if (won) std::printf("\nYou slew the Muck Chief on day %d. VICTORY!\n", day());
    else if (over) std::printf("\nYou died on day %d. The muck claims another.\n", day());
    else std::printf("\nFarewell.\n");
}

}  // namespace muck
