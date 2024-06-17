#include <cstdint>
#include <iostream>
#include <vector>

#include <poafloc/poafloc.hpp>

using namespace poafloc;  // NOLINT

void error(const std::string& message)
{
  std::cerr << message << std::endl;
}

struct arguments_t
{
  const char* output_file = "stdout";
  const char* input_file  = "stdin";

  bool debug       = false;
  bool hex         = false;
  bool relocatable = false;

  std::vector<const char*> args;
};

int parse_opt(int key, const char* arg, Parser* parser)
{
  auto* arguments = static_cast<arguments_t*>(parser->input());

  switch (key)
  {
    case 777:
      arguments->debug = true;
      break;
    case 'h':
      if (arguments->relocatable) error("cannot mix -hex and -relocatable");
      arguments->hex = true;
      break;
    case 'r':
      if (arguments->hex) error("cannot mix -hex and -relocatable");
      arguments->relocatable = true;
      break;
    case 'o':
      arguments->output_file = arg != nullptr ? arg : "stdout";
      break;
    case 'i':
      arguments->input_file = arg;
      break;
    case Key::ARG:
      arguments->args.push_back(arg);
      break;
    case Key::ERROR:
      help(parser, stderr, STD_ERR);
      break;
    default:
      break;
  }

  return 0;
}

// clang-format off
static const option_t options[] = {
    {      nullptr,  'R', nullptr,               0,        "random 0-group option"},
    {      nullptr,    0, nullptr,               0,                 "Program mode", 1},
    {"relocatable", 'r',  nullptr,               0, "Output in relocatable format"},
    {        "hex", 'h',  nullptr,               0,         "Output in hex format"},
    {"hexadecimal",   0,  nullptr,  ALIAS | HIDDEN},
    {      nullptr,   0,  nullptr,               0,               "For developers", 4},
    {      "debug", 777,  nullptr,               0,        "Enable debugging mode"},
    {      nullptr,   0,  nullptr,               0,                 "Input/output", 3},
    {     "output", 'o',   "file",    ARG_OPTIONAL,  "Output file, default stdout"},
    {      nullptr, 'i',   "file",               0,                  "Input  file"},
    {      nullptr,   0,  nullptr,               0,        "Informational Options", -1},
    {},
};

static const arg_t argp = {
	options, parse_opt, "doc string\nother usage",
	"First half of the message\vsecond half of the message"
};
// clang-format on

int main(int argc, char* argv[])
{
  arguments_t arguments;

  if (parse(&argp, argc, argv, 0, &arguments) != 0)
  {
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
  for (const auto& arg : arguments.args) std::cout << arg << " ";
  std::cout << std::endl;

  return 0;
}
