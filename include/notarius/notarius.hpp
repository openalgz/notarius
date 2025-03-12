#pragma once

#ifdef NOTARIUS_MODULE
module notarius;
#endif

#include <algorithm>
#include <array>
#include <barrier>
#include <cassert>
#include <charconv>
#include <compare>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <filesystem>
#include <format>
#include <fstream>
#include <functional>
#include <future>
#include <iostream>
#include <mutex>
#include <optional>
#include <queue>
#include <shared_mutex>
#include <stop_token>
#include <streambuf>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <type_traits>
#include <vector>

#if defined(USE_STD_PRINT)
#if defined(_MSC_VER)
#define CPP_VERSION _MSVC_LANG
#else
#define CPP_VERSION __cplusplus
#endif
#if __has_include(<print>) && CPP_VERSION >= 202302L
#include <print>
#define USE_STD_PRINT
#endif
#endif

namespace slx
{
   /** 'string_literal' code was acquired from the glaze library: https://github.com/stephenberry/glaze.git

     Glaze Library

     Copyright (c) 2019 - present, Stephen Berry

     Permission is hereby granted, free of charge, to any person obtaining
     a copy of this software and associated documentation files (the
     "Software"), to deal in the Software without restriction, including
     without limitation the rights to use, copy, modify, merge, publish,
     distribute, sublicense, and/or sell copies of the Software, and to
     permit persons to whom the Software is furnished to do so, subject to
     the following conditions:

     The above copyright notice and this permission notice shall be
     included in all copies or substantial portions of the Software.

     THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
     EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
     MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
     NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
     LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
     OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
     WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

     --- Optional exception to the license ---

     As an exception, if, as a result of your compiling your source code, portions
     of this Software are embedded into a machine-executable object form of such
     source code, you may redistribute such embedded portions in such object form
     without including the above copyright and permission notices.
    */
   using sv = std::string_view;

#ifdef NOTARIUS_MODULE
   template <size_t N>
   struct string_literal
   {
      static constexpr size_t length = (N > 0) ? (N - 1) : 0;

      constexpr size_t size() const noexcept { return length; }
      constexpr string_literal() noexcept = default;
      constexpr string_literal(const char (&str)[N]) noexcept { std::copy_n(str, N, value); }

      char value[N];

      constexpr const char* begin() const noexcept { return value; }
      constexpr const char* end() const noexcept { return value + length; }

      constexpr bool operator==(const string_literal& other) const noexcept
      {
         return std::equal(begin(), end(), other.begin(), other.end());
      }

      constexpr bool operator<(const string_literal& other) const noexcept
      {
         for (size_t i = 0; i < length && i < other.length; ++i) {
            if (value[i] < other.value[i]) return true;
            if (value[i] > other.value[i]) return false;
         }
         return length < other.length;
      }

      constexpr bool operator>(const string_literal& other) const noexcept
      {
         return other < *this; // using 'operator <' for reversed comparison
      }

      constexpr bool operator<=(const string_literal& other) const noexcept { return !(*this > other); }
      constexpr bool operator>=(const string_literal& other) const noexcept { return !(*this < other); }

      constexpr std::string_view sv() const noexcept { return {value, length}; }
      constexpr operator std::string_view() const noexcept { return {value, length}; }
   };
#else
   template <size_t N>
   struct string_literal
   {
      static constexpr size_t length = (N > 0) ? (N - 1) : 0;
      constexpr size_t size() const noexcept { return length; }
      constexpr string_literal() noexcept = default;
      constexpr string_literal(const char (&str)[N]) noexcept { std::copy_n(str, N, value); }
      char value[N];
      constexpr const char* begin() const noexcept { return value; }
      constexpr const char* end() const noexcept { return value + length; }
      //
      // NOT Compatible with CPP Modules at this time:
      //
      [[nodiscard]] constexpr auto operator<=>(const string_literal&) const = default;
      constexpr std::string_view sv() const noexcept { return {value, length}; }
      constexpr operator std::string_view() const noexcept { return {value, length}; }
   };

#endif

   template <size_t N>
   constexpr auto string_literal_from_view(sv str)
   {
      string_literal<N + 1> sl{};
      std::copy_n(str.data(), str.size(), sl.value);
      *(sl.value + N) = '\0';
      return sl;
   }

   template <size_t N>
   constexpr size_t length(const char (&)[N]) noexcept
   {
      return N;
   }

   template <string_literal Str>
   struct chars_impl
   {
      static constexpr std::string_view value{Str.value, length(Str.value) - 1};
   };

   template <string_literal Str>
   inline constexpr std::string_view chars = chars_impl<Str>::value;

   template <size_t N>
   struct fixed_string final
   {
      constexpr explicit(true) fixed_string(const auto... cs) : data{cs...} {}
      constexpr explicit(false) fixed_string(const char (&str)[N + 1]) { std::copy_n(str, N + 1, data.data()); }
      [[nodiscard]] constexpr auto operator<=>(const fixed_string&) const = default;
      [[nodiscard]] constexpr explicit(false) operator std::string_view() const { return {data.data(), N}; }
      [[nodiscard]] constexpr auto size() const -> std::size_t { return N; }
      std::array<char, N + 1> data{};
   };

   template <size_t N>
   fixed_string(const char (&str)[N]) -> fixed_string<N - 1>;

   // end of glaze library code (https://github.com/stephenberry/glaze.git)

   // RAII implementation to manage stream redirection
   //
   struct std_stream_redirection_t final
   {
      // Constructor: redirects the stream
      // Parameters:
      //   - this_stream: the stream to be redirected (e.g., std::clog) to 'other_stream'
      //   - other_stream: pointer to the new stream buffer (e.g., file buffer)
      //
      // Redirect
      std_stream_redirection_t(std::ostream& this_stream, std::streambuf* other_stream)
         : redirected_stream_(this_stream), original_rdbuf_(this_stream.rdbuf(other_stream))
      {}

      // Resets the stream to its original buffer
      inline void reset() { redirected_stream_.rdbuf(original_rdbuf_); }

      // Sets the stream to a new buffer
      inline void reset(std::streambuf* new_buf) { redirected_stream_.rdbuf(new_buf); }

      // Destructor: restores the original stream buffer
      ~std_stream_redirection_t() { reset(); }

     private:
      std::ostream& redirected_stream_; // Reference to the stream being redirected
      std::streambuf* original_rdbuf_; // Pointer to the original stream buffer
   };

   inline bool are_rdbufs_equal(std::ostream& lhv, std::ostream& rhv) { return lhv.rdbuf() == rhv.rdbuf(); }

   /*
   Example use case:

   #include <fstream>
   #include <iostream>

   int main() {

       std::ofstream logFile("log.txt");

       if (!logFile.is_open()) {
           std::cerr << "Failed to open log file." << std::endl;
           return 1;
       }

       {
           // Redirect std::clog to logFile
           std_stream_redirection_t redirected_clog(std::clog, logFile.rdbuf());

           // Test the redirection
           std::clog << "This goes to the notarius log file." << std::endl;

       } // The original stream buffer is restored here when redirector goes out of scope

       // This will go to the original destination of std::clog (usually the terminal)
       std::clog << "This goes to the original std::clog destination." << std::endl;

       logFile.close();

       return 0;
   }
   */

   template <typename T>
   concept is_formattable = requires(T t) {
      {
         std::format("{}", t)
      } -> std::convertible_to<std::string>;
   };

   template <typename T>
   concept is_streamable = requires(std::ostream& os, T t) {
      {
         os << t
      } -> std::same_as<std::ostream&>;
   };

   template <typename T>
   concept is_loggable = is_formattable<T> || is_streamable<T>;

   template <typename T>
   concept is_filesystem_path_convertable = requires(T t) { std::filesystem::path(t); };

   template <typename T>
   concept is_standard_ostream = std::is_base_of_v<std::ostream, std::remove_reference_t<T>>;

   template <is_loggable... Args>
   void cout(std::format_string<Args...> fmt, Args&&... args)
   {
#ifdef USE_STD_PRINT
      std::print(fmt, std::forward<Args>(args)...);
#else
      std::cout << std::format(fmt, std::forward<Args>(args)...);
#endif
   }

   template <is_loggable T>
   void cout(const T& msg)
   {
      using namespace std::literals;
      cout("{}"sv, msg);
   }

   template <is_loggable T>
   void cout(T&& msg)
   {
      using namespace std::literals;
      cout("{}"sv, std::forward<T>(msg));
   }

   template <is_filesystem_path_convertable T>
   std::string get_filename(const T& path)
   {
      return std::filesystem::path(path).filename().string();
   }

   template <is_filesystem_path_convertable T>
   std::string get_log_file_path(const T& path)
   {
      auto p = std::filesystem::absolute(path);
      return p.string();
   }

   template <bool publish = false>
   inline int remove_files_in_directory(const std::filesystem::path& directory, const std::string_view extension)
   {
      namespace fs = std::filesystem;

      int count_removed{};

      if (!fs::exists(directory) || !fs::is_directory(directory)) {
         if constexpr (publish) {
            std::cerr << "The specified path is not a directory or does not exist." << std::endl;
         }
         return count_removed;
      }

      for (const auto& entry : fs::directory_iterator(directory)) {
         if (!entry.is_regular_file() || entry.path().extension() != extension) {
            continue;
         }

         try {
            if (fs::remove(entry.path())) {
               ++count_removed;
               if constexpr (publish) {
                  std::cout << "Deleted: " << entry.path() << std::endl;
               }
            }
            else {
               if constexpr (publish) {
                  std::cerr << "Failed to delete: " << entry.path() << std::endl;
               }
            }
         }
         catch (const std::exception& ex) {
            if constexpr (publish) {
               std::cerr << "Error deleting " << entry.path() << ": " << ex.what() << std::endl;
            }
         }
      }

      return count_removed;
   }

   inline void remove_files(const std::vector<std::string>& files)
   {
      for (const auto& file : files) {
         if (std::filesystem::exists(file)) {
            std::filesystem::remove(file);
         }
      }
   }

   template <int max_file_index = 100>
   inline std::string get_next_available_filename(const std::string_view input_path_name,
                                                  const std::string_view default_extension = ".log")
   {
      static const auto max_file_index_exceeded_msg =
         std::format("Warning: The max file limit of {} has been reached.", max_file_index);

      if (!std::filesystem::exists(input_path_name)) return input_path_name.data();

      std::filesystem::path p = input_path_name;

      std::string directory = p.parent_path().string();
      if (directory.empty()) {
         directory = std::filesystem::current_path().string();
      }

      std::string extension = p.extension().string();
      if (extension.empty()) {
         extension = default_extension;
      }

      std::string filename = p.stem().string();

      const size_t last_underscore_pos = filename.find_last_of('_');
      if (last_underscore_pos != std::string::npos) {
         const std::string after_underscore = filename.substr(last_underscore_pos + 1);
         const bool numeric = std::all_of(after_underscore.begin(), after_underscore.end(), ::isdigit);
         if (numeric) {
            filename.erase(last_underscore_pos);
         }
      }

      for (size_t i = 1; i <= static_cast<size_t>(max_file_index); ++i) {
         std::string tmp = std::format("{}/{}_{}{}", directory, filename, i, extension);
         if (!std::filesystem::exists(tmp)) {
            return tmp;
         }
      }

      throw std::runtime_error(max_file_index_exceeded_msg);
   }

   // clang-format off
   /**
    * @brief Enumeration representing the different log levels.
    */
   enum class log_level : int {
       none,        ///< No logging level is included in the message.
       info,        ///< Information log level: label is 'info'.
       warn,        ///< Warning log level: label is 'warn'.
       error,       ///< Error log level: label is 'error'.
       exception,   ///< Exception log level: label is 'exception'.
       //
       ignore       ///< Sentinel value. Must be the last value in the enumeration.
       //
       // When adding or modifying this you must also update 'to_string(const log_level level)'
       // defined below.
   };
   // clang-format on

   /**
    * @brief Acquires enumeration representing the different log levels in a const char*.
    */
   inline constexpr const char* to_string(const log_level level)
   {
      constexpr std::array<const char*, static_cast<int>(log_level::ignore)> log_strings = {
         /*none:*/ "", "info", "warn", "error", "exception"};
      if (int(level) >= log_strings.size()) return "";
      return log_strings[int(level)];
   }

   /// @brief Defines default configuration options for the notarius logging system.
   struct notarius_opts_t
   {
      bool enable_file_logging{false}; ///< Enable logging to file.

      bool lock_free_enabled{false}; ///< Flag to enable lock-free logging.

      /**
       * @brief If immediate_mode is true, all data is written directly
       *        to enabled standard outputs (std::cout, std::cerr, and
       *        std::clog). Otherwise, the output is cached until the
       *        cache reaches its maximum size, at which point the cache
       *        associated with a given standard output is flushed.
       */
      bool immediate_mode{true};

      /** Note
       *  By default, std::cout, std::cerr, and std::clog often point to
       *  the same ostream. Therefore, enabling two or more may result in
       *  duplication of messages written to your terminal or console.
       */
      /// @name Enable/Disable Standard Outputs
      /// @{
      bool enable_stdout{true}; ///< Enable writing  to standard output.
      bool enable_stderr{true}; ///< Enable writing to standard error.
      bool enable_stdlog{false}; ///< Enable writing to standard log.
      /// @}

      // @brief Enable logging when calling notarius_t::cout, cerr, and clog.
      //        the default is not write to the log file when using these
      //        notarius APIs.
      //
      /// @name Enable/Disable Standard Outputs' logging
      /// @{
      bool log_from_stdout{false}; ///< Enable logging from standard output.
      bool log_from_stderr{false}; ///< Enable logging from standard error.
      bool log_from_stdlog{false}; ///< Enable logging from standard log.
      /// @}

      bool append_to_log{true}; ///< Append to the log file instead of overwriting.

      bool append_newline_when_missing{false}; ///< Append a newline when missing
                                               ///< at the end of a log entry.

      /**
       * @brief Split log files when they get to a certain size.
       *
       * If true, log files will be split into multiple files when they reach
       * the specified size limit (split_log_file_at_size_bytes).
       *
       * 'enable_file_logging' must be true
       *
       */
      bool split_log_files{true};

      /**
       * Benefit: Disabling buffering ensures that each write operation to
       * the file is immediately reflected in the file system. This can be
       * beneficial when you need to ensure that data is written promptly
       * without waiting for a buffer to fill up.
       *
       * Trade-off: The immediate write approach can lead to increased
       * system call overhead. Each write operation results in a system
       * call to write data to the file, which can be relatively slow
       * compared to writing to an in-memory buffer.
       *
       * Therefore the performance of this feature is effected by your
       * 'flush_to_log_at_bytes' size.
       */
      bool disable_file_buffering{true};

      /**
       * @brief Flush to stdout, stderr, or stdlog when this size is exceeded.
       *
       * When the log buffer reaches this size, it will be flushed to the
       * respective standard output streams (stdout, stderr, or stdlog).
       */
      size_t flush_to_std_outputs_at_bytes{1024};

      /**
       * @brief The maximum allowable size of a log file.
       *
       * This option is ignored when 'split_log_files' is false.
       * When a log file reaches this size, it will be split into a new file.
       *
       * 'enable_file_logging' must be true
       *
       */
      size_t split_log_file_at_size_bytes{1'048'576 * 25}; // 25 MB

      /**
       * @brief Flush to the log file when this size is exceeded.
       *
       * When the log buffer reaches this size, it will be flushed to the log file.
       *
       * 'enable_file_logging' must be true
       *
       */
      size_t flush_to_log_at_bytes{1'048'576 * 16}; // 16 MB
   };

   struct output_as_json_t
   {
      std::unordered_map<std::string, std::vector<std::string>> data;
   };

   /**
      @brief A logger class for writing log messages to a file.
      @tparam LogFileNameOrPath The file or path name of the logger. If not provided, it defaults to 'notatarius'.
      @tparam Options The options for configuring the logger. Defaults to an empty notarius_opts_t struct.
   */
   template <slx::string_literal LogFileNameOrPath, notarius_opts_t Options>
   struct notarius_t final
   {
     private:
      static constexpr std::string_view default_logger_name_or_path{LogFileNameOrPath};

      mutable std::string log_output_file_path_{get_log_file_path(default_logger_name_or_path.data())};

      std::shared_ptr<std::shared_mutex> mutex_ = std::make_shared<std::shared_mutex>();

      // The logging store.
      // using std::string as the store is slightly faster:
      // std::deque<std::string> logging_store_;
      //
      std::string logging_store_;
      std::string cout_store_;
      std::string cerr_store_;
      std::string clog_store_;

      std::atomic_bool reserve_once{true};

      void reserve_store_capacities()
      {
         if (logging_store_.capacity() < options_.split_log_file_at_size_bytes) {
            logging_store_.reserve(options_.split_log_file_at_size_bytes);
         }

         if (cout_store_.capacity() < options_.flush_to_std_outputs_at_bytes) {
            cout_store_.reserve(options_.flush_to_std_outputs_at_bytes);
         }

         if (cerr_store_.capacity() < options_.flush_to_std_outputs_at_bytes) {
            cerr_store_.reserve(options_.flush_to_std_outputs_at_bytes);
         }

         if (cout_store_.capacity() < options_.flush_to_std_outputs_at_bytes) {
            cerr_store_.reserve(options_.flush_to_std_outputs_at_bytes);
         }
      }

      std::ofstream log_output_stream_;

      // Toggle writing to the ostream on/logging_off at some logging point in your code.
      //
      std::atomic_bool toggle_immediate_mode_ = {false};

      notarius_opts_t options_{Options};

      std::shared_ptr<std::shared_mutex> get_mutex() { return mutex_; }

      // Method to get a unique lock for exclusive write access
      std::unique_lock<std::shared_mutex> get_exclusive_write_lock()
      {
         return std::unique_lock<std::shared_mutex>(*mutex_);
      }

      // Method to get a shared lock for shared read access
      std::shared_lock<std::shared_mutex> get_shared_read_lock() const
      {
         return std::shared_lock<std::shared_mutex>(*mutex_);
      }

      // Method to try acquiring an exclusive write lock with retries
      std::optional<std::unique_lock<std::shared_mutex>> try_exclusive_write_lock(const int max_attempts = 3,
                                                                                  const int delay_ms = 10)
      {
         std::unique_lock<std::shared_mutex> cs(*mutex_, std::try_to_lock);

         for (int attempts = 0; !cs.owns_lock() && attempts < max_attempts; ++attempts) {
            std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
            if (cs.try_lock()) break;
         }

         if (cs.owns_lock()) {
            return std::optional<std::unique_lock<std::shared_mutex>>(std::move(cs));
         }

         return std::nullopt;
      }

      // A delegate to forward a log message to. This is run on a separate thread.
      //
      std::function<void(std::string_view)> forward_to;

      // notarius helper method; flush a msg to an ostream
      //
      template <log_level level, bool flush = true, is_loggable... Args>
      void update_io_buffer(std::ostream& buffer, std::format_string<Args...> fmt, Args&&... args)
      {
         static thread_local std::string msg;

         if constexpr (log_level::none == level)
            msg = std::format(fmt, std::forward<Args>(args)...);
         else
            msg = std::format("{}: {}", to_string(level), std::format(fmt, std::forward<Args>(args)...));

         buffer.write(msg.c_str(), msg.size());

         buffer.flush();

         msg.clear();
      }

      void flush_cout()
      {
         if (are_rdbufs_equal(log_output_stream_, std::cout)) {
            flush();
            cout_store_.clear();
            return;
         }
         if (cout_store_.empty()) return;
         std::cout << cout_store_;
         cout_store_.clear();
         std::cout.flush();
      }

      void flush_cerr()
      {
         if (are_rdbufs_equal(log_output_stream_, std::cerr)) {
            flush();
            cerr_store_.clear();
            return;
         }
         if (cerr_store_.empty()) return;
         std::cerr << cerr_store_;
         cerr_store_.clear();
         std::cerr.flush();
      }

      void flush_clog()
      {
         if (are_rdbufs_equal(log_output_stream_, std::clog)) {
            flush();
            clog_store_.clear();
            return;
         }
         if (clog_store_.empty()) return;
         std::clog << clog_store_;
         clog_store_.clear();
         std::clog.flush();
      }

      void flush_std_outputs()
      {
         flush_cout();
         flush_cerr();
         flush_clog();
      }

      // Called from 'print' only...do not call from other locations!
      //
      void write_to_std_output_stores(const std::string& msg, log_level level)
      {
         if (!options_.enable_file_logging &&
             (options_.enable_stdout || options_.enable_stderr || options_.enable_stdlog)) {
            options_.enable_file_logging = true;
         }

         if (not options_.enable_stdout and not options_.enable_stderr and not options_.enable_stdlog) return;

         auto immediate_mode = options_.immediate_mode || toggle_immediate_mode_;

         toggle_immediate_mode_ = false;

         if (options_.enable_stdout && level <= log_level::warn) {
            if (are_rdbufs_equal(log_output_stream_, std::cout)) {
               return;
            }
            else
               cout_store_.append(msg);

            if (immediate_mode || (cout_store_.size() >= options_.flush_to_std_outputs_at_bytes)) {
               flush_cout();
            }
         }

         if (options_.enable_stderr && level >= log_level::error) {
            if (are_rdbufs_equal(log_output_stream_, std::cerr)) {
               return;
            }
            else
               cerr_store_.append(msg);

            if (immediate_mode || (cerr_store_.size() >= options_.flush_to_std_outputs_at_bytes)) {
               flush_cerr();
            }
         }

         if (options_.enable_stdlog) {
            if (are_rdbufs_equal(log_output_stream_, std::clog)) {
               return;
            }
            else
               clog_store_.append(msg);

            if (immediate_mode || (clog_store_.size() >= options_.flush_to_std_outputs_at_bytes)) {
               flush_clog();
            }
         }
      }

      void flush_impl()
      {
         flush_std_outputs();

         if (logging_store_.empty()) return;

         if (options_.enable_file_logging) {
            open_log_output_stream();

            // Note:
            //
            // The following will write data to the file stream, either directly if buffering is
            // disabled(pubsetbuf(0, 0)),or indirectly via an internal buffer if buffering is enabled.
            //
            // For details see where 'options_.disable_file_buffering' is being used.
            //
            log_output_stream_.write(logging_store_.c_str(), logging_store_.size());
            log_output_stream_.flush();
         }

         logging_store_.clear();
      };

     public:
      auto& options() { return options_; }

      std::string logfile_path()
      {
         auto lock = get_shared_read_lock();
         return log_output_file_path_;
      }

      [[maybe_unused]] std::string set_log_file_path(const std::string_view path)
      {
         close();
         auto lock = get_exclusive_write_lock();
         log_output_file_path_ = get_log_file_path(path);
         return log_output_file_path_;
      }

      std::string logfile_name()
      {
         if (log_output_file_path_.empty()) {
            log_output_file_path_ = get_log_file_path(std::string(LogFileNameOrPath));
         }
         auto lock = get_shared_read_lock();
         return get_filename(log_output_file_path_);
      }

      std::string default_extension = ".log"; // a default extension when a user does not use one

      std::streambuf* rdbuf()
      {
         open_log_output_stream();
         return log_output_stream_.rdbuf();
      }

      void pause_file_logging() { options_.enable_file_logging = false; }
      void enable_file_logging() { options_.enable_file_logging = true; }

      void pause_stdout() { options_.enable_stdout = false; }
      void enable_stdout() { options_.enable_stdout = true; }

      void pause_stderr() { options_.enable_stderr = false; }
      void enable_stderr() { options_.enable_stderr = true; }

      void pause_stdlog() { options_.enable_stdlog = false; }
      void enable_stdlog() { options_.enable_stdlog = true; }

      // This should be called at the beginning of main() or when the logger is created.
      // This results in a significant increase in stdio performance.
      inline static void disable_sync_with_stdio() { std::ios::sync_with_stdio(false); }

      inline void check_log_file_destination_path(std::string& path)
      {
         namespace fs = std::filesystem;

         if (path.empty() || fs::exists(path)) return;

         fs::path p = path;

         fs::path directory = p.parent_path();

         if (directory.empty()) {
            directory = fs::current_path();
         }

         if (!fs::exists(directory)) {
            fs::create_directories(directory);
         }

         path = fs::absolute(p).string();
      }

      inline bool open_log_output_stream()
      {
         if (not options_.enable_file_logging) {
            return false;
         }

         if (log_output_stream_.is_open()) return true;

         if (not log_output_stream_.is_open()) {
            check_log_file_destination_path(log_output_file_path_);

            if (options_.append_to_log)
               log_output_stream_.open(log_output_file_path_, std::ios_base::app);
            else
               log_output_stream_.open(log_output_file_path_);
         }

         if (not log_output_stream_.is_open()) {
            std::error_code ec = std::make_error_code(std::errc::io_error);
            throw std::system_error(
               ec, std::format("Error opening log file '{}' (error code: {})!", log_output_file_path_, ec.message()));
         }
         else {
            if (options_.disable_file_buffering) {
               //
               // The following is generally useful for scenarios where immediate
               // and unbuffered output to a file store is helpful, but it can
               // come with performance trade-offs. Since we are buffering the
               // logging by default (see 'std::string logging_store_;'), this
               // will usually be beneficial.
               //
               log_output_stream_.rdbuf()->pubsetbuf(0, 0);
            }
         }

         return log_output_stream_.is_open();
      }

      [[nodiscard]] bool is_open() const { return log_output_stream_.is_open(); }

      // See: https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p2508r1.html
      //
      template <log_level level = log_level::none, is_loggable... Args>
      void print(std::format_string<Args...> fmt, Args&&... args)
      {
         std::unique_lock<std::shared_mutex> cs(*mutex_, std::defer_lock);

         if (not options_.lock_free_enabled) {
            cs.lock();
         }

         static thread_local std::string msg;

         if (reserve_once) {
            reserve_once = false;
            reserve_store_capacities();
         }

         if constexpr (log_level::none == level) msg = std::format(fmt, std::forward<Args>(args)...);
         // msg = std::vformat(fmt.get(), std::make_format_args(args...));
         else {
            msg = std::format("{}: {}", to_string(level), std::format(fmt, std::forward<Args>(args)...));
            // auto inner_formatted = std::vformat(fmt.get(), std::make_format_args(std::forward<Args>(args)...));
            // msg = std::format("{}: {}", to_string(level), inner_formatted);
         }

         if (options_.append_newline_when_missing) {
            if (not msg.empty() and '\n' != msg.back()) {
               msg.append("\n");
            }
         }

         write_to_std_output_stores(msg, level);

         if (forward_to) forward_to(msg);

         const size_t check_size = logging_store_.size() + msg.size();

         if (options_.split_log_files and (check_size >= options_.split_log_file_at_size_bytes)) {
            flush_impl();
            if (options_.enable_file_logging) {
               log_output_stream_.close();
               log_output_file_path_ = get_next_available_filename(log_output_file_path_, default_extension);
            }
         }
         else if (logging_store_.size() >= options_.flush_to_log_at_bytes) {
            flush_impl();
         }

         logging_store_.append(std::move(msg));
      }

      template <log_level level = log_level::none, is_loggable T>
      void print(const T& msg)
      {
         using namespace std::literals;
         print<level>("{}"sv, msg);
      }

      template <log_level level = log_level::none, is_loggable T>
      void print(T&& msg)
      {
         using namespace std::literals;
         print<level>("{}"sv, std::forward<T>(msg));
      }

      template <is_loggable... Args>
      void info(std::format_string<Args...> fmt, Args&&... args)
      {
         print<log_level::info>(fmt, std::forward<Args>(args)...);
      }

      template <is_loggable T>
      void info(const T& msg)
      {
         using namespace std::literals;
         print<log_level::info>("{}"sv, msg);
      }

      template <is_loggable T>
      void info(T&& msg)
      {
         using namespace std::literals;
         print<log_level::info>("{}"sv, std::forward<T>(msg));
      }

      template <is_loggable... Args>
      void warn(std::format_string<Args...> fmt, Args&&... args)
      {
         print<log_level::warn>(fmt, std::forward<Args>(args)...);
      }

      template <is_loggable T>
      void warn(const T& msg)
      {
         using namespace std::literals;
         print<log_level::warn>("{}"sv, msg);
      }

      template <is_loggable T>
      void warn(T&& msg)
      {
         using namespace std::literals;
         print<log_level::warn>("{}"sv, std::forward<T>(msg));
      }

      template <is_loggable... Args>
      void error(std::format_string<Args...> fmt, Args&&... args)
      {
         print<log_level::error>(fmt, std::forward<Args>(args)...);
      }

      template <is_loggable T>
      void error(const T& msg)
      {
         using namespace std::literals;
         print<log_level::error>("{}"sv, msg);
      }

      template <is_loggable T>
      void error(T&& msg)
      {
         using namespace std::literals;
         print<log_level::error>("{}"sv, std::forward<T>(msg));
      }

      template <log_level level = log_level::none, is_loggable... Args>
      void operator()(std::format_string<Args...> fmt, Args&&... args)
      {
         print<level>(fmt, std::forward<Args>(args)...);
      }

      template <is_loggable T>
      void operator()(const T& msg)
      {
         using namespace std::literals;
         print("{}"sv, msg);
      }

      template <is_loggable T>
      void operator()(T&& msg)
      {
         using namespace std::literals;
         print("{}"sv, std::forward<T>(msg));
      }

      // Always writes immediately to console while also logging
      // the message to a file store.
      template <log_level level = log_level::none, is_loggable... Args>
      auto write(std::format_string<Args...> fmt, Args&&... args)
      {
         toggle_immediate_mode();
         return print<level>(fmt, std::forward<Args>(args)...);
      }

      // Writes to std::cout and then flushes
      //
      template <log_level level = log_level::none, bool flush = true, is_loggable... Args>
      void cout(std::format_string<Args...> fmt, Args&&... args)
      {
         if (not options_.enable_stdout) return;
         if (options_.log_from_stdout)
            print(fmt, std::forward<Args>(args)...);
         else
            update_io_buffer<level, flush>(std::cout, fmt, std::forward<Args>(args)...);
      }

      template <log_level level = log_level::none, bool flush = true, is_loggable T>
      void cout(const T& msg)
      {
         using namespace std::literals;
         if (not options_.enable_stdout) return;
         cout<level, flush>("{}"sv, msg);
      }

      template <log_level level = log_level::none, bool flush = true, is_loggable T>
      void cout(T&& msg)
      {
         using namespace std::literals;
         if (not options_.enable_stdout) return;
         cout<level, flush>("{}"sv, std::forward<T>(msg));
      }

      // Writes to std::cerr and then flushes
      //
      template <log_level level = log_level::none, bool flush = true, is_loggable... Args>
      void cerr(std::format_string<Args...> fmt, Args&&... args)
      {
         if (not options_.enable_stderr) return;
         if (options_.log_from_stderr)
            print(fmt, std::forward<Args>(args)...);
         else
            update_io_buffer<level, flush>(std::cout, fmt, std::forward<Args>(args)...);
      }

      template <log_level level = log_level::none, bool flush = true, is_loggable T>
      void cerr(const T& msg)
      {
         using namespace std::literals;
         if (not options_.enable_stderr) return;
         cerr<level, flush>("{}"sv, msg);
      }

      template <log_level level = log_level::none, bool flush = true, is_loggable T>
      void cerr(T&& msg)
      {
         using namespace std::literals;
         if (not options_.enable_stderr) return;
         cerr<level, flush>("{}"sv, std::forward<T>(msg));
      }

      template <log_level level = log_level::none, bool flush = true, is_loggable... Args>
      void clog(std::format_string<Args...> fmt, Args&&... args)
      {
         if (not options_.enable_stdlog) return;
         if (options_.log_from_stdlog)
            print(fmt, std::forward<Args>(args)...);
         else
            update_io_buffer<level, flush>(std::cout, fmt, std::forward<Args>(args)...);
      }

      template <log_level level = log_level::none, bool flush = true, is_loggable T>
      void clog(const T& msg)
      {
         using namespace std::literals;
         if (not options_.enable_stdlog) return;
         clog<level, flush>("{}"sv, msg);
      }

      template <log_level level = log_level::none, bool flush = true, is_loggable T>
      void clog(T&& msg)
      {
         using namespace std::literals;
         if (not options_.enable_stdlog) return;
         clog<level, flush>("{}"sv, std::forward<T>(msg));
      }

      template <log_level level = log_level::none, is_loggable... Args>
      friend auto& operator<<(notarius_t<LogFileNameOrPath, Options>& notarius, Args&&... args)
      {
         using namespace std::literals;
         notarius.print<level>("{}"sv, std::forward<Args>(args)...);
         return notarius;
      }

      void flush()
      {
         auto lock = try_exclusive_write_lock();
         if (not lock or logging_store_.empty()) return;
         flush_impl();
      }

      // Note: If append mode is set to false then the contents
      //       of the existing file WILL BE DESTROYED!.
      void append_mode(const bool enable)
      {
         if (enable == options_.append_to_log)
            return; // only process when there has been a change in state (i.e., enable to dis-able or vise versa)
         auto lock = get_exclusive_write_lock();
         close();
         options_.append_to_log = enable;
      }

      void toggle_immediate_mode() { toggle_immediate_mode_ = true; }

      void close()
      {
         auto lock = get_exclusive_write_lock();
         flush_impl();
         log_output_stream_.close();
      }

      void remove_log_file()
      {
         namespace fs = std::filesystem;

         close();

         auto lock = get_exclusive_write_lock();

         if (fs::exists(log_output_file_path_)) {
            try {
               fs::remove(log_output_file_path_);
            }
            catch (...) {
            }
         }

         logging_store_.clear();
      }

      auto size() const
      {
         auto lock = get_shared_read_lock();
         return logging_store_.size();
      }

      auto& write_string(std::string& buffer)
      {
         try {
            close();

            std::ifstream log_buf(log_path().data());
            if (!log_buf.is_open()) return buffer;

            auto lock = get_shared_read_lock(); // Acquire read lock if file is open

            // Using string constructor directly with rdbuf for more efficient handling
            //
            buffer = std::string(std::istreambuf_iterator<char>(log_buf), std::istreambuf_iterator<char>());
         }
         catch (...) {
            assert(false && "'write_string': Unexpected Exception!");
            return buffer;
         }
         return buffer;
      }

      std::string str()
      {
         std::string buffer;
         return write_string(buffer);
      }

      void clear()
      {
         auto lock = get_exclusive_write_lock();
         logging_store_.clear();
      }

      // Capacity at which the logging buffer will be forced to flush.
      //
      auto capacity() const
      {
         auto lock = get_shared_read_lock();
         return logging_store_.capacity();
      }

      const std::string_view log_path()
      {
         auto lock = get_shared_read_lock();
         return log_output_file_path_;
      }

      auto& change_log_path(const std::string_view new_path)
      {
         auto lock = get_exclusive_write_lock();
         log_output_file_path_ = get_log_file_path(new_path);
         return log_output_file_path_;
      }

      auto resize(const size_t size)
      {
         auto lock = get_exclusive_write_lock();
         return logging_store_.resize(size);
      }

      auto reset() { clear(); }

      auto empty() const
      {
         auto lock = get_shared_read_lock();
         return logging_store_.empty();
      }

      auto shrink_to_fit()
      {
         auto lock = get_exclusive_write_lock();
         return logging_store_.shrink_to_fit();
      }

      ~notarius_t()
      {
         try {
            close();
         }
         catch (...) {
            assert(false && "'~notarius_t' Unexpected Exception in notarius_t!");
         }
      }
   };

} // namespace slx

#ifdef NOTARIUS_MODULE
export
{
   namespace slx
   {
      using slx::log_level;
      using slx::notarius_opts_t;
      using slx::notarius_t;
   }
}
#endif
