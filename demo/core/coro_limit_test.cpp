#include <ioManager/ioManager.h>

io::fsm_func<void> limit_test(int num)
{
    io::fsm<void> &fsm = co_await io::get_fsm;
    for (int i = 0; i < num; i++)
    {
        fsm.spawn_now([]() -> io::fsm_func<void>
                      {
                io::fsm<void>& fsm = co_await io::get_fsm;
                io::clock cl1, cl2;
                io::future fut;
                while (1)
                {
                    fsm.make_clock(cl1, std::chrono::seconds(1));
                    fsm.make_clock(cl2, std::chrono::seconds(2));
                    io::promise prom = fsm.make_future(fut);
                    IO_SELECT_BEGIN(io::future::race(cl1, cl2, fut))
                        // default scenario:
                        //  - all the future had successed when in io::future::all
                        //  - all the future had error when in io::future::any
                        //  - use io::future::allSettle
                    IO_SELECT(cl1)
                        // clock 1 triggered
                    IO_SELECT(cl2)
                        // clock 2 triggered
                    IO_SELECT(fut)
                        // fut triggered
                    IO_SELECT_END
                } }())
            .detach();
    }
}

int main()
{
    io::manager mngr;
    mngr.async_spawn(limit_test(1000000));
    // mngr.async_spawn(limit_test(10000000)); //Up to 10M coroutines are supported and it will cost about 7GB RAM.

    while (1)
    {
        mngr.drive();
    }

    return 0;
}