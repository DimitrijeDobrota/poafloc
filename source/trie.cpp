#include <algorithm>
#include <cstdint>

#include "poafloc/poafloc.hpp"

namespace poafloc {

bool Parser::trie_t::insert(const std::string& option, int key)
{
  trie_t* crnt = this;

  if (!is_valid(option)) return false;

  for (const char chr : option)
  {
    if (!crnt->m_terminal) crnt->m_key = key;
    crnt->m_count++;

    const size_t idx = static_cast<unsigned>(chr) - 'a';
    if (!crnt->m_children.at(idx))
      crnt->m_children.at(idx) = std::make_unique<trie_t>();

    crnt = crnt->m_children.at(idx).get();
  }

  crnt->m_terminal = true;
  crnt->m_key      = key;

  return true;
}

int Parser::trie_t::get(const std::string& option) const
{
  const trie_t* crnt = this;

  if (!is_valid(option)) return 0;

  for (const char chr : option)
  {
    const size_t idx = static_cast<unsigned>(chr) - 'a';
    if (!crnt->m_children.at(idx)) return 0;

    crnt = crnt->m_children.at(idx).get();
  }

  if (!crnt->m_terminal && crnt->m_count > 1) return 0;
  return crnt->m_key;
}

bool Parser::trie_t::is_valid(const std::string& option)
{
  return std::all_of(
      begin(option), end(option), [](char chr) { return std::islower(chr); });
}

}  // namespace poafloc
