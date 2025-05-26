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
  return !args.empty() && is_option(args.front());
}

}  // namespace

namespace poafloc::detail
{

void parser_base::process(const option& option, std::string_view opts)
{
  auto istr = std::istringstream(std::string(opts));
  std::string str;

  while (std::getline(istr, str, ' ')) {
    if (std::size(str) == 1) {
      const auto& opt = str[0];
      if (!m_opt_short.set(opt, std::size(m_options))) {
        throw error<error_code::duplicate_option>(opt);
      }
    } else {
      const auto& opt = str;
      if (!m_opt_long.set(opt, std::size(m_options))) {
        throw error<error_code::duplicate_option>(opt);
      }
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
  bool terminal = false;

  for (; arg_idx != std::size(args); ++arg_idx) {
    const auto arg_raw = args[arg_idx];

    if (!::is_option(arg_raw)) {
      break;
    }

    if (arg_raw.size() == 1) {
      throw error<error_code::unknown_option>("-");
    }

    if (arg_raw == "--") {
      terminal = true;
      ++arg_idx;
      break;
    }

    const auto res = arg_raw[1] != '-'
        ? handle_short_opts(
              record, arg_raw.substr(1), args.subspan(arg_idx + 1)
          )
        : handle_long_opt(record, arg_raw.substr(2), args.subspan(arg_idx + 1));
    switch (res) {
      case handle_res::ok:
        break;
      case handle_res::next:
        arg_idx++;
        break;
    }
  }

  std::size_t count = 0;
  for (; arg_idx != std::size(args); ++arg_idx) {
    const auto arg = args[arg_idx];
    if (!terminal && arg == "--") {
      throw error<error_code::invalid_terminal>(arg);
    }

    if (!terminal && ::is_option(arg)) {
      throw error<error_code::invalid_positional>(arg);
    }

    if (count == m_positional.size()) {
      throw error<error_code::superfluous_positional>(m_positional.size());
    }

    m_positional[count++](record, arg);
  }

  if (count != m_positional.size()) {
    throw error<error_code::missing_positional>(m_positional.size());
  }
}

parser_base::handle_res parser_base::handle_short_opts(
    void* record, std::string_view arg, std::span<std::string_view> next
) const
{
  for (std::size_t opt_idx = 0; opt_idx < std::size(arg); opt_idx++) {
    const auto opt = arg[opt_idx];
    const auto option = get_option(opt);

    if (option.type() == option::type::boolean) {
      option(record, "true");
      continue;
    }

    const auto rest = arg.substr(opt_idx + 1);
    if (rest.empty()) {
      if (!next.empty()) {
        option(record, next.front());
        return handle_res::next;
      }

      throw error<error_code::missing_argument>(opt);
    }

    if (rest.front() != '=') {
      option(record, rest);
      return handle_res::ok;
    }

    const auto value = rest.substr(1);
    if (!value.empty()) {
      option(record, value);
      return handle_res::ok;
    }

    throw error<error_code::missing_argument>(opt);
  }

  return handle_res::ok;
}

parser_base::handle_res parser_base::handle_long_opt(
    void* record, std::string_view arg, std::span<std::string_view> next
) const
{
  const auto equal = arg.find('=');
  if (equal != std::string::npos) {
    auto opt = arg.substr(0, equal - 1);
    const auto value = arg.substr(equal + 1);

    const auto option = get_option(opt);

    if (option.type() == option::type::boolean) {
      throw error<error_code::superfluous_argument>(opt);
    }

    if (!value.empty()) {
      option(record, value);
      return handle_res::ok;
    }

    throw error<error_code::missing_argument>(opt);
  }

  const auto option = get_option(arg);

  if (option.type() == option::type::boolean) {
    option(record, "true");
    return handle_res::ok;
  }

  if (!next.empty()) {
    option(record, next.front());
    return handle_res::next;
  }

  throw error<error_code::missing_argument>(arg);
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
