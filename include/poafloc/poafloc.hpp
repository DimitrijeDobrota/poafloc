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
  using return_type = std::optional<value_type>;

  static constexpr const std::size_t size = 256;
  static constexpr const auto sentinel = value_type {0xFFFFFFFFFFFFFFFF};

  using container_type = std::array<value_type, size>;

  static auto convert(char chr)
  {
    return static_cast<container_type::size_type>(chr);
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

  [[nodiscard]] return_type get(char chr) const
  {
    if (!has(chr)) {
      return {};
    }

    return m_opts[convert(chr)];
  }
};

class option_long : option_base
{
  class Trie
  {
    std::array<std::unique_ptr<Trie>, size> m_children = {};
    value_type m_value = sentinel;
    std::size_t m_count = 0;
    bool m_terminal = false;

  public:
    static bool set(Trie& trie, std::string_view key, value_type value)
    {
      Trie* crnt = &trie;

      for (const auto c : key) {
        crnt->m_count++;
        if (!crnt->m_terminal) {
          crnt->m_value = value;
        }

        const auto idx = convert(c);
        if (crnt->m_children[idx] == nullptr) {
          crnt->m_children[idx] = std::make_unique<Trie>();
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

    static return_type get(const Trie& trie, std::string_view key)
    {
      const Trie* crnt = &trie;

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

  Trie m_trie;

public:
  [[nodiscard]] bool set(std::string_view opt, value_type idx)
  {
    return Trie::set(m_trie, opt, idx);
  }

  [[nodiscard]] return_type get(std::string_view opt) const
  {
    return Trie::get(m_trie, opt);
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
      if (str.size() < 2 || str[0] != '-') {
        continue;
      }

      if (str[1] != '-') {
        if (str.size() != 2) {
          throw std::runtime_error {std::format(
              "Short option requires one character: {}", str.substr(1)
          )};
        }

        const auto opt = str[1];
        if (!m_opt_short.set(opt, m_options.size())) {
          throw std::runtime_error {
              std::format("Duplicate short option: {}", opt)
          };
        }
      } else {
        const auto opt = str.substr(2);
        if (!m_opt_long.set(opt, m_options.size())) {
          throw std::runtime_error {
              std::format("Duplicate long option: {}", opt)
          };
        }
      }
    }

    m_options.emplace_back(option);
  }

  [[nodiscard]] const option_t& get_option(char chr) const
  {
    const auto idx = m_opt_short.get(chr);
    if (!idx.has_value()) {
      throw std::runtime_error(std::format("Unknown short option: {}", chr));
    }
    return m_options[idx.value()];
  }

  [[nodiscard]] const option_t& get_option(std::string_view str) const
  {
    const auto idx = m_opt_long.get(str);
    if (!idx.has_value()) {
      throw std::runtime_error(std::format("Unknown long option: {}", str));
    }
    return m_options[idx.value()];
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

  auto operator()(Record& record, const char* argc, int argv) const
  {
    return operator()(record, std::span(argc, argv));
  }

  bool operator()(Record& record, const std::span<std::string_view> args) const
  {
    std::size_t arg_idx = 0;

    for (; arg_idx != args.size(); ++arg_idx) {
      const auto arg = args[arg_idx];

      if (arg == "--") {
        break;
      }

      if (arg.size() < 2) {
        return false;
      }

      if (arg[0] != '-') {
        // TODO positional arg
        continue;
      }

      if (arg[1] != '-') {
        // short options
        for (std::size_t opt_idx = 1; opt_idx < arg.size(); opt_idx++) {
          const auto opt = arg[opt_idx];
          const auto option = get_option(opt);

          if (!option.argument()) {
            option(record, "true");
            continue;
          }

          if (opt_idx + 1 == arg.size()) {
            if (arg_idx + 1 == args.size()) {
              throw std::runtime_error {
                  std::format("Missing argument for option: {}", opt)
              };
            }
            option(record, args[++arg_idx]);
            continue;
          }

          option(record, arg.substr(opt_idx + 1));
          break;
        }
      } else {
        // long option
        const auto equal = arg.find('=', 2);

        if (equal == std::string::npos) {
          const auto opt = arg.substr(2);
          const auto option = get_option(opt);

          if (!option.argument()) {
            option(record, "true");
            continue;
          }

          if (arg_idx + 1 == args.size()) {
            throw std::runtime_error {
                std::format("Missing argument for option: {}", opt)
            };
          }

          option(record, args[++arg_idx]);

        } else {
          const auto opt = arg.substr(2, equal - 1);
          const auto option = get_option(opt);
          // TODO
        }
      }
    }

    return true;
  }
};

}  // namespace poafloc
