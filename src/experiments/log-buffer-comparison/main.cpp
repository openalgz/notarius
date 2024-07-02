#include <chrono>
#include <deque>
#include <iostream>
#include <string>

void test_string_as_buffer(size_t log_buffer_size, size_t max_log_entries)
{
   std::string log_buffer;
   log_buffer.reserve(1024 * max_log_entries);
   auto start = std::chrono::high_resolution_clock::now();
   std::string str;
   str.reserve(1024);
   size_t currentSize = 0;
   for (size_t i = 0; i < max_log_entries; ++i) {
      str = "Log entry " + std::to_string(i);
      log_buffer.append(str);
      currentSize += str.size();
      if (currentSize > log_buffer_size) {
         log_buffer.clear();
      }
   }
   auto end = std::chrono::high_resolution_clock::now();
   std::chrono::duration<double> diff = end - start;
   std::cout << "std::string time: " << diff.count() << " secs\n";
}

void test_deque_as_buffer(size_t log_buffer_size, size_t max_log_entries)
{
   std::deque<std::string> log_buffer;
   auto start = std::chrono::high_resolution_clock::now();

   std::string str;
   str.reserve(1024);
   size_t currentSize = 0;
   for (size_t i = 0; i < max_log_entries; ++i) {
      str = "Log entry " + std::to_string(i);
      log_buffer.emplace_back(str);
      currentSize += str.size();
      if (currentSize > log_buffer_size) {
         log_buffer.clear();
      }
   }
   auto end = std::chrono::high_resolution_clock::now();
   std::chrono::duration<double> diff = end - start;
   std::cout << "std::deque<std::string> time: " << diff.count() << " secs\n";
}

int main()
{
   constexpr size_t log_buffer_size = 10 * (1 << 20); // ~ 10 MB
   constexpr size_t max_log_entries = 10'000'000;

   test_string_as_buffer(log_buffer_size, max_log_entries);
   test_deque_as_buffer(log_buffer_size, max_log_entries);

   return 0;
}
