#pragma once

#include <cstdio>
#include <iostream>
#include <system_error>

#include "format.hpp"

namespace slx
{
   namespace io {

      template <int max_file_index = 100>
      inline std::string& get_next_available_filename(std::string& input_path_name) {
         const auto max_file_index_exceeded_msg =
            "Warning: The max file limit of " + std::to_string(max_file_index) + " has been reached.";

         if (!std::filesystem::exists(input_path_name)) return input_path_name;

         // Find the extension (if any) or end of string...
         //
         size_t ext_pos = input_path_name.find_last_of('.');
         std::string base_name = input_path_name.substr(0, ext_pos);
         std::string extension = (ext_pos != std::string::npos) ? input_path_name.substr(ext_pos) : "";

         size_t underscore = base_name.find_last_of('_');
         base_name = base_name.substr(0, underscore);

         for (size_t i = 1; i <= static_cast<size_t>(max_file_index); ++i) {
            input_path_name = fmt_string("{}_{}{}", base_name, i, extension);
            if (!std::filesystem::exists(input_path_name)) {
               return input_path_name;
            }
         }

         throw std::runtime_error(max_file_index_exceeded_msg);
      }

      template <int max_file_index = 100>
      inline std::string get_next_available_filename(const std::string_view input_path_name) {
         const auto max_file_index_exceeded_msg =
            "Warning: The max file limit of " + std::to_string(max_file_index) + " has been reached.";

         if (!std::filesystem::exists(input_path_name)) return input_path_name.data();

         size_t ext_pos = input_path_name.find_last_of('.');
         std::string base_name = input_path_name.substr(0, ext_pos).data();
         std::string extension = (ext_pos != std::string::npos) ? input_path_name.substr(ext_pos).data() : "";

         size_t underscore = base_name.find_last_of('_');
         base_name = base_name.substr(0, underscore);

         for (size_t i = 1; i <= static_cast<size_t>(max_file_index); ++i) {
            std::string tmp = fmt_string("{}_{}{}", base_name, i, extension);
            if (!std::filesystem::exists(tmp)) {
               return tmp;
            }
         }

         throw std::runtime_error(max_file_index_exceeded_msg);
      }

      constexpr const char* map_ios_flags(std::ios_base::openmode mode) {
         switch (mode) {
         case std::ios_base::in:
            return "r";

         case std::ios_base::out:
            return "w";

         case std::ios_base::app:
            return "a";

         case std::ios_base::in | std::ios_base::out:
            return "r+";

         case std::ios_base::out | std::ios_base::trunc:
            return "w+";

         case std::ios_base::app | std::ios_base::out:
            return "a+";

         default:
            return "";
         }
      }

      struct file_mode {
         static constexpr auto read{ "r" };
         static constexpr auto write{ "w" };
         static constexpr auto append{ "a" };
         static constexpr auto read_update{ "r+" };
         static constexpr auto write_update{ "w+" };
         static constexpr auto append_update{ "a+" };

         // "r": Open for reading. The file must exist.
         // 
         // "w": Open for writing. If the file does not exist, it will be created. 
         // If the file exists, its contents will be discarded.
         // 
         // "a": Open for appending. If the file does not exist, it will be created. 
         // All output operations will write data at the end of the file.
         // 
         // "r+": Open for reading and writing. The file must exist.
         // 
         // "w+": Open for reading and writing. If the file does not exist, it will be
         // created. If the file exists, its contents will be discarded.
         // 
         // "a+": Open for reading and appending. If the file does not exist, it will be
         // created. All output operations will write data at the end of the file.
      };

      inline void write_to_file(FILE* file_stream_ptr, const char* data, size_t size) {
         if (!file_stream_ptr) {
            throw std::runtime_error("File is not open for writing.");
         }
         size_t written = fwrite(data, 1, size, file_stream_ptr);
         if (written != size) {
            std::error_code ec = std::make_error_code(std::errc::io_error);
            throw std::system_error(ec, std::format("Error writing to file (error code: {})!", ec.message()));
         }
      }

      inline void write_to_file(FILE* file_stream_ptr, const std::string& data) {
         write_to_file(file_stream_ptr, data.c_str(), data.size());
      }

      inline FILE* open_file(const char* file_path, const char* mode) {
         FILE* file_stream_ptr = fopen(file_path, mode);
         if (!file_stream_ptr) {
            std::error_code ec = std::make_error_code(std::errc::io_error);
            throw std::system_error(ec, std::format("Error opening log file '{}' (error code: {})!", file_path, ec.message()));
         }
         return file_stream_ptr;
      }

      inline void close_file(FILE* file_stream_ptr) {
         if (file_stream_ptr) {
            try {
               fclose(file_stream_ptr);
            }
            catch (...) {
               // Ignore any exceptions thrown by fclose()
            }
         }
      }

      struct file_io_t {
         FILE* file_stream_ptr_{ nullptr };
         std::string file_path_{};
         std::string mode_ = file_mode::append;

         void write(const std::string& text) {
            write_to_file(file_stream_ptr_, text);
         }

         FILE* operator<<(const std::string& text) {
            write_to_file(file_stream_ptr_, text);
            return file_stream_ptr_;
         }

         bool is_open() const {
            return file_stream_ptr_ != nullptr;
         }

         void close() {
            close_file(file_stream_ptr_);
            file_stream_ptr_ = nullptr;
         }

         void flush()
         {
            if (file_stream_ptr_) fflush(file_stream_ptr_);
         }

         FILE* open(const std::string_view path = {}) {
            if (!path.empty()) {
               file_path_ = path;
            }
            if (!file_path_.empty()) {
               file_stream_ptr_ = open_file(file_path_.c_str(), mode_.c_str());
            }
            return file_stream_ptr_;
         }

         FILE* open(const std::string_view path, std::ios_base::openmode mode) {
            mode_ = map_ios_flags(mode);
            return open(path.data());
         }
      };

     
   }
}