#include "args.hpp"

#include <cstdint>

namespace args {

Parser::trie_t::~trie_t() noexcept {
    for (uint8_t i = 0; i < 26; i++) {
        delete children[i];
    }
}

void Parser::trie_t::insert(const std::string &option, int key) {
    trie_t *crnt = this;

    for (const char c : option) {
        if (!crnt->terminal) crnt->key = key;
        crnt->count++;

        const uint8_t idx = c - 'a';
        if (!crnt->children[idx]) crnt->children[idx] = new trie_t();
        crnt = crnt->children[idx];
    }

    crnt->terminal = true;
    crnt->key = key;
}

int Parser::trie_t::get(const std::string &option) const {
    const trie_t *crnt = this;

    for (const char c : option) {
        const uint8_t idx = c - 'a';
        if (!crnt->children[idx]) return 0;
        crnt = crnt->children[idx];
    }

    if (!crnt->terminal && crnt->count > 1) return 0;
    return crnt->key;
}

} // namespace args
