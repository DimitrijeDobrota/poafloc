#include <algorithm>
#include <format>
#include <iostream>

#include "based/algorithms/max.hpp"
#include "poafloc/poafloc.hpp"

namespace poafloc::detail
{

namespace
{

std::string format_option_long(const option& option)
{
  const auto& opt = option.opt_long();
  switch (option.get_type()) {
    case option::type::boolean:
      return std::format("--{}", opt);
    case option::type::list:
      return std::format("--{}={}...", opt, option.name());
    default:
      return std::format("--{}={}", opt, option.name());
  }
}

std::string format_option_short(const option& option)
{
  const auto& opt = option.opt_short();
  switch (option.get_type()) {
    case option::type::boolean:
      return std::format("-{}", opt);
    case option::type::list:
      return std::format("-{} {}...", opt, option.name());
    default:
      return std::format("-{} {}", opt, option.name());
  }
}

}  // namespace

void parser_base::help_usage(std::string_view program) const
{
  std::cerr << "Usage: ";
  std::cerr << program;
  std::cerr << " [OPTIONS]";
  for (const auto& pos : m_pos) {
    std::cerr << std::format(" {}", pos.name());
  }
  if (m_pos.is_list()) {
    std::cerr << "...";
  }
  std::cerr << '\n';
}

bool parser_base::help_long(std::string_view program) const
{
  help_usage(program);

  auto idx = size_type(0_u);
  for (const auto& [end_idx, name] : m_groups) {
    std::cerr << std::format("\n{}:\n", name);
    while (idx < end_idx) {
      const auto& opt = m_options[idx++];
      std::string line;

      line += " ";
      if (opt.has_opt_short()) {
        line += std::format(" -{},", opt.opt_short());
      } else {
        line += std::string(4, ' ');
      }
      if (opt.has_opt_long()) {
        line += " ";
        line += format_option_long(opt);
      }

      static constexpr const auto zero = std::size_t {0};
      static constexpr const auto mid = std::size_t {30};
      line += std::string(based::max(zero, mid - std::size(line)), ' ');

      std::cerr << line << opt.message() << '\n';
    }
  }

  std::cerr << '\n';
  return true;
}

bool parser_base::help_short(std::string_view program) const
{
  std::vector<std::string> opts_short;
  std::vector<std::string> opts_long;
  std::string flags;

  for (const auto& opt : m_options) {
    if (opt.has_opt_short()) {
      if (opt.get_type() == option::type::boolean) {
        flags += opt.opt_short().chr();
      } else {
        opts_short.emplace_back(format_option_short(opt));
      }
    }

    if (opt.has_opt_long()) {
      opts_long.emplace_back(format_option_long(opt));
    }
  }

  std::ranges::sort(opts_short);
  std::ranges::sort(opts_long);
  std::ranges::sort(flags);

  static const std::string_view usage = "Usage:";
  std::cerr << usage << ' ';

  std::string line;
  const auto print = [&line](std::string_view data)
  {
    static constexpr const auto lim = std::size_t {60};
    if (std::size(line) + std::size(data) > lim) {
      std::cerr << line << '\n';
      line = std::string(std::size(usage), ' ');
    }
    line += " ";
    line += data;
  };

  line += program;
  if (!flags.empty()) {
    print(std::format("[-{}]", flags));
  }

  for (const auto& opt : opts_short) {
    print(std::format("[{}]", opt));
  }

  for (const auto& opt : opts_long) {
    print(std::format("[{}]", opt));
  }

  for (const auto& pos : m_pos) {
    print(pos.name());
  }

  std::cerr << line;
  if (m_pos.is_list()) {
    std::cerr << "...";
  }
  std::cerr << '\n' << '\n';
  return true;
}

}  // namespace poafloc::detail
