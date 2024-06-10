#ifndef ARGS_H
#define ARGS_H

#ifdef __cplusplus
extern "C" {
struct Parser;
typedef Parser args_parser;
#else
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

enum args_option_e {
    ARGS_OPTION_ARG_OPTIONAL = 0x1,
    ARGS_OPTION_HIDDEN = 0x2,
    ARGS_OPTION_ALIAS = 0x4,
};

enum args_key_e {
    ARGS_KEY_ARG = 0,
    ARGS_KEY_END = 0x1000001,
    ARGS_KEY_NO_ARGS = 0x1000002,
    ARGS_KEY_INIT = 0x1000003,
    ARGS_KEY_SUCCESS = 0x1000004,
    ARGS_KEY_ERROR = 0x1000005,
};

struct args_Parser;

typedef int (*args_parse_f)(int key, const char *arg, args_parser *parser);
typedef struct {
    const args_option_t *options;
    const args_parse_f parse;
    const char *doc;
    const char *message;
} args_argp_t;

int args_parse(args_argp_t *argp, int argc, char *argv[], void *input);
void *args_parser_input(args_parser *parser);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
