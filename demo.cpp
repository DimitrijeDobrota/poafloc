#include "args.hpp"

#include <cstdint>
#include <unordered_map>
#include <vector>

void error(const std::string &message) {
    std::cerr << message << std::endl;
    exit(1);
}

struct arguments_t {
    std::vector<const char *> args;
    std::unordered_map<std::string, uint32_t> places;

    const char *output_file = 0;

    bool debug = 0;
    bool hex = 0;
    bool relocatable = 0;
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
    case 'p': {
        const std::string s = arg;

        const auto pos = s.find('@');
        if (pos == std::string::npos) error("Invalid argument for -place");

        const auto value = stol(s.substr(pos + 1), 0, 16);
        const auto [_, a] = arguments->places.emplace(s.substr(0, pos), value);
        if (!a) error("duplicate place for the same section");

        break;
    }
    default: arguments->args.push_back(arg);
    }

    return 0;
}

// clang-format off
static const Parser::option_t options[] = {
    {      "--debug", 'd', 0},
    {        "--hex", 'h', 0},
    {"--relocatable", 'r', 0},
    {           "-o", 'o', 1},
    {      "--place", 'p', 1},
    {0},
};
// clang-format on

int main(int argc, const char *argv[]) {
    Parser::argp_t argp = {options, parse_opt};
    arguments_t arguments;

    if (Parser::parse(&argp, argc, argv, &arguments)) {
        error("There was an error while parsing arguments");
    }

    if (arguments.args.size() == 0) {
        error("please provide at least one input file");
    }

    if (!arguments.hex && !arguments.relocatable) {
        error("please provide hex or relocatable flags");
    }

    return 0;
}
