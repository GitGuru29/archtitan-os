# Contributing to ArchTitan OS

Thank you for contributing to ArchTitan OS. To maintain system determinism, minimal bloat, and stability, please review these guidelines before submitting code.

## Development Principles
* **Performance & Memory:** Zero unnecessary allocations in hot paths. Daemons must remain lean and deterministic.
* **Modern C++:** Follow modern C++ practices (RAII, smart pointers, explicit memory lifecycles) when touching core daemons.
* **Style:** Keep code format consistent with the existing codebase (clang-format rules apply where present).

## How to Contribute
1. **Discuss First:** For architectural changes or new daemons, open a discussion under the **Discussions** tab before writing substantial code.
2. **Fork & Branch:** Create a feature branch off `main` (`feature/daemon-telemetry` or `fix/socket-flush`).
3. **Commit Messages:** Write clear, imperatively phrased commit logs (e.g., `Fix memory leak in network sync buffer`, not `fixed bug`).
4. **Pull Requests:** Provide a clean summary of what changed, relevant benchmarks, and test results.
