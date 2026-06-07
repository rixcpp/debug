/**
 * @file basic.cpp
 * @brief Basic example for rix/debug.
 */

#include <rix/debug.hpp>

int main()
{
  rixlib::debug::Debug debug{};

  debug.print("Hello", "Rix");

  auto text = debug.format("Package: {}", "rix/debug");

  debug.log("loaded {} rows", 3);
  debug.log.warn("slow request: {}ms", 120);

  debug.inspect(text);

  return 0;
}
