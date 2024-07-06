#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <concepts>
#include <cstring>
#include <deque>
#include <iostream>
#include <numeric>
#include <random>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

constexpr auto test_string = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
constexpr size_t test_string_length = sizeof(test_string) - 1;

constexpr int num_of_string_assignments = 1000000;
constexpr int size_of_test_string = sizeof(test_string);
constexpr int num_of_test_runs = 1000;
constexpr int warm_up_iterations = 10000;

std::string generate_string()
{
   static const char alphanum[] =
      "0123456789"
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
      "abcdefghijklmnopqrstuvwxyz";
   static std::mt19937 gen(std::random_device{}());
   static std::uniform_int_distribution<> dis(0, sizeof(alphanum) - 2);

   std::string str(size_of_test_string, 0);
   std::generate_n(str.begin(), size_of_test_string, [&]() { return alphanum[dis(gen)]; });
   return str;
}

struct TestResult
{
   double mean{};
   double stddev{};
};

template <typename T>
concept is_string_like = std::is_convertible_v<T, std::string_view>;

template <typename Container>
concept is_string_container = requires(Container c) {
   typename Container::value_type;
   requires is_string_like<typename Container::value_type>;
   {
      c.begin()
   } -> std::input_iterator;
   {
      c.end()
   } -> std::input_iterator;
   {
      c.size()
   } -> std::convertible_to<std::size_t>;
};

template <is_string_container Container>
std::string make_string(const Container& container)
{
   size_t total_length = std::accumulate(container.begin(), container.end(), size_t(0),
                                         [](size_t sum, const auto& s) { return sum + std::string_view(s).length(); });

   std::string result;
   result.reserve(total_length);

   for (const auto& s : container) {
      result += s;
   }

   return result;
}

template <typename Func>
TestResult run_test(Func&& func, const std::string& name)
{
   std::vector<double> times;
   times.reserve(num_of_test_runs);

   for (int run = 0; run < num_of_test_runs; ++run) {
      auto start = std::chrono::high_resolution_clock::now();
      func();
      auto end = std::chrono::high_resolution_clock::now();
      std::chrono::duration<double> duration = end - start;
      times.push_back(duration.count());
   }

   const double sum = std::accumulate(times.begin(), times.end(), 0.0);
   const double mean = sum / num_of_test_runs;

   const double sq_sum = std::inner_product(times.begin(), times.end(), times.begin(), 0.0);
   const double stddev = std::sqrt(sq_sum / num_of_test_runs - mean * mean);

   std::cout << name << ":\nmean: " << mean << "; stddev(+-): " << stddev << " seconds\n\n";
   return {mean, stddev};
}

std::string test_string_append()
{
   std::string str;
   str.reserve(num_of_string_assignments * size_of_test_string);
   for (int i = 0; i < num_of_string_assignments; ++i) {
      str.append(test_string, size_of_test_string);
   }
   return str;
}

std::string test_vector_of_std_string()
{
   std::vector<std::string> v;
   v.reserve(num_of_string_assignments);
   for (int i = 0; i < num_of_string_assignments; ++i) {
      v.emplace_back(test_string);
   }
   return make_string(v);
}

std::string test_deque_of_std_string()
{
   std::deque<std::string> v;
   for (int i = 0; i < num_of_string_assignments; ++i) {
      v.emplace_back(test_string);
   }
   return make_string(v);
}

std::string test_std_vector_of_char()
{
   std::vector<char> buffer;
   buffer.reserve(num_of_string_assignments * size_of_test_string);
   for (int i = 0; i < num_of_string_assignments; ++i) {
      buffer.insert(buffer.end(), test_string, test_string + size_of_test_string);
   }
   return std::string(buffer.begin(), buffer.end());
}

std::string test_vector_of_array_char()
{
   std::vector<std::array<char, size_of_test_string>> buffer;
   buffer.reserve(num_of_string_assignments);
   for (int i = 0; i < num_of_string_assignments; ++i) {
      std::array<char, size_of_test_string> arr;
      std::memcpy(arr.data(), test_string, size_of_test_string);
      buffer.emplace_back(arr);
   }
   return std::string(reinterpret_cast<const char*>(buffer.data()), buffer.size() * size_of_test_string);
}

std::string test_ostringstream()
{
   std::ostringstream ss;
   ss.str().reserve(num_of_string_assignments * size_of_test_string);
   for (int i = 0; i < num_of_string_assignments; ++i) {
      ss.write(test_string, size_of_test_string);
   }
   return ss.str();
}

std::string test_stringstream()
{
   std::stringstream ss;
   ss.str().reserve(num_of_string_assignments * size_of_test_string);
   for (int i = 0; i < num_of_string_assignments; ++i) {
      ss.write(test_string, size_of_test_string);
   }
   return ss.str();
}

std::string test_heap_buffer()
{
   std::unique_ptr<char[]> buffer(new char[num_of_string_assignments * size_of_test_string]);
   char* current = buffer.get();
   for (int i = 0; i < num_of_string_assignments; ++i) {
      std::memcpy(current + i * size_of_test_string, test_string, size_of_test_string);
   }
   return std::string(buffer.get(), num_of_string_assignments * size_of_test_string);
}

void warm_up()
{
   for (int i = 0; i < warm_up_iterations; ++i) {
      volatile std::string s = generate_string();
   }
}

int main()
{
   std::cout << "Warming up...\n";
   // warm_up();
   std::cout << "Starting tests...\n";
   run_test(test_string_append, "test_string_append");
   run_test(test_stringstream, "test_stringstreamm");
   run_test(test_ostringstream, "test_ostringstreamm");
   run_test(test_std_vector_of_char, "test_std_vector_of_char buffer");
   run_test(test_heap_buffer, "test_heap_buffer");
   run_test(test_vector_of_array_char, "test_vector_of_array_char");
   run_test(test_vector_of_std_string, "test_vector_of_std_string");
   run_test(test_deque_of_std_string, "test_deque_of_std_string");
   std::cout << "Sample Info:\n"
             << " Number of String Assignments: " << num_of_string_assignments << '\n'
             << "          Number of Test Runs: " << num_of_test_runs << "\n\n";
   return 0;
}