<div>
	<a style="text-decoration: none;" href="">
		<img src="https://img.shields.io/badge/C++-%2300599C.svg?logo=c%2B%2B&logoColor=white" alt="cpp">
	</a>
	<a style="text-decoration: none;" href="">
		<img src="https://ci.appveyor.com/api/projects/status/1acb366xfyg3qybk/branch/develop?svg=true" alt="building">
	</a>
	<a href="https://github.com/UF4007/memManager/blob/main/License.txt">
		<img src="https://img.shields.io/badge/license-MIT-blue" alt="MIT">
	</a>
	<a href="https://www.microsoft.com/en-us/windows">
		<img src="https://img.shields.io/badge/Windows-0078D6?style=for-the-badge&logo=windows&logoColor=white" alt="win">
	</a>
	<a href="https://www.debian.org/">
		<img src="https://img.shields.io/badge/Linux-FCC624?style=for-the-badge&logo=linux&logoColor=black" alt="linux">
	</a>
	<a href="https://www.espressif.com/en">
		<img src="https://www.espressif.com/sites/all/themes/espressif/logo-black.svg" alt="esp" width="75" height="25">
	</a>
</div>

# io::manager - High Performance Pipeline Concurrency Library

A modern C++20 header-only library for high-performance asynchronous I/O operations and coroutine-based concurrency.

## Overview

io::manager provides a comprehensive solution for building efficient, concurrent applications using C++20 coroutines. It implements a pipeline concurrency model with clear data protocol stream processing, making it ideal for network applications, real-time systems, and any scenario requiring high-performance asynchronous operations.

### Key Features

- **Pipeline Concurrency Model**: Efficient data flow processing with a clear protocol stream approach
- **C++20 Coroutines**: Full support for C++20 coroutines with a high-performance scheduler
- **Future/Promise Pattern**: JS-style future/promise with all/any/race/allSettle operations
- **Channel-based Communication**: Golang-style channels and async channels for inter-coroutine communication
- **Lock-free Structures**: Multi-thread lock-free structures for maximum performance
- **RAII Friendly**: Resource management follows RAII principles for safety and reliability
- **High Performance**: Close to the speed of native C++ coroutines, supporting more than 100M coroutines per second switch speed in a single thread
- **Protocol Support**: Built-in support for various network protocols (HTTP, DNS, SNTP, ICMP, KCP)
- **Cross-platform**: Works on Windows, Linux, and ESP platforms

## Requirements

- C++20 compatible compiler
- Supported platforms: Windows, Linux, ESP

## Installation

io::manager is a header-only library. Simply include the main header file in your project:

```cpp
#include "ioManager/ioManager.h"
```

## Basic Usage

### Creating a Simple Coroutine

```cpp
io::fsm_func<void> simple_coroutine()
{
    io::fsm<void> &fsm = co_await io::get_fsm;
    
    // Your asynchronous code here
    
    co_return;
}
```

### Using Futures and Promises

```cpp
io::fsm_func<void> future_example()
{
    io::fsm<void> &fsm = co_await io::get_fsm;
    
    io::future fut;
    io::promise prom = fsm.make_future(fut);
    
    // In another coroutine or thread
    prom.set(); // Resolves the future
    
    // Wait for the future to be resolved
    co_await fut;
    
    co_return;
}
```

### Working with Channels

```cpp
io::fsm_func<void> channel_example()
{
    io::fsm<void> &fsm = co_await io::get_fsm;
    
    // Create a channel with capacity 1000
    io::chan<char> chan = fsm.make_chan<char>(1000);
    
    // Producer
    char data[] = "Hello, World!";
    std::span<char> data_span(data, sizeof(data));
    co_await (chan << data_span);
    
    // Consumer
    io::chan<char>::span_guard recv;
    co_await (chan >> recv);
    
    // Use received data
    std::cout << recv.span.data() << std::endl;
    
    co_return;
}
```

### Network Communication

```cpp
io::fsm_func<void> tcp_client_example()
{
    io::fsm<void> &fsm = co_await io::get_fsm;
    
    io::sock::tcp client(fsm);
    
    // Connect to server
    co_await client.connect(asio::ip::tcp::endpoint(
        asio::ip::address::from_string("127.0.0.1"), 8080));
    
    // Send data
    char data[] = "Hello, Server!";
    co_await client.send(std::span<char>(data, sizeof(data)));
    
    // Receive data
    auto received = co_await client.recv();
    
    // Process received data
    std::cout << std::string(received.data(), received.size()) << std::endl;
    
    co_return;
}
```

## Advanced Features

### Pipeline Processing

```cpp
io::fsm_func<void> pipeline_example()
{
    io::fsm<void> &fsm = co_await io::get_fsm;
    
    io::sock::tcp client(fsm);
    io::sock::udp socket(fsm);
    
    // Create a pipeline that receives data from TCP and forwards to UDP
    auto pipeline = io::pipeline<>() >> client >> 
        [](const std::span<char>& data) -> std::optional<std::pair<std::span<char>, asio::ip::udp::endpoint>> {
            // Process data
            return std::make_pair(data, asio::ip::udp::endpoint(
                asio::ip::address::from_string("127.0.0.1"), 8081));
        } >> socket;
    
    // Start the pipeline
    co_await pipeline;
    
    co_return;
}
```

### Timers and Timeouts

```cpp
io::fsm_func<void> timer_example()
{
    io::fsm<void> &fsm = co_await io::get_fsm;
    
    io::clock timer;
    fsm.make_clock(timer, std::chrono::seconds(5));
    
    // Wait for timer
    co_await timer;
    
    std::cout << "5 seconds elapsed!" << std::endl;
    
    co_return;
}
```

## Performance

io::manager is designed for high performance, achieving:
- More than 100M coroutines per second switch speed in a single thread
- Near-native C++ coroutine performance
- Efficient memory usage with pooled allocations

## License

This project is licensed under the MIT License - see the [License.txt](License.txt) file for details.

## Contributing

Contributions are welcome! Feel free to open issues or submit pull requests on GitHub.

## Acknowledgements

- Uses [Asio](https://think-async.com/Asio/) for network support
- KCP protocol implementation based on [ikcp](https://github.com/skywind3000/kcp)
