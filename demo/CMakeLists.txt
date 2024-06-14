add_executable(demo main.cpp)
target_link_libraries(demo PRIVATE poafloc)
target_include_directories(demo PRIVATE ../include)
set_target_properties(demo PROPERTIES LANGUAGE CXX)
set_target_properties(demo PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

add_executable(cdemo main.c)
target_link_libraries(cdemo PRIVATE poafloc)
target_include_directories(cdemo PRIVATE ../include)
set_target_properties(cdemo PROPERTIES LANGUAGE C)
set_target_properties(cdemo PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)
