#include "args.hpp"

#include <cstring>
#include <format>
#include <sstream>

namespace args {

bool Parser::help_entry_t::operator<(const help_entry_t &rhs) const {
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

void Parser::print_other_usages(FILE *stream) const {
    if (argp->doc) {
        std::istringstream iss(argp->doc);
        std::string s;

        std::getline(iss, s, '\n');
        std::fprintf(stream, " %s", s.c_str());

        while (std::getline(iss, s, '\n')) {
            std::fprintf(stream, "\n   or: %s [OPTIONS...] %s", m_name,
                         s.c_str());
        }
    }
}

void Parser::help(FILE *stream) const {
    std::string m1, m2;
    if (argp->message) {
        std::istringstream iss(argp->message);
        std::getline(iss, m1, '\v');
        std::getline(iss, m2, '\v');
    }

    std::fprintf(stream, "Usage: %s [OPTIONS...]", m_name);
    print_other_usages(stream);

    if (!m1.empty()) std::fprintf(stream, "\n%s", m1.c_str());
    std::fprintf(stream, "\n\n");

    bool first = true;
    for (const auto &entry : help_entries) {
        bool prev = false;

        if (entry.opt_short.empty() && entry.opt_long.empty()) {
            if (!first) std::putc('\n', stream);
            if (entry.message) std::fprintf(stream, " %s:\n", entry.message);
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

        static const int limit = 30;
        if (size(message) < limit) {
            message += std::string(limit - size(message), ' ');
        }

        std::fprintf(stream, "%s", message.c_str());

        if (entry.message) {
            std::istringstream iss(entry.message);
            std::size_t count = 0;
            std::string s;

            std::fprintf(stream, "   ");
            while (iss >> s) {
                count += size(s);
                if (count > limit) {
                    std::fprintf(stream, "\n%*c", limit + 5, ' ');
                    count = size(s);
                }
                std::fprintf(stream, "%s ", s.c_str());
            }
        }
        std::putc('\n', stream);
    }

    if (!m2.empty()) std::fprintf(stream, "\n%s\n", m2.c_str());
}

void Parser::usage(FILE *stream) const {
    static const std::size_t limit = 60;
    static std::size_t count = 0;

    static const auto print = [&stream](const std::string &message) {
        if (count + size(message) > limit) {
            std::fprintf(stream, "\n      ");
            count = 6;
        }
        std::fprintf(stream, "%s", message.c_str());
        count += size(message);
    };

    std::string message = std::format("Usage: {}", m_name);

    message += " [-";
    for (const auto &entry : help_entries) {
        if (entry.arg) continue;
        for (const char c : entry.opt_short) {
            message += c;
        }
    }
    message += "]";

    std::fprintf(stream, "%s", message.c_str());
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

    print_other_usages(stream);
    std::putc('\n', stream);
}

void Parser::see(FILE *stream) const {
    std::fprintf(stream,
                 "Try '%s --help' or '%s --usage' for more information\n",
                 m_name, m_name);
}

} // namespace args
