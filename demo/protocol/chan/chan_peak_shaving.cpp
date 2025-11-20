#include <ioManager/ioManager.h>
#include <ioManager/timer.h>
#include <ioManager/pipeline.h>
#include <ioManager/protocol/chan.h>
#include <ioManager/protocol/async_chan.h>

io::fsm_func<void> chan_peak_shaving()
{
    io::fsm<void> &fsm = co_await io::get_fsm;

    struct ProducerProtocol {
        using prot_output_type = int;
        
        io::manager* mngr;
        int counter = 0;
        
        ProducerProtocol(io::fsm<void>& fsm) : mngr(fsm.getManager()) {
            fsm_handle = fsm.spawn_now(produce(this));
        }
        
        void operator>>(io::future_with<int>& fut) {
            promise = mngr->make_future(fut, &fut.data);
            fsm_handle->resolve_later();
        }
        
        io::fsm_func<io::promise<>> produce(ProducerProtocol* pthis) {
            io::fsm<io::promise<>> &fsm = co_await io::get_fsm;

            io::future fut;
            
            while (true) {
                *fsm = fsm.make_future(fut);
                co_await fut;

                // idle producer speed
                co_await fsm.setTimeout(std::chrono::milliseconds(200));
                pthis->promise.resolve_later(pthis->counter);

                if (pthis->counter % 30 == 0)
                {
                    for (int i = 0; i < 20; i++)
                    {
                        *fsm = fsm.make_future(fut);
                        co_await fut;

                        // producer boost speed
                        co_await fsm.setTimeout(std::chrono::milliseconds(5));
                        pthis->promise.resolve_later(1);
                    }
                }

                pthis->counter++;
                
                std::cout << "Produced: " << pthis->counter << std::endl;
            }
            
            co_return;
        }
        
        io::promise<int> promise;
        io::fsm_handle<io::promise<>> fsm_handle;
    };

    struct ConsumerProtocol {
        io::manager* mngr;
        io::timer::up timer;
        int value;
        
        ConsumerProtocol(io::fsm<void>& fsm) : mngr(fsm.getManager()) {
            timer.start();
            fsm_handle = fsm.spawn_now(consume(this));
        }
        
        // Input protocol implementation
        io::future operator<<(const int& input) {
            value = input;
            
            io::future fut;
            prom = mngr->make_future(fut);

            fsm_handle->resolve_later();
            return fut;
        }
        
        io::fsm_func<io::promise<>> consume(ConsumerProtocol* pthis) {
            io::fsm<io::promise<>> &fsm = co_await io::get_fsm;

            io::future fut;
            
            while (true) {
                *fsm = fsm.make_future(fut);
                co_await fut;

                co_await fsm.setTimeout(std::chrono::milliseconds(50));

                auto duration = pthis->timer.lap();
                std::cout << "Consumed: " << pthis->value
                    << " (took: "
                    << std::chrono::duration_cast<std::chrono::milliseconds>(duration).count()
                    << "ms)" << std::endl;

                pthis->prom.resolve_later();
            }
            
            std::cout << "Consumer finished" << std::endl;
            co_return;
        }
        
        io::promise<> prom;
        io::fsm_handle<io::promise<>> fsm_handle;
    };

    ProducerProtocol producer(fsm);
    ConsumerProtocol consumer(fsm);

    //io::chan<int> ch = io::chan<int>(fsm, 10);
    io::async::chan<int> ch = io::async::chan<int>(fsm, 10);
    
    // With a chan
    auto pipeline = io::pipeline<>() >> producer >> [ch](const int& a)mutable->std::optional<int> {
        std::cout << "Channel capacity: (" << ch.size() << "/" << ch.capacity() << ")" << std::endl;
        return a;
        } >> io::prot::chan(ch) >> consumer;

    // Without chan, the producer will be block during the consumer being await.
    //auto pipeline = io::pipeline<>() >> producer >> consumer;
    
    auto started_pipeline = std::move(pipeline).start();

    while(1)
    {
        //drive the pipeline
        started_pipeline <= co_await +started_pipeline;
    }
    
    co_return;
}

int main()
{
    io::manager mngr;
    mngr.async_spawn(chan_peak_shaving());

    while (1)
    {
        mngr.drive();
    }

    return 0;
} 