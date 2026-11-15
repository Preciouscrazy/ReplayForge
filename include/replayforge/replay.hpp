#pragma once
#include "replayforge/engine.hpp"
#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <optional>
#include <vector>

namespace replayforge {

struct Frame {
    Input input;
    std::uint64_t expected_hash = 0;
};

struct Mismatch {
    std::size_t tick;
    std::uint64_t expected;
    std::uint64_t actual;
};

struct Verification {
    std::size_t checked = 0;
    std::optional<Mismatch> mismatch;
    State final_state;
    [[nodiscard]] bool ok() const noexcept { return !mismatch.has_value(); }
};

std::vector<Input> read_actions(std::istream& source);
void write_replay(std::ostream& destination, const std::vector<Input>& actions);
std::vector<Frame> read_replay(std::istream& source);
Verification verify(const std::vector<Frame>& frames);

} // namespace replayforge
