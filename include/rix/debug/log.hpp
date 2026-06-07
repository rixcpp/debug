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
  /**
   * @brief Simple formatted logging component for rix/debug.
   */
  class Log
  {
  public:
    /**
     * @brief Write a debug-level log message to stdout.
     */
    template <typename... Args>
    void operator()(std::string_view fmt, const Args &...args) const
    {
      write(std::cout, "debug", fmt, args...);
    }

    /**
     * @brief Write an info-level log message to stdout.
     */
    template <typename... Args>
    void info(std::string_view fmt, const Args &...args) const
    {
      write(std::cout, "info", fmt, args...);
    }

    /**
     * @brief Write a warning-level log message to stdout.
     */
    template <typename... Args>
    void warn(std::string_view fmt, const Args &...args) const
    {
      write(std::cout, "warn", fmt, args...);
    }

    /**
     * @brief Write an error-level log message to stderr.
     */
    template <typename... Args>
    void error(std::string_view fmt, const Args &...args) const
    {
      write(std::cerr, "error", fmt, args...);
    }

  private:
    template <typename... Args>
    void write(std::ostream &out,
               std::string_view level,
               std::string_view fmt,
               const Args &...args) const
    {
      out << '[' << level << "] " << format_(fmt, args...) << '\n';
    }

    Format format_{};
  };
}

#endif // RIXCPP_DEBUG_INCLUDE_RIX_DEBUG_LOG_HPP_INCLUDED
