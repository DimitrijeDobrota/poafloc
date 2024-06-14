#ifndef POAFLOC_POAFLOC_HPP
#define POAFLOC_POAFLOC_HPP

#include "poafloc.h"

#include <cstdarg>
#include <string>
#include <unordered_map>
#include <vector>

namespace poafloc {

using option_t = poafloc_option_t;
using arg_t = poafloc_arg_t;

int parse(const arg_t *argp, int argc, char *argv[], unsigned flags,
          void *input) noexcept;

void usage(const Parser *parser);
void help(const Parser *parser, FILE *stream, unsigned flags);

void failure(const Parser *parser, int status, int errnum, const char *fmt,
             va_list args);

void failure(const Parser *parser, int status, int errnum, const char *fmt,
             ...);

class Parser {
  public:
    void *input() const { return m_input; }
    const char *name() const { return m_name; }
    unsigned flags() const { return m_flags; }

  private:
    friend int parse(const arg_t *, int, char **, unsigned, void *) noexcept;
    friend void help(const Parser *parser, FILE *stream, unsigned flags);

    Parser(const arg_t *argp, unsigned flags, void *input);
    Parser(const Parser &) = delete;
    Parser(Parser &&) = delete;
    Parser &operator=(const Parser &) = delete;
    Parser &operator=(Parser &&) = delete;
    ~Parser() noexcept = default;

    int parse(int argc, char *argv[]);

    int handle_unknown(bool shrt, const char *argv);
    int handle_missing(bool shrt, const char *argv);
    int handle_excess(const char *argv);

    void print_other_usages(FILE *stream) const;
    void help(FILE *stream) const;
    void usage(FILE *stream) const;
    void see(FILE *stream) const;

    static const char *basename(const char *name);

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
        trie_t() = default;
        trie_t(const trie_t &) = delete;
        trie_t(trie_t &&) = delete;
        trie_t &operator=(const trie_t &) = delete;
        trie_t &operator=(trie_t &&) = delete;
        ~trie_t() noexcept;

        bool insert(const char *option, int key);
        int get(const char *option) const;

      private:
        static bool is_valid(const char *option);

        trie_t *children[26] = {0};
        int count = 0;
        int key = 0;
        bool terminal = false;
    };

    const arg_t *argp;
    unsigned m_flags;
    void *m_input;

    const char *m_name;

    std::unordered_map<int, const option_t *> options;
    std::vector<help_entry_t> help_entries;
    trie_t trie;
};

} // namespace poafloc

#endif
