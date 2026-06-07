/**
 * @file log.hpp
 * @brief Simple logging API for rix/debug.
 */

#ifndef RIXCPP_DEBUG_INCLUDE_RIX_DEBUG_LOG_HPP_INCLUDED
#define RIXCPP_DEBUG_INCLUDE_RIX_DEBUG_LOG_HPP_INCLUDED

#include <iostream>
#include <ostream>
#include <string_view>

#include <rix/debug/format.hpp>

namespace rixlib::debug
{
  class Log
  {
  public:
    template <typename... Args>
    void operator()(std::string_view fmt, const Args &...args) const
    {
      write(std::cout, "debug", fmt, args...);
    }

    template <typename... Args>
    void info(std::string_view fmt, const Args &...args) const
    {
      write(std::cout, "info", fmt, args...);
    }

    template <typename... Args>
    void warn(std::string_view fmt, const Args &...args) const
    {
      write(std::cout, "warn", fmt, args...);
    }

    template <typename... Args>
    void error(std::string_view fmt, const Args &...args) const
    {
      write(std::cerr, "error", fmt, args...);
    }

  private:
    template <typename... Args>
    void write(std::ostream &out, std::string_view level, std::string_view fmt, const Args &...args) const
    {
      out << '[' << level << "] " << format_(fmt, args...) << '\n';
    }

    Format format_{};
  };
}

#endif // RIXCPP_DEBUG_INCLUDE_RIX_DEBUG_LOG_HPP_INCLUDED
