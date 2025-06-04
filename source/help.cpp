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
      return std::format("{}", opt);
    case option::type::list:
      return std::format("{}={}...", opt, option.name());
    default:
      return std::format("{}={}", opt, option.name());
  }
}

std::string format_option_short(const option& option)
{
  const auto& opt = option.opt_short();
  switch (option.get_type()) {
    case option::type::boolean:
      return std::format("{}", opt);
    case option::type::list:
      return std::format("{} {}...", opt, option.name());
    default:
      return std::format("{} {}", opt, option.name());
  }
}

}  // namespace

void parser_base::help_usage() const
{
  std::cerr << "Usage: program [OPTIONS]";
  for (const auto& pos : m_pos) {
    std::cerr << std::format(" {}", pos.name());
  }
  if (m_pos.is_list()) {
    std::cerr << "...";
  }
  std::cerr << '\n';
}

void parser_base::help_long() const
{
  help_usage();

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
        line += std::format(" --{},", format_option_long(opt));
      }
      line.pop_back();  // get rid of superfluous ','

      static constexpr const auto zero = std::size_t {0};
      static constexpr const auto mid = std::size_t {30};
      line += std::string(based::max(zero, mid - std::size(line)), ' ');

      std::cerr << line << opt.message() << '\n';
    }
  }

  std::cerr << '\n';
}

void parser_base::help_short() const
{
  std::vector<std::string> opts_short;
  std::vector<std::string> opts_long;
  std::string flags;

  for (const auto& opt : m_options) {
    if (opt.has_opt_short()) {
      if (opt.get_type() == option::type::boolean) {
        flags += opt.opt_short().chr();
      } else {
        opts_short.emplace_back(std::format("[-{}]", format_option_short(opt)));
      }
    }

    if (opt.has_opt_long()) {
      opts_long.emplace_back(std::format("[--{}]", format_option_long(opt)));
    }
  }

  std::ranges::sort(opts_short);
  std::ranges::sort(opts_long);
  std::ranges::sort(flags);

  static const std::string_view usage = "Usage: ";
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

  line += "program";
  if (!flags.empty()) {
    line += std::format(" [-{}]", flags);
  }

  for (const auto& opt : opts_short) {
    print(opt);
  }

  for (const auto& opt : opts_long) {
    print(opt);
  }

  for (const auto& pos : m_pos) {
    print(pos.name());
  }

  std::cerr << line;
  if (m_pos.is_list()) {
    std::cerr << "...";
  }
  std::cerr << '\n' << '\n';
}

}  // namespace poafloc::detail
