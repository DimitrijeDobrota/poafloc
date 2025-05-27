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
#include <vector>

#include <based/concepts/is/same.hpp>
#include <based/trait/integral_constant.hpp>
#include <based/trait/remove/cvref.hpp>
#include <based/types/types.hpp>
#include <based/utility/forward.hpp>
#include <based/utility/move.hpp>

namespace poafloc
{

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
  [[nodiscard]] const std::string& opts() const { return m_opts; }
  [[nodiscard]] type type() const { return m_type; }

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
            name,
            base::template create<Record, Type>(member)
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

  explicit direct(std::string_view opts, member_type member)
      : base(
            base::type::direct,
            opts,
            base::template create<Record, Type>(member)
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

  explicit boolean(std::string_view opts, member_type member)
      : base(base::type::boolean, opts, create(member))
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

  explicit list(std::string_view opts, member_type member)
      : base(
            base::type::list, opts, base::template create<Record, Type>(member)
        )
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

template<class T>
struct is_list : based::false_type
{
};

template<class Record, class Type>
struct is_list<list<Record, Type>> : based::true_type
{
};

template<class T>
concept IsList = is_list<T>::value;

class positional_base : public std::vector<detail::option>
{
  using base = std::vector<option>;

protected:
  template<detail::IsArgument Arg, detail::IsArgument... Args>
  explicit positional_base(Arg&& arg, Args&&... args)
      : base(std::initializer_list<option> {
            based::forward<Arg>(arg),
            based::forward<Args>(args)...,
        })
  {
  }

  template<detail::IsArgument... Args, detail::IsList List>
  explicit positional_base(Args&&... args, List&& lst)
      : base(std::initializer_list<option> {
            based::forward<Args>(args)...,
            based::forward<List>(lst),
        })
  {
  }

public:
  positional_base() = default;

  [[nodiscard]] bool is_list() const
  {
    return !empty() && back().type() == option::type::list;
  }
};

}  // namespace detail

template<class Record>
struct positional : detail::positional_base
{
  using rec_type = Record;

  template<detail::IsArgument Arg, detail::IsArgument... Args>
  explicit positional(Arg&& arg, Args&&... args)
    requires detail::SameRec<Arg, Args...>
      : positional_base(based::forward<Arg>(arg), based::forward<Args>(args)...)
  {
  }

  template<detail::IsArgument... Args, detail::IsList List>
  explicit positional(Args&&... args, List&& list)
    requires detail::SameRec<List, Args...>
      : positional_base(
            based::forward<Args>(args)..., based::forward<List>(list)
        )
  {
  }
};

template<detail::IsArgument Arg, detail::IsArgument... Args>
positional(Arg&& arg, Args&&... args) -> positional<detail::rec_type<Arg>>;

template<detail::IsArgument... Args, detail::IsList List>
positional(Args&&... args, List&& list) -> positional<detail::rec_type<List>>;

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

class group_base : public std::vector<detail::option>
{
  using base = std::vector<option>;

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
  static constexpr const auto sentinel = std::size_t {0xFFFFFFFFFFFFFFFF};

  static constexpr auto size = static_cast<std::size_t>(2 * 26);
  std::array<std::size_t, size> m_opts = {};

  [[nodiscard]] bool has(char chr) const;

public:
  option_short() { m_opts.fill(sentinel); }

  static constexpr bool is_valid(char chr);
  [[nodiscard]] bool set(char chr, std::size_t idx);
  [[nodiscard]] std::optional<std::size_t> get(char chr) const;
};

class trie_t
{
  static constexpr const auto sentinel = std::size_t {0xFFFFFFFFFFFFFFFF};

  static constexpr auto size = static_cast<std::size_t>(26ULL + 10ULL);
  std::array<std::unique_ptr<trie_t>, size> m_children = {};

  std::size_t m_value = sentinel;
  std::size_t m_count = 0;
  bool m_terminal = false;

  static constexpr auto map(char chr);

public:
  static bool set(trie_t& trie, std::string_view key, std::size_t value);
  static std::optional<std::size_t> get(
      const trie_t& trie, std::string_view key
  );
};

class option_long
{
  trie_t m_trie;

public:
  static constexpr bool is_valid(std::string_view opt);
  [[nodiscard]] bool set(std::string_view opt, std::size_t idx);
  [[nodiscard]] std::optional<std::size_t> get(std::string_view opt) const;
};

class parser_base
{
  std::vector<option> m_options;

  positional_base m_pos;

  detail::option_short m_opt_short;
  detail::option_long m_opt_long;

  void process(const option& option, std::string_view opts);

  [[nodiscard]] const option& get_option(char opt) const;
  [[nodiscard]] const option& get_option(std::string_view opt) const;

  using next_t = std::span<std::string_view>;

  next_t hdl_long_opt(void* record, std::string_view arg, next_t next) const;
  next_t hdl_short_opts(void* record, std::string_view arg, next_t next) const;
  next_t hdl_short_opt(
      void* record, char opt, std::string_view rest, next_t next
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

    const auto process = [&](const auto& group)
    {
      for (const auto& option : group) {
        this->process(option, option.opts());
      }
    };
    (process(groups), ...);
  }

  void operator()(void* record, int argc, const char** argv);
  void operator()(void* record, std::span<std::string_view> args);
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

  void operator()(Record& record, int argc, const char** argv)
  {
    parser_base::operator()(&record, argc, argv);
  }

  void operator()(Record& record, std::span<std::string_view> args)
  {
    parser_base::operator()(&record, args);
  }
};

}  // namespace poafloc
