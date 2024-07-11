# notarius

------
#### TODOs:
- [ ] Simplify/Update example tests
- [ ] Redo this documentation.
------

## Demo Project
This project includes CMake script files that incorporate getting notarius as a third-party library.

### demo/CMakeLists.txt
```cmake
cmake_minimum_required(VERSION 3.20)

include("cmake/prelude.cmake")

configure_main_project("demo" "") # blank to aquire/assign version from the git repo
                                  # yy.mm.dd is used if the repo does not have a 
                                  # a tag (e.g., a tag of 'v1.0.0' will be transformed
                                  # to '1.0.0', etc.                             
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

project("write-tests" LANGUAGES CXX)

make_project_executable("write-tests" notarius-demo.cpp)
```
### notarius-demo.hpp
```c++
// TODO
```

### notarius-demo.cpp
```c++
// TODO
```

## Contributing

Contributions to the Notarius logging library are welcome! If you find any issues or have suggestions for improvements, please open an issue or submit a pull request on the GitHub repository.

## License

The Notarius logging library is released under the [MIT License](LICENSE).
