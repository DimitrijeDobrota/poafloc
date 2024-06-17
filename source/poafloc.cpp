#include <algorithm>
#include <cstring>
#include <format>
#include <iostream>
#include <sstream>

#include "poafloc/poafloc.hpp"

namespace poafloc {

int parse(const arg_t* argp,
          int argc,
          char* argv[],
          unsigned flags,
          void* input) noexcept
{
  Parser parser(argp, flags, input);
  return parser.parse(static_cast<std::size_t>(argc), argv);
}

void usage(const Parser* parser)
{
  help(parser, stderr, Help::STD_USAGE);
}

void help(const Parser* parser, FILE* stream, unsigned flags)
{
  if ((parser == nullptr) || (stream == nullptr)) return;

  if ((flags & LONG) != 0U) parser->help(stream);
  else if ((flags & USAGE) != 0U) parser->usage(stream);
  else if ((flags & SEE) != 0U) parser->see(stream);

  if ((parser->flags() & NO_EXIT) != 0U) return;

  if ((flags & EXIT_ERR) != 0U) exit(2);
  if ((flags & EXIT_OK) != 0U) exit(0);
}

void failure(const Parser* parser,
             int status,
             int errnum,
             const char* fmt,
             std::va_list args)
{
  (void)errnum;

  std::cerr << parser->name() << ": ";
  std::ignore = std::vfprintf(stderr, fmt, args);  // NOLINT
  std::cerr << '\n';

  if (status != 0) exit(status);
}

void failure(
    const Parser* parser, int status, int errnum, const char* fmt, ...)
{
  std::va_list args;
  va_start(args, fmt);
  failure(parser, status, errnum, fmt, args);
  va_end(args);
}

Parser::Parser(const arg_t* argp, unsigned flags, void* input)
    : m_argp(argp)
    , m_flags(flags)
    , m_input(input)
{
  int group    = 0;
  int key_last = 0;
  bool hidden  = false;

  for (int i = 0; true; i++)
  {
    const auto& opt = argp->options[i];  // NOLINT

    if ((opt.name == nullptr) && (opt.key == 0) && (opt.message == nullptr))
      break;

    if ((opt.name == nullptr) && (opt.key == 0))
    {
      group = opt.group != 0U ? opt.group : group + 1;
      m_help_entries.emplace_back(nullptr, opt.message, group);
      continue;
    }

    if (opt.key == 0)
    {
      // non alias without a key, silently ignoring
      if ((opt.flags & ALIAS) == 0) continue;

      // nothing to alias, silently ignoring
      if (key_last == 0) continue;

      // option not valid, silently ignoring
      if (!m_trie.insert(opt.name, key_last)) continue;

      if (hidden) continue;
      if ((opt.flags & Option::HIDDEN) != 0U) continue;

      m_help_entries.back().push(opt.name);
    }
    else
    {
      // duplicate key, silently ignoring
      if (!m_options.contains(opt.key)) continue;

      if (opt.name != nullptr) m_trie.insert(opt.name, opt.key);
      m_options[key_last = opt.key] = &opt;

      bool const arg_opt = (opt.flags & Option::ARG_OPTIONAL) != 0U;

      if ((opt.flags & ALIAS) == 0U)
      {
        hidden = (opt.flags & Option::HIDDEN) != 0;
        if (hidden) continue;

        m_help_entries.emplace_back(opt.arg, opt.message, group, arg_opt);

        if (opt.name != nullptr) m_help_entries.back().push(opt.name);

        if (std::isprint(opt.key) != 0U)
        {
          m_help_entries.back().push(static_cast<char>(opt.key & 0xFF));
        }
      }
      else
      {
        // nothing to alias, silently ignoring
        if (key_last == 0) continue;
        if (hidden) continue;
        if ((opt.flags & Option::HIDDEN) != 0U) continue;

        if (opt.name != nullptr) m_help_entries.back().push(opt.name);
        if (std::isprint(opt.key) != 0U)
        {
          m_help_entries.back().push(static_cast<char>(opt.key & 0xFF));
        }
      }
    }
  }

  if ((m_flags & NO_HELP) == 0U)
  {
    m_help_entries.emplace_back(nullptr, "Give this help list", -1);
    m_help_entries.back().push("help");
    m_help_entries.back().push('?');
    m_help_entries.emplace_back(nullptr, "Give a short usage message", -1);
    m_help_entries.back().push("usage");
  }

  std::sort(begin(m_help_entries), end(m_help_entries));
}

int Parser::parse(std::size_t argc, char* argv[])
{
  const std::span args(argv, argv + argc);

  std::vector<const char*> args_free;
  std::size_t idx = 0;
  int arg_cnt     = 0;
  int err_code    = 0;

  const bool is_help  = (m_flags & NO_HELP) == 0U;
  const bool is_error = (m_flags & NO_ERRS) == 0U;

  m_name = basename(args[0]);
  m_argp->parse(Key::INIT, nullptr, this);

  for (idx = 1; idx < argc; idx++)
  {
    if (args[idx][0] != '-')
    {
      if ((m_flags & IN_ORDER) != 0U) m_argp->parse(Key::ARG, args[idx], this);
      else args_free.push_back(args[idx]);

      arg_cnt++;
      continue;
    }

    // stop parsing options, rest are normal arguments
    if (std::strcmp(args[idx], "--") == 0) break;

    if (args[idx][1] != '-')
    {  // short option
      const std::string opt = args[idx] + 1;

      // loop over ganged options
      for (std::size_t j = 0; opt[j] != 0; j++)
      {
        const char key = opt[j];

        if (is_help && key == '?')
        {
          if (is_error) ::poafloc::help(this, stderr, STD_HELP);
          continue;
        }

        if (!m_options.contains(key))
        {
          err_code = handle_unknown(false, args[idx]);
          goto error;
        }

        const auto* option = m_options[key];
        bool const is_opt  = (option->flags & ARG_OPTIONAL) != 0;
        if (option->arg == nullptr)
        {
          m_argp->parse(key, nullptr, this);
        }
        if (opt[j + 1] != 0)
        {
          m_argp->parse(key, opt.substr(j + 1).c_str(), this);
          break;
        }
        if (is_opt) m_argp->parse(key, nullptr, this);
        else if (idx + 1 != argc)
        {
          m_argp->parse(key, args[++idx], this);
          break;
        }
        else
        {
          err_code = handle_missing(true, args[idx]);
          goto error;
        }
      }
    }
    else
    {  // long option
      const std::string tmp = args[idx] + 2;
      const auto pos        = tmp.find_first_of('=');
      const auto opt        = tmp.substr(0, pos);
      const auto arg        = tmp.substr(pos + 1);

      if (is_help && opt == "help")
      {
        if (pos != std::string::npos)
        {
          err_code = handle_excess(args[idx]);
          goto error;
        }

        if (!is_error) continue;
        ::poafloc::help(this, stderr, STD_HELP);
      }

      if (is_help && opt == "usage")
      {
        if (pos != std::string::npos)
        {
          err_code = handle_excess(args[idx]);
          goto error;
        }

        if (!is_error) continue;
        ::poafloc::help(this, stderr, STD_USAGE);
      }

      const int key = m_trie.get(opt);
      if (key == 0)
      {
        err_code = handle_unknown(false, args[idx]);
        goto error;
      }

      const auto* option = m_options[key];
      if (pos != std::string::npos && option->arg == nullptr)
      {
        err_code = handle_excess(args[idx]);
        goto error;
      }

      const bool is_opt = (option->flags & ARG_OPTIONAL) != 0;

      if (option->arg == nullptr)
      {
        m_argp->parse(key, nullptr, this);
        continue;
      }

      if (pos != std::string::npos)
      {
        m_argp->parse(key, arg.c_str(), this);
        continue;
      }

      if (is_opt)
      {
        m_argp->parse(key, nullptr, this);
        continue;
      }

      if (idx + 1 != argc)
      {
        m_argp->parse(key, args[++idx], this);
        continue;
      }

      err_code = handle_missing(false, args[idx]);
      goto error;
    }
  }

  // parse previous arguments if IN_ORDER is not set
  for (const auto* const arg : args_free) m_argp->parse(Key::ARG, arg, this);

  // parse rest argv as normal arguments
  for (idx = idx + 1; idx < argc; idx++)
  {
    m_argp->parse(Key::ARG, args[idx], this);
    arg_cnt++;
  }

  if (arg_cnt == 0) m_argp->parse(Key::NO_ARGS, nullptr, this);

  m_argp->parse(Key::END, nullptr, this);
  m_argp->parse(Key::SUCCESS, nullptr, this);

  return 0;

error:
  return err_code;
}

int Parser::handle_unknown(bool shrt, const char* argv)
{
  if ((m_flags & NO_ERRS) != 0U)
    return m_argp->parse(Key::ERROR, nullptr, this);

  static const char* const unknown_fmt[2] = {
      "unrem_argpized option '-%s'\n",
      "invalid option -- '%s'\n",
  };

  failure(this, 1, 0, unknown_fmt[shrt], argv + 1);  // NOLINT
  see(stderr);

  if ((m_flags & NO_EXIT) != 0U) return 1;
  exit(1);
}

int Parser::handle_missing(bool shrt, const char* argv)
{
  if ((m_flags & NO_ERRS) != 0U)
    return m_argp->parse(Key::ERROR, nullptr, this);

  static const char* const missing_fmt[2] = {
      "option '-%s' requires an argument\n",
      "option requires an argument -- '%s'\n",
  };

  failure(this, 2, 0, missing_fmt[shrt], argv + 1);  // NOLINT
  see(stderr);

  if ((m_flags & NO_EXIT) != 0U) return 2;
  exit(2);
}

int Parser::handle_excess(const char* argv)
{
  if ((m_flags & NO_ERRS) != 0U)
  {
    return m_argp->parse(Key::ERROR, nullptr, this);
  }

  failure(this, 3, 0, "option '%s' doesn't allow an argument\n", argv);
  see(stderr);

  if ((m_flags & NO_EXIT) != 0U) return 3;
  exit(3);
}

std::string Parser::basename(const std::string& name)
{
  return name.substr(name.find_first_of('/') + 1);
}

}  // namespace poafloc
