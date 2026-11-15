#pragma once
#include <array>
#include <cstdint>

namespace replayforge {

enum class Action : std::uint8_t { idle, left, right, up, down, attack };

struct Input {
    Action first = Action::idle;
    Action second = Action::idle;
    bool operator==(const Input&) const = default;
};

struct Player {
    int x = 0;
    int y = 0;
    int hp = 100;
    bool operator==(const Player&) const = default;
};

struct State {
    std::array<Player, 2> players{{{1, 2, 100}, {4, 2, 100}}};
    std::uint32_t tick = 0;
    bool operator==(const State&) const = default;
};

void step(State& state, Input input);
std::uint64_t fingerprint(const State& state) noexcept;
Action parse_action(char symbol);
char action_symbol(Action action) noexcept;

} // namespace replayforge
