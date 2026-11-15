# ReplayForge — GitHub Actions verification

**Date (UTC):** 2026-11-15  
**Repository:** https://github.com/Preciouscrazy/ReplayForge  
**Commit:** `1dc2e7f54657facd026e41ac457d046dda489683`  
**GitHub Actions run:** https://github.com/Preciouscrazy/ReplayForge/actions/runs/43686211485  
**Workflow:** `.github/workflows/tests.yml` — Build and test

## Verified CI job results

| GitHub-hosted environment | Configure | Build | CTest | UTF-8/source-hygiene check |
| --- | --- | --- | --- | --- |
| Ubuntu latest | PASS | PASS | PASS | PASS |
| Windows latest | PASS | PASS | PASS | PASS |

Both jobs completed with GitHub Actions conclusion **success**.

The workflow performs:
- CMake configure: `cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug`
- CMake build: `cmake --build build --config Debug`
- CTest: `ctest --test-dir build -C Debug --output-on-failure`
- Source-hygiene scan: `python tools/check_text.py`

The CTest suite includes `core_tests` (the 13 C++ unit tests) and `golden_replay` (the committed replay fixture). User-run Windows verification separately showed **13/13 unit tests** and **2/2 CTest checks** on Visual Studio 2026 / MSVC 19.51. Earlier Linux local evidence includes GCC 14 and Clang 17 runs.

## Scope and limitations

CI verifies the code and test fixtures on the hosted runners at the linked commit. It does not establish full gameplay correctness, end-to-end production readiness, authorship, or the absence of all possible watermark/provenance signals. The UTF-8 scan checks a defined set of source-hygiene issues only.

Earlier evidence logs are preserved as dated records; their statements that CI had not yet run were true when recorded, before this verification.
