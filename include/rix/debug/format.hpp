/**
 * @file format.hpp
 * @brief Placeholder-based formatting API for rix/debug.
 */

#ifndef RIXCPP_DEBUG_INCLUDE_RIX_DEBUG_FORMAT_HPP_INCLUDED
#define RIXCPP_DEBUG_INCLUDE_RIX_DEBUG_FORMAT_HPP_INCLUDED

#include <array>
#include <cstddef>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>

namespace rixlib::debug
{
  class FormatError : public std::runtime_error
  {
  public:
    explicit FormatError(const std::string &message)
        : std::runtime_error(message)
    {
    }

    explicit FormatError(const char *message)
        : std::runtime_error(message)
    {
    }
  };

  namespace detail
  {
    template <typename T>
    [[nodiscard]] std::string render_value(const T &value)
    {
      std::ostringstream stream;

      if constexpr (std::is_same_v<std::remove_cvref_t<T>, bool>)
      {
        stream << (value ? "true" : "false");
      }
      else
      {
        stream << value;
      }

      return stream.str();
    }

    class RenderedArgs
    {
    public:
      template <std::size_t N>
      explicit RenderedArgs(const std::array<std::string, N> &values) noexcept
          : data_(values.data()), size_(N)
      {
      }

      [[nodiscard]] std::size_t size() const noexcept
      {
        return size_;
      }

      [[nodiscard]] const std::string &at(std::size_t index) const
      {
        if (index >= size_)
        {
          throw FormatError("format argument index out of range");
        }

        return data_[index];
      }

    private:
      const std::string *data_{nullptr};
      std::size_t size_{0};
    };

    [[nodiscard]] inline std::size_t parse_index(std::string_view text)
    {
      if (text.empty())
      {
        throw FormatError("empty explicit format index");
      }

      std::size_t value = 0;

      for (char ch : text)
      {
        if (ch < '0' || ch > '9')
        {
          throw FormatError("invalid explicit format index");
        }

        value = (value * 10u) + static_cast<std::size_t>(ch - '0');
      }

      return value;
    }

    inline void render_format_string(
        std::string &out,
        std::string_view fmt,
        const RenderedArgs &args)
    {
      std::size_t i = 0;
      std::size_t next_auto_index = 0;
      bool used_auto_index = false;
      bool used_explicit_index = false;

      while (i < fmt.size())
      {
        const char ch = fmt[i];

        if (ch == '{')
        {
          if ((i + 1u) < fmt.size() && fmt[i + 1u] == '{')
          {
            out.push_back('{');
            i += 2u;
            continue;
          }

          const std::size_t close = fmt.find('}', i + 1u);

          if (close == std::string_view::npos)
          {
            throw FormatError("unmatched '{' in format string");
          }

          const std::string_view token = fmt.substr(i + 1u, close - (i + 1u));

          if (token.empty())
          {
            if (used_explicit_index)
            {
              throw FormatError("cannot mix automatic and explicit format indexes");
            }

            used_auto_index = true;
            out += args.at(next_auto_index++);
          }
          else
          {
            if (used_auto_index)
            {
              throw FormatError("cannot mix automatic and explicit format indexes");
            }

            if (token.find(':') != std::string_view::npos)
            {
              throw FormatError("format specifiers are not supported yet");
            }

            used_explicit_index = true;
            out += args.at(parse_index(token));
          }

          i = close + 1u;
          continue;
        }

        if (ch == '}')
        {
          if ((i + 1u) < fmt.size() && fmt[i + 1u] == '}')
          {
            out.push_back('}');
            i += 2u;
            continue;
          }

          throw FormatError("single '}' encountered in format string");
        }

        out.push_back(ch);
        ++i;
      }
    }

    [[nodiscard]] inline std::size_t estimate_output_size(
        std::string_view fmt,
        const RenderedArgs &args)
    {
      std::size_t total = fmt.size();

      for (std::size_t i = 0; i < args.size(); ++i)
      {
        total += args.at(i).size();
      }

      return total;
    }
  }

  class Format
  {
  public:
    template <typename... Args>
    [[nodiscard]] std::string operator()(std::string_view fmt, const Args &...args) const
    {
      std::array<std::string, sizeof...(Args)> rendered_args{
          detail::render_value(args)...};

      const detail::RenderedArgs rendered{rendered_args};

      std::string output;
      output.reserve(detail::estimate_output_size(fmt, rendered));
      detail::render_format_string(output, fmt, rendered);

      return output;
    }

    template <typename... Args>
    void append(std::string &out, std::string_view fmt, const Args &...args) const
    {
      out += operator()(fmt, args...);
    }

    template <typename... Args>
    void to(std::string &out, std::string_view fmt, const Args &...args) const
    {
      out.clear();
      append(out, fmt, args...);
    }
  };
}

#endif // RIXCPP_DEBUG_INCLUDE_RIX_DEBUG_FORMAT_HPP_INCLUDED
