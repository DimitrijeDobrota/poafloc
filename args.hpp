#ifndef ARGS_HPP
#define ARGS_HPP

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <exception>
#include <format>
#include <iostream>
#include <sstream>
#include <unordered_map>
#include <vector>

class Parser {
  public:
    struct option_t {
        const char *name;
        const int key;
        const char *arg;
        const uint8_t options;
        const char *message;
    };

    enum Option {
        ARG_OPTIONAL = 0x1,
        HIDDEN = 0x2,
        ALIAS = 0x4,
    };

    enum Key {
        ARG = 0,
        END = 0x1000001,
        NO_ARGS = 0x1000002,
        INIT = 0x1000003,
        SUCCESS = 0x1000004,
        ERROR = 0x1000005,
    };

    struct argp_t {
        using parse_f = int (*)(int key, const char *arg, void *input);

        const option_t *options;
        const parse_f parser;
        const char *doc;
    };

    Parser(argp_t *argp) : argp(argp) {
        int key_last;
        int i = 0;

        while (true) {
            const auto &opt = argp->options[i];
            if (!opt.name && !opt.key) break;

            if (!opt.key) {
                if ((opt.options & ALIAS) == 0) {
                    std::cerr << "non alias without a key\n";
                    throw new std::runtime_error("no key");
                }

                if (!key_last) {
                    std::cerr << "no option to alias\n";
                    throw new std::runtime_error("no alias");
                }

                trie.insert(opt.name, key_last);
                help_entries.back().push(opt.name);
            } else {
                if (options.count(opt.key)) {
                    std::cerr << std::format("duplicate key {}\n", opt.key);
                    throw new std::runtime_error("duplicate key");
                }

                if (opt.name) trie.insert(opt.name, opt.key);
                options[key_last = opt.key] = &opt;

                bool arg_opt = opt.options & Option::ARG_OPTIONAL;

                if ((opt.options & ALIAS) == 0) {
                    help_entries.emplace_back(opt.arg, opt.message, arg_opt);

                    if (opt.name) help_entries.back().push(opt.name);
                    if (std::isprint(opt.key)) {
                        help_entries.back().push(opt.key);
                    }
                } else {
                    if (!key_last) {
                        std::cerr << "no option to alias\n";
                        throw new std::runtime_error("no alias");
                    }

                    if (opt.name) help_entries.back().push(opt.name);
                    if (std::isprint(opt.key)) {
                        help_entries.back().push(opt.key);
                    }
                }
            }

            i++;
        }

        std::sort(begin(help_entries), end(help_entries));

        help_entries.emplace_back(nullptr, "Give this help list", false);
        help_entries.back().push("help");
        help_entries.back().push('?');

        help_entries.emplace_back(nullptr, "Give a short usage message",
                                  false);
        help_entries.back().push("usage");
    }

    int parse(int argc, char *argv[], void *input) {
        int args = 0, i;

        argp->parser(Key::INIT, 0, input);

        for (i = 1; i < argc; i++) {
            if (argv[i][0] != '-') {
                argp->parser(Key::ARG, argv[i], input);
                args++;
                continue;
            }

            // stop parsing options, rest are normal arguments
            if (!std::strcmp(argv[i], "--")) break;

            if (argv[i][1] != '-') { // short option
                const char *opt = argv[i] + 1;

                // loop over ganged options
                for (int j = 0; opt[j]; j++) {
                    const char key = opt[j];

                    if (key == '?') help(argv[0]);

                    if (!options.count(key)) goto unknown;
                    const auto *option = options[key];

                    const char *arg = nullptr;
                    if (option->arg) {
                        if (opt[j + 1] != 0) {
                            // rest of the line is option argument
                            arg = opt + j + 1;
                        } else if ((option->options & ARG_OPTIONAL) == 0) {
                            // next argv is option argument
                            if (i == argc) goto missing;
                            arg = argv[++i];
                        }
                    }

                    argp->parser(key, arg, input);

                    // if last option required argument we are done
                    if (arg) break;
                }
            } else { // long option
                const char *opt = argv[i] + 2;
                const auto eq = std::strchr(opt, '=');

                std::string opt_s = !eq ? opt : std::string(opt, eq - opt);

                if (opt_s == "help") {
                    if (eq) goto excess;
                    help(argv[0]);
                }

                if (opt_s == "usage") {
                    if (eq) goto excess;
                    usage(argv[0]);
                }

                const int key = trie.get(opt_s);

                if (!key) goto unknown;

                const auto *option = options[key];
                const char *arg = nullptr;

                if (!option->arg && eq) goto excess;
                if (option->arg) {
                    if (eq) {
                        // everything after = is option argument
                        arg = eq + 1;
                    } else if ((option->options & ARG_OPTIONAL) == 0) {
                        // next argv is option argument
                        if (i == argc) goto missing;
                        arg = argv[++i];
                    }
                }

                argp->parser(key, arg, input);
            }
        }

        // parse rest argv as normal arguments
        for (i = i + 1; i < argc; i++) {
            argp->parser(Key::ARG, argv[i], input);
            args++;
        }

        if (!args) argp->parser(Key::NO_ARGS, 0, input);

        argp->parser(Key::END, 0, input);
        argp->parser(Key::SUCCESS, 0, input);

        return 0;

    unknown:
        std::cerr << std::format("unknown option {}\n", argv[i]);
        argp->parser(Key::ERROR, 0, input);
        return 1;

    missing:
        std::cerr << std::format("option {} missing a value\n", argv[i]);
        argp->parser(Key::ERROR, 0, input);
        return 2;

    excess:
        std::cerr << std::format("option {} don't require a value\n", argv[i]);
        argp->parser(Key::ERROR, 0, input);
        return 3;
    }

  private:
    class trie_t {
      public:
        ~trie_t() noexcept {
            for (uint8_t i = 0; i < 26; i++) {
                delete children[i];
            }
        }

        void insert(const std::string &option, int key) {
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

        int get(const std::string &option) const {
            const trie_t *crnt = this;

            for (const char c : option) {
                const uint8_t idx = c - 'a';
                if (!crnt->children[idx]) return 0;
                crnt = crnt->children[idx];
            }

            if (!crnt->terminal && crnt->count > 1) return 0;
            return crnt->key;
        }

      private:
        trie_t *children[26] = {0};
        uint8_t count = 0;
        int key = 0;
        bool terminal = false;
    };

    class help_entry_t {
      public:
        help_entry_t(const char *arg, const char *message, bool opt)
            : m_arg(arg), m_message(message), m_opt(opt) {}

        void push(char sh) { m_opt_short.push_back(sh); }
        void push(const char *lg) { m_opt_long.push_back(lg); }

        const auto arg() const { return m_arg; }
        const auto message() const { return m_message; }
        const auto &opt_short() const { return m_opt_short; }
        const auto &opt_long() const { return m_opt_long; }

        const auto opt() const { return m_opt; }

        bool operator<(const help_entry_t &rhs) const {
            if (m_opt_long.empty() && rhs.m_opt_long.empty())
                return m_opt_short.front() < rhs.m_opt_short.front();

            if (m_opt_long.empty())
                return m_opt_short.front() <= rhs.m_opt_long.front()[0];

            if (rhs.m_opt_long.empty())
                return m_opt_long.front()[0] <= rhs.m_opt_short.front();

            return std::strcmp(m_opt_long.front(), rhs.m_opt_long.front()) < 0;
        }

      private:
        std::vector<char> m_opt_short;
        std::vector<const char *> m_opt_long;

        const char *m_arg;
        const char *m_message;
        bool m_opt;
    };

    void help(const char *name) const {
        std::cout << std::format("Usage: {} [OPTIONS...] {}\n\n", name,
                                 argp->doc ? argp->doc : "");

        for (const auto &entry : help_entries) {
            std::size_t count = 0;
            bool prev = false;

            std::cout << "  ";
            for (const char c : entry.opt_short()) {
                if (!prev) prev = true;
                else
                    std::cout << ", ", count += 2;

                std::string message = std::format("-{}", c);
                if (entry.arg() && entry.opt_long().empty()) {
                    if (entry.opt())
                        message += std::format("[{}]", entry.arg());
                    else
                        message += std::format(" {}", entry.arg());
                }

                std::cout << message;
                count += size(message);
            }

            if (!prev) std::cout << "    ", count += 4;

            for (const auto l : entry.opt_long()) {
                if (!prev) prev = true;
                else
                    std::cout << ", ", count += 2;

                std::string message = std::format("--{}", l);
                if (entry.arg()) {
                    if (entry.opt())
                        message += std::format("[={}]", entry.arg());
                    else
                        message += std::format("={}", entry.arg());
                }

                std::cout << message;
                count += size(message);
            }

            static const std::size_t limit = 30;
            if (count < limit) std::cout << std::string(limit - count, ' ');

            if (entry.message()) {
                std::istringstream iss(entry.message());
                std::size_t count = 0;
                std::string s;

                std::cout << "   ";
                while (iss >> s) {
                    count += size(s);
                    if (count > limit) {
                        std::cout << std::endl << std::string(limit + 5, ' ');
                        count = size(s);
                    }
                    std::cout << s << " ";
                }
            }
            std::cout << std::endl;
        }

        exit(0);
    }

    void usage(const char *name) const {
        static const std::size_t limit = 60;
        static std::size_t count = 0;

        static const auto print = [](const std::string &message) {
            if (count + size(message) > limit) {
                std::cout << "\n      ";
                count = 6;
            }
            std::cout << message;
            count += size(message);
        };

        std::string message = std::format("Usage: {}", name);

        message += " [-";
        for (int i = 0; true; i++) {
            const auto &opt = argp->options[i];
            if (!opt.name && !opt.key) break;
            if (!std::isprint(opt.key)) continue;
            if (opt.arg) continue;

            message += (char)opt.key;
        }
        message += "?]";

        std::cout << message;
        count = size(message);

        for (int i = 0; true; i++) {
            const auto &opt = argp->options[i];
            if (!opt.name && !opt.key) break;
            if (!std::isprint(opt.key)) continue;
            if (!opt.arg) continue;

            if (opt.options & Option::ARG_OPTIONAL) {
                print(std::format(" [-{}[{}]]", (char)opt.key, opt.arg));
            } else {
                print(std::format(" [-{} {}]", (char)opt.key, opt.arg));
            }
        }

        for (int i = 0; true; i++) {
            const auto &opt = argp->options[i];
            if (!opt.name && !opt.key) break;
            if (!opt.name) continue;

            if (!opt.arg) print(std::format(" [--{}]", opt.name));
            else {
                if (opt.options & Option::ARG_OPTIONAL) {
                    print(std::format(" [--{}[={}]]", opt.name, opt.arg));
                } else {
                    print(std::format(" [--{}={}]", opt.name, opt.arg));
                }
            }
        }

        print(" [--help]");
        print(" [--usage] ");
        if (argp->doc) print(argp->doc);

        std::cout << std::endl;

        exit(0);
    }

    const argp_t *argp;

    std::unordered_map<int, const option_t *> options;
    std::vector<help_entry_t> help_entries;
    trie_t trie;
};

#endif
