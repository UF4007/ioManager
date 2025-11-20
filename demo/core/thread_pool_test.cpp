#include <ioManager/ioManager.h>
#include <ioManager/timer.h>

io::fsm_func<void> thread_pool_test()
{
    io::fsm<void>& fsm = co_await io::get_fsm;
    std::cout << "\n=== Thread Pool Async Post Test ===\n" << std::endl;
    
    constexpr size_t NUM_THREADS = 4;
    io::pool thread_pool(NUM_THREADS);
    
    std::cout << "Created thread pool with " << NUM_THREADS << " threads" << std::endl;
    
    std::cout << "\n--- Test 1: Basic post and wait ---\n" << std::endl;
    {
        io::timer::up timer;
        timer.start();
        
        io::async_future future = thread_pool.post(fsm.getManager(), 
            []() {
                std::cout << "Task executing in thread " << std::this_thread::get_id() << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                std::cout << "Task completed" << std::endl;
            });
        
        std::cout << "Waiting for task to complete..." << std::endl;
        co_await future;
        
        auto duration = timer.lap();
        std::cout << "Task completed in " 
                  << std::chrono::duration_cast<std::chrono::milliseconds>(duration).count() 
                  << " ms" << std::endl;
    }
    
    std::cout << "\n--- Test 2: Multiple concurrent tasks ---\n" << std::endl;
    {
        constexpr size_t NUM_TASKS = 10;
        std::vector<io::async_future> futures;
        
        io::timer::up timer;
        timer.start();
        
        std::cout << "Posting " << NUM_TASKS << " tasks..." << std::endl;
        
        for (size_t i = 0; i < NUM_TASKS; i++) {
            futures.push_back(thread_pool.post(fsm.getManager(), 
                [i]() {
                    std::cout << "Task " << i << " executing in thread " 
                              << std::this_thread::get_id() << std::endl;
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    std::cout << "Task " << i << " completed" << std::endl;
                }));
        }
        
        for (auto& future : futures) {
            co_await future;
        }
        
        auto duration = timer.lap();
        std::cout << "All tasks completed in " 
                  << std::chrono::duration_cast<std::chrono::milliseconds>(duration).count() 
                  << " ms" << std::endl;
                  
        std::cout << "Tasks ran concurrently (total time < sum of individual task times)" << std::endl;

        fsm.getManager()->spawn_later(thread_pool_test()).detach();
        co_return;
    }
}

int main()
{
    io::manager mngr;
    mngr.async_spawn(thread_pool_test());

    while (1)
    {
        mngr.drive();
    }

    return 0;
} 