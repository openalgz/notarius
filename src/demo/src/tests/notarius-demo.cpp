
// Test options must be defined prior to 'write-tests.hpp'.
//
constexpr auto test_runs_to_perform = 5;
constexpr auto max_lines_count_ = 10;

#include "notarius/notarius.hpp"
#include "ut/ut.hpp"

using namespace ut;
using namespace slx; // the notarius namespace

// Note:
// I recommend using a Markdown editor for reviewing result files.

suite example_notarius_use_cases = [] {
   notarius_t<"demo-log.md", notarius_opts_t{.enable_file_logging = true}> demo;

   demo("Hello World");
   auto actual = demo.str();
   expect("Hello World" == actual);

   demo("Hello World Again {} {} {} {}", 1.23, 2.23, 3.23, 4.23);
   auto expected_result = std::format("Hello World Again {} {} {} {}", 1.23, 2.23, 3.23, 4.23);
   actual = demo.str();
   expect(actual == expected_result);
};

int main() { return 0; }
