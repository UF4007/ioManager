#include <ioManager/ioManager.h>

io::fsm_func<io::awaitable> demo(int initial)
{
    io::fsm<io::awaitable> &fsm = co_await io::get_fsm;
    io::fsm_handle<io::awaitable> task_handle;
    if (initial)
    {
        task_handle = fsm.getManager()->spawn_later(demo(initial - 1));
        co_await *task_handle;
        std::cout << initial << std::endl;
    }
    if (fsm->operator bool())
        fsm->resume();
}

int main()
{
    io::manager mngr;
    mngr.async_spawn(demo(100));

    while (1)
    {
        mngr.drive();
    }

    return 0;
} 