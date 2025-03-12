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

## Future/Promise: Coroutine Communication

The future/promise pattern in io::manager provides a powerful way for coroutines to communicate and synchronize with each other. This pattern is similar to JavaScript's Promise system but optimized for C++ coroutines.

### Creating and Using Future/Promise Pairs

#### Basic Usage

To create a future/promise pair within a coroutine:

```cpp
io::fsm_func<void> example_coroutine()
{
    // Get the FSM context
    io::fsm<void> &fsm = co_await io::get_fsm;
    
    // Create a future and its corresponding promise
    io::future fut;
    io::promise<void> prom = fsm.make_future(fut);
    
    // Pass the future to another coroutine that will wait on it
    fsm.spawn_now(wait_for_future(fut)).detach();
    
    // Do some work...
    
    // Resolve the promise when ready, which will resume the waiting coroutine
    prom.resolve();
    
    co_return;
}

io::fsm_func<void> wait_for_future(io::future fut)
{
    // Get the FSM context
    io::fsm<void> &fsm = co_await io::get_fsm;
    
    // Wait for the future to be resolved
    co_await fut;
    
    // Continue execution after the future is resolved
    std::cout << "Future resolved!" << std::endl;
    
    co_return;
}
```

#### Passing Data with Future/Promise

To pass data from one coroutine to another:

```cpp
io::fsm_func<void> data_producer()
{
    io::fsm<void> &fsm = co_await io::get_fsm;
    
    // Create a future_with that can carry data
    io::future_with<std::string> fut;
    io::promise<std::string> prom = fsm.make_future(fut, &fut.data);
    
    // Pass the future to a consumer
    fsm.spawn_now(data_consumer(std::move(fut))).detach();
    
    // Set the data and resolve the promise
    *prom.data() = "Hello from producer!";
    prom.resolve();
    
    co_return;
}

io::fsm_func<void> data_consumer(io::future_with<std::string> fut)
{
    io::fsm<void> &fsm = co_await io::get_fsm;
    
    // Wait for the future to be resolved
    co_await fut;
    
    // Access the data
    std::cout << "Received: " << fut.data << std::endl;
    
    co_return;
}
```

### Error Handling

Promises can also be rejected with an error code:

```cpp
io::fsm_func<void> error_example()
{
    io::fsm<void> &fsm = co_await io::get_fsm;
    
    io::future fut;
    io::promise<void> prom = fsm.make_future(fut);
    
    // Reject the promise with an error
    prom.reject(std::make_error_code(std::errc::operation_canceled));
    
    co_return;
}

io::fsm_func<void> handle_errors(io::future fut)
{
    io::fsm<void> &fsm = co_await io::get_fsm;
    
    try {
        // Wait for the future
        co_await fut;
        std::cout << "Success!" << std::endl;
    } catch (const std::system_error& e) {
        // Handle the error
        std::cout << "Error: " << e.what() << std::endl;
    }
    
    co_return;
}
```

### Combining Multiple Futures

io::manager provides JavaScript-style combinators for working with multiple futures:

#### all - Wait for all futures to resolve

```cpp
io::fsm_func<void> wait_for_all()
{
    io::fsm<void> &fsm = co_await io::get_fsm;
    
    io::future fut1, fut2, fut3;
    io::promise<void> prom1 = fsm.make_future(fut1);
    io::promise<void> prom2 = fsm.make_future(fut2);
    io::promise<void> prom3 = fsm.make_future(fut3);
    
    // Create a combined future that resolves when all futures resolve
    auto combined = io::future::all(fut1, fut2, fut3);
    
    // Resolve the promises (could be in different coroutines)
    prom1.resolve();
    prom2.resolve();
    prom3.resolve();
    
    // Wait for all futures to resolve
    co_await combined;
    
    std::cout << "All futures resolved!" << std::endl;
    
    co_return;
}
```

#### any - Wait for any future to resolve

```cpp
io::fsm_func<void> wait_for_any()
{
    io::fsm<void> &fsm = co_await io::get_fsm;
    
    io::future fut1, fut2, fut3;
    io::promise<void> prom1 = fsm.make_future(fut1);
    io::promise<void> prom2 = fsm.make_future(fut2);
    io::promise<void> prom3 = fsm.make_future(fut3);
    
    // Create a combined future that resolves when any future resolves
    auto combined = io::future::any(fut1, fut2, fut3);
    
    // Resolve one promise
    prom2.resolve();
    
    // Wait for any future to resolve
    co_await combined;
    
    std::cout << "At least one future resolved!" << std::endl;
    
    co_return;
}
```

#### race - Similar to any, but rejects if any future rejects

```cpp
io::fsm_func<void> race_example()
{
    io::fsm<void> &fsm = co_await io::get_fsm;
    
    io::future fut1, fut2;
    io::promise<void> prom1 = fsm.make_future(fut1);
    io::promise<void> prom2 = fsm.make_future(fut2);
    
    // Create a race between futures
    auto race_result = io::future::race(fut1, fut2);
    
    // Start two operations in parallel
    fsm.spawn_now([prom1]() -> io::fsm_func<void> {
        io::fsm<void> &fsm = co_await io::get_fsm;
        // Simulate work
        co_await fsm.setTimeout(std::chrono::seconds(2));
        prom1.resolve();
        co_return;
    }()).detach();
    
    fsm.spawn_now([prom2]() -> io::fsm_func<void> {
        io::fsm<void> &fsm = co_await io::get_fsm;
        // Simulate work
        co_await fsm.setTimeout(std::chrono::seconds(1));
        prom2.resolve();
        co_return;
    }()).detach();
    
    // Wait for the race result
    co_await race_result;
    
    std::cout << "Race completed!" << std::endl;
    
    co_return;
}
```

### Deferred Resolution

For operations that may complete asynchronously, you can use deferred resolution:

```cpp
io::fsm_func<void> async_operation()
{
    io::fsm<void> &fsm = co_await io::get_fsm;
    
    io::future fut;
    io::promise<void> prom = fsm.make_future(fut);
    
    // Start an asynchronous operation
    std::thread([prom = std::move(prom)]() mutable {
        // Simulate work in another thread
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        // Resolve the promise later
        prom.resolve_later();
    }).detach();
    
    // Wait for the future
    co_await fut;
    
    std::cout << "Async operation completed!" << std::endl;
    
    co_return;
}
```

### Best Practices

1. **Lifetime Management**: Ensure futures outlive their usage in coroutines.
2. **Single Awaiter**: A future should only be awaited by one coroutine at a time.
3. **Error Handling**: Always handle potential rejections when awaiting futures.
4. **Combinators**: Use combinators like `all`, `any`, and `race` to manage multiple concurrent operations.

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
