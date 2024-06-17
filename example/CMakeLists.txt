cmake_minimum_required(VERSION 3.14)

project(poaflocExamples CXX)

include(../cmake/project-is-top-level.cmake)
include(../cmake/folders.cmake)

if(PROJECT_IS_TOP_LEVEL)
  find_package(poafloc REQUIRED)
endif()

add_custom_target(run-examples)

function(add_example NAME EXT)
   add_executable("${NAME}" "${NAME}.${EXT}")
  target_link_libraries("${NAME}" PRIVATE poafloc::poafloc)
  target_compile_features("${NAME}" PRIVATE cxx_std_20)
  add_custom_target("run_${NAME}" COMMAND "${NAME}" VERBATIM)
  add_dependencies("run_${NAME}" "${NAME}")
  add_dependencies(run-examples "run_${NAME}")
endfunction()

add_example(example_c c)
add_example(example_cpp cpp)

add_folders(Example)
