#ifndef ARGS_HPP
#define ARGS_HPP

#include <cstring>
#include <format>
#include <iostream>

class Parser {
  public:
    struct option_t {
        const char *name;
        char key;
        bool arg;
    };

    struct argp_t {
        using parse_f = int (*)(int key, const char *arg, void *input);

        const option_t *options;
        const parse_f parser;
    };

    static int parse(const argp_t *argp, int argc, const char *argv[], void *input) {

        for (int i = 1; i < argc; i++) {
            bool found = false;
            for (int j = 0; argp->options[j].name; j++) {
                const auto &option = argp->options[j];
                const auto n = std::strlen(option.name);
                const char *arg = 0;

                if (std::strncmp(argv[i], option.name, n)) continue;

                if (argv[i][n] == '=') {
                    if (!option.arg) {
                        std::cerr << "option doesn't require a value\n";
                        exit(1);
                    }

                    arg = argv[i] + n + 1;
                } else if (option.arg) {
                    if (i == argc) {
                        std::cerr << "option missing a value\n";
                        exit(1);
                    }

                    arg = argv[++i];
                }

                argp->parser(option.key, arg, input);

                found = true;
                break;
            }

            if (found) continue;

            if (argv[i][0] == '-') {
                std::cerr << std::format("unknown option {}\n", argv[i]);
                return 1;
            }

            argp->parser(-1, argv[i], input);
        }

        return 0;
    }

  private:
};

#endif

