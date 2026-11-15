#include "replayforge/engine.hpp"
#include <algorithm>
#include <cstdlib>
#include <stdexcept>

namespace replayforge {
namespace {
constexpr int kGridMax = 7;
constexpr int kDamage = 10;

void move(Player& p, Action action) {
    if (p.hp == 0) return;
    switch (action) {
    case Action::left:  p.x = std::max(0, p.x - 1); break;
    case Action::right: p.x = std::min(kGridMax, p.x + 1); break;
    case Action::up:    p.y = std::min(kGridMax, p.y + 1); break;
    case Action::down:  p.y = std::max(0, p.y - 1); break;
    default: break;
    }
}

bool adjacent(const Player& a, const Player& b) {
    return std::abs(a.x - b.x) + std::abs(a.y - b.y) <= 1;
}
} // namespace

void step(State& state, Input input) {
    auto& a = state.players[0];
    auto& b = state.players[1];
    move(a, input.first);
    move(b, input.second);
    const bool hit_a = a.hp > 0 && input.first == Action::attack && adjacent(a, b);
    const bool hit_b = b.hp > 0 && input.second == Action::attack && adjacent(a, b);
    if (hit_a) b.hp = std::max(0, b.hp - kDamage);
    if (hit_b) a.hp = std::max(0, a.hp - kDamage);
    ++state.tick;
}

std::uint64_t fingerprint(const State& state) noexcept {
    // FNV-1a over explicitly ordered 32-bit values, independent of struct padding.
    std::uint64_t hash = 14695981039346656037ULL;
    auto mix = [&hash](std::uint32_t value) {
        for (int byte = 0; byte < 4; ++byte) {
            hash ^= (value >> (byte * 8)) & 0xffU;
            hash *= 1099511628211ULL;
        }
    };
    mix(state.tick);
    for (const auto& p : state.players) {
        mix(static_cast<std::uint32_t>(p.x));
        mix(static_cast<std::uint32_t>(p.y));
        mix(static_cast<std::uint32_t>(p.hp));
    }
    return hash;
}

Action parse_action(char symbol) {
    switch (symbol) {
    case 'N': return Action::idle;
    case 'L': return Action::left;
    case 'R': return Action::right;
    case 'U': return Action::up;
    case 'D': return Action::down;
    case 'A': return Action::attack;
    default: throw std::invalid_argument("Invalid action symbol");
    }
}

char action_symbol(Action action) noexcept {
    switch (action) {
    case Action::left: return 'L';
    case Action::right: return 'R';
    case Action::up: return 'U';
    case Action::down: return 'D';
    case Action::attack: return 'A';
    default: return 'N';
    }
}
} // namespace replayforge
