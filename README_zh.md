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

# io::manager - 管线化并发

- **管线化并发**：将协议组装成自动收发数据的管线
- **Future/Promise**：JavaScript风格的future/promise，支持all/any/race/allSettle操作
- **chan**：Go语言风格的chan和async_chan，用于协程间通信、线程间通信（还没有做完）
- **RAII友好**：资源管理遵循RAII原则，确保安全性和可靠性
- **高性能**：接近原生C++协程的速度

## 目录

- [有限状态机(FSM)：协程函数](#有限状态机FSM协程函数)
  - [创建管理器](#创建管理器)
  - [创建基本的FSM协程](#创建基本的fsm协程)
  - [生成和管理协程](#生成和管理协程)
  - [使用延迟](#使用延迟)
  - [带内建值的FSM](#带内建值的fsm)
  - [管理协程生命周期](#管理协程生命周期)
- [Future/Promise：协程通信](#futurepromise协程通信)
  - [创建和使用Future/Promise对](#创建和使用futurepromise对)
  - [错误处理](#错误处理)
  - [组合多个Future](#组合多个future)
  - [多线程：async_future/async_promise](#多线程async_futureasync_promise)
  - [带Future返回类型的协程](#带future返回类型的协程)
- [协议和管线](#协议和管线)
  - [协议Concept](#协议Concept)
  - [管线机制](#管线机制)
- [性能](#性能)

## 使用

- 支持C++20的编译器
- 支持的平台：Windows、Linux

```cpp
#include "ioManager/ioManager.h"
```

## 有限状态机(FSM)：协程函数

### 创建管理器

一个管理器 == 一个线程 == 协程执行器

```cpp
io::manager mgr;
int main() {
    // 驱动管理器（处理协程）
    while (true) {
        mgr.drive();
    }
    
    return 0;
}
```

### 生成和管理协程

使用`spawn_now`或`getManager()->spawn_later`方法从现有协程生成新的协程：

```cpp
io::fsm_func<void> parent_coroutine()
{
    io::fsm<void> &fsm = co_await io::get_fsm;
    
    // 生成子协程并获取其句柄
    io::fsm_handle<void> child_handle = fsm.spawn_now(simple_coroutine());
    
    // 或者分离它，让它独立运行
    // child_handle.detach();
    
    std::cout << "父协程在子协程之后继续执行" << std::endl;
    
    co_return;
}
```

`spawn_now`和`getManager()->spawn_later`的区别：
- `spawn_now`：立即启动协程
- `getManager()->spawn_later`：将协程排队等待稍后执行

### 延时

```cpp
io::fsm_func<void> timer_example()
{
    io::fsm<void> &fsm = co_await io::get_fsm;
    
    // 创建一个2秒的定时器
    io::clock timer;
    fsm.make_clock(timer, std::chrono::seconds(2));
    
    std::cout << "等待2秒..." << std::endl;
    
    // 等待定时器
    co_await timer;
    
    std::cout << "定时器完成！" << std::endl;
    
    // 创建并等待定时器的简写方式
    co_await fsm.setTimeout(std::chrono::milliseconds(500));
    
    std::cout << "另一个延迟完成！" << std::endl;
    
    co_return;
}
```

### 带内建值的FSM

协程可以使用`io::fsm_func<T>`内建值，其中`T`是内建类型：

```cpp
io::fsm_func<int> compute_value()
{
    io::fsm<int> &fsm = co_await io::get_fsm;
    
    // 执行一些计算
    int result = 42;
    
    // 将结果存储在FSM的内建值中
    *fsm = result;
    
    co_return;
}

io::fsm_func<void> use_computed_value()
{
    io::fsm<void> &fsm = co_await io::get_fsm;
    
    // 生成一个返回值的协程
    io::fsm_handle<int> handle = fsm.spawn_now(compute_value());
    
    // 访问返回的值
    int value = *handle;
    std::cout << "计算的值: " << value << std::endl;
    
    co_return;
}
```

### 管理协程生命周期

`fsm_handle<T>`类提供了管理协程生命周期的方法：

```cpp
io::fsm_func<void> lifetime_example()
{
    io::fsm<void> &fsm = co_await io::get_fsm;
    
    // 创建一个协程的句柄
    io::fsm_handle<void> handle = fsm.spawn_now(simple_coroutine());
    
    // 检查协程是否完成
    if (handle.done()) {
        std::cout << "协程已完成" << std::endl;
    }
    
    // 分离协程（它将继续独立运行）
    handle.detach();
    
    // 销毁协程（仅当未分离时）
    // handle.destroy();
    
    co_return;
}
```

> **重要提示：**
> - 当`fsm_handle`在未分离的情况下被销毁时，相应的协程也会被销毁。
> - 分离的协程会继续运行，直到它们完成或管理器被销毁。
> - FSM上下文（`io::fsm<T>&`）仅在获取它的协程内有效。

## Future/Promise：协程通信

这种模式类似于JavaScript的Promise系统，但针对C++协程进行了优化。

### 创建和使用Future/Promise对

#### 基本用法

要在协程中创建future/promise对：

```cpp
// 生产者协程
io::fsm_func<void> producer_coroutine()
{
    // 获取FSM上下文
    io::fsm<void> &fsm = co_await io::get_fsm;
    
    // 创建一个将返回给消费者的future
    io::future fut;
    
    // 创建一个生产者将保留的promise
    io::promise<void> prom = fsm.make_future(fut);
    
    // 生成一个消费者协程并将future传递给它
    fsm.spawn_now(consumer_coroutine(std::move(fut))).detach();
    
    // 执行一些工作...
    
    // 准备好时resolve promise，这将恢复消费者协程
    prom.resolve();
    
    co_return;
}

// 消费者协程
io::fsm_func<void> consumer_coroutine(io::future fut)
{
    // 获取FSM上下文
    io::fsm<void> &fsm = co_await io::get_fsm;
    
    // 等待future被生产者resolve 
    co_await fut;
    
    // futureresolve 后继续执行
    std::cout << "Future已resolve ！" << std::endl;
    
    co_return;
}
```

> **注意：** future/promise对是一次性的。在调用`prom.resolve()`后，promise将无效，future将不能再用于await操作。必须重新构造才能重用。

#### 使用Future/Promise传递数据

要从一个协程向另一个协程传递数据：

```cpp
// 消费者协程（启动通信）
io::fsm_func<void> data_consumer()
{
    // 获取FSM上下文
    io::fsm<void> &fsm = co_await io::get_fsm;
    
    // 消费者创建将保存数据的future_with
    io::future_with<std::string> fut;
    
    // 创建将传递给生产者的promise
    io::promise<std::string> prom = fsm.make_future(fut, &fut.data);
    
    // 生成一个生产者并将promise传递给它
    fsm.spawn_now(data_producer(std::move(prom))).detach();
    
    // 等待future被生产者resolve 
    co_await fut;
    
    // 访问数据
    std::cout << "接收到: " << fut.data << std::endl;
    
    co_return;
}

// 生产者协程
io::fsm_func<void> data_producer(io::promise<std::string> prom)
{
    // 获取FSM上下文
    io::fsm<void> &fsm = co_await io::get_fsm;
    
    // 设置数据并resolve promise
    *prom.data() = "来自生产者的问候！";
    prom.resolve();
    
    co_return;
}
```

> **重要提示：** 
> - fsm.make_future(fut, &data)中的数据必须具有比future本身更长的生命周期。`future_with<T>`结构体可以帮助您管理它。
> - 如果future在promise之前被销毁，`prom.data()`将返回空指针。这保证了future/promise机制的内存安全性。

### 错误处理

Promise也可以使用错误代码reject：

```cpp
io::fsm_func<void> error_example()
{
    io::fsm<void> &fsm = co_await io::get_fsm;
    
    io::future fut;
    io::promise<void> prom = fsm.make_future(fut);
    
    // 使用错误reject promise
    prom.reject(std::make_error_code(std::errc::operation_canceled));
    
    co_return;
}

io::fsm_func<void> handle_errors(std::reference_wrapper<io::future> fut_ref)
{
    io::fsm<void> &fsm = co_await io::get_fsm;
    
    // 等待future被resolve 
    co_await fut_ref.get();
    
    // 等待后检查错误
    if (fut_ref.get().getErr())
    {
        std::cout << "错误: " << fut_ref.get().getErr().message() << std::endl;
    }
    else
    {
        std::cout << "成功！" << std::endl;
    }
    
    co_return;
}
```

### 组合多个Future

提供JavaScript风格的组合器，用于处理多个future。组合器返回的类型不能被长期保存，需要立刻co_await：

#### all - 等待所有future resolve 或任何future reject 

#### any - 等待任何future resolve 或所有future reject 

#### race - 任何future被设置，则返回

#### allSettle - 等待所有future被设置

```cpp
io::fsm_func<void> race_example()
{
    io::fsm<void> &fsm = co_await io::get_fsm;
    
    io::future fut1, fut2;
    io::promise<void> prom1 = fsm.make_future(fut1);
    io::promise<void> prom2 = fsm.make_future(fut2);
    
    // 并行启动两个操作
    fsm.spawn_now([&prom1]() -> io::fsm_func<void> {
        io::fsm<void> &fsm = co_await io::get_fsm;
        // 模拟工作
        co_await fsm.setTimeout(std::chrono::seconds(2));
        prom1.resolve();
        co_return;
    }()).detach();
    
    fsm.spawn_now([&prom2]() -> io::fsm_func<void> {
        io::fsm<void> &fsm = co_await io::get_fsm;
        // 模拟工作
        co_await fsm.setTimeout(std::chrono::seconds(1));
        prom2.resolve();
        co_return;
    }()).detach();
    
    // 等待race结果
    co_await io::future::race(fut1, fut2);
    
    std::cout << "Race完成！" << std::endl;
    
    co_return;
}
```

### 多线程：async_future/async_promise

```cpp
io::fsm_func<void> async_operation()
{
    io::fsm<void> &fsm = co_await io::get_fsm;
    
    // 创建一个async_future及其对应的async_promise
    io::async_future fut;
    io::async_promise prom = fsm.make_future(fut);
    
    // 启动一个异步操作
    std::thread([prom = std::move(prom)]() mutable {
        // 在另一个线程中模拟工作
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        // 从另一个线程resolve async_promise
        // async_promise是线程安全的
        prom.resolve();
    }).detach();
    
    // 等待async_future
    co_await fut;
    
    std::cout << "异步操作完成！" << std::endl;
    
    co_return;
}
```

> **重要提示：** 
> - 与常规promise/future不同，`async_promise`是线程安全的，可以安全地跨线程使用。
> - 一次性：在resolve 或reject 后，async_future和async_promise都变为无效，需要重新构造

### 带Future返回类型的协程

当FSM内建值为future时，有一种特殊机制：自动构造此future，并在返回时resolve：

```cpp
// 返回future的协程
io::fsm_func<io::future> async_task()
{
    io::fsm<io::future> &fsm = co_await io::get_fsm;
    
    // future自动创建并与FSM关联
    
    // 模拟一些异步工作
    co_await fsm.setTimeout(std::chrono::seconds(1));
    
    // 协程完成时future自动resolve 
    // 不需要显式调用resolve()
    co_return;
}

// 对于带数据的future，使用io::fsm_func<io::future_with<T>>或其别名io::future_fsm_func<T>
io::future_fsm_func<std::string> async_data_task()
{
    io::fsm<io::future_with<std::string>> &fsm = co_await io::get_fsm;
    
    // 模拟工作
    co_await fsm.setTimeout(std::chrono::milliseconds(500));
    
    // 直接在future中设置数据
    fsm->data = "来自异步任务的结果";
    
    // future_with自动resolve 并带有数据
    co_return;
}

// 使用返回future的协程
io::fsm_func<void> use_future_coroutines()
{
    io::fsm<void> &fsm = co_await io::get_fsm;
    
    // 生成返回future的协程并获取句柄
    io::future_fsm_handle_ task_handle = fsm.spawn_now(async_task());
    
    // 等待句柄以获取future结果
    co_await *task_handle;
    
    // 对于带数据的协程
    io::future_fsm_handle<std::string> data_task_handle = fsm.spawn_now(async_data_task());
    
    // 等待句柄
    co_await *data_task_handle;
    
    // 从句柄访问数据
    std::cout << "接收到: " << data_task_handle->data << std::endl;
    
    co_return;
}
```

## 协议和管线

### 协议Concept

各个异步类传递异步消息的方式千奇百怪。为规范它们，抽象出了"协议"的接口。

协议分为两种：

#### 输出协议（2种类型）

1. **带Future的输出协议**：
   - 定义了非void的`prot_output_type`，指定它产生的数据类型
   - 实现了`void operator>>(future_with<prot_output_type>&)`，用于带关联数据的异步数据输出
   - 通过等待future以接收数据

2. **直接输出协议**：
   - 定义了非void的`prot_output_type`，指定它产生的数据类型
   - 实现了`void operator>>(prot_output_type&)`，用于不带future的直接数据输出
   - 适用于任意时间、任意次数都可立即获取的协议

#### 输入协议（2种类型）

1. **返回Future的输入协议**：
   - 实现了返回`future`的`operator<<(T&)`，其中T可以被前级协议输出的`prot_output_type`或适配器返回类型转换得出。
   - 返回的future在操作完成时resolve 

2. **直接输入协议**：
   - 实现了返回`void`的`operator<<(T&)`，其中T可以被前级协议输出的`prot_output_type`或适配器返回类型转换得出。
   - 操作能够立刻完成

#### 协议接口

一个协议类只能有一个定义在`prot_output_type`的输出类型，但是可以有无数个输入类型。

一个典型的双向协议实现：

```cpp
struct my_protocol {
    using prot_output_type = OutputType; // 此协议的输出数据类型
    
    // 输出操作（实现输出协议）
    // 可以是两种输出协议类型中的任何一种
    void operator>>(future_with<OutputType>& fut) {
        //operator>>需要负责future的构造
        io::promise<OutputType> promise = fsm.make_future(fut,&fut.data);
        // 输出数据的实现
        // 当数据可用时，resolve此future对应的promise
    }
    
    // 输入操作（实现输入协议）
    // 可以是两种输入协议类型中的任何一种
    future operator<<(InputType& data) {
        // 接受输入数据的实现
        // 返回一个在输入操作完成时resolve 的future
    }
};
```

### 管线机制

管线其实就是把所有协议组成一个个出-入对（管线段），并且统一在某个协程中进行处理。

1. **独立触发**：管线的每个段可以独立触发，并搬运数据，而不依赖前面或后面的段。

2. **单向**：数据在管线段中从输出协议流向输入协议，方向单一。

3. **有序循环**：管线段会首先读取输出协议，再写入输入协议，再读取输出协议，如此循环。若读取过程阻塞，则一定会先等待读取再写入；反之亦然。

在管线中：
- 第一个协议必须是输出协议
- 最后一个协议必须是输入协议
- 中间协议必须同时实现两个接口（双向协议）
- 适配器可用于在不兼容的协议之间转换数据
- 两个直接协议（直接输出协议和直接输入协议）不能连接

#### 创建管线

使用`>>`操作符将协议链接在一起创建管线：

```cpp
auto pipeline = io::pipeline<>() >> output_protocol >> middle_protocol >> input_protocol;
```

若使用左值引用创建管线，则管线内部保存协议的左值引用，协议的生命周期必须长于管线。若调用右值创建管线，则会使用移动构造，移动这个右值到管线内部。

#### 适配器

适配器是在具有不兼容类型的协议之间转换数据的可调用体：

```cpp
auto pipeline = io::pipeline<>() >> protocol1 >> 
    [](Protocol1OutputType& data) -> std::optional<Protocol2InputType> {
        // 将数据从protocol1的输出转换为protocol2的输入
        // 主动放弃该数据时，返回std::nullopt
        return transformed_data;
    } >> protocol2;
```

#### 驱动管线

创建管线后，需要先调用 `start()` 方法获取一个 `pipeline_started` 对象，然后在协程中使用 `<=` 操作符和 `+` 操作符驱动它：

```cpp
// 启动管线，获取 pipeline_started 对象
auto started_pipeline = std::move(pipeline).start();

// 处理管线一次
started_pipeline <= co_await +started_pipeline;
```

您也可以在启动管线时提供一个错误处理可调用体。当管线中的任何 future 返回错误时，可调用体被调用：

```cpp
// 使用错误处理器启动管线
auto started_pipeline = std::move(pipeline).start([](int which, bool output_or_input, std::error_code ec) {
    std::cout << "管线段 " << which << " 发生错误" 
              << (output_or_input ? " (output)" : " (input)")
              << " - 错误: " << ec.message()
              << " [代码: " << ec.value() << "]" << std::endl;
    return;
});
```

错误处理可调用体应该具有 `void(int, bool, std::error_code)` 的签名。参数含义：

- `which`：发生错误的管线段序号
- `output_or_input`：若为 true，则表示发生错误的是管线段的出口端，否则为入口端
- `ec`：包含具体错误信息的错误代码对象

另外，您可以直接将管线生成到一个会持续驱动它的协程中：

```cpp
// 将管线生成到一个会持续驱动它的协程中
auto pipeline_handle = std::move(pipeline).spawn(fsm);

// 带错误处理器的生成
auto pipeline_handle = std::move(pipeline).spawn(fsm, [](int which, bool output_or_input, std::error_code ec) {
    std::cout << "管线段 " << which << " 发生错误"
              << (output_or_input ? " (output)" : " (input)")
              << " - 错误: " << ec.message() << std::endl;
    return;
});
```

`spawn` 方法创建一个持续驱动管线的协程，无需手动驱动。

`pipeline_started` 类封装了管线的启动状态，并防止进一步修改。这种设计确保：

1. 用户必须调用 `pipeline::start()` 才能获得可驱动的管线。
2. 一旦管线启动，就不能添加新的协议。
3. `pipeline_started` 类是不可移动和不可复制的，确保管线的稳定性。

## 性能

基于 Intel Core i5-9300HF CPU @ 2.40GHz 和 24GB RAM 的 Windows 环境（MSVC VS2022）进行的基准测试结果：

### 协程性能

30000协程下：

- 平均每秒创建协程数：约 4.8 百万次/秒
- 平均协程创建时间：约 208 纳秒

- 平均每秒切换次数：约 1.15 亿次/秒
- 平均切换时间：约 8.7 纳秒

*此基准测试的实现可以在 `demo.h` 文件中的 `coro_benchmark()` 函数中找到。*

GCC下的速度甚至更快，已接近原生 C++20 协程的性能。

### TCP吞吐量测试

该测试测量使用ioManager库的TCP客户端和服务器之间的最大数据传输速率。

**测试配置：**
- 数据包大小：64KB
- 测试持续时间：每次运行5秒
- 连接：本地回环（127.0.0.1）

**结果：**

| 运行次数 | 传输数据量 | 数据包数 | 吞吐量 |
|---------|------------|---------|--------|
| 1       | 5334.69 MB | 85,355  | 8950.07 Mbps |
| 2       | 6134.69 MB | 98,155  | 10292.20 Mbps |
| 3       | 5734.88 MB | 91,758  | 9621.47 Mbps |

**平均值：** 9621.25 Mbps（约9.62 Gbps）

*此基准测试的实现可以在 `demo.h` 文件中的 `coro_tcp_throughput_test()` 函数中找到。*

这些结果显示了本地TCP通信的卓越性能，吞吐量接近10 Gbps。

### TCP并发连接测试

该测试评估建立和维护大量并发TCP连接的能力。

**测试配置：**
- 连接：本地回环（127.0.0.1）
- 连接建立：分批进行（每批100个连接）

**10,000连接目标的结果：**

| 运行次数 | 建立的连接数 | 耗时 | 连接速率 |
|---------|-------------|------|---------|
| 1       | 10,000      | 3.38秒 | 2953.55个/秒 |
| 2       | 10,000      | 3.10秒 | 3223.65个/秒 |

**100,000连接目标的结果：**

| 运行次数 | 建立的连接数 | 耗时 | 连接速率 |
|---------|-------------|------|---------|
| 1       | 64,165      | 25.55秒 | 2511.29个/秒 |
| 2       | 64,168      | 25.61秒 | 2505.32个/秒 |

*此基准测试的实现可以在 `demo.h` 文件中的 `coro_tcp_concurrent_connections_test()` 函数中找到。*

系统成功建立了10,000个并发连接，速率超过每秒3,000个连接。当尝试达到100,000个连接时，系统达到了约64,000个连接，这是由于单个回环IP地址上可用临时端口范围的实际限制。

### HTTP服务器性能测试

此测试评估库中最小HTTP RPC服务器实现的性能。

| 测试场景 | 配置 | 每秒请求数 | 平均延迟 |
|---------|------|-----------|---------|
| 高并发负载 | 12线程，10,000连接，30秒 | 23,064.67 | 31.57ms |
| 中等负载 | 2线程，100连接，10秒 | 25,665.37 | 3.55ms |

*HTTP服务器实现可以在`demo.h`文件中的`coro_http_rpc_demo()`函数中找到。*

这些结果展示了出色的HTTP服务器性能，即使在极高负载（10,000并发连接）下也能处理每秒超过23,000个请求。在中等负载（100连接）下，延迟显著降低至平均仅3.55毫秒，同时保持每秒超过25,000个请求的高吞吐量。

## 使用的库

- 使用[Asio](https://think-async.com/Asio/)提供网络支持
- KCP协议实现基于[ikcp](https://github.com/skywind3000/kcp) 
- HTTP解析由[llhttp](https://github.com/nodejs/llhttp)提供支持 