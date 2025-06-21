#include <catch2/catch_test_macros.hpp>

TEST_CASE("Basic test to verify Catch2 is working", "[basic]") {
  REQUIRE(2 + 2 == 4);
  REQUIRE(true);
}

TEST_CASE("String operations work correctly", "[string]") {
  std::string hello = "Hello";
  std::string world = "World";
  std::string result = hello + " " + world;

  REQUIRE(result == "Hello World");
  REQUIRE(result.length() == 11);
}