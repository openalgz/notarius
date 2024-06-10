
// Test options must be defined prior to 'write-tests.hpp'.
//
constexpr auto test_runs_to_perform = 5;
constexpr auto max_lines_count_ = 10;

#include "notarius-demo.hpp"

// I recommend using a Markdown editor for reviewing result files.

int main()
{
   // Note: Not all markdown editors support html color attributes.
   notarius_results_logger("<span style=\"color:#2F4C99\">**Date / Time Run {}:**</span>\n",
                           slx::chrono::current_time_as_string());

   for (auto i = 0; i < test_runs_to_perform; ++i) {
      run_notarius_tests(std::format("Run {}", i + 1), test_runs_to_perform);
   }

   report_final_results();

   return 0;
}
