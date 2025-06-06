#define CATCH_CONFIG_RUNTIME_STATIC_REQUIRE

#include <string_view>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "poafloc/error.hpp"
#include "poafloc/poafloc.hpp"

using namespace poafloc;  // NOLINT

// NOLINTBEGIN(*complexity*)
TEST_CASE("invalid", "[poafloc/parser]")
{
  struct arguments
  {
    bool flag;
    int value;
  };

  SECTION("empty")
  {
    auto program = parser<arguments> {
        group {
            "unnamed",
            boolean {"f", &arguments::flag, "NUM something"},
        },
    };
    arguments args = {};
    std::vector<std::string_view> cmdline = {};
    REQUIRE_THROWS_AS(program(args, cmdline), error<error_code::empty>);
  };

  SECTION("short number")
  {
    auto construct = []()
    {
      return parser<arguments> {
          group {
              "unnamed",
              boolean {"1", &arguments::flag, "NUM something"},
          },
      };
    };
    REQUIRE_THROWS_AS(construct(), error<error_code::invalid_option>);
  }

  SECTION("long upper")
  {
    auto construct = []()
    {
      return parser<arguments> {
          group {
              "unnamed",
              boolean {"FLAG", &arguments::flag, "something"},
          },
      };
    };
    REQUIRE_THROWS_AS(construct(), error<error_code::invalid_option>);
  }

  SECTION("long number start")
  {
    auto construct = []()
    {
      return parser<arguments> {
          group {
              "unnamed",
              direct {"1value", &arguments::value, "NUM something"},
          },
      };
    };
    REQUIRE_THROWS_AS(construct(), error<error_code::invalid_option>);
  }

  SECTION("short duplicate")
  {
    auto construct = []()
    {
      return parser<arguments> {
          group {
              "unnamed",
              boolean {"f flag", &arguments::flag, "something"},
              direct {"f follow", &arguments::value, "NUM something"},
          },
      };
    };
    REQUIRE_THROWS_AS(construct(), error<error_code::duplicate_option>);
  }

  SECTION("long duplicate")
  {
    auto construct = []()
    {
      return parser<arguments> {
          group {
              "unnamed",
              boolean {"f flag", &arguments::flag, "something"},
              direct {"v flag", &arguments::value, "something"},
          },
      };
    };
    REQUIRE_THROWS_AS(construct(), error<error_code::duplicate_option>);
  }
}

TEST_CASE("boolean", "[poafloc/parser]")
{
  struct arguments
  {
    bool flag = false;
  } args;

  auto program = parser<arguments> {
      group {
          "unnamed",
          boolean {"f flag", &arguments::flag, "something"},
      },
  };

  SECTION("short")
  {
    std::vector<std::string_view> cmdline = {"test", "-f"};
    REQUIRE_NOTHROW(program(args, cmdline));
    REQUIRE(args.flag == true);
  }

  SECTION("long")
  {
    std::vector<std::string_view> cmdline = {"test", "--flag"};
    REQUIRE_NOTHROW(program(args, cmdline));
    REQUIRE(args.flag == true);
  }

  SECTION("long partial")
  {
    std::vector<std::string_view> cmdline = {"test", "--fl"};
    REQUIRE_NOTHROW(program(args, cmdline));
    REQUIRE(args.flag == true);
  }

  SECTION("short unknown")
  {
    std::vector<std::string_view> cmdline = {"test", "-u"};
    REQUIRE_THROWS_AS(program(args, cmdline), error<error_code::unknown_option>);
    REQUIRE(args.flag == false);
  }

  SECTION("long unknown")
  {
    std::vector<std::string_view> cmdline = {"test", "--unknown"};
    REQUIRE_THROWS_AS(program(args, cmdline), error<error_code::unknown_option>);
    REQUIRE(args.flag == false);
  }

  SECTION("long superfluous")
  {
    std::vector<std::string_view> cmdline = {"test", "--fl=something"};
    REQUIRE_THROWS_AS(program(args, cmdline), error<error_code::superfluous_argument>);
    REQUIRE(args.flag == false);
  }

  SECTION("long superfluous missing")
  {
    std::vector<std::string_view> cmdline = {"test", "--fl="};
    REQUIRE_THROWS_AS(program(args, cmdline), error<error_code::superfluous_argument>);
    REQUIRE(args.flag == false);
  }
}

TEST_CASE("direct string", "[poafloc/parser]")
{
  struct arguments
  {
    std::string name = "default";
  } args;

  auto program = parser<arguments> {
      group {
          "unnamed",
          direct {"n name", &arguments::name, "NAME something"},
      },
  };

  SECTION("short")
  {
    std::vector<std::string_view> cmdline = {"test", "-n", "something"};
    REQUIRE_NOTHROW(program(args, cmdline));
    REQUIRE(args.name == "something");
  }

  SECTION("short equal")
  {
    std::vector<std::string_view> cmdline = {"test", "-n=something"};
    REQUIRE_NOTHROW(program(args, cmdline));
    REQUIRE(args.name == "something");
  }

  SECTION("short together")
  {
    std::vector<std::string_view> cmdline = {"test", "-nsomething"};
    REQUIRE_NOTHROW(program(args, cmdline));
    REQUIRE(args.name == "something");
  }

  SECTION("long")
  {
    std::vector<std::string_view> cmdline = {"test", "--name", "something"};
    REQUIRE_NOTHROW(program(args, cmdline));
    REQUIRE(args.name == "something");
  }

  SECTION("long partial")
  {
    std::vector<std::string_view> cmdline = {"test", "--na", "something"};
    REQUIRE_NOTHROW(program(args, cmdline));
    REQUIRE(args.name == "something");
  }

  SECTION("long equal")
  {
    std::vector<std::string_view> cmdline = {"test", "--name=something"};
    REQUIRE_NOTHROW(program(args, cmdline));
    REQUIRE(args.name == "something");
  }

  SECTION("long equal partial")
  {
    std::vector<std::string_view> cmdline = {"test", "--na=something"};
    REQUIRE_NOTHROW(program(args, cmdline));
    REQUIRE(args.name == "something");
  }

  SECTION("short missing")
  {
    std::vector<std::string_view> cmdline = {"test", "-n"};
    REQUIRE_THROWS_AS(program(args, cmdline), error<error_code::missing_argument>);
    REQUIRE(args.name == "default");
  }

  SECTION("short equal missing")
  {
    std::vector<std::string_view> cmdline = {"test", "-n="};
    REQUIRE_THROWS_AS(program(args, cmdline), error<error_code::missing_argument>);
    REQUIRE(args.name == "default");
  }

  SECTION("long missing")
  {
    std::vector<std::string_view> cmdline = {"test", "--name"};
    REQUIRE_THROWS_AS(program(args, cmdline), error<error_code::missing_argument>);
    REQUIRE(args.name == "default");
  }

  SECTION("long partial missing")
  {
    std::vector<std::string_view> cmdline = {"test", "--na"};
    REQUIRE_THROWS_AS(program(args, cmdline), error<error_code::missing_argument>);
    REQUIRE(args.name == "default");
  }

  SECTION("long equal missing")
  {
    std::vector<std::string_view> cmdline = {"test", "--name="};
    REQUIRE_THROWS_AS(program(args, cmdline), error<error_code::missing_argument>);
    REQUIRE(args.name == "default");
  }

  SECTION("long equal partial missing")
  {
    std::vector<std::string_view> cmdline = {"test", "--na="};
    REQUIRE_THROWS_AS(program(args, cmdline), error<error_code::missing_argument>);
    REQUIRE(args.name == "default");
  }

  SECTION("short unknown")
  {
    std::vector<std::string_view> cmdline = {"test", "-u", "something"};
    REQUIRE_THROWS_AS(program(args, cmdline), error<error_code::unknown_option>);
    REQUIRE(args.name == "default");
  }

  SECTION("long unknown")
  {
    std::vector<std::string_view> cmdline = {"test", "--unknown", "something"};
    REQUIRE_THROWS_AS(program(args, cmdline), error<error_code::unknown_option>);
    REQUIRE(args.name == "default");
  }

  SECTION("long equal unknown")
  {
    std::vector<std::string_view> cmdline = {"test", "--unknown=something"};
    REQUIRE_THROWS_AS(program(args, cmdline), error<error_code::unknown_option>);
    REQUIRE(args.name == "default");
  }
}

TEST_CASE("direct value", "[poafloc/parser]")
{
  struct arguments
  {
    int value = 0;
  } args;

  auto program = parser<arguments> {
      group {
          "unnamed",
          direct {"v value", &arguments::value, "NUM something"},
      },
  };

  SECTION("short")
  {
    std::vector<std::string_view> cmdline = {"test", "-v", "135"};
    REQUIRE_NOTHROW(program(args, cmdline));
    REQUIRE(args.value == 135);
  }

  SECTION("short equal")
  {
    std::vector<std::string_view> cmdline = {"test", "-v=135"};
    REQUIRE_NOTHROW(program(args, cmdline));
    REQUIRE(args.value == 135);
  }

  SECTION("short together")
  {
    std::vector<std::string_view> cmdline = {"test", "-v135"};
    REQUIRE_NOTHROW(program(args, cmdline));
    REQUIRE(args.value == 135);
  }

  SECTION("long")
  {
    std::vector<std::string_view> cmdline = {"test", "--value", "135"};
    REQUIRE_NOTHROW(program(args, cmdline));
    REQUIRE(args.value == 135);
  }

  SECTION("long partial")
  {
    std::vector<std::string_view> cmdline = {"test", "--val", "135"};
    REQUIRE_NOTHROW(program(args, cmdline));
    REQUIRE(args.value == 135);
  }

  SECTION("long equal")
  {
    std::vector<std::string_view> cmdline = {"test", "--value=135"};
    REQUIRE_NOTHROW(program(args, cmdline));
    REQUIRE(args.value == 135);
  }

  SECTION("long equal partial")
  {
    std::vector<std::string_view> cmdline = {"test", "--val=135"};
    REQUIRE_NOTHROW(program(args, cmdline));
    REQUIRE(args.value == 135);
  }

  SECTION("short missing")
  {
    std::vector<std::string_view> cmdline = {"test", "-v"};
    REQUIRE_THROWS_AS(program(args, cmdline), error<error_code::missing_argument>);
    REQUIRE(args.value == 0);
  }

  SECTION("short equal missing")
  {
    std::vector<std::string_view> cmdline = {"test", "-v="};
    REQUIRE_THROWS_AS(program(args, cmdline), error<error_code::missing_argument>);
    REQUIRE(args.value == 0);
  }

  SECTION("long missing")
  {
    std::vector<std::string_view> cmdline = {"test", "--value"};
    REQUIRE_THROWS_AS(program(args, cmdline), error<error_code::missing_argument>);
    REQUIRE(args.value == 0);
  }

  SECTION("long partial missing")
  {
    std::vector<std::string_view> cmdline = {"test", "--val"};
    REQUIRE_THROWS_AS(program(args, cmdline), error<error_code::missing_argument>);
    REQUIRE(args.value == 0);
  }

  SECTION("long equal missing")
  {
    std::vector<std::string_view> cmdline = {"test", "--value="};
    REQUIRE_THROWS_AS(program(args, cmdline), error<error_code::missing_argument>);
    REQUIRE(args.value == 0);
  }

  SECTION("long equal partial missing")
  {
    std::vector<std::string_view> cmdline = {"test", "--val="};
    REQUIRE_THROWS_AS(program(args, cmdline), error<error_code::missing_argument>);
    REQUIRE(args.value == 0);
  }

  SECTION("short unknown")
  {
    std::vector<std::string_view> cmdline = {"test", "-u", "135"};
    REQUIRE_THROWS_AS(program(args, cmdline), error<error_code::unknown_option>);
    REQUIRE(args.value == 0);
  }

  SECTION("long unknown")
  {
    std::vector<std::string_view> cmdline = {"test", "--unknown", "135"};
    REQUIRE_THROWS_AS(program(args, cmdline), error<error_code::unknown_option>);
    REQUIRE(args.value == 0);
  }

  SECTION("long equal unknown")
  {
    std::vector<std::string_view> cmdline = {"test", "--unknown=135"};
    REQUIRE_THROWS_AS(program(args, cmdline), error<error_code::unknown_option>);
    REQUIRE(args.value == 0);
  }
}

TEST_CASE("list", "[poafloc/parser]")
{
  struct arguments
  {
    void add(std::string_view value) { list.emplace_back(value); }

    std::vector<std::string> list;
  } args;

  auto program = parser<arguments> {group {
      "unnamed",
      list {"l list", &arguments::add, "NAME something"},
  }};

  SECTION("short empty")
  {
    std::vector<std::string_view> cmdline = {"test", "-l"};
    REQUIRE_THROWS_AS(program(args, cmdline), error<error_code::missing_argument>);
  }

  SECTION("short one")
  {
    std::vector<std::string_view> cmdline = {"test", "-l", "one"};
    REQUIRE_NOTHROW(program(args, cmdline));
    REQUIRE(std::size(args.list) == 1);
    REQUIRE(args.list[0] == "one");
  }

  SECTION("short two")
  {
    std::vector<std::string_view> cmdline = {"test", "-l", "one", "two"};
    REQUIRE_NOTHROW(program(args, cmdline));
    REQUIRE(std::size(args.list) == 2);
    REQUIRE(args.list[0] == "one");
    REQUIRE(args.list[1] == "two");
  }

  SECTION("short three")
  {
    std::vector<std::string_view> cmdline = {
        "test", "-l", "one", "two", "three"
    };
    REQUIRE_NOTHROW(program(args, cmdline));
    REQUIRE(std::size(args.list) == 3);
    REQUIRE(args.list[0] == "one");
    REQUIRE(args.list[1] == "two");
    REQUIRE(args.list[2] == "three");
  }

  SECTION("short terminal")
  {
    std::vector<std::string_view> cmdline = {"test", "-l", "--"};
    REQUIRE_THROWS_AS(program(args, cmdline), error<error_code::missing_argument>);
  }

  SECTION("short terminal one")
  {
    std::vector<std::string_view> cmdline = {"test", "-l", "one", "--"};
    REQUIRE_NOTHROW(program(args, cmdline));
    REQUIRE(std::size(args.list) == 1);
    REQUIRE(args.list[0] == "one");
  }

  SECTION("short terminal two")
  {
    std::vector<std::string_view> cmdline = {"test", "-l", "one", "two", "--"};
    REQUIRE_NOTHROW(program(args, cmdline));
    REQUIRE(std::size(args.list) == 2);
    REQUIRE(args.list[0] == "one");
    REQUIRE(args.list[1] == "two");
  }

  SECTION("short terminal three")
  {
    std::vector<std::string_view> cmdline = {
        "test", "-l", "one", "two", "three", "--"
    };
    REQUIRE_NOTHROW(program(args, cmdline));
    REQUIRE(std::size(args.list) == 3);
    REQUIRE(args.list[0] == "one");
    REQUIRE(args.list[1] == "two");
    REQUIRE(args.list[2] == "three");
  }

  SECTION("long empty")
  {
    std::vector<std::string_view> cmdline = {"test", "--list"};
    REQUIRE_THROWS_AS(program(args, cmdline), error<error_code::missing_argument>);
  }

  SECTION("long one")
  {
    std::vector<std::string_view> cmdline = {"test", "--list", "one"};
    REQUIRE_NOTHROW(program(args, cmdline));
    REQUIRE(std::size(args.list) == 1);
    REQUIRE(args.list[0] == "one");
  }

  SECTION("long two")
  {
    std::vector<std::string_view> cmdline = {"test", "--list", "one", "two"};
    REQUIRE_NOTHROW(program(args, cmdline));
    REQUIRE(std::size(args.list) == 2);
    REQUIRE(args.list[0] == "one");
    REQUIRE(args.list[1] == "two");
  }

  SECTION("long three")
  {
    std::vector<std::string_view> cmdline = {
        "test", "--list", "one", "two", "three"
    };
    REQUIRE_NOTHROW(program(args, cmdline));
    REQUIRE(std::size(args.list) == 3);
    REQUIRE(args.list[0] == "one");
    REQUIRE(args.list[1] == "two");
    REQUIRE(args.list[2] == "three");
  }

  SECTION("long terminal")
  {
    std::vector<std::string_view> cmdline = {"test", "--list", "--"};
    REQUIRE_THROWS_AS(program(args, cmdline), error<error_code::missing_argument>);
  }

  SECTION("long terminal one")
  {
    std::vector<std::string_view> cmdline = {"test", "--list", "one", "--"};
    REQUIRE_NOTHROW(program(args, cmdline));
    REQUIRE(std::size(args.list) == 1);
    REQUIRE(args.list[0] == "one");
  }

  SECTION("long terminal two")
  {
    std::vector<std::string_view> cmdline = {
        "test", "--list", "one", "two", "--"
    };
    REQUIRE_NOTHROW(program(args, cmdline));
    REQUIRE(std::size(args.list) == 2);
    REQUIRE(args.list[0] == "one");
    REQUIRE(args.list[1] == "two");
  }

  SECTION("long terminal three")
  {
    std::vector<std::string_view> cmdline = {
        "test", "--list", "one", "two", "three", "--"
    };
    REQUIRE_NOTHROW(program(args, cmdline));
    REQUIRE(std::size(args.list) == 3);
    REQUIRE(args.list[0] == "one");
    REQUIRE(args.list[1] == "two");
    REQUIRE(args.list[2] == "three");
  }
}

TEST_CASE("positional", "[poafloc/parser]")
{
  struct arguments
  {
    bool flag = false;
    int value = 0;
    std::string one;
    std::string two;
  } args;

  auto program = parser<arguments> {
      positional {
          argument {"one", &arguments::one},
          argument {"two", &arguments::two},
      },
      group {
          "unnamed",
          boolean {"f flag", &arguments::flag, "something"},
          direct {"v value", &arguments::value, "NUM something"},
      }
  };

  SECTION("empty")
  {
    std::vector<std::string_view> cmdline = {
        "test",
    };
    REQUIRE_THROWS_AS(program(args, cmdline), error<error_code::missing_positional>);
  }

  SECTION("one")
  {
    std::vector<std::string_view> cmdline = {"test", "one"};
    REQUIRE_THROWS_AS(program(args, cmdline), error<error_code::missing_positional>);
    REQUIRE(args.one == "one");
  }

  SECTION("two")
  {
    std::vector<std::string_view> cmdline = {"test", "one", "two"};
    REQUIRE_NOTHROW(program(args, cmdline));
    REQUIRE(args.one == "one");
    REQUIRE(args.two == "two");
  }

  SECTION("three")
  {
    std::vector<std::string_view> cmdline = {"test", "one", "two", "three"};
    REQUIRE_THROWS_AS(program(args, cmdline), error<error_code::superfluous_positional>);
    REQUIRE(args.one == "one");
    REQUIRE(args.two == "two");
  }

  SECTION("flag short")
  {
    std::vector<std::string_view> cmdline = {"test", "-f", "one", "two"};
    REQUIRE_NOTHROW(program(args, cmdline));
    REQUIRE(args.flag == true);
    REQUIRE(args.one == "one");
    REQUIRE(args.two == "two");
  }

  SECTION("flag long")
  {
    std::vector<std::string_view> cmdline = {"test", "--flag", "one", "two"};
    REQUIRE_NOTHROW(program(args, cmdline));
    REQUIRE(args.flag == true);
    REQUIRE(args.one == "one");
    REQUIRE(args.two == "two");
  }

  SECTION("value short")
  {
    std::vector<std::string_view> cmdline = {"test", "-v", "135", "one", "two"};
    REQUIRE_NOTHROW(program(args, cmdline));
    REQUIRE(args.value == 135);
    REQUIRE(args.one == "one");
    REQUIRE(args.two == "two");
  }

  SECTION("value short together")
  {
    std::vector<std::string_view> cmdline = {"test", "-v135", "one", "two"};
    REQUIRE_NOTHROW(program(args, cmdline));
    REQUIRE(args.value == 135);
    REQUIRE(args.one == "one");
    REQUIRE(args.two == "two");
  }

  SECTION("value short together")
  {
    std::vector<std::string_view> cmdline = {"test", "-v=135", "one", "two"};
    REQUIRE_NOTHROW(program(args, cmdline));
    REQUIRE(args.value == 135);
    REQUIRE(args.one == "one");
    REQUIRE(args.two == "two");
  }

  SECTION("value long")
  {
    std::vector<std::string_view> cmdline = {
        "test", "--value", "135", "one", "two"
    };
    REQUIRE_NOTHROW(program(args, cmdline));
    REQUIRE(args.value == 135);
    REQUIRE(args.one == "one");
    REQUIRE(args.two == "two");
  }

  SECTION("value long equal")
  {
    std::vector<std::string_view> cmdline = {
        "test", "--value=135", "one", "two"
    };
    REQUIRE_NOTHROW(program(args, cmdline));
    REQUIRE(args.value == 135);
    REQUIRE(args.one == "one");
    REQUIRE(args.two == "two");
  }

  SECTION("flag short terminal")
  {
    std::vector<std::string_view> cmdline = {"test", "--", "one", "-f", "two"};
    REQUIRE_THROWS_AS(program(args, cmdline), error<error_code::superfluous_positional>);
    REQUIRE(args.one == "one");
    REQUIRE(args.two == "-f");
  }

  SECTION("invalid terminal")
  {
    std::vector<std::string_view> cmdline = {"test", "one", "--", "-f", "two"};
    REQUIRE_THROWS_AS(program(args, cmdline), error<error_code::invalid_terminal>);
  }

  SECTION("flag short non-terminal")
  {
    std::vector<std::string_view> cmdline = {"test", "one", "-f", "two"};
    REQUIRE_THROWS_AS(program(args, cmdline), error<error_code::invalid_positional>);
  }

  SECTION("flag long non-terminal")
  {
    std::vector<std::string_view> cmdline = {"test", "one", "--flag", "two"};
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
      group {
          "unnamed",
          boolean {"f flag1", &arguments::flag1, "something"},
          boolean {"F flag2", &arguments::flag2, "something"},
          direct {"v value1", &arguments::value1, "NUM something"},
          direct {"V value2", &arguments::value2, "NUM something"},
      },
  };

  SECTION("valid")
  {
    std::vector<std::string_view> cmdline = {"test", "--flag1", "--flag2"};
    REQUIRE_NOTHROW(program(args, cmdline));
    REQUIRE(args.flag1 == true);
    REQUIRE(args.flag2 == true);
    REQUIRE(args.value1 == "default");
    REQUIRE(args.value2 == "default");
  }

  SECTION("partial overlap")
  {
    std::vector<std::string_view> cmdline = {"test", "--fla", "--fla"};
    REQUIRE_THROWS_AS(program(args, cmdline), error<error_code::unknown_option>);
    REQUIRE(args.flag1 == false);
    REQUIRE(args.flag2 == false);
    REQUIRE(args.value1 == "default");
    REQUIRE(args.value2 == "default");
  }

  SECTION("together")
  {
    std::vector<std::string_view> cmdline = {"test", "-fvF"};
    REQUIRE_NOTHROW(program(args, cmdline));
    REQUIRE(args.flag1 == true);
    REQUIRE(args.flag2 == false);
    REQUIRE(args.value1 == "F");
    REQUIRE(args.value2 == "default");
  }

  SECTION("together equal")
  {
    std::vector<std::string_view> cmdline = {"test", "-fv=F"};
    REQUIRE_NOTHROW(program(args, cmdline));
    REQUIRE(args.flag1 == true);
    REQUIRE(args.flag2 == false);
    REQUIRE(args.value1 == "F");
    REQUIRE(args.value2 == "default");
  }

  SECTION("together next")
  {
    std::vector<std::string_view> cmdline = {"test", "-fv", "F"};
    REQUIRE_NOTHROW(program(args, cmdline));
    REQUIRE(args.flag1 == true);
    REQUIRE(args.flag2 == false);
    REQUIRE(args.value1 == "F");
    REQUIRE(args.value2 == "default");
  }
}

// NOLINTEND(*complexity*)
