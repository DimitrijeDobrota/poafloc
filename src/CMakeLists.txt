add_library(args STATIC args.cpp help.cpp trie.cpp)
target_include_directories(args PUBLIC ../include)
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

 add_library(cargs STATIC c_bindings.cpp args.cpp help.cpp trie.cpp)
target_include_directories(cargs PUBLIC ../include)
set_target_properties(cargs PROPERTIES LINKER_LANGUAGE CXX)

set_target_properties(cargs PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    PUBLIC_HEADER ../include/args.h
)

install(TARGETS cargs
    LIBRARY DESTINATION lib
    PUBLIC_HEADER DESTINATION include
)
