#include <iostream>
#include <string>

#include "poafloc/poafloc.hpp"

using namespace poafloc;  // NOLINT
                          //
int parse_opt(int key, const char* arg, Parser* parser)
{
  std::ignore = parser;

  switch (key)
  {
    case 's':
      std::cout << 's' << std::endl;
      break;
    case 'l':
      std::cout << 'l' << std::endl;
      break;
    case 'a':
      std::cout << "a:" << arg << std::endl;
      break;
    case 'o':
      std::cout << "o:" << (arg != nullptr ? arg : "default") << std::endl;
      break;
    case ARG:
      std::cout << "arg:" << arg << std::endl;
      break;
    case INIT:
      std::cout << "init" << std::endl;
      break;
    case END:
      std::cout << "end" << std::endl;
      break;
    case SUCCESS:
      std::cout << "success" << std::endl;
      break;
    case ERROR:
      std::cout << "error" << std::endl;
      break;
    case NO_ARGS:
      std::cout << "noargs" << std::endl;
      break;
    default:
      break;
  }

  return 0;
}

// clang-format off
static const option_t options[] = {
    {nullptr, 's', nullptr, 0, ""},
    { "long", 'l', nullptr, 0, ""},
    {  "arg", 'a',   "arg", 0, ""},
    {  "opt", 'o',   "arg", ARG_OPTIONAL, ""},
    {},
};
// clang-format on

static const arg_t argp = {options, parse_opt, "", ""};

int main(int argc, char* argv[])
{
  return parse(&argp, argc, argv, 0, nullptr);
}
