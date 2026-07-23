# Compatibility notes for external graders and course runners

This repository implements the HULK compiler. The course runner / graders expect a few simple conventions so they can build and run the compiler automatically. This document explains those conventions and how this repository satisfies them.

1) Executable location and name

- Expected: an executable named `./hulk` at the repository root.
- This repo's Makefile builds an executable named `hulk` when you run `make build` or `make`.

2) Exit codes and CLI behavior

- The runner expects that the compiler can be invoked as `./hulk <source-file.hulk>` and that it returns a zero exit code for successful runs (even if the program being compiled prints nothing), and a non-zero exit code when the compiler crashes or there is a runtime error.
- The compatibility test script `tests/hulk/run_tests.sh` invokes `./hulk <file>` for each file in `tests/input/` and treats non-zero exit codes as test failures.

3) Build invocation

- The runner will often attempt to build the project; the compatibility script will call `make build` (or `make`) when `./hulk` isn't present. If you use a different build workflow, update either the Makefile or the script.

4) Tests

- The `tests/` directory contains many input `.hulk` files used by the script. The script ensures the compiler can run each input without crashing. It does not assert program output (but can be extended to do so).

5) CI

- If you want automated checks, add a GitHub Actions workflow that runs `tests/hulk/run_tests.sh` on push and PR. (I can add one if you want.)

6) Troubleshooting

- If `./hulk` cannot be produced by `make`, ensure the Makefile's `TARGET`, `SOURCES` and compile flags match your project organization. The current Makefile builds all .cpp files in `src/` and places object files under `build/`.


Repository-specific notes

- This repo already contains a `Makefile` with a `build` target that produces a `hulk` binary. The compatibility script tries `make build` first, then `make` as a fallback.
