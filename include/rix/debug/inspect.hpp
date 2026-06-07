/**
 * @file inspect.hpp
 * @brief Basic inspection API for rix/debug.
 */

#ifndef RIXCPP_DEBUG_INCLUDE_RIX_DEBUG_INSPECT_HPP_INCLUDED
#define RIXCPP_DEBUG_INCLUDE_RIX_DEBUG_INSPECT_HPP_INCLUDED

#include <iostream>
#include <sstream>
#include <string>
#include <typeinfo>

#include <rix/debug/format.hpp>

namespace rixlib::debug
{
  class Inspect
  {
  public:
    template <typename T>
    void operator()(const T &value) const
    {
      std::cout << to_string(value) << '\n';
    }

    template <typename T>
    [[nodiscard]] std::string to_string(const T &value) const
    {
      return detail::render_value(value);
    }

    template <typename T>
    void type() const
    {
      std::cout << type_name<T>() << '\n';
    }

    template <typename T>
    [[nodiscard]] std::string type_name() const
    {
      return typeid(T).name();
    }

    template <typename A, typename B>
    void diff(const A &a, const B &b) const
    {
      std::cout << "a: " << to_string(a) << '\n';
      std::cout << "b: " << to_string(b) << '\n';
    }

    template <typename Expected, typename Actual>
    [[nodiscard]] bool check(const Expected &expected, const Actual &actual) const
    {
      const bool ok = actual == expected;

      if (!ok)
      {
        std::cout << "[fail]\n";
        std::cout << "expected: " << to_string(expected) << '\n';
        std::cout << "actual:   " << to_string(actual) << '\n';
      }

      return ok;
    }
  };
}

#endif // RIXCPP_DEBUG_INCLUDE_RIX_DEBUG_INSPECT_HPP_INCLUDED
