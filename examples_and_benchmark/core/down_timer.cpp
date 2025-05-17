#include <ioManager/ioManager.h>
#include <ioManager/timer.h>

io::fsm_func<void> down_timer() {
    auto& fsm = co_await io::get_fsm;
    io::timer::down timer(10);
    timer.start(std::chrono::seconds(1));
    while (timer.isReach() == false)
    {
        std::cout << "tick" << std::endl;
        co_await timer.await_tm(fsm);
    }
    std::cout << "end" << std::endl;
}

int main()
{
    io::manager mngr;
    mngr.async_spawn(down_timer());

    while (1)
    {
        mngr.drive();
    }

    return 0;
} 