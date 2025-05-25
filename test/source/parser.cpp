#define CATCH_CONFIG_RUNTIME_STATIC_REQUIRE

#include <string_view>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "poafloc/poafloc.hpp"

using poafloc::error;
using poafloc::error_code;
using poafloc::option;
using poafloc::parser;

// NOLINTBEGIN(*complexity*)
TEST_CASE("invalid", "[poafloc/parser]")
{
  struct arguments
  {
    bool flag;
    int value;
  };

  SECTION("duplicate short")
  {
    const auto construct = []()
    {
      return parser<arguments> {
          option {"f flag", &arguments::flag},
          option {"f follow", &arguments::value},
      };
    };
    REQUIRE_THROWS_AS(construct(), error<error_code::duplicate_option>);
  }

  SECTION("duplicate long")
  {
    const auto construct = []()
    {
      return parser<arguments> {
          option {"f flag", &arguments::flag},
          option {"v flag", &arguments::value},
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

  const auto program = parser<arguments> {
      option {"f flag", &arguments::flag},
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

  const auto program = parser<arguments> {
      option {"n name", &arguments::name},
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

  const auto program = parser<arguments> {
      option {"v value", &arguments::value},
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

TEST_CASE("multiple", "[poafloc/parser]")
{
  struct arguments
  {
    bool flag1 = false;
    bool flag2 = false;
    std::string value1 = "default";
    std::string value2 = "default";
  } args;

  const auto program = parser<arguments> {
      option {"f flag1", &arguments::flag1},
      option {"F flag2", &arguments::flag2},
      option {"v value1", &arguments::value1},
      option {"V value2", &arguments::value2},
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
