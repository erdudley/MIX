# MIX

A C11 interpreter for Donald Knuth's MIX computer, with a two-pass MIXAL assembler and test programs based on *The Art of Computer Programming* (Josephus problem, Vol. 1 Ex. 1.3.2–22).

## Features

- **4000-word memory** and full register file (rA, rX, rJ, rI1–rI6)
- **MIXAL assembler** — labels, `EQU`/`ORIG`/`CON`/`ALF`/`END`, literals (`=N=`, `=text=`), indexed addressing (`X`, `A`, numeric index registers), `JMP *`, and subroutine linkage via `STJ`/`JMP`
- **Instruction execution** — arithmetic, comparisons, jumps, shifts, field operations, I/O (`IN`/`OUT`), and `HLT`
- **Test suite** — unit tests plus integration tests that assemble and run `.mix` programs against expected output

## Requirements

- CMake 3.16+
- A C11 compiler (GCC or Clang)
- Optional: [Valgrind](https://valgrind.org/) for memory checking

## Build

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

CMake options:

| Option | Default | Description |
|--------|---------|-------------|
| `MIX_WARNINGS_AS_ERRORS` | `OFF` | Treat warnings as errors |
| `MIX_ENABLE_SANITIZERS` | `OFF` | Enable ASan/UBSan in debug builds |

## Run a program

```bash
./build/mix-run tests/data/1.mix
```

`mix-run` assembles the source file, loads it into memory, runs until `HLT`, and prints captured output to stdout.

Example — compare benchmark output to the expected file:

```bash
./build/mix-run tests/data/3.mix
diff tests/data/3.expected <(./build/mix-run tests/data/3.mix)
```

## Tests

```bash
./build/tests/mix-tests
```

Or via CTest:

```bash
cd build && ctest --output-on-failure
```

The suite covers:

| Suite | What it checks |
|-------|----------------|
| `memory` | Word encoding, load/store, fields |
| `registers` | Register read/write |
| `interpreter` | `HLT`, step limits |
| `josephus` | `tests/data/1.mix` — Josephus ranks (n=24, m=11) |
| `benchmark` | `tests/data/2.mix` — checksum benchmark |
| `benchmark3` | `tests/data/3.mix` — full dynamic simulation benchmark |

## Valgrind

With Valgrind installed:

```bash
cmake --build build --target memcheck
```

Logs are written to `build/valgrind/`. If Valgrind is not found, the `memcheck` target prints a short notice instead of running.

## Performance benchmarks

```bash
cmake --build build --target perf-report
```

This runs `mix-perf` and writes a report to `build/perf_report.txt`.

## Project layout

```
include/mix/     Public headers
src/             Core library (memory, registers, instructions, I/O, assembler, interpreter)
src/main.c       mix-run CLI
tests/           Unit and integration tests
tests/data/      MIXAL programs and expected output
perf/            Micro-benchmark harness
cmake/           Compiler warnings and Valgrind helpers
```

## Test programs

| File | Description |
|------|-------------|
| `tests/data/1.mix` | Josephus problem — prints execution ranks |
| `tests/data/2.mix` | Benchmark with hardcoded rank table and checksum |
| `tests/data/3.mix` | Full benchmark — subroutines, linked-list simulation, decimal print, `DIV`/`MUL`/`CHAR`, multi-line `OUT` |

## License

No license file is included yet. Add one if you plan to distribute this project.