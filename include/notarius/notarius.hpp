#pragma once

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstring>
#include <deque>
#include <filesystem>
#include <format>
#include <fstream>
#include <future>
#include <iostream>
#include <memory_resource>
#include <mutex>
#include <shared_mutex>
#include <stdexcept>
#include <string>
#include <type_traits>

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

      constexpr const std::string_view sv() const noexcept { return {value, length}; }

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

   template <typename T>
   concept is_standard_ostream = std::is_same_v<T, std::ostream> || std::is_same_v<T, std::ostream&>;

   // E.g.,  ostream_flusher_t<std::ostream&> flshr{std::cout};
   template <is_standard_ostream T, size_t Capacity = 512>
   struct ostream_flusher_t final
   {
      T ostream_;
      std::string memory_buffer_;
      std::shared_mutex mtx_;
      std::future<void> flushing_future_;

      constexpr size_t capacity() const { return Capacity; }
      size_t size() const { return memory_buffer_.size(); }

      void flush_if_not_empty()
      {
         if (!memory_buffer_.empty()) {
            flush_async();
         }
      }

      static inline void disable_sync_with_stdio() { std::ios::sync_with_stdio(false); }

      auto& append(std::string_view sv)
      {
         std::unique_lock lock(mtx_);
         if ((memory_buffer_.size() + sv.size()) >= Capacity) {
            flush_async();
         }
         memory_buffer_.append(sv.data());
         return *this;
      }

      void flush_async()
      {
         if (!flushing_future_.valid() ||
             flushing_future_.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
            flushing_future_ = std::async(std::launch::async, [this] {
               std::unique_lock lock(this->mtx_);
               this->flush();
            });
         }
      }

      void flush()
      {
         if (memory_buffer_.empty()) return;
         std::unique_lock lock(mtx_, std::try_to_lock);
         ostream_ << memory_buffer_;
         ostream_.flush();
         memory_buffer_.clear();
      }

      ~ostream_flusher_t() { flush(); }
   };

   // clang-format off
   enum class log_level : int {
      none,        // none will insert an empty string, "", for the logging level in the message.
      info,        // label will be 'info'
      warn,        // etc.
      error,
      exception,
   };
   // clang-format on

   inline constexpr const char* to_string(const log_level level)
   {
      constexpr std::array<const char*, 6> log_strings = {
         "logging_off:", /*none:*/ "", "info: ", "warn: ", "error: ", "exception: "};
      if (int(level) >= log_strings.size()) return "";
      return log_strings[int(level)];
   }

   // Default Logging Settings.
   //
   struct notarius_opts_t
   {
      bool lock_free_enabled{false};

      // If immediate_mode is true, all output is written directly
      // to the console or terminal. Otherwise, the output is cached
      // until the cache reaches its maximum size, at which point
      // the cache is flushed.
      //
      bool immediate_mode{false};

      bool enable_stdout{true};

      bool enable_stderr{true};

      bool enable_stdlog{true};

      bool enable_file_logging{true};

      bool open_file_store_for_appending{true};

      bool append_to_log{true};

      // Split log files when they get to a certain size.
      //
      bool split_log_files{true};

      // The max allowable size of a log file (this is ignored when 'split_log_files' is false).
      //
      size_t split_max_log_file_size_bytes_{2'097'152}; // ~2MB
   };

   template <slx::string_literal Name = "", notarius_opts_t Options = notarius_opts_t{},
             slx::string_literal FileExtension = "log">
   class notarius_t final
   {
     private:
      size_t sum_msg_reserve{128};
      size_t dt{0};
      std::future<void> flushing_future_;

     public:
      static constexpr std::string_view file_name_v{Name};
      static constexpr std::string_view file_extension_v{FileExtension};

      std::shared_mutex mutex_;

      std::string log_output_file_path_{create_output_path(file_name_v, file_extension_v)};

      auto logfile_path() const { return log_output_file_path_; }

      notarius_opts_t options_{Options};

      // Flush the logging store to the ostream when it reaches this capacity.
      //
      size_t flush_at_bytes_{Options.split_max_log_file_size_bytes_ / 2};

      size_t file_split_at_bytes_{Options.split_max_log_file_size_bytes_};

      // Note:
      // booleans are not atomic, so we need to use the mutex.
      // Note that std::atomic<bool> and std::atomic_bool are not copyable.
      // We could use std::shared_ptr<std::atomic_bool> but that is overkill
      // since the class generally needs to support a std::shared_mutex already.

      // toggle writing to the ostream on/logging_off at some logging point in your code.
      bool toggle_immediate_mode_ = {false};

      // enable/disable flushing to a log file
      bool flush_to_log_file_{true};

      // Pause file logging.
      //
      void pause_file_logging()
      {
         std::unique_lock lock(mutex_);
         options_.enable_file_logging_ = false;
      }
      void enable_file_logging()
      {
         std::unique_lock lock(mutex_);
         options_.enable_file_logging_ = true;
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

      ostream_flusher_t<std::ostream&, 512> cout_buffer_{std::cout};
      ostream_flusher_t<std::ostream&, 512> cerr_buffer_{std::cout};
      ostream_flusher_t<std::ostream&, 512> clog_buffer_{std::cout};

      // The logging store.
      // using std::string as the store is slightly faster:
      // std::deque<std::string> logging_store_;
      //
      std::string logging_store_;

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

      inline void open_log_output_stream()
      {
         if (not options_.enable_file_logging) {
            return;
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
      }

      // See: https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p2508r1.html
      //
      template <log_level level = log_level::none, typename... Args>
      void print(std::format_string<Args...> fmt, Args&&... args)
      {
         std::unique_lock<std::shared_mutex> cs(mutex_, std::defer_lock);
         if (not options_.lock_free_enabled) {
            cs.lock();
         }

         std::string msg;
         msg.reserve((sum_msg_reserve / ++dt));

         if (logging_store_.capacity() < file_split_at_bytes_) logging_store_.reserve(file_split_at_bytes_);

         if constexpr (log_level::none == level)
            msg = std::format(fmt, std::forward<Args>(args)...);
         else
            msg = to_string(level) + std::format(fmt, std::forward<Args>(args)...);

         sum_msg_reserve += msg.size();

         if (options_.enable_stdout) {
            if (toggle_immediate_mode_ || options_.immediate_mode) {
               toggle_immediate_mode_ = false;

               if (log_level::error == level) {
                  cerr_buffer_.append(msg);
                  cerr_buffer_.flush_if_not_empty();
               } else {
                  cout_buffer_.append(msg);
                  cout_buffer_.flush_if_not_empty();
               }
            }
            else {
               if (log_level::error == level) {
                  cerr_buffer_.append(msg);
               }
               else {
                  cout_buffer_.append(msg);
               }
            }
         }

         if (options_.enable_file_logging) {
            if (options_.split_log_files) {
               if (logging_store_.size() >= file_split_at_bytes_) {
                  flush();
                  log_output_stream_.close();
                  log_output_file_path_ = slx::io::get_next_available_filename(log_output_file_path_);
               }
            }
            if (logging_store_.size() >= flush_at_bytes_) flush_async();
            logging_store_.append(std::move(msg));
         }

         return;
      }

      template <typename T>
      void print(const T& msg)
      {
         print("{}", msg);
      }

      template <typename T>
      void print(T&& msg)
      {
         print("{}", std::forward<T>(msg));
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

      template <log_level level, typename... Args>
      void std_flush_impl(ostream_flusher_t<std::ostream&, 512>& buffer, std::format_string<Args...> fmt, Args&&... args)
      {
         std::unique_lock<std::shared_mutex> lock(mutex_, std::defer_lock);
         if (not options_.lock_free_enabled) {
            lock.lock();
         }

         std::string msg;
         msg.reserve((sum_msg_reserve / ++dt));
         if constexpr (level != log_level::none) {
            msg = to_string(level) + std::format(fmt, std::forward<Args>(args)...);
         }
         else {
            msg = std::format(fmt, std::forward<Args>(args)...);
         }

         buffer.append(msg);
         buffer.flush_if_not_empty();
      }

      // Writes to std::cout and then flushes
      //
      template <log_level level = log_level::none, typename... Args>
      void cout(std::format_string<Args...> fmt, Args&&... args)
      {
         std_flush_impl<level>(cout_buffer_, fmt, std::forward<Args>(args)...);
      }

      // Writes to std::cerr and then flushes
      //
      template <log_level level = log_level::none, typename... Args>
      void cerr(std::format_string<Args...> fmt, Args&&... args)
      {
         std_flush_impl<level>(cerr_buffer_, fmt, std::forward<Args>(args)...);
      }

      // Writes to std::clog and then flushes
      //
      template <log_level level = log_level::none, typename... Args>
      void clog(std::format_string<Args...> fmt, Args&&... args)
      {
         std_flush_impl<level>(clog_buffer_, fmt, std::forward<Args>(args)...);
      }

      // Note:
      //
      //  logger << "something else...", "foo", 4, 5, 6, "\n";
      //
      //   ... will be more efficient than:
      //
      //   md_log << "Logging with '<<' operator: " << "something else...";
      //
      template <log_level level = log_level::none, typename... Args>
      friend auto& operator<<(notarius_t<Name, Options, FileExtension>& n, Args&&... args)
      {
         n.print<level>("{}", std::forward<Args>(args)...);
         return n;
      }

      void flush_async()
      {
         if (!flushing_future_.valid() ||
             flushing_future_.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
            flushing_future_ = std::async(std::launch::async, [this] {
               std::unique_lock lock(this->mutex_);
               this->flush();
            });
         }
      }

      void flush()
      {
         if (not flush_to_log_file_ or logging_store_.empty()) return;

         auto flush_ = [&]() {
            if (options_.enable_stdout) {
               cout_buffer_.flush();
            }
            open_log_output_stream();
            log_output_stream_.write(logging_store_.c_str(), logging_store_.size());
            log_output_stream_.flush();
            logging_store_.clear();
         };

         if (options_.lock_free_enabled) {
            flush_();
         }
         else {
            std::unique_lock lock(mutex_, std::try_to_lock);
            flush_();
         }
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

      void toggle_immediate_mode()
      {
         std::unique_lock lock(mutex_);
         toggle_immediate_mode_ = true;
      }

      void close()
      {
         flush();
         log_output_stream_.close();
      }

      void remove_log_file()
      {
         namespace fs = std::filesystem;

         std::unique_lock lock(mutex_);

         if (log_output_stream_.is_open()) log_output_stream_.close();

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

      void clear()
      {
         std::unique_lock lock(mutex_);
         logging_store_.clear();
      }

      // Capacity at which the logging buffer will be forced to flush.
      //
      auto capacity() const { return flush_at_bytes_; }

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

     private:
      std::ofstream log_output_stream_;

      // Try C style file stream.
      //
      // slx::io::file_io_t log_output_stream_;
   };
} // namespace slx
