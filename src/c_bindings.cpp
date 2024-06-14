#include "poafloc.h"
#include "poafloc.hpp"

#include <cstdarg>

namespace poafloc {

int poafloc_parse(const poafloc_arg_t *argp, int argc, char *argv[], unsigned flags,
               void *input) {
    return parse(argp, argc, argv, flags, input);
}

void *poafloc_parser_input(poafloc_parser_t *parser) { return parser->input(); }

void poafloc_usage(poafloc_parser_t *parser) { return usage(parser); }

void poafloc_help(const poafloc_parser_t *parser, FILE *stream, unsigned flags) {
    help(parser, stream, flags);
}

void poafloc_failure(const poafloc_parser_t *parser, int status, int errnum,
                  const char *fmt, ...) {
    std::va_list args;
    va_start(args, fmt);
    failure(parser, status, errnum, fmt, args);
    va_end(args);
}

} // namespace args
