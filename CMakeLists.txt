cmake_minimum_required(VERSION 3.20)

project(matrix_app_core C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(MATRIX_BUILD_CE "Build for TI-84 Plus CE via CEdevToolchain.cmake" OFF)
option(MATRIX_ENABLE_TESTS "Build native tests" ON)
option(MATRIX_STRICT_WARNINGS "Enable -Werror and extra warnings" ON)
option(MATRIX_BUILD_CE_SHELL "Build the Matrix shell program when MATRIX_BUILD_CE=ON" ON)
option(MATRIX_BUILD_CE_SHELL_EN "Build English Matrix shell when MATRIX_BUILD_CE_SHELL=ON" ON)
option(MATRIX_BUILD_CE_SHELL_FR "Build French Matrix shell when MATRIX_BUILD_CE_SHELL=ON" OFF)
option(MATRIX_BUILD_CE_TESTS "Build core unit tests as CE programs when MATRIX_BUILD_CE=ON" OFF)

include(${CMAKE_CURRENT_LIST_DIR}/cmake/matrix_features.cmake)
matrix_print_feature_summary()

matrix_get_feature_compile_definitions(MATRIX_FEATURE_DEFS)
matrix_get_feature_compile_options(MATRIX_FEATURE_COMPILE_OPTIONS)

set(MATRIX_CORE_DIR "${CMAKE_CURRENT_LIST_DIR}/core")

set(MATRIX_CORE_HEADERS
  ${MATRIX_CORE_DIR}/include/matrix_core/config.hpp
  ${MATRIX_CORE_DIR}/include/matrix_core/error.hpp
  ${MATRIX_CORE_DIR}/include/matrix_core/explanation.hpp
  ${MATRIX_CORE_DIR}/include/matrix_core/latex.hpp
  ${MATRIX_CORE_DIR}/include/matrix_core/matrix.hpp
  ${MATRIX_CORE_DIR}/include/matrix_core/matrix_core.hpp
  ${MATRIX_CORE_DIR}/include/matrix_core/ops.hpp
  ${MATRIX_CORE_DIR}/include/matrix_core/rational.hpp
  ${MATRIX_CORE_DIR}/include/matrix_core/row_ops.hpp
  ${MATRIX_CORE_DIR}/include/matrix_core/row_reduction.hpp
  ${MATRIX_CORE_DIR}/include/matrix_core/spaces.hpp
  ${MATRIX_CORE_DIR}/include/matrix_core/writer.hpp
)

set(MATRIX_CORE_SOURCES
  ${MATRIX_CORE_DIR}/src/det_detail.cpp
  ${MATRIX_CORE_DIR}/src/explanation.cpp
  ${MATRIX_CORE_DIR}/src/latex_view.cpp
  ${MATRIX_CORE_DIR}/src/matrix_view.cpp
  ${MATRIX_CORE_DIR}/src/ops_basic_view.cpp
  ${MATRIX_CORE_DIR}/src/ops_det_ondemand.cpp
  ${MATRIX_CORE_DIR}/src/ops_inverse_ondemand.cpp
  ${MATRIX_CORE_DIR}/src/ops_optional_stubs.cpp
  ${MATRIX_CORE_DIR}/src/ops_rref_ondemand.cpp
  ${MATRIX_CORE_DIR}/src/ops_vector_ondemand.cpp
  ${MATRIX_CORE_DIR}/src/rational.cpp
  ${MATRIX_CORE_DIR}/src/row_reduction.cpp
  ${MATRIX_CORE_DIR}/src/row_ops.cpp
  ${MATRIX_CORE_DIR}/src/spaces.cpp
)

if(MATRIX_FEATURE_CRAMER)
  list(APPEND MATRIX_CORE_SOURCES
    ${MATRIX_CORE_DIR}/src/ops_cramer.cpp
    ${MATRIX_CORE_DIR}/src/ops_det_replace_column_ondemand.cpp
  )
endif()

if(MATRIX_FEATURE_COFACTOR)
  list(APPEND MATRIX_CORE_SOURCES
    ${MATRIX_CORE_DIR}/src/ops_cofactor_element_ondemand.cpp
  )
endif()

if(MATRIX_FEATURE_MINOR_MATRIX)
  list(APPEND MATRIX_CORE_SOURCES
    ${MATRIX_CORE_DIR}/src/ops_minor_matrix.cpp
  )
endif()

if(MATRIX_BUILD_CE)
  set(MATRIX_LIBTEXCE_DIR "${CMAKE_CURRENT_LIST_DIR}/third_party/libtexce")

  # CE toolchain integration (kept in-repo to avoid patching submodules).
  set(CEDEV_DEBUG_CONSOLE OFF CACHE BOOL "Disable debug console for size")
  include(${CMAKE_CURRENT_LIST_DIR}/cmake/CEdevToolchain.cmake)

  if(MATRIX_BUILD_CE_SHELL)
    set(MATRIX_SHELL_CE_SOURCES
      ${MATRIX_CORE_SOURCES}
      ${MATRIX_LIBTEXCE_DIR}/src/tex/tex_util.c
      ${MATRIX_LIBTEXCE_DIR}/src/tex/tex_pool.c
      ${MATRIX_LIBTEXCE_DIR}/src/tex/tex_symbols.c
      ${MATRIX_LIBTEXCE_DIR}/src/tex/tex_metrics.c
      ${MATRIX_LIBTEXCE_DIR}/src/tex/tex_fonts.c
      ${MATRIX_LIBTEXCE_DIR}/src/tex/tex_token.c
      ${MATRIX_LIBTEXCE_DIR}/src/tex/tex_parse.c
      ${MATRIX_LIBTEXCE_DIR}/src/tex/tex_measure.c
      ${MATRIX_LIBTEXCE_DIR}/src/tex/tex_layout.c
      ${MATRIX_LIBTEXCE_DIR}/src/tex/tex_renderer.c
      ${MATRIX_LIBTEXCE_DIR}/src/tex/tex_draw.c
      ${CMAKE_CURRENT_LIST_DIR}/shell/src/app.cpp
      ${CMAKE_CURRENT_LIST_DIR}/shell/src/app_internal.cpp
      ${CMAKE_CURRENT_LIST_DIR}/shell/src/input.cpp
      ${CMAKE_CURRENT_LIST_DIR}/shell/src/page.cpp
      ${CMAKE_CURRENT_LIST_DIR}/shell/src/main_ce.cpp
      ${CMAKE_CURRENT_LIST_DIR}/shell/src/text.cpp
      ${CMAKE_CURRENT_LIST_DIR}/shell/src/pages/cofactor.cpp
      ${CMAKE_CURRENT_LIST_DIR}/shell/src/pages/confirm.cpp
      ${CMAKE_CURRENT_LIST_DIR}/shell/src/pages/dim.cpp
      ${CMAKE_CURRENT_LIST_DIR}/shell/src/pages/editor.cpp
      ${CMAKE_CURRENT_LIST_DIR}/shell/src/pages/menu.cpp
      ${CMAKE_CURRENT_LIST_DIR}/shell/src/pages/result.cpp
      ${CMAKE_CURRENT_LIST_DIR}/shell/src/pages/slot_pick.cpp
      ${CMAKE_CURRENT_LIST_DIR}/shell/src/pages/steps.cpp
    )

    set(MATRIX_SHELL_CE_INCLUDE_DIRS
      ${MATRIX_CORE_DIR}/include
      ${CMAKE_CURRENT_LIST_DIR}/shell/include
      ${MATRIX_LIBTEXCE_DIR}/src
      ${MATRIX_LIBTEXCE_DIR}/src/tex
      ${MATRIX_LIBTEXCE_DIR}/include
    )

    set(MATRIX_SHELL_CE_COMPILE_OPTIONS
      -DTEX_DIRECT_RENDER
      -Wframe-larger-than=127
      -Wno-error=frame-larger-than
      -fstack-usage
      ${MATRIX_FEATURE_COMPILE_OPTIONS}
    )

    function(matrix_add_ce_shell target_name program_name description_text lang_fr_value)
      cedev_add_program(
        TARGET ${target_name}
        NAME ${program_name}
        DESCRIPTION ${description_text}
        MODE auto

        SOURCES
          ${MATRIX_SHELL_CE_SOURCES}

        INCLUDE_DIRECTORIES
          ${MATRIX_SHELL_CE_INCLUDE_DIRS}

        LIBLOAD
          graphx
          keypadc
          fileioc

        COMPILE_OPTIONS
          ${MATRIX_SHELL_CE_COMPILE_OPTIONS}
          -DMATRIX_SHELL_LANG_FR=${lang_fr_value}
      )
    endfunction()

    if(MATRIX_BUILD_CE_SHELL_EN)
      matrix_add_ce_shell(matrix_shell MATRIX "Matrix program (English)" 0)
    endif()

    if(MATRIX_BUILD_CE_SHELL_FR)
      matrix_add_ce_shell(matrix_shell_fr MATRIXFR "Programme matrice (Francaise)" 1)
    endif()

    if(NOT MATRIX_BUILD_CE_SHELL_EN AND NOT MATRIX_BUILD_CE_SHELL_FR)
      message(WARNING "MATRIX_BUILD_CE_SHELL=ON but both MATRIX_BUILD_CE_SHELL_EN and MATRIX_BUILD_CE_SHELL_FR are OFF; no shell binary will be produced.")
    endif()
  endif()

  if(MATRIX_BUILD_CE_TESTS)
    function(matrix_add_ce_core_test target name source)
      cedev_add_program(
        TARGET ${target}
        NAME ${name}
        DESCRIPTION "Matrix core test (${name})"
        MODE auto

        SOURCES
          ${MATRIX_CORE_SOURCES}
          ${MATRIX_CORE_DIR}/tests_ce/ce_test_runtime.cpp
          ${source}

        INCLUDE_DIRECTORIES
          ${MATRIX_CORE_DIR}/include

        COMPILE_OPTIONS
          -UNDEBUG
          -DDEBUG=1
          -DMATRIX_CE_TESTS=1
          -Wframe-larger-than=127
          -Wno-error=frame-larger-than
          -fstack-usage
          ${MATRIX_FEATURE_COMPILE_OPTIONS}
      )
    endfunction()

    matrix_add_ce_core_test(ce_test_rational TSTRAT ${MATRIX_CORE_DIR}/tests_ce/test_rational_ce.cpp)
    matrix_add_ce_core_test(ce_test_matrix_ops TSTMOPS ${MATRIX_CORE_DIR}/tests_ce/test_matrix_ops_ce.cpp)
    matrix_add_ce_core_test(ce_test_rref TSTRREF ${MATRIX_CORE_DIR}/tests_ce/test_rref_ce.cpp)
    matrix_add_ce_core_test(ce_test_det TSTDET ${MATRIX_CORE_DIR}/tests_ce/test_det_ce.cpp)
    matrix_add_ce_core_test(ce_test_inverse TSTINV ${MATRIX_CORE_DIR}/tests_ce/test_inverse_ce.cpp)
    matrix_add_ce_core_test(ce_test_vectors TSTVEC ${MATRIX_CORE_DIR}/tests_ce/test_vectors_ce.cpp)
    matrix_add_ce_core_test(ce_test_spaces TSTSPC ${MATRIX_CORE_DIR}/tests_ce/test_spaces_ce.cpp)
    if(MATRIX_FEATURE_MINOR_MATRIX)
      matrix_add_ce_core_test(ce_test_minor_matrix TSTMIN ${MATRIX_CORE_DIR}/tests_ce/test_minor_matrix_ce.cpp)
    endif()
    if(MATRIX_FEATURE_COFACTOR)
      matrix_add_ce_core_test(ce_test_cofactor_element TSTCOF ${MATRIX_CORE_DIR}/tests_ce/test_cofactor_element_ce.cpp)
    endif()
    if(MATRIX_FEATURE_CRAMER)
      matrix_add_ce_core_test(ce_test_cramer TSTCRAM ${MATRIX_CORE_DIR}/tests_ce/test_cramer_ce.cpp)
    endif()
  endif()

  message(STATUS "Matrix CE build enabled")
  return()
endif()

add_library(matrix_core STATIC ${MATRIX_CORE_SOURCES} ${MATRIX_CORE_HEADERS})
target_include_directories(matrix_core PUBLIC ${MATRIX_CORE_DIR}/include)
target_compile_features(matrix_core PUBLIC cxx_std_20)
target_compile_definitions(matrix_core PUBLIC ${MATRIX_FEATURE_DEFS})
target_compile_options(matrix_core PUBLIC -fno-exceptions -fno-rtti)
target_compile_options(matrix_core PRIVATE -fstack-usage)

if(MATRIX_STRICT_WARNINGS)
  target_compile_options(matrix_core PUBLIC
    -Wall -Wextra -Wpedantic -Werror
    -Wconversion -Wsign-conversion
    -Wshadow -Wdouble-promotion
    -Wformat=2 -Wformat-security
    -Wcast-qual -Wpointer-arith
    -Wswitch-enum -Wundef
    -Wnull-dereference -Wfloat-equal
    -Wredundant-decls -Wunreachable-code
    -Wimplicit-fallthrough
  )
endif()

add_executable(matrix_profile_cramer ${MATRIX_CORE_DIR}/profile/profile_cramer.cpp)
target_link_libraries(matrix_profile_cramer PRIVATE matrix_core)
target_compile_options(matrix_profile_cramer PRIVATE -fstack-usage)

if(MATRIX_ENABLE_TESTS)
  enable_testing()
  target_compile_options(matrix_core PUBLIC -UNDEBUG)

  add_executable(test_rational ${MATRIX_CORE_DIR}/tests/test_rational.cpp)
  target_link_libraries(test_rational PRIVATE matrix_core)

  add_executable(test_matrix_ops ${MATRIX_CORE_DIR}/tests/test_matrix_ops.cpp)
  target_link_libraries(test_matrix_ops PRIVATE matrix_core)

  add_executable(test_rref ${MATRIX_CORE_DIR}/tests/test_rref.cpp)
  target_link_libraries(test_rref PRIVATE matrix_core)

  add_executable(test_det ${MATRIX_CORE_DIR}/tests/test_det.cpp)
  target_link_libraries(test_det PRIVATE matrix_core)

  if(MATRIX_FEATURE_MINOR_MATRIX)
    add_executable(test_minor_matrix ${MATRIX_CORE_DIR}/tests/test_minor_matrix.cpp)
    target_link_libraries(test_minor_matrix PRIVATE matrix_core)
  endif()

  if(MATRIX_FEATURE_COFACTOR)
    add_executable(test_cofactor_element ${MATRIX_CORE_DIR}/tests/test_cofactor_element.cpp)
    target_link_libraries(test_cofactor_element PRIVATE matrix_core)
  endif()

  if(MATRIX_FEATURE_CRAMER)
    add_executable(test_cramer ${MATRIX_CORE_DIR}/tests/test_cramer.cpp)
    target_link_libraries(test_cramer PRIVATE matrix_core)
  endif()

  add_executable(test_inverse ${MATRIX_CORE_DIR}/tests/test_inverse.cpp)
  target_link_libraries(test_inverse PRIVATE matrix_core)

  add_executable(test_vectors ${MATRIX_CORE_DIR}/tests/test_vectors.cpp)
  target_link_libraries(test_vectors PRIVATE matrix_core)

  add_executable(test_spaces ${MATRIX_CORE_DIR}/tests/test_spaces.cpp)
  target_link_libraries(test_spaces PRIVATE matrix_core)

  add_test(NAME rational COMMAND test_rational)
  add_test(NAME matrix_ops COMMAND test_matrix_ops)
  add_test(NAME rref COMMAND test_rref)
  add_test(NAME det COMMAND test_det)
  add_test(NAME inverse COMMAND test_inverse)
  add_test(NAME vectors COMMAND test_vectors)
  add_test(NAME spaces COMMAND test_spaces)

  if(MATRIX_FEATURE_MINOR_MATRIX)
    add_test(NAME minor_matrix COMMAND test_minor_matrix)
  endif()
  if(MATRIX_FEATURE_COFACTOR)
    add_test(NAME cofactor_element COMMAND test_cofactor_element)
  endif()
  if(MATRIX_FEATURE_CRAMER)
    add_test(NAME cramer COMMAND test_cramer)
  endif()
endif()
