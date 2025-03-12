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

using namespace boost::ut;
using namespace slx;

void run_api_tests()
{
   auto test_cleanup = [] {
      const std::string filename = "test_log.txt";
      std::filesystem::path p = filename;

      std::string directory = p.parent_path().string();
      if (directory.empty()) {
         directory = std::filesystem::current_path().string();
      }
      remove_files_in_directory<true>(directory, ".txt");
   };

   test_cleanup();

   suite string_literal_tests = [] {
      "string_literal size"_test = [] {
         constexpr string_literal<5> str{"test"};
         expect(str.size() == 4_ul);
      };

      "string_literal comparison"_test = [] {
         constexpr string_literal<5> str1{"test"};
         constexpr string_literal<5> str2{"test"};
         expect(str1 == str2);
      };

      "string_literal to string_view"_test = [] {
         constexpr string_literal<5> str{"test"};
         expect(str.sv() == std::string_view{"test"});
      };
   };

   suite std_stream_redirection_tests = [] {
      "std_stream_redirection_t redirect"_test = [] {
         std::ostringstream oss;
         {
            std_stream_redirection_t redirect(std::clog, oss.rdbuf());
            std::clog << "test\n";
         }
         expect(oss.str() == "test\n");
      };

      "std_stream_redirection_t reset"_test = [] {
         std::ostringstream oss;
         std_stream_redirection_t redirect(std::clog, oss.rdbuf());
         redirect.reset();
         std::clog << "test\n";
         expect(oss.str().empty());
      };
   };

   suite get_next_available_filename_tests = [] {
      "get_next_available_filename no file exists"_test = [] {
         const std::string filename = "test_log.txt";
         auto actual = get_filename(get_next_available_filename(filename));
         expect(actual == filename);
      };

      "get_next_available_filename file exists"_test = [] {
         const std::string filename = "test_log.txt";
         std::ofstream(filename).close(); // Create an empty file
         auto actual = get_filename(get_next_available_filename(filename));
         expect(actual == "test_log_1.txt");
         remove_files({filename, "test_log_1.txt"});
      };

      "get_next_available_filename multiple files exist"_test = [] {
         const std::string filename = "test_log.txt";
         std::ofstream(filename).close();
         std::ofstream("test_log_1.txt").close();
         auto actual = get_filename(get_next_available_filename(filename));
         expect(actual == "test_log_2.txt");
         remove_files({filename, "test_log_1.txt", "test_log_2.txt"});
      };
   };

   suite notarius_std_cout_cerr_test = [] {
      constexpr auto max_runs = 100;
      "std_cout"_test = [] {
         notarius_t<"std_cout_test.md", notarius_opts_t{.log_from_stdout = true}> logger;
         for (auto i = 0; i < max_runs; ++i) {
            logger.cout("writing to std::cout and log file: std_cout_test.md (line {} of {}).\n", i + 1, max_runs);
         }
      };

      "std_cerr"_test = [] {
         notarius_t<"std_cerr_test.md", notarius_opts_t{.log_from_stderr = true}> logger;
         for (auto i = 0; i < max_runs; ++i) {
            logger.cerr("writing to std::cerr and log file: std_cerr_test.md (line {} of {}).\n", i + 1, max_runs);
         }
      };

      "std_cout_cerr"_test = [] {
         notarius_t<"std_cout_cerr_test.md", notarius_opts_t{.log_from_stdout = true, .log_from_stderr = true}> logger;
         for (auto i = 0; i < max_runs; ++i) {
            logger.cout("writing to std::cout and log file: std_cout_cerr_test.md (line {} of {}).\n", i + 1, max_runs);
            logger.cerr("writing to std::cerr and log file: std_cout_cerr_test.md (line {} of {}).\n", i + 1, max_runs);
         }
      };
   };

   suite notarius_t_tests = [] {
      "notarius_t print method"_test = [] {
         notarius_t<"test-log-file.md", notarius_opts_t{.enable_file_logging = true}> logger;
         remove_files({logger.logfile_name()});
         logger.print<log_level::none>("Hello, {}", "world\n");
         logger.print<log_level::info>("Hello, {}", "world\n");
         logger.print<log_level::warn>("Hello, {}", "world\n");
         logger.print<log_level::error>("Hello, {}", "world\n");
         logger.print<log_level::exception>("Hello, {}", "world\n");
         auto actual = logger.str();
         constexpr auto expected =
            "Hello, world\ninfo: Hello, world\nwarn: Hello, world\nerror: Hello, world\nexception: Hello, world\n";
         expect(actual == expected);
      };

      "notarius_t operator() method"_test = [] {
         notarius_t<"test-log-file.md", notarius_opts_t{}> logger;
         logger.enable_file_logging();
         remove_files({logger.logfile_name()});
         logger.operator()<log_level::none>("Hello, {}", "world\n");
         logger.operator()<log_level::info>("Hello, {}", "world\n");
         logger.operator()<log_level::warn>("Hello, {}", "world\n");
         logger.operator()<log_level::error>("Hello, {}", "world\n");
         logger.operator()<log_level::exception>("Hello, {}", "world\n");
         auto actual = logger.str();
         constexpr auto expected =
            "Hello, world\ninfo: Hello, world\nwarn: Hello, world\nerror: Hello, world\nexception: Hello, world\n";
         expect(actual == expected);
      };

      "notarius_t write method"_test = [] {
         notarius_t<"test-log-file.md", notarius_opts_t{.enable_file_logging = true}> logger;
         remove_files({logger.logfile_name()});
         logger.write<log_level::none>("Hello, {}", "world\n");
         logger.write<log_level::info>("Hello, {}", "world\n");
         logger.write<log_level::warn>("Hello, {}", "world\n");
         logger.write<log_level::error>("Hello, {}", "world\n");
         logger.write<log_level::exception>("Hello, {}", "world\n");
         auto actual = logger.str();
         constexpr auto expected =
            "Hello, world\ninfo: Hello, world\nwarn: Hello, world\nerror: Hello, world\nexception: Hello, world\n";
         expect(actual == expected);
      };

      "notarius_t cout method"_test = [] {
         notarius_t<"test-log-file.md", notarius_opts_t{}> logger;
         logger.enable_stdout();
         std_stream_redirection_t redirect_stdout_to_logger(std::cout, logger.rdbuf());
         logger.cout<log_level::none>("Hello, {}", "world\n");
         logger.cout<log_level::info>("Hello, {}", "world\n");
         logger.cout<log_level::warn>("Hello, {}", "world\n");
         logger.cout<log_level::error>("Hello, {}", "world\n");
         logger.cout<log_level::exception>("Hello, {}", "world\n");
         auto actual = logger.str();
         constexpr auto expected =
            "Hello, world\ninfo: Hello, world\nwarn: Hello, world\nerror: Hello, world\nexception: Hello, world\n";
         expect(actual == expected);
      };

      "notarius_t cerr method"_test = [] {
         notarius_t<"test-log-file.md", notarius_opts_t{}> logger;
         logger.enable_stderr();
         std_stream_redirection_t redirect_stdout_to_logger(std::cerr, logger.rdbuf());
         logger.cerr<log_level::none>("Hello, {}", "world\n");
         logger.cerr<log_level::info>("Hello, {}", "world\n");
         logger.cerr<log_level::warn>("Hello, {}", "world\n");
         logger.cerr<log_level::error>("Hello, {}", "world\n");
         logger.cerr<log_level::exception>("Hello, {}", "world\n");
         auto actual = logger.str();
         constexpr auto expected =
            "Hello, world\ninfo: Hello, world\nwarn: Hello, world\nerror: Hello, world\nexception: Hello, world\n";
         expect(actual == expected);
      };

      "notarius_t clog method"_test = [] {
         notarius_t<"test-log-file.md", notarius_opts_t{}> logger;
         logger.enable_stdlog();
         std::ostringstream oss;
         std_stream_redirection_t redirect(std::clog, oss.rdbuf());
         logger.cerr<log_level::none>("Hello, {}", "world\n");
         logger.cerr<log_level::info>("Hello, {}", "world\n");
         logger.cerr<log_level::warn>("Hello, {}", "world\n");
         logger.cerr<log_level::error>("Hello, {}", "world\n");
         logger.cerr<log_level::exception>("Hello, {}", "world\n");
         auto actual = logger.str();
         constexpr auto expected =
            "Hello, world\ninfo: Hello, world\nwarn: Hello, world\nerror: Hello, world\nexception: Hello, world\n";
         expect(actual == expected);
      };

      "notarius_t operator<< method"_test = [] {
         notarius_t<"test-log-file.md", notarius_opts_t{}> logger;
         logger.enable_file_logging();
         remove_files({logger.logfile_name()});
         logger << std::format("Hello, {}", "world\n");
         logger << std::format("{}: Hello, {}", "info", "world\n");
         logger << std::format("{}: Hello, {}", "warn", "world\n");
         logger << std::format("{}: Hello, {}", "error", "world\n");
         logger << std::format("{}: Hello, {}", "exception", "world\n");
         auto actual = logger.str();
         constexpr auto expected =
            "Hello, world\ninfo: Hello, world\nwarn: Hello, world\nerror: Hello, world\nexception: Hello, world\n";
         expect(actual == expected);
      };

      "notarius_t default options"_test = [] {
         notarius_t<"test-log-file.md", notarius_opts_t{}> logger;
         expect(logger.options().enable_stdout == true);
         expect(logger.options().enable_stderr == true);
         expect(logger.options().enable_stdlog == false);
         expect(logger.options().enable_file_logging == false);
      };

      "notarius_t log to file"_test = [] {
         slx::notarius_t<"test_log.md", notarius_opts_t{}> logger;
         logger.enable_file_logging();
         logger("This is a test log entry.");
         logger.close();
         std::ifstream log_file(logger.logfile_path());
         expect(log_file.is_open() == true);
         std::string log_content;
         std::getline(log_file, log_content);
         expect(log_content == "This is a test log entry.");
         log_file.close();
         remove_files({logger.logfile_path()});
      };
   };
};

int main()
{
   run_api_tests();

   init_notarius_logging_options_t init;

#ifdef SPDLOG
   // Turn off the time stamp output in spdlog.
   spdlog::set_pattern("%v");
#endif

   // Note: Not all markdown editors support html color attributes.
   notarius_results_logger("<span style=\"color:#2F4C99\">**Date / Time Run {}:**</span>\n",
                           slx::chrono::current_time_as_string());

   for (auto i = 0; i < test_runs_to_perform; ++i) {
      spdlog_vs_notarius_tests(std::format("Run {}", i + 1), test_runs_to_perform);
   }

   report_final_results();

   fflush(stdout);

   return 0;
}
