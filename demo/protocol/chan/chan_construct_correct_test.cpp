#include <cstdio>
#include <iostream>
#include <string>
#include <array>
#include "../../../ioManager/all.h"

io::fsm_func<void> chan_construct_correct_test()
{
    // Class with construction/destruction tracking for testing
    static int constructed = 0;
    static int destroyed = 0;
    static int moved = 0;
    static int copied = 0;
    struct LifetimeTracker {
        int value;

        LifetimeTracker(int val = 0) : value(val) {
            constructed++;
            std::cout << "LifetimeTracker constructed: " << value << " (total: " << constructed << ")" << std::endl;
        }

        LifetimeTracker(const LifetimeTracker& other) : value(other.value) {
            copied++;
            std::cout << "LifetimeTracker copied: " << value << " (total copies: " << copied << ")" << std::endl;
        }

        LifetimeTracker(LifetimeTracker&& other) noexcept : value(other.value) {
            moved++;
            other.value = -1; // Mark as moved
            std::cout << "LifetimeTracker moved: " << value << " (total moves: " << moved << ")" << std::endl;
        }

        LifetimeTracker& operator=(const LifetimeTracker& other) {
            if (this != &other) {
                value = other.value;
                copied++;
                std::cout << "LifetimeTracker copy assigned: " << value << " (total copies: " << copied << ")" << std::endl;
            }
            return *this;
        }

        LifetimeTracker& operator=(LifetimeTracker&& other) noexcept {
            if (this != &other) {
                value = other.value;
                other.value = -1; // Mark as moved
                moved++;
                std::cout << "LifetimeTracker move assigned: " << value << " (total moves: " << moved << ")" << std::endl;
            }
            return *this;
        }

        ~LifetimeTracker() {
            destroyed++;
            std::cout << "LifetimeTracker destroyed: " << value << " (total: " << destroyed << ")" << std::endl;
        }

        static void resetCounters() {
            constructed = 0;
            destroyed = 0;
            moved = 0;
            copied = 0;
        }

        static void printStats() {
            std::cout << "\n--- LifetimeTracker Stats ---" << std::endl;
            std::cout << "Constructed: " << constructed << std::endl;
            std::cout << "Destroyed: " << destroyed << std::endl;
            std::cout << "Moved: " << moved << std::endl;
            std::cout << "Copied: " << copied << std::endl;
            std::cout << "Balance (constructed - destroyed): " << (constructed - destroyed) << std::endl;
            std::cout << "-------------------------\n" << std::endl;
        }
    };

    io::fsm<void>& fsm = co_await io::get_fsm;
    std::cout << "\n=== Starting chan construction/destruction tests ===\n" << std::endl;

    // Test 1: Basic construction and destruction
    std::cout << "Test 1: Basic construction and destruction" << std::endl;
    {
        LifetimeTracker::resetCounters();
        std::cout << "Creating chan with 5 capacity..." << std::endl;
        io::chan<LifetimeTracker> chan(fsm, 5);

        std::cout << "Creating and sending 3 elements..." << std::endl;
        std::array<LifetimeTracker, 3> items = { LifetimeTracker(1), LifetimeTracker(2), LifetimeTracker(3) };
        co_await(chan << std::span<LifetimeTracker>(items));

        std::cout << "Chan before destruction:" << std::endl;
        std::cout << "  Size: " << chan.size() << std::endl;
        std::cout << "  Capacity: " << chan.capacity() << std::endl;
        std::cout << "Channel will be destroyed now..." << std::endl;
    }
    std::cout << "After chan destruction:" << std::endl;
    LifetimeTracker::printStats();

    // Test 2: Testing copy and move construction of chan
    std::cout << "Test 2: Copy and move construction of chan" << std::endl;
    {
        LifetimeTracker::resetCounters();

        std::cout << "Creating original chan..." << std::endl;
        io::chan<LifetimeTracker> original_chan(fsm, 5);

        // Add some elements to the channel
        std::array<LifetimeTracker, 2> items = { LifetimeTracker(10), LifetimeTracker(20) };
        co_await(original_chan << std::span<LifetimeTracker>(items));

        std::cout << "Creating copy constructed chan..." << std::endl;
        io::chan<LifetimeTracker> copy_chan(original_chan);

        std::cout << "Creating move constructed chan..." << std::endl;
        io::chan<LifetimeTracker> move_chan(std::move(copy_chan));

        std::cout << "Stats after construction:" << std::endl;
        std::cout << "  Original size: " << original_chan.size() << std::endl;
        std::cout << "  Moved size: " << move_chan.size() << std::endl;

        // Receive from move_chan to verify data was preserved
        std::array<LifetimeTracker, 2> received;
        co_await move_chan.get_and_copy(std::span<LifetimeTracker>(received));

        std::cout << "Received values from moved chan: ";
        for (const auto& item : received) {
            std::cout << item.value << " ";
        }
        std::cout << std::endl;
    }
    LifetimeTracker::printStats();

    // Test 3: Testing chan_r and chan_s conversion
    std::cout << "Test 3: Testing chan_r and chan_s conversion" << std::endl;
    {
        LifetimeTracker::resetCounters();

        std::cout << "Creating original chan..." << std::endl;
        io::chan<LifetimeTracker> original_chan(fsm, 5);

        // Test conversion to chan_r
        std::cout << "Converting to chan_r..." << std::endl;
        io::chan_r<LifetimeTracker> read_chan = original_chan;

        // Test conversion to chan_s
        std::cout << "Converting to chan_s..." << std::endl;
        io::chan_s<LifetimeTracker> write_chan = original_chan;

        // Add some elements through write_chan
        std::array<LifetimeTracker, 2> items = { LifetimeTracker(30), LifetimeTracker(40) };
        co_await(write_chan << std::span<LifetimeTracker>(items));

        // Read them through read_chan
        std::array<LifetimeTracker, 2> received;
        co_await read_chan.get_and_copy(std::span<LifetimeTracker>(received));

        std::cout << "Received values from chan_r: ";
        for (const auto& item : received) {
            std::cout << item.value << " ";
        }
        std::cout << std::endl;
    }
    LifetimeTracker::printStats();

    // Test 4: Testing ring buffer behavior
    std::cout << "Test 4: Testing ring buffer behavior" << std::endl;
    {
        LifetimeTracker::resetCounters();

        std::cout << "Creating chan with capacity 4..." << std::endl;
        io::chan<LifetimeTracker> chan(fsm, 4);

        // First, fill the buffer
        std::cout << "Filling buffer with 4 elements..." << std::endl;
        std::array<LifetimeTracker, 4> items1 = {
            LifetimeTracker(100), LifetimeTracker(101),
            LifetimeTracker(102), LifetimeTracker(103)
        };
        co_await(chan << std::span<LifetimeTracker>(items1));

        // Read 2 elements
        std::cout << "Reading 2 elements..." << std::endl;
        std::array<LifetimeTracker, 2> received1;
        co_await chan.get_and_copy(std::span<LifetimeTracker>(received1));

        // Add 2 more elements (should wrap around)
        std::cout << "Adding 2 more elements (should wrap around)..." << std::endl;
        std::array<LifetimeTracker, 2> items2 = { LifetimeTracker(104), LifetimeTracker(105) };
        co_await(chan << std::span<LifetimeTracker>(items2));

        // Read all remaining elements
        std::cout << "Reading all remaining elements..." << std::endl;
        std::array<LifetimeTracker, 4> received2;
        co_await chan.get_and_copy(std::span<LifetimeTracker>(received2));

        std::cout << "Read values from first batch: ";
        for (const auto& item : received1) {
            std::cout << item.value << " ";
        }
        std::cout << std::endl;

        std::cout << "Read values from second batch: ";
        for (const auto& item : received2) {
            std::cout << item.value << " ";
        }
        std::cout << std::endl;
    }
    LifetimeTracker::printStats();

    // Test 5: Testing close() behavior
    std::cout << "Test 5: Testing close() behavior" << std::endl;
    {
        LifetimeTracker::resetCounters();

        std::cout << "Creating chan..." << std::endl;
        io::chan<LifetimeTracker> chan(fsm, 3);

        // Add some elements
        std::cout << "Adding elements..." << std::endl;
        std::array<LifetimeTracker, 2> items = { LifetimeTracker(200), LifetimeTracker(201) };
        co_await(chan << std::span<LifetimeTracker>(items));

        std::cout << "Closing channel..." << std::endl;
        chan.close();

        std::cout << "Channel closed, checking counters..." << std::endl;
    }
    LifetimeTracker::printStats();

    std::cout << "\n=== chan construction/destruction tests completed ===\n" << std::endl;
    co_return;
}

int main()
{
    io::manager mngr;
    mngr.async_spawn(chan_construct_correct_test());

    while (1)
    {
        mngr.drive();
    }

    return 0;
} 