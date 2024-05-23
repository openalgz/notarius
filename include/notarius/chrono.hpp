#pragma once

#include <chrono>
#include <format>
#include <string>

#include "io.hpp"

namespace slx
{
   namespace chrono
   {
      // Nanosecond constants
      inline constexpr int64_t ns_per_min = 60'000'000'000LL;
      inline constexpr int64_t ns_per_sec = 1'000'000'000LL;
      inline constexpr int64_t ns_per_ms = 1'000'000LL;

      // Millisecond constants
      inline constexpr int64_t ms_per_min = 60'000LL;
      inline constexpr int64_t ms_per_sec = 1'000LL;

      // Second constants
      inline constexpr int64_t sec_per_min = 60LL;
      inline constexpr int64_t sec_per_hour = 3'600LL;

      // Minute constants
      inline constexpr int64_t min_per_hour = 60LL;

      // double conversions:
      inline constexpr double secs_to_minutes = 1.0 / 60.0;
      inline constexpr double secs_to_hours = 1.0 / 3600.0;
      inline constexpr double millisecs_to_secs = 0.001;
      inline constexpr double nanosecs_to_millisecs = 1e-6;

      std::string write_ave_duration_message(std::chrono::steady_clock::duration duration)
      {
         using namespace std::chrono;
         auto duration_ns = duration_cast<nanoseconds>(duration).count();
         const int64_t minutes = duration_ns / ns_per_min;
         duration_ns %= ns_per_min;
         const int64_t seconds = duration_ns / ns_per_sec;
         duration_ns %= ns_per_sec;
         const int64_t milliseconds = duration_ns / ns_per_ms;
         const int64_t nanoseconds = duration_ns % ns_per_ms;
         if (nanoseconds > 0) {
            return std::format("{}m {}s {}ms {}ns", minutes, seconds, milliseconds, nanoseconds);
         }
         return std::format("{}m {}s {}ms", minutes, seconds, milliseconds);
      }

      // returns: yyyymmdd::hh:mm:ss
      //
      std::string current_time_as_string(
         const std::string& format = "%Y-%m-%d (%H:%M:%S)",
         const std::chrono::system_clock::time_point& time_point = std::chrono::system_clock::now())
      {
         auto time = std::chrono::system_clock::to_time_t(time_point);
         auto localtime = std::localtime(&time);
         std::ostringstream oss;
         oss << std::put_time(localtime, format.c_str());
         return oss.str();
      }

      /* The above is slightly faster ...
       *
      std::string current_time_as_string(const std::chrono::system_clock::time_point& time_point =
      std::chrono::system_clock::now(), const std::string_view format =
      "{0:04d}{1:02d}{2:02d}::{3:02d}:{4:02d}:{5:02d}") { auto time = std::chrono::system_clock::to_time_t(time_point);
         auto localtime = std::localtime(&time);
         std::string formatted_time = std::fmt_string(format,
            localtime->tm_year + 1900,
            localtime->tm_mon + 1,
            localtime->tm_mday,
            localtime->tm_hour,
            localtime->tm_min,
            localtime->tm_sec);
         return formatted_time;
      }
      */

      inline double to_hours_from_seconds(const double seconds) { return seconds * secs_to_hours; }
      inline double to_minutes_from_seconds(const double seconds) { return seconds * secs_to_minutes; }
      inline double to_seconds_from_milliseconds(const double milliseconds) { return milliseconds * 0.001; }

      inline std::string write_duration_message(int hours, int minutes, int seconds, int milliseconds)
      {
         return std::format("{}h {}m {}s {}ms", hours, minutes, seconds, milliseconds);
      }

      inline std::string write_duration_message(int minutes, int seconds, int milliseconds)
      {
         return std::format("{}m {}s {}ms", minutes, seconds, milliseconds);
      }

      inline std::string write_duration_message(int seconds, int milliseconds)
      {
         return std::format("{}s {}ms", seconds, milliseconds);
      }

      inline std::string write_duration_message(int nanoseconds) { return std::format("{} nanoseconds", nanoseconds); }

      inline std::string duration_to_string(std::chrono::steady_clock::duration duration)
      {
         auto minutes = std::chrono::duration_cast<std::chrono::minutes>(duration).count();
         auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration).count() % 60;
         auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count() % 1000;
         return std::format("{}m {}s {}ms", minutes, seconds, milliseconds);
      }

      inline std::string date_time_to_string(const std::string_view fmt = "%Y-%m-%d %H:%M:%S")
      {
         const auto now = std::chrono::system_clock::now();
         auto ms = static_cast<int>(now.time_since_epoch().count() % 1000);

         std::time_t t = std::chrono::system_clock::to_time_t(now);
         std::tm tm{};
#ifdef _WIN32
         localtime_s(&tm, &t);
#else
         localtime_r(&t, &tm);
#endif
         char buffer[24]{};
         std::strftime(buffer, sizeof(buffer), fmt.data(), &tm);
         return std::string(buffer) + "." + std::to_string(ms);
      }

      struct duration_t
      {
         std::chrono::steady_clock::time_point t0{std::chrono::steady_clock::now()};
         std::chrono::steady_clock::time_point t_end{};

         void reset() { t0 = std::chrono::steady_clock::now(); }

         auto start()
         {
            t0 = std::chrono::steady_clock::now();
            return t0;
         }

         auto stop()
         {
            t_end = std::chrono::steady_clock::now();
            return t_end;
         }

         double duration_in_seconds()
         {
            stop();
            return std::chrono::duration<double>(t_end - t0).count();
         }

         double duration_in_milliseconds()
         {
            stop();
            return std::chrono::duration<double, std::milli>(t_end - t0).count();
         }

         double duration_in_hours()
         {
            stop();
            return duration_in_seconds() * secs_to_hours;
         }

         double duration_in_nanoseconds()
         {
            stop();
            return std::chrono::duration<double, std::nano>(t_end - t0).count();
         }

         std::chrono::steady_clock::duration duration()
         {
            stop();
            return (t_end - t0);
         }

         std::string duration_to_string()
         {
            const double duration_milliseconds = duration_in_milliseconds();
            const double duration_seconds = duration_in_seconds();
            const double duration_hours = duration_seconds * secs_to_hours;

            const int hours = static_cast<int>(duration_hours);
            const int remaining_seconds = static_cast<int>((duration_seconds - hours * 3600.0));
            const int minutes = remaining_seconds / 60;
            const int seconds = remaining_seconds % 60;
            const int milliseconds =
               static_cast<int>(duration_milliseconds - hours * 3600 * 1000 - minutes * 60 * 1000 - seconds * 1000);
            const int nanoseconds = static_cast<int>(
               (milliseconds * 1000) +
               ((duration_seconds - hours * 3600.0 - minutes * 60.0 - seconds - milliseconds / 1000.0) * 1e6));

            if (duration_hours >= 1.0) {
               return write_duration_message(hours, minutes, seconds, milliseconds);
            }
            else if (duration_seconds >= 1.0) {
               return write_duration_message(minutes, seconds, milliseconds);
            }
            else if (duration_milliseconds >= 1.0) {
               return write_duration_message(seconds, milliseconds);
            }
            else {
               return write_duration_message(milliseconds, nanoseconds);
            }
         }
      };
   }
}