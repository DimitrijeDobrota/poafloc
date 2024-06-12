#ifndef ARGS_H
#define ARGS_H

#ifdef __cplusplus

#include <cstdio>

#define MANGLE_ENUM(enumn, name) name
#define ENUM_OPTION Option
#define ENUM_KEY Key
#define ENUM_HELP Help
#define ENUM_PARSE Parse

extern "C" {
namespace args {

struct Parser;
typedef Parser args_parser;

#else

#include <stdio.h>

#define MANGLE_ENUM(enumn, name) ARGS_##enumn##_##name
#define ENUM_OPTION args_option_e
#define ENUM_KEY args_key_e
#define ENUM_HELP args_help_e
#define ENUM_PARSE args_parse_e

struct __Parser;
typedef struct __Parser args_parser;

#endif

typedef struct {
    char const *name;
    int key;
    char const *arg;
    int flags;
    char const *message;
    int group;
} args_option_t;

typedef int (*args_parse_f)(int key, const char *arg, args_parser *parser);

typedef struct {
    args_option_t const *options;
    args_parse_f parse;
    char const *doc;
    char const *message;
} args_argp_t;

enum ENUM_OPTION {
    MANGLE_ENUM(OPTION, ARG_OPTIONAL) = 0x1,
    MANGLE_ENUM(OPTION, HIDDEN) = 0x2,
    MANGLE_ENUM(OPTION, ALIAS) = 0x4,
};

enum ENUM_KEY {
    MANGLE_ENUM(KEY, ARG) = 0,
    MANGLE_ENUM(KEY, END) = 0x1000001,
    MANGLE_ENUM(KEY, NO_ARGS) = 0x1000002,
    MANGLE_ENUM(KEY, INIT) = 0x1000003,
    MANGLE_ENUM(KEY, SUCCESS) = 0x1000004,
    MANGLE_ENUM(KEY, ERROR) = 0x1000005,
};

enum ENUM_HELP {

    MANGLE_ENUM(HELP, SHORT_USAGE) = 0x1,
    MANGLE_ENUM(HELP, USAGE) = 0x2,
    MANGLE_ENUM(HELP, SEE) = 0x4,
    MANGLE_ENUM(HELP, LONG) = 0x8,

    MANGLE_ENUM(HELP, EXIT_ERR) = 0x10,
    MANGLE_ENUM(HELP, EXIT_OK) = 0x20,

    MANGLE_ENUM(HELP, STD_ERR) = MANGLE_ENUM(HELP, SEE) |
                                 MANGLE_ENUM(HELP, EXIT_ERR),

    MANGLE_ENUM(HELP, STD_HELP) = MANGLE_ENUM(HELP, LONG) |
                                  MANGLE_ENUM(HELP, EXIT_OK),

    MANGLE_ENUM(HELP, STD_USAGE) = MANGLE_ENUM(HELP, USAGE) |
                                   MANGLE_ENUM(HELP, EXIT_ERR),
};

enum ENUM_PARSE {
    MANGLE_ENUM(PARSE, NO_ERRS) = 0x1,
    // MANGLE_ENUM(PARSE, NO_ARGS),
    MANGLE_ENUM(PARSE, NO_HELP) = 0x2,
    MANGLE_ENUM(PARSE, NO_EXIT) = 0x4,
    MANGLE_ENUM(PARSE, SILENT) = 0x8,
    MANGLE_ENUM(PARSE, IN_ORDER) = 0x10,
};

#if !defined __cplusplus || defined WITH_C_BINDINGS

void *args_parser_input(args_parser *parser);

void args_usage(args_parser *parser);

void argp_help(const args_parser *state, FILE *stream, unsigned flags);

int args_parse(const args_argp_t *argp, int argc, char *argv[], unsigned flags,
               void *input);

void argp_failure(const args_parser *parser, int status, int errnum,
                  const char *fmt, ...);

#endif

#undef MANGLE_ENUM
#undef ENUM_OPTION
#undef ENUM_KEY

#ifdef __cplusplus
} // namespace args
} // extern "C"
#endif

#endif
