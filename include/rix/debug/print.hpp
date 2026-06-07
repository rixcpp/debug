/**
 * @file print.hpp
 * @brief Console printing API for rix/debug.
 */

#ifndef RIXCPP_DEBUG_INCLUDE_RIX_DEBUG_PRINT_HPP_INCLUDED
#define RIXCPP_DEBUG_INCLUDE_RIX_DEBUG_PRINT_HPP_INCLUDED

#include <iostream>
#include <ostream>
#include <string_view>

#include <rix/debug/format.hpp>

namespace rixlib::debug
{
  class Print
  {
  public:
    template <typename... Args>
    void operator()(std::string_view fmt, const Args &...args) const
    {
      std::cout << format_(fmt, args...) << '\n';
    }

    void operator()() const
    {
      std::cout << '\n';
    }

    template <typename... Args>
    void to(std::ostream &out, std::string_view fmt, const Args &...args) const
    {
      out << format_(fmt, args...) << '\n';
    }

    template <typename... Args>
    void inline_text(std::string_view fmt, const Args &...args) const
    {
      std::cout << format_(fmt, args...);
    }

  private:
    Format format_{};
  };
}

#endif // RIXCPP_DEBUG_INCLUDE_RIX_DEBUG_PRINT_HPP_INCLUDED
