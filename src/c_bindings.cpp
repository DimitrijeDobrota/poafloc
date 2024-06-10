#include "args.h"
#include "args.hpp"

namespace args {

int args_parse(args_argp_t *argp, int argc, char *argv[], void *input) {
    return Parser::parse(argp, argc, argv, input);
}

void *args_parser_input(args_parser *parser) { return parser->input; }

} // namespace args
