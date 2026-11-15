#include "replayforge/replay.hpp"
#include <charconv>
#include <istream>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace replayforge {
namespace {
constexpr std::size_t kMaxFrames = 100000;

bool next_line(std::istream& in, std::string& line) {
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (!line.empty()) return true;
    }
    return false;
}

void expect_header(std::istream& in, const char* expected) {
    std::string line;
    if (!next_line(in, line) || line != expected) {
        throw std::invalid_argument("Missing or unsupported file header");
    }
}

std::uint64_t parse_uint(const std::string& text) {
    if (text.empty()) throw std::invalid_argument("Empty integer");
    std::uint64_t value{};
    const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (error != std::errc{} || end != text.data() + text.size()) {
        throw std::invalid_argument("Invalid unsigned integer: " + text);
    }
    return value;
}

Input parse_pair(const std::string& first, const std::string& second) {
    if (first.size() != 1 || second.size() != 1) {
        throw std::invalid_argument("Each action must be one character");
    }
    return {parse_action(first[0]), parse_action(second[0])};
}
} // namespace

std::vector<Input> read_actions(std::istream& source) {
    expect_header(source, "ACTIONS 1");
    std::vector<Input> actions;
    std::string line;
    while (next_line(source, line)) {
        std::istringstream fields(line);
        std::string first, second, extra;
        if (!(fields >> first >> second) || fields >> extra) {
            throw std::invalid_argument("Action row must contain exactly two symbols");
        }
        if (actions.size() >= kMaxFrames) throw std::invalid_argument("Too many actions");
        actions.push_back(parse_pair(first, second));
    }
    if (actions.empty()) throw std::invalid_argument("Action file is empty");
    return actions;
}

void write_replay(std::ostream& destination, const std::vector<Input>& actions) {
    if (actions.empty() || actions.size() > kMaxFrames) {
        throw std::invalid_argument("Replay must have 1 to 100000 frames");
    }
    State state;
    destination << "REPLAYFORGE 1\n";
    for (std::size_t tick = 0; tick < actions.size(); ++tick) {
        step(state, actions[tick]);
        destination << tick << ' ' << action_symbol(actions[tick].first) << ' '
                    << action_symbol(actions[tick].second) << ' ' << fingerprint(state) << '\n';
    }
    if (!destination) throw std::runtime_error("Failed writing replay");
}

std::vector<Frame> read_replay(std::istream& source) {
    expect_header(source, "REPLAYFORGE 1");
    std::vector<Frame> frames;
    std::string line;
    while (next_line(source, line)) {
        std::istringstream fields(line);
        std::string tick, first, second, hash, extra;
        if (!(fields >> tick >> first >> second >> hash) || fields >> extra) {
            throw std::invalid_argument("Replay row must have tick, two actions, and hash");
        }
        if (frames.size() >= kMaxFrames) throw std::invalid_argument("Too many frames");
        if (parse_uint(tick) != frames.size()) throw std::invalid_argument("Replay ticks must be consecutive from zero");
        frames.push_back({parse_pair(first, second), parse_uint(hash)});
    }
    if (frames.empty()) throw std::invalid_argument("Replay has no frames");
    return frames;
}

Verification verify(const std::vector<Frame>& frames) {
    Verification result;
    for (std::size_t tick = 0; tick < frames.size(); ++tick) {
        step(result.final_state, frames[tick].input);
        const auto actual = fingerprint(result.final_state);
        result.checked = tick + 1;
        if (actual != frames[tick].expected_hash) {
            result.mismatch = Mismatch{tick, frames[tick].expected_hash, actual};
            return result;
        }
    }
    return result;
}
} // namespace replayforge
