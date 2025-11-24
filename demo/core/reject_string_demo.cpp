#include <ioManager/ioManager.h>
#include <iostream>
#include <string>

io::fsm_func<void> helper_coroutine1(io::promise<std::string> prom)
{
    io::fsm<void> &fsm = co_await io::get_fsm;

    co_await fsm.setTimeout(std::chrono::milliseconds(100));

    prom.reject("IO Error!");

    co_return;
}

io::fsm_func<void> helper_coroutine2(io::promise<std::string> prom)
{
    io::fsm<void> &fsm = co_await io::get_fsm;

    co_await fsm.setTimeout(std::chrono::milliseconds(150));

    io::future_with<std::string> fut1;
    io::promise<std::string> prom1 = fsm.make_future(fut1, &fut1.data);
    fsm.spawn_now(helper_coroutine1(std::move(prom1))).detach();

    co_await fut1;

    prom.reject("Network Error!");

    co_return;
}

io::fsm_func<void> main_coroutine()
{
    io::fsm<void> &fsm = co_await io::get_fsm;

    io::future_with<std::string> fut1, fut2;
    io::promise<std::string> prom1 = fsm.make_future(fut1, &fut1.data);
    io::promise<std::string> prom2 = fsm.make_future(fut2, &fut2.data);

    fsm.spawn_now(helper_coroutine1(std::move(prom1))).detach();
    fsm.spawn_now(helper_coroutine2(std::move(prom2))).detach();

    std::cout << "===== Waiting for helper coroutines =====" << std::endl;

    std::cout << "\n--- Handling future ---" << std::endl;
    
    co_await io::future::allSettle(fut1, fut2);

    if (fut1.getErr())
    {
        std::cout << "Future 1 rejected: " << fut1.getErr().message() << std::endl;
    }
    else
    {
        std::cout << "Future 1 resolved: " << fut1.data << std::endl;
    }

    if (fut2.getErr())
    {
        std::cout << "Future 2 rejected: " << fut2.getErr().message() << std::endl;
    }
    else
    {
        std::cout << "Future 2 resolved: " << fut2.data << std::endl;
    }

    std::cout << "\n===== All coroutines completed =====" << std::endl;

    co_return;
}

int main()
{
    io::manager mngr;
    mngr.async_spawn(main_coroutine());

    while (1)
    {
        mngr.drive();
    }

    return 0;
}
