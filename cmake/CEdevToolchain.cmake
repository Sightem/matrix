# -----------------------------------------------------------------------------
# CEdev Toolchain for TI‑84 Plus CE (CMake)
# -----------------------------------------------------------------------------
# this file makes the CEdev toolchain available in CMake and provides the
# cedev_add_program() helper.
#
# CEdev toolchain variants:
# - Legacy: clang + fasmg linker script pipeline (meta/ld.alm, lib/crt/crt0.src)
# - New (v17+): clang + binutils pipeline (meta/linker_script.ld, lib/crt/crt0.S)
#
# quick start:
#   cmake -S . -B build -DCEDEV_ROOT=/path/to/CEdev
#   cmake --build build --target demo
#
# minimal CMakeLists.txt example:
#   cmake_minimum_required(VERSION 3.20)
#   project(my_ce_program C CXX)
#
#   include(cmake/CEdevToolchain.cmake)
#
#   cedev_add_program(
#     TARGET demo
#     NAME "DEMO"
#     SOURCES src/main.cpp
#
#     INCLUDE_DIRECTORIES include src
#
#     # defaults to 20 if omitted
#     CXX_STANDARD 20
#
#     # stack & memory
#     LINKER_GLOBALS
#        "range .bss $D052C6 : $D13FD8"
#        "provide __stack = $D10000"
#
#     # libload libraries
#     LIBLOAD graphx keypadc
#   )
#
# toggles (pass to cmake -DNAME=ON/OFF):
#   CEDEV_HAS_PRINTF        Link toolchain printf instead of OS (default OFF)
#   CEDEV_PREFER_OS_LIBC    Prefer OS libc where possible (default ON)
#   CEDEV_ENABLE_LTO        Enable link-time optimization (default ON)
#   CEDEV_DEBUG_CONSOLE     Enable dbg_printf / keep assertions (default ON)
# -----------------------------------------------------------------------------

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_VERSION 1)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

option(CEDEV_HAS_PRINTF       "Link toolchain printf implementations" OFF)
option(CEDEV_PREFER_OS_LIBC   "Prefer OS libc where possible" ON)
option(CEDEV_PREFER_OS_CRT    "Prefer OS crt where possible" OFF)
option(CEDEV_ENABLE_LTO         "Enable link-time optimization" ON)
option(CEDEV_DEBUG_CONSOLE      "Enable emulator debug console" ON)
option(CEDEV_IDE_HELPER         "Emit hidden OBJECT target for IDEs" ON)
option(CEDEV_DEFAULT_COMPRESS   "Package compressed .8xp by default" ON)
option(CEDEV_DEFAULT_ARCHIVED   "Mark program archived by default" ON)
option(CEDEV_STRICT_WARNINGS    "Enable aggressive warnings for ez80 sources" ON)
option(CEDEV_WARNINGS_AS_ERRORS "Treat warnings as errors for ez80 sources" ON)
option(CEDEV_FORCE_CXX_LANGUAGE "Force -x c++ when CEDEV_CXX_MODE is enabled" OFF)

if(NOT DEFINED CEDEV_ROOT)
  set(CEDEV_ROOT "/home/sightem/CEdev" CACHE PATH "Path to CEdev toolchain root")
endif()

set(CEDEV_BIN        "${CEDEV_ROOT}/bin")
set(CEDEV_BINUTILS_BIN "${CEDEV_ROOT}/binutils/bin")
set(CEDEV_META       "${CEDEV_ROOT}/meta")
set(CEDEV_INCLUDE    "${CEDEV_ROOT}/include")
set(CEDEV_LIB        "${CEDEV_ROOT}/lib")

set(_CEDEV_TOOLCHAIN_HAS_BINUTILS OFF)
if(EXISTS "${CEDEV_BINUTILS_BIN}/z80-none-elf-ld" AND EXISTS "${CEDEV_META}/linker_script.ld")
  set(_CEDEV_TOOLCHAIN_HAS_BINUTILS ON)
endif()

set(_CEDEV_USE_BINUTILS_DEFAULT OFF)
if(_CEDEV_TOOLCHAIN_HAS_BINUTILS)
  set(_CEDEV_USE_BINUTILS_DEFAULT ON)
endif()
option(CEDEV_USE_BINUTILS "Use the CEdev binutils-based pipeline when available" ${_CEDEV_USE_BINUTILS_DEFAULT})

set(CEDEV_C_COMPILER_DEFAULT   "${CEDEV_BIN}/ez80-clang")
set(CEDEV_CXX_COMPILER_DEFAULT "${CEDEV_BIN}/ez80-clang")
set(CEDEV_C_COMPILER   "${CEDEV_C_COMPILER_DEFAULT}" CACHE FILEPATH "C compiler for ez80")
set(CEDEV_CXX_COMPILER "${CEDEV_CXX_COMPILER_DEFAULT}" CACHE FILEPATH "C++ compiler for ez80")

set(CMAKE_C_COMPILER   "${CEDEV_C_COMPILER}")
set(CMAKE_CXX_COMPILER "${CEDEV_CXX_COMPILER}")
set(CEDEV_LINKER       "${CEDEV_BIN}/ez80-link")
set(CEDEV_CONVBIN      "${CEDEV_BIN}/convbin")
set(CEDEV_CEDEV_OBJ    "${CEDEV_BIN}/cedev-obj")

set(CMAKE_C_COMPILER_WORKS TRUE)
set(CMAKE_CXX_COMPILER_WORKS TRUE)

if(NOT DEFINED ENV{PATH})
  if(CEDEV_USE_BINUTILS)
    set(ENV{PATH} "${CEDEV_BINUTILS_BIN}:${CEDEV_BIN}")
  else()
    set(ENV{PATH} "${CEDEV_BIN}")
  endif()
else()
  if(CEDEV_USE_BINUTILS)
    set(ENV{PATH} "${CEDEV_BINUTILS_BIN}:${CEDEV_BIN}:$ENV{PATH}")
  else()
    set(ENV{PATH} "${CEDEV_BIN}:$ENV{PATH}")
  endif()
endif()

set(CMAKE_C_STANDARD_INCLUDE_DIRECTORIES "${CEDEV_INCLUDE}")
set(CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES "${CEDEV_INCLUDE}")
include_directories("${CEDEV_INCLUDE}")

set(_CEDEV_WARNINGS
  -Wall -Wextra
)
if(CEDEV_STRICT_WARNINGS)
  list(APPEND _CEDEV_WARNINGS
    -Wpedantic
    -Wconversion -Wsign-conversion
    -Wshadow -Wdouble-promotion
    -Wformat=2 -Wformat-security
    -Wcast-qual -Wpointer-arith
    -Wundef -Wnull-dereference
    -Wimplicit-fallthrough
  )
endif()
if(CEDEV_WARNINGS_AS_ERRORS)
  list(APPEND _CEDEV_WARNINGS -Werror)
endif()

add_compile_options(${_CEDEV_WARNINGS} -Oz)
add_compile_options(-ffunction-sections -fdata-sections)

if(CEDEV_ENABLE_LTO)
  add_link_options(--lto)
endif()
if(CEDEV_PREFER_OS_LIBC)
  add_link_options(--prefer-os-libc)
endif()

# cedev_add_program
function(cedev_add_program)
  set(options CXX_MODE)
  set(oneValueArgs TARGET NAME DESCRIPTION ICON COMPRESS MODE ARCHIVED CXX_STANDARD)
  set(multiValueArgs SOURCES LIBLOAD OPTIONAL_LIBS INCLUDE_DIRECTORIES COMPILE_OPTIONS LINKER_GLOBALS)
  cmake_parse_arguments(CEDEV "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if(NOT CEDEV_TARGET)
    message(FATAL_ERROR "cedev_add_program requires TARGET <name>")
  endif()

  if(CEDEV_CXX_MODE)
    set_source_files_properties(${CEDEV_SOURCES} PROPERTIES LANGUAGE CXX)
  endif()

  if(NOT CEDEV_NAME)
    string(TOUPPER "${CEDEV_TARGET}" CEDEV_NAME)
  endif()

  if(NOT DEFINED CEDEV_CXX_STANDARD)
    if(DEFINED CMAKE_CXX_STANDARD)
      set(CEDEV_CXX_STANDARD ${CMAKE_CXX_STANDARD})
    else()
      set(CEDEV_CXX_STANDARD 20)
    endif()
  endif()

  if(NOT CEDEV_LINKER_GLOBALS)
    if(CEDEV_USE_BINUTILS)
      # Match CEdev v17 makefile defaults.
      set(CEDEV_LINKER_GLOBALS
        "range .bss $D052C6 : $D13FD8"
        "provide __stack = $D1A87E"
        "locate .header at $D1A87F"
      )
    else()
      # Legacy defaults (fasmg linker_script).
      set(CEDEV_LINKER_GLOBALS
        "range .bss $D052C6 : $D13FD8"
        "provide __stack = $D1987E"
        "locate .header at $D1A87F"
      )
    endif()
  endif()

  set(BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/${CEDEV_TARGET}")
  set(OBJ_DIR   "${BUILD_DIR}/obj")
  set(BIN_DIR   "${BUILD_DIR}/bin")
  file(MAKE_DIRECTORY "${OBJ_DIR}")
  file(MAKE_DIRECTORY "${BIN_DIR}")

  if(CEDEV_USE_BINUTILS)
    # -----------------------------------------------------------------------
    # binutils-based pipeline (CEdev v17+)
    # mirrors the CEdev makefile + linker_script.ld flow
    # -----------------------------------------------------------------------
    set(_AS     "${CEDEV_BINUTILS_BIN}/z80-none-elf-as")
    set(_LD     "${CEDEV_BINUTILS_BIN}/z80-none-elf-ld")
    set(_STRIP  "${CEDEV_BINUTILS_BIN}/z80-none-elf-strip")
    set(_OBJCOPY "${CEDEV_BINUTILS_BIN}/z80-none-elf-objcopy")

    if(NOT EXISTS "${_AS}" OR NOT EXISTS "${_LD}" OR NOT EXISTS "${_STRIP}")
      message(FATAL_ERROR "CEDEV_USE_BINUTILS=ON but binutils not found under: ${CEDEV_BINUTILS_BIN}")
    endif()
    if(NOT EXISTS "${CEDEV_CEDEV_OBJ}")
      message(FATAL_ERROR "CEDEV_USE_BINUTILS=ON but cedev-obj not found under: ${CEDEV_BIN}")
    endif()

    set(_bss_low "0xD052C6")
    set(_bss_high "0xD13FD8")
    set(_stack_high "0xD1A87E")
    set(_load_addr "0xD1A87F")

    # parse legacy LINKER_GLOBALS if provided.
    foreach(_g IN LISTS CEDEV_LINKER_GLOBALS)
      if(_g MATCHES "range[ ]+\\.bss[ ]+\\$([0-9A-Fa-f]+)[ ]*:[ ]*\\$([0-9A-Fa-f]+)")
        set(_bss_low "0x${CMAKE_MATCH_1}")
        set(_bss_high "0x${CMAKE_MATCH_2}")
      endif()
      if(_g MATCHES "provide[ ]+__stack[ ]*=[ ]*\\$([0-9A-Fa-f]+)")
        set(_stack_high "0x${CMAKE_MATCH_1}")
      endif()
      if(_g MATCHES "locate[ ]+\\.header[ ]+at[ ]+\\$([0-9A-Fa-f]+)")
        set(_load_addr "0x${CMAKE_MATCH_1}")
      endif()
    endforeach()

    # Toolchain libraries
    set(_LIB_ALLOCATOR "${CEDEV_LIB}/libc/allocator_standard.a")
    if(CEDEV_HAS_PRINTF)
      set(_LIB_PRINTF "${CEDEV_LIB}/libc/libnanoprintf.a")
      set(_SPRINTF_SYMBOL)
    else()
      set(_LIB_PRINTF)
      set(_SPRINTF_SYMBOL "--defsym" "_sprintf=0x0000BC")
    endif()

    if(CEDEV_PREFER_OS_CRT)
      set(_LIB_CRT "${CEDEV_LIB}/crt/libcrt_os.a")
    else()
      set(_LIB_CRT "${CEDEV_LIB}/crt/libcrt.a")
    endif()

    if(CEDEV_PREFER_OS_LIBC)
      set(_LIB_C "${CEDEV_LIB}/libc/libc_os.a")
    else()
      set(_LIB_C "${CEDEV_LIB}/libc/libc.a")
    endif()

    set(_LIB_CXX "${CEDEV_LIB}/libcxx/libcxx.a")
    set(_LIB_CE "${CEDEV_LIB}/ce/libce.a")
    set(_LIB_SOFTFLOAT "${CEDEV_LIB}/softfloat/libsoftfloat.a")

    # debug defaults that match the CEdev makefile
    set(_cc_debug_flags -DNDEBUG=1)
    set(_ld_debug_flags "--defsym" "NDEBUG=1")
    set(_do_strip 1)
    if(CEDEV_DEBUG_CONSOLE)
      set(_cc_debug_flags -DDEBUG=1 -O0 -gdwarf-5 -g3)
      set(_ld_debug_flags "--defsym" "DEBUG=1")
      set(_do_strip 0)
    endif()

    # compile includes
    set(_custom_includes_flags)
    list(APPEND CEDEV_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_BINARY_DIR}")
    foreach(_inc IN LISTS CEDEV_INCLUDE_DIRECTORIES)
      get_filename_component(_abs_inc "${_inc}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
      list(APPEND _custom_includes_flags "-I${_abs_inc}")
    endforeach()

    # compile options
    set(_user_compile_options)
    if(CEDEV_COMPILE_OPTIONS)
      set(_user_compile_options ${CEDEV_COMPILE_OPTIONS})
    endif()

    # CEdev makefile-ish common flags.
    set(_ezllvm_flags
      -mllvm -profile-guided-section-prefix=false
      -mllvm -z80-gas-style
      -ffunction-sections -fdata-sections
      -fno-addrsig
      -fno-autolink
      -fno-threadsafe-statics
      -fno-math-errno
    )

    set(_common_compile_flags
      -nostdinc
      -isystem "${CEDEV_INCLUDE}"
      -Xclang -fforce-mangle-main-argc-argv
      ${_ezllvm_flags}
      -D__TICE__=1
      ${_CEDEV_WARNINGS} -Oz
      ${_cc_debug_flags}
      ${_custom_includes_flags}
      ${_user_compile_options}
    )

    set(BC_FILES)
    set(ASM_OBJ_FILES)
    set(OBJ_FILES)

    foreach(SRC IN LISTS CEDEV_SOURCES)
      get_filename_component(SRC_EXT "${SRC}" EXT)
      get_filename_component(ABS_SRC "${SRC}" ABSOLUTE)
      get_filename_component(SRC_NAME_WE "${SRC}" NAME_WE)

      string(MD5 _src_hash "${ABS_SRC}")
      string(SUBSTRING "${_src_hash}" 0 10 _src_hash10)
      set(_stem "${OBJ_DIR}/${SRC_NAME_WE}_${_src_hash10}")

      # handle assembly inputs (.s / .S) without LTO
      if(SRC_EXT STREQUAL ".s")
        set(OUT_O "${_stem}.s.o")
        add_custom_command(
          OUTPUT "${OUT_O}"
          COMMAND "${CMAKE_COMMAND}" -E echo "[as] ${SRC}"
          COMMAND "${_AS}" -march=ez80+full "${ABS_SRC}" -o "${OUT_O}"
          DEPENDS "${ABS_SRC}"
          VERBATIM
        )
        list(APPEND ASM_OBJ_FILES "${OUT_O}")
        continue()
      endif()
      if(SRC_EXT STREQUAL ".S")
        set(OUT_S "${_stem}.S.s")
        set(OUT_O "${_stem}.S.o")
        add_custom_command(
          OUTPUT "${OUT_S}"
          COMMAND "${CMAKE_COMMAND}" -E echo "[as] ${SRC}"
          COMMAND "${CMAKE_C_COMPILER}" -E -P "${ABS_SRC}" -o "${OUT_S}"
          DEPENDS "${ABS_SRC}"
          VERBATIM
        )
        add_custom_command(
          OUTPUT "${OUT_O}"
          COMMAND "${_AS}" -march=ez80+full "${OUT_S}" -o "${OUT_O}"
          DEPENDS "${OUT_S}"
          VERBATIM
        )
        list(APPEND ASM_OBJ_FILES "${OUT_O}")
        continue()
      endif()

      # C/C++ compilation
      set(_compiler "${CMAKE_C_COMPILER}")
      set(_std_flags)
      set(_lang_specific_flags)
      set(_is_cxx 0)

      if(SRC_EXT STREQUAL ".cpp" OR SRC_EXT STREQUAL ".cxx" OR SRC_EXT STREQUAL ".cc")
        set(_is_cxx 1)
      endif()
      if(CEDEV_CXX_MODE)
        set(_is_cxx 1)
      endif()

      if(_is_cxx)
        set(_compiler "${CMAKE_CXX_COMPILER}")
        set(_std_flags "-std=c++${CEDEV_CXX_STANDARD}")
        if(CEDEV_FORCE_CXX_LANGUAGE)
          list(APPEND _std_flags -xc++)
        endif()
        set(_lang_specific_flags
          -isystem "${CEDEV_INCLUDE}/c++"
          -fno-exceptions
          -fno-use-cxa-atexit
          -DEASTL_USER_CONFIG_HEADER=<__EASTL_user_config.h>
        )
      endif()

      if(CEDEV_ENABLE_LTO)
        set(OUT_BC "${_stem}.bc")
        set(_depfile "${OUT_BC}.d")
        add_custom_command(
          OUTPUT "${OUT_BC}"
          DEPFILE "${_depfile}"
          BYPRODUCTS "${_depfile}"
          COMMAND "${CMAKE_COMMAND}" -E echo "[cc] ${SRC}"
          COMMAND "${_compiler}"
                  -MMD -MF "${_depfile}" -MT "${OUT_BC}" -c -emit-llvm
                  ${_std_flags}
                  ${_lang_specific_flags}
                  ${_common_compile_flags}
                  "${ABS_SRC}" -o "${OUT_BC}"
          DEPENDS "${ABS_SRC}"
          VERBATIM
        )
        list(APPEND BC_FILES "${OUT_BC}")
      else()
        set(OUT_S "${_stem}.s")
        set(OUT_O "${_stem}.o")
        set(_depfile "${OUT_S}.d")
        add_custom_command(
          OUTPUT "${OUT_S}"
          DEPFILE "${_depfile}"
          BYPRODUCTS "${_depfile}"
          COMMAND "${CMAKE_COMMAND}" -E echo "[cc] ${SRC}"
          COMMAND "${_compiler}"
                  -S -MMD -MF "${_depfile}" -MT "${OUT_S}"
                  ${_std_flags}
                  ${_lang_specific_flags}
                  ${_common_compile_flags}
                  "${ABS_SRC}" -o "${OUT_S}"
          DEPENDS "${ABS_SRC}"
          VERBATIM
        )
        add_custom_command(
          OUTPUT "${OUT_O}"
          COMMAND "${_AS}" -march=ez80+full "${OUT_S}" -o "${OUT_O}"
          DEPENDS "${OUT_S}"
          VERBATIM
        )
        list(APPEND OBJ_FILES "${OUT_O}")
      endif()
    endforeach()

    if(CEDEV_ENABLE_LTO)
      set(LTO_BC "${OBJ_DIR}/lto.bc")
      set(LTO_S  "${OBJ_DIR}/lto.s")
      set(LTO_O  "${OBJ_DIR}/lto.o")

      add_custom_command(
        OUTPUT "${LTO_BC}"
        COMMAND "${CMAKE_COMMAND}" -E echo "[lto] ${LTO_BC}"
        COMMAND "${CEDEV_LINKER}" ${BC_FILES} -o "${LTO_BC}"
        DEPENDS ${BC_FILES}
        VERBATIM
      )

      add_custom_command(
        OUTPUT "${LTO_S}"
        COMMAND "${CMAKE_COMMAND}" -E echo "[cc] ${LTO_S}"
        COMMAND "${CMAKE_C_COMPILER}" -S ${_ezllvm_flags} -Oz ${_cc_debug_flags} "${LTO_BC}" -o "${LTO_S}"
        DEPENDS "${LTO_BC}"
        VERBATIM
      )

      add_custom_command(
        OUTPUT "${LTO_O}"
        COMMAND "${_AS}" -march=ez80+full "${LTO_S}" -o "${LTO_O}"
        DEPENDS "${LTO_S}"
        VERBATIM
      )

      list(APPEND OBJ_FILES "${LTO_O}")
    endif()

    list(APPEND OBJ_FILES ${ASM_OBJ_FILES})

    # LibLoad export lists for cedev-obj
    set(LIBLOAD_FILES)
    foreach(LIB_NAME IN LISTS CEDEV_LIBLOAD)
      set(LIB_FILE "${CEDEV_LIB}/libload/${LIB_NAME}.lib")
      if(NOT EXISTS "${LIB_FILE}")
        message(FATAL_ERROR "Requested LIBLOAD library not found: ${LIB_FILE}")
      endif()
      list(APPEND LIBLOAD_FILES "${LIB_FILE}")
    endforeach()
    foreach(LIB_NAME IN LISTS CEDEV_OPTIONAL_LIBS)
      set(LIB_FILE "${CEDEV_LIB}/libload/${LIB_NAME}.lib")
      if(EXISTS "${LIB_FILE}")
        list(APPEND LIBLOAD_FILES "${LIB_FILE}")
      endif()
    endforeach()

    set(LINKER_SCRIPT "${CEDEV_META}/linker_script.ld")
    set(CRT0_SRC "${CEDEV_LIB}/crt/crt0.S")
    set(CRT_H    "${OBJ_DIR}/crt.h")
    set(CRT0_TMP "${OBJ_DIR}/crt0.s")
    set(CRT0_OBJ "${OBJ_DIR}/crt0.o")

    set(TARGET_TMP "${OBJ_DIR}/${CEDEV_NAME}.o")
    set(TARGET_OBJ "${BIN_DIR}/${CEDEV_NAME}.obj")
    set(TARGET_8XP "${BIN_DIR}/${CEDEV_NAME}.8xp")

    set(_TMP_LINK_INPUTS
      ${OBJ_FILES}
      "${_LIB_ALLOCATOR}"
    )
    if(_LIB_PRINTF)
      list(APPEND _TMP_LINK_INPUTS "${_LIB_PRINTF}")
    endif()
    list(APPEND _TMP_LINK_INPUTS
      "${_LIB_CXX}"
      "${_LIB_CE}"
      "${_LIB_CRT}"
      "${_LIB_C}"
      "${_LIB_SOFTFLOAT}"
    )

    if(_do_strip)
      add_custom_command(
        OUTPUT "${TARGET_TMP}"
        COMMAND "${CMAKE_COMMAND}" -E echo "[linking] ${TARGET_TMP}"
        COMMAND "${_LD}"
                -i
                -static
                --entry=__start
                --build-id=none
                --gc-sections
                --omagic
                --defsym __TICE__=1
                ${_SPRINTF_SYMBOL}
                ${_ld_debug_flags}
                ${_TMP_LINK_INPUTS}
                -o "${TARGET_TMP}"
        COMMAND "${CMAKE_COMMAND}" -E echo "[strip] ${TARGET_TMP}"
        COMMAND "${_STRIP}" --strip-unneeded "${TARGET_TMP}"
        DEPENDS ${OBJ_FILES}
        VERBATIM
      )
    else()
      add_custom_command(
        OUTPUT "${TARGET_TMP}"
        COMMAND "${CMAKE_COMMAND}" -E echo "[linking] ${TARGET_TMP}"
        COMMAND "${_LD}"
                -i
                -static
                --entry=__start
                --build-id=none
                --gc-sections
                --omagic
                --defsym __TICE__=1
                ${_SPRINTF_SYMBOL}
                ${_ld_debug_flags}
                ${_TMP_LINK_INPUTS}
                -o "${TARGET_TMP}"
        DEPENDS ${OBJ_FILES}
        VERBATIM
      )
    endif()

    set(_CEDEV_OBJ_ARGS --elf "${TARGET_TMP}" --output "${CRT_H}")
    if(LIBLOAD_FILES)
      list(APPEND _CEDEV_OBJ_ARGS --libs ${LIBLOAD_FILES})
    endif()
    if(CEDEV_OPTIONAL_LIBS)
      list(APPEND _CEDEV_OBJ_ARGS --optional-libs ${CEDEV_OPTIONAL_LIBS})
    endif()

    add_custom_command(
      OUTPUT "${CRT_H}"
      COMMAND "${CMAKE_COMMAND}" -E echo "[cedev-obj] ${CRT_H}"
      COMMAND "${CEDEV_CEDEV_OBJ}" ${_CEDEV_OBJ_ARGS}
      DEPENDS "${TARGET_TMP}"
      VERBATIM
    )

    add_custom_command(
      OUTPUT "${CRT0_TMP}"
      COMMAND "${CMAKE_COMMAND}" -E echo "[cc] ${CRT0_TMP}"
      COMMAND "${CMAKE_C_COMPILER}" -I "${OBJ_DIR}" -E -P "${CRT0_SRC}" -o "${CRT0_TMP}"
      DEPENDS "${CRT0_SRC}" "${CRT_H}"
      VERBATIM
    )

    add_custom_command(
      OUTPUT "${CRT0_OBJ}"
      COMMAND "${CMAKE_COMMAND}" -E echo "[as] ${CRT0_OBJ}"
      COMMAND "${_AS}" -I "${OBJ_DIR}" -march=ez80+full "${CRT0_TMP}" -o "${CRT0_OBJ}"
      DEPENDS "${CRT0_TMP}"
      VERBATIM
    )

    add_custom_command(
      OUTPUT "${TARGET_OBJ}"
      COMMAND "${CMAKE_COMMAND}" -E echo "[linking] ${TARGET_OBJ}"
      COMMAND "${_LD}"
              -static
              --entry=__start
              --no-warn-rwx-segments
              --gc-sections
              --omagic
              --defsym=LOAD_ADDR=${_load_addr}
              --defsym=BSSHEAP_LOW=${_bss_low}
              --defsym=BSSHEAP_HIGH=${_bss_high}
              --defsym=STACK_HIGH=${_stack_high}
              --defsym __TICE__=1
              -T "${LINKER_SCRIPT}"
              "${TARGET_TMP}"
              "${CRT0_OBJ}"
              "${_LIB_ALLOCATOR}"
              --start-group
              "${_LIB_SOFTFLOAT}"
              "${_LIB_CRT}"
              --end-group
              "${_LIB_C}"
              "${_LIB_CXX}"
              "${_LIB_CE}"
              -o "${TARGET_OBJ}"
      DEPENDS "${TARGET_TMP}" "${CRT0_OBJ}"
      VERBATIM
    )

    # convbin packaging
    set(CONVBIN_ARGS -j elf -i "${TARGET_OBJ}" -o "${TARGET_8XP}" -n "${CEDEV_NAME}")
    set(USE_ARCHIVED ${CEDEV_ARCHIVED})
    set(USE_COMPRESS ${CEDEV_COMPRESS})
    if(NOT DEFINED USE_ARCHIVED)
      set(USE_ARCHIVED ${CEDEV_DEFAULT_ARCHIVED})
    endif()
    if(NOT DEFINED USE_COMPRESS)
      set(USE_COMPRESS ${CEDEV_DEFAULT_COMPRESS})
    endif()
    if(USE_ARCHIVED)
      list(APPEND CONVBIN_ARGS -r)
    endif()
    if(USE_COMPRESS)
      if(CEDEV_MODE)
        list(APPEND CONVBIN_ARGS -e ${CEDEV_MODE} -k 8xp-compressed)
      else()
        list(APPEND CONVBIN_ARGS -e zx7 -k 8xp-compressed)
      endif()
    else()
      list(APPEND CONVBIN_ARGS -k 8xp)
    endif()

    add_custom_command(
      OUTPUT "${TARGET_8XP}"
      COMMAND "${CMAKE_COMMAND}" -E echo "[convbin] ${TARGET_8XP}"
      COMMAND "${CEDEV_CONVBIN}" ${CONVBIN_ARGS}
      DEPENDS "${TARGET_OBJ}"
      VERBATIM
    )

    add_custom_target(${CEDEV_TARGET} ALL DEPENDS "${TARGET_8XP}")
    add_custom_command(TARGET ${CEDEV_TARGET} POST_BUILD
      COMMAND "${CMAKE_COMMAND}" -E echo "Built: ${TARGET_8XP}"
    )

    return()
  endif()

  # compile includes
  set(_custom_includes_flags)
  list(APPEND CEDEV_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_BINARY_DIR}")
  
  foreach(_inc IN LISTS CEDEV_INCLUDE_DIRECTORIES)
    # resolve absolute path if relative provided
    get_filename_component(_abs_inc "${_inc}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
    list(APPEND _custom_includes_flags "-I${_abs_inc}")
  endforeach()

  # compile options
  set(_user_compile_options)
  if(CEDEV_COMPILE_OPTIONS)
    set(_user_compile_options ${CEDEV_COMPILE_OPTIONS})
  endif()

  set(COMPILED_SRCS)
  foreach(SRC IN LISTS CEDEV_SOURCES)
    get_filename_component(SRC_NAME "${SRC}" NAME)
    get_filename_component(SRC_EXT  "${SRC}" EXT)
    set(OUT_SRC "${OBJ_DIR}/${SRC_NAME}.src")
    get_filename_component(ABS_SRC "${SRC}" ABSOLUTE)

    set(_compiler "${CMAKE_C_COMPILER}")
    set(_std_flags)
    set(_lang_specific_flags)
    set(_is_cxx 0)

    if(SRC_EXT STREQUAL ".cpp" OR SRC_EXT STREQUAL ".cxx" OR SRC_EXT STREQUAL ".cc")
      set(_is_cxx 1)
    endif()
    if(CEDEV_CXX_MODE)
      set(_is_cxx 1)
    endif()

    if(_is_cxx)
      set(_compiler "${CMAKE_CXX_COMPILER}")
      set(_std_flags "-std=c++${CEDEV_CXX_STANDARD}")
      if(CEDEV_FORCE_CXX_LANGUAGE)
        list(APPEND _std_flags -xc++)
      endif()
      set(_lang_specific_flags 
          -fno-exceptions 
          -fno-use-cxa-atexit 
          -DEASTL_USER_CONFIG_HEADER=<__EASTL_user_config.h> 
          -isystem "${CEDEV_INCLUDE}/c++"
      )
    endif()

    set(_debug_flags)
    if(CEDEV_DEBUG_CONSOLE)
      set(_debug_flags -UNDEBUG)
    else()
      set(_debug_flags -DNDEBUG)
    endif()

    set(_base_flags
      -target ez80-none-elf
      -S -nostdinc
      -isystem "${CEDEV_INCLUDE}"
      -fno-threadsafe-statics
      -fno-addrsig
      -D__TICE__ 
      ${_CEDEV_WARNINGS} -Oz
    )

    set(_depfile "${OUT_SRC}.d")
    add_custom_command(
      OUTPUT "${OUT_SRC}"
      DEPFILE "${_depfile}"
      BYPRODUCTS "${_depfile}"
      COMMAND "${CMAKE_COMMAND}" -E echo "[compiling] ${SRC}"
      COMMAND "${_compiler}"
              ${_base_flags}
              ${_std_flags}
              ${_lang_specific_flags}
              ${_debug_flags}
              ${_custom_includes_flags}
              ${_user_compile_options}
              -MMD -MF "${_depfile}" -MT "${OUT_SRC}"
              "${ABS_SRC}" -o "${OUT_SRC}"
      DEPENDS "${ABS_SRC}"
      VERBATIM
    )
    list(APPEND COMPILED_SRCS "${OUT_SRC}")
  endforeach()

  set(LIBLOAD_FILES)
  foreach(LIB_NAME IN LISTS CEDEV_LIBLOAD)
    set(LIB_FILE "${CEDEV_LIB}/libload/${LIB_NAME}.lib")
    if(NOT EXISTS "${LIB_FILE}")
      message(FATAL_ERROR "Requested LIBLOAD library not found: ${LIB_FILE}")
    endif()
    list(APPEND LIBLOAD_FILES "${LIB_FILE}")
  endforeach()

  set(LINKER_SCRIPT "${CEDEV_ROOT}/meta/linker_script")
  set(LD_ALM        "${CEDEV_ROOT}/meta/ld.alm")
  set(CRT0          "${CEDEV_ROOT}/lib/crt/crt0.src")

  set(LIBLIST)
  foreach(LIB_FILE IN LISTS LIBLOAD_FILES)
    if(LIBLIST)
      set(LIBLIST "${LIBLIST}, \"${LIB_FILE}\"")
    else()
      set(LIBLIST "\"${LIB_FILE}\"")
    endif()
  endforeach()

  set(TARGET_BIN "${BIN_DIR}/${CEDEV_NAME}.bin")
  set(TARGET_8XP "${BIN_DIR}/${CEDEV_NAME}.8xp")

  if(CEDEV_HAS_PRINTF)
    set(_HAS_PRINTF 1)
  else()
    set(_HAS_PRINTF 0)
  endif()
  if(CEDEV_PREFER_OS_LIBC)
    set(_PREF_OS_LIBC 1)
  else()
    set(_PREF_OS_LIBC 0)
  endif()
  if(CEDEV_DEBUG_CONSOLE)
    set(_DBG 1)
  else()
    set(_DBG 0)
  endif()

  # FASMG arguments
  set(FASMG_ARGS
    "${LD_ALM}"
    -i "DEBUG := ${_DBG}"
    -i "HAS_PRINTF := ${_HAS_PRINTF}"
    -i "HAS_LIBC := 1"
    -i "HAS_LIBCXX := 1"
    -i "HAS_EASTL := 1"
    -i "PREFER_OS_CRT := 0"
    -i "PREFER_OS_LIBC := ${_PREF_OS_LIBC}"
    -i "ALLOCATOR_STANDARD := 1"
    -i "__TICE__ := 1"
    -i "include \"${LINKER_SCRIPT}\""
  )

  # inject User Mmemory map and globals
  foreach(_global IN LISTS CEDEV_LINKER_GLOBALS)
    list(APPEND FASMG_ARGS -i "${_global}")
  endforeach()

  if(LIBLIST)
    list(APPEND FASMG_ARGS -i "library ${LIBLIST}")
  endif()

  set(SOURCE_LIST "\"${CRT0}\"")
  foreach(S IN LISTS COMPILED_SRCS)
    set(SOURCE_LIST "${SOURCE_LIST}, \"${S}\"")
  endforeach()
  list(APPEND FASMG_ARGS -i "source ${SOURCE_LIST}")

  add_custom_command(
    OUTPUT "${TARGET_BIN}"
    COMMAND "${CMAKE_COMMAND}" -E echo "[linking] ${TARGET_BIN}"
    COMMAND "${CEDEV_BIN}/fasmg" ${FASMG_ARGS} "${TARGET_BIN}"
    DEPENDS ${COMPILED_SRCS}
    VERBATIM
  )

  # compile_commands.json stuff
  if(CEDEV_IDE_HELPER)
    set(_ide_target "${CEDEV_TARGET}__compiledb")
    add_library(${_ide_target} OBJECT EXCLUDE_FROM_ALL ${CEDEV_SOURCES})
    
    target_include_directories(${_ide_target} PRIVATE 
      "${CEDEV_INCLUDE}" 
      "${CEDEV_INCLUDE}/c++" 
      ${CEDEV_INCLUDE_DIRECTORIES} # user provided includes
    )

    set(_ide_cxx_flags)
    if(CEDEV_CXX_STANDARD)
       set(_ide_cxx_flags "-std=c++${CEDEV_CXX_STANDARD}")
    endif()

    target_compile_options(${_ide_target} PRIVATE
      -nostdinc -fno-threadsafe-statics -D__TICE__ ${_debug_flags} ${_CEDEV_WARNINGS} -Oz
      $<$<OR:$<COMPILE_LANGUAGE:CXX>,$<BOOL:${CEDEV_CXX_MODE}>>:${_ide_cxx_flags} $<$<BOOL:${CEDEV_FORCE_CXX_LANGUAGE}>:-xc++> -fno-use-cxa-atexit -DEASTL_USER_CONFIG_HEADER=<__EASTL_user_config.h>>
      ${_user_compile_options}
    )
  endif()

  # convbin
  set(CONVBIN_ARGS -k 8xp -n "${CEDEV_NAME}")
  set(USE_ARCHIVED ${CEDEV_ARCHIVED})
  set(USE_COMPRESS ${CEDEV_COMPRESS})

  if(NOT DEFINED USE_ARCHIVED)
    set(USE_ARCHIVED ${CEDEV_DEFAULT_ARCHIVED})
  endif()
  if(NOT DEFINED USE_COMPRESS)
    set(USE_COMPRESS ${CEDEV_DEFAULT_COMPRESS})
  endif()

  if(USE_ARCHIVED)
    list(APPEND CONVBIN_ARGS -r)
  endif()
  if(USE_COMPRESS)
    if(CEDEV_MODE)
      list(APPEND CONVBIN_ARGS -e ${CEDEV_MODE} -k 8xp-compressed)
    else()
      list(APPEND CONVBIN_ARGS -e zx7 -k 8xp-compressed)
    endif()
  endif()

  add_custom_command(
    OUTPUT "${TARGET_8XP}"
    COMMAND "${CMAKE_COMMAND}" -E echo "[convbin] ${TARGET_8XP}"
    COMMAND "${CEDEV_CONVBIN}" ${CONVBIN_ARGS} -i "${TARGET_BIN}" -o "${TARGET_8XP}"
    DEPENDS "${TARGET_BIN}"
    VERBATIM
  )

  add_custom_target(${CEDEV_TARGET} ALL DEPENDS "${TARGET_8XP}")

  add_custom_command(TARGET ${CEDEV_TARGET} POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E echo "Built: ${TARGET_8XP}"
  )
endfunction()
