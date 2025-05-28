#include <format>
#include <iostream>

#include "based/algorithms/max.hpp"
#include "poafloc/poafloc.hpp"

namespace poafloc::detail
{

void parser_base::help_long() const
{
  std::cerr << "Usage: program [OPTIONS]";
  for (const auto& pos : m_pos) {
    std::cerr << std::format(" {}", pos.name());
  }
  if (m_pos.is_list()) {
    std::cerr << "...";
  }
  std::cerr << '\n';

  std::size_t idx = 0;
  for (const auto& [end_idx, name] : m_groups) {
    std::cerr << std::format("\n{}:\n", name);
    while (idx < end_idx) {
      const auto& option = m_options[idx++];
      std::string line;

      line += " ";
      for (const auto opt_short : option.opts_short()) {
        line += std::format(" -{},", opt_short);
      }
      for (const auto& opt_long : option.opts_long()) {
        switch (option.get_type()) {
          case option::type::boolean:
            line += std::format(" --{},", opt_long);
            break;
          case option::type::list:
            line += std::format(" --{}={}...,", opt_long, option.name());
            break;
          default:
            line += std::format(" --{}={},", opt_long, option.name());
            break;
        }
      }
      line.pop_back();  // get rid of superfluous ','

      static constexpr const auto zero = std::size_t {0};
      static constexpr const auto mid = std::size_t {30};
      line += std::string(based::max(zero, mid - std::size(line)), ' ');

      std::cerr << line << option.message() << '\n';
    }
  }
}

void parser_base::help_short() const {}

}  // namespace poafloc::detail
