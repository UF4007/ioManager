#include<stdio.h>
//#include<mariadb/mysql.h>
#define IO_USE_SELECT 1
#include "ioManager.h"

asio::awaitable<void> asioCoroTest()
{
    using namespace std::chrono_literals;
    static int time = 1;
    auto executor = co_await asio::this_coro::executor;
    asio::steady_timer timer(executor, 1s);
    while(1)
    {
        //std::cout << "a" << std::endl;
        co_await timer.async_wait(asio::use_awaitable);
        timer.expires_after(std::chrono::seconds(1));
        time++;
        if (time > 5)
            time = 1;
    }
    co_return;
}

io::coAsync<> jointest(io::ioManager* para)
{
    using namespace std::chrono_literals;
    static int time = 1;
    io::coPromise<> prom(para);
    prom.setTimeout(std::chrono::seconds(1));
    time++;
    if (time > 5)
        time = 1;
    co_await *prom;
    co_return true;
}

io::coTask benchmark_coroutine(io::ioManager* para)
{
    while (1)
    {
        io::coPromise<> joint = jointest(para);
        co_await *joint;
        static int i = 0;
        //std::cout << i++ << std::endl;    //watch cpu usage rate curve. it will be tidal.
    }
}

io::coTask randomFunc(io::ioManager* para, io::coPromise<> prom)
{
    //prom.resolve(); //primary status of coMultiplex
    static int num = 1;
    int n = num++;
    io::coPromise<> timer(para);
    while (1)
    {
        int i = rand() % 5000 + 100;
        timer.reset();
        timer.setTimeout(std::chrono::microseconds(i));
        co_await *timer;
        //prom.resolve();
        prom.resolveLocal();
        std::cout << "rand send: " << n << ", await during: " << i << std::endl;
    }
}

void randomFunc2(io::ioManager* para, io::coPromise<> prom)
{
    static int num = 1;
    int n = num++;
    while (1)
    {
        int i = rand() % 3000;
        std::this_thread::sleep_for(std::chrono::nanoseconds(i));
        if (prom.tryOccupy() == io::err::ok)
        {
            std::cout << "rand send: " << n << ", await during: " << i << std::endl;
            prom.resolve();
        }
    }
}

io::coTask test_multi(io::ioManager* para)
{
    //single thread
    //io::coPromise<> rand1(para);
    //randomFunc(para, rand1);
    //io::coPromise<> rand2(para);
    //randomFunc(para, rand2);
    //io::coPromise<> rand3(para);
    //randomFunc(para, rand3);

    //multi thread
    io::coPromise<> rand1(para);
    std::thread(randomFunc2, para, rand1).detach();
    io::coPromise<> rand2(para);
    std::thread(randomFunc2, para, rand2).detach();
    io::coPromise<> rand3(para);
    std::thread(randomFunc2, para, rand3).detach();

    while (1)
    {
        io::coSelector multi(rand1, rand2, rand3);         //memory leak test, deconstruct and reconstruct constantly. In usual programming, we don't do this.
        co_await *multi;
        std::cout << "triggered!" << std::endl;
        if (rand1.isSettled())
        {
            std::cout << "rand recv: 1" << std::endl;
            rand1.reset();
        }
        if (rand2.isSettled())
        {
            std::cout << "rand recv: 2" << std::endl;
            rand2.reset();
        }
        if (rand3.isSettled())
        {
            std::cout << "rand recv: 3" << std::endl;
            rand3.reset();
        }
        multi.remove(rand1);
        multi.remove(rand2);
        multi.remove(rand3);
    }
}

void io_testmain()
{
    io::ioManager context;

    //asio coroutine benchmark
    if (false)
    {
        asio::io_context asio_(1);
        for (int i = 0; i < 250000; i++)   //asio coroutine is about one fourth CPU performance compare with ioManager, memory usage:400MB at 250k coroutines.
        {
            asio::co_spawn(asio_, asioCoroTest, asio::detached);
        }
        while (1)
        {
            asio_.run();
        }
    }

    //coroutine library benchmark
    if (true)
    {
        io::ioManager::auto_go(1);  //single thread
        for (int i = 0; i < 1000000; i++)   //less than 1 us per task recircle when in 1M coroutines, memory usage:460MB
        {
            io::ioManager::auto_once(benchmark_coroutine);
        }
        std::this_thread::sleep_for(std::chrono::years(30));
    }

    //coSelector test
    if (true)
    {
        io::ioManager::auto_go(1);
        io::ioManager::auto_once(test_multi);
        std::this_thread::sleep_for(std::chrono::years(30));
    }

    //coroutine test
    for (int i = 0; i < 64; i++)
    {
        //io::ioManager::auto_once(test_udp_client);
    }
    std::this_thread::sleep_for(std::chrono::years(30));
}