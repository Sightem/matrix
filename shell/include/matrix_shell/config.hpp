#pragma once

// -----------------------------------------------------------------------------
// Feature flags
// -----------------------------------------------------------------------------
// these are used to keep the build under size limits by compiling
// out rarely needed functionality
//
#ifndef MATRIX_SHELL_ENABLE_PROJECTION
#define MATRIX_SHELL_ENABLE_PROJECTION 1
#endif
#ifndef MATRIX_SHELL_ENABLE_MINOR_MATRIX
#define MATRIX_SHELL_ENABLE_MINOR_MATRIX 1
#endif
#ifndef MATRIX_SHELL_ENABLE_COFACTOR
#define MATRIX_SHELL_ENABLE_COFACTOR 1
#endif
#ifndef MATRIX_SHELL_ENABLE_CRAMER
#define MATRIX_SHELL_ENABLE_CRAMER 1
#endif

#ifndef MATRIX_SHELL_ENABLE_DEBUG
#define MATRIX_SHELL_ENABLE_DEBUG 1
#endif

#ifndef MATRIX_SHELL_LANG_FR
#define MATRIX_SHELL_LANG_FR 0
#endif
