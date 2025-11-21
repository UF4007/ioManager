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
- **chan**：Go风格的io::chan和io::async::chan，用于协程间通信、线程间通信
- **RAII友好**：资源管理遵循RAII原则，确保安全性和可靠性
- **高性能**：接近原生C++协程的速度
- **有栈协程**：与无栈协程使用相同的future/promise等待子；无栈跑得快，有栈应用广，我们全都要！

## 目录

- [有限状态机(FSM)：协程函数](#有限状态机fsm协程函数)
  - [创建管理器](#创建管理器)
  - [生成和管理协程](#生成和管理协程)
  - [延时](#延时)
  - [带内建值的FSM](#带内建值的fsm)
  - [管理协程生命周期](#管理协程生命周期)
- [Future/Promise：协程通信](#futurepromise协程通信)
  - [创建和使用Future/Promise对](#创建和使用futurepromise对)
  - [错误处理](#错误处理)
  - [组合多个Future](#组合多个future)
  - [多线程：async_future/async_promise](#多线程async_futureasync_promise)
  - [带Future返回类型的协程](#带future返回类型的协程)
- [chan与async::chan：高性能协程/线程通信通道](#chan与asyncchan高性能协程线程通信通道)
- [协议和管线](#协议和管线)
  - [协议Concept](#协议concept)
  - [管线机制](#管线机制)
- [性能](#性能)
- [杂项](#杂项)

## 使用

- 支持C++20的编译器
- 支持的平台：Windows、Linux

cmake安装（可选）  
```bash
git clone https://github.com/UF4007/ioManager.git
cd ioManager
cmake .
cmake --install .
```

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

> - manager析构时不会销毁所有未完成协程帧（包括detach协程）。这是为了避免每个协程维护析构表带来的性能开销。
> - manager的设计宗旨是与进程同寿，通常不会频繁创建和销毁。当进程退出时，所有未完成的协程会被操作系统自动回收。
> - 如果有特殊需求（如希望在manager析构时强制清理所有协程），可以由用户自行维护协程注册表（通过协程内建值+侵入式链表最佳），在析构时手动销毁相关协程。

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

`spawn_now` `manager::spawn_later` `manager::async_spawn` 之间的区别：

- `spawn_now`：立即启动协程，返回 `fsm_handle`，非线程安全。
- `getManager()->spawn_later`：将协程排队等待稍后执行，返回 `fsm_handle`，非线程安全。
- `getManager()->async_spawn`：异步排队，并自动detach，不返回，线程安全。

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
// 协程内建值为promise，外部多次送入，协程定时resolve，外部用future等待
io::fsm_func<io::promise<void>> timer_worker() {
    io::fsm<io::promise<void>>& fsm = co_await io::get_fsm;
    while (true) {
        co_await fsm.setTimeout(std::chrono::seconds(1));
        fsm->resolve();
    }
}

// 外部代码
auto handle = fsm.spawn_now(timer_worker());
for (int i = 0; i < 3; ++i) {
    io::future fut;
    io::promise<void> prom = fsm.make_future(fut);
    *handle = std::move(prom); // 送入promise
    co_await fut;
}
```

- 内建值采用默认构造函数构造，其生命周期与协程帧同生共死。
- 内建值更利于协程与外部代码在RAII的基础上互相通信：  
- 如果没有内建值（如 asio 的一次性 awaitable 协程），要实现长期运行的状态机或任务，通常需要通过协程参数传递指针、引用或智能指针等“指针类似物”来与外部通信和保存状态。这样做不仅容易出错，还可能带来额外的内存分配和管理负担。

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
> - FSM上下文（`io::fsm<T>&`）仅在获取它的协程内有效。
> - `fsm_handle`完全不是线程安全的。不要用于任何多线程操作。

## Future/Promise：协程通信

[查看future-promise对内存模型](./document/README_Memory_zh.md#1-future%2Fpromise对)

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

> **注意：** future/promise对是一次性的。在调用`prom.resolve()`后，promise将无效，future将不能再用于await操作。必须重新make才能重用。
> - 当一个左值future正在被等待时，试图操作它，行为未定义。

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
    
    // 启动两个操作
    fsm.spawn_now([](io::promise<> prom) -> io::fsm_func<void> {
        io::fsm<void> &fsm = co_await io::get_fsm;
        // 模拟工作
        co_await fsm.setTimeout(std::chrono::seconds(2));
        prom.resolve();
        co_return;
    }(std::move(prom1))).detach();
    
    fsm.spawn_now([](io::promise<> prom) -> io::fsm_func<void> {
        io::fsm<void> &fsm = co_await io::get_fsm;
        // 模拟工作
        co_await fsm.setTimeout(std::chrono::seconds(1));
        prom.resolve();
        co_return;
    }(std::move(prom2))).detach();
    
    // 等待race结果
    IO_SELECT_BEGIN(io::future::race(fut1, fut2))
    // 此处执行默认结果（all任意失败、any任意成功、allSettle完成）
    // Default Codes...
    IO_SELECT(fut1)
    // 此处对应 fut1 被触发的代码
    // Codes...
    IO_SELECT(fut2)
    // 此处对应 fut2 被触发的代码
    // Codes...
    IO_SELECT_END
    
    std::cout << "Race完成！" << std::endl;
    
    co_return;
}
```

### 多线程：async_future/async_promise

[查看异步future-promise对内存模型](./document/README_Memory_zh.md#2-async_future%2Fasync_promise对)

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
> - 注意：`async_future` 只保证 resolve/reject 操作的线程安全，co_await 操作必须在创建它的线程/manager 内进行，不能跨线程 co_await，否则行为未定义。

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
    
    // 等待协程句柄内建的future
    co_await *task_handle;
    
    // 对于内建future_with的协程
    io::future_fsm_handle<std::string> data_task_handle = fsm.spawn_now(async_data_task());
    
    // 等待协程句柄内建的future
    co_await *data_task_handle;
    
    // 从句柄访问数据
    std::cout << "接收到: " << data_task_handle->data << std::endl;
    
    co_return;
}
```

## chan与async::chan：高性能协程/线程通信通道

[查看chan内存模型](./document/README_Memory_zh.md#3-chan)
[查看async::chan内存模型](./document/README_Memory_zh.md#4-async_chan)

类似 Golang 的 chan。内部有一个智能指针指向实际的chan控制块。

#### 设计理念

- **chan**：适用于同一 manager（线程）内的高效协程通信，采用环形缓冲区实现，零动态分配，极低延迟。
- **async::chan**：支持多线程安全，适合跨线程/跨 manager 通信，内部采用分段链表+自旋锁，需要动态内存分配。

#### 用法对比

| 特性           | chan                | async::chan                |
|----------------|---------------------|----------------------------|
| 线程安全       | 否                  | 是                         |
| 内存分配       | 零动态分配（环形缓冲）| 动态分配（分段链表）       |
| 并发支持       | 多生产者/多消费者    | 多生产者/多消费者          |
| 顺序保证       | FIFO                | FIFO（SCSP时严格有序）     |
| 关闭/异常      | 支持                | 支持                       |

#### 典型用法

**chan（单线程/单manager）**

```cpp
io::chan<int> ch(fsm, 100);                 // 实际会分配100个int的堆内存
co_await (ch << std::span<int>(&value, 1)); // 发送
co_await ch.get_and_copy(std::span<int>(&recv, 1)); // 接收
```

**async::chan（多线程/多manager）**

```cpp
io::async::chan<int> ch(fsm, 100);          // 并不分配int的堆内存，每次调用时动态内存分配+复制
// 线程A
co_await (ch << std::span<int>(&value, 1)); // 发送
// 线程B
co_await ch.listen(); // 等待有数据
auto n = ch.accept(std::span<int>(&recv, 1)); // 尝试接收，返回实际接受数量
```

#### 注意事项

- chan 只能在同一 manager/线程内使用，async::chan 可跨线程。
- 关闭通道后，所有未完成的 future 会被 reject。
- 不支持 select 语法，但可用 future::race 等组合器实现多路等待。

## 协议和管线

### 协议Concept

各个异步类传递异步消息的方式千奇百怪。为规范它们，抽象出了"协议"的接口。

协议分为两种：

#### 输出协议（2种类型）

1. **带Future的输出协议**：
   - 定义了非void的`prot_output_type`，指定它产生的数据类型
   - 实现了`void operator>>(future_with<prot_output_type>&)`，用于带关联数据的异步数据输出，数据会输出到输入的引用中
   - 通过等待future以接收数据
   - 除非函数中特别说明，否则一个协议对象的输出/输入协议运算符（即 `operator>>`、`operator<<`）在上一次调用返回的 future 未完成前，不能再次调用，否则为未定义行为（UB）。如 `io::prot::chan` 等协议会特别说明支持多次并发调用。

2. **直接输出协议**：
   - 定义了非void的`prot_output_type`，指定它产生的数据类型
   - 实现了`void operator>>(prot_output_type&)`，用于不带future的直接数据输出，数据会输出到输入的引用中
   - 适用于任意时间、任意次数都可立即获取的协议

#### 输入协议（2种类型）

1. **返回Future的输入协议**：
   - 实现了返回`future`的`operator<<(T&)`，其中T可以被前级协议输出的`prot_output_type`或适配器返回类型转换得出。
   - 输入协议的 `operator<<` 参数通常是数据的引用。实现时禁止保留该引用的地址或指针，输入引用的数据必须仅在本次函数调用期间读取。
   - 返回的future在有能力接受下个数据时resolve  
   - 除非函数中特别说明，否则一个协议对象的输出/输入协议运算符（即 `operator>>`、`operator<<`）在上一次调用返回的 future 未完成前，不能再次调用，否则为未定义行为（UB）。如 `io::prot::chan` 等协议会特别说明支持多次并发调用。

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

[查看管线机制内存模型](./document/README_Memory_zh.md#5-管线)

管线其实就是把所有协议组成一个个出-入对（管线段），并且统一在某个协程中平行处理。

1. **平行触发**：管线的每个段可以平行、独立触发，并搬运数据，而不依赖前面或后面的段。

2. **单向**：数据在管线段中从输出协议流向输入协议，方向单一。

3. **有序循环**：管线段会首先读取输出协议，再写入输入协议，再读取输出协议，如此循环。若读取过程阻塞，则此段一定会先等待读取再写入；反之亦然。

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

#### 管线原理

所有管线段的future会被同时等待（内部通过future::race实现）。每一段都有独立的四状态状态机，只要任意一段的future被触发，awaitable就会被触发，协程恢复。此时，只有触发的那一段会推进到下一个状态，其它段保持原状态。  
每一段的状态推进是完全独立的。每一段的四状态状态机控制着：等待读取、读取、等待写入、写入的步骤，并按此循环。

与C++ 26 Sender/Receiver及类似惰性模型的区别：ioManager的管线是"所有段同时等待、平行推进"，而Sender/Receiver是"严格链式依赖、顺序推进"。

## 性能

基于 Intel Core i5-9300HF CPU @ 2.40GHz 和 24GB RAM 的 Windows 环境（MSVC VS2022）进行的基准测试结果：

### 协程性能

#### 无栈协程（C++20 coroutine）

30000协程下：

- 平均每秒创建协程数：约 512 万次/秒
- 平均协程创建时间：约 195 纳秒

- 平均每秒切换次数：约 1065 万次/秒
- 平均切换时间：约 93.9 纳秒

#### 有栈协程（minicoro）

3000协程下：

- 平均每秒创建协程数：约 13.8 万次/秒
- 平均协程创建时间：约 7.22 微秒

- 平均每秒切换次数：约 850 万次/秒
- 平均切换时间：约 117.6 纳秒

**性能对比分析：**

| 指标 | 无栈 | 有栈 | 倍数差异 |
|------|------|------|---------|
| 创建速度 | 195 ns | 7223 ns | **37倍** |
| 切换速度 | 93.9 ns | 117.6 ns | **1.25倍** |
| 并发规模 | 30000+ | < 1000 | - |

*此基准测试的实现在 `demo/core/coro_benchmark.cpp` 。*

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

*此基准测试的实现在 `demo/socket/tcp_throughput_test.cpp` 。*

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

*此基准测试的实现在 `demo/socket/tcp_concurrent_connections_test.cpp`。*

### HTTP服务器性能测试

此测试评估库中单线程最小HTTP RPC服务器实现的性能。

| 测试场景 | 配置 | 每秒请求数 | 平均延迟 |
|---------|------|-----------|---------|
| 高并发负载 | 10,000连接 | 23,064.67 | 31.57ms |
| 中等负载 | 100连接 | 25,665.37 | 3.55ms |

*HTTP服务器实现在 `demo/protocol/http/http_rpc_demo.cpp`。*

## 使用的库

- 使用[Asio](https://think-async.com/Asio/)提供网络支持
- KCP协议实现基于[ikcp](https://github.com/skywind3000/kcp)  
- HTTP解析由[llhttp](https://github.com/nodejs/llhttp)提供支持  
- 有栈协程由[minicoro](https://github.com/edubart/minicoro)实现

## 杂项

### io::pool 线程池工具类

**TODO**：让manager由负载均衡器，自动运行在不同的线程内。很快就会实现这个。

```cpp
// 创建线程池，指定线程数量（manager数量）
io::pool thread_pool(4); // 4线程池

// post —— 向线程池投递同步阻塞任务，返回async_future，可co_await等待完成
// future_carrier为future归属的manager，func为要执行的函数，args为参数
io::async_future fut = thread_pool.post(fsm->getManager(), blocking_func, args...);
co_await fut;

// async_spawn —— 向线程池投递一个协程（fsm_func），自动分配到某个manager执行
thread_pool.async_spawn(my_coro());

// stop —— 停止线程池，等待所有线程安全退出
thread_pool.stop();

// is_running —— 判断线程池是否正在运行
bool running = thread_pool.is_running();
```

### manager::post —— 阻塞函数的异步适配

`manager::post` 可将阻塞/同步函数投递到指定 manager（线程/调度器）异步执行，返回 `io::async_future`，便于协程 `co_await` 等待。

**用法示例：**

```cpp
io::async_future fut = fsm->getManager()->post(execute_manager, blocking_func, args...);
//                          ^^^^^^^^^^^^
//                          持有async_future的线程
//                                              ^^^^^^^^^^^^^^
//                                              真正执行目标函数的线程、线程池
co_await fut;
```

**性能注意：**

- 每次调用都会创建和销毁一个协程，分配 future/promise，参数打包，队列调度，并可能涉及线程切换。
- 适合偶发阻塞操作，高频/批量场景下性能开销较大，不建议频繁使用。

### io::timer 计时器工具类

```cpp
// io::timer::counter —— 计数器型定时器
io::timer::counter counter(3); // 构造，计数目标3
counter.count(); // 增加计数（可指定步长）
counter.stop(); // 强制计数到目标
counter.reset(); // 重置计数
bool reached = counter.isReach(); // 是否已达到计数目标
co_await counter.onReach(fsm); // 计数完成时可await

// io::timer::down —— 补偿型倒计时定时器
io::timer::down down_timer(5); // 构造，5次倒计时
down_timer.start(std::chrono::seconds(1)); // 启动，周期1秒
co_await down_timer.await_tm(fsm); // 协程等待下一个周期点，自动补偿调度延迟
down_timer.reset(); // 重置计数和起点
auto dur = down_timer.getDuration(); // 获取当前周期时长
bool reached2 = down_timer.isReach(); // 是否已达到计数目标
co_await down_timer.onReach(fsm); // 计数完成时可await

// io::timer::up —— 计时器/秒表
io::timer::up timer;
timer.start(); // 启动计时，记录当前时间点
auto interval = timer.lap(); // 获取距离上次start/lap的时间间隔，并重置起点
auto elapsed = timer.elapsed(); // 获取距离上次start/lap的累计时间（不重置起点）
timer.reset(); // 重置计时器，清空起点
```

### IO_DEFER 宏 —— defer_t的推荐用法

`IO_DEFER` 是配合 `io::defer_t` 使用的宏，可自动生成唯一变量名，简化作用域清理写法。

```cpp
IO_DEFER([&]{
    // 作用域退出时自动执行的代码
    cleanup();
});
```
