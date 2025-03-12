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

## Future/Promise Memory Model

The core of io::manager's asynchronous capabilities is its efficient future/promise implementation. Understanding its memory model is essential for effective usage.

### Core Components

#### Awaiter Structure
At the heart of the future/promise system is the `lowlevel::awaiter` structure:
- Maintains state flags in `bit_set` to track the status of future/promise pairs
- Contains a function pointer `coro` for coroutine resumption
- Uses a union to store either error codes or timer information
- Each awaiter is associated with a manager instance

#### Future and Promise Classes
- `future` holds a pointer to an `awaiter` object
- `promise<T>` inherits from `lowlevel::promise_base` and also holds an `awaiter` pointer
- For data-carrying futures, `future_with<T>` stores data directly within the object

### Memory Management

#### Object Pool Pattern
To minimize allocation overhead, io::manager uses an object pool to manage awaiter objects:

```cpp
// From manager implementation
auto new_awaiter = this->awaiter_hive.emplace();
```

The `awaiter_hive` is a type-specific memory pool that efficiently allocates and recycles awaiter objects, avoiding frequent heap allocations and deallocations.

#### Reference Counting Mechanism
Future/promise pairs use a manual reference counting mechanism:
- The `bit_set` field in awaiter tracks whether promise and future still reference the awaiter
- When a future or promise is destroyed, it clears its respective bit
- When both bits are cleared and no coroutine references exist, the awaiter is returned to the pool

```cpp
// From future::decons()
if ((this->awaiter->bit_set & this->awaiter->promise_handled) == false &&
    this->awaiter->coro == nullptr)
    this->awaiter->erase_this();
else
    this->awaiter->bit_set &= ~this->awaiter->future_handled;
```

### Lifecycle Management

#### Creation
Future/promise pairs are created through the manager's `make_future` method:

```cpp
template <typename T_Prom>
inline promise<T_Prom> make_future(future& fut, T_Prom* mem_bind)
{
    FutureVaild(fut);
    return { fut.awaiter, mem_bind };
}
```

The `FutureVaild` method checks if the future already has an awaiter that can be reused; otherwise, it creates a new one.

#### Data Binding
For `future_with<T>`, data is stored directly in the future object, and the promise references this data via pointer:

```cpp
// From future_with<T>::getPromise()
return io::promise<T>(awaiter, &this->data);
```

This design allows the promise to modify the future's data without additional memory allocation.

#### Resolution and Rejection
Promises can complete futures through `resolve()` or `reject()` methods:

```cpp
// From promise_base::resolve()
awaiter->bit_set |= awaiter->occupy_lock;
this->awaiter->set();
```

When a promise resolves or rejects, it sets the awaiter's `set_lock` bit and calls the awaiter's `set()` method, which triggers the waiting coroutine to resume.

### Coroutine Integration

When a coroutine awaits a future, it registers a resumption function to the awaiter's `coro` field:

```cpp
coro_set = [this](awaiter* awa) {
    // Check conditions and then resume the coroutine
    std::coroutine_handle<io::fsm_promise<T_FSM>> h;
    h = h.from_promise(this->f_p);
    h.resume();
};
```

When the future is resolved, this function is called, resuming the coroutine's execution.

### Composition Operations

The library supports combining multiple futures with operations like `all`, `any`, `race`, and `allSettle`:

```cpp
// Example usage
auto combined_future = io::future::all(future1, future2, future3);
co_await combined_future;
```

These combinators track multiple awaiter objects and implement different completion semantics.

### Memory Safety Considerations

The library explicitly notes several memory safety constraints:

```cpp
// Not Thread safe.
// UB: The lifetime of memory binding by the future-promise pair is shorter than the io::future
// future cannot being co_await by more than one coroutine.
```

This indicates:
1. Future/promise pairs are not thread-safe
2. Memory bound to a future must outlive the future itself
3. A future cannot be awaited by multiple coroutines simultaneously

### Deferred Execution

The library supports deferred resolution/rejection:

```cpp
// From promise_base::resolve_later()
awaiter->no_tm.queue_next = awaiter;
std::swap(awaiter->no_tm.queue_next, awaiter->mngr->resolve_queue_local);
awaiter->mngr->suspend_release();
```

These operations don't immediately trigger coroutine resumption but queue the awaiter for later processing by the manager.

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
