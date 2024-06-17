cmake_minimum_required(VERSION 3.14)

project(poaflocTests LANGUAGES CXX)

include(../cmake/project-is-top-level.cmake)
include(../cmake/folders.cmake)

# ---- Dependencies ----

if(PROJECT_IS_TOP_LEVEL)
  find_package(poafloc REQUIRED)
  enable_testing()
endif()

# ---- Tests ----

add_executable(poafloc_test source/poafloc_test.cpp)
target_link_libraries(poafloc_test PRIVATE poafloc::poafloc)
target_compile_features(poafloc_test PRIVATE cxx_std_20)

add_test(NAME poafloc_test COMMAND poafloc_test)

# ---- End-of-file commands ----

add_folders(Test)
