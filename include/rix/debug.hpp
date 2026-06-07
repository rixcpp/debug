/**
 * @file debug.hpp
 * @brief Unified debug module for Rix.
 */

#ifndef RIXCPP_DEBUG_INCLUDE_RIX_DEBUG_HPP_INCLUDED
#define RIXCPP_DEBUG_INCLUDE_RIX_DEBUG_HPP_INCLUDED

#include <rix/debug/format.hpp>
#include <rix/debug/inspect.hpp>
#include <rix/debug/log.hpp>
#include <rix/debug/print.hpp>

namespace rixlib::debug
{
  class Debug
  {
  public:
    Format format{};
    Print print{};
    Log log{};
    Inspect inspect{};
  };
}

#endif // RIXCPP_DEBUG_INCLUDE_RIX_DEBUG_HPP_INCLUDED
