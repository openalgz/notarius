#pragma once

#include <concepts>
#include <format>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>

namespace slx
{
   template <typename T>
   concept is_numeric = std::is_arithmetic_v<T>;

   template <typename... Args>
   std::string fmt_string(std::string_view format_str, Args&&... args)
   {
      return std::vformat(format_str, std::make_format_args(std::forward<Args>(args)...));
   }

   // Overload for compile-time format string checking (C++20 and later)
   //
   template <typename... Args>
   constexpr auto fmt_string(const std::format_string<Args...>& format_str, Args&&... args)
   {
      return std::format(format_str, std::forward<Args>(args)...);
   }

   template <is_numeric T>
   std::string to_string(const T value, const int precision = 4)
   {
      static thread_local std::ostringstream out;
      out.str("");
      out.clear(); // Clear any error flags
      if constexpr (std::is_floating_point_v<T>) {
         out << std::fixed << std::setprecision(precision);
      }
      out << value;
      return out.str();
   }

}
