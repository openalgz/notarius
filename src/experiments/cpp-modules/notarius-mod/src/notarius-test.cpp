#include <ut/ut.hpp>

import notarius;

using namespace ut;
using namespace slx;

int main()
{
   constexpr auto options = slx::notarius_opts_t{.enable_file_logging = true};
   notarius_t<"test-log-file.md", options> logger;

   logger.remove_log_file();

   suite notarius_standard_api_tests = [&] {
      // Test case for `print` method
      "notarius_t print method"_test = [&] {
         logger.print<log_level::none>("Hello, {}", "world\n");
         auto actual = logger.str();
         constexpr auto expected = "Hello, world\n";
         expect(actual == expected);
         logger.remove_log_file();
      };

      // Test case for `cout` method
      "notarius_t cout method"_test = [&] {
         logger.cout<log_level::info>("Test cout {}", "output\n");
         auto actual = logger.str();
         constexpr auto expected = ""; // Writing to cout only
         expect(actual == expected);
         logger.remove_log_file();
      };

      // Test case for `cerr` method
      "notarius_t cerr method"_test = [&] {
         logger.cerr<log_level::error>("Test cerr {}", "output\n");
         auto actual = logger.str();
         constexpr auto expected = ""; //  // Writing to cerr only
         expect(actual == expected);
         logger.remove_log_file();
      };

      // Test case for `clog` method
      "notarius_t clog method"_test = [&] {
         logger.clog<log_level::warn>("Test clog {}", "output\n");
         auto actual = logger.str();
         constexpr auto expected = ""; // Writing to clog only
         expect(actual == expected);
         logger.remove_log_file();
      };

      // Test case for `str` method
      "notarius_t str method"_test = [&] {
         logger.print<log_level::none>("Message for str test\n");
         auto actual = logger.str();
         constexpr auto expected = "Message for str test\n";
         expect(actual == expected);
         logger.remove_log_file();
      };

      // Test case for `enable_stdout` and `pause_stdout` methods
      "notarius_t enable and pause stdout"_test = [&] {
         logger.enable_stdout();
         logger.cout("This should appear in stdout.\n");
         logger.pause_stdout();
         logger.cout("This should NOT appear in stdout.\n");

         auto actual = logger.str();
         constexpr auto expected = "";  // Writing to cout only
         expect(actual == expected);
         logger.remove_log_file();
      };

      // Test case for `flush` method
      "notarius_t flush method"_test = [&] {
         logger.print("This should be flushed.\n");
         logger.flush();

         auto actual = logger.str();
         constexpr auto expected = "This should be flushed.\n";
         expect(actual == expected);
         logger.remove_log_file();
      };

      // Test case for `clear` method
      "notarius_t clear method"_test = [&] {
         logger.print("This will be cleared.\n");
         logger.clear();
         expect(logger.empty() == true);
      };

      // Test case for `remove_log_file` method
      "notarius_t remove log file method"_test = [&] {
         logger.print("Log file to be removed.\n");
         logger.remove_log_file();

         // Checking if the log file was removed by verifying the logger is empty
         expect(logger.empty() == true);
      };

      // Test case for `resize` method
      "notarius_t resize method"_test = [&] {
         logger.print("Message before resize.\n");
         logger.resize(0); // Resizing to zero should clear the content
         expect(logger.empty() == true);
      };

      // Test case for `capacity` method
      "notarius_t capacity method"_test = [&] {
         logger.print("Message to check capacity.\n");
         auto capacity = logger.capacity();
         expect(capacity >= logger.size()); // Capacity should be at least the size
      };
   };

   return 0;
}
