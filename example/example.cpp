#include <iostream>

#include <poafloc/poafloc.hpp>

class arguments
{
  std::string m_priv = "default";

public:
  // NOLINTBEGIN(*non-private*)
  int val = 0;
  double mul = 0;
  std::string name = "default";
  bool flag1 = false;
  bool flag2 = false;
  // NOLINTEND(*non-private*)

  void set_priv(std::string_view value) { m_priv = value; }

  friend std::ostream& operator<<(std::ostream& ost, const arguments& args)
  {
    return ost << args.val << ' ' << args.mul << ' ' << args.name << ' '
               << args.m_priv << ' ' << args.flag1 << ' ' << args.flag2;
  }
};

int main()
{
  using namespace poafloc;  // NOLINT

  auto program = parser<arguments> {
      positional {
          argument {
              "value",
              &arguments::val,
          },
          argument_list {
              "rest",
              &arguments::val,
          },
      },
      group {
          "standard",
          direct {
              "m multiply",
              &arguments::mul,
              "NUM Multiplication constant",
          },
          list {
              "n names",
              &arguments::name,
              "NAME Names of the variables",
          },
      },
      group {
          "test",
          direct {
              "p priv",
              &arguments::set_priv,
              "PRIV Private code",
          },
          boolean {
              "flag1",
              &arguments::flag1,
              "Some flag1",
          },
          boolean {
              "F flag2",
              &arguments::flag2,
              "Some flag2",
          },
      },
  };

  const std::vector<std::string_view> cmd_args {
      "example",
      "-m1.34",
      "--name",
      "Hello there!",
      "-p",
      "General Kenobi!",
      "--flag1",
      "-F",
      "150",
      "40",
      "30",
  };

  arguments args;

  {
    try {
      const std::vector<std::string_view> help {
          "example",
          "-?",
      };
      program(args, help);
    } catch (const error<error_code::help>& err) {
      (void)err;
    }
  }

  {
    try {
      const std::vector<std::string_view> usage {
          "example",
          "--usage",
      };
      program(args, usage);
    } catch (const error<error_code::help>& err) {
      (void)err;
    }
  }

  std::cout << args << '\n';
  program(args, cmd_args);
  std::cout << args << '\n';

  return 0;
}
