#pragma once

#include <cctype>
#include <concepts>
#include <format>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

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

   // Function to create the partial match table (failure function)
   // i.e., KMP algorithm implementation
   //
   // The KMP algorithm preprocesses the pattern to create a "partial match" table
   // (also known as the "failure function") that allows the search to skip sections
   // of the text, resulting in a more efficient search.
   //
   inline std::vector<int> build_kmp_table(std::string_view pattern)
   {
      std::vector<int> table(pattern.size(), 0);
      int j = 0;

      for (size_t i = 1; i < pattern.size(); ++i) {
         if (pattern[i] == pattern[j]) {
            table[i] = ++j;
         }
         else {
            if (j != 0) {
               j = table[j - 1];
               --i; // Decrement i to recheck this position
            }
            else {
               table[i] = 0;
            }
         }
      }

      return table;
   }

   inline std::string replace_substr(std::string input, std::string_view replace, std::string_view token)
   {
      if (replace.empty()) return input;

      std::string result;
      result.reserve(input.length());

      if (input.size() < 1024) {
         //
         // use basic approach for shorter strings
         //
         size_t pos = 0;
         while ((pos = input.find(replace, pos)) != std::string::npos) {
            result.append(input, 0, pos);
            result.append(token);
            pos += replace.length();
            input = input.substr(pos);
            pos = 0;
         }
         result.append(input);
      }
      else {
         // KMP approach for longer strings
         std::vector<int> kmp_table = build_kmp_table(replace);

         size_t i = 0, j = 0;
         while (i < input.size()) {
            if (input[i] == replace[j]) {
               ++i;
               ++j;
            }

            if (j == replace.size()) {
               result.append(token);
               j = 0;
            }
            else if (i < input.size() && input[i] != replace[j]) {
               if (j != 0) {
                  j = kmp_table[j - 1];
               }
               else {
                  result.push_back(input[i]);
                  ++i;
               }
            }
         }

         // Append the remaining part of the input string
         result.append(input.substr(i - j));
      }

      return result;
   }

   inline std::string replace_substrings(
      std::string input, const std::vector<std::tuple<std::string_view /*replace*/, std::string_view /*with*/>>&
                            substrings_and_replace_values)
   {
      std::string result;
      for (const auto& [replace, with] : substrings_and_replace_values)
         result.append(replace_substr(input, replace, with));
      return {result};
   }

   inline std::string remove_any_of(std::string str, const std::vector<std::string>& tokens = {" "})
   {
      size_t start = 0;
      size_t end = str.length();

      // Check if a position matches any token
      auto match_any_token = [&](size_t pos) {
         return std::any_of(tokens.begin(), tokens.end(),
                            [&](const std::string& token) { return str.compare(pos, token.length(), token) == 0; });
      };

      // Find first non-token character
      while (start < str.length() && match_any_token(start)) {
         ++start;
      }

      // Find last non-token character
      while (end > start && match_any_token(end - 1)) {
         --end;
      }

      auto t = str.substr(start, end - start);
      std::cout << t << '\n';
      return t;
   }

   inline std::string reduce_consecutive_whitespace(const std::string& str,
                                                    const std::vector<char>& whitespace_chars = {' '},
                                                    int reduce_to = 1, bool reduce_all_whitespace_types = false)
   {
      std::string result;
      int whitespace_count = 0;

      auto is_target_whitespace = [&whitespace_chars, reduce_all_whitespace_types](char c) -> bool {
         if (reduce_all_whitespace_types) {
            return std::isspace(static_cast<unsigned char>(c));
         }
         return std::find(whitespace_chars.begin(), whitespace_chars.end(), c) != whitespace_chars.end();
      };

      for (char ch : str) {
         if (is_target_whitespace(ch)) {
            if (whitespace_count < reduce_to) {
               result += ch;
            }
            ++whitespace_count;
         }
         else {
            result += ch;
            whitespace_count = 0;
         }
      }

      return result;
   }

   inline std::string trim_left(const std::string& str, const std::vector<char>& tokens = {' '})
   {
      auto match_token = [&tokens](char ch) { return std::find(tokens.begin(), tokens.end(), ch) != tokens.end(); };
      auto it = std::find_if_not(str.begin(), str.end(), match_token);
      return std::string(it, str.end());
   }

   inline std::string trim_right(const std::string& str, const std::vector<char>& tokens = {' '})
   {
      auto match_token = [&tokens](char ch) { return std::find(tokens.begin(), tokens.end(), ch) != tokens.end(); };
      auto it = std::find_if_not(str.rbegin(), str.rend(), match_token);
      return std::string(str.begin(), it.base());
   }

   // trim left/right
   inline std::string trim_ends(const std::string& str, const std::vector<char>& tokens = {' '})
   {
      return trim_left(trim_right(str, tokens), tokens);
   }
}
