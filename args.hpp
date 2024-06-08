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
        const uint8_t flags;
        const char *message;
        const int group;
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
        using parse_f = int (*)(int key, const char *arg, Parser *parser);

        const option_t *options;
        const parse_f parse;
        const char *doc;
        const char *message;
    };

    static int parse(argp_t *argp, int argc, char *argv[], void *input) {
        Parser parser(input, argp);
        return parser.parse(argc, argv, &parser);
    }

    void *input;

  private:
    Parser(void *input, argp_t *argp) : input(input), argp(argp) {
        int group = 0, key_last = 0;
        bool hidden = false;

        for (int i = 0; true; i++) {
            const auto &opt = argp->options[i];
            if (!opt.name && !opt.key && !opt.message) break;

            if (!opt.name && !opt.key) {
                group = opt.group ? opt.group : group + 1;
                help_entries.emplace_back(nullptr, opt.message, group);
                continue;
            }

            if (!opt.key) {
                if ((opt.flags & ALIAS) == 0) {
                    std::cerr << "non alias without a key\n";
                    throw new std::runtime_error("no key");
                }

                if (!key_last) {
                    std::cerr << "no option to alias\n";
                    throw new std::runtime_error("no alias");
                }

                trie.insert(opt.name, key_last);

                if (hidden) continue;
                if (opt.flags & Option::HIDDEN) continue;

                help_entries.back().push(opt.name);
            } else {
                if (options.count(opt.key)) {
                    std::cerr << std::format("duplicate key {}\n", opt.key);
                    throw new std::runtime_error("duplicate key");
                }

                if (opt.name) trie.insert(opt.name, opt.key);
                options[key_last = opt.key] = &opt;

                bool arg_opt = opt.flags & Option::ARG_OPTIONAL;

                if ((opt.flags & ALIAS) == 0) {
                    if ((hidden = opt.flags & Option::HIDDEN)) continue;

                    help_entries.emplace_back(opt.arg, opt.message, group,
                                              arg_opt);

                    if (opt.name) help_entries.back().push(opt.name);
                    if (std::isprint(opt.key)) {
                        help_entries.back().push(opt.key);
                    }
                } else {
                    if (!key_last) {
                        std::cerr << "no option to alias\n";
                        throw new std::runtime_error("no alias");
                    }

                    if (hidden) continue;
                    if (opt.flags & Option::HIDDEN) continue;

                    if (opt.name) help_entries.back().push(opt.name);
                    if (std::isprint(opt.key)) {
                        help_entries.back().push(opt.key);
                    }
                }
            }
        }

        help_entries.emplace_back(nullptr, "Give this help list", -1);
        help_entries.back().push("help");
        help_entries.back().push('?');

        help_entries.emplace_back(nullptr, "Give a short usage message", -1);
        help_entries.back().push("usage");

        std::sort(begin(help_entries), end(help_entries));
    }

    int parse(int argc, char *argv[], void *input) {
        int args = 0, i;

        argp->parse(Key::INIT, 0, this);

        for (i = 1; i < argc; i++) {
            if (argv[i][0] != '-') {
                argp->parse(Key::ARG, argv[i], this);
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
                        } else if ((option->flags & ARG_OPTIONAL) == 0) {
                            // next argv is option argument
                            if (i == argc) goto missing;
                            arg = argv[++i];
                        }
                    }

                    argp->parse(key, arg, this);

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
                    } else if ((option->flags & ARG_OPTIONAL) == 0) {
                        // next argv is option argument
                        if (i == argc) goto missing;
                        arg = argv[++i];
                    }
                }

                argp->parse(key, arg, this);
            }
        }

        // parse rest argv as normal arguments
        for (i = i + 1; i < argc; i++) {
            argp->parse(Key::ARG, argv[i], this);
            args++;
        }

        if (!args) argp->parse(Key::NO_ARGS, 0, this);

        argp->parse(Key::END, 0, this);
        argp->parse(Key::SUCCESS, 0, this);

        return 0;

    unknown:
        std::cerr << std::format("unknown option {}\n", argv[i]);
        argp->parse(Key::ERROR, 0, this);
        return 1;

    missing:
        std::cerr << std::format("option {} missing a value\n", argv[i]);
        argp->parse(Key::ERROR, 0, this);
        return 2;

    excess:
        std::cerr << std::format("option {} don't require a value\n", argv[i]);
        argp->parse(Key::ERROR, 0, this);
        return 3;
    }

    struct help_entry_t {
        std::vector<const char *> opt_long;
        std::vector<char> opt_short;

        const char *arg;
        const char *message;
        int group;
        bool opt;

        help_entry_t(const char *arg, const char *message, int group,
                     bool opt = false)
            : arg(arg), message(message), group(group), opt(opt) {}

        void push(char sh) { opt_short.push_back(sh); }
        void push(const char *lg) { opt_long.push_back(lg); }

        bool operator<(const help_entry_t &rhs) const {
            if (group != rhs.group) {
                if (group && rhs.group) {
                    if (group < 0 && rhs.group < 0) return group < rhs.group;
                    if (group < 0 || rhs.group < 0) return rhs.group < 0;
                    return group < rhs.group;
                }

                return !group;
            }

            const char l1 = !opt_long.empty()    ? opt_long.front()[0]
                            : !opt_short.empty() ? opt_short.front()
                                                 : '0';

            const char l2 = !rhs.opt_long.empty()    ? rhs.opt_long.front()[0]
                            : !rhs.opt_short.empty() ? rhs.opt_short.front()
                                                     : '0';

            if (l1 != l2) return l1 < l2;

            return std::strcmp(opt_long.front(), rhs.opt_long.front()) < 0;
        }
    };

    void print_usage(const char *name) const {
        if (argp->doc) {
            std::istringstream iss(argp->doc);
            std::string s;

            std::getline(iss, s, '\n');
            std::cout << " " << s;

            while (std::getline(iss, s, '\n')) {
                std::cout << std::format("\n   or: {} [OPTIONS...] {}", name,
                                         s);
            }
        }
    }

    void help(const char *name) const {
        std::string m1, m2;
        if (argp->message) {
            std::istringstream iss(argp->message);
            std::getline(iss, m1, '\v');
            std::getline(iss, m2, '\v');
        }

        std::cout << std::format("Usage: {} [OPTIONS...]", name);
        print_usage(name);
        if (!m1.empty()) std::cout << "\n" << m1;
        std::cout << "\n\n";

        bool first = true;
        for (const auto &entry : help_entries) {
            bool prev = false;

            if (entry.opt_short.empty() && entry.opt_long.empty()) {
                if (!first) std::cout << "\n";
                if (entry.message) std::cout << " " << entry.message << ":\n";
                continue;
            }

            first = false;

            std::string message = "  ";
            for (const char c : entry.opt_short) {
                if (!prev) prev = true;
                else message += ", ";

                message += std::format("-{}", c);

                if (!entry.arg || !entry.opt_long.empty()) continue;

                if (entry.opt) message += std::format("[{}]", entry.arg);
                else message += std::format(" {}", entry.arg);
            }

            if (!prev) message += "    ";

            for (const auto l : entry.opt_long) {
                if (!prev) prev = true;
                else message += ", ";

                message += std::format("--{}", l);

                if (!entry.arg) continue;

                if (entry.opt) message += std::format("[={}]", entry.arg);
                else message += std::format("={}", entry.arg);
            }

            static const std::size_t limit = 30;
            if (size(message) < limit) {
                message += std::string(limit - size(message), ' ');
            }

            std::cout << message;

            if (entry.message) {
                std::istringstream iss(entry.message);
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

        if (!m2.empty()) std::cout << "\n" << m2 << "\n";

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
        for (const auto &entry : help_entries) {
            if (entry.arg) continue;
            for (const char c : entry.opt_short) {
                message += c;
            }
        }
        message += "]";

        std::cout << message;
        count = size(message);

        for (const auto &entry : help_entries) {
            if (!entry.arg) continue;
            for (const char c : entry.opt_short) {
                if (entry.opt) print(std::format(" [-{}[{}]]", c, entry.arg));
                else print(std::format(" [-{} {}]", c, entry.arg));
            }
        }

        for (const auto &entry : help_entries) {
            for (const char *name : entry.opt_long) {
                if (!entry.arg) {
                    print(std::format(" [--{}]", name));
                    continue;
                }

                if (entry.opt) {
                    print(std::format(" [--{}[={}]]", name, entry.arg));
                } else {
                    print(std::format(" [--{}={}]", name, entry.arg));
                }
            }
        }

        print_usage(name);
        std::cout << std::endl;

        exit(0);
    }

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

    const argp_t *argp;

    std::unordered_map<int, const option_t *> options;
    std::vector<help_entry_t> help_entries;
    trie_t trie;
};

#endif
