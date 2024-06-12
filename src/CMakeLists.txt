add_library(args STATIC args.cpp help.cpp trie.cpp c_bindings.cpp)
target_include_directories(args PUBLIC ../include)
target_compile_definitions(args PRIVATE WITH_C_BINDINGS)
set_target_properties(args PROPERTIES LINKER_LANGUAGE CXX)

set_target_properties(args PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
	OUTPUT_NAME "args"
	DEBUG_POSTFIX "d"
    PUBLIC_HEADER "include/args.hpp;include/args.h"
	MACOSX_RPATH ON
	WINDOWS_EXPORT_ALL_SYMBOLS ON
)
