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
    case 777: arguments->debug = true; break;
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
    case Parser::Key::ARG: arguments->args.push_back(arg); break;
    case Parser::Key::ERROR: std::cerr << "handled error\n";
    }

    return 0;
}

using enum Parser::Option;

// clang-format off
static const Parser::option_t options[] = {
    {           0,  'R',      0,               0,        "random 0-group option"},
    {           0,    0,      0,               0,                 "Program mode", 1},
    {"relocatable", 'r',      0,               0, "Output in relocatable format"},
    {        "hex", 'h',      0,               0,         "Output in hex format"},
    {"hexadecimal",   0,      0, ALIAS | HIDDEN},
    {            0,   0,      0,               0,               "For developers", 4},
    {      "debug", 777,      0,               0,        "Enable debugging mode"},
    {            0,   0,      0,               0,                 "Input/output", 3},
    {     "output", 'o', "file",    ARG_OPTIONAL,  "Output file, default stdout"},
    {            0, 'i', "file",               0,                  "Input  file"},
    {            0,   0,      0,               0,        "Informational Options", -1},
    {0},
};
// clang-format on

int main(int argc, char *argv[]) {
    Parser::argp_t argp = {
        options,
        parse_opt,
        "doc string\nother usage",
        "First half of the message\vsecond half of the message"
    };
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
