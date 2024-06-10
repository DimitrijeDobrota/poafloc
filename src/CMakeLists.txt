add_library(args args.cpp help.cpp trie.cpp)
target_include_directories(args PUBLIC ../include)

set_target_properties(args PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    PUBLIC_HEADER ../include/args.hpp
)

install(TARGETS args
    LIBRARY DESTINATION lib
    PUBLIC_HEADER DESTINATION include
)
