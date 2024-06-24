// Tests spdlog vs notarius
// - cout/cerr have been turned off for the tests
// - date/time and info, error, etc. prefixes are not included also.

// Test options must be defined prior to 'write-tests.hpp'.
//
constexpr auto test_runs_to_perform = 20;
constexpr auto max_lines_count_ = 1000;

// The following option is not available for spdlog.
//
// Note: Using the notarius '<<' operator for streaming may result in slightly
//       slower performance depending on how it is used.
//
static constexpr bool test_notarius_operators = false;

#include "write-tests.hpp"

// I recommend using a Markdown editor for reviewing result files.

int main()
{
#ifdef SPDLOG
   // Turn off the time stamp output in spdlog.
   spdlog::set_pattern("%v");
#endif

   // Note: Not all markdown editors support html color attributes.
   notarius_results_logger("<span style=\"color:#2F4C99\">**Date / Time Run {}:**</span>\n",
                           slx::chrono::current_time_as_string());

   // for (auto i = 0; i < test_runs_to_perform; ++i) {
   //    spdlog_vs_notarius_tests(std::format("Run {}", i + 1), test_runs_to_perform);
   //  }

   notarius_logger.options().immediate_mode = true;
   notarius_logger.write("Hello\n");

   report_final_results();

   return 0;
}
