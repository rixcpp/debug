/**
 * @file debug_tests.cpp
 * @brief Basic tests for rix/debug.
 *
 * @author Gaspard Kirira
 */

#include <rix/debug.hpp>

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>

namespace
{
  void expect_true(bool condition, const std::string &message)
  {
    if (!condition)
    {
      std::cerr << "FAILED: " << message << '\n';
      std::exit(1);
    }
  }

  void test_format_auto_placeholders()
  {
    const rixlib::debug::Debug debug;

    const std::string output = debug.format("Hello {}", "Rix");

    expect_true(
        output == "Hello Rix",
        "debug.format should support automatic placeholders");
  }

  void test_format_explicit_placeholders()
  {
    const rixlib::debug::Debug debug;

    const std::string output = debug.format("{0} + {0} = {1}", 2, 4);

    expect_true(
        output == "2 + 2 = 4",
        "debug.format should support explicit placeholders");
  }

  void test_format_escaped_braces()
  {
    const rixlib::debug::Debug debug;

    const std::string output = debug.format("{{ value }} = {}", 42);

    expect_true(
        output == "{ value } = 42",
        "debug.format should support escaped braces");
  }

  void test_format_append()
  {
    const rixlib::debug::Debug debug;

    std::string output = "prefix: ";
    debug.format.append(output, "{}", "ready");

    expect_true(
        output == "prefix: ready",
        "debug.format.append should append formatted text");
  }

  void test_format_to()
  {
    const rixlib::debug::Debug debug;

    std::string output = "old";
    debug.format.to(output, "status: {}", "ok");

    expect_true(
        output == "status: ok",
        "debug.format.to should replace destination content");
  }

  void test_print_to_stream()
  {
    const rixlib::debug::Debug debug;

    std::ostringstream out;
    debug.print.to(out, "Hello {}", "Rix");

    expect_true(
        out.str() == "Hello Rix\n",
        "debug.print.to should write formatted output with newline");
  }

  void test_inspect_to_string()
  {
    const rixlib::debug::Debug debug;

    expect_true(
        debug.inspect.to_string(42) == "42",
        "debug.inspect.to_string should render integers");

    expect_true(
        debug.inspect.to_string(true) == "true",
        "debug.inspect.to_string should render booleans");
  }

  void test_inspect_check_pass()
  {
    const rixlib::debug::Debug debug;

    const bool ok = debug.inspect.check(42, 42);

    expect_true(
        ok,
        "debug.inspect.check should return true when values are equal");
  }

  void test_inspect_check_fail()
  {
    const rixlib::debug::Debug debug;

    const bool ok = debug.inspect.check(42, 24);

    expect_true(
        !ok,
        "debug.inspect.check should return false when values are different");
  }

  void run_tests()
  {
    test_format_auto_placeholders();
    test_format_explicit_placeholders();
    test_format_escaped_braces();
    test_format_append();
    test_format_to();
    test_print_to_stream();
    test_inspect_to_string();
    test_inspect_check_pass();
    test_inspect_check_fail();
  }
}

int main()
{
  run_tests();

  std::cout << "debug tests passed\n";
  return 0;
}
