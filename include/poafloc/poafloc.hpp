#pragma once

#include <algorithm>
#include <cstring>
#include <functional>
#include <iostream>
#include <memory>
#include <optional>
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

protected:
  explicit option_base(func_t func)
      : m_func(std::move(func))
  {
  }

public:
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
      : option_base<Record>(create(member))
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
  using setter_t = option_base<Record>;
  std::vector<setter_t> m_setters;

  detail::option_short m_opt_short;
  detail::option_long m_opt_long;

  void process(const option_base<Record>& opt, std::string_view opts)
  {
    auto istr = std::istringstream(std::string(opts));
    std::string str;

    while (std::getline(istr, str, ' ')) {
      if (str.size() < 2 || str[0] != '-') {
        continue;
      }

      if (str[1] != '-') {
        if (!m_opt_short.set(str[1], m_setters.size())) {
          throw std::runtime_error("Duplicate short option");
        }
      } else {
        if (!m_opt_long.set(str.substr(2), m_setters.size())) {
          throw std::runtime_error("Duplicate long option");
        }
      }
    }

    m_setters.emplace_back(opt);
  }

public:
  template<class... Args>
  explicit parser(Args&&... args)
    requires(std::same_as<Record, typename Args::record_type> && ...)
  {
    constexpr auto size = sizeof...(Args);

    m_setters.reserve(size);
    (process(args, args.opts()), ...);
  }

  void set(char chr, Record& record, std::string_view value) const
  {
    const auto idx = m_opt_short.get(chr);
    if (!idx.has_value()) {
      throw std::runtime_error("Unknown short option");
    }
    m_setters[idx.value()](record, value);
  }

  void set(std::string_view str, Record& record, std::string_view value) const
  {
    const auto idx = m_opt_long.get(str);
    if (!idx.has_value()) {
      throw std::runtime_error("Unknown long option");
    }
    m_setters[idx.value()](record, value);
  }
};

}  // namespace poafloc
