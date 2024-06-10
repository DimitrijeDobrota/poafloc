#ifndef ARGS_HPP
#define ARGS_HPP

#include "args.h"

#include <string>
#include <unordered_map>
#include <vector>

class Parser {
  public:
    using option_t = args_option_t;
    using argp_t = args_argp_t;

    enum Option {
        ARG_OPTIONAL = ARGS_OPTION_ARG_OPTIONAL,
        HIDDEN = ARGS_OPTION_HIDDEN,
        ALIAS = ARGS_OPTION_ALIAS,
    };

    enum Key {
        ARG = ARGS_KEY_ARG,
        END = ARGS_KEY_END,
        NO_ARGS = ARGS_KEY_NO_ARGS,
        INIT = ARGS_KEY_INIT,
        SUCCESS = ARGS_KEY_SUCCESS,
        ERROR = ARGS_KEY_ERROR,
    };

    static int parse(argp_t *argp, int argc, char *argv[], void *input) {
        Parser parser(input, argp);
        return parser.parse(argc, argv, &parser);
    }

    void *input;

  private:
    Parser(void *input, argp_t *argp);

    int parse(int argc, char *argv[], void *input);

    void print_usage(const char *name) const;
    void help(const char *name) const;
    void usage(const char *name) const;

    struct help_entry_t {
        help_entry_t(const char *arg, const char *message, int group,
                     bool opt = false)
            : arg(arg), message(message), group(group), opt(opt) {}

        void push(char sh) { opt_short.push_back(sh); }
        void push(const char *lg) { opt_long.push_back(lg); }

        bool operator<(const help_entry_t &rhs) const;

        const char *arg;
        const char *message;
        int group;
        bool opt;

        std::vector<const char *> opt_long;
        std::vector<char> opt_short;
    };

    class trie_t {
      public:
        ~trie_t() noexcept;

        void insert(const std::string &option, int key);
        int get(const std::string &option) const;

      private:
        trie_t *children[26] = {0};
        int count = 0;
        int key = 0;
        bool terminal = false;
    };

    const argp_t *argp;

    std::unordered_map<int, const option_t *> options;
    std::vector<help_entry_t> help_entries;
    trie_t trie;
};

#endif
