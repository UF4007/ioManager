#include <cstdio>
#include <iostream>
#include <string>
#include <vector>
#include <ioManager/ioManager.h>
#include <ioManager/pipeline.h>

io::fsm_func<void> pipeline_test() {

    // Protocol implementations for testing

    // 1. Direct Output Protocol - outputs data directly without future
    struct DirectOutputProtocol {
        using prot_output_type = std::string;

        std::vector<std::string> data = { "Hello", "World", "Testing", "Pipeline" };
        size_t current_index = 0;

        void operator>>(std::string& output) {
            // Direct output to the reference
            output = data[current_index];
            current_index = (current_index + 1) % data.size();
            std::cout << "DirectOutputProtocol produced: " << output << std::endl;
        }
    };

    // 2. Future Output Protocol - outputs data through a future
    struct FutureOutputProtocol {
        using prot_output_type = int;

        int counter = 0;

        io::manager* mngr;
        FutureOutputProtocol(io::manager* mngr) :mngr(mngr) {}

        void operator>>(io::future_with<int>& fut) {
            // Output through future
            fut.data = counter++;
            std::cout << "FutureOutputProtocol produced: " << fut.data << std::endl;
            auto promise = mngr->make_future(fut, &counter);
            promise.resolve();
        }
    };

    // 3. Direct Input Protocol - accepts data directly without returning a future
    struct DirectInputProtocol {
        void operator<<(std::string& input) {
            // Process input directly
            std::cout << "DirectInputProtocol received: " << input << std::endl;
        }
    };

    // 4. Future Input Protocol - accepts data and returns a future
    struct FutureInputProtocol {
        io::manager* mngr;
        FutureInputProtocol(io::manager* mngr) :mngr(mngr) {}
        
        io::future operator<<(int& input) {
            // Process input and return a future
            std::cout << "FutureInputProtocol received: " << input << std::endl;
            
            // Create a future to return
            io::future fut;
            io::promise<void> prom = mngr->make_future(fut);
            
            // Resolve the promise immediately for this example
            prom.resolve();
            return fut;
        }
    };

    // 5. Bidirectional Protocol with Direct I/O - implements both direct input and output
    struct DirectBidirectionalProtocol {
        using prot_output_type = std::string;

        std::vector<std::string> data = { "Processed", "Data", "From", "Bidirectional" };
        size_t current_index = 0;

        void operator>>(std::string& output) {
            // Direct output
            output = data[current_index];
            current_index = (current_index + 1) % data.size();
            std::cout << "DirectBidirectionalProtocol produced: " << output << std::endl;
        }

        void operator<<(std::string& input) {
            // Direct input
            std::cout << "DirectBidirectionalProtocol received: " << input << std::endl;
        }
    };

    // 6. Bidirectional Protocol with Future I/O - implements both future input and output
    struct FutureBidirectionalProtocol {
        using prot_output_type = int;
        
        int counter = 100;
        io::manager* mngr;
        
        FutureBidirectionalProtocol(io::manager* mngr) :mngr(mngr) {}
        
        void operator>>(io::future_with<int>& fut) {
            // Output through future
            fut.data = counter++;
            std::cout << "FutureBidirectionalProtocol produced: " << fut.data << std::endl;
            io::promise<void> prom = mngr->make_future(fut);
            prom.resolve();
        }
        
        io::future operator<<(int& input) {
            // Process input and return a future
            std::cout << "FutureBidirectionalProtocol received: " << input << std::endl;
            
            // Create a future to return
            io::future fut;
            io::promise<void> prom = mngr->make_future(fut);
            
            // Resolve the promise immediately for this example
            prom.resolve();
            return fut;
        }
    };

    // 7. Mixed Bidirectional Protocol - direct output and future input
    struct MixedBidirectionalProtocol1 {
        using prot_output_type = std::string;
        
        std::vector<std::string> data = { "Mixed", "Protocol", "Direct", "Output" };
        size_t current_index = 0;
        io::manager* mngr;
        
        MixedBidirectionalProtocol1(io::manager* mngr) :mngr(mngr) {}

        void operator>>(io::future_with<std::string>& fut) {
            // Future output
            fut.data = std::to_string(current_index++);
            std::cout << "MixedBidirectionalProtocol1 produced: " << fut.data << std::endl;
            io::promise<void> prom = mngr->make_future(fut);
            prom.resolve();
        }
        
        io::future operator<<(int& input) {
            // Future input
            std::cout << "MixedBidirectionalProtocol1 received: " << input << std::endl;
            
            // Create a future to return
            io::future fut;
            io::promise<void> prom = mngr->make_future(fut);
            
            // Resolve the promise immediately for this example
            prom.resolve();
            return fut;
        }
    };

    // 8. Mixed Bidirectional Protocol - future output and direct input
    struct MixedBidirectionalProtocol2 {
        using prot_output_type = int;
        
        int counter = 200;
        io::manager* mngr;
        
        MixedBidirectionalProtocol2(io::manager* mngr) :mngr(mngr) {}
        
        void operator>>(io::future_with<int>& fut) {
            // Future output
            fut.data = counter++;
            std::cout << "MixedBidirectionalProtocol2 produced: " << fut.data << std::endl;
            io::promise<void> prom = mngr->make_future(fut);
            prom.resolve();
        }
        
        void operator<<(std::string& input) {
            // Direct input
            std::cout << "MixedBidirectionalProtocol2 received: " << input << std::endl;
        }
    };

    io::fsm<void>& fsm = co_await io::get_fsm;
    
    std::cout << "\n=== Testing Pipeline Connections ===\n" << std::endl;
    
    // Test 1: DirectOutput -> DirectInput is not allowed in the pipeline
    // According to the README, two direct protocols cannot be directly connected
    // We need to use at least one future-based protocol
    
    // Test 2: FutureOutput -> DirectInput
    std::cout << "\n--- Test 2: FutureOutput -> DirectInput ---\n" << std::endl;
    {
        FutureOutputProtocol output_prot(fsm.getManager());
        DirectInputProtocol input_prot;
        
        // Need an adapter to convert int to string
        auto pipeline = io::pipeline<>() >> output_prot
            >> [](int& value) -> std::optional<std::string> {
            return "Number: " + std::to_string(value);
            } 
        >> input_prot;
        auto started_pipeline = std::move(pipeline).start();
        
        for (int i = 0; i < 3; i++) {
            std::cout << "Cycle " << i+1 << ":" << std::endl;
            started_pipeline <= co_await +started_pipeline;
        }
    }
    
    // Test 3: DirectOutput -> FutureInput
    std::cout << "\n--- Test 3: DirectOutput -> FutureInput ---\n" << std::endl;
    {
        DirectOutputProtocol output_prot;
        FutureInputProtocol input_prot(fsm.getManager());
        
        // Need an adapter to convert string to int
        auto pipeline = io::pipeline<>() >> output_prot >> 
            [](std::string& value) -> std::optional<int> {
                return static_cast<int>(value.length());
            } >> input_prot;
        auto started_pipeline = std::move(pipeline).start();
        
        for (int i = 0; i < 3; i++) {
            std::cout << "Cycle " << i+1 << ":" << std::endl;
            started_pipeline <= co_await +started_pipeline;
        }
    }
    
    // Test 4: FutureOutput -> FutureInput
    std::cout << "\n--- Test 4: FutureOutput -> FutureInput ---\n" << std::endl;
    {
        FutureOutputProtocol output_prot(fsm.getManager());
        FutureInputProtocol input_prot(fsm.getManager());
        
        auto pipeline = io::pipeline<>() >> output_prot >> input_prot;
        auto started_pipeline = std::move(pipeline).start();
        
        for (int i = 0; i < 10; i++) {
            std::cout << "Cycle " << i+1 << ":" << std::endl;
            started_pipeline <= co_await +started_pipeline;
        }
    }
    
    // Test 5: FutureOutput -> FutureBidirectional -> FutureInput
    std::cout << "\n--- Test 5: FutureOutput -> FutureBidirectional -> FutureInput ---\n" << std::endl;
    {
        FutureOutputProtocol output_prot(fsm.getManager());
        FutureBidirectionalProtocol middle_prot(fsm.getManager());
        FutureBidirectionalProtocol middle_prot2(fsm.getManager());
        FutureInputProtocol input_prot(fsm.getManager());
        
        auto pipeline = io::pipeline<>() >> output_prot >> middle_prot >> middle_prot2 >> input_prot;
        auto started_pipeline = std::move(pipeline).start();
        
        for (int i = 0; i < 10; i++) {
            std::cout << "Cycle " << i+1 << ":" << std::endl;
            started_pipeline <= co_await +started_pipeline;
        }
    }
    
    // Test 6: Mixed protocols with adapters
    std::cout << "\n--- Test 6: Mixed protocols with adapters ---\n" << std::endl;
    {
        DirectOutputProtocol output_prot;
        MixedBidirectionalProtocol1 middle_prot1(fsm.getManager());
        MixedBidirectionalProtocol2 middle_prot2(fsm.getManager());
        FutureInputProtocol input_prot(fsm.getManager());
        
        auto pipeline = io::pipeline<>() >> output_prot >> 
            [](std::string& value) -> std::optional<int> {
                return static_cast<int>(value.length());
            } >> middle_prot1 >> 
            [](std::string& value) -> std::optional<std::string> {
                return "Transformed: " + value;
            } >> middle_prot2 >> input_prot;
            auto started_pipeline = std::move(pipeline).start();
        
        for (int i = 0; i < 10; i++) {
            std::cout << "Cycle " << i+1 << ":" << std::endl;
            started_pipeline <= co_await +started_pipeline;
        }
    }
    
    std::cout << "\n=== Pipeline Testing Complete ===\n" << std::endl;
    
    co_return;
}

int main()
{
    io::manager mngr;
    mngr.async_spawn(pipeline_test());

    while (1)
    {
        mngr.drive();
    }

    return 0;
} 