add_library(poafloc STATIC poafloc.cpp help.cpp trie.cpp c_bindings.cpp)
target_include_directories(poafloc PRIVATE ../include)
target_compile_definitions(poafloc PRIVATE WITH_C_BINDINGS)
set_target_properties(poafloc PROPERTIES LINKER_LANGUAGE CXX)

set_target_properties(poafloc PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
	OUTPUT_NAME "poafloc"
	DEBUG_POSTFIX "d"
    PUBLIC_HEADER "include/poafloc.hpp;include/poafloc.h"
	MACOSX_RPATH ON
	WINDOWS_EXPORT_ALL_SYMBOLS ON
)
