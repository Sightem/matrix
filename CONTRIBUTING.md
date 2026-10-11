# Contributing

## Technical Overview

Matrix is split into two layers:

- **matrix_core**: a platform-agnostic C++20 library providing exact rational matrix arithmetic and on-demand explanations. Callers supply matrix storage; the core does not allocate memory.

- **matrix_shell**: the TI-84+ CE front end. A page stack state machine drives the UI using the CE's graphics and keyboard libraries. The bundled **libtexce** engine loads its Latin Modern font AppVars through `fileioc`; its font handles live with the application, and palette entries 253–254 are reserved for antialiasing.

### Memory Architecture

The calculator has roughly **60 KB** of usable RAM and **4 KB** of stack space. The static application owns fixed-capacity buffers with variable matrix shapes:

| Storage | Rational cells | Bytes | Lifetime |
|---|---:|---:|---|
| Eight 6×6 slots | 288 | 4,608 | Application |
| Current result: six-variable particular solution plus null-space basis | 42 | 672 | Current calculation |
| Retained `[A|b]` or `Aᵀ` for explanation replay | 42 | 672 | Current calculation |
| Reusable work buffer, large enough for 6×12 `[A|I]` | 72 | 1,152 | Overwritten by each operation or replay |

The matrix buffers total **7,104 bytes**, plus inline explanation metadata and slot dimensions. They do not consume the calculator stack or require allocation, rollback, or arena marks. Cramer's result stays in the result buffer while its selected determinant explanation changes independently.

The shell also owns a separate 20 KiB libtexce renderer and allocates a TeX layout when the displayed step changes; those renderer allocations are separate from the mathematical storage.

### Design Decisions

- **Matrix views**: a `MatrixView` is a pointer, dimensions, and stride. Output and work storage belong to the caller. `matrix_work_view` checks capacity and returns a narrowed view without clearing or allocating memory. Empty bases retain zero rows or columns and null data; inverse work can hold `[A | I]`.
- **Exact Rational type**: 64 bit numerator and denominator, auto reduced to lowest terms. No floating point anywhere in the math pipeline
- **On-demand step generation**: `Explanation` holds tagged metadata by value. Passing a null explanation pointer disables step metadata. Borrowed matrices must remain alive and unchanged; the RREF solver retains `[A|b]` separately from its results. `render_step(i)` reconstructs the requested step in `StepRenderBuffers::work`, so navigation does not retain matrix snapshots.
- **Compile time feature flags**: optional operations (Cramer, cofactor, minor matrix, projection) can be toggled off to reduce binary size for the CE
- **Core solving, typed results**: `op_solve_rref` returns the particular solution, null-space basis, and rank information separately. The shell's operation-tagged result union has named scalar, predicate, basis, solution, and projection payloads. Projection stores two matrix views and an exact rational coefficient in the common result page.
- **Bounded formatting**: `Writer` retains its first error and stops writing; formatting callers inspect `status()` once. `CheckedWriter` uses the same implementation for shell buffers whose capacities are established by the UI contract.
- **Shell flow**: operand selection feeds one operation switch, error presenter, and successful result transition. Menus share one entry type; menus, slot pickers, and the Cramer selector share list navigation and highlighting.
- **Localization**: callers use `tr(TextId::...)`. `text_catalog.inc` supplies enum identifiers and English/French strings without a separate string-key language.


## Building from Source

### Prerequisites

- **CMake** .= 3.20
- **Ninja** build system
- **Clang** for native builds
- **[CE C/C++ Toolchain](https://ce-programming.github.io/toolchain/)** (for TI84 CE builds)

The eZ80 compiler must include [the frame-index spill/add fix](https://github.com/CE-Programming/llvm-project/commit/b88a5b408f48). Earlier builds can reload deep stack slots from the wrong address and reset the calculator during operand selection; native tests cannot detect a target-code-generation bug.

Calculator builds stay CMake-based and require the CEdev v17+ binutils pipeline. The older fasmg backend and its configuration have been removed.

### Native Build (for development and testing)

```bash
cmake --preset native
cmake --build --preset native
```

This builds `matrix_core` once as a static library and links the separate test executables and profiler against it. Strict warnings (`-Werror`) and test assertions remain enabled.

### TI84 CE Build (produces the .8xp program)

```bash
cmake --preset ce
cmake --build --preset ce
```

The output `MATRIX.8xp` is located in `build/ce/matrix_shell/bin/`.

Set `CEDEV_ROOT` in the environment or pass `-DCEDEV_ROOT=/path/to/CEdev` when configuring if the toolchain is not installed at `~/CEdev`. Compiler/linker overrides use `CEDEV_C_COMPILER`, `CEDEV_CXX_COMPILER`, and `CEDEV_LINKER`; presets contain no workstation-specific paths.

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

Tests keep representative numerical results and distinct failure mechanisms, rather than every branch or every sample. Shared errors, storage boundaries, and replay invariants are checked once where practical; renderer wording, internal null-guard probes, and duplicate examples are not coverage targets. The CE programs reuse the same test sources.

| Test Suite | What It Covers |
|---|---|
| `test_rational` | Sign normalization, fractional arithmetic, division by zero, and signed overflow |
| `test_matrix_ops` | Arithmetic and transpose results, shape rejection, workspace stride/capacity, and bounded-writer truncation |
| `test_det` | Singular determinant, pivot-swap sign, and square-input constraint |
| `test_rref` | Distinct REF/RREF results and reproducible replay after workspace reuse |
| `test_inverse` | Fractional inverse, singular rejection, and the 6×12 workspace boundary |
| `test_cramer` | One system's solution and replacement determinants; inputs/results preserved during replay |
| `test_vectors` | Mixed-orientation dot/cross products, projection components/norms, and zero-vector rejection |
| `test_cofactor_element` | Cofactor sign, 1×1 definition, and index boundary |
| `test_minor_matrix` | One complete matrix of minors and the minimum supported size |
| `test_spaces` | Basis selection, empty bases, unique/dependent/inconsistent solves, and saved-input/result lifetimes |

### CE Target Tests

The core can also be compiled and run directly on [CEmu](github.com/CE-Programming/CEmu) as individual .8xp test programs:

```bash
cmake --preset ce -DMATRIX_BUILD_CE_TESTS=ON
cmake --build --preset ce
```
