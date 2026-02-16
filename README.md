<h1 align="center">Step by Step Linear Algebra for the TI84+ CE</h1>

<p align="center">
  <img src="assets/animated.png" alt="Matrix page" />
  &nbsp;&nbsp;
  <img src="assets/operations.png" alt="Operations menu" />
  &nbsp;&nbsp;
  <img src="assets/RREF.png" alt="RREF steps" />
</p>

A linear algebra program for the TI-84 Plus CE graphing calculator. **Matrix** automates the mechanical parts of linear algebra, such as computing determinants, row reductions, inverses, Cramer's rule, and more, while **showing every step** so you can follow along

Steps are rendered as typeset LaTeX directly, powered by the bundled [libtexce](https://github.com/Sightem/libtexce) rendering engine

---

- [Features](#features)
- [Getting Started](#getting-started)
- [How to Use](#how-to-use)
- [Technical Overview](#technical-overview)
- [Project Structure](#project-structure)
- [Building from Source](#building-from-source)
- [Testing](#testing)

---

Linear algebra courses require you to work through lengthy and repetitive calculations by hand. **Matrix** does the calculations for you and shows you its steps:

- Enter integer matrices up to **6x6** using a familiar spreadsheet style editor
- Pick an operation from a menu and get the result instantly
- Tap a key to page through **step by step LaTeX explanations** of the entire computation
- **Exact rational arithmetic** results are displayed as fractions
- Runs natively on your TI-84+ CE

## Features

### Matrix Operations

| Operation | Description |
|---|---|
| **Add / Subtract** | Element-wise matrix addition and subtraction |
| **Multiply** | Full matrix multiplication with dot-product steps |
| **Transpose** | Transpose of any matrix |
| **Determinant** | Cofactor expansion with smart row/column selection |
| **Inverse** | Gauss-Jordan elimination on the augmented matrix [A \| I] |
| **REF / RREF** | Row Echelon Form and Reduced Row Echelon Form |
| **Cramer's Rule** | Solve Ax = b with per-Δ step breakdowns |
| **Minor / Cofactor** | Compute M\_{ij} and C\_{ij} for any element |
| **Minor Matrix** | Full matrix of minors |

### Vector Operations

| Operation | Description |
|---|---|
| **Dot Product** | Scalar dot product of two vectors |
| **Cross Product** | 3D cross product |
| **Projection** | Orthogonal projection of **u** onto **v** plus the orthogonal complement |

### Subspace Analysis

| Operation | Description |
|---|---|
| **Column Space Basis** | Basis for Col(A) via pivot columns |
| **Row Space Basis** | Basis for Row(A) via non zero rows of RREF |
| **Null Space Basis** | Basis for Null(A) from free variables |
| **Left Null Space Basis** | Basis for Null(Aᵀ) |
| **Span Test** | Test whether a vector is in the span of a set or R^m |
| **Independence Test** | Test whether a set of vectors is linearly independent |
| **Solve via RREF** | Solve a linear system using row reduction |

### Step by Step Explanations

Every operation generates a detailed explanation:

- Navigate forward and backward through steps with the **left/right arrow keys**
- Each step shows a plain text caption (e.g., `R_2 ← R_2 − 2R_2`) and a **LaTeX rendered matrix or expression**
- For multi part operations like Cramers rule, a sub menu lets you inspect each individual Δ\_i determinant

## Getting Started

### Requirements

- A **TI-84 Plus CE** or **TI-84 Plus CE Python Edition**
- A USB cable and software to transfer files to the calculator [Ti connect CE](https://education.ti.com/en/products/computer-software/ti-connect-ce-sw) for windows and [webtilp](https://tiplanet.org/scripts/webtilibs/webtilp.html) for MacOS/Linux/Chromebooks

### Installation

1. Download the latest **MATRIX.8xp** from the Releases page
2. Connect your calculator to your computer via USB
3. Get clibs from https://tiny.cc/clibs
4. If you are on an OS above version 5.4, you need [arTIfiCE](https://yvantt.github.io/arTIfiCE/) to launch the program (this is likely the case)
3. Open TI Connect CE or webtilp and send `MATRIX.8xp` to your calculator, as well as clibs and arTIfiCE (make sure to choose ARCHIVE when prompted)

### Quick Start

1. From the main menu, select **Matrices** -> **Edit A**
2. Set the dimensions (e.g., 3x3) using the arrow keys then press **[enter]**
3. Navigate to each cell with the arrow keys, press **[enter]** to type a value and confirm with **[enter]**
4. Press **[clear]** to finish editing, then go to **Operations** -> **Determinant**
5. Select slot **A**. the determinant is computed and displayed
6. Press a key to view the **step by step** cofactor expansion, rendered in LaTeX
7. Use **left/right arrows** to page through steps; **[clear]** to go back

---

## How to Use

### Navigation

| Key | Action |
|---|---|
| **UP / DOWN** | Scroll through menus and matrix rows |
| **LEFT / RIGHT** | Navigate matrix columns; page through steps |
| **[enter]** | Select a menu item, confirm input, or edit a cell |
| **[clear]** | Go back one screen |
| **[del]** | Set the current cell to 0 (in the editor) |
| **[2nd]** | Finish editing a matrix |

### Editing Matrices

- Dimensions are set with an arrow key selector (1-6 for both rows and columns)
- The grid editor shows all cells at once
- Cell values are entered in a footer input line (supports multi digit numbers and negative signs)
- Resizing a matrix clears its contents

### Step Viewer

- Steps are rendered one at a time using LaTeX. each page shows a caption and a typeset formula or matrix
- The step counter (e.g., `3/7`) tells you where you are
- For complex operations (Cramer's rule), a sub menu lets you view each sub computation


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

## Testing

### Running Tests

```bash
cmake --preset native
cmake --build --preset native
ctest --preset native
```

### Coverage

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