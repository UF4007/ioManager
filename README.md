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

<div align="right">
    <a href="README_zh.md">
        <img src="https://img.shields.io/badge/语言-中文-red.svg" alt="Chinese">
    </a>
    <a href="README.md">
        <img src="https://img.shields.io/badge/Language-English-blue.svg" alt="English">
    </a>
</div>

# io::manager - High Performance Pipeline Concurrency Library

A modern C++20 header-only library for high-performance asynchronous I/O operations and coroutine-based concurrency.

## Overview

io::manager provides a comprehensive solution for building efficient, concurrent applications using C++20 coroutines. It implements a pipeline concurrency model with clear data protocol stream processing, making it ideal for network applications, real-time systems, and any scenario requiring high-performance asynchronous operations.

### Key Features

- **Pipeline Concurrency Model**: Efficient data flow processing with a clear protocol stream approach
- **Future/Promise Pattern**: JS-style future/promise with all/any/race/allSettle operations
- **Channel-based Communication**: Golang-style channels and async channels for inter-coroutine communication and inter-thread communication
- **RAII Friendly**: Resource management follows RAII principles for safety and reliability
- **High Performance**: Close to the speed of native C++ coroutines, supporting more than 100M coroutines per second switch speed in a single thread

## Table of Contents

- [Finite State Machine (FSM): The Core of io::manager](#finite-state-machine-fsm-the-core-of-iomanager)
  - [Creating a Manager](#creating-a-manager)
  - [Creating a Basic FSM Coroutine](#creating-a-basic-fsm-coroutine)
  - [Spawning and Managing Coroutines](#spawning-and-managing-coroutines)
  - [Using Delays](#using-delays)
  - [FSM with Associated Values](#fsm-with-associated-values)
  - [Managing Coroutine Lifetime](#managing-coroutine-lifetime)
- [Future/Promise: Coroutine Communication](#futurepromise-coroutine-communication)
  - [Creating and Using Future/Promise Pairs](#creating-and-using-futurepromise-pairs)
  - [Error Handling](#error-handling)
  - [Combining Multiple Futures](#combining-multiple-futures)
  - [Multi-thread Resolution with async_future/async_promise](#multi-thread-resolution-with-async_futureasync_promise)
  - [Coroutines with Future Return Type](#coroutines-with-future-return-type)
- [Protocols and Pipelines](#protocols-and-pipelines)
  - [Protocol Concept](#protocol-concept)
  - [Pipeline Mechanism](#pipeline-mechanism)
- [Performance](#performance)
- [License](#license)
- [Contributing](#contributing)
- [Acknowledgements](#acknowledgements)

## Requirements and Installation

- C++20 compatible compiler
- Supported platforms: Windows, Linux

io::manager is a header-only library. Simply include the main header file in your project:

```cpp
#include "ioManager/ioManager.h"
```

## Finite State Machine (FSM): The Core of io::manager

At the heart of io::manager is its Finite State Machine (FSM) implementation, which provides the foundation for creating and managing coroutines. Understanding how to create and use FSMs is essential before diving into more advanced features.

### Creating a Manager

a io::manager == a thread == coroutine executor

```cpp
io::manager mgr;
int main() {
    // Drive the manager (process coroutines)
    while (true) {
        mgr.drive();
    }
    
    return 0;
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

There is built-in value in coroutines using `io::fsm_func<T>` where `T` is the associated type:

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
> - The FSM context (`io::fsm<T>&`) is only valid within the coroutine that obtained it.

## Future/Promise: Coroutine Communication

The future/promise pattern in io::manager provides a powerful way for coroutines to communicate and synchronize with each other. This pattern is similar to JavaScript's Promise system but optimized for C++ coroutines.

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

> **Note:** After `prom.resolve()` is called, the promise become invalid and the future cannot be used for await operations. Must be reconstructed for reuse.

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
> - If the future is destructed before the promise, `prom.data()` will return a null pointer. It guarantees the memory-safeness of future/promise mechanisms.

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

Asynchronous classes transfer asynchronous messages in countless different ways. To standardize them, we've abstracted the "Protocol" interface.

### Protocol Concept

In io::manager, protocols are divided into two main categories with specific subtypes:

#### Output Protocols (2 types)

1. **Future-with Output Protocol**: 
   - Defines a non-void `prot_output_type` that specifies the type of data it produces
   - Implements `void operator>>(future_with<prot_output_type>&)` for asynchronous data output with associated data
   - Allows awaiting the future to receive data

2. **Direct Output Protocol**: 
   - Defines a non-void `prot_output_type` that specifies the type of data it produces
   - Implements `void operator>>(prot_output_type&)` for direct data output without futures
   - A protocol that can be obtained instantly at any time and any number of times

#### Input Protocols (2 types)

1. **Future-returning Input Protocol**: 
   - Implements `operator<<(const T&)` that returns a `future`, where T can be converted from the `prot_output_type` of the previous protocol or the return type of an adapter
   - The returned future resolves when the operation completes

2. **Direct Input Protocol**:
   - Implements `operator<<(const T&)` that returns `void`, where T can be converted from the `prot_output_type` of the previous protocol or the return type of an adapter
   - Operation completes synchronously or manages its own completion

#### Protocol Interface

A protocol class can only have one output type defined in `prot_output_type`, but it can accept numerous input types.

A typical dual-protocol implementation includes:

```cpp
struct my_protocol {
    using prot_output_type = OutputType; // Type of data this protocol produces
    
    // Output operation (implements Output Protocol)
    // Can be any of the two output protocol types
    void operator>>(future_with<OutputType>& fut) {
        //operator>> Need to be responsible for the construction of future
        io::promise<OutputType> promise = fsm.make_future(fut,&fut.data);
        // Implementation for outputting data
        // When data is available, resolve the future with the data
    }
    
    // Input operation (implements Input Protocol)
    // Can be either of the two input protocol types
    future operator<<(const InputType& data) {
        // Implementation for accepting input data
        // Return a future that resolves when the operation completes
    }
};
```

### Pipeline Mechanism

The pipeline actually groups all protocols into output-input pairs (pipeline segments) and processes them in a specific coroutine.

1. **Independent triggering**: Each segment of the pipeline can be triggered independently and move data without relying on the previous or subsequent segments.

2. **Directionality**: Data flows from the output protocol to the input protocol in the pipeline segment, with a single direction.

3. **Orderly**: The pipeline segment will first read the output protocol, then write the input protocol, then read the output protocol, and so on. If the reading process is blocked, it will wait for reading before writing; vice versa.

In a pipeline:
- The first protocol must be an Output Protocol
- The last protocol must be an Input Protocol
- Intermediate protocols must implement both interfaces (dual-protocol)
- Adapters can be used to transform data between incompatible protocols
- Two Direct protocols (Direct Output Protocol and Direct Input Protocol) cannot be connected to each other in a pipeline segment

#### Creating a Pipeline

A pipeline is created using the `>>` operator to chain protocols together:

```cpp
auto pipeline = io::pipeline<>() >> output_protocol >> middle_protocol >> input_protocol;
```

When creating a pipeline with lvalue references, the pipeline internally stores references to the protocols, and the protocols' lifetime must exceed that of the pipeline. When creating a pipeline with rvalue references, move construction is used to move the rvalues into the pipeline.

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

#### Driving Pipelines

After creating a pipeline, you first need to call the `start()` method to get a `pipeline_started` object, then use the `<=` operator and `+` operator to drive it in a coroutine:

```cpp
// Start the pipeline to get a pipeline_started object
auto started_pipeline = std::move(pipeline).start();

// Process the pipeline once
started_pipeline <= co_await +started_pipeline;
```

You can also provide an error handler callable when starting the pipeline. The callable is invoked when any future in the pipeline returns an error:

```cpp
// Start the pipeline with an error handler
auto started_pipeline = std::move(pipeline).start([](int which, bool output_or_input, std::error_code ec) {
    std::cout << "Error in pipeline segment " << which 
              << (output_or_input ? " (output)" : " (input)")
              << " - Error: " << ec.message()
              << " [Code: " << ec.value() << "]" << std::endl;
    return;
});
```

The error handler callable should have a signature of `void(int, bool, std::error_code)`. Parameters:

- `which`: The segment number where the error occurred
- `output_or_input`: If true, the error occurred at the output side of the pipeline segment, otherwise at the input side
- `ec`: The error code object containing detailed error information

Alternatively, you can spawn the pipeline directly into a coroutine that will drive it continuously:

```cpp
// Spawn the pipeline into a coroutine that will drive it
auto pipeline_handle = std::move(pipeline).spawn(fsm);

// Spawn with an error handler
auto pipeline_handle = std::move(pipeline).spawn(fsm, [](int which, bool output_or_input, std::error_code ec) {
    std::cout << "Error in pipeline segment " << which 
              << (output_or_input ? " (output)" : " (input)")
              << " - Error: " << ec.message() << std::endl;
    return;
});
```

The `spawn` method creates a coroutine that continuously drives the pipeline without manual driving.

The `pipeline_started` class encapsulates the started state of the pipeline and prevents further modification. This design ensures:

1. Users must call `pipeline::start()` to get a drivable pipeline.
2. Once a pipeline is started, no new protocols can be added.
3. The `pipeline_started` class is non-movable and non-copyable, ensuring pipeline stability.

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
    
    // Start the pipeline to get a pipeline_started object
    auto started_pipeline = std::move(pipeline).start();
    
    // Drive the pipeline in a loop
    while (true) {
        // Wait for the pipeline to complete a cycle
        started_pipeline <= co_await +started_pipeline;
        
        std::cout << "Pipeline cycle completed" << std::endl;
    }
    
    co_return;
}
```

## Performance

Benchmark results on Windows environment (MSVC VS2022) with Intel Core i5-9300HF CPU @ 2.40GHz and 24GB RAM: (30000 coroutines)

### Coroutine Creation Performance
- Average coroutine creation rate: ~4.8 million per second
- Average creation time: ~208 nanoseconds

### Coroutine Switching Performance
- Average switching rate: ~115 million switches per second
- Average switch time: ~8.7 nanoseconds

These performance metrics demonstrate the high efficiency of ioManager, particularly in coroutine switching where it can handle over 100 million switches per second, approaching the performance of native C++20 coroutines. The library also achieves efficient memory usage through its pooled allocation strategy.

## License

This project is licensed under the MIT License - see the [License.txt](License.txt) file for details.

## Contributing

Contributions are welcome! Feel free to open issues or submit pull requests on GitHub.

## Acknowledgements

- Uses [Asio](https://think-async.com/Asio/) for network support
- KCP protocol implementation based on [ikcp](https://github.com/skywind3000/kcp)
