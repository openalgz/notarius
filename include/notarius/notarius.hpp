#pragma once

#include <algorithm>
#include <array>
#include <cstdio>
#include <deque>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <memory_resource>
#include <mutex>
#include <shared_mutex>
#include <stdexcept>
#include <string>
#include <syncstream>

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

      constexpr string_literal(const char(&str)[N]) noexcept { std::copy_n(str, N, value); }

      char value[N];
      constexpr const char* begin() const noexcept { return value; }
      constexpr const char* end() const noexcept { return value + length; }

      [[nodiscard]] constexpr auto operator<=>(const string_literal&) const = default;

      constexpr const std::string_view sv() const noexcept { return { value, length }; }

      constexpr operator std::string_view() const noexcept { return { value, length }; }
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
   constexpr size_t length(const char(&)[N]) noexcept
   {
      return N;
   }

   template <string_literal Str>
   struct chars_impl
   {
      static constexpr std::string_view value{ Str.value, length(Str.value) - 1 };
   };

   template <string_literal Str>
   inline constexpr std::string_view chars = chars_impl<Str>::value;

   template <size_t N>
   struct fixed_string final
   {
      constexpr explicit(true) fixed_string(const auto... cs) : data{ cs... } {}
      constexpr explicit(false) fixed_string(const char(&str)[N + 1]) { std::copy_n(str, N + 1, data.data()); }
      [[nodiscard]] constexpr auto operator<=>(const fixed_string&) const = default;
      [[nodiscard]] constexpr explicit(false) operator std::string_view() const { return { data.data(), N }; }
      [[nodiscard]] constexpr auto size() const -> std::size_t { return N; }
      std::array<char, N + 1> data{};
   };

   template <size_t N>
   fixed_string(const char(&str)[N]) -> fixed_string<N - 1>;

   // end of glaze library code (https://github.com/stephenberry/glaze.git)

   enum class log_level : int {
      logging_off, // Compiled code will never log; set notarius_t::enable_file_logging_ to false to turn logging_off logging at runtime.
      stdout_off,  // Compiled code will never stream to stdout
      stderr_off,  // Compiled code will never stream to stderr
      none, // none is going to "" 'blank' for the logging level in the message.
      info, // label will be 'info'
      warn, // etc.
      error,
      exception,
   };

   inline constexpr const char* to_string(const log_level level)
   {
      constexpr std::array<const char*, 6> log_strings = { "logging_off:", /*none:*/ "", "info: ", "warn: ", "error: ", "exception: " };
      if (int(level) >= log_strings.size()) return "";
      return log_strings[int(level)];
   }

   // Default Logging Settings.
   //
   struct notarius_opts
   {
      bool lock_free_enabled{ false };

      // If immediate_mode is true, all output is written directly
      // to the console or terminal. Otherwise, the output is cached
      // until the cache reaches its maximum size, at which point 
      // the cache is flushed.
      //
      bool immediate_mode{ false };

      bool enable_stdout{ false };

      bool enable_stderr{ false };

      bool enable_file_logging{ true };

      bool open_file_store_for_appending{ true };

      // Split log files when they get to a certain size.
      //
      bool split_log_files{ false };

      // The max allowable size of a log file (this is ignored when 'split_log_files' is false).
      //  
      size_t split_max_log_file_size_bytes_{ 2'097'152 }; // ~2MB
   };

   // TODO: Optimize...std::pmr, circular buffer/allocator, etc.
   //
   //       With C++ 23 cout may be updated with std::print.
   //
   //  Expectation is that user of this class provides synchronization.
   //  Therefore, this is not thread-safe as implemented.
   //
   template <size_t Capacity = 512>
   struct ostream_flusher_t
   {
      std::ostream* ostream_ptr_{ stdout };
      std::string memory_buffer_;
      constexpr size_t capacity() const { return Capacity; }
      size_t size() const { return memory_buffer_.size(); }
      size_t assumed_max_string_size{ 64 };

      void flush_if_not_empty()
      {
         if (not memory_buffer_.empty()) flush();
      }

      static inline void disable_sync_with_stdio() {
         std::ios::sync_with_stdio(false);
      }

      auto& append(std::string_view sv)
      {
         if ((memory_buffer_.size() + 1) >= Capacity) flush();
         memory_buffer_.append(sv.data());
         return *this;
      }

      auto& append(std::string_view sv, std::ostream& os)
      {
         ostream_ptr_ = &os;
         return append(sv);
      }

      void flush()
      {
         if (!ostream_ptr_ || memory_buffer_.empty()) return;
         (*ostream_ptr_) << memory_buffer_;
         memory_buffer_.clear();
      }

      ~ostream_flusher_t() { flush(); }
   };

   template<slx::string_literal Name = "", notarius_opts Options = notarius_opts{}, slx::string_literal FileExtension = "log" >
   struct notarius_t
   {
      static constexpr std::string_view file_name_v{ Name };
      static constexpr std::string_view file_extension_v{ FileExtension };

      std::string log_output_file_path_{ create_output_path(file_name_v, file_extension_v) };

      auto logfile_path() const { return log_output_file_path_; }

      std::shared_mutex mutex_;

      // Flush the logging store to the ostream when it reaches this capacity.
      //
      size_t flush_at_bytes_{ std::numeric_limits<size_t>::max() };

      size_t file_split_at_bytes_{ Options.split_max_log_file_size_bytes_ };

      // The ostream to write to. The default is std::cout.
      //
      std::ostream& ostream_ptr_ = std::cout;

      // Note:
      // booleans are not atomic, so we need to use the mutex.
      // Note that std::atomic<bool> and std::atomic_bool are not copyable.
      // We could use std::shared_ptr<std::atomic_bool> but that is overkill
      // since the class generally needs to support a std::shared_mutex already.

      // toggle writing to the ostream on/logging_off at some logging point in your code.
      bool toggle_immediate_mode_ = { false };

      // enable/disable flushing to a log file
      bool flush_to_log_file_{ true };

      // enable/disable appending to a log file
      //
      // If this is 'false' then the file will be deleted when opened
      // and therefore a new file will be created when the logger opens
      // the target file store.
      //
      bool append_mode_{ Options.open_file_store_for_appending };

      // enable/disable file logging. If this is 'true' then file logging
      // is paused and therefore ignored.
      //
      bool enable_file_logging_ = { Options.enable_file_logging };

      // Pause file logging.
      //
      void pause_file_logging(bool) {
         std::unique_lock lock(mutex_);
         enable_file_logging_ = false;
      }

      // Pause stdout. If enabled steaming to stdout will be ignored.
      bool pause_stdout_{};

      // Pause stderr. If enabled steaming to stderr will be ignored.
      bool pause_stderr_{};

      void pause_stdout(bool pause) {
         std::unique_lock lock(mutex_);
         pause_stdout_ = pause;
      }
      void pause_stderr(bool pause) {
         std::unique_lock lock(mutex_);
         pause_stderr_ = pause;
      }

      // Flush the ostream when it reaches the assigned capacity.
      // This can be used to improve performance to a console or terminal,
      //
      ostream_flusher_t</*capacity:*/ 512> ostream_buffer_{ .ostream_ptr_ = &ostream_ptr_ };

      // The logging store.
      //
      // [[experimental("todo: experiment with std::pmr in the future")]]
      // static const std::size_t prm_pool_size = 1024 * 8;
      // alignas(alignof(std::max_align_t)) char logging_memoryl[prm_pool_size];
      // std::pmr::monotonic_buffer_resource pool{logging_memoryl, prm_pool_size};
      // std::pmr::polymorphic_allocator<char> alloc{&pool};
      // std::pmr::deque<std::string> logging_store_{ std::pmr::polymorphic_allocator<std::string>{&pool}};
      //
      //std::deque<std::string> logging_store_;
      std::string logging_store_;

      // This should be called at the beginning of main() or when the logger is created.
      // This results in a significant increase in stdio performance. 
      inline static void disable_sync_with_stdio() { std::ios::sync_with_stdio(false); }

      // The following routines manage the log output (file store) path:
      // 
      inline void check_log_file_path() { create_output_path(log_output_file_path_, file_name_v, file_extension_v); }

      inline void create_output_path(std::string& output_path, const std::string_view file_name_v, const std::string_view file_extension_v)
      {
         namespace fs = std::filesystem;

         if (output_path.empty()) {
            output_path = std::format("./{}.{}", file_name_v, file_extension_v);
         }

         if (!fs::exists(output_path)) {
            const auto absolute_path = fs::absolute(output_path);
            const auto destination_folder = absolute_path.parent_path();
            if (!fs::exists(destination_folder))
               fs::create_directory(destination_folder);
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
         if constexpr (not Options.enable_file_logging) {
            return;
         }
         else
         {
            check_log_file_path();

            if (not log_output_stream_.is_open()) {
               if (append_mode_)
                  log_output_stream_.open(log_output_file_path_, std::ios_base::app);
               else
                  log_output_stream_.open(log_output_file_path_);
            }

            if (not log_output_stream_.is_open()) {
               std::error_code ec = std::make_error_code(std::errc::io_error);
               throw std::system_error(ec, std::format("Error opening log file '{}' (error code: {})!", log_output_file_path_, ec.message()));
            }
         }
      }

      //void write_ostream_async(const std::string& msg) {
      //   std::async(std::launch::async, [this, msg]() {
      //      ostream_buffer_.flush_if_not_empty();
      //      ostream_ptr_ << msg;
      //      });
      //}

      // See: https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p2508r1.html
      //
      template <log_level level = log_level::none, typename... Args>
      void print(std::format_string<Args...> fmt, Args&&... args)
      {
         if constexpr (log_level::logging_off == level) {
            return;
         }
         else
         {
            if (not enable_file_logging_) return;

            std::string msg;

            if (logging_store_.capacity() < file_split_at_bytes_) logging_store_.reserve(file_split_at_bytes_);

            if constexpr (log_level::none == level)
               msg = std::format(fmt, std::forward<Args>(args)...);
            else
               msg = to_string(level) + std::format(fmt, std::forward<Args>(args)...);

            auto update_log = [msg = std::move(msg), this]()
               {

                  if constexpr (Options.enable_stdout)
                  {
                     if (toggle_immediate_mode_ || Options.immediate_mode) {
                        toggle_immediate_mode_ = false;
                        //
                        // It may be desirable to think about this differently.
                        //
                        ostream_buffer_.flush_if_not_empty();
                        ostream_ptr_ << msg;
                     }
                     else {
                        ostream_buffer_.append(msg, ostream_ptr_);
                     }
                  }

                  if constexpr (Options.split_log_files)
                  {
                     if (logging_store_.size() >= file_split_at_bytes_)
                     {
                        flush();
                        log_output_stream_.close();
                        log_output_file_path_ = slx::io::get_next_available_filename(log_output_file_path_);
                     }
                  }

                  if (logging_store_.size() >= flush_at_bytes_)  flush();
  
                  logging_store_.append(std::move(msg));
               };


            if constexpr (Options.lock_free_enabled)
            {
               update_log();
            }
            else
            {
               std::unique_lock lock(mutex_);
               update_log();
            }
            return;
         }
      }

      template <typename T>
      void print(const T& msg) {
         print("{}", msg);
      }

      template <typename T>
      void print(T&& msg) {
         print("{}", std::forward<T>(msg));
      }

      template <log_level level = log_level::none, typename... Args>
      void operator()(std::format_string<Args...> fmt, Args&&... args) {
         print<level>(fmt, std::forward<Args>(args)...);
      }

      template <typename T>
      void operator()(const T& msg) {
         print("{}", msg);
      }

      template <typename T>
      void operator()(T&& msg) {
         print("{}", std::forward<T>(msg));
      }

      // Always writes immediately to console while also logging
      // the message to a file store.
      template <log_level level = log_level::none, typename... Args>
      auto write(std::format_string<Args...> fmt, Args&&... args) {
         toggle_immediate_mode();
         return print<level>(fmt, std::forward<Args>(args)...);
      }

      template <log_level level = log_level::none, typename... Args>
      void cout(std::format_string<Args...> fmt, Args&&... args)
      {
         if constexpr (not Options.enable_stdout) {
            return;
         }
         else {
            if (pause_stdout_) return;

            static std::string msg;

            if constexpr (log_level::none == level)
               msg = std::format(fmt, std::forward<Args>(args)...);
            else
               msg = to_string(level) + std::format(fmt, std::forward<Args>(args)...);

            if constexpr (Options.lock_free_enabled)
            {
               std::cout << msg;
            }
            else
            {
               std::unique_lock lock(mutex_);
               std::cout << msg;
            }
         }
      }

      template <log_level level = log_level::none, typename... Args>
      void cerr(std::format_string<Args...> fmt, Args&&... args)
      {
         if constexpr (not Options.enable_stderr) {
            return;
         }
         else {
            if (pause_stderr_) return;

            static std::string msg;

            if constexpr (log_level::none == level)
               msg = std::format(fmt, std::forward<Args>(args)...);
            else
               msg = to_string(level) + std::format(fmt, std::forward<Args>(args)...);

            if constexpr (Options.lock_free_enabled)
            {
               std::cerr << msg;
            }
            else
            {
               std::unique_lock lock(mutex_);
               std::cerr << msg;
            }
         }
      }

      // Note:
      // 
      //  logger << "something else...", "foo", 4, 5, 6, "\n";
      // 
      //   ... will be more efficient than:
      // 
      //   md_log << "Logging with '<<' operator: " << "something else...";
      //   
      template<log_level level = log_level::none, typename... Args>
      friend auto& operator << (notarius_t<Name, Options, FileExtension>& n, Args&&... args)
      {
         n.print<level>("{}", std::forward<Args>(args)...);
         return n;
      }

      void flush()
      {
         if (not flush_to_log_file_ or logging_store_.empty()) return;

          auto flush_ = [&]() {

             if constexpr (Options.enable_stdout)
             {
                ostream_buffer_.flush();
             }

            open_log_output_stream();
            //log_output_stream_ << logging_store_;
            log_output_stream_.write(logging_store_.c_str(), logging_store_.size());
            log_output_stream_.flush();
            logging_store_.clear();
            };

         if constexpr (Options.lock_free_enabled)
         {
            flush_();
         }
         else
         {
            std::unique_lock lock(mutex_, std::try_to_lock);
            flush_();
         }
      }

      // Note: If append mode is set to false then the contents
      //       of the existing file WILL BE DESTROYED!.
      void append_mode(const bool enable)
      {
         if (enable == append_mode_) return;
         std::unique_lock lock(mutex_);
         close();
         append_mode_ = enable;
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
            try
            {
               fs::remove(log_output_file_path_);
            }
            catch (...) {}
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

      const std::string_view log_path() const {
         std::unique_lock lock(mutex_);
         return log_output_file_path_;
      }

      auto& change_log_path(const std::string_view new_path) {
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
         try { close(); }
         catch (...) {}
      }

   private:
      std::ofstream log_output_stream_;
      
      // Try C style file stream.
      //
      //slx::io::file_io_t log_output_stream_;
   };
} // namespace slx
