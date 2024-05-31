# notarius

### A Fast and Simple C++ Logging Library

> [!NOTE]
>
> This library is fully functional and is expected to be safe to use but is still under construction.
>
> Goals:
>
> - It should remain header only.
>- Fast (see spdlog test comparison)
> - Easy to use (e.g., easy configurability of logging instances, and, the implementation of the source code should be easy to read and understand).
> - Thread safety that can be turned off when not required.

------

#### TODOs:

- [ ] Complete Documentation
- [ ] Support earlier versions of C++. Currently geared for C++ 20+.
- [ ] Create test cases to evaluate all features.
- [ ] Consider adding logging sinks to enable log messages to be sent to various destinations, such as network sockets, message queues, or databases. It may be desirable to not do this in the logger itself but have a file store 'watcher' to transfer data to other destinations. In other words support a local file store that is then utilized by another process for moving the data to other destinations. 

------

Notarius is a header-only (the only header dependencies should come from the STL) C++ logging library that provides a flexible and efficient way to handle logging in your applications. It offers the following features:

- **Log Levels**: Supports different log levels (none, info, warn, error, exception) for controlling the verbosity of logging.
- **Output Streams**: Allows logging to multiple output streams, including stdout, stderr, and file. Each of these features may be enabled or disabled. 
- **File Logging**: Logs can be written to a file with automatic file creation, appending, and splitting based on file size.
- **Formatting**: Supports formatting log messages using `std::format` syntax.
- **Thread Safety**: Provides thread-safe logging using a shared mutex.
- **Performance**: Offers options for lock-free logging and immediate mode to output streams for improved control over performance characteristics.
- **Customization**: Allows customization of log file names, extensions, and paths.

## Example Usage

To use the Notarius logging library, simply include the `notarius.hpp` header file in your project and create an instance of the `notarius_t` class:

```cpp
#include "notarius.hpp"

// Create a log file name 'lgr' using the default notarius options with the 
// Markdown extension md.
//
inline slx::notarius_t < "lgr", slx::notarius_opts{}, "md" > my_logger;

int main() {
    
    // All functions may be enabled or disabled at runtime.
    //
    lgr("Hello, World!\n");
    lgr.print("Hello, World!\n");
    lgr.print("Hello, World! **Numbers:** {}, {}, {}\n", 1.2f, 1.2879945, -1);
    lgr.print<slx::log_level::info>("This is an info message.\n");
    lgr.print<slx::log_level::error>("An error occurred: {}\n", error_message);
    lgr << "Hello, " << "World..." << 1.2f << "; " << 1.2879945 << "; " << -1 << '\n'; 
	
    // Do not log the message, only write it to stdout, stderr, or std::clog:
    // 
    lgr.cout("Hello Word\n");
    lgr.cout("Hello {}\n", "World");
    
    lgr.pause_stderr();  // Pause lgr.cerr output 
    lgr.cerr("There were {} {}\n", 5, "errors."); // this call is ignored.
    lgr.enable_stderr(); // Enable stderr once again.
    
    lgr.clog("This will go to std::clog {}", "...");

    return 0;
}
```

You can customize the logging behavior by passing options to the `notarius_t` class template:

```cpp
slx::notarius_t<"MyApp", slx::notarius_opts{.lock_free_enabled = true}> logger;
```

Refer to the code documentation for more details on available options and usage examples.

------

> [!NOTE]
>
> The notarius.hpp header only requires STL header dependencies. The other header files in the `./include/notarius` folder are to support the provided test project.

------

### Specialized Write Method

The `notarius::write` method is used to log a message immediately to the console (`stdout` or `stderr`) vs caching the string to be written later when the `ostream_buffer` reaches its defined capacity. This way you can use caching to speed up console output but also having the ability to forcing critical outputs to be displayed right away.

```cpp
template <log_level level = log_level::none, typename... Args>
auto write(std::format_string<Args...> fmt, Args&&... args) {
    toggle_immediate_mode(); // Enable immediate mode for this log
    return print<level>(fmt, std::forward<Args>(args)...); // Call print method
}
```

The key difference between `write` and `print` is that `write` enables immediate mode before calling `print`. This ensures that the log message is written to the console immediately, without being buffered.

Additionally, the `write` method still calls the `print` method, which means the log message will also be written to the log file when file logging is enabled.

So, in summary, the `write` method provides a way to log a message to both the console (immediately) and the log file simultaneously. It's useful when you want to see the log message on the console right away while still maintaining a persistent log in a file.

### Configuring Notarius Options

The `notarius_opts` struct is a configuration structure that provides various options to customize the behavior of the `notarius_t` logging class:

```C++
struct notarius_opts
{
  bool lock_free_enabled{ false };
  bool immediate_mode{ false };
  bool enable_stdout{ false };
  bool enable_stderr{ false };
  bool enable_file_logging{ true };
  bool open_file_store_for_appending{ true };

  // Split log files @ split_max_log_file_size_bytes_ 
  //
  bool split_log_files{ false };

  // The max allowable size of a log file (this is ignored when 'split_log_files' is false).
  //  
  size_t split_max_log_file_size_bytes_{ 2'097'152 }; // ~2MB
};
```

**lock_free_enabled:**
Enables/disables lock-free logging. When set to `true`, the logging operations will be performed without acquiring a lock, potentially improving performance in multi-threaded environments.

**immediate_mode:**
If set to `true`, all output will be written directly to the console or terminal. Otherwise, the output will be cached until the cache reaches its maximum size, at which point the cache is flushed.

**enable_stdout:**
 Enables/disables logging to the standard output stream (`std::cout`).

**enable_stderr:**
Enables/disables logging to the standard error stream (`std::cerr`).

**enable_file_logging:**
 Enables/disables logging to a file.

**open_file_store_for_appending:**
Determines whether the log file should be opened in append mode or overwritten. If set to `true`, log entries will be appended to the existing file. If set to `false`, the log file will be overwritten each time the logger is initialized.

**split_log_files**:
Enables or disables splitting log files when they reach a certain size.

**split_max_log_file_size_bytes_:**
Specifies the maximum allowable size of a log file in bytes. This option is ignored if `split_log_files` is set to `false`.

These options can be passed as a template argument to the `notarius_t` class to customize the logging behavior according to your requirements.

## Contributing

Contributions to the Notarius logging library are welcome! If you find any issues or have suggestions for improvements, please open an issue or submit a pull request on the GitHub repository.

## License

The Notarius logging library is released under the [MIT License](LICENSE).
