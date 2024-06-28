if(PROJECT_IS_TOP_LEVEL)
  set(
      CMAKE_INSTALL_INCLUDEDIR "include/poafloc-${PROJECT_VERSION}"
      CACHE STRING ""
  )
  set_property(CACHE CMAKE_INSTALL_INCLUDEDIR PROPERTY TYPE PATH)
endif()

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

# find_package(<package>) call for consumers to find this project
set(package poafloc)

install(
    DIRECTORY
    include/
    "${PROJECT_BINARY_DIR}/export/"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    COMPONENT poafloc_Development
)

install(
    TARGETS poafloc_poafloc
    EXPORT poaflocTargets
    RUNTIME #
    COMPONENT poafloc_Runtime
    LIBRARY #
    COMPONENT poafloc_Runtime
    NAMELINK_COMPONENT poafloc_Development
    ARCHIVE #
    COMPONENT poafloc_Development
    INCLUDES #
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)

write_basic_package_version_file(
    "${package}ConfigVersion.cmake"
    COMPATIBILITY SameMajorVersion
)

# Allow package maintainers to freely override the path for the configs
set(
    poafloc_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/${package}"
    CACHE STRING "CMake package config location relative to the install prefix"
)
set_property(CACHE poafloc_INSTALL_CMAKEDIR PROPERTY TYPE PATH)
mark_as_advanced(poafloc_INSTALL_CMAKEDIR)

install(
    FILES cmake/install-config.cmake
    DESTINATION "${poafloc_INSTALL_CMAKEDIR}"
    RENAME "${package}Config.cmake"
    COMPONENT poafloc_Development
)

install(
    FILES "${PROJECT_BINARY_DIR}/${package}ConfigVersion.cmake"
    DESTINATION "${poafloc_INSTALL_CMAKEDIR}"
    COMPONENT poafloc_Development
)

install(
    EXPORT poaflocTargets
    NAMESPACE poafloc::
    DESTINATION "${poafloc_INSTALL_CMAKEDIR}"
    COMPONENT poafloc_Development
)

if(PROJECT_IS_TOP_LEVEL)
  include(CPack)
endif()
