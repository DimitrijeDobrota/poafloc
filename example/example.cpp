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
  // NOLINTEND(*non-private*)

  void set_priv(std::string_view value) { m_priv = value; }

  friend std::ostream& operator<<(std::ostream& ost, const arguments& args)
  {
    return ost << args.val << ' ' << args.mul << ' ' << args.name << ' '
               << args.m_priv;
  }
};

int main()
{
  using poafloc::option;
  using poafloc::parser;

  const auto program = parser<arguments> {
      option {
          "-v,--value",
          &arguments::val,
      },
      option {
          "-m,--multiply",
          &arguments::mul,
      },
      option {
          "-n,--name",
          &arguments::name,
      },
      option {
          "-p,--priv",
          &arguments::set_priv,
      },
  };

  arguments args;
  std::cout << args << '\n';

  program.set('v', args, "150");
  std::cout << args << '\n';

  program.set('m', args, "1.34");
  std::cout << args << '\n';

  program.set("name", args, "Hello there!");
  std::cout << args << '\n';

  program.set('p', args, "General Kenobi!");
  std::cout << args << '\n';

  return 0;
}
