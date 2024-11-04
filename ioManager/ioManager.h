/* driver library for cross-platform IoT dev.
 * ------Head-Only------, but may not, depends on the driver your need.
 * 
 * for unifying various interfaces and their multiplexing provided by controllers, systems even computers.
 * 
 * ---EXPERIMENTAL LIBRARY---
 * 
 * Considering: use LVGL for display support
 * 
 * Licensed under the MIT License.
 * Looking forward to visiting https://github.com/UF4007/ to propose issues, pull your device driver, and make ioManager stronger and more universal.
*/
#pragma once
#include "internal/config.h"
#include "internal/includes.h"
namespace io
{
    //headonly version distinguish, prevent the linker from mixing differental versions when multi-reference.
    inline namespace v247a {

#include "internal/forwardDeclarations.h"

        // -----------------------------------basis---------------------------------

        //error type of this lib
        enum class err : uint8_t {
            ok = 0,
            failed = 1,
            repeat = 2,         //try again please
            notfound = 3,
            less = 4,           //more data are required
            more = 5,           //too much data was received
            bufoverf = 6,       //buffer overflow
            formaterr = 7,      //format error
        };

        using duration_ms = std::chrono::duration<unsigned long long, std::milli>;

        template <size_t _capacity>
        struct byteBuffer {
            char data[_capacity];
            size_t depleted = 0;
            bool overflow = false;
            static constexpr size_t capacity = _capacity;
        };

        template <typename _Struc>
        class dualBuffer {                        //dual buffer, multi-thread outbound, multi-thread inbound definitely safety. Reentrancy correct.
            _Struc buffer[2];
            std::atomic_flag bufferLock[2] = { ATOMIC_FLAG_INIT, ATOMIC_FLAG_INIT };
            std::atomic<bool> rotateLock{ false };

            dualBuffer(const dualBuffer&) = delete;
            const dualBuffer& operator=(const dualBuffer&) = delete;

        public:
            inline dualBuffer() {};
            [[nodiscard]] inline _Struc* outbound_get()     //if outbound gets nullptr don't repeat otherwise deadlock.
            {
                bool expected = true;
                if (rotateLock.compare_exchange_strong(expected, true, std::memory_order_acquire, std::memory_order_relaxed))
                {
                    if (!bufferLock[0].test_and_set(std::memory_order_acquire))
                    {
                        return &buffer[0];
                    }
                }
                else
                {
                    if (!bufferLock[1].test_and_set(std::memory_order_acquire))
                    {
                        return &buffer[1];
                    }
                }
                return nullptr;
            }
            inline void outbound_unlock(_Struc* unlock)
            {
                if (unlock == &buffer[0])
                {
                    bufferLock[0].clear(std::memory_order_release);
                }
                else if (unlock == &buffer[1])
                {
                    bufferLock[1].clear(std::memory_order_release);
                }
                else
                    return;
            }
            inline void outbound_rotate()
            {
                bool expected = true;
                if (rotateLock.compare_exchange_strong(expected, false) == false)
                    rotateLock.store(true, std::memory_order_relaxed);
            }

            [[nodiscard]] inline _Struc* inbound_get()    //if inbound gets nullptr, repeat then (spinning lock) or give in then (reentrancy)
            {
                bool expected = false;
                if (rotateLock.compare_exchange_strong(expected, false, std::memory_order_acquire, std::memory_order_relaxed))
                {
                    if (!bufferLock[0].test_and_set(std::memory_order_acquire))
                        return &buffer[0];
                }
                else
                {
                    if (!bufferLock[1].test_and_set(std::memory_order_acquire))
                        return &buffer[1];
                }
                return nullptr;
            }
            inline void inbound_unlock(_Struc* unlock)
            {
                if (unlock == &buffer[0])
                {
                    bufferLock[0].clear(std::memory_order_release);
                }
                else if (unlock == &buffer[1])
                {
                    bufferLock[1].clear(std::memory_order_release);
                }
            }
        };






        // ----------------------------------coroutine---------------------------------

        enum class coStatus :uint8_t {
            null = 0,
            idle = 1,
            timing = 2,
            ready = 3,
            completed = 4,
            timeout = 5
        };

#include "internal/lowlevel.h"

        struct coPara {
            ioManager* mngr;
        };

        struct coTask
        {
            __IO_INTERNAL_HEADER_PERMISSION
            struct promise_type {
                inline coTask get_return_object() { return coTask{}; }

                inline std::suspend_never initial_suspend() { return {}; }
                inline std::suspend_never final_suspend() noexcept { return {}; }

                std::suspend_never yield_value() = delete;

                inline void return_void() {}
                inline void unhandled_exception() { std::terminate(); }
            };

            using processPtr = coTask(*)(coPara);
        };

        enum class co_return_v {
            nothing,
            complete,
            abort
        };

        template <typename _T = void>
        class coPromise
        {
            __IO_INTERNAL_HEADER_PERMISSION
            void cdd();
            lowlevel::awaiter* _base = nullptr;
        public:
            struct promise_type {
                coPromise<_T> prom;
                template <typename ...Args, typename = std::enable_if_t<std::is_constructible<_T, Args...>::value>>
                promise_type(ioManager* m, Args&&... consArgs); // param of coroutine can construct the promise object
                template <typename ...Args2, typename = std::enable_if_t<!std::is_constructible<_T, Args2...>::value && std::is_default_constructible<_T>::value>>
                promise_type(ioManager* m, Args2... consArgs);  // param of coroutine can't construct the promise object, the promise object uses default construct func
                inline coPromise<_T> get_return_object() { return prom; }

                inline std::suspend_never initial_suspend() { return {}; }
                inline std::suspend_never final_suspend() noexcept { return {}; }

                inline std::suspend_never yield_value(coPromise<_T>& returnvl) { returnvl = prom; return {}; }

                inline void return_value(co_return_v ret = co_return_v::nothing) {
                    switch (ret)
                    {
                    case co_return_v::complete:
                        prom.complete();
                        break;
                    case co_return_v::abort:
                        prom.abort();
                        break;
                    }
                }
                inline void unhandled_exception() { std::terminate(); }
            };
            struct awaiterIntermediate
            {
                lowlevel::awaiter* _a;
                inline awaiterIntermediate(lowlevel::awaiter* awt) { _a = awt; }
                inline bool await_ready() const noexcept { return false; }
                inline void await_suspend(std::coroutine_handle<> h)
                {
                    _a->coro = h;
                }
                inline bool await_resume() noexcept
                {
                    _a->coro = nullptr;
                    return true;
                }
            };

            coPromise() = default;
            template <typename ...Args>
            coPromise(ioManager* m, Args&&... consArgs);
            coPromise(const coPromise<_T>& right);
            void operator=(const coPromise<_T>& right);
            operator bool();
            inline bool operator==(void* opr) { return _base == opr; };
            ~coPromise();           //asynchronously safe

            template <typename _Duration>
            io::err setTimeout(_Duration time);           //asynchronously safe

            inline bool countCheck() {                       //if true, this count == 1, which means only this coroutine owns the promise.
                uint32_t expected = 1;
                return !_base->count.compare_exchange_strong(expected, 1);
            }
            inline std::atomic_flag* getLock() { return &_base->lock; }
            inline lowlevel::awaiter* getAwaiter() { return _base; };
            _T* getPointer();

            bool completable();     //asynchronously safe
            io::err abort();        //asynchronously safe
            io::err complete();     //asynchronously safe

            inline bool isCompleted() { return !_base->aborted && _base->status == coStatus::completed; }
            inline bool isTimeout() { return !_base->aborted && _base->status == coStatus::timeout; }
            inline bool isAborted() { return _base->aborted; }

            bool reset();       //return value meaningless.
        };

#define task_await(___cofuture___) ((___cofuture___.getLock()->test_and_set(std::memory_order_acquire) == false) ? (co_await io::coPromise<>::awaiterIntermediate(___cofuture___.getAwaiter())) : true) ?  ___cofuture___.getPointer() : nullptr\

        // manager of coroutine tasks
        //  a ioManager == a thread
        class ioManager final
        {
            __IO_INTERNAL_HEADER_PERMISSION

            lowlevel::awaiter::awaiterListNode timeAwaiterCentral;
            std::atomic_flag spinLock_tm = ATOMIC_FLAG_INIT;    //time based awaiter list

            std::queue<coTask::processPtr> pendingTask;
            std::atomic_flag spinLock_pd = ATOMIC_FLAG_INIT;    //pending task

            lowlevel::awaiter* readyAwaiter = nullptr;
            std::atomic_flag spinLock_rd = ATOMIC_FLAG_INIT;    //pending to continue (includes completed or aborted awaiter)

            std::atomic_flag isEnd = ATOMIC_FLAG_INIT;
            std::atomic<bool> going = false;

            std::chrono::nanoseconds suspend_max = std::chrono::nanoseconds(1000000);   //defalut 1ms
            std::binary_semaphore suspend_sem = std::binary_semaphore(1);
        public:
            ioManager();
            ioManager(ioManager&) = delete;
            ioManager(ioManager&&) = delete;
            void operator=(ioManager& right) = delete;
            void operator=(ioManager&& right) = delete;
            ~ioManager();
            void once(coTask::processPtr ptr);
            void drive();
            void go();
            void stop(bool sync = false);
            inline bool isGoing() { return going; }

            //global automatic thread pool of ioManager
        private:
            static std::deque<ioManager> all;
            inline static std::atomic<int> getIndex = 0;
            inline static bool auto_going = false;
            static int getPendingFromAll();

        public:
            inline static bool isAutoGoing() { return auto_going; }
            static void auto_go(uint32_t threadSum = 1);
            static void auto_stop();
            static void auto_once(coTask::processPtr ptr);
        };
        inline std::deque<ioManager> ioManager::all = {};






        // ----------------------------------io multiplexing---------------------------------

        using ISR = void(*)();                                                                      //must return in a transient, and reentrancy correct.

        struct socketData : byteBuffer<IO_SOCKET_BUFFER_SIZE> {};

        template <typename _T>
        struct ioChannel {
            coPromise<_T> promise;
            dualBuffer<_T> buffer;
        };

        struct socketDataUdp : socketData {
            sockaddr_in addr;
        };

        //volunteer drive
        class volunteerDriver {
            __IO_INTERNAL_HEADER_PERMISSION
#if IO_USE_SELECT
            inline static std::map<uint64_t, coPromise<socketData>> select_rbt;
            inline static std::atomic_flag select_rbt_busy = ATOMIC_FLAG_INIT;              //tcp client (connect oriented)
            inline static std::map<uint64_t, coPromise<tcp_client_socket>> select_rbt_server;
            inline static std::atomic_flag select_rbt_server_busy = ATOMIC_FLAG_INIT;       //tcp server (accept)
            inline static std::map<uint64_t, coPromise<socketDataUdp>> select_rbt_udp;
            inline static std::atomic_flag select_rbt_udp_busy = ATOMIC_FLAG_INIT;          //udp, raw socket
#elif IO_USE_IOCP
#elif IO_USE_EPOLL
#else
            static_assert(false, "io manager compile ERROR: cannot find io multiplexing strategy.")
#endif
            inline static std::atomic<uint32_t> socket_count = 0;
            inline static std::atomic_flag socket_enable = ATOMIC_FLAG_INIT;

            static void go();

            static void selectDrive();
            static void iocpDrive();
            static void epollDrive();
        };





        // -------------------------------hardware--------------------------------

        namespace interrupt
        {
            io::err static all_open();
            io::err static all_close();
        };

        namespace gpio
        {
            using num = int;
            enum class mode {
                disable,
                functional,
                interrupter,
                iopin
            };
            enum class dir {
                input,
                output,
                both
            };
            enum class pull {   //transistor pull
                pullup,
                pulldown,
                pushpull,
                floating
            };
            enum class resistor {
                up,
                down,
                both,
                none
            };
            static io::err setmode(num, mode);
            static io::err setmode(num, dir);
            static io::err setmode(num, pull);
            static io::err setmode(num, resistor);
            static io::err get_level(num, bool&);
            static io::err set_level(num, bool);
            static io::err open_interrupt();
            static io::err close_interrupt();
        };

        namespace timer
        {
            using num = int;
            static err setmode();
            static err set_counter();
            static err open();
            static err start();
            static err reset();
            static err stop();
            static err close();
        };

        namespace watchDog
        {
            static err set(bool open, std::chrono::microseconds us);
            static err feed();
        };

        namespace pwm
        {
            using num = int;
            static err open();
            static err set_duty();
            static err set_freq();
            static err close();
            static int duty_max();
        };

        class dma {};

        class adc{};

        class dac{};

        class uart{};

        class i2c{};

        class spi{};

        class can{};

        class camera{};

        class usb{};





        // -------------------------------network io--------------------------------

        // abstract class, system network interface.
        //  ESP32:    esp_netif_t*
        //  Linux:    ifaddrs*
        //  Win:      PIP_ADAPTER_ADDRESSES
        class netif
        {
            __IO_INTERNAL_HEADER_PERMISSION
            inline static std::atomic_flag _is_init_netif = ATOMIC_FLAG_INIT;
#if defined(ESP_PLATFORM)
#elif defined(_WIN32)
            struct windows_netif_handle;
            std::shared_ptr<windows_netif_handle> _handle = nullptr;
#endif
        public:
            static void global_startup();
            static void global_cleanup();
            netif();
            ~netif();
            err reload();
            err next();
        };

        //encryption
        namespace encrypt
        {
            //referenced from: https://github.com/JieweiWei/md5
            //under Apache-2.0 license
            //calibrated in: https://www.sojson.com/encrypt_md5.html
            //2024.10.18
            class md5 {
            public:
                md5(const char* byte, size_t size);
                md5(const std::string& message);
                std::span<const std::byte, 16> getDigest();
                std::string toStr();
            private:
                void init(const std::byte* input, size_t len);
                void transform(const std::byte block[64]);
                void encode(const uint32_t* input, std::byte* output, size_t length);
                void decode(const std::byte* input, uint32_t* output, size_t length);
                static uint32_t F(uint32_t, uint32_t, uint32_t);
                static uint32_t G(uint32_t, uint32_t, uint32_t);
                static uint32_t H(uint32_t, uint32_t, uint32_t);
                static uint32_t I(uint32_t, uint32_t, uint32_t);
                static uint32_t ROTATELEFT(uint32_t num, uint32_t n);
                static void FF(uint32_t& a, uint32_t b, uint32_t c, uint32_t d, uint32_t x, uint32_t s, uint32_t ac);
                static void GG(uint32_t& a, uint32_t b, uint32_t c, uint32_t d, uint32_t x, uint32_t s, uint32_t ac);
                static void HH(uint32_t& a, uint32_t b, uint32_t c, uint32_t d, uint32_t x, uint32_t s, uint32_t ac);
                static void II(uint32_t& a, uint32_t b, uint32_t c, uint32_t d, uint32_t x, uint32_t s, uint32_t ac);
                static constexpr uint32_t s11 = 7;
                static constexpr uint32_t s12 = 12;
                static constexpr uint32_t s13 = 17;
                static constexpr uint32_t s14 = 22;
                static constexpr uint32_t s21 = 5;
                static constexpr uint32_t s22 = 9;
                static constexpr uint32_t s23 = 14;
                static constexpr uint32_t s24 = 20;
                static constexpr uint32_t s31 = 4;
                static constexpr uint32_t s32 = 11;
                static constexpr uint32_t s33 = 16;
                static constexpr uint32_t s34 = 23;
                static constexpr uint32_t s41 = 6;
                static constexpr uint32_t s42 = 10;
                static constexpr uint32_t s43 = 15;
                static constexpr uint32_t s44 = 21;

                bool finished;
                uint32_t state[4];
                uint32_t count[2];
                std::byte buffer[64];
                std::byte digest[16];
                inline static const std::byte PADDING[64] = { (std::byte)0x80 };
                inline static const char HEX_NUMBERS[16] = {
  '0', '1', '2', '3',
  '4', '5', '6', '7',
  '8', '9', 'a', 'b',
  'c', 'd', 'e', 'f'
                };
            };
            //referenced from: https://github.com/kokke/tiny-AES-c
            //under license: free
            //calibrated in: https://tool.hiofd.com/aes-encrypt-online/
            //2024.10.18
            class aes {
            public:
                static constexpr int block_len = 16;
                using key128 = std::span<uint8_t, 16>;
                using key192 = std::span<uint8_t, 24>;
                using key256 = std::span<uint8_t, 32>;
                using ivector = std::span<uint8_t, block_len>;
                using ivector_m = uint8_t[block_len];

                enum { AES128, AES192, AES256 } type = AES256;
                uint8_t key[32];

                inline aes() {};
                aes(key128);
                aes(key192);
                aes(key256);
                void set_iv(ivector);
                ivector get_iv();
                void rand_key();
                void rand_iv();
                void ECB_encrypt(std::span<uint8_t, 16>);
                void ECB_decrypt(std::span<uint8_t, 16>);
                void CBC_encrypt(std::span<uint8_t>);
                void CBC_decrypt(std::span<uint8_t>);
                void CTR_xcrypt(std::span<uint8_t>);
            private:
                uint8_t RoundKey[240];
                uint8_t Iv[block_len];
                typedef uint8_t state_t[4][4];
                uint32_t getNk();
                uint32_t getNr();
                uint8_t getSBoxInvert(uint8_t num);
                uint8_t getSBoxValue(uint8_t num);
                void KeyExpansion(uint8_t* RoundKey, const uint8_t* Key);
                void AddRoundKey(uint8_t round, state_t* state, const uint8_t* RoundKey);
                void SubBytes(state_t* state);
                void ShiftRows(state_t* state);
                uint8_t xtime(uint8_t x);
                void MixColumns(state_t* state);
                uint8_t Multiply(uint8_t x, uint8_t y);
                void InvMixColumns(state_t* state);
                void InvSubBytes(state_t* state);
                void InvShiftRows(state_t* state);
                void Cipher(state_t* state, const uint8_t* RoundKey);
                void InvCipher(state_t* state, const uint8_t* RoundKey);
                void XorWithIv(uint8_t* buf, const uint8_t* Iv);
                static constexpr uint32_t Nb = 4;
                static constexpr uint32_t Nk128 = 4;
                static constexpr uint32_t Nr128 = 10;
                static constexpr uint32_t Nk192 = 6;
                static constexpr uint32_t Nr192 = 12;
                static constexpr uint32_t Nk256 = 8;
                static constexpr uint32_t Nr256 = 14;
                inline static const uint8_t sbox[256] = {
                    //0     1    2      3     4    5     6     7      8    9     A      B    C     D     E     F
                    0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
                    0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
                    0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
                    0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
                    0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
                    0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
                    0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
                    0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
                    0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
                    0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
                    0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
                    0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
                    0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
                    0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
                    0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
                    0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16 };
                inline static const uint8_t rsbox[256] = {
                  0x52, 0x09, 0x6a, 0xd5, 0x30, 0x36, 0xa5, 0x38, 0xbf, 0x40, 0xa3, 0x9e, 0x81, 0xf3, 0xd7, 0xfb,
                  0x7c, 0xe3, 0x39, 0x82, 0x9b, 0x2f, 0xff, 0x87, 0x34, 0x8e, 0x43, 0x44, 0xc4, 0xde, 0xe9, 0xcb,
                  0x54, 0x7b, 0x94, 0x32, 0xa6, 0xc2, 0x23, 0x3d, 0xee, 0x4c, 0x95, 0x0b, 0x42, 0xfa, 0xc3, 0x4e,
                  0x08, 0x2e, 0xa1, 0x66, 0x28, 0xd9, 0x24, 0xb2, 0x76, 0x5b, 0xa2, 0x49, 0x6d, 0x8b, 0xd1, 0x25,
                  0x72, 0xf8, 0xf6, 0x64, 0x86, 0x68, 0x98, 0x16, 0xd4, 0xa4, 0x5c, 0xcc, 0x5d, 0x65, 0xb6, 0x92,
                  0x6c, 0x70, 0x48, 0x50, 0xfd, 0xed, 0xb9, 0xda, 0x5e, 0x15, 0x46, 0x57, 0xa7, 0x8d, 0x9d, 0x84,
                  0x90, 0xd8, 0xab, 0x00, 0x8c, 0xbc, 0xd3, 0x0a, 0xf7, 0xe4, 0x58, 0x05, 0xb8, 0xb3, 0x45, 0x06,
                  0xd0, 0x2c, 0x1e, 0x8f, 0xca, 0x3f, 0x0f, 0x02, 0xc1, 0xaf, 0xbd, 0x03, 0x01, 0x13, 0x8a, 0x6b,
                  0x3a, 0x91, 0x11, 0x41, 0x4f, 0x67, 0xdc, 0xea, 0x97, 0xf2, 0xcf, 0xce, 0xf0, 0xb4, 0xe6, 0x73,
                  0x96, 0xac, 0x74, 0x22, 0xe7, 0xad, 0x35, 0x85, 0xe2, 0xf9, 0x37, 0xe8, 0x1c, 0x75, 0xdf, 0x6e,
                  0x47, 0xf1, 0x1a, 0x71, 0x1d, 0x29, 0xc5, 0x89, 0x6f, 0xb7, 0x62, 0x0e, 0xaa, 0x18, 0xbe, 0x1b,
                  0xfc, 0x56, 0x3e, 0x4b, 0xc6, 0xd2, 0x79, 0x20, 0x9a, 0xdb, 0xc0, 0xfe, 0x78, 0xcd, 0x5a, 0xf4,
                  0x1f, 0xdd, 0xa8, 0x33, 0x88, 0x07, 0xc7, 0x31, 0xb1, 0x12, 0x10, 0x59, 0x27, 0x80, 0xec, 0x5f,
                  0x60, 0x51, 0x7f, 0xa9, 0x19, 0xb5, 0x4a, 0x0d, 0x2d, 0xe5, 0x7a, 0x9f, 0x93, 0xc9, 0x9c, 0xef,
                  0xa0, 0xe0, 0x3b, 0x4d, 0xae, 0x2a, 0xf5, 0xb0, 0xc8, 0xeb, 0xbb, 0x3c, 0x83, 0x53, 0x99, 0x61,
                  0x17, 0x2b, 0x04, 0x7e, 0xba, 0x77, 0xd6, 0x26, 0xe1, 0x69, 0x14, 0x63, 0x55, 0x21, 0x0c, 0x7d };
                inline static const uint8_t Rcon[11] = {
  0x8d, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36 };
            };
            //referenced from: https://github.com/shaojunhan/RSA
            //under license: free
            //calibrated in: https://www.lddgo.net/encrypt/rsa
            // "rightShift" function error was corrected.
            // temporary object optimized.                  2048bit spawn cost:10min -> 6min
            // small prime number pre-selection added.      2048bit spawn cost:6min -> 1m30s
            // montgomery modular mul/exp added.            2048bit spawn cost:1m30s -> incorrect...
            //2024.10.20
            class rsa {
            public:
                struct mon_domain;
#include "internal/BigInt.h"
                void generate(unsigned int n);                      //initialize, generate a public key and private key.

                BigInt encryptByPu(const BigInt& m);
                BigInt decodeByPuPr(const BigInt& c);

                BigInt encryptByPuPr(const BigInt& m);
                BigInt decodeByPu(const BigInt& m);

                struct mon_domain {
                    const BigInt& mod;
                    BigInt r, k;
                    uint64_t Rbits;
                    //BigInt R;
                    inline mon_domain(const BigInt& N) :mod(N) {}
                    inline void respawn() {
                        BigInt::temp_warpper R(1);
                        BigInt::bit b(mod);
                        R = R << b.size();
                        this->r = R->moden(2, mod);

                        this->k = mod.extendEuclid(R);
                        this->k = R - this->k;
                        this->Rbits = b.size();
                        //this->R = R;
                    }
                };

                void keyInit();
                BigInt& public_key = N;             //public key
                BigInt& private_key = _d;           //private key
                BigInt& euler_num = e;
            private:
                BigInt createOddNum(unsigned int n, std::random_device& rd);                //spawn a (n) size odd number
                bool isPrime(const BigInt& a, const unsigned int k, BigInt& buffer);        //prime number judgment
                BigInt createPrime(unsigned int n, int it_cout, std::random_device& rd);    //spawn a (n) size prime number
                void createExp(const BigInt& ou);                                           //spawn public and private exp from Euler number
                BigInt createRandomSmallThan(const BigInt& a);                              //spawn a smaller number

                BigInt e = 65537, N;    //public key
                BigInt _d;              //private key
                mon_domain mond = N;    //mon domain, including public key(mod) N
            };
            //referenced from: https://github.com/orlp/ed25519?tab=Zlib-1-ov-file
            //under license: Zlib
            //pending
            class ed_25519 {};
        };

        namespace bluetooth
        {
            using num = int64_t;
        }

        namespace wifi
        {
            using num = int64_t;
        }

        class lte{};

        class ethernet{};

#include "protocol/icmp.h"
        class icmp_client_socket {
            __IO_INTERNAL_HEADER_PERMISSION
#if defined(_WIN32)
                SOCKET handle = INVALID_SOCKET;
#elif defined(__linux__)
                int handle = 0;
#endif
            std::chrono::steady_clock::time_point _tp;
        public:
            icmp_client_socket();
            icmp_client_socket(const icmp_client_socket&) = delete;
            void operator=(const icmp_client_socket&) = delete;
            icmp_client_socket(icmp_client_socket&& right);
            void operator=(icmp_client_socket&& right);
            ~icmp_client_socket();
            io::err sendPing(coPromise<socketDataUdp>& promise, const sockaddr_in& addr, std::chrono::microseconds overtime, uint16_t id = 0xff);
            std::chrono::nanoseconds getDelay();
        };

        class icmp_server_socket {};

        class tcp_client_socket {
            __IO_INTERNAL_HEADER_PERMISSION
#if defined(_WIN32)
            SOCKET handle = INVALID_SOCKET;
#elif defined(__linux__)
            int handle = -1;
#endif
        public:
            tcp_client_socket();
            tcp_client_socket(const tcp_client_socket&) = delete;
            void operator=(const tcp_client_socket&) = delete;
            tcp_client_socket(tcp_client_socket&& right);
            void operator=(tcp_client_socket&& right);
            ~tcp_client_socket();
            coPromise<socketData> findPromise();
            io::err open();
            io::err connect(coPromise<socketData>& promise, const sockaddr_in& addr);
            void connect(coPromise<socketData>& promise, const sockaddr_in& addr, std::chrono::microseconds overtime, const netif& net);
            void close();
            io::err send(socketData& data);
            io::err send(const char* c, size_t s);
        };

        class tcp_server_socket {
            __IO_INTERNAL_HEADER_PERMISSION
#if defined(_WIN32)
                SOCKET handle = INVALID_SOCKET;
#elif defined(__linux__)
                int handle = -1;
#endif
        public:
            tcp_server_socket();
            tcp_server_socket(const tcp_server_socket&) = delete;
            void operator=(const tcp_server_socket&) = delete;
            tcp_server_socket(tcp_server_socket&& right);
            void operator=(tcp_server_socket&& right);
            ~tcp_server_socket();
            io::err open();
            io::err bind(coPromise<tcp_client_socket>& promise, const sockaddr_in& addr);
            void close();
        };

        class udp_socket {
            __IO_INTERNAL_HEADER_PERMISSION
#if defined(_WIN32)
                SOCKET handle = INVALID_SOCKET;
#elif defined(__linux__)
                int handle = -1;
#endif
        public:
            udp_socket();
            udp_socket(const udp_socket&) = delete;
            void operator=(const udp_socket&) = delete;
            udp_socket(udp_socket&& right);
            void operator=(udp_socket&& right);
            ~udp_socket();
            io::err open();
            io::err bind(coPromise<socketDataUdp>& promise, const sockaddr_in& addr);
            void close();
            io::err sendto(const char* c, size_t s, const sockaddr_in& addr);
            io::err sendtoAndBind(const char* c, size_t s, const sockaddr_in& addr, coPromise<socketDataUdp>& promise);
        };

        class kcp_client {};

        class kcp_server {};

#include "protocol/sntp.h"
        class sntp_client
        {
        };

#include "protocol/dns.h"
        class dns_client {
            __IO_INTERNAL_HEADER_PERMISSION
            io::coPromise<io::socketDataUdp> fu;
        public:
            udp_socket socket;
            inline dns_client(coPara para) {
                fu = io::coPromise<io::socketDataUdp>(para.mngr);
            }
            inline io::coTask query(coPromise<sockaddr_in>& promise, const std::string& domain, const sockaddr_in& dns_server, std::chrono::microseconds overtime) {
                std::string dnsReq = io::dns_data::toString(domain);
                socket.sendtoAndBind(dnsReq.c_str(), dnsReq.length(), dns_server, fu);
                fu.setTimeout(overtime);
                promise.setTimeout(overtime);

                io::socketDataUdp* data = task_await(fu);

                if (fu.isCompleted())
                {
                    io::dns_data mes;
                    mes.fromChar(data->data, data->depleted);
                    mes.getIp(*promise.getPointer());
                    data->depleted = 0;
                    promise.complete();
                }
                else if (fu.isAborted())
                {
                    promise.abort();
                }
                fu.reset();
                co_return;
            }
        };

        class dns_server {};

#include "protocol/http.h"
        class http_client {
            __IO_INTERNAL_HEADER_PERMISSION
            io::coPromise<io::socketData> fu;
        public:
            tcp_client_socket tcp;
            dns_client dns;
            sockaddr_in dns_addr;
            std::chrono::microseconds overtime;
            inline http_client(coPara para):dns(para) {
                fu = io::coPromise<io::socketData>(para.mngr);
                while (dns.socket.open() == io::err::failed);
                while (tcp.open() == io::err::failed);

                memset(&dns_addr, 0, sizeof(dns_addr));
                dns_addr.sin_family = AF_INET;
                dns_addr.sin_port = htons(53);
                dns_addr.sin_addr.s_addr = inet_addr("8.8.8.8");

                overtime = std::chrono::seconds(10);
            };
            http_client(const http_client&) = delete;
            void operator=(const http_client&) = delete;
            inline ~http_client() {}
            inline static io::coPromise<httpResponce> send(io::ioManager* mngr, http_client* thisp, httpRequest& req, const sockaddr_in& dest_addr) {
                io::coPromise<httpResponce> ret;
                co_yield ret;                   //get returned coroutine promise, coroutine not really yield

                thisp->tcp.connect(thisp->fu, dest_addr);
                thisp->fu.setTimeout(thisp->overtime);

                io::socketData *data = task_await(thisp->fu);

                if (thisp->fu.isCompleted())
                {
                    httpResponce *prom_resp = ret.getPointer();
                    std::string request = req.toString();
                    thisp->tcp.send(request.c_str(), request.length());
                    while (1)
                    {
                        thisp->fu.reset();
                        thisp->fu.setTimeout(thisp->overtime);
                        data = task_await(thisp->fu);
                        if (thisp->fu.isCompleted())
                        {
                            io::err i = prom_resp->fromChar(data->data, data->depleted);
                            data->depleted = 0;
                            if (i == io::err::ok)
                            {
                                thisp->fu.reset();
                                co_return co_return_v::complete;
                            }
                            else if (i == io::err::less)
                            {
                                continue;
                            }
                            else
                                break;
                        }
                        else
                            break;
                    }
                }
                thisp->fu.reset();
                co_return co_return_v::abort;
            }
            inline static io::coPromise<httpResponce> send(io::ioManager* mngr, http_client* thisp, httpRequest& req, const std::string& domain, int port = 80) {
                io::coPromise<httpResponce> ret;
                co_yield ret;                   //get returned coroutine promise, coroutine not really yield

                io::coPromise<sockaddr_in> dns_prom(mngr);
                thisp->dns.query(dns_prom, domain, thisp->dns_addr, thisp->overtime);
                sockaddr_in* paddr = task_await(dns_prom);

                if (dns_prom.isCompleted())
                {
                    paddr->sin_port = htons(port);
                    thisp->tcp.connect(thisp->fu, *paddr);
                    thisp->fu.setTimeout(thisp->overtime);

                    io::socketData* data = task_await(thisp->fu);

                    if (thisp->fu.isCompleted())
                    {
                        httpResponce* prom_resp = ret.getPointer();
                        std::string request = req.toString();
                        thisp->tcp.send(request.c_str(), request.length());
                        while (1)
                        {
                            thisp->fu.reset();
                            thisp->fu.setTimeout(thisp->overtime);
                            data = task_await(thisp->fu);
                            if (thisp->fu.isCompleted())
                            {
                                io::err i = prom_resp->fromChar(data->data, data->depleted);
                                data->depleted = 0;
                                if (i == io::err::ok)
                                {
                                    thisp->fu.reset();
                                    co_return co_return_v::complete;
                                }
                                else if (i == io::err::less)
                                {
                                    continue;
                                }
                                else break;
                            }
                            else break;
                        }
                    }
                    thisp->fu.reset();
                }
                co_return co_return_v::abort;
            }
        };

        //not finished yet.
        class http_server {
            __IO_INTERNAL_HEADER_PERMISSION
            io::coPromise<io::tcp_client_socket> tcpc;
        public:
            tcp_server_socket tcps;
            struct client_connect{
                io::tcp_client_socket sock;
            };
            inline http_server(coPara para){
                while (tcps.open() == io::err::failed);
            }
            http_server(const http_server&) = delete;
            void operator=(const http_server&) = delete;
            inline ~http_server() {}
            inline io::err bind(coPromise<http_server::client_connect>& promise, int port = 80){
                // sockaddr_in server_addr{};
                // server_addr.sin_family = AF_INET;
                // server_addr.sin_addr.s_addr = INADDR_ANY;
                // server_addr.sin_port = htons(port);

                // client_connect* conn = promise.getPointer();
                return io::err::ok;
            }
        };

        class mqtt_client {};

        class mqtt_server {};

#ifdef _WIN32
#include "platform\win.h"       //windows special backslash!
#elif defined(__linux__)
#include "platform/linux.h"
#ifdef _mysql_h
    #include "platform/iomysql.h"        //we must use select to do things un-blocking
#endif
#elif defined(ESP_PLATFORM)
#include "platform/esp32.h"
#endif

#include "internal/definitions.h"
#include "internal/BigIntDefinition.h"

    }
}