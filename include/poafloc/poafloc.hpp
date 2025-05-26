#pragma once

#include <algorithm>
#include <concepts>
#include <cstring>
#include <format>
#include <functional>
#include <initializer_list>
#include <memory>
#include <optional>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include <based/trait/integral_constant.hpp>
#include <based/types/types.hpp>

#include "poafloc/error.hpp"

namespace poafloc
{

namespace detail
{

template<class Record>
class option
{
public:
  enum class type : based::bu8
  {
    argument,
    direct,
    optional,
    boolean,
    list,
  };

private:
  using func_t = std::function<void(Record&, std::string_view)>;

  type m_type;
  std::string m_opts;
  func_t m_func;

protected:
  explicit option(type type, std::string_view opts, func_t func)
      : m_type(type)
      , m_opts(opts)
      , m_func(std::move(func))
  {
  }

  template<class Type, class Member = Type Record::*>
  static auto create(Member member)
  {
    return [member](Record& record, std::string_view value)
    {
      if constexpr (std::is_invocable_v<Member, Record, std::string_view>) {
        std::invoke(member, record, value);
      } else if constexpr (std::is_invocable_v<Member, Record, Type>) {
        std::invoke(member, record, convert<Type>(value));
      } else if constexpr (std::is_assignable_v<Type, std::string_view>) {
        std::invoke(member, record) = value;
      } else {
        std::invoke(member, record) = convert<Type>(value);
      }
    };
  }

  template<class T>
  static T convert(std::string_view value)
  {
    T tmp;
    auto istr = std::istringstream(std::string(value));
    istr >> tmp;
    return tmp;
  }

public:
  using record_type = Record;

  [[nodiscard]] const std::string& opts() const { return m_opts; }
  [[nodiscard]] type type() const { return m_type; }

  void operator()(Record& record, std::string_view value) const
  {
    m_func(record, value);
  }
};

}  // namespace detail

template<class Record, class Type>
  requires(!std::same_as<bool, Type>)
class argument : public detail::option<Record>
{
  using base = detail::option<Record>;
  using member_type = Type Record::*;

public:
  explicit argument(std::string_view name, member_type member)
      : base(base::type::argument, name, base::template create<Type>(member))
  {
  }
};

namespace detail
{

template<class T>
struct is_argument : based::false_type
{
};

template<class Record, class Type>
struct is_argument<argument<Record, Type>> : based::true_type
{
};

template<class T>
concept IsArgument = is_argument<T>::value;

}  // namespace detail

template<class Record>
class positional : public std::vector<detail::option<Record>>
{
  using option_t = detail::option<Record>;
  using base = std::vector<option_t>;

public:
  explicit positional(detail::IsArgument auto... args)
    requires(std::same_as<Record, typename decltype(args)::record_type> && ...)
      : base(std::initializer_list<option_t> {
            based::forward<decltype(args)>(args)...
        })
  {
  }
};

positional(detail::IsArgument auto arg, detail::IsArgument auto... args)
    -> positional<typename decltype(arg)::record_type>;

template<class Record, class Type>
  requires(!std::same_as<bool, Type>)
class direct : public detail::option<Record>
{
  using base = detail::option<Record>;
  using member_type = Type Record::*;

public:
  explicit direct(std::string_view opts, member_type member)
      : base(base::type::direct, opts, base::template create<Type>(member))
  {
  }
};

template<class Record>
class boolean : public detail::option<Record>
{
  using base = detail::option<Record>;
  using member_type = bool Record::*;

  static auto create(member_type member)
  {
    return [member](Record& record, std::string_view value)
    {
      (void)value;
      std::invoke(member, record) = true;
    };
  }

public:
  explicit boolean(std::string_view opts, member_type member)
      : base(base::type::boolean, opts, create(member))
  {
  }
};

namespace detail
{

template<class T>
struct is_option : based::false_type
{
};

template<class Record, class Type>
struct is_option<direct<Record, Type>> : based::true_type
{
};

template<class Record>
struct is_option<boolean<Record>> : based::true_type
{
};

template<class T>
concept IsOption = is_option<T>::value;

}  // namespace detail

template<class Record>
class group : public std::vector<detail::option<Record>>
{
  using option_t = detail::option<Record>;
  using base = std::vector<option_t>;

  std::string m_name;

public:
  using record_type = Record;

  explicit group(std::string_view name, detail::IsOption auto... args)
    requires(std::same_as<Record, typename decltype(args)::record_type> && ...)
      : base(std::initializer_list<option_t> {
            based::forward<decltype(args)>(args)...
        })
      , m_name(name)
  {
  }

  [[nodiscard]] const auto& name() const { return m_name; }
};

group(
    std::string_view name,
    detail::IsOption auto arg,
    detail::IsOption auto... args
) -> group<typename decltype(arg)::record_type>;

namespace detail
{

struct option_lookup_base
{
  using size_t = std::size_t;
  using value_type = std::size_t;
  using optional_type = std::optional<value_type>;

  static constexpr const auto sentinel = value_type {0xFFFFFFFFFFFFFFFF};

  static constexpr bool is_digit(char c) { return c >= '0' && c <= '9'; }
  static constexpr bool is_alpha_lower(char c) { return c >= 'a' && c <= 'z'; }
  static constexpr bool is_alpha_upper(char c) { return c >= 'A' && c <= 'Z'; }

  static constexpr auto convert(char chr)
  {
    return static_cast<size_t>(static_cast<unsigned char>(chr));
  }
};

class option_short : option_lookup_base
{
  static constexpr auto size = static_cast<size_t>(2 * 26);
  std::array<value_type, size> m_opts = {};

  static constexpr auto map(char chr)
  {
    if (is_alpha_lower(chr)) {
      return convert(chr) - convert('a');
    }

    return convert(chr) - convert('A') + 26;  // NOLINT(*magic*)
  }

  [[nodiscard]] bool has(char chr) const
  {
    return m_opts[map(chr)] != sentinel;
  }

public:
  option_short() { m_opts.fill(sentinel); }

  static constexpr bool is_valid(char chr)
  {
    return is_alpha_lower(chr) || is_alpha_upper(chr);
  }

  [[nodiscard]] bool set(char chr, value_type idx)
  {
    if (!is_valid(chr)) {
      throw error<error_code::invalid_option>(chr);
    }

    if (has(chr)) {
      return false;
    }

    m_opts[map(chr)] = idx;
    return true;
  }

  [[nodiscard]] optional_type get(char chr) const
  {
    if (!is_valid(chr)) {
      throw error<error_code::invalid_option>(chr);
    }

    if (!has(chr)) {
      return {};
    }

    return m_opts[map(chr)];
  }
};

class option_long : option_lookup_base
{
  class trie_t
  {
    static constexpr auto size = static_cast<size_t>(26ULL + 10ULL);
    std::array<std::unique_ptr<trie_t>, size> m_children = {};

    value_type m_value = sentinel;
    std::size_t m_count = 0;
    bool m_terminal = false;

    static constexpr auto map(char chr)
    {
      if (is_alpha_lower(chr)) {
        return convert(chr) - convert('a');
      }

      return convert(chr) - convert('0') + 26;  // NOLINT(*magic*)
    }

  public:
    static bool set(trie_t& trie, std::string_view key, value_type value)
    {
      trie_t* crnt = &trie;
      for (const auto c : key) {
        crnt->m_count++;
        if (!crnt->m_terminal) {
          crnt->m_value = value;
        }

        const auto idx = map(c);
        if (crnt->m_children[idx] == nullptr) {
          crnt->m_children[idx] = std::make_unique<trie_t>();
        }
        crnt = crnt->m_children[idx].get();
      }

      if (crnt->m_terminal) {
        return false;
      }

      crnt->m_value = value;
      crnt->m_terminal = true;
      return true;
    }

    static optional_type get(const trie_t& trie, std::string_view key)
    {
      const trie_t* crnt = &trie;

      for (const auto c : key) {
        const auto idx = map(c);
        if (crnt->m_children[idx] == nullptr) {
          return {};
        }
        crnt = crnt->m_children[idx].get();
      }

      if (crnt->m_terminal || crnt->m_count == 1) {
        return crnt->m_value;
      }

      return {};
    }
  };

  trie_t m_trie;

public:
  static constexpr bool is_valid(std::string_view opt)
  {
    return is_alpha_lower(opt.front())
        && std::ranges::all_of(
               opt,
               [](const char chr)
               {
                 return is_alpha_lower(chr) || is_digit(chr);
               }
        );
  }

  [[nodiscard]] bool set(std::string_view opt, value_type idx)
  {
    if (!is_valid(opt)) {
      throw error<error_code::invalid_option>(opt);
    }

    return trie_t::set(m_trie, opt, idx);
  }

  [[nodiscard]] optional_type get(std::string_view opt) const
  {
    if (!is_valid(opt)) {
      throw error<error_code::invalid_option>(opt);
    }

    return trie_t::get(m_trie, opt);
  }
};

}  // namespace detail

template<class Record>
class parser
{
  using option_t = detail::option<Record>;
  std::vector<option_t> m_options;

  using positional_t = positional<Record>;
  positional_t m_positional;

  using group_t = group<Record>;

  detail::option_short m_opt_short;
  detail::option_long m_opt_long;

  static constexpr bool is_option(std::string_view arg)
  {
    return arg.starts_with("-");
  }

  static constexpr bool is_next_option(std::span<std::string_view> args)
  {
    return !args.empty() && is_option(args.front());
  }

  void process(const option_t& option, std::string_view opts)
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

  void process_group(const group_t& group)
  {
    for (const auto& option : group) {
      process(option, option.opts());
    }
  }

  [[nodiscard]] const option_t& get_option(char opt) const
  {
    const auto idx = m_opt_short.get(opt);
    if (!idx.has_value()) {
      throw error<error_code::unknown_option>(opt);
    }
    return m_options[idx.value()];
  }

  [[nodiscard]] const option_t& get_option(std::string_view opt) const
  {
    const auto idx = m_opt_long.get(opt);
    if (!idx.has_value()) {
      throw error<error_code::unknown_option>(opt);
    }
    return m_options[idx.value()];
  }

  [[noreturn]] static void unhandled_positional(std::string_view arg)
  {
    throw std::runtime_error {
        std::format("Unhandled positional arg: {}", arg),
    };
  }

  enum class handle_res : std::uint8_t
  {
    next,
    ok,
  };

  handle_res handle_long_opt(
      Record& record, std::string_view arg, std::span<std::string_view> next
  ) const
  {
    const auto equal = arg.find('=');
    if (equal != std::string::npos) {
      auto opt = arg.substr(0, equal - 1);
      const auto value = arg.substr(equal + 1);

      const auto option = get_option(opt);

      if (option.type() == option_t::type::boolean) {
        throw error<error_code::superfluous_argument>(opt);
      }

      if (!value.empty()) {
        option(record, value);
        return handle_res::ok;
      }

      throw error<error_code::missing_argument>(opt);
    }

    const auto option = get_option(arg);

    if (option.type() == option_t::type::boolean) {
      option(record, "true");
      return handle_res::ok;
    }

    if (!next.empty()) {
      option(record, next.front());
      return handle_res::next;
    }

    throw error<error_code::missing_argument>(arg);
  }

  handle_res handle_short_opts(
      Record& record, std::string_view arg, std::span<std::string_view> next
  ) const
  {
    for (std::size_t opt_idx = 0; opt_idx < std::size(arg); opt_idx++) {
      const auto opt = arg[opt_idx];
      const auto option = get_option(opt);

      if (option.type() == option_t::type::boolean) {
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

public:
  template<class... Groups>
  explicit parser(Groups&&... groups)
    requires(std::same_as<group_t, Groups> && ...)
  {
    m_options.reserve(m_options.size() + (groups.size() + ...));
    (process_group(groups), ...);
  }

  template<class... Groups>
  explicit parser(positional_t positional, Groups&&... groups)
    requires(std::same_as<group_t, Groups> && ...)
      : m_positional(std::move(positional))
  {
    m_options.reserve(m_options.size() + (groups.size() + ...));
    (process_group(groups), ...);
  }

  void operator()(Record& record, int argc, const char** argv)
  {
    std::vector<std::string_view> args(
        argv + 1, argv + argc  // NOLINT(*pointer*)
    );
    operator()(record, args);
  }

  void operator()(Record& record, std::span<std::string_view> args)
  {
    std::size_t arg_idx = 0;
    bool terminal = false;

    for (; arg_idx != std::size(args); ++arg_idx) {
      const auto arg_raw = args[arg_idx];

      if (!is_option(arg_raw)) {
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
          : handle_long_opt(
                record, arg_raw.substr(2), args.subspan(arg_idx + 1)
            );
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

      if (!terminal && is_option(arg)) {
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
};

}  // namespace poafloc
