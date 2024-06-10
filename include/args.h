#ifndef ARGS_H
#define ARGS_H

#ifdef __cplusplus

#define MANGLE_ENUM(enumn, name) name
#define ENUM_OPTION Option
#define ENUM_KEY Key

extern "C" {
namespace args {

struct Parser;
typedef Parser args_parser;

#else

#define MANGLE_ENUM(enumn, name) ARGS_##enumn##_##name
#define ENUM_OPTION args_option_e
#define ENUM_KEY args_key_e

struct __Parser;
typedef struct __Parser args_parser;

#endif

typedef struct {
    const char *name;
    int key;
    const char *arg;
    int flags;
    const char *message;
    int group;
} args_option_t;

typedef int (*args_parse_f)(int key, const char *arg, args_parser *parser);

typedef struct {
    const args_option_t *options;
    const args_parse_f parse;
    const char *doc;
    const char *message;
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

#if !defined __cplusplus || defined WITH_C_BINDINGS

int args_parse(args_argp_t *argp, int argc, char *argv[], void *input);
void *args_parser_input(args_parser *parser);

#endif

#undef MANGLE_ENUM
#undef ENUM_OPTION
#undef ENUM_KEY

#ifdef __cplusplus
} // namespace args
} // extern "C"
#endif

#endif
