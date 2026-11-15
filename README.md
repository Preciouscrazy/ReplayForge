# ReplayForge

**Deterministic game replay and divergence detection | C++20 | CMake + CTest**

ReplayForge is a compact, standalone exercise in reproducible gameplay behavior. It simulates two players on an 8x8 grid, records one action per player per tick, and verifies the complete resulting state against a stored checksum. A changed action or result can be located at the first divergent tick.

This is **independent educational code** with original placeholder mechanics, not a Riot Games product, client integration, anti-cheat tool, or networked game.

## Build and run

Requires CMake 3.20+ and a C++20 compiler. No external runtime dependencies.

### Windows PowerShell (Visual Studio 2026)

```powershell
cmake -S . -B build -G "Visual Studio 18 2026"
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
.\build\Debug\replayforge.exe record data\sample_actions.txt sample_trace.rfg
.\build\Debug\replayforge.exe verify sample_trace.rfg
python tools\check_text.py
```

### Linux/macOS

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
./build/replayforge record data/sample_actions.txt sample_trace.rfg
./build/replayforge verify sample_trace.rfg
python3 tools/check_text.py
```

If another Visual Studio version is installed, use your installed CMake generator instead. Output executables may vary by generator/configuration.

## Inputs, outputs, and failure behavior

An action file starts with `ACTIONS 1`, followed by exactly two action symbols per tick (first and second player): `N` idle, `L` left, `R` right, `U` up, `D` down, and `A` attack. The bundled sample has eight ticks.

A replay begins with `REPLAYFORGE 1`. Each subsequent line contains `tick first_action second_action state_checksum`. The checksum is deterministic **FNV-1a over explicit state fields**, not a cryptographic integrity guarantee. Replays reject invalid input, missing headers, nonsequential ticks, and malformed action symbols.

- `record` saves a text replay from an action file.
- `verify` returns exit code `0` for a matching replay, `1` for a state divergence, and `2` for invalid input or an I/O error.
- The CLI reports the **first divergent tick** for troubleshooting.

For a negative test, copy `data/sample_trace.rfg`, replace a stored checksum with `0`, then run `verify` on the copy. It should return a divergence rather than success.

## Evidence and test coverage

This starter includes **13 C++ unit tests** and **2 CTest checks** (unit suite + committed golden replay). Tests cover movement bounds, simultaneous attacks, inactive defeated players, deterministic state evolution, in-memory record/replay round trips, tampering, malformed input, and file-format validation.

**Cross-platform CI verified:** GitHub Actions successfully configured, built, ran CTest, and completed the text-hygiene check on both **Ubuntu** and **Windows** for commit [`1dc2e7f`](https://github.com/Preciouscrazy/ReplayForge/commit/1dc2e7f54657facd026e41ac457d046dda489683). See the [passing CI run](https://github.com/Preciouscrazy/ReplayForge/actions/runs/43686211485), [CI verification record](evidence/github-actions-validation.md), [local Linux validation](evidence/local-validation.txt), and [Windows validation](evidence/windows-validation.md).

## Source layout

```text
include/replayforge/engine.hpp    gameplay rules and stable state interface
include/replayforge/replay.hpp    replay parsing and verification interface
src/engine.cpp                   pure gameplay rules and state checksum
src/replay.cpp                   read/write replay and divergence checks
src/main.cpp                     CLI error handling
tests/test_main.cpp              no third-party test framework
data/                            sample actions + expected replay
.github/workflows/tests.yml      cross-platform CI configuration
tools/check_text.py              source-encoding and invisible-character scan
docs/design.md                   architecture, constraints, tradeoffs
```

## Key design decisions

Tick updates use **integer coordinates and integer health**, eliminating floating-point and RNG differences in this narrow simulation. Inputs are recorded explicitly. State hashes iterate fields rather than hashing struct memory or padding. The test data is intentionally fictional; nothing accesses game accounts or proprietary assets.

The rules intentionally omit collisions, physics, client/server networking, and rollback prediction. Multiple actors can occupy or cross the same grid cell. The file cap is 100,000 ticks. For a next version, build an independently testable network-delay simulator rather than adding unrelated UI features to this repository.

## Source hygiene and provenance

All deliverables are plain text and generated sample data, with no bundled images or tracking code. `tools/check_text.py` checks for common UTF-8 BOMs and selected invisible control characters; **it is not an AI-authorship detector or exhaustive watermark certification**. Review and understand the implementation yourself, preserve accurate provenance and attribution, and follow the relevant application/disclosure rules.
