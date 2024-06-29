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

add_test(NAME poafloc_test_empty COMMAND poafloc_test)
set_tests_properties(poafloc_test_empty PROPERTIES PASS_REGULAR_EXPRESSION "init\nnoargs\nend\nsuccess")

add_test(NAME poafloc_test_arg_one COMMAND poafloc_test a1)
set_tests_properties(poafloc_test_arg_one PROPERTIES PASS_REGULAR_EXPRESSION "init\narg:a1\nend\nsuccess")

add_test(NAME poafloc_test_arg_two COMMAND poafloc_test a1 a2)
set_tests_properties(poafloc_test_arg_two PROPERTIES PASS_REGULAR_EXPRESSION "init\narg:a1\narg:a2\nend\nsuccess")

add_test(NAME poafloc_test_short COMMAND poafloc_test -s)
set_tests_properties(poafloc_test_short PROPERTIES PASS_REGULAR_EXPRESSION "init\ns\nnoargs\nend\nsuccess")

add_test(NAME poafloc_test_long COMMAND poafloc_test --long)
set_tests_properties(poafloc_test_long PROPERTIES PASS_REGULAR_EXPRESSION "init\nl\nnoargs\nend\nsuccess")

add_test(NAME poafloc_test_short_arg COMMAND poafloc_test -a a1)
set_tests_properties(poafloc_test_short_arg PROPERTIES PASS_REGULAR_EXPRESSION "init\na:a1\nnoargs\nend\nsuccess")

add_test(NAME poafloc_test_long_arg COMMAND poafloc_test --arg a1)
set_tests_properties(poafloc_test_long_arg PROPERTIES PASS_REGULAR_EXPRESSION "init\na:a1\nnoargs\nend\nsuccess")

add_test(NAME poafloc_test_short_arg_equal COMMAND poafloc_test -aa1)
set_tests_properties(poafloc_test_short_arg_equal PROPERTIES PASS_REGULAR_EXPRESSION "init\na:a1\nnoargs\nend\nsuccess")

add_test(NAME poafloc_test_long_arg_equal COMMAND poafloc_test --arg=a1)
set_tests_properties(poafloc_test_long_arg_equal PROPERTIES PASS_REGULAR_EXPRESSION "init\na:a1\nnoargs\nend\nsuccess")

add_test(NAME poafloc_test_short_arg_without COMMAND poafloc_test -a)
set_tests_properties(poafloc_test_short_arg_without PROPERTIES WILL_FAIL TRUE)

add_test(NAME poafloc_test_long_arg_without COMMAND poafloc_test --arg)
set_tests_properties(poafloc_test_long_arg_without PROPERTIES WILL_FAIL TRUE)

add_test(NAME poafloc_test_short_opt COMMAND poafloc_test -oa1)
set_tests_properties(poafloc_test_short_opt PROPERTIES PASS_REGULAR_EXPRESSION "init\no:a1\nnoargs\nend\nsuccess")

add_test(NAME poafloc_test_long_opt COMMAND poafloc_test --opt=a1)
set_tests_properties(poafloc_test_long_opt PROPERTIES PASS_REGULAR_EXPRESSION "init\no:a1\nnoargs\nend\nsuccess")

add_test(NAME poafloc_test_short_opt_without COMMAND poafloc_test -o)
set_tests_properties(poafloc_test_short_opt_without PROPERTIES PASS_REGULAR_EXPRESSION "init\no:default\nnoargs\nend\nsuccess")

add_test(NAME poafloc_test_long_opt_without COMMAND poafloc_test --opt)
set_tests_properties(poafloc_test_long_opt_without PROPERTIES PASS_REGULAR_EXPRESSION "init\no:default\nnoargs\nend\nsuccess")

add_test(NAME poafloc_test_short_opt_after COMMAND poafloc_test -o a1)
set_tests_properties(poafloc_test_short_opt_after PROPERTIES PASS_REGULAR_EXPRESSION "init\no:default\narg:a1\nend\nsuccess")

add_test(NAME poafloc_test_long_opt_after COMMAND poafloc_test --opt a1)
set_tests_properties(poafloc_test_long_opt_after PROPERTIES PASS_REGULAR_EXPRESSION "init\no:default\narg:a1\nend\nsuccess")

# ---- End-of-file commands ----

add_folders(Test)
