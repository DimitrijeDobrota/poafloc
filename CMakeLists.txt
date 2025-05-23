cmake_minimum_required(VERSION 3.14)

include(cmake/prelude.cmake)

project(
    poafloc
    VERSION 2.0.0
    DESCRIPTION "Parser Of Arguments For Lines Of Commands"
    HOMEPAGE_URL "https://git.dimitrijedobrota.com/poafloc.git"
    LANGUAGES C CXX
)

include(cmake/project-is-top-level.cmake)
include(cmake/variables.cmake)

find_package(based 0.2.0 CONFIG REQUIRED)

# ---- Declare library ----

add_library(
    poafloc_poafloc
    source/poafloc.cpp
    # source/help.cpp
    # source/trie.cpp
)
add_library(poafloc::poafloc ALIAS poafloc_poafloc)
target_link_libraries(poafloc_poafloc PUBLIC based::based)

include(GenerateExportHeader)
generate_export_header(
    poafloc_poafloc
    BASE_NAME poafloc
    EXPORT_FILE_NAME export/poafloc/poafloc_export.hpp
    CUSTOM_CONTENT_FROM_VARIABLE pragma_suppress_c4251
)

if(NOT BUILD_SHARED_LIBS)
  target_compile_definitions(poafloc_poafloc PUBLIC POAFLOC_STATIC_DEFINE)
endif()

set_target_properties(
    poafloc_poafloc PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN YES
    VERSION "${PROJECT_VERSION}"
    SOVERSION "${PROJECT_VERSION_MAJOR}"
    EXPORT_NAME poafloc
    OUTPUT_NAME poafloc
)

target_include_directories(
    poafloc_poafloc ${warning_guard}
    PUBLIC
    "\$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
)

target_include_directories(
    poafloc_poafloc SYSTEM
    PUBLIC
    "\$<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/export>"
)

target_compile_features(poafloc_poafloc PUBLIC cxx_std_20)

# ---- Install rules ----

if(NOT CMAKE_SKIP_INSTALL_RULES)
  include(cmake/install-rules.cmake)
endif()

# ---- Examples ----

if(PROJECT_IS_TOP_LEVEL)
  option(BUILD_EXAMPLES "Build examples tree." "${poafloc_DEVELOPER_MODE}")
  if(BUILD_EXAMPLES)
    add_subdirectory(example)
  endif()
endif()

# ---- Developer mode ----

if(NOT poafloc_DEVELOPER_MODE)
  return()
elseif(NOT PROJECT_IS_TOP_LEVEL)
  message(
      AUTHOR_WARNING
      "Developer mode is intended for developers of poafloc"
  )
endif()

include(cmake/dev-mode.cmake)
