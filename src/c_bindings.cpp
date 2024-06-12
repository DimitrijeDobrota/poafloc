#include "args.h"
#include "args.hpp"

#include <cstdarg>

namespace args {

int args_parse(const args_argp_t *argp, int argc, char *argv[], unsigned flags,
               void *input) {
    return parse(argp, argc, argv, flags, input);
}

void *args_parser_input(args_parser_t *parser) { return parser->input(); }

void args_usage(args_parser_t *parser) { return usage(parser); }

void args_help(const args_parser_t *parser, FILE *stream, unsigned flags) {
    help(parser, stream, flags);
}

void args_failure(const args_parser_t *parser, int status, int errnum,
                  const char *fmt, ...) {
    std::va_list args;
    va_start(args, fmt);
    failure(parser, status, errnum, fmt, args);
    va_end(args);
}

} // namespace args
