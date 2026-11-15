#include "replayforge/replay.hpp"
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <system_error>

namespace {
namespace fs = std::filesystem;

// Record to a sibling file first; a failed write must not leave a partial replay.
void save_replay(const fs::path& path, const std::vector<replayforge::Input>& actions) {
    if (fs::exists(path) || fs::is_symlink(path)) {
        throw std::invalid_argument("Refusing to overwrite existing output");
    }

    fs::path temporary = path;
    temporary += ".tmp";
    if (fs::exists(temporary) || fs::is_symlink(temporary)) {
        throw std::invalid_argument("Temporary output already exists");
    }

    // Clean up an incomplete staged file if serialization, flush or rename fails.
    struct RemoveOnExit {
        fs::path path;
        ~RemoveOnExit() {
            std::error_code error;
            fs::remove(path, error);
        }
    } cleanup{temporary};

    {
        std::ofstream output(temporary, std::ios::binary);
        if (!output) throw std::runtime_error("Cannot create temporary replay file");
        replayforge::write_replay(output, actions);
        output.close();
        if (!output) throw std::runtime_error("Could not finish writing replay");
    }

    if (fs::exists(path) || fs::is_symlink(path)) {
        throw std::invalid_argument("Output was created before replay was finalized");
    }
    fs::rename(temporary, path);
}
} // namespace

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
            save_replay(argv[3], actions);
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
