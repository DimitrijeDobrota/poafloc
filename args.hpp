#ifndef ARGS_HPP
#define ARGS_HPP

#include <cstdint>
#include <cstring>
#include <exception>
#include <format>
#include <iostream>
#include <unordered_map>

class Parser {
  public:
    struct option_t {
        const char *name;
        const int key;
        const char *arg;
        const uint8_t options;
    };

    enum Option {
        ARG_OPTIONAL = 0x1,
        HIDDEN = 0x2,
        ALIAS = 0x3,
    };

    enum Key {
        ARG = 0,
        END = 0x1000001,
        NO_ARGS = 0x1000002,
        INIT = 0x1000003,
        SUCCESS = 0x1000004,
        ERROR = 0x1000005,
    };

    struct argp_t {
        using parse_f = int (*)(int key, const char *arg, void *input);

        const option_t *options;
        const parse_f parser;
    };

    Parser(const argp_t *argp) : argp(argp) {
        int key_last;
        int i = 0;
        while (true) {
            const auto &option = argp->options[i];
            if (!option.name && !option.key) break;

            if (!option.key) {
                if ((option.options & ALIAS) == 0) {
                    std::cerr << "non alias without a key\n";
                    throw new std::runtime_error("no key");
                }

                if (!key_last) {
                    std::cerr << "no option to alias\n";
                    throw new std::runtime_error("no alias");
                }

                // TODO: connect aliases in --help

                trie.insert(option.name, key_last);
            } else {
                if (options.count(option.key)) {
                    std::cerr << std::format("duplicate key {}\n", option.key);
                    throw new std::runtime_error("duplicate key");
                }

                // TODO: connect aliases in --help

                if (option.name) trie.insert(option.name, option.key);
                options[option.key] = &option;

                key_last = option.key;
            }

            i++;
        }
    }

    int parse(int argc, char *argv[], void *input) {
        int args = 0, i;

        argp->parser(Key::INIT, 0, input);

        for (i = 1; i < argc; i++) {
            if (argv[i][0] != '-') {
                argp->parser(Key::ARG, argv[i], input);
                args++;
                continue;
            }

            // stop parsing options, rest are normal arguments
            if (!std::strcmp(argv[i], "--")) break;

            if (argv[i][1] != '-') { // short option
                const char *opt = argv[i] + 1;

                // loop over ganged options
                for (int j = 0; opt[j]; j++) {
                    const char key = opt[j];

                    if (!options.count(key)) goto unknown;
                    const auto *option = options[key];

                    const char *arg = nullptr;
                    if (option->arg) {
                        if (opt[j + 1] != 0) {
                            // rest of the line is option argument
                            arg = opt + j + 1;
                        } else if ((option->options & ARG_OPTIONAL) == 0) {
                            // next argv is option argument
                            if (i == argc) goto missing;
                            arg = argv[++i];
                        }
                    }

                    argp->parser(key, arg, input);

                    // if last option required argument we are done
                    if (arg) break;
                }
            } else { // long option
                const char *opt = argv[i] + 2;
                const auto eq = std::strchr(opt, '=');

                const int key =
                    trie.get(!eq ? opt : std::string(opt, eq - opt));

                if (!key) goto unknown;

                const auto *option = options[key];
                const char *arg = nullptr;

                if (!option->arg && eq) goto excess;
                if (option->arg) {
                    if (eq) {
                        // everything after = is option argument
                        arg = eq + 1;
                    } else if ((option->options & ARG_OPTIONAL) == 0) {
                        // next argv is option argument
                        if (i == argc) goto missing;
                        arg = argv[++i];
                    }
                }

                argp->parser(key, arg, input);
            }
        }

		// parse rest argv as normal arguments
        for (i = i + 1; i < argc; i++) {
            argp->parser(Key::ARG, argv[i], input);
            args++;
        }

        if (!args) argp->parser(Key::NO_ARGS, 0, input);

        argp->parser(Key::END, 0, input);
        argp->parser(Key::SUCCESS, 0, input);

        return 0;

    unknown:
        std::cerr << std::format("unknown option {}\n", argv[i]);
        argp->parser(Key::ERROR, 0, input);
        return 1;

    missing:
        std::cerr << std::format("option {} missing a value\n", argv[i]);
        argp->parser(Key::ERROR, 0, input);
        return 2;

    excess:
        std::cerr << std::format("option {} don't require a value\n", argv[i]);
        argp->parser(Key::ERROR, 0, input);
        return 3;
    }

  private:
    class trie_t {
      public:
        ~trie_t() noexcept {
            for (uint8_t i = 0; i < 26; i++) {
                delete children[i];
            }
        }

        void insert(const std::string &option, int key) {
            trie_t *crnt = this;

            for (const char c : option) {
                if (!crnt->terminal) crnt->key = key;
                crnt->count++;

                const uint8_t idx = c - 'a';
                if (!crnt->children[idx]) crnt->children[idx] = new trie_t();
                crnt = crnt->children[idx];
            }

            crnt->terminal = true;
            crnt->key = key;
        }

        int get(const std::string &option) const {
            const trie_t *crnt = this;

            for (const char c : option) {
                const uint8_t idx = c - 'a';
                if (!crnt->children[idx]) return 0;
                crnt = crnt->children[idx];
            }

            if (!crnt->terminal && crnt->count > 1) return 0;
            return crnt->key;
        }

      private:
        trie_t *children[26] = {0};
        uint8_t count = 0;
        int key = 0;
        bool terminal = false;
    };

    const argp_t *argp;

    std::unordered_map<int, const option_t *> options;
    trie_t trie;
};

#endif
