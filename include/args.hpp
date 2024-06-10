#ifndef ARGS_HPP
#define ARGS_HPP

#include <string>
#include <unordered_map>
#include <vector>

class Parser {
  public:
    struct option_t {
        const char *name;
        const int key;
        const char *arg;
        const int flags;
        const char *message;
        const int group;
    };

    enum Option {
        ARG_OPTIONAL = 0x1,
        HIDDEN = 0x2,
        ALIAS = 0x4,
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
        using parse_f = int (*)(int key, const char *arg, Parser *parser);

        const option_t *options;
        const parse_f parse;
        const char *doc;
        const char *message;
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
