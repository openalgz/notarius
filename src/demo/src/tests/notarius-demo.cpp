#include "notarius/notarius.hpp"
#include "ut/ut.hpp"

using namespace ut;
using namespace slx; // the notarius namespace

// Note:
// I recommend using a Markdown editor for reviewing result files.

suite example_notarius_use_cases = [] {

   notarius_t<"demo-log.md", notarius_opts_t{.enable_file_logging = true}> demo;

   demo.remove_log_file();
   {
      demo("Hello World\n");
      auto actual = demo.str();
      expect("Hello World\n" == actual);

      demo("Hello World Again {} {} {} {}\n", 1.23, 2.23, 3.23, 4.23);
      actual = demo.str();
      auto expected_result = std::format("Hello World\nHello World Again {} {} {} {}\n", 1.23, 2.23, 3.23, 4.23);
      expect(actual == expected_result);
   }
   demo.remove_log_file();

   demo.remove_log_file();
   {
      demo.print("Hello World\n");
      auto actual = demo.str();
      expect("Hello World\n" == actual);

      demo.enable_stdout();
      demo.write("Hello World Again {} {} {} {}\n", 1.23, 2.23, 3.23, 4.23);
      actual = demo.str();
      auto expected_result = std::format("Hello World\nHello World Again {} {} {} {}\n", 1.23, 2.23, 3.23, 4.23);
      expect(actual == expected_result);
   }
   demo.remove_log_file();

   demo.remove_log_file();
   {
      demo << "Hello World " << 1.23 << " " << 2.23 << '\n';
      auto actual = demo.str();
      auto expected_result = "Hello World 1.23 2.23\n";
      expect(actual == expected_result);
      demo.remove_log_file();
   }
   demo.remove_log_file();
};

int main()
{
   std::cout << '\n';
   return 0;
}