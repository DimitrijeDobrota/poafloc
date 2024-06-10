set(GENERATE_OUT "${CMAKE_BINARY_DIR}/bin")

add_executable(demo
    main.cpp
)

target_link_libraries(demo PRIVATE args)

set_target_properties(demo PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    RUNTIME_OUTPUT_DIRECTORY "${GENERATE_OUT}"
)
