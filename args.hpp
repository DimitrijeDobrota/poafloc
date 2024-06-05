#ifndef ARGS_HPP
#define ARGS_HPP

#include <cstring>
#include <format>
#include <iostream>

class Parser {
  public:
    struct option_t {
        const char *name;
        const char key;
        const char *arg;
    };

    struct argp_t {
        using parse_f = int (*)(int key, const char *arg, void *input);

        const option_t *options;
        const parse_f parser;
    };

    static int parse(argp_t *argp, int argc, char *argv[], void *input) {
        for (int i = 1; i < argc; i++) {
            bool opt_short = false, opt_long = false;

            if (argv[i][0] != '-') {
                argp->parser(-1, argv[i], input);
                continue;
            }

            if (argv[i][1] != '-') opt_short = true;
            else opt_long = true;

            const char *opt = argv[i] + opt_long + 1;

            bool found = false;
            for (int j = 0; argp->options[j].key; j++) {
                const auto &option = argp->options[j];
                const char *arg = 0;

                if (opt_short && opt[0] != option.key) continue;

                if (opt_long) {
                    if(!option.name) continue;

                    const auto n = std::strlen(option.name);
                    if (std::strncmp(argv[i] + 2, option.name, n)) continue;

                    if (opt[n] == '=') {
                        if (!option.arg) {
                            std::cerr << "option doesn't require a value\n";
                            exit(1);
                        }

                        arg = opt + n + 1;
                    }
                }

                if (option.arg && !arg) {
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
        }

        return 0;
    }

  private:
};

#endif
