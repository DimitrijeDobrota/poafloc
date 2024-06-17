#include <cstring>
#include <format>
#include <sstream>

#include "poafloc/poafloc.hpp"

namespace poafloc {

bool Parser::help_entry_t::operator<(const help_entry_t& rhs) const
{
  if (m_group != rhs.m_group)
  {
    if ((m_group != 0) && (rhs.m_group != 0))
    {
      if (m_group < 0 && rhs.m_group < 0) return m_group < rhs.m_group;
      if (m_group < 0 || rhs.m_group < 0) return rhs.m_group < 0;
      return m_group < rhs.m_group;
    }

    return m_group == 0;
  }

  const char ch1 = !m_opt_long.empty() ? m_opt_long.front()[0]
      : !m_opt_short.empty()           ? m_opt_short.front()
                                       : '0';

  const char ch22 = !rhs.m_opt_long.empty() ? rhs.m_opt_long.front()[0]
      : !rhs.m_opt_short.empty()            ? rhs.m_opt_short.front()
                                            : '0';

  if (ch1 != ch22)
  {
    return ch1 < ch22;
  }

  if (!m_opt_long.empty() || !rhs.m_opt_long.empty())
  {
    return !m_opt_long.empty();
  }

  return std::strcmp(m_opt_long.front(), rhs.m_opt_long.front()) < 0;
}

void Parser::print_other_usages(FILE* stream) const
{
  if (m_argp->doc != nullptr)
  {
    std::istringstream iss(m_argp->doc);
    std::string str;

    std::getline(iss, str, '\n');
    std::ignore = std::fprintf(stream, " %s", str.c_str());

    while (std::getline(iss, str, '\n'))
    {
      std::ignore = std::fprintf(
          stream, "\n   or: %s [OPTIONS...] %s", m_name.c_str(), str.c_str());
    }
  }
}

void Parser::help(FILE* stream) const
{
  std::string msg1;
  std::string msg2;

  if (m_argp->message != nullptr)
  {
    std::istringstream iss(m_argp->message);
    std::getline(iss, msg1, '\v');
    std::getline(iss, msg2, '\v');
  }

  std::ignore = std::fprintf(stream, "Usage: %s [OPTIONS...]", m_name.c_str());
  print_other_usages(stream);

  if (!msg1.empty()) std::ignore = std::fprintf(stream, "\n%s", msg1.c_str());
  std::ignore = std::fprintf(stream, "\n\n");

  bool first = true;
  for (const auto& entry : m_help_entries)
  {
    bool prev = false;

    if (entry.m_opt_short.empty() && entry.m_opt_long.empty())
    {
      if (!first) std::ignore = std::putc('\n', stream);
      if (entry.m_message != nullptr)
        std::ignore = std::fprintf(stream, " %s:\n", entry.m_message);

      continue;
    }

    first = false;

    std::string message = "  ";
    for (const char shrt : entry.m_opt_short)
    {
      if (!prev) prev = true;
      else message += ", ";

      message += std::format("-{}", shrt);

      if ((entry.m_arg == nullptr) || !entry.m_opt_long.empty()) continue;

      if (entry.m_opt) message += std::format("[{}]", entry.m_arg);
      else message += std::format(" {}", entry.m_arg);
    }

    if (!prev) message += "    ";

    for (const auto* const lng : entry.m_opt_long)
    {
      if (!prev) prev = true;
      else message += ", ";

      message += std::format("--{}", lng);

      if (entry.m_arg == nullptr) continue;

      if (entry.m_opt) message += std::format("[={}]", entry.m_arg);
      else message += std::format("={}", entry.m_arg);
    }

    static const int limit = 30;
    if (size(message) < limit)
      message += std::string(limit - size(message), ' ');

    std::ignore = std::fprintf(stream, "%s", message.c_str());

    if (entry.m_message != nullptr)
    {
      std::istringstream iss(entry.m_message);
      std::size_t count = 0;
      std::string str;

      std::ignore = std::fprintf(stream, "   ");
      while (iss >> str)
      {
        count += size(str);
        if (count > limit)
        {
          std::ignore = std::fprintf(stream, "\n%*c", limit + 5, ' ');
          count       = size(str);
        }
        std::ignore = std::fprintf(stream, "%s ", str.c_str());
      }
    }
    std::ignore = std::putc('\n', stream);
  }

  if (!msg2.empty())
    std::ignore = std::fprintf(stream, "\n%s\n", msg2.c_str());
}

void Parser::usage(FILE* stream) const
{
  static const std::size_t limit = 60;
  static std::size_t count       = 0;

  const auto print = [&stream](const std::string& message)
  {
    if (count + size(message) > limit)
      count = static_cast<std::size_t>(std::fprintf(stream, "\n      "));

    std::ignore = std::fprintf(stream, "%s", message.c_str());
    count += size(message);
  };

  std::string message = std::format("Usage: {}", m_name);

  message += " [-";
  for (const auto& entry : m_help_entries)
  {
    if (entry.m_arg != nullptr) continue;

    for (const char shrt : entry.m_opt_short) message += shrt;
  }
  message += "]";

  std::ignore = std::fprintf(stream, "%s", message.c_str());
  count       = size(message);

  for (const auto& entry : m_help_entries)
  {
    if (entry.m_arg == nullptr) continue;
    for (const char shrt : entry.m_opt_short)
    {
      if (entry.m_opt) print(std::format(" [-{}[{}]]", shrt, entry.m_arg));
      else print(std::format(" [-{} {}]", shrt, entry.m_arg));
    }
  }

  for (const auto& entry : m_help_entries)
  {
    for (const char* name : entry.m_opt_long)
    {
      if (entry.m_arg == nullptr)
      {
        print(std::format(" [--{}]", name));
        continue;
      }

      if (entry.m_opt) print(std::format(" [--{}[={}]]", name, entry.m_arg));
      else print(std::format(" [--{}={}]", name, entry.m_arg));
    }
  }

  print_other_usages(stream);
  std::ignore = std::putc('\n', stream);
}

void Parser::see(FILE* stream) const
{
  std::ignore =
      std::fprintf(stream,
                   "Try '%s --help' or '%s --usage' for more information\n",
                   m_name.c_str(),
                   m_name.c_str());
}

}  // namespace poafloc
