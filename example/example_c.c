#include <poafloc/poafloc.h>
#include <stdio.h>

void error(const char* message)
{
  (void)fprintf(stderr, "%s\n", message);
}

typedef struct
{
  const char* output_file;
  const char* input_file;

  int debug;
  int hex;
  int relocatable;
} arguments_t;

int parse_opt(int key, const char* arg, poafloc_parser_t* parser)
{
  arguments_t* arguments = (arguments_t*)poafloc_parser_input(parser);

  switch (key)
  {
    case 777:
      arguments->debug = 1;
      break;
    case 'h':
      if (arguments->relocatable) error("cannot mix -hex and -relocatable");
      arguments->hex = 1;
      break;
    case 'r':
      if (arguments->hex) error("cannot mix -hex and -relocatable");
      arguments->relocatable = 1;
      break;
    case 'o':
      arguments->output_file = arg ? arg : "stdout";
      break;
    case 'i':
      arguments->input_file = arg;
      break;
    // case Parser::Key::ARG: arguments->args.push_back(arg); break;
    case POAFLOC_KEY_ERROR:
      (void)fprintf(stderr, "handled error\n");
      break;
    case POAFLOC_KEY_INIT:
      arguments->input_file  = "stdin";
      arguments->output_file = "stdout";
    default:
      break;
  }

  return 0;
}

// clang-format off
static const poafloc_option_t options[] = {
    {           0,  'R',      0,                        0,        "random 0-group option"},
    {           0,    0,      0,                        0,                 "Program mode", 1},
    {"relocatable", 'r',      0,                        0, "Output in relocatable format"},
    {        "hex", 'h',      0,                        0,         "Output in hex format"},
    {"hexadecimal",   0,      0,  POAFLOC_OPTION_ALIAS | POAFLOC_OPTION_HIDDEN},
    {            0,   0,      0,                        0,               "For developers", 4},
    {      "debug", 777,      0,                        0,        "Enable debugging mode"},
    {            0,   0,      0,                        0,                 "Input/output", 3},
    {     "output", 'o', "file", POAFLOC_OPTION_ARG_OPTIONAL,  "Output file, default stdout"},
    {            0, 'i', "file",                        0,                  "Input  file"},
    {            0,   0,      0,                        0,        "Informational Options", -1},
    {0},
};

static const poafloc_arg_t argp = {
	options, parse_opt, "doc string\nother usage",
	"First half of the message\vsecond half of the message"
};
// clang-format on

int main(int argc, char* argv[])
{
  arguments_t arguments = {0};

  if (poafloc_parse(&argp, argc, argv, 0, &arguments))
  {
    error("There was an error while parsing arguments");
    return 1;
  }

  printf("Command line options:\n");
  printf("\t       input: %s\n", arguments.input_file);
  printf("\t      output: %s\n", arguments.output_file);
  printf("\t         hex: %d\n", arguments.hex);
  printf("\t       debug: %d\n", arguments.debug);
  printf("\t relocatable: %d\n", arguments.relocatable);

  // std::cout << "\t args: ";
  // for (const auto &arg : arguments.args)
  //     std::cout << arg << " ";
  // std::cout << std::endl;

  return 0;
}
