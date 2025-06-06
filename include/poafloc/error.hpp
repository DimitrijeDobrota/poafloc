#pragma once

#include <format>
#include <stdexcept>

#include <based/enum/enum.hpp>
#include <based/format.hpp>
#include <based/types/types.hpp>
#include <based/utility/forward.hpp>

namespace poafloc
{

#define ENUM_ERROR                                                             \
  help, empty, invalid_option, invalid_positional, invalid_terminal,           \
      missing_option, missing_argument, missing_positional,                    \
      superfluous_argument, superfluous_positional, unknown_option,            \
      duplicate_option
BASED_DECLARE_ENUM(error_code, based::bu8, 0, ENUM_ERROR)
BASED_DEFINE_ENUM(error_code, based::bu8, 0, ENUM_ERROR)
#undef ENUM_ERROR

static constexpr const char* error_get_message(error_code::enum_type error)
{
  switch (error()) {
    case error_code::help():
      return "Help text was displayed";
    case error_code::empty():
      return "Option list is empty";
    case error_code::invalid_option():
      return "Invalid option name: {}";
    case error_code::invalid_positional():
      return "Invalid positional argument: {}";
    case error_code::invalid_terminal():
      return "Invalid positional argument";
    case error_code::missing_option():
      return "Missing at least on of long and short options";
    case error_code::missing_argument():
      return "Missing argument for option: {}";
    case error_code::missing_positional():
      return "Too little positional arguments, require: {}";
    case error_code::superfluous_argument():
      return "Option doesn't require an argument: {}";
    case error_code::superfluous_positional():
      return "Too few positional arguments, require: {}";
    case error_code::unknown_option():
      return "Unknown option: {}";
    case error_code::duplicate_option():
      return "Duplicate option: {}";
    default:
      return "poafloc error, should not happen...";
  }
}

class runtime_error : public std::runtime_error
{
public:
  explicit runtime_error(const std::string& err)
      : std::runtime_error(err)
  {
  }
};

template<error_code::enum_type e>
class error : public runtime_error
{
public:
  template<class... Args>
  explicit error(Args... args)
      : runtime_error(
            std::format(error_get_message(e), based::forward<Args>(args)...)
        )
  {
  }
};

}  // namespace poafloc
