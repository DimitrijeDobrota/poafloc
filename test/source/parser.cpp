#define CATCH_CONFIG_RUNTIME_STATIC_REQUIRE

#include <string_view>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "poafloc/poafloc.hpp"

using poafloc::boolean;
using poafloc::direct;
using poafloc::error;
using poafloc::error_code;
using poafloc::parser;

// NOLINTBEGIN(*complexity*)
TEST_CASE("invalid", "[poafloc/parser]")
{
  struct arguments
  {
    bool flag;
    int value;
  };

  SECTION("short number")
  {
    auto construct = []()
    {
      return parser<arguments> {
          boolean {"1", &arguments::flag},
      };
    };
    REQUIRE_THROWS_AS(construct(), error<error_code::invalid_option>);
  }

  SECTION("long upper")
  {
    auto construct = []()
    {
      return parser<arguments> {
          boolean {"FLAG", &arguments::flag},
      };
    };
    REQUIRE_THROWS_AS(construct(), error<error_code::invalid_option>);
  }

  SECTION("long number start")
  {
    auto construct = []()
    {
      return parser<arguments> {
          direct {"1value", &arguments::value},
      };
    };
    REQUIRE_THROWS_AS(construct(), error<error_code::invalid_option>);
  }

  SECTION("short duplicate")
  {
    auto construct = []()
    {
      return parser<arguments> {
          boolean {"f flag", &arguments::flag},
          direct {"f follow", &arguments::value},
      };
    };
    REQUIRE_THROWS_AS(construct(), error<error_code::duplicate_option>);
  }

  SECTION("long duplicate")
  {
    auto construct = []()
    {
      return parser<arguments> {
          boolean {"f flag", &arguments::flag},
          direct {"v flag", &arguments::value},
      };
    };
    REQUIRE_THROWS_AS(construct(), error<error_code::duplicate_option>);
  }
}

TEST_CASE("flag", "[poafloc/parser]")
{
  struct arguments
  {
    bool flag = false;
  } args;

  auto program = parser<arguments> {
      boolean {"f flag", &arguments::flag},
  };

  SECTION("short")
  {
    std::vector<std::string_view> cmdline = {"-f"};
    REQUIRE_NOTHROW(program(args, cmdline));
    REQUIRE(args.flag == true);
  }

  SECTION("long")
  {
    std::vector<std::string_view> cmdline = {"--flag"};
    REQUIRE_NOTHROW(program(args, cmdline));
    REQUIRE(args.flag == true);
  }

  SECTION("long partial")
  {
    std::vector<std::string_view> cmdline = {"--fl"};
    REQUIRE_NOTHROW(program(args, cmdline));
    REQUIRE(args.flag == true);
  }

  SECTION("short unknown")
  {
    std::vector<std::string_view> cmdline = {"-u"};
    REQUIRE_THROWS_AS(program(args, cmdline), error<error_code::unknown_option>);
    REQUIRE(args.flag == false);
  }

  SECTION("long unknown")
  {
    std::vector<std::string_view> cmdline = {"--unknown"};
    REQUIRE_THROWS_AS(program(args, cmdline), error<error_code::unknown_option>);
    REQUIRE(args.flag == false);
  }

  SECTION("long superfluous")
  {
    std::vector<std::string_view> cmdline = {"--fl=something"};
    REQUIRE_THROWS_AS(program(args, cmdline), error<error_code::superfluous_argument>);
    REQUIRE(args.flag == false);
  }

  SECTION("long superfluous missing")
  {
    std::vector<std::string_view> cmdline = {"--fl="};
    REQUIRE_THROWS_AS(program(args, cmdline), error<error_code::superfluous_argument>);
    REQUIRE(args.flag == false);
  }
}

TEST_CASE("option string", "[poafloc/parser]")
{
  struct arguments
  {
    std::string name = "default";
  } args;

  auto program = parser<arguments> {
      direct {"n name", &arguments::name},
  };

  SECTION("short")
  {
    std::vector<std::string_view> cmdline = {"-n", "something"};
    REQUIRE_NOTHROW(program(args, cmdline));
    REQUIRE(args.name == "something");
  }

  SECTION("short equal")
  {
    std::vector<std::string_view> cmdline = {"-n=something"};
    REQUIRE_NOTHROW(program(args, cmdline));
    REQUIRE(args.name == "something");
  }

  SECTION("short together")
  {
    std::vector<std::string_view> cmdline = {"-nsomething"};
    REQUIRE_NOTHROW(program(args, cmdline));
    REQUIRE(args.name == "something");
  }

  SECTION("long")
  {
    std::vector<std::string_view> cmdline = {"--name", "something"};
    REQUIRE_NOTHROW(program(args, cmdline));
    REQUIRE(args.name == "something");
  }

  SECTION("long partial")
  {
    std::vector<std::string_view> cmdline = {"--na", "something"};
    REQUIRE_NOTHROW(program(args, cmdline));
    REQUIRE(args.name == "something");
  }

  SECTION("long equal")
  {
    std::vector<std::string_view> cmdline = {"--name=something"};
    REQUIRE_NOTHROW(program(args, cmdline));
    REQUIRE(args.name == "something");
  }

  SECTION("long equal partial")
  {
    std::vector<std::string_view> cmdline = {"--na=something"};
    REQUIRE_NOTHROW(program(args, cmdline));
    REQUIRE(args.name == "something");
  }

  SECTION("short missing")
  {
    std::vector<std::string_view> cmdline = {"-n"};
    REQUIRE_THROWS_AS(program(args, cmdline), error<error_code::missing_argument>);
    REQUIRE(args.name == "default");
  }

  SECTION("short equal missing")
  {
    std::vector<std::string_view> cmdline = {"-n="};
    REQUIRE_THROWS_AS(program(args, cmdline), error<error_code::missing_argument>);
    REQUIRE(args.name == "default");
  }

  SECTION("long missing")
  {
    std::vector<std::string_view> cmdline = {"--name"};
    REQUIRE_THROWS_AS(program(args, cmdline), error<error_code::missing_argument>);
    REQUIRE(args.name == "default");
  }

  SECTION("long partial missing")
  {
    std::vector<std::string_view> cmdline = {"--na"};
    REQUIRE_THROWS_AS(program(args, cmdline), error<error_code::missing_argument>);
    REQUIRE(args.name == "default");
  }

  SECTION("long equal missing")
  {
    std::vector<std::string_view> cmdline = {"--name="};
    REQUIRE_THROWS_AS(program(args, cmdline), error<error_code::missing_argument>);
    REQUIRE(args.name == "default");
  }

  SECTION("long equal partial missing")
  {
    std::vector<std::string_view> cmdline = {"--na="};
    REQUIRE_THROWS_AS(program(args, cmdline), error<error_code::missing_argument>);
    REQUIRE(args.name == "default");
  }

  SECTION("short unknown")
  {
    std::vector<std::string_view> cmdline = {"-u", "something"};
    REQUIRE_THROWS_AS(program(args, cmdline), error<error_code::unknown_option>);
    REQUIRE(args.name == "default");
  }

  SECTION("long unknown")
  {
    std::vector<std::string_view> cmdline = {"--unknown", "something"};
    REQUIRE_THROWS_AS(program(args, cmdline), error<error_code::unknown_option>);
    REQUIRE(args.name == "default");
  }

  SECTION("long equal unknown")
  {
    std::vector<std::string_view> cmdline = {"--unknown=something"};
    REQUIRE_THROWS_AS(program(args, cmdline), error<error_code::unknown_option>);
    REQUIRE(args.name == "default");
  }
}

TEST_CASE("option value", "[poafloc/parser]")
{
  struct arguments
  {
    int value = 0;
  } args;

  auto program = parser<arguments> {
      direct {"v value", &arguments::value},
  };

  SECTION("short")
  {
    std::vector<std::string_view> cmdline = {"-v", "135"};
    REQUIRE_NOTHROW(program(args, cmdline));
    REQUIRE(args.value == 135);
  }

  SECTION("short equal")
  {
    std::vector<std::string_view> cmdline = {"-v=135"};
    REQUIRE_NOTHROW(program(args, cmdline));
    REQUIRE(args.value == 135);
  }

  SECTION("short together")
  {
    std::vector<std::string_view> cmdline = {"-v135"};
    REQUIRE_NOTHROW(program(args, cmdline));
    REQUIRE(args.value == 135);
  }

  SECTION("long")
  {
    std::vector<std::string_view> cmdline = {"--value", "135"};
    REQUIRE_NOTHROW(program(args, cmdline));
    REQUIRE(args.value == 135);
  }

  SECTION("long partial")
  {
    std::vector<std::string_view> cmdline = {"--val", "135"};
    REQUIRE_NOTHROW(program(args, cmdline));
    REQUIRE(args.value == 135);
  }

  SECTION("long equal")
  {
    std::vector<std::string_view> cmdline = {"--value=135"};
    REQUIRE_NOTHROW(program(args, cmdline));
    REQUIRE(args.value == 135);
  }

  SECTION("long equal partial")
  {
    std::vector<std::string_view> cmdline = {"--val=135"};
    REQUIRE_NOTHROW(program(args, cmdline));
    REQUIRE(args.value == 135);
  }

  SECTION("short missing")
  {
    std::vector<std::string_view> cmdline = {"-v"};
    REQUIRE_THROWS_AS(program(args, cmdline), error<error_code::missing_argument>);
    REQUIRE(args.value == 0);
  }

  SECTION("short equal missing")
  {
    std::vector<std::string_view> cmdline = {"-v="};
    REQUIRE_THROWS_AS(program(args, cmdline), error<error_code::missing_argument>);
    REQUIRE(args.value == 0);
  }

  SECTION("long missing")
  {
    std::vector<std::string_view> cmdline = {"--value"};
    REQUIRE_THROWS_AS(program(args, cmdline), error<error_code::missing_argument>);
    REQUIRE(args.value == 0);
  }

  SECTION("long partial missing")
  {
    std::vector<std::string_view> cmdline = {"--val"};
    REQUIRE_THROWS_AS(program(args, cmdline), error<error_code::missing_argument>);
    REQUIRE(args.value == 0);
  }

  SECTION("long equal missing")
  {
    std::vector<std::string_view> cmdline = {"--value="};
    REQUIRE_THROWS_AS(program(args, cmdline), error<error_code::missing_argument>);
    REQUIRE(args.value == 0);
  }

  SECTION("long equal partial missing")
  {
    std::vector<std::string_view> cmdline = {"--val="};
    REQUIRE_THROWS_AS(program(args, cmdline), error<error_code::missing_argument>);
    REQUIRE(args.value == 0);
  }

  SECTION("short unknown")
  {
    std::vector<std::string_view> cmdline = {"-u", "135"};
    REQUIRE_THROWS_AS(program(args, cmdline), error<error_code::unknown_option>);
    REQUIRE(args.value == 0);
  }

  SECTION("long unknown")
  {
    std::vector<std::string_view> cmdline = {"--unknown", "135"};
    REQUIRE_THROWS_AS(program(args, cmdline), error<error_code::unknown_option>);
    REQUIRE(args.value == 0);
  }

  SECTION("long equal unknown")
  {
    std::vector<std::string_view> cmdline = {"--unknown=135"};
    REQUIRE_THROWS_AS(program(args, cmdline), error<error_code::unknown_option>);
    REQUIRE(args.value == 0);
  }
}

TEST_CASE("positional", "[poafloc/parser]")
{
  struct arguments
  {
    bool flag = false;
    int value = 0;
  } args;

  auto program = parser<arguments> {
      boolean {"f flag", &arguments::flag},
      direct {"v value", &arguments::value},
  };

  SECTION("empty")
  {
    std::vector<std::string_view> cmdline = {};
    REQUIRE_NOTHROW(program(args, cmdline));
    REQUIRE(program.empty());
  }

  SECTION("one")
  {
    std::vector<std::string_view> cmdline = {"one"};
    REQUIRE_NOTHROW(program(args, cmdline));
    REQUIRE(program[0] == "one");
  }

  SECTION("two")
  {
    std::vector<std::string_view> cmdline = {"one", "two"};
    REQUIRE_NOTHROW(program(args, cmdline));
    REQUIRE(program[0] == "one");
    REQUIRE(program[1] == "two");
  }

  SECTION("flag short")
  {
    std::vector<std::string_view> cmdline = {"-f", "one", "two"};
    REQUIRE_NOTHROW(program(args, cmdline));
    REQUIRE(args.flag == true);
    REQUIRE(program[0] == "one");
    REQUIRE(program[1] == "two");
  }

  SECTION("flag long")
  {
    std::vector<std::string_view> cmdline = {"--flag", "one", "two"};
    REQUIRE_NOTHROW(program(args, cmdline));
    REQUIRE(args.flag == true);
    REQUIRE(program[0] == "one");
    REQUIRE(program[1] == "two");
  }

  SECTION("value short")
  {
    std::vector<std::string_view> cmdline = {"-v", "135", "one", "two"};
    REQUIRE_NOTHROW(program(args, cmdline));
    REQUIRE(args.value == 135);
    REQUIRE(program[0] == "one");
    REQUIRE(program[1] == "two");
  }

  SECTION("value short together")
  {
    std::vector<std::string_view> cmdline = {"-v135", "one", "two"};
    REQUIRE_NOTHROW(program(args, cmdline));
    REQUIRE(args.value == 135);
    REQUIRE(program[0] == "one");
    REQUIRE(program[1] == "two");
  }

  SECTION("value short together")
  {
    std::vector<std::string_view> cmdline = {"-v=135", "one", "two"};
    REQUIRE_NOTHROW(program(args, cmdline));
    REQUIRE(args.value == 135);
    REQUIRE(program[0] == "one");
    REQUIRE(program[1] == "two");
  }

  SECTION("value long")
  {
    std::vector<std::string_view> cmdline = {"--value", "135", "one", "two"};
    REQUIRE_NOTHROW(program(args, cmdline));
    REQUIRE(args.value == 135);
    REQUIRE(program[0] == "one");
    REQUIRE(program[1] == "two");
  }

  SECTION("value long equal")
  {
    std::vector<std::string_view> cmdline = {"--value=135", "one", "two"};
    REQUIRE_NOTHROW(program(args, cmdline));
    REQUIRE(args.value == 135);
    REQUIRE(program[0] == "one");
    REQUIRE(program[1] == "two");
  }

  SECTION("flag short terminal")
  {
    std::vector<std::string_view> cmdline = {"--", "one", "-f", "two"};
    REQUIRE_NOTHROW(program(args, cmdline));
    REQUIRE(program[0] == "one");
    REQUIRE(program[1] == "-f");
    REQUIRE(program[2] == "two");
  }

  SECTION("invalid terminal")
  {
    std::vector<std::string_view> cmdline = {"one", "--", "-f", "two"};
    REQUIRE_THROWS_AS(program(args, cmdline), error<error_code::invalid_terminal>);
  }

  SECTION("flag short non-terminal")
  {
    std::vector<std::string_view> cmdline = {"one", "-f", "two"};
    REQUIRE_THROWS_AS(program(args, cmdline), error<error_code::invalid_positional>);
  }

  SECTION("flag long non-terminal")
  {
    std::vector<std::string_view> cmdline = {"one", "--flag", "two"};
    REQUIRE_THROWS_AS(program(args, cmdline), error<error_code::invalid_positional>);
  }
}

TEST_CASE("multiple", "[poafloc/parser]")
{
  struct arguments
  {
    bool flag1 = false;
    bool flag2 = false;
    std::string value1 = "default";
    std::string value2 = "default";
  } args;

  auto program = parser<arguments> {
      boolean {"f flag1", &arguments::flag1},
      boolean {"F flag2", &arguments::flag2},
      direct {"v value1", &arguments::value1},
      direct {"V value2", &arguments::value2},
  };

  SECTION("valid")
  {
    std::vector<std::string_view> cmdline = {"--flag1", "--flag2"};
    REQUIRE_NOTHROW(program(args, cmdline));
    REQUIRE(args.flag1 == true);
    REQUIRE(args.flag2 == true);
    REQUIRE(args.value1 == "default");
    REQUIRE(args.value2 == "default");
  }

  SECTION("partial overlap")
  {
    std::vector<std::string_view> cmdline = {"--fla", "--fla"};
    REQUIRE_THROWS_AS(program(args, cmdline), error<error_code::unknown_option>);
    REQUIRE(args.flag1 == false);
    REQUIRE(args.flag2 == false);
    REQUIRE(args.value1 == "default");
    REQUIRE(args.value2 == "default");
  }

  SECTION("together")
  {
    std::vector<std::string_view> cmdline = {"-fvF"};
    REQUIRE_NOTHROW(program(args, cmdline));
    REQUIRE(args.flag1 == true);
    REQUIRE(args.flag2 == false);
    REQUIRE(args.value1 == "F");
    REQUIRE(args.value2 == "default");
  }

  SECTION("together equal")
  {
    std::vector<std::string_view> cmdline = {"-fv=F"};
    REQUIRE_NOTHROW(program(args, cmdline));
    REQUIRE(args.flag1 == true);
    REQUIRE(args.flag2 == false);
    REQUIRE(args.value1 == "F");
    REQUIRE(args.value2 == "default");
  }

  SECTION("together next")
  {
    std::vector<std::string_view> cmdline = {"-fv", "F"};
    REQUIRE_NOTHROW(program(args, cmdline));
    REQUIRE(args.flag1 == true);
    REQUIRE(args.flag2 == false);
    REQUIRE(args.value1 == "F");
    REQUIRE(args.value2 == "default");
  }
}

// NOLINTEND(*complexity*)
