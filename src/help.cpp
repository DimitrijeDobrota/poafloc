#include "args.hpp"

#include <cstring>
#include <format>
#include <iostream>
#include <sstream>

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

void Parser::print_usage(const char *name) const {
    if (argp->doc) {
        std::istringstream iss(argp->doc);
        std::string s;

        std::getline(iss, s, '\n');
        std::cout << " " << s;

        while (std::getline(iss, s, '\n')) {
            std::cout << std::format("\n   or: {} [OPTIONS...] {}", name, s);
        }
    }
}

void Parser::help(const char *name) const {
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

void Parser::usage(const char *name) const {
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
