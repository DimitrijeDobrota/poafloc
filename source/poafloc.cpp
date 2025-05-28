#include <algorithm>

#include "poafloc/poafloc.hpp"

#include "poafloc/error.hpp"

namespace
{

constexpr bool is_option(std::string_view arg)
{
  return arg.starts_with("-");
}

constexpr bool is_next_option(std::span<std::string_view> args)
{
  return args.empty() || is_option(args.front());
}

}  // namespace

namespace poafloc::detail
{

option::option(
    option::type type, std::string_view opts, func_t func, std::string_view help
)
    : m_type(type)
    , m_func(std::move(func))
{
  auto istr = std::istringstream(std::string(opts));
  std::string str;

  while (std::getline(istr, str, ' ')) {
    if (std::size(str) == 1) {
      m_opts_short.emplace_back(str[0]);
    } else {
      m_opts_long.emplace_back(str);
    }
  }

  std::ranges::sort(m_opts_short);
  std::ranges::sort(m_opts_long);

  if (type != option::type::boolean) {
    const auto pos = help.find(' ');
    m_name = help.substr(0, pos);
    m_message = help.substr(pos + 1);
  } else {
    m_message = help;
  }
}

void parser_base::process(const option& option)
{
  for (const auto opt_short : option.opts_short()) {
    if (!m_opt_short.set(opt_short, std::size(m_options))) {
      throw error<error_code::duplicate_option>(opt_short);
    }
  }

  for (const auto& opt_long : option.opts_long()) {
    if (!m_opt_long.set(opt_long, std::size(m_options))) {
      throw error<error_code::duplicate_option>(opt_long);
    }
  }

  m_options.emplace_back(option);
}

void parser_base::operator()(void* record, int argc, const char** argv)
{
  std::vector<std::string_view> args(
      argv + 1, argv + argc  // NOLINT(*pointer*)
  );
  operator()(record, args);
}

void parser_base::operator()(void* record, std::span<std::string_view> args)
{
  std::size_t arg_idx = 0;
  bool is_term = false;

  while (arg_idx != std::size(args)) {
    const auto arg_raw = args[arg_idx];

    if (!::is_option(arg_raw)) {
      break;
    }

    if (std::size(arg_raw) == 1) {
      throw error<error_code::unknown_option>("-");
    }

    if (arg_raw == "--") {
      is_term = true;
      ++arg_idx;
      break;
    }

    const auto next = args.subspan(arg_idx + 1);
    const auto res = arg_raw[1] != '-'
        ? hdl_short_opts(record, arg_raw.substr(1), next)
        : hdl_long_opt(record, arg_raw.substr(2), next);
    arg_idx = std::size(args) - std::size(res);
  }

  std::size_t count = 0;
  while (arg_idx != std::size(args)) {
    const auto arg = args[arg_idx++];
    if (!is_term && arg == "--") {
      throw error<error_code::invalid_terminal>(arg);
    }

    if (!is_term && ::is_option(arg)) {
      throw error<error_code::invalid_positional>(arg);
    }

    if (!m_pos.is_list() && count == std::size(m_pos)) {
      throw error<error_code::superfluous_positional>(std::size(m_pos));
    }

    if (count == std::size(m_pos)) {
      count--;
    }

    m_pos[count++](record, arg);
  }

  if (count < std::size(m_pos)) {
    throw error<error_code::missing_positional>(std::size(m_pos));
  }
}

parser_base::next_t parser_base::hdl_short_opt(
    void* record, char opt, std::string_view rest, next_t next
) const
{
  const auto option = get_option(opt);

  if (!rest.empty()) {
    if (rest.front() != '=') {
      option(record, rest);
      return next;
    }

    const auto value = rest.substr(1);
    if (!value.empty()) {
      option(record, value);
      return next;
    }

    throw error<error_code::missing_argument>(opt);
  }

  if (is_next_option(next)) {
    throw error<error_code::missing_argument>(opt);
  }

  if (option.get_type() != option::type::list) {
    option(record, next.front());
    return next.subspan(1);
  }

  while (!is_next_option(next)) {
    option(record, next.front());
    next = next.subspan(1);
  }

  return next;
}

parser_base::next_t parser_base::hdl_short_opts(
    void* record, std::string_view arg, next_t next
) const
{
  std::size_t opt_idx = 0;
  while (opt_idx < std::size(arg)) {
    const auto opt = arg[opt_idx];
    const auto option = get_option(opt);

    if (option.get_type() != option::type::boolean) {
      break;
    }

    option(record, "true");
    opt_idx++;
  }

  return opt_idx == std::size(arg)
      ? next
      : hdl_short_opt(record, arg[opt_idx], arg.substr(opt_idx + 1), next);
}

parser_base::next_t parser_base::hdl_long_opt(
    void* record, std::string_view arg, next_t next
) const
{
  const auto equal = arg.find('=');
  if (equal != std::string::npos) {
    auto opt = arg.substr(0, equal - 1);
    const auto value = arg.substr(equal + 1);

    const auto option = get_option(opt);

    if (option.get_type() == option::type::boolean) {
      throw error<error_code::superfluous_argument>(opt);
    }

    if (!value.empty()) {
      option(record, value);
      return next;
    }

    throw error<error_code::missing_argument>(opt);
  }

  const auto opt = arg;
  const auto option = get_option(opt);

  if (option.get_type() == option::type::boolean) {
    option(record, "true");
    return next;
  }

  if (is_next_option(next)) {
    throw error<error_code::missing_argument>(opt);
  }

  if (option.get_type() != option::type::list) {
    option(record, next.front());
    return next.subspan(1);
  }

  while (!is_next_option(next)) {
    option(record, next.front());
    next = next.subspan(1);
  }

  return next;
}

[[nodiscard]] const option& parser_base::get_option(char opt) const
{
  const auto idx = m_opt_short.get(opt);
  if (!idx.has_value()) {
    throw error<error_code::unknown_option>(opt);
  }
  return m_options[idx.value()];
}

[[nodiscard]] const option& parser_base::get_option(std::string_view opt) const
{
  const auto idx = m_opt_long.get(opt);
  if (!idx.has_value()) {
    throw error<error_code::unknown_option>(opt);
  }
  return m_options[idx.value()];
}

}  // namespace poafloc::detail
