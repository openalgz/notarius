#pragma once

#include <filesystem>

#include "notarius/chrono.hpp"
#include "notarius/notarius.hpp"

#if __has_include(<ut/ut.hpp>)
#include <ut/ut.hpp>
#endif

using namespace slx;
using namespace slx::chrono;
namespace fs = std::filesystem;

// Results timing
static std::string notarius_time_result;
static std::string notarius_async_time_result;
static std::chrono::steady_clock::duration ave_notarius_time_result{};
static std::chrono::steady_clock::duration ave_async_notarius_time_result{};

inline slx::notarius_t<"notarius-results", slx::notarius_opts_t{.enable_file_logging = true}, "md"> notarius_logger;

// A log file containing a summary of results.
inline slx::notarius_t<"notarius_test_results", slx::notarius_opts_t{.enable_file_logging = true}, "md">
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

inline constexpr uint32_t v3 = 12345;
inline constexpr float v4 = 3.14159f;
inline constexpr double v5 = 6.78901;
inline constexpr const char* v6 = "A string in Markdown";

inline void test_notarius_streaming_operator(const std::string& caption, int id, int max_lines_count_)
{
   for (auto i = 0; i < max_lines_count_; ++i) {
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
}

inline void test_notarius(const std::string& caption, int id, int max_lines_count_)
{
   for (auto i = 0; i < max_lines_count_; ++i) {
      notarius_logger(fmt_notarius, caption, id, i, v3, v4, v5, v6);
   }
}

inline void test_notarius_async(const std::string& caption, int id, int max_lines_count_)
{
   auto log_async = [&](int i) { notarius_logger(fmt_notarius, caption, id, i, v3, v4, v5, v6); };
   std::vector<std::future<void>> futures;
   futures.reserve(max_lines_count_);
   for (int i = 0; i < max_lines_count_; ++i) {
      futures.emplace_back(std::async(std::launch::async, log_async, i));
   }
   // Wait for all tasks to complete
   for (auto& future : futures) {
      future.get();
   }
}

inline void publish_results(const std::string_view caption)
{
   notarius_results_logger("**{}**:\n", caption);
   notarius_results_logger("{}\n", notarius_time_result);
   notarius_results_logger("{}", notarius_async_time_result);
   notarius_results_logger("\n\n");
   notarius_results_logger.close();
}

int run_notarius_tests(std::string_view run, int total_test_runs_count)
{
   using namespace ut;

   "notarius_test_streaming_operator"_test = [&] {
      for (int i = 0; i < 10; ++i) {
         test_notarius(std::string(run), i, max_lines_count_);
      }
   };

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

   publish_results(run);

   return 0;
}

inline void report_final_results()
{
   ave_notarius_time_result /= test_runs_to_perform;
   const auto ave_notarius = write_ave_duration_message(ave_notarius_time_result);
   const auto ave_async_notarius = write_ave_duration_message(ave_async_notarius_time_result);
   constexpr auto summary =
      "\n> [!NOTE]\n>**Summary of Results (Avg. Time):**\n>\n> ```C++\nnotarius: {}; async: {}\n>```\n>\n------\n\n";
   notarius_results_logger(summary, ave_notarius, ave_async_notarius);
   notarius_results_logger.close();
}
