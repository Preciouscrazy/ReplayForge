#include "replayforge/replay.hpp"
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

int main(int argc, char** argv) {
    if (argc != 3 && argc != 4) {
        std::cerr << "Usage: replayforge record <actions.txt> <replay.rfg>\n"
                     "       replayforge verify <replay.rfg>\n";
        return 2;
    }
    try {
        const std::string command = argv[1];
        if (command == "record" && argc == 4) {
            if (std::filesystem::absolute(argv[2]).lexically_normal() ==
                std::filesystem::absolute(argv[3]).lexically_normal()) {
                throw std::invalid_argument("Input and output paths must differ");
            }
            std::ifstream input(argv[2]);
            if (!input) throw std::runtime_error("Cannot open action file");
            const auto actions = replayforge::read_actions(input);
            std::ofstream output(argv[3]);
            if (!output) throw std::runtime_error("Cannot create replay file");
            replayforge::write_replay(output, actions);
            std::cout << "Recorded " << actions.size() << " frames to " << argv[3] << '\n';
            return 0;
        }
        if (command == "verify" && argc == 3) {
            std::ifstream input(argv[2]);
            if (!input) throw std::runtime_error("Cannot open replay file");
            const auto result = replayforge::verify(replayforge::read_replay(input));
            if (result.ok()) {
                std::cout << "PASS: " << result.checked << " replay frames verified\n";
                return 0;
            }
            const auto& mismatch = *result.mismatch;
            std::cerr << "DIVERGENCE at tick " << mismatch.tick
                      << ": expected " << mismatch.expected << ", actual " << mismatch.actual << '\n';
            return 1;
        }
        throw std::invalid_argument("Unknown command or wrong number of arguments");
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 2;
    }
}
