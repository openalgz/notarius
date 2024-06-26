#pragma once

#include <array>
#include <charconv>
#include <deque>
#include <format>
#include <fstream>
#include <functional>
#include <iostream>
#include <mutex>
#include <streambuf>
#include <string>
#include <string_view>
#include <system_error>

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

      [[nodiscard]] constexpr auto operator<=>(const string_literal&) const = default;

      constexpr std::string_view sv() const noexcept { return {value, length}; }

      constexpr operator std::string_view() const noexcept { return {value, length}; }
   };

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
      //   - stream: the stream to be redirected (e.g., std::clog)
      //   - new_buf: pointer to the new stream buffer (e.g., file buffer)
      //
      std_stream_redirection_t(std::ostream& stream, std::streambuf* new_buf)
         : redirected_stream_(stream), original_rdbuf_(stream.rdbuf(new_buf))
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
   concept is_standard_ostream = std::is_base_of_v<std::ostream, std::remove_reference_t<T>>;

   template <int max_file_index = 100>
   inline std::string get_next_available_filename(const std::string_view input_path_name)
   {
      const auto max_file_index_exceeded_msg =
         "Warning: The max file limit of " + std::to_string(max_file_index) + " has been reached.";

      if (!std::filesystem::exists(input_path_name)) return {input_path_name.data()};

      std::filesystem::path p = input_path_name;
      const std::string directory = p.parent_path().string();
      const std::string extension = p.extension().string();
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
       count        ///< Sentinel value. Must be the last value in the enumeration.

       // When adding or modifying this you must also update 'to_string(const log_level level)'
       // defined below.
   };
   // clang-format on

   /**
    * @brief Acquires enumeration representing the different log levels to a const char*.
    */
   inline constexpr const char* to_string(const log_level level)
   {
      constexpr std::array<const char*, static_cast<int>(log_level::count)> log_strings = {
         /*none:*/ "", "info: ", "warn: ", "error: ", "exception: "};
      if (int(level) >= log_strings.size()) return "";
      return log_strings[int(level)];
   }

   /// @brief Defines default configuration options for the notarius logging system.
   struct notarius_opts_t
   {
      bool lock_free_enabled{false}; ///< Flag to enable lock-free logging.

      /**
       * @brief If immediate_mode is true, all data is written directly
       *        to enabled standard outputs (std::cout, std::cerr, and std::clog).
       *        Otherwise, the output is cached until the cache reaches its maximum size,
       *        at which point the cache associated with a given standard output is flushed.
       */
      bool immediate_mode{true};

      /** Note
       *
       *  By default, std::cout, std::cerr, and std::clog usually point to the same
       *  ostream. Therefore, enabling all three may result in duplication of messages
       *  written to these ostreams when all of them are enabled.
       *
       *  Also the following log level filters are applied to the streams (currently
       *  not configurable):
       *
       *  std::cout ->  level <= log_level::warn
       *  std::cerr ->  level <= log_level::error
       *  std::clog ->  level <= log_level::none
       */
      /// @name Enable/Disable Standard Outputs
      /// @{
      bool enable_stdout{true}; ///< Enable logging to standard output.
      bool enable_stderr{true}; ///< Enable logging to standard error.
      bool enable_stdlog{false}; ///< Enable logging to standard log.
      /// @}

      bool enable_file_logging{false}; ///< Enable logging to file.

      bool append_to_log{true}; ///< Append to the log file instead of overwriting.

      bool append_newline_when_missing{false}; ///< Append a newline when missing at the end of a log entry.

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
       * Benefit: Disabling buffering ensures that each write operation to the file is
       * immediately reflected in the file system. This can be beneficial when you need
       * to ensure that data is written promptly without waiting for a buffer to fill up.
       *
       * Trade-off: The immediate write approach can lead to increased system call overhead.
       * Each write operation results in a system call to write data to the file, which can
       * be relatively slow compared to writing to an in-memory buffer.
       *
       * Therefore the performance of this feature is effected by your 'flush_to_log_at_bytes'
       * size.
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

   /**
      @brief A logger class for writing log messages to a file.
      @tparam Name The name of the logger. If not provided, it defaults to 'notatarius'.
      @tparam Options The options for configuring the logger. Defaults to an empty notarius_opts_t struct.
      @tparam FileExtension The file extension for the log file. Defaults to "log".
   */
   template <slx::string_literal Name = "notatarius", notarius_opts_t Options = notarius_opts_t{},
             slx::string_literal FileExtension = "log">
   struct notarius_t final
   {
     private:
      static constexpr std::string_view file_name_v{Name};
      static constexpr std::string_view file_extension_v{FileExtension};

      mutable std::mutex mutex_; // mutable for const functions.

      // The logging store.
      // using std::string as the store is slightly faster:
      // std::deque<std::string> logging_store_;
      //
      std::string logging_store_;
      std::string cout_store_;
      std::string cerr_store_;
      std::string clog_store_;

      bool reserve_once{true};

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

      std::string log_output_file_path_{create_output_path(file_name_v, file_extension_v)};

      // Toggle writing to the ostream on/logging_off at some logging point in your code.
      //
      std::atomic_bool toggle_immediate_mode_ = {false};

      notarius_opts_t options_{Options};

      // notarius helper method; flush a msg to an ostream
      //
      template <log_level level, bool flush = true, typename... Args>
      void update_io_buffer(std::ostream& buffer, std::format_string<Args...> fmt, Args&&... args)
      {
         static thread_local std::string msg;

         if constexpr (level != log_level::none) {
            msg = to_string(level) + std::format(fmt, std::forward<Args>(args)...);
         }
         else {
            msg = std::format(fmt, std::forward<Args>(args)...);
         }

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

      auto logfile_path() const { return log_output_file_path_; }

      auto logfile_name() -> std::string { return std::format("./{}.{}", file_name_v, file_extension_v); }

      std::streambuf* rdbuf()
      {
         open_log_output_stream();
         return log_output_stream_.rdbuf();
      }

      void pause_file_logging()
      {
         std::unique_lock lock(mutex_);
         options_.enable_file_logging = false;
      }
      void enable_file_logging()
      {
         std::unique_lock lock(mutex_);
         options_.enable_file_logging = true;
      }

      void pause_stdout()
      {
         std::unique_lock lock(mutex_);
         options_.enable_stdout = false;
      }
      void enable_stdout()
      {
         std::unique_lock lock(mutex_);
         options_.enable_stdout = true;
      }

      void pause_stderr()
      {
         std::unique_lock lock(mutex_);
         options_.enable_stderr = false;
      }
      void enable_stderr()
      {
         std::unique_lock lock(mutex_);
         options_.enable_stderr = true;
      }

      void pause_stdlog()
      {
         std::unique_lock lock(mutex_);
         options_.enable_stdlog = false;
      }
      void enable_stdlog()
      {
         std::unique_lock lock(mutex_);
         options_.enable_stdlog = true;
      }

      // This should be called at the beginning of main() or when the logger is created.
      // This results in a significant increase in stdio performance.
      inline static void disable_sync_with_stdio() { std::ios::sync_with_stdio(false); }

      // The following routines manage the log output (file store) path:
      //
      inline void check_log_file_path() { create_output_path(log_output_file_path_, file_name_v, file_extension_v); }

      inline void create_output_path(std::string& output_path, const std::string_view file_name_v,
                                     const std::string_view file_extension_v)
      {
         namespace fs = std::filesystem;

         if (output_path.empty()) {
            output_path = std::format("./{}.{}", file_name_v, file_extension_v);
         }

         if (!fs::exists(output_path)) {
            const auto absolute_path = fs::absolute(output_path);
            const auto destination_folder = absolute_path.parent_path();
            if (!fs::exists(destination_folder)) fs::create_directory(destination_folder);
         }
      }

      inline auto create_output_path(const std::string_view file_name_v, const std::string_view file_extension_v)
      {
         std::string p;
         create_output_path(p, file_name_v, file_extension_v);
         return p;
      }

      inline bool open_log_output_stream()
      {
         if (not options_.enable_file_logging) {
            return false;
         }

         check_log_file_path();

         if (not log_output_stream_.is_open()) {
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
               // logging info already, this may be beneficial.
               //
               log_output_stream_.rdbuf()->pubsetbuf(0, 0);
            }
         }

         return log_output_stream_.is_open();
      }

      bool is_open() const { return log_output_stream_.is_open(); }

      // See: https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p2508r1.html
      //
      template <log_level level = log_level::none, typename... Args>
      void print(std::format_string<Args...> fmt, Args&&... args)
      {
         std::unique_lock cs(mutex_, std::defer_lock);

         if (not options_.lock_free_enabled) {
            cs.lock();
         }

         static thread_local std::string msg;

         if (reserve_once) {
            reserve_once = false;
            reserve_store_capacities();
         }

         if constexpr (log_level::none == level)
            msg = std::format(fmt, std::forward<Args>(args)...);
         else
            msg = to_string(level) + std::format(fmt, std::forward<Args>(args)...);

         if (options_.append_newline_when_missing) {
            if (not msg.empty() and '\n' != msg.back()) {
               msg.append("\n");
            }
         }

         write_to_std_output_stores(msg, level);

         const size_t check_size = logging_store_.size() + msg.size();

         if (options_.split_log_files and (check_size >= options_.split_log_file_at_size_bytes)) {
            flush_impl();
            if (options_.enable_file_logging) {
               log_output_stream_.close();
               log_output_file_path_ = get_next_available_filename(log_output_file_path_);
            }
         }

         if (logging_store_.size() >= options_.flush_to_log_at_bytes) {
            flush_impl();
         }

         logging_store_.append({msg});
      }

      template <log_level level = log_level::none, typename T>
      void print(const T& msg)
      {
         print<level>("{}", msg);
      }

      template <log_level level = log_level::none, typename T>
      void print(T&& msg)
      {
         print<level>("{}", std::forward<T>(msg));
      }

      template <log_level level = log_level::none, typename... Args>
      void operator()(std::format_string<Args...> fmt, Args&&... args)
      {
         print<level>(fmt, std::forward<Args>(args)...);
      }

      template <typename T>
      void operator()(const T& msg)
      {
         print("{}", msg);
      }

      template <typename T>
      void operator()(T&& msg)
      {
         print("{}", std::forward<T>(msg));
      }

      // Always writes immediately to console while also logging
      // the message to a file store.
      template <log_level level = log_level::none, typename... Args>
      auto write(std::format_string<Args...> fmt, Args&&... args)
      {
         toggle_immediate_mode();
         return print<level>(fmt, std::forward<Args>(args)...);
      }

      // Writes to std::cout and then flushes
      //
      template <log_level level = log_level::none, bool flush = true, typename... Args>
      void cout(std::format_string<Args...> fmt, Args&&... args)
      {
         if (not options_.enable_stdout) return;
         update_io_buffer<level, flush>(std::cout, fmt, std::forward<Args>(args)...);
      }

      template <log_level level = log_level::none, bool flush = true, typename T>
      void cout(const T& msg)
      {
         if (not options_.enable_stdout) return;
         cout<level, flush>("{}", msg);
      }

      template <log_level level = log_level::none, bool flush = true, typename T>
      void cout(T&& msg)
      {
         if (not options_.enable_stdout) return;
         cout<level, flush>("{}", std::forward<T>(msg));
      }

      // Writes to std::cerr and then flushes
      //
      template <log_level level = log_level::none, bool flush = true, typename... Args>
      void cerr(std::format_string<Args...> fmt, Args&&... args)
      {
         if (not options_.enable_stderr) return;
         update_io_buffer<level, flush>(std::cerr, fmt, std::forward<Args>(args)...);
      }

      template <log_level level = log_level::none, bool flush = true, typename T>
      void cerr(const T& msg)
      {
         if (not options_.enable_stderr) return;
         cerr<level, flush>("{}", msg);
      }

      template <log_level level = log_level::none, bool flush = true, typename T>
      void cerr(T&& msg)
      {
         if (not options_.enable_stderr) return;
         cerr<level, flush>("{}", std::forward<T>(msg));
      }

      template <log_level level = log_level::none, bool flush = true, typename... Args>
      void clog(std::format_string<Args...> fmt, Args&&... args)
      {
         if (not options_.enable_stdlog) return;
         update_io_buffer<level, flush>(std::clog, fmt, std::forward<Args>(args)...);
      }

      template <log_level level = log_level::none, bool flush = true, typename T>
      void clog(const T& msg)
      {
         if (not options_.enable_stdlog) return;
         clog<level, flush>("{}", msg);
      }

      template <log_level level = log_level::none, bool flush = true, typename T>
      void clog(T&& msg)
      {
         if (not options_.enable_stdlog) return;
         clog<level, flush>("{}", std::forward<T>(msg));
      }

      template <log_level level = log_level::none, typename... Args>
      friend auto& operator<<(notarius_t<Name, Options, FileExtension>& notarius, Args&&... args)
      {
         notarius.print<level>("{}", std::forward<Args>(args)...);
         return notarius;
      }

      void flush()
      {
         if (logging_store_.empty()) return;
         std::unique_lock cs(mutex_, std::try_to_lock);
         if (not cs.owns_lock()) {
            return;
         }
         flush_impl();
      }

      // Note: If append mode is set to false then the contents
      //       of the existing file WILL BE DESTROYED!.
      void append_mode(const bool enable)
      {
         if (enable == options_.append_to_log) return;
         std::unique_lock lock(mutex_);
         close();
         options_.append_to_log = enable;
      }

      void toggle_immediate_mode() { toggle_immediate_mode_ = true; }

      void close()
      {
         std::unique_lock lock(mutex_);
         flush_impl();
         log_output_stream_.close();
      }

      void remove_log_file()
      {
         namespace fs = std::filesystem;

         close();

         std::unique_lock lock(mutex_);

         if (fs::exists(log_output_file_path_)) {
            try {
               fs::remove(log_output_file_path_);
            }
            catch (...) {
            }
         }

         logging_store_.clear();
      }

      auto size() const { return logging_store_.size(); }

      // TODO: use std::error_code?
      //
      bool to_string(std::string& buffer)
      {
         try {
            close();

            std::ifstream log_buf(log_path().data());

            if (not log_buf) return false;

            std::unique_lock lock(mutex_);

            std::ostringstream os;

            os << log_buf.rdbuf();

            // cast 'os' to an rvalue reference, allowing the move
            // constructor of std::string to be invoked.
            //
            buffer = {std::move(os).str()};
         }
         catch (...) {
            return false;
         }
         return true;
      }

      std::string to_string()
      {
         std::string b;

         return { to_string(b); };
      }

      void clear()
      {
         std::unique_lock lock(mutex_);
         logging_store_.clear();
      }

      // Capacity at which the logging buffer will be forced to flush.
      //
      auto capacity() const { return logging_store_.capacity(); }

      const std::string_view log_path() const
      {
         std::unique_lock lock(mutex_);
         return log_output_file_path_;
      }

      auto& change_log_path(const std::string_view new_path)
      {
         std::unique_lock lock(mutex_);
         log_output_file_path_ = new_path;
         check_log_file_path();
         return log_output_file_path_;
      }

      auto resize(const size_t size)
      {
         std::unique_lock lock(mutex_);
         return logging_store_.resize(size);
      }

      auto reset() { clear(); }

      auto empty() const { return logging_store_.empty(); }

      auto shrink_to_fit()
      {
         std::unique_lock lock(mutex_);
         return logging_store_.shrink_to_fit();
      }

      ~notarius_t()
      {
         try {
            close();
         }
         catch (...) {
         }
      }
   };
} // namespace slx
