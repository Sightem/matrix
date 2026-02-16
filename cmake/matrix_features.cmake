include_guard(GLOBAL)


set(_matrix_optional_default ON)
if(MATRIX_BUILD_CE)
  set(_matrix_optional_default OFF)
endif()

option(MATRIX_FEATURE_PROJECTION "Enable vector projection decomposition operation" ${_matrix_optional_default})
option(MATRIX_FEATURE_MINOR_MATRIX "Enable minor matrix operation" ${_matrix_optional_default})
option(MATRIX_FEATURE_COFACTOR "Enable cofactor element operation" ${_matrix_optional_default})
option(MATRIX_FEATURE_CRAMER "Enable Cramer's rule operation (and det-replace-column helper for steps)" ${_matrix_optional_default})

option(MATRIX_SHELL_DEBUG "Enable shell debug logging" OFF)

function(matrix_resolve_feature_deps)
  # for future dependency toggles
endfunction()

function(matrix_get_feature_compile_definitions out_var)
  set(defs)

  if(MATRIX_FEATURE_PROJECTION)
    list(APPEND defs MATRIX_CORE_ENABLE_PROJECTION=1 MATRIX_SHELL_ENABLE_PROJECTION=1)
  else()
    list(APPEND defs MATRIX_CORE_ENABLE_PROJECTION=0 MATRIX_SHELL_ENABLE_PROJECTION=0)
  endif()

  if(MATRIX_FEATURE_MINOR_MATRIX)
    list(APPEND defs MATRIX_CORE_ENABLE_MINOR_MATRIX=1 MATRIX_SHELL_ENABLE_MINOR_MATRIX=1)
  else()
    list(APPEND defs MATRIX_CORE_ENABLE_MINOR_MATRIX=0 MATRIX_SHELL_ENABLE_MINOR_MATRIX=0)
  endif()

  if(MATRIX_FEATURE_COFACTOR)
    list(APPEND defs MATRIX_CORE_ENABLE_COFACTOR=1 MATRIX_SHELL_ENABLE_COFACTOR=1)
  else()
    list(APPEND defs MATRIX_CORE_ENABLE_COFACTOR=0 MATRIX_SHELL_ENABLE_COFACTOR=0)
  endif()

  if(MATRIX_FEATURE_CRAMER)
    list(APPEND defs MATRIX_CORE_ENABLE_CRAMER=1 MATRIX_SHELL_ENABLE_CRAMER=1)
  else()
    list(APPEND defs MATRIX_CORE_ENABLE_CRAMER=0 MATRIX_SHELL_ENABLE_CRAMER=0)
  endif()

  if(MATRIX_SHELL_DEBUG)
    list(APPEND defs MATRIX_SHELL_ENABLE_DEBUG=1)
  else()
    list(APPEND defs MATRIX_SHELL_ENABLE_DEBUG=0)
  endif()

  set(${out_var} "${defs}" PARENT_SCOPE)
endfunction()

function(matrix_get_feature_compile_options out_var)
  matrix_get_feature_compile_definitions(defs)
  set(opts "${defs}")
  list(TRANSFORM opts PREPEND "-D")
  set(${out_var} "${opts}" PARENT_SCOPE)
endfunction()

function(matrix_print_feature_summary)
  message(STATUS "Matrix features: PROJECTION=${MATRIX_FEATURE_PROJECTION} MINOR_MATRIX=${MATRIX_FEATURE_MINOR_MATRIX} COFACTOR=${MATRIX_FEATURE_COFACTOR} CRAMER=${MATRIX_FEATURE_CRAMER} SHELL_DEBUG=${MATRIX_SHELL_DEBUG}")
endfunction()
