# notarius

### A Fast and Simple C++ Logging Library

> [!NOTE]
>
> **Goals:**
>
> - It should remain header only. It is currently less than 800 lines of code including the comments. The only file needed for your projects is `notarius.hpp`
>- Fast, equal to spdlog (see spdlog tests comparisons) in performance.
> - Easy to use (e.g., easy configurability of logging instances, and, the implementation of the source code should be easy to read and understand).
>- Thread safety that can be turned off when not required.

------

#### TODOs:

- [ ] Complete Documentation (e.g., currently demo project documentation is just a copy of this; provide more examples, and so forth).
- [ ] Add Doxygen comments and doxygen configuration file.
- [ ] Support earlier versions of C++. Currently geared for C++ 20+. This will require std::format and std::format_string replacements. 
- [ ] Create test cases to evaluate all features.
- [ ] Add logging sinks to enable log messages to be sent to various destinations, such as network sockets, message queues, or databases. It may be desirable to not do this in the logger itself but have a file store 'watcher' to transfer data to other destinations. In other words support a local file store that is then utilized by another process for moving the data to other destinations. 

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

// Create a logger named 'lgr' with file name 'lgr.md' using the default notarius options with the 
// Markdown extension md.
//
inline slx::notarius_t < "lgr", slx::notarius_opts{}, "md" > lgr;

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
    {
    	lgr.cerr("There were {} {}\n", 5, "errors."); // this call is ignored.
    }
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
### Configuring Notarius Options

The `notarius_opts`_t struct provides various options to customize the behavior of the `notarius_t` logging class:

```C++
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
  size_t split_log_file_at_size_bytes{1'048'576*25}; // 25 MB

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
```

> [!NOTE]
>
> The following performance considerations should be noted when using `disable_file_buffering` (the default setting is true):
>
> - **Buffering vs. No Buffering:** 
>   Disabling buffering (`pubsetbuf(0, 0)`) can lead to more frequent write operations to the file system, which may impact performance negatively, especially if your application writes data frequently in small chunks.
> - **Buffered Write:** 
>   Using buffering can improve performance by reducing the number of actual write operations to the file system, aggregating multiple small writes into fewer larger writes. In general how this feature effects your performance is related to the 'flush_to_std_outputs_at_bytes' setting.

### notarius write vs print (or notarius::operator(...))

The `notarius::write` method is used to log a message immediately to the console (`stdout`, `stderr`, or `std::clog`) vs caching the string to be written later when an associated stream buffer reaches its defined capacity for these objects. This way you can use caching to speed up console output but also having the ability to force critical outputs to be displayed right away.

```cpp
template <log_level level = log_level::none, typename... Args>
auto write(std::format_string<Args...> fmt, Args&&... args) {
    toggle_immediate_mode(); // Enable immediate mode for std::cout, std::cerr, or std::clog
    return print<level>(fmt, std::forward<Args>(args)...); // Call print method
}
```

The key difference between `write` and `print` is that `write` calls `toggle_immediate_mode()` prior to calling`print`. This ensures that the log message is written to the console immediately, without being buffered. This is equivalent to:
```c++
lgr.toggle_immediate_mode();
lgr.print("Print something...");
//
// is the same as:
//
lgr.write("Print something...");
```

In summary, the `write` method provides a way to log a message to both the console (immediately) and the log file simultaneously. It's useful when you want to see the log message on the console right away while still writing to the logging file store.

> [!IMPORTANT]
>
> In C++, `std::cout`, `std::cerr`, and `std::clog` are different stream objects, *<u>but they typically direct their output to the same destination</u>*, such as common debug terminal. Therefore note that the `notarius::print or notarius::operator()` will print to the standard streams when they are enabled and stream the messages as follows: to `std::cout` if  `log_level <= log_leve::warn`, to `std::cerr` if `log_level >= log_level::error` , and to `std::clog` regardless of the level.
>
> *This means if you have all three standard outputs enabled that you may create a situation where output to a terminal or console could be duplicated.* For example, if `notarius::cout(...)`, `notarius::cerr(...)`, and `notarius::clog(...)` are all enabled, and if all three standard outputs are directed to the same terminal, then calling notarius::("message") or notarius::print("message"), will result in a double output of the string "message". This is because at a minimum two outputs streams will be called. In this example `log_level` is `<= log_leve::warn`, the default level, therefore a write to std::cout is run, and since std::clog is enabled it will also output to this stream.
>
> See: **Understanding C++ Standard Stream Objects** below for additional details.

#### Notarius Log Levels
```c++
enum class log_level : int {
   none,        ///< No logging level is included in the message.
   info,        ///< Information log level: label is 'info'.
   warn,        ///< Warning log level: label is 'warn'.
   error,       ///< Error log level: label is 'error'.
   exception,   ///< Exception log level: label is 'exception'.
};
```
### Understanding C++ Standard Stream Objects

In C++, `std::cout`, `std::cerr`, and `std::clog` are different stream objects, but they typically direct their output to the same destination, such as the terminal. Here’s a detailed explanation of how they work and why they can be different stream objects yet appear to behave similarly:

1. **Stream Objects**:
   - `std::cout` is an instance of `std::ostream` used for standard output.
   - `std::cerr` is also an instance of `std::ostream`, but it is designed for error output and is typically unbuffered.
   - `std::clog` is another instance of `std::ostream`, used for logging and is typically buffered.
2. **Buffering**:
   - **`std::cout`** is usually line-buffered if it is connected to a terminal, meaning it flushes its buffer on encountering a newline.
   - **`std::cerr`** is unbuffered by default, meaning it outputs characters immediately.
   - **`std::clog`** is buffered, meaning it collects output in a buffer and writes it in larger chunks.
3. **Underlying Stream Buffer**:
   - Even though they are different stream objects, they can share the same underlying `streambuf` object. The `streambuf` object is responsible for the actual input and output operations.
   - By default, `std::cout`, `std::cerr`, and `std::clog` may use the same `streambuf` associated with the terminal. This is why their outputs appear on the same terminal.

In the following example the std::clog is redefined to have the same output as the notarius log file store:

```C++
#include "notarius/notarius.hpp"

inline slx::notarius_t <"lgr", slx::notarius_opts{}, "md"> lgr;

int main() {
    
    // Redirect std::clog to output to the notarius log file:
    //
    slx::std_stream_redirection_t redirected_clog_output_stream(std::clog, lgr.rdbuf());

    std::clog << "This goes to the notarius log file." << std::endl;

    return 0;
}

```

> [!TIP]
>
> It can be helpful to use notarius stream redirection when updating code that may be using std::clog to notarius. This way the use of std::clog does not have to be refactored in the program.
>
> In a similar manner std::cout and std::cerr may also be redirected.

## Contributing

Contributions to the Notarius logging library are welcome! If you find any issues or have suggestions for improvements, please open an issue or submit a pull request on the GitHub repository.

## License

The Notarius logging library is released under the [MIT License](LICENSE).
