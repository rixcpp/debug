/**
 *
 *  @file inspect.hpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2025, Gaspard Kirira. All rights reserved.
 *  https://github.com/rixcpp/rix
 *  Use of this source code is governed by a MIT license
 *  that can be found in the License file.
 *
 *  Rix
 *
 *  Inspection and diagnostic rendering API for rix/debug.
 *
 */

#ifndef RIXCPP_DEBUG_INCLUDE_RIX_DEBUG_INSPECT_HPP_INCLUDED
#define RIXCPP_DEBUG_INCLUDE_RIX_DEBUG_INSPECT_HPP_INCLUDED

#include <algorithm>
#include <any>
#include <array>
#include <chrono>
#include <cmath>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <filesystem>
#include <forward_list>
#include <functional>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <ostream>
#include <queue>
#include <set>
#include <sstream>
#include <stack>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <typeinfo>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>
#include <cstdlib>
#include <system_error>

#include <rix/debug/traits.hpp>

#if defined(__cpp_lib_expected) && __cpp_lib_expected >= 202202L
#include <expected>
#define RIXCPP_DEBUG_HAS_EXPECTED 1
#else
#define RIXCPP_DEBUG_HAS_EXPECTED 0
#endif

#if defined(__GNUC__) || defined(__clang__)
#include <cxxabi.h>
#define RIXCPP_DEBUG_HAS_DEMANGLE 1
#else
#define RIXCPP_DEBUG_HAS_DEMANGLE 0
#endif

namespace rixlib
{
  struct inspect_context;
  struct inspect_options;

  /// @brief Primary inspector template — specialize to teach inspect about your type.
  template <typename T, typename>
  struct inspector;

  /// @brief Field map template — specialize to register struct fields for reflection.
  template <typename T, typename>
  struct field_map;

  namespace detail
  {
    template <typename T>
    void iwrite(inspect_context &ctx, const T &value);
  } // namespace detail

  namespace demangle
  {
    /**
     * @brief Return a human-readable type name for type T.
     *
     * Uses abi::__cxa_demangle on GCC/Clang. Falls back to typeid().name()
     * on MSVC and other compilers.
     */
    template <typename T>
    [[nodiscard]] std::string type_name()
    {
#if RIXCPP_DEBUG_HAS_DEMANGLE
      int status = 0;
      const char *mangled = typeid(T).name();
      char *demangled = abi::__cxa_demangle(mangled, nullptr, nullptr, &status);
      std::string result = (status == 0 && demangled) ? demangled : mangled;
      std::free(demangled);
      return result;
#else
      return typeid(T).name();
#endif
    }

    /**
     * @brief Return a human-readable type name for the type of a value.
     */
    template <typename T>
    [[nodiscard]] std::string type_name_of(const T &)
    {
      return type_name<T>();
    }

    /**
     * @brief Shorten a fully-qualified type name by removing common namespaces.
     *
     * Strips std::__cxx11::, std::__1::, std::allocator<...> and similar noise.
     */
    [[nodiscard]] inline std::string shorten(std::string name)
    {
      // Remove std::__cxx11:: (GCC inline namespace)
      for (std::string ns : {"std::__cxx11::", "std::__1::", "std::pmr::"})
      {
        std::string::size_type pos;
        while ((pos = name.find(ns)) != std::string::npos)
          name.erase(pos, ns.size());
      }
      // Collapse allocator noise: ", std::allocator<...>"
      auto collapse_alloc = [&](std::string &s)
      {
        std::string tok = ", std::allocator<";
        std::string::size_type p = s.find(tok);
        while (p != std::string::npos)
        {
          // Find matching closing '>'
          int depth = 1;
          std::string::size_type i = p + tok.size();
          while (i < s.size() && depth > 0)
          {
            if (s[i] == '<')
              ++depth;
            else if (s[i] == '>')
              --depth;
            ++i;
          }
          s.erase(p, i - p);
          p = s.find(tok);
        }
      };
      collapse_alloc(name);
      return name;
    }

    /**
     * @brief Return shortened demangled type name for T.
     */
    template <typename T>
    [[nodiscard]] std::string short_type_name()
    {
      return shorten(type_name<T>());
    }

  } // namespace demangle

  /**
   * @brief Complete set of compile-time and runtime metadata for a type T.
   *
   * Gathered once, this struct provides a unified view of a type's properties:
   * size, alignment, qualifiers, category flags, and a human-readable name.
   */
  struct type_metadata
  {
    std::string name;        ///< Shortened demangled type name
    std::string full_name;   ///< Full demangled type name
    std::size_t size_bytes;  ///< sizeof(T)
    std::size_t align_bytes; ///< alignof(T)

    // Category flags
    bool is_void;
    bool is_bool;
    bool is_integral;
    bool is_floating_point;
    bool is_arithmetic;
    bool is_enum;
    bool is_class;
    bool is_union;
    bool is_pointer;
    bool is_lvalue_ref;
    bool is_rvalue_ref;
    bool is_array;
    bool is_function;
    bool is_member_pointer;

    // Qualifier flags
    bool is_const;
    bool is_volatile;

    // Class property flags
    bool is_abstract;
    bool is_polymorphic;
    bool is_final;
    bool is_empty;
    bool is_aggregate;
    bool is_standard_layout;
    bool is_trivially_copyable;
    bool is_trivially_destructible;
    bool is_trivially_constructible;
    bool is_default_constructible;
    bool is_copy_constructible;
    bool is_move_constructible;
    bool is_copy_assignable;
    bool is_move_assignable;
    bool is_destructible;

    // Sign and size
    bool is_signed;
    bool is_unsigned;
  };

  /**
   * @brief Build a type_metadata instance for type T at compile time (constexpr-friendly).
   */
  template <typename T>
  [[nodiscard]] type_metadata make_type_metadata()
  {
    using U = std::remove_cvref_t<T>;
    return type_metadata{
        .name = demangle::short_type_name<U>(),
        .full_name = demangle::type_name<U>(),
        .size_bytes = sizeof(U),
        .align_bytes = alignof(U),
        .is_void = std::is_void_v<U>,
        .is_bool = std::is_same_v<U, bool>,
        .is_integral = std::is_integral_v<U>,
        .is_floating_point = std::is_floating_point_v<U>,
        .is_arithmetic = std::is_arithmetic_v<U>,
        .is_enum = std::is_enum_v<U>,
        .is_class = std::is_class_v<U>,
        .is_union = std::is_union_v<U>,
        .is_pointer = std::is_pointer_v<U>,
        .is_lvalue_ref = std::is_lvalue_reference_v<T>,
        .is_rvalue_ref = std::is_rvalue_reference_v<T>,
        .is_array = std::is_array_v<U>,
        .is_function = std::is_function_v<U>,
        .is_member_pointer = std::is_member_pointer_v<U>,
        .is_const = std::is_const_v<std::remove_reference_t<T>>,
        .is_volatile = std::is_volatile_v<std::remove_reference_t<T>>,
        .is_abstract = std::is_abstract_v<U>,
        .is_polymorphic = std::is_polymorphic_v<U>,
        .is_final = std::is_final_v<U>,
        .is_empty = std::is_empty_v<U>,
        .is_aggregate = std::is_aggregate_v<U>,
        .is_standard_layout = std::is_standard_layout_v<U>,
        .is_trivially_copyable = std::is_trivially_copyable_v<U>,
        .is_trivially_destructible = std::is_trivially_destructible_v<U>,
        .is_trivially_constructible = std::is_trivially_constructible_v<U>,
        .is_default_constructible = std::is_default_constructible_v<U>,
        .is_copy_constructible = std::is_copy_constructible_v<U>,
        .is_move_constructible = std::is_move_constructible_v<U>,
        .is_copy_assignable = std::is_copy_assignable_v<U>,
        .is_move_assignable = std::is_move_assignable_v<U>,
        .is_destructible = std::is_destructible_v<U>,
        .is_signed = std::is_signed_v<U>,
        .is_unsigned = std::is_unsigned_v<U>,
    };
  }

  /**
   * @brief A single registered field descriptor (name + member pointer).
   *
   * @tparam Owner   Struct/class that owns this field.
   * @tparam FieldT  Type of the field.
   */
  template <typename Owner, typename FieldT>
  struct field_descriptor
  {
    std::string_view name;
    FieldT Owner::*ptr;

    constexpr field_descriptor(std::string_view n, FieldT Owner::*p)
        : name(n), ptr(p) {}

    const FieldT &get(const Owner &obj) const { return obj.*ptr; }
  };

  /**
   * @brief Helper function to create a field_descriptor (type-deduced).
   *
   * @example
   *   vix::field("x", &MyStruct::x)
   */
  template <typename Owner, typename FieldT>
  constexpr auto field(std::string_view name, FieldT Owner::*ptr)
      -> field_descriptor<Owner, FieldT>
  {
    return {name, ptr};
  }

  /**
   * @brief Tuple of field descriptors for a given type — returned by field_map<T>::fields().
   */
  template <typename... Fields>
  constexpr auto fields(Fields &&...fs)
  {
    return std::tuple<std::decay_t<Fields>...>(std::forward<Fields>(fs)...);
  }

  /**
   * @brief Options controlling the depth, format, and verbosity of inspection.
   */
  struct inspect_options
  {
    int max_depth = 8;             ///< Maximum nesting depth
    std::size_t max_items = 64;    ///< Max items per container
    bool show_type = true;         ///< Show type annotation
    bool show_meta = false;        ///< Show full type metadata (size, align…)
    bool compact = false;          ///< Single-line compact mode
    bool show_address = false;     ///< Show object address (for pointers)
    std::string indent_str = "  "; ///< Indentation unit
    std::ostream *out = &std::cout;
  };

  inline inspect_options &default_options() noexcept
  {
    static thread_local inspect_options opts;
    return opts;
  }

  /**
   * @brief Mutable context threaded through the recursive inspection engine.
   *
   * Carries output stream, indentation state, depth tracking, and options.
   * All rendering operations emit to ctx.os.
   */
  struct inspect_context
  {
    std::ostream &os;
    int depth = 0;
    inspect_options opts;

    explicit inspect_context(std::ostream &os_,
                             inspect_options o = default_options())
        : os(os_), opts(std::move(o)) {}

    // Indentation helpers
    void indent() const
    {
      for (int i = 0; i < depth; ++i)
        os << opts.indent_str;
    }

    void newline_indent() const
    {
      if (!opts.compact)
      {
        os << '\n';
        indent();
      }
      else
        os << ' ';
    }

    [[nodiscard]] bool at_max_depth() const noexcept
    {
      return depth >= opts.max_depth;
    }

    // Output helpers
    inspect_context child() const
    {
      inspect_context c{os, opts};
      c.depth = depth + 1;
      return c;
    }

    void emit(std::string_view sv) { os << sv; }
    void emit(char c) { os << c; }

    template <typename T>
    void emit_value(const T &v) { os << v; }
  };

  namespace detail
  {

    // Type annotation helper
    template <typename T>
    void emit_type_tag(inspect_context &ctx)
    {
      if (ctx.opts.show_type)
      {
        ctx.os << '<' << demangle::short_type_name<std::remove_cvref_t<T>>() << '>';
      }
    }

    // Bool
    inline void iwrite_bool(inspect_context &ctx, bool v)
    {
      ctx.os << (v ? "true" : "false");
    }

    // Char types
    inline void iwrite_char(inspect_context &ctx, char c)
    {
      ctx.os << '\'' << c << '\'';
    }
    inline void iwrite_char(inspect_context &ctx, signed char c)
    {
      ctx.os << '\'' << static_cast<char>(c) << '\'';
    }
    inline void iwrite_char(inspect_context &ctx, unsigned char c)
    {
      ctx.os << "0x" << std::hex << std::uppercase
             << static_cast<unsigned>(c) << std::dec;
    }
    inline void iwrite_char(inspect_context &ctx, wchar_t c)
    {
      ctx.os << "L'\\u" << std::hex << std::uppercase
             << static_cast<unsigned>(c) << std::dec << "'";
    }
    inline void iwrite_char(inspect_context &ctx, char8_t c)
    {
      ctx.os << "u8'" << static_cast<char>(c) << '\'';
    }
    inline void iwrite_char(inspect_context &ctx, char16_t c)
    {
      ctx.os << "u'\\u" << std::hex << std::uppercase
             << static_cast<unsigned>(c) << std::dec << "'";
    }
    inline void iwrite_char(inspect_context &ctx, char32_t c)
    {
      ctx.os << "U'\\U" << std::hex << std::uppercase
             << static_cast<unsigned long>(c) << std::dec << "'";
    }

    // Nullptr
    inline void iwrite_nullptr(inspect_context &ctx)
    {
      ctx.os << "nullptr";
    }

    // 8.5  String
    inline void iwrite_string(inspect_context &ctx, std::string_view sv)
    {
      ctx.os << '"' << sv << '"';
      if (ctx.opts.show_type)
      {
        ctx.os << " [len=" << sv.size() << ']';
      }
    }

    inline void iwrite_wstring(inspect_context &ctx, std::wstring_view wsv)
    {
      ctx.os << "L\"";
      for (wchar_t wc : wsv)
      {
        if (wc < 0x80)
          ctx.os << static_cast<char>(wc);
        else
          ctx.os << "\\u" << std::hex << std::uppercase
                 << static_cast<unsigned>(wc) << std::dec;
      }
      ctx.os << '"';
      if (ctx.opts.show_type)
        ctx.os << " [len=" << wsv.size() << ']';
    }

    // 8.6  Pointer
    template <typename T>
    void iwrite_pointer(inspect_context &ctx, const T *ptr)
    {
      if (!ptr)
      {
        ctx.os << "nullptr";
        return;
      }
      ctx.os << "<ptr:0x"
             << std::hex << std::uppercase
             << reinterpret_cast<std::uintptr_t>(ptr)
             << std::dec << '>';
      if (ctx.opts.show_address)
      {
        // Attempt to dereference and inspect the pointed-to value
        if (!ctx.at_max_depth())
        {
          ctx.os << " -> ";
          auto child = ctx.child();
          iwrite(child, *ptr);
        }
      }
    }

    // Enum
    template <typename E>
      requires std::is_enum_v<E>
    void iwrite_enum(inspect_context &ctx, E e)
    {
      ctx.os << static_cast<std::underlying_type_t<E>>(e);
      if (ctx.opts.show_type)
      {
        ctx.os << " [enum:"
               << demangle::short_type_name<E>()
               << ']';
      }
    }

    // Duration
    template <typename Rep, typename Period>
    void iwrite_duration(inspect_context &ctx,
                         const std::chrono::duration<Rep, Period> &d)
    {
      using namespace std::chrono;
      if constexpr (std::is_same_v<Period, std::nano>)
        ctx.os << d.count() << "ns";
      else if constexpr (std::is_same_v<Period, std::micro>)
        ctx.os << d.count() << "µs";
      else if constexpr (std::is_same_v<Period, std::milli>)
        ctx.os << d.count() << "ms";
      else if constexpr (std::ratio_equal_v<Period, std::ratio<1>>)
        ctx.os << d.count() << "s";
      else if constexpr (std::ratio_equal_v<Period, std::ratio<60>>)
        ctx.os << d.count() << "min";
      else if constexpr (std::ratio_equal_v<Period, std::ratio<3600>>)
        ctx.os << d.count() << "h";
      else
        ctx.os << d.count() << " [" << Period::num << '/' << Period::den << "]s";
    }

    // time_point
    template <typename Clock, typename Dur>
    void iwrite_time_point(inspect_context &ctx,
                           const std::chrono::time_point<Clock, Dur> &tp)
    {
      ctx.os << "<time_point: ";
      iwrite_duration(ctx, tp.time_since_epoch());
      ctx.os << " from epoch>";
    }

    // filesystem::path
    inline void iwrite_fs_path(inspect_context &ctx,
                               const std::filesystem::path &p)
    {
      ctx.os << "path(\"" << p.string() << "\")";
      if (ctx.opts.show_type)
      {
        ctx.os << " [exists=" << std::boolalpha
               << std::filesystem::exists(p) << "]";
      }
    }

    // Optional
    template <typename T>
    void iwrite_optional(inspect_context &ctx, const std::optional<T> &opt)
    {
      if (!opt.has_value())
      {
        ctx.os << "None";
        if (ctx.opts.show_type)
          ctx.os << '<' << demangle::short_type_name<T>() << '>';
        return;
      }
      ctx.os << "Some(";
      auto child = ctx.child();
      iwrite(child, *opt);
      ctx.os << ')';
    }

    // Variant
    template <typename... Ts>
    void iwrite_variant(inspect_context &ctx,
                        const std::variant<Ts...> &var)
    {
      ctx.os << "variant<index=" << var.index() << ">(";
      std::visit([&ctx](const auto &v)
                 {
        auto child = ctx.child();
        iwrite(child, v); }, var);
      ctx.os << ')';
    }

    // 8.13  std::any
    inline void iwrite_any(inspect_context &ctx, const std::any &a)
    {
      if (!a.has_value())
      {
        ctx.os << "any(empty)";
      }
      else
      {
#if RIXCPP_DEBUG_HAS_DEMANGLE
        int status = 0;
        const char *mangled = a.type().name();
        char *d = abi::__cxa_demangle(mangled, nullptr, nullptr, &status);
        ctx.os << "any(" << (status == 0 && d ? d : mangled) << ')';
        std::free(d);
#else
        ctx.os << "any(" << a.type().name() << ')';
#endif
      }
    }

    // reference_wrapper
    template <typename T>
    void iwrite_ref_wrapper(inspect_context &ctx,
                            const std::reference_wrapper<T> &rw)
    {
      ctx.os << "ref(";
      auto child = ctx.child();
      iwrite(child, rw.get());
      ctx.os << ')';
    }

    // Smart pointers
    template <typename T, typename D>
    void iwrite_unique(inspect_context &ctx, const std::unique_ptr<T, D> &p)
    {
      if (!p)
      {
        ctx.os << "unique_ptr(null)";
        return;
      }
      ctx.os << "unique_ptr(";
      auto child = ctx.child();
      iwrite(child, *p);
      ctx.os << ')';
    }

    template <typename T>
    void iwrite_shared(inspect_context &ctx, const std::shared_ptr<T> &p)
    {
      if (!p)
      {
        ctx.os << "shared_ptr(null)";
        return;
      }
      ctx.os << "shared_ptr[use_count=" << p.use_count() << "](";
      auto child = ctx.child();
      iwrite(child, *p);
      ctx.os << ')';
    }

    template <typename T>
    void iwrite_weak(inspect_context &ctx, const std::weak_ptr<T> &p)
    {
      if (p.expired())
      {
        ctx.os << "weak_ptr(expired)";
        return;
      }
      auto sp = p.lock();
      ctx.os << "weak_ptr[use_count=" << sp.use_count() << "](";
      auto child = ctx.child();
      iwrite(child, *sp);
      ctx.os << ')';
    }

    // Container adapter helpers (copy-drain)
    template <typename T, typename C>
    void iwrite_stack(inspect_context &ctx, std::stack<T, C> s, std::size_t max)
    {
      std::vector<T> items;
      items.reserve(s.size());
      while (!s.empty())
      {
        items.push_back(s.top());
        s.pop();
      }
      std::reverse(items.begin(), items.end());
      ctx.os << "stack[";
      std::size_t count = 0;
      bool first = true;
      for (const auto &v : items)
      {
        if (count++ >= max)
        {
          ctx.os << ", ...";
          break;
        }
        if (!first)
          ctx.os << ", ";
        first = false;
        auto child = ctx.child();
        iwrite(child, v);
      }
      ctx.os << ']';
    }

    template <typename T, typename C>
    void iwrite_queue(inspect_context &ctx, std::queue<T, C> q, std::size_t max)
    {
      ctx.os << "queue[";
      std::size_t count = 0;
      bool first = true;
      while (!q.empty())
      {
        if (count++ >= max)
        {
          ctx.os << ", ...";
          break;
        }
        if (!first)
          ctx.os << ", ";
        first = false;
        auto child = ctx.child();
        iwrite(child, q.front());
        q.pop();
      }
      ctx.os << ']';
    }

    template <typename T, typename C, typename Cmp>
    void iwrite_pqueue(inspect_context &ctx,
                       std::priority_queue<T, C, Cmp> pq, std::size_t max)
    {
      ctx.os << "priority_queue[";
      std::size_t count = 0;
      bool first = true;
      while (!pq.empty())
      {
        if (count++ >= max)
        {
          ctx.os << ", ...";
          break;
        }
        if (!first)
          ctx.os << ", ";
        first = false;
        auto child = ctx.child();
        iwrite(child, pq.top());
        pq.pop();
      }
      ctx.os << ']';
    }

    // Range rendering
    template <typename Range>
    void iwrite_range(inspect_context &ctx, const Range &rng,
                      char open, char close)
    {
      ctx.os << open;
      std::size_t count = 0;
      bool first = true;
      for (const auto &elem : rng)
      {
        if (count >= ctx.opts.max_items)
        {
          ctx.os << ", ...";
          break;
        }
        if (!first)
          ctx.os << ", ";
        first = false;
        if constexpr (traits::is_map_like_v<Range>)
        {
          auto kctx = ctx.child();
          iwrite(kctx, elem.first);
          ctx.os << ": ";
          auto vctx = ctx.child();
          iwrite(vctx, elem.second);
        }
        else
        {
          auto child = ctx.child();
          iwrite(child, elem);
        }
        ++count;
      }
      ctx.os << close;
      if (ctx.opts.show_type)
      {
        // Show size if available via std::size or distance
        if constexpr (requires { std::size(rng); })
        {
          ctx.os << " [n=" << std::size(rng) << ']';
        }
      }
    }

    // Tuple / pair
    template <typename Tuple, std::size_t... Is>
    void iwrite_tuple_elems(inspect_context &ctx, const Tuple &t,
                            std::index_sequence<Is...>)
    {
      ((
           [&]()
           {
             if constexpr (Is > 0)
               ctx.os << ", ";
             auto child = ctx.child();
             iwrite(child, std::get<Is>(t));
           }()),
       ...);
    }

    template <typename Tuple>
    void iwrite_tuple(inspect_context &ctx, const Tuple &t)
    {
      ctx.os << '(';
      iwrite_tuple_elems(ctx, t, std::make_index_sequence<std::tuple_size_v<Tuple>>{});
      ctx.os << ')';
    }

    template <typename A, typename B>
    void iwrite_pair(inspect_context &ctx, const std::pair<A, B> &p)
    {
      ctx.os << '(';
      auto fst = ctx.child();
      iwrite(fst, p.first);
      ctx.os << ": ";
      auto snd = ctx.child();
      iwrite(snd, p.second);
      ctx.os << ')';
    }

    // Field-map based struct rendering
    template <typename T, typename Fields, std::size_t... Is>
    void iwrite_fields(inspect_context &ctx, const T &obj,
                       const Fields &fmap,
                       std::index_sequence<Is...>)
    {
      ((
           [&]()
           {
             if constexpr (Is > 0)
             {
               ctx.os << ',';
               ctx.newline_indent();
             }
             const auto &fd = std::get<Is>(fmap);
             ctx.os << fd.name << ": ";
             auto child = ctx.child();
             iwrite(child, fd.get(obj));
           }()),
       ...);
    }

    template <typename T>
      requires traits::has_field_map_v<T>
    void iwrite_struct(inspect_context &ctx, const T &obj)
    {
      const auto fmap = field_map<T>::fields();
      constexpr std::size_t N = std::tuple_size_v<std::decay_t<decltype(fmap)>>;
      ctx.os << demangle::short_type_name<T>() << " {";
      if (!ctx.opts.compact)
      {
        ctx.os << '\n';
        auto child = ctx.child();
        child.indent();
        iwrite_fields(child, obj, fmap, std::make_index_sequence<N>{});
        ctx.os << '\n';
        ctx.indent();
      }
      else
      {
        iwrite_fields(ctx, obj, fmap, std::make_index_sequence<N>{});
      }
      ctx.os << '}';
    }

#if RIXCPP_DEBUG_HAS_EXPECTED
    // std::expected
    template <typename T, typename E>
    void iwrite_expected(inspect_context &ctx, const std::expected<T, E> &exp)
    {
      if (exp.has_value())
      {
        ctx.os << "Ok(";
        auto child = ctx.child();
        iwrite(child, *exp);
        ctx.os << ')';
      }
      else
      {
        ctx.os << "Err(";
        auto child = ctx.child();
        iwrite(child, exp.error());
        ctx.os << ')';
      }
    }
#endif

    /**
     * @brief Central dispatch for the inspection engine.
     *
     * Priority:
     *  1.  rixlib::inspector<T> specialization
     *  2.  ADL rix_inspect hook
     *  3.  field_map<T> struct reflection
     *  4.  nullptr_t
     *  5.  bool
     *  6.  char types
     *  7.  wstring / wstring_view
     *  8.  string-like
     *  9.  filesystem::path
     * 10.  std::any
     * 11.  std::optional
     * 12.  std::variant
     * 13.  std::reference_wrapper
     * 14.  chrono duration
     * 15.  chrono time_point
     * 16.  std::unique_ptr
     * 17.  std::shared_ptr
     * 18.  std::weak_ptr
     * 19.  Container adapters (stack / queue / priority_queue)
     * 20.  std::expected (C++23)
     * 21.  enum
     * 22.  map-like range
     * 23.  other range (vector, list, set, …)
     * 24.  pair
     * 25.  tuple-like
     * 26.  raw pointer (dereferences if show_address)
     * 27.  streamable via operator<<
     * 28.  fallback: unprintable
     */
    template <typename T>
    void iwrite(inspect_context &ctx, const T &value)
    {
      using U = std::remove_cvref_t<T>;

      if (ctx.at_max_depth())
      {
        ctx.os << "<depth-limit>";
        return;
      }

      // inspector<T> specialization
      if constexpr (traits::has_inspector_v<U>)
      {
        inspector<U>::inspect(ctx, value);
      }
      // ADL rix_inspect hook
      else if constexpr (traits::has_rix_inspect_hook_v<U>)
      {
        rix_inspect(ctx, value);
      }
      // field_map struct reflection
      else if constexpr (traits::has_field_map_v<U>)
      {
        iwrite_struct(ctx, value);
      }
      // nullptr_t
      else if constexpr (traits::is_nullptr_v<U>)
      {
        iwrite_nullptr(ctx);
      }
      // bool
      else if constexpr (traits::is_bool_v<U>)
      {
        iwrite_bool(ctx, value);
      }
      // char types
      else if constexpr (traits::is_char_v<U>)
      {
        iwrite_char(ctx, value);
      }
      // wstring
      else if constexpr (std::is_same_v<U, std::wstring> ||
                         std::is_same_v<U, std::wstring_view>)
      {
        iwrite_wstring(ctx, value);
      }
      else if constexpr (std::is_same_v<U, const wchar_t *> ||
                         std::is_same_v<U, wchar_t *>)
      {
        if (value == nullptr)
          iwrite_nullptr(ctx);
        else
          iwrite_wstring(ctx, std::wstring_view{value});
      }
      // string-like
      else if constexpr (traits::is_string_like_v<U>)
      {
        iwrite_string(ctx, value);
      }
      // filesystem::path
      else if constexpr (traits::is_fs_path_v<U>)
      {
        iwrite_fs_path(ctx, value);
      }
      // std::any
      else if constexpr (traits::is_any_v<U>)
      {
        iwrite_any(ctx, value);
      }
      // std::optional
      else if constexpr (traits::is_optional_v<U>)
      {
        iwrite_optional(ctx, value);
      }
      // std::variant
      else if constexpr (traits::is_variant_v<U>)
      {
        iwrite_variant(ctx, value);
      }
      // reference_wrapper
      else if constexpr (traits::is_ref_wrapper_v<U>)
      {
        iwrite_ref_wrapper(ctx, value);
      }
      // chrono duration
      else if constexpr (traits::is_duration_v<U>)
      {
        iwrite_duration(ctx, value);
      }
      // chrono time_point
      else if constexpr (traits::is_time_point_v<U>)
      {
        iwrite_time_point(ctx, value);
      }
      // unique_ptr
      else if constexpr (traits::is_unique_ptr<U>::value)
      {
        iwrite_unique(ctx, value);
      }
      // shared_ptr
      else if constexpr (traits::is_shared_ptr<U>::value)
      {
        iwrite_shared(ctx, value);
      }
      // weak_ptr
      else if constexpr (traits::is_weak_ptr<U>::value)
      {
        iwrite_weak(ctx, value);
      }
      // Container adapters
      else if constexpr (traits::is_stack<U>::value)
      {
        iwrite_stack(ctx, value, ctx.opts.max_items);
      }
      else if constexpr (traits::is_queue<U>::value)
      {
        iwrite_queue(ctx, value, ctx.opts.max_items);
      }
      else if constexpr (traits::is_priority_queue<U>::value)
      {
        iwrite_pqueue(ctx, value, ctx.opts.max_items);
      }
#if RIXCPP_DEBUG_HAS_EXPECTED
      // std::expected
      else if constexpr (traits::is_expected_v<U>)
      {
        iwrite_expected(ctx, value);
      }
#endif
      // enum
      else if constexpr (std::is_enum_v<U>)
      {
        iwrite_enum(ctx, value);
      }
      // map-like range
      else if constexpr (traits::is_map_like_v<U> && traits::is_range_v<U>)
      {
        iwrite_range(ctx, value, '{', '}');
      }
      // sequence range
      else if constexpr (traits::is_range_v<U>)
      {
        iwrite_range(ctx, value, '[', ']');
      }
      // pair
      else if constexpr (traits::is_pair_v<U>)
      {
        iwrite_pair(ctx, value);
      }
      // tuple-like
      else if constexpr (traits::is_tuple_like_v<U>)
      {
        iwrite_tuple(ctx, value);
      }
      // raw pointer
      else if constexpr (std::is_pointer_v<U> && !traits::is_string_like_v<U>)
      {
        if constexpr (std::is_function_v<std::remove_pointer_t<U>>)
        {
          ctx.os << "<fptr:0x" << std::hex << std::uppercase
                 << reinterpret_cast<std::uintptr_t>(
                        reinterpret_cast<const void *>(value))
                 << std::dec << '>';
        }
        else
        {
          iwrite_pointer(ctx, value);
        }
      }
      // streamable via operator<<
      else if constexpr (traits::is_ostreamable_v<U>)
      {
        ctx.os << value;
      }
      // fallback
      else
      {
        ctx.os << "<uninspectable:" << typeid(U).name() << '>';
      }
    }

  } // namespace detail

  namespace detail
  {

    /**
     * @brief Emit a full type_metadata block to the context stream.
     */
    inline void render_metadata(inspect_context &ctx, const type_metadata &m)
    {
      ctx.os << "type_info {\n";
      auto ind = [&](std::string_view key, auto val)
      {
        ctx.os << ctx.opts.indent_str << "  "
               << key << ": " << val << '\n';
      };
      auto indb = [&](std::string_view key, bool val)
      {
        if (val)
          ctx.os << ctx.opts.indent_str << "  "
                 << key << ": true\n";
      };

      ind("name", m.name);
      ind("full_name", m.full_name);
      ind("size", std::to_string(m.size_bytes) + " bytes");
      ind("align", std::to_string(m.align_bytes) + " bytes");

      ctx.os << ctx.opts.indent_str << "  categories: [";
      bool first = true;
      auto cat = [&](std::string_view s, bool v)
      {
        if (!v)
          return;
        if (!first)
          ctx.os << ", ";
        first = false;
        ctx.os << s;
      };
      cat("void", m.is_void);
      cat("bool", m.is_bool);
      cat("integral", m.is_integral);
      cat("float", m.is_floating_point);
      cat("enum", m.is_enum);
      cat("class", m.is_class);
      cat("union", m.is_union);
      cat("pointer", m.is_pointer);
      cat("array", m.is_array);
      cat("function", m.is_function);
      cat("const", m.is_const);
      cat("volatile", m.is_volatile);
      ctx.os << "]\n";

      ctx.os << ctx.opts.indent_str << "  traits: [";
      first = true;
      auto tr = [&](std::string_view s, bool v)
      {
        if (!v)
          return;
        if (!first)
          ctx.os << ", ";
        first = false;
        ctx.os << s;
      };
      tr("abstract", m.is_abstract);
      tr("polymorphic", m.is_polymorphic);
      tr("final", m.is_final);
      tr("empty", m.is_empty);
      tr("aggregate", m.is_aggregate);
      tr("standard_layout", m.is_standard_layout);
      tr("trivially_copyable", m.is_trivially_copyable);
      tr("trivially_destructible", m.is_trivially_destructible);
      tr("default_constructible", m.is_default_constructible);
      tr("copy_constructible", m.is_copy_constructible);
      tr("move_constructible", m.is_move_constructible);
      tr("copy_assignable", m.is_copy_assignable);
      tr("move_assignable", m.is_move_assignable);
      ctx.os << "]\n";

      ctx.os << "}";
    }

  } // namespace detail

  /**
   * @brief Inspect a value and write its representation to stdout.
   *
   * Uses the default inspect_options. Every supported type is rendered
   * with contextual information: value, type name (if show_type), size, etc.
   *
   * @param value  Any value of a supported type.
   *
   * @example
   *   rixlib::inspect(42);
   *   rixlib::inspect(std::vector{1, 2, 3});
   *   rixlib::inspect(std::make_optional(3.14));
   */
  template <typename T>
  void inspect(const T &value)
  {
    inspect_context ctx{*default_options().out};
    detail::iwrite(ctx, value);
    ctx.os << '\n';
  }

  /**
   * @brief Inspect a value with custom options.
   * @param value  Value to inspect.
   * @param opts   Custom inspect_options.
   */
  template <typename T>
  void inspect(const T &value, const inspect_options &opts)
  {
    inspect_context ctx{*opts.out, opts};
    detail::iwrite(ctx, value);
    ctx.os << '\n';
  }

  /**
   * @brief Inspect a value into a specific ostream.
   * @param os     Destination stream.
   * @param value  Value to inspect.
   */
  template <typename T>
  void inspect_to(std::ostream &os, const T &value)
  {
    inspect_options opts = default_options();
    opts.out = &os;
    inspect_context ctx{os, opts};
    detail::iwrite(ctx, value);
    ctx.os << '\n';
  }

  // inspect_to_string — capture as std::string

  /**
   * @brief Inspect a value and return its representation as a std::string.
   * @param value  Any supported value.
   * @return       Inspection string (no trailing newline).
   */
  template <typename T>
  [[nodiscard]] std::string inspect_to_string(const T &value)
  {
    std::ostringstream oss;
    inspect_options opts = default_options();
    opts.out = &oss;
    inspect_context ctx{oss, opts};
    detail::iwrite(ctx, value);
    return oss.str();
  }

  /**
   * @brief Inspect a value to string with custom options.
   */
  template <typename T>
  [[nodiscard]] std::string inspect_to_string(const T &value,
                                              const inspect_options &opts)
  {
    std::ostringstream oss;
    inspect_options o = opts;
    o.out = &oss;
    inspect_context ctx{oss, o};
    detail::iwrite(ctx, value);
    return oss.str();
  }

  /**
   * @brief Inspect the type T, printing its full metadata report.
   *
   * Does not require a value. Reports size, alignment, category flags,
   * trait flags, and the demangled type name.
   *
   * @example
   *   rixlib::inspect_type<std::vector<int>>();
   *   rixlib::inspect_type<MyStruct>();
   */
  template <typename T>
  void inspect_type()
  {
    auto &os = *default_options().out;
    type_metadata m = make_type_metadata<T>();
    inspect_context ctx{os};
    detail::render_metadata(ctx, m);
    os << '\n';
  }

  /**
   * @brief Inspect the type of a value (deduced, no explicit template arg needed).
   * @param value  Value whose type will be inspected.
   */
  template <typename T>
  void inspect_type(const T &)
  {
    inspect_type<std::remove_cvref_t<T>>();
  }

  /**
   * @brief Inspect multiple values on a single line, separated by " | ".
   *
   * @example
   *   rixlib::inspect_line(x, y, z);
   *   // x_value | y_value | z_value
   */
  template <typename... Args>
  void inspect_line(const Args &...args)
  {
    auto &os = *default_options().out;
    bool first = true;
    ([&]()
     {
        if (!first) os << " | ";
        first = false;
        inspect_context ctx{os};
        detail::iwrite(ctx, args); }(), ...);
    os << '\n';
  }

  /**
   * @brief Inspect a named value: "label: <value>".
   *
   * @example
   *   rixlib::inspect_value("batch_size", config.batch_size);
   */
  template <typename T>
  void inspect_value(std::string_view label, const T &value)
  {
    auto &os = *default_options().out;
    os << label << ": ";
    inspect_context ctx{os};
    detail::iwrite(ctx, value);
    os << '\n';
  }

  /**
   * @brief Inspect multiple values, each on its own line with a type tag.
   */
  template <typename... Args>
  void inspect_all(const Args &...args)
  {
    inspect_options opts = default_options();
    opts.show_type = true;
    ([&]()
     {
        inspect_context ctx{*opts.out, opts};
        detail::iwrite(ctx, args);
        ctx.os << '\n'; }(), ...);
  }

  /**
   * @brief Inspect a value AND its full type metadata.
   *
   * Shows the value representation followed by a complete type_metadata block.
   */
  template <typename T>
  void inspect_meta(const T &value)
  {
    auto &os = *default_options().out;
    // Value line
    os << "value: ";
    inspect_context vctx{os};
    detail::iwrite(vctx, value);
    os << '\n';
    // Type metadata
    auto m = make_type_metadata<T>();
    inspect_context mctx{os};
    detail::render_metadata(mctx, m);
    os << '\n';
  }

  /**
   * @brief Inspect in compact mode: single line, no type annotations.
   */
  template <typename T>
  [[nodiscard]] std::string inspect_compact(const T &value)
  {
    inspect_options opts;
    opts.compact = true;
    opts.show_type = false;
    opts.show_meta = false;
    return inspect_to_string(value, opts);
  }

  /**
   * @brief Inspect in verbose mode: type tags, metadata, show addresses.
   */
  template <typename T>
  void inspect_verbose(const T &value)
  {
    inspect_options opts = default_options();
    opts.show_type = true;
    opts.show_meta = true;
    opts.show_address = true;
    inspect(value, opts);
  }

  /**
   * @brief RAII guard that temporarily overrides the default inspect_options.
   *
   * @example
   *   {
   *       rixlib::scoped_inspect_options g{};
   *       g.opts.show_type = true;
   *       g.opts.compact = true;
   *       rixlib::inspect(my_object);
   *   }
   *   // original options restored
   */
  class scoped_inspect_options
  {
  public:
    inspect_options opts;

    explicit scoped_inspect_options(inspect_options override = default_options())
        : opts(std::move(override)), saved_(default_options())
    {
      default_options() = opts;
    }

    ~scoped_inspect_options()
    {
      default_options() = saved_;
    }

    scoped_inspect_options(const scoped_inspect_options &) = delete;
    scoped_inspect_options &operator=(const scoped_inspect_options &) = delete;

  private:
    inspect_options saved_;
  };

  /// @brief Base for inspector specializations that delegate to operator<<.
  template <typename T>
  struct streamable_inspector
  {
    static void inspect(inspect_context &ctx, const T &v)
    {
      ctx.os << v;
    }
  };

  /// @brief Inspector for std::monostate.
  template <>
  struct inspector<std::monostate>
  {
    static void inspect(inspect_context &ctx, const std::monostate &)
    {
      ctx.os << "monostate";
    }
  };

  /// @brief Inspector for std::byte.
  template <>
  struct inspector<std::byte>
  {
    static void inspect(inspect_context &ctx, const std::byte &b)
    {
      ctx.os << "0x" << std::hex << std::uppercase
             << static_cast<unsigned>(b) << std::dec;
      if (ctx.opts.show_type)
        ctx.os << " [byte]";
    }
  };

  /// @brief Inspector for std::error_code.
  template <>
  struct inspector<std::error_code>
  {
    static void inspect(inspect_context &ctx, const std::error_code &ec)
    {
      ctx.os << "error_code(" << ec.value()
             << ", \"" << ec.message() << "\")";
    }
  };

  /// @brief Inspector for std::error_condition.
  template <>
  struct inspector<std::error_condition>
  {
    static void inspect(inspect_context &ctx, const std::error_condition &ec)
    {
      ctx.os << "error_condition(" << ec.value()
             << ", \"" << ec.message() << "\")";
    }
  };

  /**
   * @brief Return a string describing which rendering path would be used for T.
   */
  template <typename T>
  [[nodiscard]] std::string_view inspect_path() noexcept
  {
    using U = std::remove_cvref_t<T>;
    if constexpr (traits::has_inspector_v<U>)
      return "inspector<T>";
    else if constexpr (traits::has_rix_inspect_hook_v<U>)
      return "ADL rix_inspect";
    else if constexpr (traits::has_field_map_v<U>)
      return "field_map<T>";
    else if constexpr (traits::is_nullptr_v<U>)
      return "nullptr_t";
    else if constexpr (traits::is_bool_v<U>)
      return "bool";
    else if constexpr (traits::is_char_v<U>)
      return "char type";
    else if constexpr (traits::is_string_like_v<U>)
      return "string-like";
    else if constexpr (traits::is_fs_path_v<U>)
      return "filesystem::path";
    else if constexpr (traits::is_any_v<U>)
      return "std::any";
    else if constexpr (traits::is_optional_v<U>)
      return "std::optional";
    else if constexpr (traits::is_variant_v<U>)
      return "std::variant";
    else if constexpr (traits::is_ref_wrapper_v<U>)
      return "reference_wrapper";
    else if constexpr (traits::is_duration_v<U>)
      return "chrono::duration";
    else if constexpr (traits::is_time_point_v<U>)
      return "chrono::time_point";
    else if constexpr (traits::is_unique_ptr<U>::value)
      return "unique_ptr";
    else if constexpr (traits::is_shared_ptr<U>::value)
      return "shared_ptr";
    else if constexpr (traits::is_weak_ptr<U>::value)
      return "weak_ptr";
    else if constexpr (traits::is_stack<U>::value)
      return "std::stack";
    else if constexpr (traits::is_queue<U>::value)
      return "std::queue";
    else if constexpr (traits::is_priority_queue<U>::value)
      return "priority_queue";
#if RIXCPP_DEBUG_HAS_EXPECTED
    else if constexpr (traits::is_expected_v<U>)
      return "std::expected";
#endif
    else if constexpr (std::is_enum_v<U>)
      return "enum";
    else if constexpr (traits::is_map_like_v<U>)
      return "map-like range";
    else if constexpr (traits::is_range_v<U>)
      return "range";
    else if constexpr (traits::is_pair_v<U>)
      return "std::pair";
    else if constexpr (traits::is_tuple_like_v<U>)
      return "tuple-like";
    else if constexpr (std::is_pointer_v<U>)
      return "raw pointer";
    else if constexpr (traits::is_ostreamable_v<U>)
      return "operator<<";
    else
      return "fallback";
  }

  /**
   * @brief Print the rendering path for each type in a pack.
   */
  template <typename... Ts>
  void inspect_paths()
  {
    auto &os = *default_options().out;
    ((os << demangle::short_type_name<Ts>()
         << "  =>  " << inspect_path<Ts>() << '\n'),
     ...);
  }

  /**
   * @brief Print a detailed inspection report for a struct with a field_map.
   *
   * Produces a multi-line report with type metadata and field-by-field values.
   */
  template <typename T>
    requires traits::has_field_map_v<T>
  void inspect_report(const T &obj)
  {
    auto &os = *default_options().out;
    const auto m = make_type_metadata<T>();

    os << "══ Inspect Report: " << m.name << " ══\n";
    os << "  sizeof:  " << m.size_bytes << " bytes\n";
    os << "  alignof: " << m.align_bytes << " bytes\n";
    os << "  layout:  "
       << (m.is_standard_layout ? "standard" : "non-standard") << '\n';
    os << "  trivial: "
       << (m.is_trivially_copyable ? "yes" : "no") << '\n';

    os << "  fields:\n";
    const auto fmap = field_map<T>::fields();
    constexpr std::size_t N =
        std::tuple_size_v<std::decay_t<decltype(fmap)>>;

    auto print_field = [&os](const auto &fd, const T &o)
    {
      os << "    " << fd.name << ": ";
      inspect_options opts = default_options();
      opts.show_type = true;
      inspect_context ctx{os, opts};
      detail::iwrite(ctx, fd.get(o));
      os << '\n';
    };

    [&]<std::size_t... Is>(std::index_sequence<Is...>)
    {
      (print_field(std::get<Is>(fmap), obj), ...);
    }(std::make_index_sequence<N>{});

    os << "══════════════════════════════\n";
  }

  /**
   * @brief Print a size/capacity/type report for a sequence container.
   */
  template <typename T>
    requires traits::is_range_v<T>
  void inspect_container(const T &c)
  {
    auto &os = *default_options().out;
    os << demangle::short_type_name<T>() << " {\n";
    os << "  size:    ";

    if constexpr (requires { c.size(); })
      os << c.size();
    else
      os << std::distance(std::begin(c), std::end(c));
    os << '\n';

    if constexpr (requires { c.capacity(); })
      os << "  capacity: " << c.capacity() << '\n';

    if constexpr (requires { c.bucket_count(); })
      os << "  buckets:  " << c.bucket_count() << '\n';

    if constexpr (requires { c.max_size(); })
      os << "  max_size: " << c.max_size() << '\n';

    if constexpr (requires { c.empty(); })
      os << "  empty:    " << std::boolalpha << c.empty() << '\n';

    // Value type metadata
    using V = std::remove_cvref_t<decltype(*std::begin(c))>;
    os << "  value_type: " << demangle::short_type_name<V>() << '\n';
    os << "  value_size: " << sizeof(V) << " bytes\n";

    os << "  elements: ";
    inspect_options opts = default_options();
    opts.show_type = false;
    inspect_context ctx{os, opts};
    detail::iwrite_range(ctx, c,
                         traits::is_map_like_v<T> ? '{' : '[',
                         traits::is_map_like_v<T> ? '}' : ']');
    os << "\n}\n";
  }

  /**
   * @brief Print a hex dump of the raw bytes of a trivially-copyable value.
   *
   * Only available for trivially copyable types.
   * @param value   Value to dump.
   * @param label   Optional label.
   */
  template <typename T>
    requires std::is_trivially_copyable_v<T>
  void inspect_bytes(const T &value, std::string_view label = "")
  {
    auto &os = *default_options().out;
    const std::size_t n = sizeof(T);
    const auto *bytes = reinterpret_cast<const unsigned char *>(&value);

    if (!label.empty())
      os << label << " - ";
    os << demangle::short_type_name<T>()
       << " [" << n << " bytes @ 0x"
       << std::hex << std::uppercase
       << reinterpret_cast<std::uintptr_t>(&value)
       << std::dec << "]:\n  ";

    for (std::size_t i = 0; i < n; ++i)
    {
      os << std::hex << std::uppercase
         << std::setfill('0') << std::setw(2)
         << static_cast<unsigned>(bytes[i]) << std::dec;
      if (i + 1 < n)
        os << (((i + 1) % 8 == 0) ? "\n  " : " ");
    }
    os << '\n';
  }

  /**
   * @brief Inspect two values side-by-side for comparison.
   */
  template <typename A, typename B>
  void inspect_diff(const A &a, const B &b,
                    std::string_view label_a = "a",
                    std::string_view label_b = "b")
  {
    auto &os = *default_options().out;
    os << label_a << ": ";
    inspect_context ca{os};
    detail::iwrite(ca, a);
    os << '\n';
    os << label_b << ": ";
    inspect_context cb{os};
    detail::iwrite(cb, b);
    os << '\n';
    os << "types_equal: " << std::boolalpha
       << std::is_same_v<std::remove_cvref_t<A>, std::remove_cvref_t<B>> << '\n';
    if constexpr (requires { a == b; })
    {
      os << "values_equal: " << (a == b) << '\n';
    }
  }

  /**
   * @brief Inspect a check: compare expected vs actual and label PASS/FAIL.
   */
  template <typename Expected, typename Actual>
  bool inspect_check(std::string_view label,
                     const Expected &expected,
                     const Actual &actual)
  {
    auto &os = *default_options().out;
    bool ok = false;
    if constexpr (requires { actual == expected; })
    {
      ok = (actual == expected);
    }
    os << (ok ? "[PASS] " : "[FAIL] ") << label << '\n';
    if (!ok)
    {
      os << "  expected: ";
      inspect_context ce{os};
      detail::iwrite(ce, expected);
      os << '\n';
      os << "  actual:   ";
      inspect_context ca{os};
      detail::iwrite(ca, actual);
      os << '\n';
    }
    return ok;
  }

  /**
   * @brief Inspect a value mid-expression and return it unchanged.
   *
   * @example
   *   auto result = rixlib::inspect_tap(compute());  // prints and returns
   */
  template <typename T>
  const T &inspect_tap(const T &value, std::string_view label = "")
  {
    auto &os = *default_options().out;
    if (!label.empty())
      os << label << ": ";
    inspect_context ctx{os};
    detail::iwrite(ctx, value);
    os << '\n';
    return value;
  }

  /**
   * @brief Conditionally inspect: only outputs if condition is true.
   */
  template <typename T>
  void inspect_if(bool condition, const T &value,
                  std::string_view label = "")
  {
    if (condition)
    {
      if (!label.empty())
        inspect_value(label, value);
      else
        inspect(value);
    }
  }

  /**
   * @brief Inspect a value inside a lambda, then return the original value.
   *
   * @example
   *   auto v = rixlib::tap_with(compute(), [](const auto& x){
   *       rixlib::inspect_value("intermediate", x);
   *   });
   */
  template <typename T, typename Fn>
  const T &tap_with(const T &value, Fn &&fn)
  {
    fn(value);
    return value;
  }

  /**
   * @brief Inspect a numeric range: min, max, sum, mean, and distribution.
   */
  template <typename Range>
    requires traits::is_range_v<Range> &&
             std::is_arithmetic_v<
                 std::remove_cvref_t<decltype(*std::begin(std::declval<Range>()))>>
  void inspect_numeric(const Range &rng, std::string_view label = "")
  {
    auto &os = *default_options().out;
    using V = std::remove_cvref_t<decltype(*std::begin(rng))>;

    if (!label.empty())
      os << label << ":\n";

    std::vector<V> items(std::begin(rng), std::end(rng));
    const std::size_t n = items.size();

    if (n == 0)
    {
      os << "  <empty range>\n";
      return;
    }

    V mn = items[0], mx = items[0];
    double sum = 0.0, sum_sq = 0.0;
    for (const auto &v : items)
    {
      if (v < mn)
        mn = v;
      if (v > mx)
        mx = v;
      double d = static_cast<double>(v);
      sum += d;
      sum_sq += d * d;
    }
    double mean = sum / n;
    double variance = (sum_sq / n) - (mean * mean);
    double stddev = std::sqrt(variance < 0.0 ? 0.0 : variance);

    os << "  count:    " << n << '\n'
       << "  min:      " << mn << '\n'
       << "  max:      " << mx << '\n'
       << "  sum:      " << sum << '\n'
       << "  mean:     " << mean << '\n'
       << "  stddev:   " << stddev << '\n';

    // Histogram (5 buckets)
    if (n >= 5 && mx > mn)
    {
      constexpr int BUCKETS = 5;
      double range_w = static_cast<double>(mx - mn);
      double bucket_w = range_w / BUCKETS;
      std::array<int, BUCKETS> hist{};
      for (const auto &v : items)
      {
        int idx = static_cast<int>(
            std::min(static_cast<double>(BUCKETS - 1),
                     (static_cast<double>(v) - static_cast<double>(mn)) / bucket_w));
        ++hist[idx];
      }
      os << "  histogram:\n";
      for (int i = 0; i < BUCKETS; ++i)
      {
        double lo = static_cast<double>(mn) + i * bucket_w;
        double hi = lo + bucket_w;
        int bar = hist[i] * 20 / static_cast<int>(n);
        os << "    [" << std::setw(7) << lo
           << ", " << std::setw(7) << hi << ") | ";
        for (int j = 0; j < bar; ++j)
          os << '#';
        os << " (" << hist[i] << ")\n";
      }
    }
  }

  namespace tree
  {
    /**
     * @brief Recursive tree-rendering context.
     *
     * Renders nested structures as a visual tree using box-drawing characters.
     */
    struct tree_context
    {
      std::ostream &os;
      int depth = 0;
      int max_depth = 6;
      bool last = true;
      std::string prefix;

      void node(std::string_view label) const
      {
        os << prefix;
        os << (last ? "└─ " : "├─ ");
        os << label;
      }

      tree_context child_ctx(bool is_last) const
      {
        tree_context c{os, depth + 1, max_depth, is_last,
                       prefix + (last ? "   " : "│  ")};
        return c;
      }
    };

    template <typename T>
    void twrite(tree_context &tctx, std::string_view label, const T &value);

    template <typename T>
    void twrite_leaf(tree_context &tctx, std::string_view label, const T &value)
    {
      tctx.node(label);
      tctx.os << ": ";
      inspect_context ictx{tctx.os};
      detail::iwrite(ictx, value);
      tctx.os << '\n';
    }

    template <typename Range>
    void twrite_range(tree_context &tctx, std::string_view label, const Range &rng)
    {
      tctx.node(label);
      std::size_t n = 0;
      if constexpr (requires { rng.size(); })
        n = rng.size();
      else
        n = static_cast<std::size_t>(std::distance(std::begin(rng), std::end(rng)));
      tctx.os << " [" << n << " items]\n";

      if (tctx.depth >= tctx.max_depth)
        return;

      std::size_t idx = 0;
      std::size_t total = n;
      for (const auto &elem : rng)
      {
        if (idx >= 8)
        {
          auto child = tctx.child_ctx(true);
          child.node("...");
          child.os << " (" << (total - idx) << " more)\n";
          break;
        }
        bool is_last = (idx + 1 == total || idx + 1 == 8);
        auto child = tctx.child_ctx(is_last);
        std::string lbl = "[" + std::to_string(idx) + "]";
        if constexpr (traits::is_map_like_v<Range>)
        {
          std::ostringstream ks;
          inspect_context kctx{ks};
          detail::iwrite(kctx, elem.first);
          twrite(child, ks.str(), elem.second);
        }
        else
        {
          twrite(child, lbl, elem);
        }
        ++idx;
      }
    }

    template <typename T>
    void twrite(tree_context &tctx, std::string_view label, const T &value)
    {
      using U = std::remove_cvref_t<T>;

      if (tctx.depth >= tctx.max_depth)
      {
        twrite_leaf(tctx, label, std::string_view{"<depth-limit>"});
        return;
      }

      if constexpr (traits::is_map_like_v<U> && traits::is_range_v<U>)
      {
        twrite_range(tctx, label, value);
      }
      else if constexpr (traits::is_range_v<U> && !traits::is_string_like_v<U>)
      {
        twrite_range(tctx, label, value);
      }
      else if constexpr (traits::is_optional_v<U>)
      {
        if (!value.has_value())
        {
          twrite_leaf(tctx, label, std::string_view{"None"});
        }
        else
        {
          auto child = tctx.child_ctx(true);
          twrite(child, std::string(label) + " (Some)", *value);
        }
      }
      else if constexpr (traits::is_variant_v<U>)
      {
        std::string vlabel = std::string(label) +
                             " <variant:" + std::to_string(value.index()) + ">";
        std::visit([&tctx, &vlabel](const auto &v)
                   {
            auto child = tctx.child_ctx(true);
            twrite(child, vlabel, v); }, value);
      }
      else if constexpr (traits::has_field_map_v<U>)
      {
        tctx.node(label);
        tctx.os << " [" << demangle::short_type_name<U>() << "]\n";
        if (tctx.depth < tctx.max_depth)
        {
          const auto fmap = field_map<U>::fields();
          constexpr std::size_t N =
              std::tuple_size_v<std::decay_t<decltype(fmap)>>;
          [&]<std::size_t... Is>(std::index_sequence<Is...>)
          {
            ((
                 [&]()
                 {
                   bool is_last = (Is + 1 == N);
                   auto child = tctx.child_ctx(is_last);
                   twrite(child, std::get<Is>(fmap).name,
                          std::get<Is>(fmap).get(value));
                 }()),
             ...);
          }(std::make_index_sequence<N>{});
        }
      }
      else
      {
        twrite_leaf(tctx, label, value);
      }
    }

  } // namespace tree

  /**
   * @brief Print a visual tree representation of any value.
   *
   * Uses box-drawing characters to show structure recursively.
   *
   * @example
   *   rixlib::inspect_tree(my_config, "config");
   */
  template <typename T>
  void inspect_tree(const T &value, std::string_view root_label = "root")
  {
    auto &os = *default_options().out;
    tree::tree_context tctx{os};
    tree::twrite(tctx, root_label, value);
  }

} // namespace rixlib

namespace rixlib::debug
{
  /**
   * @brief Object-style inspection API mounted into rixlib::debug::Debug.
   */
  class Inspect
  {
  public:
    template <typename T>
    void operator()(const T &value) const
    {
      rixlib::inspect(value);
    }

    template <typename T>
    void to(std::ostream &os, const T &value) const
    {
      rixlib::inspect_to(os, value);
    }

    template <typename T>
    [[nodiscard]] std::string to_string(const T &value) const
    {
      return rixlib::inspect_to_string(value);
    }

    template <typename T>
    [[nodiscard]] std::string compact(const T &value) const
    {
      return rixlib::inspect_compact(value);
    }

    template <typename Expected, typename Actual>
    [[nodiscard]] bool check(const Expected &expected, const Actual &actual) const
    {
      return rixlib::inspect_check("inspect.check", expected, actual);
    }

    template <typename Expected, typename Actual>
    [[nodiscard]] bool check(std::string_view label,
                             const Expected &expected,
                             const Actual &actual) const
    {
      return rixlib::inspect_check(label, expected, actual);
    }
  };
}

#endif // RIXCPP_DEBUG_INCLUDE_RIX_DEBUG_INSPECT_HPP_INCLUDED
