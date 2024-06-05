#include "args.hpp"

#include <cstdint>
#include <vector>

void error(const std::string &message) {
    std::cerr << message << std::endl;
    exit(1);
}

struct arguments_t {
    const char *output_file = 0;

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
    case 'o': arguments->output_file = arg; break;
    default: arguments->args.push_back(arg);
    }

    return 0;
}

// clang-format off
static const Parser::option_t options[] = {
    {            0, 'o', "file"},
    {      "debug", 'd',      0},
    {        "hex", 'h',      0},
    {"relocatable", 'r',      0},
    {0},
};
// clang-format on

int main(int argc, char *argv[]) {
    Parser::argp_t argp = {options, parse_opt};
    arguments_t arguments;

    if (Parser::parse(&argp, argc, argv, &arguments)) {
        error("There was an error while parsing arguments");
    }

    std::cout << "Command line options: " << std::endl;

    std::cout << "\t      output: " << arguments.output_file << std::endl;
    std::cout << "\t         hex: " << arguments.hex << std::endl;
    std::cout << "\t       debug: " << arguments.debug << std::endl;
    std::cout << "\t relocatable: " << arguments.relocatable << std::endl;

    std::cout << "\t args: ";
    for (const auto &arg : arguments.args) std::cout << arg << " ";
    std::cout << std::endl;

    return 0;
}
