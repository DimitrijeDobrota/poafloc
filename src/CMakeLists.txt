add_library(args STATIC args.cpp help.cpp trie.cpp c_bindings.cpp)
target_include_directories(args PUBLIC ../include)
target_compile_definitions(args PRIVATE WITH_C_BINDINGS)
set_target_properties(args PROPERTIES LINKER_LANGUAGE CXX)

set_target_properties(args PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    PUBLIC_HEADER ../include/args.hpp
)

install(TARGETS args
    LIBRARY DESTINATION lib
    PUBLIC_HEADER DESTINATION include
)
