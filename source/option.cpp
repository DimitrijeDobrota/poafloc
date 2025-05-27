#include <algorithm>

#include "poafloc/error.hpp"
#include "poafloc/poafloc.hpp"

namespace
{

constexpr bool is_digit(char c)
{
  return c >= '0' && c <= '9';
}
constexpr bool is_alpha_lower(char c)
{
  return c >= 'a' && c <= 'z';
}
constexpr bool is_alpha_upper(char c)
{
  return c >= 'A' && c <= 'Z';
}

constexpr auto convert(char chr)
{
  return static_cast<size_t>(static_cast<unsigned char>(chr));
}

constexpr auto map(char chr)
{
  if (is_alpha_lower(chr)) {
    return convert(chr) - convert('a');
  }

  return convert(chr) - convert('A') + 26;  // NOLINT(*magic*)
}

}  // namespace

// option_short
namespace poafloc::detail
{

constexpr bool option_short::is_valid(char chr)
{
  return is_alpha_lower(chr) || is_alpha_upper(chr);
}

[[nodiscard]] bool option_short::has(char chr) const
{
  return m_opts[map(chr)] != sentinel;
}

[[nodiscard]] bool option_short::set(char chr, std::size_t idx)
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

[[nodiscard]] std::optional<std::size_t> option_short::get(char chr) const
{
  if (!is_valid(chr)) {
    throw error<error_code::invalid_option>(chr);
  }

  if (!has(chr)) {
    return {};
  }

  return m_opts[map(chr)];
}

}  // namespace poafloc::detail

// trie_t
namespace poafloc::detail
{

constexpr auto trie_t::map(char chr)
{
  if (is_alpha_lower(chr)) {
    return convert(chr) - convert('a');
  }

  return convert(chr) - convert('0') + 26;  // NOLINT(*magic*)
}

bool trie_t::set(trie_t& trie, std::string_view key, std::size_t value)
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

std::optional<std::size_t> trie_t::get(const trie_t& trie, std::string_view key)
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

}  // namespace poafloc::detail

// option_long
namespace poafloc::detail
{

constexpr bool option_long::is_valid(std::string_view opt)
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

[[nodiscard]] bool option_long::set(std::string_view opt, std::size_t idx)
{
  if (!is_valid(opt)) {
    throw error<error_code::invalid_option>(opt);
  }

  return trie_t::set(m_trie, opt, idx);
}

[[nodiscard]] std::optional<std::size_t> option_long::get(std::string_view opt
) const
{
  if (!is_valid(opt)) {
    throw error<error_code::invalid_option>(opt);
  }

  return trie_t::get(m_trie, opt);
}

}  // namespace poafloc::detail
