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
                    do {
                        auto tag = co_await io::future::race(cl1, cl2, fut);
                        if (tag == cl1) {}
                    } while (0);;
                } }())
            .detach();
    }
}

int main()
{
    io::manager mngr;
    mngr.async_spawn(limit_test(10000000));

    while (1)
    {
        mngr.drive();
    }

    return 0;
}