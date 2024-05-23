#pragma once

// Include/Compile SPDLOG Tests
//
#define SPDLOG
// #define INCLUDE_ASYC_TESTS

#include <array>
#include <filesystem>
#include <format>
#include <future>
#include <iostream>
#include <memory>
#include <shared_mutex>
#include <stdexcept>
#include <string>

#include "notarius/chrono.hpp"
#include "notarius/notarius.hpp"

// clang-format off
#ifdef SPDLOG
   #include "spdlog/sinks/basic_file_sink.h"
   #include "spdlog/spdlog.h"
   #ifdef SPDLOG_HEADER_ONLY
      #pragma message("SPDLOG_HEADER_ONLY is defined")
   #else
      #pragma message("SPDLOG_HEADER_ONLY is not defined")
   #endif
#endif
// clang-format on

#if __has_include(<boost/ut.hpp>)
#include <boost/ut.hpp>
#endif

using namespace slx;
using namespace slx::chrono;
namespace fs = std::filesystem;

// Results timing
static std::string notarius_time_result;
static std::string spdlog_time_result;

static std::string notarius_async_time_result;
static std::string spdlog_async_time_result;

static std::chrono::steady_clock::duration ave_notarius_time_result{};
static std::chrono::steady_clock::duration ave_spdlog_time_result{};

static std::chrono::steady_clock::duration ave_async_notarius_time_result{};
static std::chrono::steady_clock::duration ave_async_spdlog_time_result{};

// Loggers have been setup to function the same as far as possible. 'stdout' and 'stderr' are off in each.
//
// In other words file logging alone is enabled for the timing test.
//
inline slx::notarius_t<"notarius-results", slx::notarius_opts_t{}, "md"> notarius_logger;

#ifdef SPDLOG
inline auto spdlog_logger = spdlog::basic_logger_mt("basic_logger", "spdlog-results.md");
#endif

// A log file containing a summary of results.
inline slx::notarius_t<"spdlog_vs_notarius_test_results", slx::notarius_opts_t{.enable_file_logging = true}, "md">
   notarius_results_logger;

inline void record_tests_duration(const std::string_view caption, int total_tests_run_count,
                                  slx::chrono::duration_t& timer, std::string& result)
{
   auto end_time = timer.duration_to_string();
   result = std::format("{}: {} test runs each writing {} lines; Total Time: *{}*\n", caption, total_tests_run_count,
                        max_lines_count_, end_time);
}

// Two different formats are used because spdlog appends a '\n' with each line logged.
// Therefore the format differences below.
//
inline constexpr const char* fmt_notarius =
   "{0}: Thread Id: {1} line {2}: values: uint32_t: {3}; float: {4}; double: {5}; string: **{6}**\n";
inline constexpr const char* fmt_spdlog =
   "{0}: Thread Id: {1} line {2}: values: uint32_t: {3}; float: {4}; double: {5}; string: **{6}**";
inline constexpr uint32_t v3 = 12345;
inline constexpr float v4 = 3.14159f;
inline constexpr double v5 = 6.78901;
inline constexpr const char* v6 = "A string in Markdown";

#ifdef SPDLOG
inline void test_spdlog(const std::string& caption, int id, int max_lines_count_)
{
   for (auto i = 0; i < max_lines_count_; ++i) {
      spdlog_logger->info(fmt_spdlog, caption, id, i, v3, v4, v5, v6);
   }
}

inline void test_spdlog_async(const std::string& caption, int id, int max_lines_count_)
{
   auto log_async = [&](int i) { spdlog_logger->info(fmt_spdlog, caption, id, i, v3, v4, v5, v6); };
   for (auto i = 0; i < max_lines_count_; ++i) {
      auto future = std::async(std::launch::async, log_async, i);
   }
}
#endif

inline void test_notarius(const std::string& caption, int id, int max_lines_count_)
{
   for (auto i = 0; i < max_lines_count_; ++i) {
      if constexpr (test_notarius_operators) {
         // This would be more typical and cleaner...
         //
         // notarius_logger << std::format(fmt_notarius, caption, id, i, v3, v4, v5, v6);

         // This option is not available for spdlog and will result in
         // slightly slower logging times.
         //
         notarius_logger << "\nUsing '<<' operator for streaming: " << caption << ":"
                         << " Thread Id : " << id << "line " << i << ": values unint32_t: " << v3 << "; float: " << v4
                         << "; double: " << v5 << "; string: **" << v6 << "{6}**";
      }
      else
         notarius_logger(fmt_notarius, caption, id, i, v3, v4, v5, v6);
   }
}

inline void test_notarius_async(const std::string& caption, int id, int max_lines_count_)
{
   auto log_async = [&](int i) { notarius_logger(fmt_notarius, caption, id, i, v3, v4, v5, v6); };
   for (auto i = 0; i < max_lines_count_; ++i) {
      auto future = std::async(std::launch::async, log_async, i);
   }
}

inline void publish_results(const std::string_view caption)
{
   notarius_results_logger("**{}**:\n", caption);

   notarius_results_logger("{}", spdlog_time_result);
   notarius_results_logger("{}\n", notarius_time_result);

   notarius_results_logger("{}", spdlog_async_time_result);
   notarius_results_logger("{}", notarius_async_time_result);

   notarius_results_logger("\n\n");
   notarius_results_logger.close();
}

int spdlog_vs_notarius_tests(std::string_view run, int total_test_runs_count)
{
   // std::cout, etc. are disabled of logging tests.
   // in notarius_logger.options_ these are all set to true.
   //
   notarius_logger.pause_stderr();
   notarius_logger.pause_stdout();
   notarius_logger.pause_stdlog();

#if __has_include(<boost/ut.hpp>)
   using namespace boost::ut;
#else
   using namespace ut;
#endif

   "notarius_file_logging"_test = [&] {
      slx::chrono::duration_t timer;
      for (int i = 0; i < 10; ++i) {
         test_notarius(std::string(run), i, max_lines_count_);
      }
      notarius_logger("\n");
      ave_notarius_time_result += timer.duration();
      record_tests_duration("notarius", total_test_runs_count, timer, notarius_time_result);
   };

   "notarius_file_logging_async"_test = [&] {
      slx::chrono::duration_t timer;
      for (int i = 0; i < 10; ++i) {
         test_notarius_async(std::string(run), i, max_lines_count_);
      }
      notarius_logger("\n");
      ave_async_notarius_time_result += timer.duration();
      record_tests_duration("notarius-async", total_test_runs_count, timer, notarius_async_time_result);
   };

#ifdef SPDLOG
   "spdlog_file_logging"_test = [&] {
      slx::chrono::duration_t timer;
      for (int i = 0; i < 10; ++i) {
         test_spdlog(std::string(run), i, max_lines_count_);
      }
      spdlog_logger->info("\n");
      ave_spdlog_time_result += timer.duration();
      record_tests_duration("spdlog", total_test_runs_count, timer, spdlog_time_result);
   };

   "spdlog_file_logging_async"_test = [&] {
      slx::chrono::duration_t timer;
      for (int i = 0; i < 10; ++i) {
         test_spdlog_async(std::string(run), i, max_lines_count_);
      }
      spdlog_logger->info("\n");
      ave_async_spdlog_time_result += timer.duration();
      record_tests_duration("spdlog-async", total_test_runs_count, timer, spdlog_async_time_result);
   };
#endif

   publish_results(run);

   return 0;
}

inline void report_final_results()
{
   ave_spdlog_time_result /= test_runs_to_perform;
   ave_notarius_time_result /= test_runs_to_perform;
   const auto ave_spdlog = write_ave_duration_message(ave_spdlog_time_result);
   const auto ave_notarius = write_ave_duration_message(ave_notarius_time_result);
   const auto ave_async_spdlog = write_ave_duration_message(ave_async_spdlog_time_result);
   const auto ave_async_notarius = write_ave_duration_message(ave_async_notarius_time_result);
   constexpr auto summary =
      "\n> [!NOTE]\n>**Summary of Results (Avg. Time):**\n>\n> ```C++\n  spdlog: {}; async: {}\nnotarius: {}; "
      "async: {}\n>```\n>\n------\n\n";
   notarius_results_logger(summary, ave_spdlog, ave_async_spdlog, ave_notarius, ave_async_notarius);
   notarius_results_logger.close();
   const auto cmd = std::format("start {}", fs::absolute(notarius_results_logger.log_output_file_path_).string());
   std::system(cmd.c_str());
}
