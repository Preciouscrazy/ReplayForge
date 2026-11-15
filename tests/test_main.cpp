#include "replayforge/replay.hpp"
#include <functional>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace replayforge;

namespace {
void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

template <typename Fn>
void require_invalid(Fn&& fn) {
    try { fn(); }
    catch (const std::invalid_argument&) { return; }
    throw std::runtime_error("Expected std::invalid_argument");
}

void initial_state_and_idle_tick() {
    State state;
    require(state.tick == 0 && state.players[0] == Player{1, 2, 100}, "Initial player A");
    require(state.players[1] == Player{4, 2, 100}, "Initial player B");
    step(state, {});
    require(state.tick == 1 && state.players[0] == Player{1, 2, 100}, "Idle tick changed position");
}

void bounded_movement() {
    State state;
    for (int i = 0; i < 20; ++i) step(state, {Action::left, Action::right});
    require(state.players[0].x == 0 && state.players[1].x == 7, "Horizontal boundaries");
    for (int i = 0; i < 20; ++i) step(state, {Action::up, Action::down});
    require(state.players[0].y == 7 && state.players[1].y == 0, "Vertical boundaries");
}

void simultaneous_attacks() {
    State state;
    step(state, {Action::right, Action::left});
    step(state, {Action::attack, Action::attack});
    require(state.players[0].hp == 90 && state.players[1].hp == 90, "Expected simultaneous damage");
}

void distant_attacks_miss() {
    State state;
    step(state, {Action::attack, Action::attack});
    require(state.players[0].hp == 100 && state.players[1].hp == 100, "Far attacks should miss");
}

void defeated_player_is_inactive() {
    State state;
    step(state, {Action::right, Action::left});
    for (int i = 0; i < 10; ++i) step(state, {Action::attack, Action::idle});
    require(state.players[1].hp == 0, "Expected opponent to be defeated");
    const auto before = state.players[1];
    step(state, {Action::idle, Action::attack});
    step(state, {Action::idle, Action::right});
    require(state.players[1] == before && state.players[0].hp == 100, "Defeated player moved or attacked");
}

void deterministic_replay() {
    const std::vector<Input> actions{{Action::right, Action::left}, {Action::attack, Action::attack},
                                     {Action::up, Action::down}, {Action::idle, Action::idle}};
    State one, two;
    for (const auto& input : actions) {
        step(one, input);
        step(two, input);
        require(one == two && fingerprint(one) == fingerprint(two), "Replay was not deterministic");
    }
    require(fingerprint(one) != fingerprint(State{}), "Hash should reflect a changed state");
}

void roundtrip_serialization() {
    std::istringstream script("ACTIONS 1\nR L\nA A\nN N\n");
    const auto actions = read_actions(script);
    require(actions.size() == 3, "Wrong action count");
    std::ostringstream output;
    write_replay(output, actions);
    std::istringstream recorded(output.str());
    const auto frames = read_replay(recorded);
    const auto check = verify(frames);
    require(check.ok() && check.checked == 3 && check.final_state.tick == 3, "Round-trip failed");
}

void mismatch_detected_at_exact_tick() {
    std::istringstream script("ACTIONS 1\nR L\nA A\nN N\n");
    std::ostringstream output;
    write_replay(output, read_actions(script));
    std::istringstream recorded(output.str());
    auto frames = read_replay(recorded);
    frames[1].expected_hash ^= 1U;
    const auto check = verify(frames);
    require(!check.ok() && check.checked == 2 && check.mismatch->tick == 1, "Wrong mismatch location");
    require(check.mismatch->expected != check.mismatch->actual, "Mismatch details incorrect");
}

void changed_input_detected() {
    std::istringstream script("ACTIONS 1\nR L\nA A\n");
    std::ostringstream output;
    write_replay(output, read_actions(script));
    std::istringstream recorded(output.str());
    auto frames = read_replay(recorded);
    frames[0].input.first = Action::idle;
    const auto result = verify(frames);
    require(result.mismatch.has_value() && result.mismatch->tick == 0, "Changed action was not detected");
}

void invalid_action_inputs_rejected() {
    require_invalid([] { std::istringstream in("ACTIONS 1\nX L\n"); (void)read_actions(in); });
    require_invalid([] { std::istringstream in("ACTIONS 1\nR\n"); (void)read_actions(in); });
    require_invalid([] { std::istringstream in("ACTIONS 1\nR L A\n"); (void)read_actions(in); });
    require_invalid([] { std::istringstream in("ACTIONS 1\n"); (void)read_actions(in); });
    require_invalid([] { std::istringstream in("ACTIONS 2\nR L\n"); (void)read_actions(in); });
}

void invalid_replay_inputs_rejected() {
    require_invalid([] { std::istringstream in("REPLAYFORGE 1\n2 R L 12\n"); (void)read_replay(in); });
    require_invalid([] { std::istringstream in("REPLAYFORGE 1\n-1 R L 12\n"); (void)read_replay(in); });
    require_invalid([] { std::istringstream in("REPLAYFORGE 1\n0 R L bananas\n"); (void)read_replay(in); });
    require_invalid([] { std::istringstream in("REPLAYFORGE 1\n0 R L 12 extra\n"); (void)read_replay(in); });
    require_invalid([] { std::istringstream in("REPLAYFORGE 1\n"); (void)read_replay(in); });
    require_invalid([] { std::istringstream in("REPLAYFORGE 2\n0 R L 12\n"); (void)read_replay(in); });
}

void empty_recording_rejected() {
    require_invalid([] { std::ostringstream out; write_replay(out, {}); });
}

void action_symbol_roundtrip() {
    for (char token : std::string("NLRUDA")) {
        require(action_symbol(parse_action(token)) == token, "Action symbol round-trip failed");
    }
}
} // namespace

int main() {
    const std::vector<std::pair<std::string, std::function<void()>>> tests{
        {"initial_state_and_idle_tick", initial_state_and_idle_tick},
        {"bounded_movement", bounded_movement},
        {"simultaneous_attacks", simultaneous_attacks},
        {"distant_attacks_miss", distant_attacks_miss},
        {"defeated_player_is_inactive", defeated_player_is_inactive},
        {"deterministic_replay", deterministic_replay},
        {"roundtrip_serialization", roundtrip_serialization},
        {"mismatch_detected_at_exact_tick", mismatch_detected_at_exact_tick},
        {"changed_input_detected", changed_input_detected},
        {"invalid_action_inputs_rejected", invalid_action_inputs_rejected},
        {"invalid_replay_inputs_rejected", invalid_replay_inputs_rejected},
        {"empty_recording_rejected", empty_recording_rejected},
        {"action_symbol_roundtrip", action_symbol_roundtrip},
    };
    int failures = 0;
    for (const auto& [name, test] : tests) {
        try { test(); std::cout << "PASS " << name << '\n'; }
        catch (const std::exception& e) { ++failures; std::cerr << "FAIL " << name << ": " << e.what() << '\n'; }
    }
    std::cout << "RESULT " << tests.size() - failures << "/" << tests.size() << " unit tests passed\n";
    return failures == 0 ? 0 : 1;
}
