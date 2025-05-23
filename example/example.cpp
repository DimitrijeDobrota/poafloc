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
  using poafloc::option;
  using poafloc::parser;

  const auto program = parser<arguments> {
      option {
          "-v --value",
          &arguments::val,
      },
      option {
          "-m --multiply",
          &arguments::mul,
      },
      option {
          "-n --name",
          &arguments::name,
      },
      option {
          "-p --priv",
          &arguments::set_priv,
      },
      option {
          "-f --flag1",
          &arguments::flag1,
      },
      option {
          "-F --flag2",
          &arguments::flag2,
      },
  };

  std::vector<std::string_view> cmd_args {
      "--value=150",
      "-m1.34",
      "--name",
      "Hello there!",
      "-p",
      "General Kenobi!",
      "--flag1",
      "-F",
  };

  arguments args;

  std::cout << args << '\n';
  program(args, cmd_args);
  std::cout << args << '\n';

  return 0;
}
