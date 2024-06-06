#ifndef ARGS_HPP
#define ARGS_HPP

#include <cstdint>
#include <cstring>
#include <exception>
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

    Parser(const argp_t *argp) : argp(argp) {
        for (int i = 0; argp->options[i].key; i++) {
            const auto &option = argp->options[i];
            const uint8_t idx = option.key - 'a';
            if (options[idx]) {
                std::cerr << std::format("duplicate key {}\n", option.key);
                throw new std::runtime_error("duplicate key");
            }

            options[idx] = &option;
            if (option.name) trie.insert(option.name, option.key);
        }
    }

    int parse(int argc, char *argv[], void *input) {
        const char *arg = nullptr;
        char key;
        int i;

        for (i = 1; i < argc; i++) {
            if (argv[i][0] != '-') {
                argp->parser(-1, argv[i], input);
                continue;
            }

            if (argv[i][1] != '-') {
                const char *opt = argv[i] + 1;
                key = opt[0];

                const auto *option = options[key - 'a'];
                if (!option) goto unknown;

                if (option->arg) {
                    if (i == argc) goto missing;
                    arg = argv[++i];
                }
            } else {
                const char *opt = argv[i] + 2;
                const auto eq = std::strchr(opt, '=');

                key = trie.get(!eq ? opt : std::string(opt, eq - opt));

                if (!key) goto unknown;

                const auto *option = options[key - 'a'];

                if (eq) {
                    if (!option->arg) goto excess;
                    arg = eq + 1;
                } else if (option->arg) {
                    if (i == argc) goto missing;
                    arg = argv[++i];
                }
            }

            argp->parser(key, arg, input);
        }

        return 0;

    unknown:
        std::cerr << std::format("unknown option {}\n", argv[i]);
        return 1;

    missing:
        std::cerr << std::format("option {} missing a value\n", argv[i]);
        return 2;

    excess:
        std::cerr << std::format("option {} don't require a value\n", argv[i]);
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

        void insert(const std::string &option, char key) {
            trie_t *crnt = this;

            for (const char c : option) {
                crnt->count++;
                if (!crnt->terminal) crnt->key = key;

                const uint8_t idx = c - 'a';
                if (!crnt->children[idx]) crnt->children[idx] = new trie_t();
                crnt = crnt->children[idx];
            }

            crnt->terminal = true;
            crnt->key = key;
        }

        char get(const std::string &option) const {
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
        char key = 0;
        bool terminal = false;
    };

    const argp_t *argp;

    const option_t *options[26] = {0};
    trie_t trie;
};

#endif
