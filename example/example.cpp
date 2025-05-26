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
      group {
          "standard",
          direct {
              "v value",
              &arguments::val,
          },
          direct {
              "m multiply",
              &arguments::mul,
          },
          direct {
              "n name",
              &arguments::name,
          },
          direct {
              "p priv",
              &arguments::set_priv,
          },
          boolean {
              "f flag1",
              &arguments::flag1,
          },
          boolean {
              "F flag2",
              &arguments::flag2,
          },
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
