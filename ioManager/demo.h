#include<stdio.h>
//#include<mariadb/mysql.h>
#define IO_USE_SELECT 1
#include "ioManager.h"

io::coAsync<> jointest(io::ioManager* para)
{
    io::coPromise<> prom(para);
    prom.setTimeout(std::chrono::milliseconds(1000));
    task_await(prom);
    co_return true;
}

io::coTask benchmark_coroutine(io::ioManager* para)
{
    while (1)
    {
        io::coPromise<> joint = jointest(para);
        task_await(joint);
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
        task_await(timer);
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
        task_multi_await(multi);
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

io::ioChannel<float, int> count_proto1(io::ioManager* para)
{
    using promise_value = io::ioChannel<float, int>::promise_value;
    promise_value* prom;
    co_yield &prom;
    while (1)
    {
        task_await(prom->in_handle);
        if (prom->destroy)
            co_return;
        if (*prom->in_handle.data() > 3)
        {
            *prom->in_handle.data() = 0;
            for (auto& out : prom->out_slot)
            {
                *out.data() += 1.0;         // original data consume
                std::cout << "channel1 out:" << *out.data() << std::endl;
                out.resolveLocal();
            }
        }
        prom->in_handle.reset();
    }
    co_return;
}

io::ioChannel<std::string, float> count_proto2(io::ioManager* para)
{
    using promise_value = io::ioChannel<std::string, float>::promise_value;
    promise_value* prom;
    co_yield &prom;
    while (1)
    {
        task_await(prom->in_handle);
        if (prom->destroy)
            co_return;
        if (*prom->in_handle.data() > 3.0)
        {
            *prom->in_handle.data() = 0.0;
            for (auto& out : prom->out_slot)
            {
                *out.data() += "A";         // original data consume
                std::cout << "channel2 out:" << *out.data() << std::endl;
                out.resolveLocal();
            }
        }
        prom->in_handle.reset();
    }
    co_return;
}

io::coTask count_producer(io::ioManager* para, io::coPromise<int> channel_in)
{
    using namespace std::chrono_literals;
    while (1)
    {
        io::coPromise<> delayer = io::coPromise<>(para);
        task_delay(delayer, 1ms);
        *channel_in.data() += 1;        // original network flow received
        std::cout << "original in:" << *channel_in.data() << std::endl;
        channel_in.resolveLocal();
    }
    co_return;
}

void io_testmain()
{
    io::ioManager context;

    //coroutine library benchmark
    if (false)
    {
        io::ioManager::auto_go(1);  //single thread
        for (int i = 0; i < 1000000; i++)   //less than 1 us per task recircle when in 1M coroutines (condition: same timeout for each promise)
        {
            io::ioManager::auto_once(benchmark_coroutine);
        }
        std::this_thread::sleep_for(std::chrono::years(30));
    }

    //coSelector test
    if (false)
    {
        io::ioManager::auto_go(1);
        io::ioManager::auto_once(test_multi);
        std::this_thread::sleep_for(std::chrono::years(30));
    }

    //ioChannel test
    if (true)
    {
        io::ioChannel<float, int> chan1 = count_proto1(&context);
        io::ioChannel<std::string, float> chan2 = count_proto2(&context);
        io::coPromise<std::string> final_out = io::coPromise<std::string>(&context);
        //count_producer(&context, chan1.get_in_promise());

        final_out << chan2 << chan1;

        while(1)
        {
            context.drive();
        }
    }

    //coroutine test
    for (int i = 0; i < 64; i++)
    {
        //io::ioManager::auto_once(test_udp_client);
    }
    std::this_thread::sleep_for(std::chrono::years(30));
}