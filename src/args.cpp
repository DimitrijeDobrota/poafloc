#include "args.hpp"

#include <algorithm>
#include <cstring>
#include <format>
#include <iostream>
#include <sstream>

namespace args {

int parse(const argp_t *argp, int argc, char *argv[], unsigned flags,
          void *input) noexcept {
    Parser parser(argp, flags, input);
    return parser.parse(argc, argv);
}

void usage(const Parser *parser) { help(parser, stderr, Help::STD_USAGE); }
void help(const Parser *parser, FILE *stream, unsigned flags) {
    if (!parser || !stream) return;

    if (flags & LONG) parser->help(stream);
    else if (flags & USAGE) parser->usage(stream);
    else if (flags & SEE) parser->see(stream);

    if (parser->flags() & NO_EXIT) return;
    if (flags & EXIT_ERR) exit(2);
    if (flags & EXIT_OK) exit(0);
}

void failure(const Parser *parser, int status, int errnum, const char *fmt,
             std::va_list args) {
    (void)errnum;
    std::fprintf(stderr, "%s: ", parser->name());
    std::vfprintf(stderr, fmt, args);
    std::putc('\n', stderr);
    if (status) exit(status);
}

void failure(const Parser *parser, int status, int errnum, const char *fmt,
             ...) {
    std::va_list args;
    va_start(args, fmt);
    failure(parser, status, errnum, fmt, args);
    va_end(args);
}

Parser::Parser(const argp_t *argp, unsigned flags, void *input)
    : argp(argp), m_flags(flags), m_input(input) {
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
            // non alias without a key, silently ignoring
            if (!(opt.flags & ALIAS)) continue;

            // nothing to alias, silently ignoring
            if (!key_last) continue;

            // option not valid, silently ignoring
            if (!trie.insert(opt.name, key_last)) continue;

            if (hidden) continue;
            if (opt.flags & Option::HIDDEN) continue;

            help_entries.back().push(opt.name);
        } else {
            // duplicate key, silently ignoring
            if (options.count(opt.key)) continue;

            if (opt.name) trie.insert(opt.name, opt.key);
            options[key_last = opt.key] = &opt;

            bool arg_opt = opt.flags & Option::ARG_OPTIONAL;

            if (!(opt.flags & ALIAS)) {
                if ((hidden = opt.flags & Option::HIDDEN)) continue;

                help_entries.emplace_back(opt.arg, opt.message, group,
                                          arg_opt);

                if (opt.name) help_entries.back().push(opt.name);
                if (std::isprint(opt.key)) help_entries.back().push(opt.key);
            } else {
                // nothing to alias, silently ignoring
                if (!key_last) continue;

                if (hidden) continue;
                if (opt.flags & Option::HIDDEN) continue;

                if (opt.name) help_entries.back().push(opt.name);
                if (std::isprint(opt.key)) help_entries.back().push(opt.key);
            }
        }
    }

    if (!(m_flags & NO_HELP)) {
        help_entries.emplace_back(nullptr, "Give this help list", -1);
        help_entries.back().push("help");
        help_entries.back().push('?');

        help_entries.emplace_back(nullptr, "Give a short usage message", -1);
        help_entries.back().push("usage");
    }

    std::sort(begin(help_entries), end(help_entries));
}

int Parser::parse(int argc, char *argv[]) {
    std::vector<const char *> args;
    int arg_cnt = 0, err_code = 0, i;

    const bool is_help = !(m_flags & NO_HELP);
    const bool is_error = !(m_flags & NO_ERRS);

    m_name = basename(argv[0]);

    argp->parse(Key::INIT, 0, this);

    for (i = 1; i < argc; i++) {
        if (argv[i][0] != '-') {
            if (m_flags & IN_ORDER) argp->parse(Key::ARG, argv[i], this);
            else args.push_back(argv[i]);
            arg_cnt++;
            continue;
        }

        // stop parsing options, rest are normal arguments
        if (!std::strcmp(argv[i], "--")) break;

        if (argv[i][1] != '-') { // short option
            const char *opt = argv[i] + 1;

            // loop over ganged options
            for (int j = 0; opt[j]; j++) {
                const char key = opt[j];

                if (is_help && key == '?') {
                    if (is_error) ::args::help(this, stderr, STD_HELP);
                    continue;
                }

                if (!options.count(key)) {
                    err_code = handle_unknown(1, argv[i]);
                    goto error;
                }

                const auto *option = options[key];
                bool is_opt = option->flags & ARG_OPTIONAL;
                if (!option->arg) argp->parse(key, nullptr, this);
                if (opt[j + 1] != 0) {
                    argp->parse(key, opt + j + 1, this);
                    break;
                } else if (!is_opt) argp->parse(key, nullptr, this);
                else if (i + 1 != argc) {
                    argp->parse(key, argv[++i], this);
                    break;
                } else {
                    err_code = handle_missing(1, argv[i]);
                    goto error;
                }
            }
        } else { // long option
            const char *opt = argv[i] + 2;
            const auto is_eq = std::strchr(opt, '=');

            std::string opt_s = !is_eq ? opt : std::string(opt, is_eq - opt);

            if (is_help && opt_s == "help") {
                if (is_eq) {
                    err_code = handle_excess(argv[i]);
                    goto error;
                }
                if (is_error) ::args::help(this, stderr, STD_HELP);
                continue;
            }

            if (is_help && opt_s == "usage") {
                if (is_eq) {
                    err_code = handle_excess(argv[i]);
                    goto error;
                }
                if (is_error) ::args::help(this, stderr, STD_USAGE);
                continue;
            }

            const int key = trie.get(opt_s.data());
            if (!key) {
                err_code = handle_unknown(0, argv[i]);
                goto error;
            }

            const auto *option = options[key];
            if (!option->arg && is_eq) {
                err_code = handle_excess(argv[i]);
                goto error;
            }

            bool is_opt = option->flags & ARG_OPTIONAL;
            if (!option->arg) argp->parse(key, nullptr, this);
            else if (is_eq) argp->parse(key, is_eq + 1, this);
            else if (is_opt) argp->parse(key, nullptr, this);
            else if (i + 1 != argc) argp->parse(key, argv[++i], this);
            else {
                err_code = handle_missing(0, argv[i]);
                goto error;
            }
        }
    }

    // parse previous arguments if IN_ORDER is not set
    for (const auto arg : args) {
        argp->parse(Key::ARG, arg, this);
    }

    // parse rest argv as normal arguments
    for (i = i + 1; i < argc; i++) {
        argp->parse(Key::ARG, argv[i], this);
        arg_cnt++;
    }

    if (!arg_cnt) argp->parse(Key::NO_ARGS, 0, this);

    argp->parse(Key::END, 0, this);
    argp->parse(Key::SUCCESS, 0, this);

    return 0;

error:
    return err_code;
}

int Parser::handle_unknown(bool shrt, const char *argv) {
    if (m_flags & NO_ERRS) return argp->parse(Key::ERROR, 0, this);

    static const char *const unknown_fmt[2] = {
        "unrecognized option '-%s'\n",
        "invalid option -- '%s'\n",
    };

    failure(this, 1, 0, unknown_fmt[shrt], argv + 1);
    see(stderr);

    if (m_flags & NO_EXIT) return 1;
    exit(1);
}

int Parser::handle_missing(bool shrt, const char *argv) {
    if (m_flags & NO_ERRS) return argp->parse(Key::ERROR, 0, this);

    static const char *const missing_fmt[2] = {
        "option '-%s' requires an argument\n",
        "option requires an argument -- '%s'\n",
    };

    failure(this, 2, 0, missing_fmt[shrt], argv + 1);
    see(stderr);

    if (m_flags & NO_EXIT) return 2;
    exit(2);
}

int Parser::handle_excess(const char *argv) {
    if (m_flags & NO_ERRS) return argp->parse(Key::ERROR, 0, this);

    failure(this, 3, 0, "option '%s' doesn't allow an argument\n", argv);
    see(stderr);

    if (m_flags & NO_EXIT) return 3;
    exit(3);
}

const char *Parser::basename(const char *name) {
    const char *name_sh = std::strrchr(name, '/');
    return name_sh ? name_sh + 1 : name;
}

} // namespace args
