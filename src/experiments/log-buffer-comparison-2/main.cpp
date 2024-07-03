#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <deque>
#include <iostream>
#include <numeric>
#include <random>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

constexpr int NUM_ITERATIONS = 1000000;
constexpr int STRING_SIZE = 100;
constexpr int NUM_RUNS = 10;
constexpr int WARM_UP_ITERATIONS = 10000;

std::string generate_string()
{
   static const char alphanum[] =
      "0123456789"
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
      "abcdefghijklmnopqrstuvwxyz";
   static std::mt19937 gen(std::random_device{}());
   static std::uniform_int_distribution<> dis(0, sizeof(alphanum) - 2);

   std::string str(STRING_SIZE, 0);
   std::generate_n(str.begin(), STRING_SIZE, [&]() { return alphanum[dis(gen)]; });
   return str;
}

struct TestResult
{
   double mean;
   double stddev;
};

template <typename Func>
TestResult run_test(Func&& func, const std::string& name)
{
   std::vector<double> times;
   times.reserve(NUM_RUNS);

   for (int run = 0; run < NUM_RUNS; ++run) {
      auto start = std::chrono::high_resolution_clock::now();
      func();
      auto end = std::chrono::high_resolution_clock::now();
      std::chrono::duration<double> duration = end - start;
      times.push_back(duration.count());
   }

   double sum = std::accumulate(times.begin(), times.end(), 0.0);
   double mean = sum / NUM_RUNS;

   double sq_sum = std::inner_product(times.begin(), times.end(), times.begin(), 0.0);
   double stddev = std::sqrt(sq_sum / NUM_RUNS - mean * mean);

   std::cout << name << ":\nmean: " << mean << "; stddev(+-): " << stddev << " seconds\n\n";
   return {mean, stddev};
}

void test_string_append()
{
   std::string str;
   str.reserve(NUM_ITERATIONS * STRING_SIZE);
   for (int i = 0; i < NUM_ITERATIONS; ++i) {
      str.append(generate_string());
   }
}

void test_stringstream()
{
   std::stringstream ss;
   for (int i = 0; i < NUM_ITERATIONS; ++i) {
      ss << generate_string();
   }
   std::string result = ss.str();
}

void test_custom_buffer()
{
   std::vector<char> buffer;
   buffer.reserve(NUM_ITERATIONS * STRING_SIZE);
   for (int i = 0; i < NUM_ITERATIONS; ++i) {
      std::string s = generate_string();
      buffer.insert(buffer.end(), s.begin(), s.end());
   }
   std::string result(buffer.begin(), buffer.end());
}

void test_string_view()
{
   std::vector<std::string> strings;
   strings.reserve(NUM_ITERATIONS);

   for (int i = 0; i < NUM_ITERATIONS; ++i) {
      strings.push_back(generate_string());
   }

   std::string result;
   result.reserve(NUM_ITERATIONS * STRING_SIZE);
   for (const auto& str : strings) {
      result.append(str);
   }
}

void test_static_buffer()
{
   static char buffer[NUM_ITERATIONS * STRING_SIZE];
   char* current = buffer;

   for (int i = 0; i < NUM_ITERATIONS; ++i) {
      std::string s = generate_string();
      std::memcpy(current, s.data(), STRING_SIZE);
      current += STRING_SIZE;
   }

   std::string result(buffer, NUM_ITERATIONS * STRING_SIZE);
}

void test_vector()
{
   std::vector<char> buffer;
   buffer.resize(NUM_ITERATIONS * STRING_SIZE);
   char* current = buffer.data();
   for (int i = 0; i < NUM_ITERATIONS; ++i) {
      std::string s = generate_string();
      std::memcpy(current, s.data(), STRING_SIZE);
      current += STRING_SIZE;
   }
   std::string result(buffer.begin(), buffer.end());
}

void warm_up()
{
   for (int i = 0; i < WARM_UP_ITERATIONS; ++i) {
      volatile std::string s = generate_string();
   }
}

int main()
{
   std::cout << "Warming up...\n";
   warm_up();
   std::cout << "Starting tests...\n";

   run_test(test_string_append, "std::string append");
   run_test(test_stringstream, "std::stringstream");
   run_test(test_custom_buffer, "Custom buffer");
   run_test(test_string_view, "std::string_view");
   run_test(test_static_buffer, "char buffer");
   run_test(test_vector, "std::vector<char>");

   return 0;
}
