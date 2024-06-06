#include "args.hpp"

#include <cstdint>
#include <vector>

void error(const std::string &message) { std::cerr << message << std::endl; }

struct arguments_t {
    const char *output_file = "";
    const char *input_file = "";

    bool debug = 0;
    bool hex = 0;
    bool relocatable = 0;

    std::vector<const char *> args;
};

int parse_opt(int key, const char *arg, void *input) {
    auto arguments = (arguments_t *)input;

    switch (key) {
    case 'd': arguments->debug = true; break;
    case 'h':
        if (arguments->relocatable) error("cannot mix -hex and -relocatable");
        arguments->hex = true;
        break;
    case 'r':
        if (arguments->hex) error("cannot mix -hex and -relocatable");
        arguments->relocatable = true;
        break;
    case 'o': arguments->output_file = arg ? arg : "stdout"; break;
    case 'i': arguments->input_file = arg; break;
    default: arguments->args.push_back(arg);
    }

    return 0;
}

using enum Parser::Option;

// clang-format off
static const Parser::option_t options[] = {
    {     "output", 'o', "file", ARG_OPTIONAL},
    {            0, 'i', "file",            0},
    {      "debug", 'd',      0,            0},
    {        "hex", 'h',      0,            0},
    {"relocatable", 'r',      0,            0},
    {0},
};
// clang-format on

int main(int argc, char *argv[]) {
    Parser::argp_t argp = {options, parse_opt};
    Parser parser(&argp);

    arguments_t arguments;

    if (parser.parse(argc, argv, &arguments)) {
        error("There was an error while parsing arguments");
        return 1;
    }

    std::cout << "Command line options: " << std::endl;

    std::cout << "\t       input: " << arguments.input_file << std::endl;
    std::cout << "\t      output: " << arguments.output_file << std::endl;
    std::cout << "\t         hex: " << arguments.hex << std::endl;
    std::cout << "\t       debug: " << arguments.debug << std::endl;
    std::cout << "\t relocatable: " << arguments.relocatable << std::endl;

    std::cout << "\t args: ";
    for (const auto &arg : arguments.args) std::cout << arg << " ";
    std::cout << std::endl;

    return 0;
}
