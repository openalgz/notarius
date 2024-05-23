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

   template <typename First, typename... Args>
   auto fmt_string(First&& first, Args&&... args)
   {
      return std::vformat(std::forward<First>(first), std::make_format_args(args...));
   }

   template <is_numeric T>
   std::string to_string(const T value, const int precision = 4)
   {
      static thread_local std::ostringstream out;
      out.str("");
      out << std::fixed << std::setprecision(precision) << value;
      return out.str();
   }

}