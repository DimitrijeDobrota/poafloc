#pragma once

#include <cstring>
#include <functional>
#include <initializer_list>
#include <memory>
#include <optional>
#include <span>
#include <sstream>
#include <string>
#include <string_view>

#include <based/concepts/is/same.hpp>
#include <based/container/array.hpp>
#include <based/container/vector.hpp>
#include <based/trait/integral_constant.hpp>
#include <based/trait/remove/cvref.hpp>
#include <based/types/literals.hpp>
#include <based/types/types.hpp>
#include <based/utility/forward.hpp>
#include <based/utility/move.hpp>

#include "poafloc/error.hpp"

namespace poafloc
{

using namespace based::literals;  // NOLINT(*namespace*)

namespace detail
{

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
  using func_t = std::function<void(void*, std::string_view)>;

  using size_type = based::u64;

  type m_type;
  func_t m_func;

  based::vector<based::character, size_type> m_opts_short;
  based::vector<std::string, size_type> m_opts_long;

  std::string m_name;
  std::string m_message;

protected:
  explicit option(
      type type, std::string_view opts, func_t func, std::string_view help = ""
  );

  template<class Record, class Type, class Member = Type Record::*>
  static auto create(Member member)
  {
    return [member](void* record_raw, std::string_view value)
    {
      auto* record = static_cast<Record*>(record_raw);
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
  [[nodiscard]] const auto& opts_long() const { return m_opts_long; }
  [[nodiscard]] const auto& opts_short() const { return m_opts_short; }
  [[nodiscard]] const std::string& name() const { return m_name; }
  [[nodiscard]] const std::string& message() const { return m_message; }
  [[nodiscard]] type get_type() const { return m_type; }

  void operator()(void* record, std::string_view value) const
  {
    m_func(record, value);
  }
};

template<class T>
using rec_type = typename based::remove_cvref_t<T>::rec_type;

template<class T, class... Rest>
concept SameRec = (based::SameAs<rec_type<T>, rec_type<Rest>> && ...);

}  // namespace detail

template<class Record, class Type>
  requires(!based::SameAs<bool, Type>)
class argument : public detail::option
{
  using base = detail::option;
  using member_type = Type Record::*;

public:
  using rec_type = Record;

  explicit argument(std::string_view name, member_type member)
      : base(
            base::type::argument,
            "",
            base::template create<Record, Type>(member),
            name
        )
  {
  }
};

template<class Record, class Type>
  requires(!based::SameAs<bool, Type>)
class argument_list : public detail::option
{
  using base = detail::option;
  using member_type = Type Record::*;

public:
  using rec_type = Record;

  explicit argument_list(std::string_view name, member_type member)
      : base(
            base::type::list,
            "",
            base::template create<Record, Type>(member),
            name
        )
  {
  }
};

template<class Record, class Type>
  requires(!based::SameAs<bool, Type>)
class direct : public detail::option
{
  using base = detail::option;
  using member_type = Type Record::*;

public:
  using rec_type = Record;

  explicit direct(
      std::string_view opts, member_type member, std::string_view help = ""
  )
      : base(
            base::type::direct,
            opts,
            base::template create<Record, Type>(member),
            help
        )
  {
  }
};

template<class Record>
class boolean : public detail::option
{
  using base = detail::option;
  using member_type = bool Record::*;

  static auto create(member_type member)
  {
    return [member](void* record_raw, std::string_view value)
    {
      (void)value;
      auto* record = static_cast<Record*>(record_raw);
      std::invoke(member, record) = true;
    };
  }

public:
  using rec_type = Record;

  explicit boolean(
      std::string_view opts, member_type member, std::string_view help = ""
  )
      : base(base::type::boolean, opts, create(member), help)
  {
  }
};

template<class Record, class Type>
  requires(!based::SameAs<bool, Type>)
class list : public detail::option
{
  using base = detail::option;
  using member_type = Type Record::*;

public:
  using rec_type = Record;

  explicit list(
      std::string_view opts, member_type member, std::string_view help = ""
  )
      : base(
            base::type::list,
            opts,
            base::template create<Record, Type>(member),
            help
        )
  {
  }
};

namespace detail
{

template<class T>
struct is_positional : based::false_type
{
};

template<class Record, class Type>
struct is_positional<argument_list<Record, Type>> : based::true_type
{
};

template<class Record, class Type>
struct is_positional<argument<Record, Type>> : based::true_type
{
};

template<class T>
concept IsPositional = is_positional<T>::value;

class positional_base : public based::vector<detail::option, based::u64>
{
  using base = based::vector<option, based::u64>;

protected:
  template<detail::IsPositional Arg, detail::IsPositional... Args>
  explicit positional_base(Arg&& arg, Args&&... args)
      : base(std::initializer_list<option> {
            based::forward<Arg>(arg),
            based::forward<Args>(args)...,
        })
  {
    for (size_type i = 0_u; i + 1_u8 < base::size(); i++) {
      if (base::operator[](i).get_type() == option::type::list) {
        throw runtime_error("invalid positional constructor");
      }
    }
  }

public:
  positional_base() = default;

  [[nodiscard]] bool is_list() const
  {
    return !empty() && back().get_type() == option::type::list;
  }
};

}  // namespace detail

template<class Record>
struct positional : detail::positional_base
{
  using rec_type = Record;

  template<detail::IsPositional Arg, detail::IsPositional... Args>
  explicit positional(Arg&& arg, Args&&... args)
    requires detail::SameRec<Arg, Args...>
      : positional_base(based::forward<Arg>(arg), based::forward<Args>(args)...)
  {
  }
};

template<detail::IsPositional Arg, detail::IsPositional... Args>
positional(Arg&& arg, Args&&... args) -> positional<detail::rec_type<Arg>>;

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

template<class Record, class Type>
struct is_option<list<Record, Type>> : based::true_type
{
};

template<class T>
concept IsOption = is_option<T>::value;

class group_base : public based::vector<detail::option, based::u64>
{
  using base = based::vector<option, based::u64>;

  std::string m_name;

protected:
  template<detail::IsOption Opt, detail::IsOption... Opts>
  explicit group_base(std::string_view name, Opt&& opt, Opts&&... opts)
      : base(std::initializer_list<option> {
            based::forward<Opt>(opt),
            based::forward<Opts>(opts)...,
        })
      , m_name(name)
  {
  }

public:
  group_base() = default;

  [[nodiscard]] const auto& name() const { return m_name; }
};

}  // namespace detail

template<class Record>
struct group : detail::group_base
{
  using rec_type = Record;

  template<detail::IsOption Opt, detail::IsOption... Opts>
  explicit group(std::string_view name, Opt&& opt, Opts&&... opts)
    requires detail::SameRec<Opt, Opts...>
      : group_base(
            name, based::forward<Opt>(opt), based::forward<Opts>(opts)...
        )
  {
  }
};

template<detail::IsOption Opt, detail::IsOption... Opts>
group(std::string_view name, Opt&& opt, Opts&&... opts)
    -> group<typename Opt::rec_type>;

namespace detail
{

class option_short
{
  using size_type = based::u8;
  using value_type = based::u64;
  using opt_type = std::optional<value_type>;

  static constexpr auto size = size_type(2_u * 26_u);
  static constexpr const auto sentinel = based::limits<value_type>::max;

  using array_type = based::array<value_type, size_type, size>;
  array_type m_opts = {};

  [[nodiscard]] bool has(based::character chr) const;

public:
  option_short() { m_opts.fill(sentinel); }

  static constexpr bool is_valid(based::character chr);
  [[nodiscard]] bool set(based::character chr, value_type value);
  [[nodiscard]] opt_type get(based::character chr) const;
};

class trie_t
{
  using size_type = based::u8;
  using value_type = based::u64;
  using opt_type = std::optional<value_type>;

  static constexpr auto size = size_type(26_u + 10_u);
  static constexpr const auto sentinel = based::limits<value_type>::max;

  using ptr_type = std::unique_ptr<trie_t>;
  using array_type = based::array<ptr_type, size_type, size>;
  array_type m_children = {};

  value_type m_value = sentinel;
  based::u8 m_count = 0_u8;

  bool m_terminal = false;
  trie_t* m_parent = nullptr;

  static constexpr auto map(based::character chr);

public:
  explicit trie_t(trie_t* parent = nullptr)
      : m_parent(parent)
  {
  }

  static bool set(trie_t& trie, std::string_view key, value_type value);
  static opt_type get(const trie_t& trie, std::string_view key);
};

class option_long
{
  using value_type = based::u64;
  using opt_type = std::optional<value_type>;

  trie_t m_trie;

public:
  static constexpr bool is_valid(std::string_view opt);
  [[nodiscard]] bool set(std::string_view opt, value_type idx);
  [[nodiscard]] opt_type get(std::string_view opt) const;
};

class parser_base
{
  using size_type = based::u64;

  based::vector<option, size_type> m_options;

  using group_type = std::pair<size_type, std::string>;
  based::vector<group_type, size_type> m_groups;

  positional_base m_pos;
  option_short m_opt_short;
  option_long m_opt_long;

  void process(const option& option);

  [[nodiscard]] const option& get_option(based::character opt) const;
  [[nodiscard]] const option& get_option(std::string_view opt) const;

  using next_t = std::span<const std::string_view>;

  next_t hdl_long_opt(void* record, std::string_view arg, next_t next) const;
  next_t hdl_short_opts(void* record, std::string_view arg, next_t next) const;
  next_t hdl_short_opt(
      void* record, based::character opt, std::string_view rest, next_t next
  ) const;

protected:
  template<class... Groups>
  explicit parser_base(Groups&&... groups)
    requires(based::SameAs<group_base, Groups> && ...)
      : parser_base({}, based::forward<group_base>(groups)...)
  {
  }

  template<class... Groups>
  explicit parser_base(positional_base&& positional, Groups&&... groups)
    requires(based::SameAs<group_base, Groups> && ...)
      : m_pos(based::forward<decltype(positional)>(positional))
  {
    m_options.reserve(m_options.size() + (groups.size() + ...));
    m_groups.reserve(size_type::underlying_cast(sizeof...(groups)));

    const auto process = [&](const auto& group)
    {
      for (const auto& option : group) {
        this->process(option);
      }
      m_groups.emplace_back(m_options.size(), group.name());
    };
    (process(groups), ...);
  }

  void operator()(void* record, int argc, const char** argv);
  void operator()(void* record, std::span<const std::string_view> args);

  void help_long() const;
  void help_short() const;
};

}  // namespace detail

template<class Record>
struct parser : detail::parser_base
{
  template<class Group, class... Groups>
  explicit parser(Group&& grp, Groups&&... groups)
    requires(
        based::SameAs<group<Record>, Group>
        && (based::SameAs<group<Record>, Groups> && ...)
    )
      : parser_base(
            based::forward<detail::group_base>(grp),
            based::forward<detail::group_base>(groups)...
        )
  {
  }

  template<class... Groups>
  explicit parser(positional<Record>&& positional, Groups&&... groups)
    requires(based::SameAs<group<Record>, Groups> && ...)
      : parser_base(
            based::move(positional),
            based::forward<detail::group_base>(groups)...
        )
  {
  }

  using parser_base::help_long;
  using parser_base::help_short;

  void operator()(Record& record, int argc, const char** argv)
  {
    parser_base::operator()(&record, argc, argv);
  }

  void operator()(Record& record, std::span<const std::string_view> args)
  {
    parser_base::operator()(&record, args);
  }
};

}  // namespace poafloc
