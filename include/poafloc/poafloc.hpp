#ifndef POAFLOC_POAFLOC_HPP
#define POAFLOC_POAFLOC_HPP

#include <array>
#include <cstdarg>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "poafloc.h"

namespace poafloc {

using option_t = poafloc_option_t;
using arg_t    = poafloc_arg_t;

int parse(const arg_t* argp,
          int argc,
          char* argv[],
          unsigned flags,
          void* input) noexcept;

void usage(const Parser* parser);
void help(const Parser* parser, FILE* stream, unsigned flags);

void failure(const Parser* parser,
             int status,
             int errnum,
             const char* fmt,
             va_list args);

void failure(
    const Parser* parser, int status, int errnum, const char* fmt, ...);

struct Parser
{
  Parser(const Parser&)            = delete;
  Parser(Parser&&)                 = delete;
  Parser& operator=(const Parser&) = delete;
  Parser& operator=(Parser&&)      = delete;

  void* input() const { return m_input; }
  unsigned flags() const { return m_flags; }
  const std::string& name() const { return m_name; }

private:
  friend void help(const Parser* parser, FILE* stream, unsigned flags);
  friend int parse(const arg_t* argp,
                   int argc,
                   char** argv,
                   unsigned flags,
                   void* input) noexcept;

  Parser(const arg_t* argp, unsigned flags, void* input);
  ~Parser() noexcept = default;

  int parse(std::size_t argc, char* argv[]);

  int handle_unknown(bool shrt, const char* argv);
  int handle_missing(bool shrt, const char* argv);
  int handle_excess(const char* argv);

  void print_other_usages(FILE* stream) const;
  void help(FILE* stream) const;
  void usage(FILE* stream) const;
  void see(FILE* stream) const;

  static std::string basename(const std::string& name);

  struct help_entry_t
  {
    help_entry_t(const char* arg,
                 const char* message,
                 int group,
                 bool opt = false)
        : m_arg(arg)
        , m_message(message)
        , m_group(group)
        , m_opt(opt)
    {
    }

    void push(char shrt) { m_opt_short.push_back(shrt); }
    void push(const char* lng) { m_opt_long.push_back(lng); }

    bool operator<(const help_entry_t& rhs) const;

    const char* m_arg;
    const char* m_message;
    int m_group;
    bool m_opt;

    std::vector<const char*> m_opt_long;
    std::vector<char> m_opt_short;
  };

  class trie_t
  {
  public:
    trie_t()           = default;
    ~trie_t() noexcept = default;

    trie_t(const trie_t&)            = delete;
    trie_t(trie_t&&)                 = delete;
    trie_t& operator=(const trie_t&) = delete;
    trie_t& operator=(trie_t&&)      = delete;

    bool insert(const std::string& option, int key);
    int get(const std::string& option) const;

  private:
    static bool is_valid(const std::string& option);

    std::array<std::unique_ptr<trie_t>, 26> m_children;
    int m_count     = 0;
    int m_key       = 0;
    bool m_terminal = false;
  };

  const arg_t* m_argp;
  unsigned m_flags;
  void* m_input;

  std::string m_name;

  std::unordered_map<int, const option_t*> m_options;
  std::vector<help_entry_t> m_help_entries;
  trie_t m_trie;
};

}  // namespace poafloc

#endif
