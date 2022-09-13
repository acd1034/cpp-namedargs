#include <catch2/catch_test_macros.hpp>
#include <namedargs/fundamental.hpp>

TEST_CASE("main", "[main][squared]") {
  static_assert(std::is_same_v<decltype(namedargs::squared(0)), int>);
  CHECK(namedargs::squared(2) == 4);
}
