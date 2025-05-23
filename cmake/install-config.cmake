include(CMakeFindDependencyMacro)
find_dependency(based)

if(based_FOUND)
  include("${CMAKE_CURRENT_LIST_DIR}/poaflocTargets.cmake")
endif()
