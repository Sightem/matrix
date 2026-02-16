# Contributing

## Technical Overview

Matrix is split into two layers:

- **matrix_core**: a platform agnostic C++20 library that provides exact rational matrix arithmetic, 21 operations, and a type erased on demand explanation system. it has zero heap allocations during operation (everything runs on a preallocated arena). fully tested with native sanitizer builds (ASan, UBSan, LSan)

- **matrix_shell**: the TI-84+ CE front end. A page stack state machine drives the UI using the CEs graphics library, keyboard library, and font library. LaTeX rendering is handled by the bundled **libtexce** engine

### Memory Architecture

The calculator has roughly **60 KB** of usable RAM and **4 KB** of stack space. Matrix's math engine uses an 18 KiB slab allocated once at startup split into two monotonic bump arenas:

| Arena | Size | Purpose |
|---|---|---|
| **persist** | 9 KiB | Long lived matrix slot storage + ephemeral explanation contexts |
| **scratch** | 9 KiB | Temporary working memory, reset per-operation or per step render |

There are **zero heap allocations** during normal operation — the single `malloc` happens at startup. This was verified with Valgrind/Massif profiling.

### Design Decisions

- **View/handle pattern** for matrices: a `MatrixView` is just a pointer + dimensions (~24 bytes). Backing storage (`Rational[36]` = 576 bytes per slot) lives in the arena. Copying a matrix copies metadata, not data
- **Exact Rational type**: 64 bit numerator and denominator, auto reduced to lowest terms. No floating point anywhere in the math pipeline
- **On demand step generation**: explanation objects are type erased closures. the shell calls `render_step(i)` only when the user navigates to step *i*. only one steps LaTeX is in memory at a time
- **Compile time feature flags**: optional operations (Cramer, cofactor, minor matrix, projection) can be toggled off to reduce binary size for the CE


## Building from Source

### Prerequisites

- **CMake** .= 3.20
- **Ninja** build system
- **Clang** for native builds
- **[CE C/C++ Toolchain](https://ce-programming.github.io/toolchain/)** (for TI84 CE builds)

### Native Build (for development and testing)

```bash
cmake --preset native
cmake --build --preset native
```

This builds the core library and all test executables with strict warnings (`-Werror`) enabled

### TI84 CE Build (produces the .8xp program)

```bash
cmake --preset ce
cmake --build --preset ce
```

The output `MATRIX.8xp` is located in `build/ce/matrix_shell/bin/`.

### Sanitizer Builds

Several sanitizer presets are available for catching memory and undefined behavior bugs

```bash
cmake --preset native-asan && cmake --build --preset native-asan

cmake --preset native-ubsan && cmake --build --preset native-ubsan

cmake --preset native-lsan && cmake --build --preset native-lsan

cmake --preset native-coverage && cmake --build --preset native-coverage
```

### Feature Flags

Optional operations can be toggled at configure time to reduce binary size:

| Flag | Default (native) | Default (CE) | Description |
|---|---|---|---|
| `MATRIX_FEATURE_PROJECTION` | ON | OFF | Vector projection decomposition |
| `MATRIX_FEATURE_MINOR_MATRIX` | ON | OFF | Full matrix of minors |
| `MATRIX_FEATURE_COFACTOR` | ON | OFF | Single-element cofactor / minor |
| `MATRIX_FEATURE_CRAMER` | ON | OFF | Cramer's rule solver |

Example: enable Cramer's rule on the CE build:

```bash
cmake --preset ce -DMATRIX_FEATURE_CRAMER=ON
cmake --build --preset ce
```

## Testing

### Running Tests

```bash
cmake --preset native
cmake --build --preset native
ctest --preset native
```

### Test Suites

| Test Suite | What It Covers |
|---|---|
| `test_rational` | Rational number arithmetic, reduction, edge cases |
| `test_matrix_ops` | Add, subtract, multiply, transpose |
| `test_det` | Determinant computation and step generation |
| `test_rref` | REF and RREF with step verification |
| `test_inverse` | Matrix inverse via Gauss-Jordan |
| `test_cramer` | Cramer's rule solution and Δ/Δ\_i explanations |
| `test_vectors` | Dot product, cross product, projection |
| `test_cofactor_element` | Single cofactor/minor with step rendering |
| `test_minor_matrix` | Full matrix of minors |
| `test_spaces` | Column/row/null space basis extraction |
| `test_arena_guards` | Arena allocator boundary checks |

### CE Target Tests

The core can also be compiled and run directly on [CEmu](github.com/CE-Programming/CEmu) as individual .8xp test programs:

```bash
cmake --preset ce -DMATRIX_BUILD_CE_TESTS=ON
cmake --build --preset ce
```

These are somewhat scuffed, I have not bothered setting up autotests. you are reliant on the output you see on the emulator.
