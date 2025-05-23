#pragma once

#include <algorithm>
#include <concepts>
#include <functional>
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

template<class Record>
class parser
{
  std::vector<option_base<Record>> m_setters;
  std::vector<char> m_opt_short;
  std::vector<std::string> m_opt_long;

  void process(std::string_view opts)
  {
    auto istr = std::istringstream(std::string(opts));
    std::string opt;

    m_opt_short.emplace_back('\0');
    m_opt_long.emplace_back("");
    while (std::getline(istr, opt, ',')) {
      if (opt.size() < 2 || opt[0] != '-') {
        continue;
      }

      if (opt[1] != '-') {
        m_opt_short.back() = opt[1];
      } else {
        m_opt_long.back() = opt.substr(2);
      }
    }
  }

public:
  template<class... Args>
  explicit parser(Args&&... args)
    requires(std::same_as<Record, typename Args::record_type> && ...)
  {
    constexpr auto size = sizeof...(Args);
    m_opt_short.reserve(size);
    m_opt_long.reserve(size);
    (process(args.opts()), ...);

    m_setters.reserve(size);
    (m_setters.emplace_back(std::forward<Args>(args)), ...);
  }

  bool set(char chr, Record& record, std::string_view value) const
  {
    const auto itr =
        std::find(std::begin(m_opt_short), std::end(m_opt_short), chr);

    if (itr == m_opt_short.end()) {
      return false;
    }

    const auto setter =
        std::begin(m_setters) + std::distance(std::begin(m_opt_short), itr);
    (*setter)(record, value);
    return false;
  }

  bool set(std::string_view str, Record& record, std::string_view value) const
  {
    const auto itr =
        std::find(std::begin(m_opt_long), std::end(m_opt_long), str);

    if (itr == m_opt_long.end()) {
      return false;
    }

    const auto setter =
        std::begin(m_setters) + std::distance(std::begin(m_opt_long), itr);
    (*setter)(record, value);
    return false;
  }
};

}  // namespace poafloc
