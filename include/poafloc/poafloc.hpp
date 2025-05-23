#pragma once

#include <algorithm>
#include <concepts>
#include <cstring>
#include <format>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "poafloc/error.hpp"

namespace poafloc
{

template<class Record>
class option_base
{
  using func_t = std::function<void(Record&, std::string_view)>;

  func_t m_func;
  bool m_arg;

protected:
  explicit option_base(func_t func, bool argument)
      : m_func(std::move(func))
      , m_arg(argument)
  {
  }

public:
  [[nodiscard]] bool argument() const { return m_arg; }

  void operator()(Record& record, std::string_view value) const
  {
    m_func(record, value);
  }
};

template<class Record, class Type>
class option : public option_base<Record>
{
  std::string m_opts;

  using member_type = Type Record::*;

  static auto create(member_type member)
  {
    return [member](Record& record, std::string_view value)
    {
      if constexpr (std::is_invocable_v<member_type, Record, std::string_view>)
      {
        std::invoke(member, record, value);
      } else if constexpr (std::same_as<bool, Type>) {
        std::invoke(member, record) = true;
      } else if constexpr (requires(Type tmp, std::string_view val) {
                             tmp = val;
                           })
      {
        std::invoke(member, record) = value;
      } else {
        auto istr = std::istringstream(std::string(value));
        Type tmp;
        istr >> tmp;
        std::invoke(member, record) = tmp;
      }
    };
  }

public:
  using record_type = Record;
  using value_type = Type;

  option(std::string_view opts, member_type member)
      : option_base<Record>(create(member), !std::same_as<bool, Type>)
      , m_opts(opts)
  {
  }

  [[nodiscard]] const std::string& opts() const { return m_opts; }
};

namespace detail
{

struct option_base
{
  using value_type = std::size_t;
  using optional_type = std::optional<value_type>;

  static constexpr const auto sentinel = value_type {0xFFFFFFFFFFFFFFFF};

  static constexpr bool is_valid(char c)
  {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')
        || (c >= '0' && c <= '9');
  }

  static constexpr const auto size_char = 256;
  static constexpr const auto size = []()
  {
    std::size_t count = 0;

    for (std::size_t idx = 0; idx < size_char; ++idx) {
      const char c = static_cast<char>(idx);
      if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')
          || (c >= '0' && c <= '9'))
      {
        count++;
        continue;
      }
    }
    return count;
  }();

  static constexpr const auto mapping = []()
  {
    std::array<std::size_t, size_char> res = {};
    std::size_t count = 0;

    for (std::size_t idx = 0; idx < std::size(res); ++idx) {
      const char c = static_cast<char>(idx);
      if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')
          || (c >= '0' && c <= '9'))
      {
        res[idx] = count++;
        continue;
      }

      res[idx] = sentinel;
    }

    return res;
  }();

  using container_type = std::array<value_type, size>;

  static auto convert(char chr)
  {
    if (!is_valid(chr)) {
      throw error<error_t::invalid_char>(chr);
    }

    return mapping[static_cast<container_type::size_type>(
        static_cast<unsigned char>(chr)
    )];
  }
};

class option_short : option_base
{
  container_type m_opts = {};

  [[nodiscard]] bool has(char chr) const
  {
    return m_opts[convert(chr)] != sentinel;
  }

public:
  option_short() { m_opts.fill(sentinel); }

  [[nodiscard]] bool set(char chr, value_type idx)
  {
    if (has(chr)) {
      return false;
    }

    m_opts[convert(chr)] = idx;
    return true;
  }

  [[nodiscard]] optional_type get(char chr) const
  {
    if (!has(chr)) {
      return {};
    }

    return m_opts[convert(chr)];
  }
};

class option_long : option_base
{
  class trie_t
  {
    std::array<std::unique_ptr<trie_t>, size> m_children = {};
    value_type m_value = sentinel;
    std::size_t m_count = 0;
    bool m_terminal = false;

  public:
    static bool set(trie_t& trie, std::string_view key, value_type value)
    {
      trie_t* crnt = &trie;

      for (const auto c : key) {
        crnt->m_count++;
        if (!crnt->m_terminal) {
          crnt->m_value = value;
        }

        const auto idx = convert(c);
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
        const auto idx = convert(c);
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
  [[nodiscard]] bool set(std::string_view opt, value_type idx)
  {
    return trie_t::set(m_trie, opt, idx);
  }

  [[nodiscard]] optional_type get(std::string_view opt) const
  {
    return trie_t::get(m_trie, opt);
  }
};

}  // namespace detail

template<class Record>
class parser
{
  using option_t = option_base<Record>;
  std::vector<option_t> m_options;

  detail::option_short m_opt_short;
  detail::option_long m_opt_long;

  void process(const option_base<Record>& option, std::string_view opts)
  {
    auto istr = std::istringstream(std::string(opts));
    std::string str;

    while (std::getline(istr, str, ' ')) {
      if (std::size(str) < 2 || str[0] != '-') {
        continue;
      }

      if (str[1] != '-') {
        if (std::size(str) != 2) {
          continue;
        }

        const auto opt = str[1];
        if (!m_opt_short.set(opt, std::size(m_options))) {
          throw error<error_t::duplicate_option>(opt);
        }
      } else {
        const auto opt = str.substr(2);
        if (!m_opt_long.set(opt, std::size(m_options))) {
          throw error<error_t::duplicate_option>(opt);
        }
      }
    }

    m_options.emplace_back(option);
  }

  [[nodiscard]] const option_t& get_option(char opt) const
  {
    const auto idx = m_opt_short.get(opt);
    if (!idx.has_value()) {
      throw error<error_t::unknown_option>(opt);
    }
    return m_options[idx.value()];
  }

  [[nodiscard]] const option_t& get_option(std::string_view opt) const
  {
    const auto idx = m_opt_long.get(opt);
    if (!idx.has_value()) {
      throw error<error_t::unknown_option>(opt);
    }
    return m_options[idx.value()];
  }

  [[noreturn]] static void unhandled_positional(std::string_view arg)
  {
    throw std::runtime_error {
        std::format("Unhandled positional arg: {}", arg),
    };
  }

  enum class short_res : std::uint8_t
  {
    flag,
    rest,
    next,
    missing,
  };

  [[nodiscard]] short_res handle_short(
      Record& record,
      char opt,
      std::string_view rest,
      std::span<std::string_view> next
  ) const
  {
    const auto option = get_option(opt);

    if (!option.argument()) {
      option(record, "true");
      return short_res::flag;
    }

    if (!rest.empty()) {
      option(record, rest);
      return short_res::rest;
    }

    if (!next.empty()) {
      option(record, *std::begin(next));
      return short_res::next;
    }

    return short_res::missing;
  }

  enum class long_res : std::uint8_t
  {
    flag,
    next,
    missing,
  };

  [[nodiscard]] long_res handle_long(
      Record& record, std::string_view opt, std::span<std::string_view> next
  ) const
  {
    const auto option = get_option(opt);

    if (!option.argument()) {
      option(record, "true");
      return long_res::flag;
    }

    if (!next.empty()) {
      option(record, *std::begin(next));
      return long_res::next;
    }

    return long_res::missing;
  }

  void handle_long_equal(
      Record& record, std::string_view mix, std::string_view::size_type equal
  ) const
  {
    const auto opt = mix.substr(0, equal - 1);
    const auto option = get_option(opt);

    if (!option.argument()) {
      throw error<error_t::superfluous_argument>(opt);
    }

    const auto arg = mix.substr(equal + 1);
    if (arg.empty()) {
      throw error<error_t::missing_argument>(opt);
    }

    option(record, arg);
  }

public:
  template<class... Args>
  explicit parser(Args&&... args)
    requires(std::same_as<Record, typename Args::record_type> && ...)
  {
    constexpr auto size = sizeof...(Args);

    m_options.reserve(size);
    (process(args, args.opts()), ...);
  }

  void operator()(Record& record, const char* argc, int argv) const
  {
    operator()(record, std::span(argc, argv));
  }

  void operator()(Record& record, std::span<std::string_view> args) const
  {
    std::size_t arg_idx = 0;

    for (; arg_idx != std::size(args); ++arg_idx) {
      const auto arg_raw = args[arg_idx];

      if (arg_raw.size() < 2) {
        throw error<error_t::unknown_option>(arg_raw);
      }

      if (arg_raw == "--") {
        break;
      }
      if (arg_raw[0] != '-') {
        // TODO positional arg
        unhandled_positional(arg_raw);
        continue;
      }

      if (arg_raw[1] != '-') {
        // short options
        auto arg = arg_raw.substr(1);
        for (std::size_t opt_idx = 0; opt_idx < std::size(arg); opt_idx++) {
          const auto res = handle_short(
              record,
              arg[opt_idx],
              arg.substr(opt_idx + 1),
              args.subspan(arg_idx + 1)
          );

          switch (res) {
            case short_res::flag:
              continue;
            case short_res::rest:
              break;
            case short_res::next:
              arg_idx++;
              break;
            case short_res::missing:
              throw error<error_t::missing_argument>(arg);
              break;
          }

          break;
        }
      } else {
        // long option
        auto arg = arg_raw.substr(2);
        const auto equal = arg.find('=');

        if (equal != std::string::npos) {
          handle_long_equal(record, arg, equal);
          continue;
        }

        const auto res = handle_long(record, arg, args.subspan(arg_idx + 1));
        switch (res) {
          case long_res::flag:
            break;
          case long_res::next:
            arg_idx++;
            break;
          case long_res::missing:
            throw error<error_t::missing_argument>(arg);
            break;
        }
      }
    }

    for (; arg_idx != std::size(args); ++arg_idx) {
      unhandled_positional(args[arg_idx]);
    }
  }
};

}  // namespace poafloc
