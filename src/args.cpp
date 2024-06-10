#include "args.hpp"

#include <algorithm>
#include <cstring>
#include <exception>
#include <format>
#include <iostream>
#include <sstream>

namespace args {

Parser::Parser(void *input, argp_t *argp) : input(input), argp(argp) {
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

int Parser::parse(int argc, char *argv[], void *input) {
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

} // namespace args
