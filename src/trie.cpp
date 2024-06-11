#include "args.hpp"

#include <cstdint>

namespace args {

Parser::trie_t::~trie_t() noexcept {
    for (uint8_t i = 0; i < 26; i++) {
        delete children[i];
    }
}

bool Parser::trie_t::insert(const char *option, int key) {
    trie_t *crnt = this;

    if (!is_valid(option)) return false;
    for (; *option; option++) {
        if (!crnt->terminal) crnt->key = key;
        crnt->count++;

        const uint8_t idx = *option - 'a';
        if (!crnt->children[idx]) crnt->children[idx] = new trie_t();
        crnt = crnt->children[idx];
    }

    crnt->terminal = true;
    crnt->key = key;

    return true;
}

int Parser::trie_t::get(const char *option) const {
    const trie_t *crnt = this;

    if (!is_valid(option)) return 0;
    for (; *option; option++) {
        const uint8_t idx = *option - 'a';
        if (!crnt->children[idx]) return 0;
        crnt = crnt->children[idx];
    }

    if (!crnt->terminal && crnt->count > 1) return 0;
    return crnt->key;
}

bool Parser::trie_t::is_valid(const char *option) {
    for (; *option; option++) {
        if (!std::islower(*option)) return false;
    }
    return true;
}

} // namespace args
