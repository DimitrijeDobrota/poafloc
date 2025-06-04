#include <algorithm>

#include <based/char/character.hpp>
#include <based/char/is/alpha.hpp>
#include <based/char/is/alpha_lower.hpp>
#include <based/char/is/digit.hpp>
#include <based/char/mapper.hpp>
#include <based/functional/predicate/not_null.hpp>
#include <based/trait/iterator.hpp>

#include "poafloc/error.hpp"
#include "poafloc/poafloc.hpp"

namespace
{

struct short_map
{
  constexpr bool operator()(based::character chr) const
  {
    return based::is_alpha(chr);
  }
};

struct long_map
{
  constexpr bool operator()(based::character chr) const
  {
    return based::is_alpha_lower(chr) || based::is_digit(chr);
  }
};

using short_mapper = based::mapper<short_map>;
using long_mapper = based::mapper<long_map>;

}  // namespace

// option_short
namespace poafloc::detail
{

constexpr bool option_short::is_valid(based::character chr)
{
  return short_mapper::predicate(chr);
}

bool option_short::has(based::character chr) const
{
  return m_opts[short_mapper::map(chr)] != sentinel;
}

bool option_short::set(based::character chr, value_type value)
{
  if (!is_valid(chr)) {
    throw error<error_code::invalid_option>(chr);
  }

  if (has(chr)) {
    return false;
  }

  m_opts[short_mapper::map(chr)] = value;
  return true;
}

option_short::opt_type option_short::get(based::character chr) const
{
  if (!is_valid(chr)) {
    throw error<error_code::invalid_option>(chr);
  }

  if (!has(chr)) {
    return {};
  }

  return m_opts[short_mapper::map(chr)];
}

}  // namespace poafloc::detail

// trie_t
namespace poafloc::detail
{

bool trie_t::set(trie_t& trie, std::string_view key, value_type value)
{
  trie_t* crnt = &trie;
  for (const auto c : key) {
    crnt->m_count++;
    if (!crnt->m_terminal) {
      crnt->m_value = value;
    }

    const auto idx = long_mapper::map(c);
    if (crnt->m_children[idx] == nullptr) {
      crnt->m_children[idx] = std::make_unique<trie_t>(nullptr);
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

trie_t::opt_type trie_t::get(const trie_t& trie, std::string_view key)
{
  const trie_t* crnt = &trie;

  for (const auto c : key) {
    const auto idx = long_mapper::map(c);
    if (crnt->m_children[idx] == nullptr) {
      return {};
    }
    crnt = crnt->m_children[idx].get();
  }

  if (crnt->m_terminal || crnt->m_count == 1_u8) {
    return crnt->m_value;
  }

  return {};
}

}  // namespace poafloc::detail

// option_long
namespace poafloc::detail
{

constexpr bool option_long::is_valid(std::string_view opt)
{
  return based::is_alpha_lower(opt.front())
      && std::ranges::all_of(opt, long_mapper::predicate);
}

bool option_long::set(std::string_view opt, value_type idx)
{
  if (!is_valid(opt)) {
    throw error<error_code::invalid_option>(opt);
  }

  return trie_t::set(m_trie, opt, idx);
}

option_long::opt_type option_long::get(std::string_view opt) const
{
  if (!is_valid(opt)) {
    throw error<error_code::invalid_option>(opt);
  }

  return trie_t::get(m_trie, opt);
}

}  // namespace poafloc::detail
