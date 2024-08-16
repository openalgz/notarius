# <span style="color: #1f618d;">notarius_t</span>

## Demo Project
This project includes CMake script files that incorporate getting notarius as a third-party library.

### demo/CMakeLists.txt
```cmake
cmake_minimum_required(VERSION 3.20)

include("cmake/prelude.cmake")

configure_main_project("demo" "") # blank to aquire/assign version from the git repo
                                  # yy.mm.dd is used if the repo does not have a 
                                  # a tag.
                                  #
                                  # Note: A tag of 'v1.0.0' will be transformed to
                                  #       '1.0.0', etc. for MY_PROJECT_VERSION       
project(
   ${PROJECT_NAME}
   VERSION "${MY_PROJECT_VERSION}"
   LANGUAGES CXX
)

# See: config.cmake for details.
#
generate_demo()
```

### demo/src/tests/CMakeLists.txt
```cmake
cmake_minimum_required(VERSION 3.27)

project("notarius-demo" LANGUAGES CXX)

make_project_executable("notarius-demo" notarius-demo.cpp)
```
### notarius-demo.cpp
```c++
#include "notarius/notarius.hpp"
#include "ut/ut.hpp"

using namespace ut;
using namespace slx; // the notarius namespace

suite example_notarius_use_cases = [] {
    
   notarius_t<"demo-log.md", notarius_opts_t{.enable_file_logging = true}> demo;

   // Sanity Check . . .remove log file from previous test.
   demo.remove_log_file();
   {
      demo("Hello World\n");
      auto actual = demo.str();
      expect("Hello World\n" == actual);

      demo("Hello World Again {} {} {} {}\n", 1.23, 2.23, 3.23, 4.23);
      actual = demo.str();
      auto expected_result = std::format("Hello World\nHello World Again {} {} {} {}\n", 1.23, 2.23, 3.23, 4.23);
      expect(actual == expected_result);
   }

   demo.remove_log_file();
   {
      demo.print("Hello World\n");
      auto actual = demo.str();
      expect("Hello World\n" == actual);

      demo.enable_stdout();
      demo.write("Hello World Again {} {} {} {}\n", 1.23, 2.23, 3.23, 4.23);
      actual = demo.str();
      auto expected_result = std::format("Hello World\nHello World Again {} {} {} {}\n", 1.23, 2.23, 3.23, 4.23);
      expect(actual == expected_result);
   }
    
   demo.remove_log_file();
   {
      demo << "Hello World " << 1.23 << " " << 2.23 << '\n';
      auto actual = demo.str();
      auto expected_result = "Hello World 1.23 2.23\n";
      expect(actual == expected_result);
      demo.remove_log_file();
   }
    
   // Clean-up
   demo.remove_log_file();
};

int main()
{
   std::cout << '\n';
   return 0;
}
```

## Contributing

Contributions to the Notarius logging library are welcome! If you find any issues or have suggestions for improvements, please open an issue or submit a pull request on the GitHub repository.

## License

The Notarius logging library is released under the [MIT License](LICENSE).
