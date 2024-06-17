#ifndef POAFLOC_POAFLOC_H
#define POAFLOC_POAFLOC_H

#ifdef __cplusplus

#  include <cstdio>

#  include "poafloc/poafloc_export.hpp"

// NOLINTNEXTLINE
#  define MANGLE_ENUM(enumn, name) name

#  define ENUM_OPTION Option
#  define ENUM_KEY Key
#  define ENUM_HELP Help
#  define ENUM_PARSE Parse

#  define INIT_0 = 0
#  define INIT_NULLPTR = nullptr

extern "C"
{
namespace poafloc {

#else

#  include <stdio.h>

#  define MANGLE_ENUM(enumn, name) POAFLOC_##enumn##_##name

#  define ENUM_OPTION poafloc_option_e
#  define ENUM_KEY poafloc_key_e
#  define ENUM_HELP poafloc_help_e
#  define ENUM_PARSE poafloc_parse_e

#  define INIT_0
#  define INIT_NULLPTR

#endif

struct Parser;
typedef struct Parser poafloc_parser_t;  // NOLINT

typedef struct  // NOLINT
    POAFLOC_EXPORT
{
  char const* name INIT_NULLPTR;
  int key INIT_0;
  char const* arg INIT_NULLPTR;
  int flags INIT_0;
  char const* message INIT_NULLPTR;
  int group INIT_0;
} poafloc_option_t;

// NOLINTNEXTLINE
typedef int (*poafloc_parse_f)(int key,
                               const char* arg,
                               poafloc_parser_t* parser);

// NOLINTNEXTLINE
typedef struct
{
  poafloc_option_t const* options INIT_NULLPTR;
  poafloc_parse_f parse INIT_NULLPTR;
  char const* doc INIT_NULLPTR;
  char const* message INIT_NULLPTR;
} poafloc_arg_t;

enum ENUM_OPTION
{
  MANGLE_ENUM(OPTION, ARG_OPTIONAL) = 0x1,
  MANGLE_ENUM(OPTION, HIDDEN)       = 0x2,
  MANGLE_ENUM(OPTION, ALIAS)        = 0x4,
};

enum ENUM_KEY
{
  MANGLE_ENUM(KEY, ARG)     = 0,
  MANGLE_ENUM(KEY, END)     = 0x1000001,
  MANGLE_ENUM(KEY, NO_ARGS) = 0x1000002,
  MANGLE_ENUM(KEY, INIT)    = 0x1000003,
  MANGLE_ENUM(KEY, SUCCESS) = 0x1000004,
  MANGLE_ENUM(KEY, ERROR)   = 0x1000005,
};

enum ENUM_HELP
{
  MANGLE_ENUM(HELP, SHORT_USAGE) = 0x1,
  MANGLE_ENUM(HELP, USAGE)       = 0x2,
  MANGLE_ENUM(HELP, SEE)         = 0x4,
  MANGLE_ENUM(HELP, LONG)        = 0x8,

  MANGLE_ENUM(HELP, EXIT_ERR) = 0x10,
  MANGLE_ENUM(HELP, EXIT_OK)  = 0x20,

  MANGLE_ENUM(HELP, STD_ERR) = MANGLE_ENUM(HELP, SEE)
      | MANGLE_ENUM(HELP, EXIT_ERR),

  MANGLE_ENUM(HELP, STD_HELP) = MANGLE_ENUM(HELP, LONG)
      | MANGLE_ENUM(HELP, EXIT_OK),

  MANGLE_ENUM(HELP, STD_USAGE) = MANGLE_ENUM(HELP, USAGE)
      | MANGLE_ENUM(HELP, EXIT_ERR),
};

enum ENUM_PARSE
{
  MANGLE_ENUM(PARSE, NO_ERRS)  = 0x1,
  MANGLE_ENUM(PARSE, NO_HELP)  = 0x2,
  MANGLE_ENUM(PARSE, NO_EXIT)  = 0x4,
  MANGLE_ENUM(PARSE, SILENT)   = 0x8,
  MANGLE_ENUM(PARSE, IN_ORDER) = 0x10,
};

#if !defined __cplusplus || defined WITH_C_BINDINGS

void poafloc_usage(poafloc_parser_t* parser);

void poafloc_help(const poafloc_parser_t* state, FILE* stream, unsigned flags);

int poafloc_parse(const poafloc_arg_t* argp,
                  int argc,
                  char* argv[],
                  unsigned flags,
                  void* input);

void* poafloc_parser_input(poafloc_parser_t* parser);

void poafloc_failure(const poafloc_parser_t* parser,
                     int status,
                     int errnum,
                     const char* fmt,
                     ...);

#endif

#undef MANGLE_ENUM
#undef ENUM_OPTION
#undef ENUM_KEY

#undef INIT_0
#undef INIT_NULLPTR

#ifdef __cplusplus
}  // namespace poafloc
}  // extern "C"
#endif

#endif
