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

## Finite State Machine (FSM): The Core of io::manager

At the heart of io::manager is its Finite State Machine (FSM) implementation, which provides the foundation for creating and managing coroutines. Understanding how to create and use FSMs is essential before diving into more advanced features.

### Creating a Basic FSM Coroutine

Every coroutine in io::manager is defined as an FSM function using the `io::fsm_func<T>` template, where `T` is the associated type:

```cpp
io::fsm_func<void> simple_coroutine()
{
    // Get the FSM context
    io::fsm<void> &fsm = co_await io::get_fsm;
    
    // Your coroutine code here
    std::cout << "Hello from a coroutine!" << std::endl;
    
    co_return;
}
```

The first step in any coroutine is to obtain the FSM context using `co_await io::get_fsm`. This gives you access to the FSM instance that manages the coroutine's state and provides methods for creating futures, timers, channels, and spawning other coroutines.

### Spawning and Managing Coroutines

You can spawn new coroutines from an existing one using the `spawn_now` or `getManager()->spawn_later` methods:

```cpp
io::fsm_func<void> parent_coroutine()
{
    io::fsm<void> &fsm = co_await io::get_fsm;
    
    // Spawn a child coroutine and get a handle to it
    io::fsm_handle<void> child_handle = fsm.spawn_now(simple_coroutine());
    
    // Or detach it to let it run independently
    // child_handle.detach();
    
    std::cout << "Parent coroutine continues after child" << std::endl;
    
    co_return;
}
```

The difference between `spawn_now` and `getManager()->spawn_later`:
- `spawn_now`: Starts the coroutine immediately
- `getManager()->spawn_later`: Queues the coroutine for later execution

### Using Delays

FSMs can create clocks for scheduling operations:

```cpp
io::fsm_func<void> timer_example()
{
    io::fsm<void> &fsm = co_await io::get_fsm;
    
    // Create a timer for 2 seconds
    io::clock timer;
    fsm.make_clock(timer, std::chrono::seconds(2));
    
    std::cout << "Waiting for 2 seconds..." << std::endl;
    
    // Wait for the timer
    co_await timer;
    
    std::cout << "Timer completed!" << std::endl;
    
    // Shorthand for creating and awaiting a timer
    co_await fsm.setTimeout(std::chrono::milliseconds(500));
    
    std::cout << "Another delay completed!" << std::endl;
    
    co_return;
}
```

### FSM with Associated Values

Coroutines can return values using `io::fsm_func<T>` where `T` is the associated type:

```cpp
io::fsm_func<int> compute_value()
{
    io::fsm<int> &fsm = co_await io::get_fsm;
    
    // Perform some computation
    int result = 42;
    
    // Store the result in the FSM's data
    *fsm = result;
    
    co_return;
}

io::fsm_func<void> use_computed_value()
{
    io::fsm<void> &fsm = co_await io::get_fsm;
    
    // Spawn a coroutine that returns a value
    io::fsm_handle<int> handle = fsm.spawn_now(compute_value());
    
    // Access the returned value
    int value = *handle;
    std::cout << "Computed value: " << value << std::endl;
    
    co_return;
}
```

### Managing Coroutine Lifetime

The `fsm_handle<T>` class provides methods for managing coroutine lifetime:

```cpp
io::fsm_func<void> lifetime_example()
{
    io::fsm<void> &fsm = co_await io::get_fsm;
    
    // Create a handle to a coroutine
    io::fsm_handle<void> handle = fsm.spawn_now(simple_coroutine());
    
    // Check if the coroutine is done
    if (handle.done()) {
        std::cout << "Coroutine completed" << std::endl;
    }
    
    // Detach the coroutine (it will continue running independently)
    handle.detach();
    
    // Destroy the coroutine (only if not detached)
    // handle.destroy();
    
    co_return;
}
```

> **Important Notes:**
> - When a `fsm_handle` is destroyed without being detached, the corresponding coroutine is also destroyed.
> - Detached coroutines continue running until they complete or the manager is destroyed.
> - The FSM context (`io::fsm<T>`) is only valid within the coroutine that obtained it.

### Creating a Manager

In most cases, you'll use the default manager provided by io::manager. However, you can create and drive your own manager:

```cpp
int main() {
    // Create a manager
    io::manager mgr;
    
    // Spawn a coroutine
    auto handle = mgr.spawn_later(parent_coroutine());
    
    // Drive the manager (process coroutines)
    while (true) {
        mgr.drive();
        // Break condition when all work is done
        if (handle.done()) break;
    }
    
    return 0;
}
```

The `drive()` method processes pending coroutines, resolves futures, and handles timers. It's typically called in a loop until all work is complete.

## Future/Promise: Coroutine Communication

The future/promise pattern in io::manager provides a powerful way for coroutines to communicate and synchronize with each other. This pattern is similar to JavaScript's Promise system but optimized for C++ coroutines.

> **Important Notes:**
> - future/promise pairs are one-time use only. After resolution or rejection, both future and promise are reset and must be reconstructed for reuse.
> - For data-carrying futures , the data's lifetime must exceed the future's lifetime.
> - If a future is destructed before its corresponding promise, the promise will not be able to access any data pointers from the future side. It guarantees the memory-safeness of future/promise mechanisms.

### Creating and Using Future/Promise Pairs

#### Basic Usage

To create a future/promise pair within a coroutine:

```cpp
// Producer coroutine
io::fsm_func<void> producer_coroutine()
{
    // Get the FSM context
    io::fsm<void> &fsm = co_await io::get_fsm;
    
    // Create a future that will be returned to the consumer
    io::future fut;
    
    // Create a promise that the producer will keep
    io::promise<void> prom = fsm.make_future(fut);
    
    // Spawn a consumer coroutine and pass the future to it
    fsm.spawn_now(consumer_coroutine(std::move(fut))).detach();
    
    // Do some work...
    
    // Resolve the promise when ready, which will resume the consumer coroutine
    prom.resolve();
    
    co_return;
}

// Consumer coroutine
io::fsm_func<void> consumer_coroutine(io::future fut)
{
    // Get the FSM context
    io::fsm<void> &fsm = co_await io::get_fsm;
    
    // Wait for the future to be resolved by the producer
    co_await fut;
    
    // Continue execution after the future is resolved
    std::cout << "Future resolved!" << std::endl;
    
    co_return;
}
```

> **Note:** After `prom.resolve()` is called, both the future and promise become invalid for await operations. To reuse them, you need to create a new future/promise pair.

#### Passing Data with Future/Promise

To pass data from one coroutine to another:

```cpp
// Consumer coroutine (initiates the communication)
io::fsm_func<void> data_consumer()
{
    // Get the FSM context
    io::fsm<void> &fsm = co_await io::get_fsm;
    
    // Consumer creates the future_with that will hold the data
    io::future_with<std::string> fut;
    
    // Create a promise that will be passed to the producer
    io::promise<std::string> prom = fsm.make_future(fut, &fut.data);
    
    // Spawn a producer and pass the promise to it
    fsm.spawn_now(data_producer(std::move(prom))).detach();
    
    // Wait for the future to be resolved by the producer
    co_await fut;
    
    // Access the data
    std::cout << "Received: " << fut.data << std::endl;
    
    co_return;
}

// Producer coroutine
io::fsm_func<void> data_producer(io::promise<std::string> prom)
{
    // Get the FSM context
    io::fsm<void> &fsm = co_await io::get_fsm;
    
    // Set the data and resolve the promise
    *prom.data() = "Hello from producer!";
    prom.resolve();
    
    co_return;
}
```

> **Important:** 
> - The data in fsm.make_future(fut, &data) must have a lifetime longer than the future itself. The `future_with<T>` struct helps you to manage it.
> - If the future is destructed before the promise, `prom.data()` will return a null pointer.

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

io::fsm_func<void> handle_errors(std::reference_wrapper<io::future> fut_ref)
{
    io::fsm<void> &fsm = co_await io::get_fsm;
    
    // Wait for the future to be resolved
    co_await fut_ref.get();
    
    // Check for errors after awaiting
    if (fut_ref.get().getErr())
    {
        std::cout << "Error: " << fut_ref.get().getErr().message() << std::endl;
    }
    else
    {
        std::cout << "Success!" << std::endl;
    }
    
    co_return;
}
```

### Combining Multiple Futures

io::manager provides JavaScript-style combinators for working with multiple futures:

#### all - Wait for all futures to resolve or any future to reject

#### any - Wait for any future to resolve or all future to reject

#### race - Similar to any, but rejects if any future rejects

#### allSettle - Wait for all futures to be set

```cpp
io::fsm_func<void> race_example()
{
    io::fsm<void> &fsm = co_await io::get_fsm;
    
    io::future fut1, fut2;
    io::promise<void> prom1 = fsm.make_future(fut1);
    io::promise<void> prom2 = fsm.make_future(fut2);
    
    // Start two operations in parallel
    fsm.spawn_now([&prom1]() -> io::fsm_func<void> {
        io::fsm<void> &fsm = co_await io::get_fsm;
        // Simulate work
        co_await fsm.setTimeout(std::chrono::seconds(2));
        prom1.resolve();
        co_return;
    }()).detach();
    
    fsm.spawn_now([&prom2]() -> io::fsm_func<void> {
        io::fsm<void> &fsm = co_await io::get_fsm;
        // Simulate work
        co_await fsm.setTimeout(std::chrono::seconds(1));
        prom2.resolve();
        co_return;
    }()).detach();
    
    // Wait for the race result
    co_await io::future::race(fut1, fut2);
    
    std::cout << "Race completed!" << std::endl;
    
    co_return;
}
```

### Multi-thread Resolution with async_future/async_promise

For operations that may complete asynchronously (especially from other threads), use async_future/async_promise:

```cpp
io::fsm_func<void> async_operation()
{
    io::fsm<void> &fsm = co_await io::get_fsm;
    
    // Create an async_future and its corresponding async_promise
    io::async_future fut;
    io::async_promise prom = fsm.make_future(fut);
    
    // Start an asynchronous operation
    std::thread([prom = std::move(prom)]() mutable {
        // Simulate work in another thread
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        // Resolve the async_promise from another thread
        // async_promise is thread-safe
        prom.resolve();
    }).detach();
    
    // Wait for the async_future
    co_await fut;
    
    std::cout << "Async operation completed!" << std::endl;
    
    co_return;
}
```

> **Important:** 
> - Unlike regular promise/future, `async_promise` is thread-safe and can be safely used across threads.
> - The same one-time use rule applies: after resolution or rejection, both async_future and async_promise become invalid.

### Coroutines with Future Return Type

io::manager provides a special mechanism for coroutines that directly return futures. Using `io::fsm_func<io::future>` (or its alias `io::future_fsm_func_`), you can create coroutines that automatically manage future resolution:

```cpp
// A coroutine that returns a future
io::fsm_func<io::future> async_task()
{
    io::fsm<io::future> &fsm = co_await io::get_fsm;
    
    // The future is automatically created and associated with the FSM
    
    // Simulate some asynchronous work
    co_await fsm.setTimeout(std::chrono::seconds(1));
    
    // The future is automatically resolved when the coroutine completes
    // No explicit resolve() call is needed
    co_return;
}

// For futures with data, use io::fsm_func<io::future_with<T>> or its alias io::future_fsm_func<T>
io::future_fsm_func<std::string> async_data_task()
{
    io::fsm<io::future_with<std::string>> &fsm = co_await io::get_fsm;
    
    // Simulate work
    co_await fsm.setTimeout(std::chrono::milliseconds(500));
    
    // Set the data directly in the future
    fsm->data = "Result from async task";
    
    // The future_with is automatically resolved with the data
    co_return;
}

// Using the future-returning coroutines
io::fsm_func<void> use_future_coroutines()
{
    io::fsm<void> &fsm = co_await io::get_fsm;
    
    // Spawn the future-returning coroutine and get a handle
    io::future_fsm_handle_ task_handle = fsm.spawn_now(async_task());
    
    // Await the handle to get the future result
    co_await *task_handle;
    
    // For coroutines with data
    io::future_fsm_handle<std::string> data_task_handle = fsm.spawn_now(async_data_task());
    
    // Await the handle
    co_await *data_task_handle;
    
    // Access the data from the handle
    std::cout << "Received: " << data_task_handle->data << std::endl;
    
    co_return;
}
```

## Protocols and Pipelines

io::manager provides a powerful protocol and pipeline system that enables efficient data flow processing with a clear protocol stream approach. This system is particularly useful for network applications, data processing, and any scenario requiring structured data flow between components.

### Protocol Concept

In io::manager, protocols are divided into two distinct categories:

1. **Output Protocol**: A protocol that can output data, implementing the `operator>>` operation.
   - Defines a `prot_output_type` that specifies the type of data it produces
   - Implements `operator>>(future_with<prot_output_type>&)` for asynchronous data output
   - Used as a data source in a pipeline

2. **Input Protocol**: A protocol that can accept input data, implementing the `operator<<` operation.
   - Defines a `prot_input_type` that specifies the type of data it accepts
   - Implements `operator<<(const prot_input_type&)` that returns a future for asynchronous operations
   - Used as a data sink in a pipeline

Many protocols implement both interfaces, making them dual-protocol components that can both receive and send data.

#### Protocol Interface

A typical dual-protocol implementation includes:

```cpp
struct my_protocol {
    // Define the data types for input and output
    using prot_input_type = InputType;  // Type of data this protocol accepts
    using prot_output_type = OutputType; // Type of data this protocol produces
    
    // Output operation (implements Output Protocol)
    void operator>>(future_with<OutputType>& fut) {
        // Implementation for outputting data
        // When data is available, resolve the future with the data
    }
    
    // Input operation (implements Input Protocol)
    future operator<<(const InputType& data) {
        // Implementation for accepting input data
        // Return a future that resolves when the operation completes
    }
};
```

### Pipeline Mechanism

Pipelines in io::manager allow you to connect multiple protocols together to create a data processing flow. Data flows from one protocol to another, with optional adapters in between to transform the data.

In a pipeline:
- The first protocol must be an Output Protocol
- The last protocol must be an Input Protocol
- Intermediate protocols must implement both interfaces (dual-protocol)
- Adapters can be used to transform data between incompatible protocols

#### Creating a Pipeline

A pipeline is created using the `>>` operator to chain protocols together:

```cpp
auto pipeline = io::pipeline<>() >> output_protocol >> middle_protocol >> input_protocol;
```

#### Adapters

Adapters are functions that transform data between protocols with incompatible types:

```cpp
auto pipeline = io::pipeline<>() >> protocol1 >> 
    [](const Protocol1OutputType& data) -> std::optional<Protocol2InputType> {
        // Transform data from protocol1's output to protocol2's input
        // Return std::nullopt if the data should be skipped
        return transformed_data;
    } >> protocol2;
```

#### Driving a Pipeline

Once a pipeline is created, you can drive it using the `<=` operator and the `+` operator:

```cpp
// Process the pipeline once
pipeline <= co_await +pipeline;
```

#### Complete Pipeline Example

Here's a complete example of a pipeline that transfers data from a TCP socket to a UDP socket and back:

```cpp
io::fsm_func<void> pipeline_example() {
    io::fsm<void>& fsm = co_await io::get_fsm;
    
    // Create protocols
    io::sock::tcp tcp_socket(fsm);
    io::sock::udp udp_socket(fsm);
    
    // Connect TCP socket
    asio::ip::tcp::endpoint tcp_endpoint(
        asio::ip::address::from_string("127.0.0.1"), 8080);
    co_await tcp_socket.connect(tcp_endpoint);
    
    // Bind UDP socket
    asio::ip::udp::endpoint udp_endpoint(
        asio::ip::address::from_string("0.0.0.0"), 8081);
    co_await udp_socket.bind(udp_endpoint);
    
    // Create a pipeline: TCP -> UDP -> TCP
    auto pipeline = io::pipeline<>() >> tcp_socket >> 
        // Adapter from TCP to UDP
        [](const std::span<char>& tcp_data) -> std::optional<std::pair<std::span<char>, asio::ip::udp::endpoint>> {
            // Create a UDP endpoint to send to
            asio::ip::udp::endpoint target(
                asio::ip::address::from_string("127.0.0.1"), 9000);
            return std::make_pair(tcp_data, target);
        } >> udp_socket >> 
        // Adapter from UDP to TCP
        [](const std::pair<std::span<char>, asio::ip::udp::endpoint>& udp_data) -> std::optional<std::span<char>> {
            // Extract just the data part
            return udp_data.first;
        } >> tcp_socket;
    
    // Drive the pipeline in a loop
    while (true) {
        // Wait for the pipeline to complete a cycle
        pipeline <= co_await +pipeline;
        
        std::cout << "Pipeline cycle completed" << std::endl;
    }
    
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
