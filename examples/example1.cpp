#include <iostream>
#include <namedargs/parser.hpp>

namespace na = namedargs;

struct params {
  int num;
  std::string_view str;
};

template <>
struct na::ArgParserTraits<params> {
  static constexpr params convert(const na::ArgParser& p) {
    params result{};
    p.assign_or(result.num, "num", 0);
    p.assign_or(result.str, "str", "");
    return result;
  }
};

int main() {
  constexpr params p =
    na::parse_args<params>("num = 42, str = 'Hello, world!'");
    // na::parse_args<params>("num = 42, str = 'Hello, world!'     "); // ok
    // na::parse_args<params>("num = 42, str = 'Hello, world!', "); // ng
    // na::parse_args<params>("num = 42, str = 'Hello, world!', dummy"); // ng
    // na::parse_args<params>("num = 42, str = 'Hello, world!', dummy = "); // ng

  std::cout << "num: " << p.num << std::endl; // → num: 42
  std::cout << "str: " << p.str << std::endl; // → str: Hello, world!
}
