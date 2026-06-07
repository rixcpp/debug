/**
 * @file debug.hpp
 * @brief Unified debug module for Rix.
 */

#ifndef RIXCPP_DEBUG_INCLUDE_RIX_DEBUG_HPP_INCLUDED
#define RIXCPP_DEBUG_INCLUDE_RIX_DEBUG_HPP_INCLUDED

#include <string>

#include <rix/debug/format.hpp>
#include <rix/debug/inspect.hpp>
#include <rix/debug/log.hpp>
#include <rix/debug/print.hpp>

namespace rixlib::debug
{
  /**
   * @brief Object-style facade for the rix/debug package.
   */
  class Debug
  {
  public:
    /**
     * @brief Placeholder-based formatting component.
     */
    Format format{};

    /**
     * @brief Logging component.
     */
    Log log{};

    /**
     * @brief Inspection component.
     */
    Inspect inspect{};

    /**
     * @brief Print values separated by spaces, with a trailing newline.
     */
    template <typename... Args>
    void print(const Args &...args) const
    {
      rixlib::print(args...);
    }

    /**
     * @brief Print values to stderr.
     */
    template <typename... Args>
    void eprint(const Args &...args) const
    {
      rixlib::eprint(args...);
    }

    /**
     * @brief Debug-only print to stderr.
     */
    template <typename... Args>
    void dprint(const Args &...args) const
    {
      rixlib::dprint(args...);
    }

    /**
     * @brief Format printed values into a string.
     */
    template <typename... Args>
    [[nodiscard]] std::string sprint(const Args &...args) const
    {
      return rixlib::sprint(args...);
    }
  };
}

#endif // RIXCPP_DEBUG_INCLUDE_RIX_DEBUG_HPP_INCLUDED
