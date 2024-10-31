
//
//        struct gpioMode
//        {
//#if defined(ESP_PLATFORM)
//            using pinMode = enum : uint8_t {
//                disable = 0,
//                functional = 1,
//                interrupter = 2,
//                iopin = 3
//            };
//            using pinDir = enum : std::underlying_type_t<gpio_mode_t> {
//                input = GPIO_MODE_DEF_INPUT,
//                output = GPIO_MODE_DEF_OUTPUT,
//                both = ((GPIO_MODE_DEF_INPUT) | (GPIO_MODE_DEF_OUTPUT)),
//            };
//            using pinPull = enum : std::underlying_type_t<gpio_pull_mode_t> {
//                PullUp = GPIO_PULLUP_ONLY,
//                PullDown = GPIO_PULLDOWN_ONLY,
//                PullBoth = GPIO_PULLUP_PULLDOWN,
//                floating = GPIO_FLOATING
//            };
//            using pinRes = enum : uint8_t {
//                ResUp = 1,
//                ResDown = 1 << 1,
//                ResBoth = 1 | 2,
//                ResNone = 0
//            };
//#endif
//        };
//        template <uint32_t _Pin>
//        class gpio
//        {
//            inline static volatile bool used = false;            // gpio is a single instance class, if a pin is used by someone, another one cannot creat it again.
//        public:
//            inline gpio() {
//                if constexpr (_Pin == noUsed)
//                    return;
//                assert((used == false) || !"this pin has created an instance or used it for another purpose.");
//                used = true;
//            }
//            inline ~gpio() {
//                if constexpr (_Pin == noUsed)
//                    return;
//                used = false;
//            }
//            gpio(gpio&) = delete;
//            void operator=(gpio&) = delete;
//            static void* operator new(size_t size) = delete;    // prohibit to allocate gpio on heap solely, restrict for reuse
//            static void operator delete(void* pthis) = delete;
//            inline static bool isUsing() {
//                return used;
//            }
//        private:
//#if defined(ESP_PLATFORM)
//        public:
//            static_assert(_Pin < gpio_num_t::GPIO_NUM_MAX || _Pin == noUsed, "Invaild pin.");
//            using pinMode = gpioMode::pinMode;
//            using pinDir = gpioMode::pinDir;
//            using pinPull = gpioMode::pinPull;
//            using pinRes = gpioMode::pinRes;
//            inline static constexpr bool fulfilled = true;
//            inline err open(pinMode mode, pinDir dir, pinPull pull, pinRes res)
//            {
//                switch (mode)
//                {
//                case pinMode::iopin:
//                {
//                    gpio_config_t io_conf;
//                    io_conf.intr_type = GPIO_INTR_DISABLE;
//                    io_conf.mode = (gpio_mode_t)dir;
//                    io_conf.pin_bit_mask = (1ULL << _Pin);
//                    io_conf.pull_down_en = (gpio_pulldown_t)((uint8_t)res & pinRes::ResDown);
//                    io_conf.pull_up_en = (gpio_pullup_t)((uint8_t)res & pinRes::ResUp);
//                    if (gpio_config(&io_conf))
//                        return failed;
//                    if (gpio_set_pull_mode((gpio_num_t)_Pin, (gpio_pull_mode_t)pull))
//                        return failed;
//                    return ok;
//                }
//                break;
//                case pinMode::disable:
//                {
//                    gpio_config_t io_conf;
//                    io_conf.intr_type = GPIO_INTR_DISABLE;
//                    io_conf.mode = GPIO_MODE_DISABLE;
//                    return (err)gpio_config(&io_conf);
//                }
//                break;
//                }
//            }
//            inline void setLevel(bool level)
//            {
//                gpio_set_level((gpio_num_t)_Pin, level);
//            }
//            inline bool getLevel()
//            {
//                return gpio_get_level((gpio_num_t)_Pin);
//            }
//            inline err close()
//            {
//                gpio_config_t io_conf;
//                io_conf.intr_type = GPIO_INTR_DISABLE;
//                io_conf.mode = GPIO_MODE_DISABLE;
//                return (err)gpio_config(&io_conf);
//            }
//#else
//        public:
//            inline static constexpr bool fulfilled = false;
//#endif
//        };
//
//class watchDog
//{
//#if (defined(ESP_PLATFORM) && defined(INC_FREERTOS_H))
//public:
//    inline static constexpr bool fulfilled = true;
//    inline static err set(bool isRunning, uint32_t timeout_ms)
//    {
//        if (isRunning)
//        {
//            if (esp_task_wdt_deinit())
//                return failed;
//            esp_task_wdt_config_t conf;
//            conf.timeout_ms = timeout_ms;
//            conf.trigger_panic = isRunning;
//            conf.idle_core_mask = 0;
//            if (esp_task_wdt_init(&conf))
//                return failed;
//            if (esp_task_wdt_add(xTaskGetCurrentTaskHandle()))
//                return failed;
//            return ok;
//        }
//        else {
//            return (err)esp_task_wdt_deinit();
//        }
//    }
//    inline static void feed()
//    {
//        esp_task_wdt_reset();
//    }
//#else
//public:
//    // this machine has no wdt, let's pretend it works well
//    inline static constexpr bool fulfilled = false;
//    inline static err set(bool isRunning, uint32_t timeout_ms) { return err::ok; }
//    inline static void feed() {}
//#endif
//};
//inline static uint64_t _pwmTimerUsed = 0;                    // pwm internal timer, for controllers which do not use pwm via the general timer.
//inline static uint64_t _pwmChannelUsed = 0;                  // pwm channel, pwm channel is restricted for reuse like gpio.
//template <uint32_t _Pin, uint32_t _Timer, uint32_t _Channel>
//class pwm
//{
//#if defined(ESP_PLATFORM)
//    gpio<_Pin> _gpioUsed;
//public:
//    // pwm in esp32 does not use the general timer module.
//    // if you wanna use motor controller pwm, go yourself because mcpwm includes too many relative functions that are not universal.
//    // todo: make a specialized header "io-esp32.h" to add a 'advancedPwm' class fulfill the template of pwm class
//    inline pwm()
//    {
//
//        assert(!(_pwmChannelUsed & 1 << _Channel) || !"this pwm channel has created an instance or used it for another purpose.");
//        _pwmChannelUsed |= 1 << _Channel;
//        assert(!(_pwmTimerUsed & 1 << _Timer) || !"this pwm internal timer has created an instance or used it for another purpose.");
//        _pwmTimerUsed |= 1 << _Timer;
//    }
//    inline ~pwm()
//    {
//        _pwmChannelUsed &= ~(1 << _Channel);
//        _pwmTimerUsed &= ~(1 << _Timer);
//    }
//    pwm(pwm&) = delete;
//    void operator=(pwm&) = delete;
//    static void* operator new(size_t size) = delete;
//    static void operator delete(void* pthis) = delete;
//    static_assert(_Channel < ledc_channel_t::LEDC_CHANNEL_MAX, "Invaild pwm channel.");
//    static_assert(_Timer < ledc_timer_t::LEDC_TIMER_MAX, "Invaild pwm internal timer.");
//    inline err open(uint32_t freq_hz, uint32_t duty, uint32_t hpoint = 0)
//    {
//        ledc_timer_config_t ledc_timer = {
//            .speed_mode = LEDC_LOW_SPEED_MODE,
//            .timer_num = (ledc_timer_t)_Timer,
//            .freq_hz = freq_hz,
//            .clk_cfg = LEDC_AUTO_CLK
//        };
//        ledc_timer.duty_resolution = (ledc_timer_bit_t)(LEDC_TIMER_BIT_MAX - 1);
//        if (ledc_timer_config(&ledc_timer))
//            return failed;
//        ledc_channel_config_t ledc_channel = {
//            .speed_mode = LEDC_LOW_SPEED_MODE,
//            .channel = (ledc_channel_t)_Channel,
//            .timer_sel = (ledc_timer_t)_Timer,
//            .duty = duty,
//            .hpoint = hpoint
//        };
//        ledc_channel.intr_type = LEDC_INTR_DISABLE;
//        ledc_channel.gpio_num = _Pin;
//        if (ledc_channel_config(&ledc_channel))
//            return failed;
//        if (ledc_fade_func_install(0))
//            return failed;
//        return ok;
//    }
//    inline err setDuty(uint32_t duty, uint32_t hpoint = 0)
//    {
//        return (err)ledc_set_duty_and_update(LEDC_LOW_SPEED_MODE, (ledc_channel_t)_Channel, duty, hpoint);
//    }
//    inline err setFreq(uint32_t freq_hz)
//    {
//        return (err)ledc_set_freq(LEDC_LOW_SPEED_MODE, (ledc_timer_t)_Timer, freq_hz);
//    }
//    inline void close()
//    {
//        ledc_stop(LEDC_LOW_SPEED_MODE, (ledc_channel_t)_Channel, 0);
//    }
//    inline static uint32_t dutyMax()
//    {
//        return 1 << (LEDC_TIMER_BIT_MAX - 1);
//    }
//    inline static constexpr bool fulfilled = true;
//#else
//public:
//    inline static constexpr bool fulfilled = false;
//#endif
//};
//class nvs    // disk, nvs or something else that stores data, especially for mcu which is not well supported for filesystem. denotes easily what kind of resource is required.
//{
//#if defined(_WIN32)
//    HANDLE handle;
//
//public:
//#ifdef UNICODE
//    wchar_t* path;
//    inline nvs(wchar_t* _path)
//    {
//        path = _path;
//    }
//#else
//    char* path;
//    inline nvs(char* _path)
//    {
//        path = _path;
//    }
//#endif
//    inline static constexpr bool fulfilled = true;
//    using openMode = enum : DWORD {
//        read_only = FILE_GENERIC_READ,
//        read_write = FILE_GENERIC_WRITE | FILE_GENERIC_READ
//    };
//    using shareMode = enum : DWORD {
//        none = 0
//    };
//    using createMode = enum : DWORD {
//        newOnly = CREATE_NEW,
//        newAlways = CREATE_ALWAYS,
//        openOnly = OPEN_EXISTING,
//        openAlways = OPEN_ALWAYS,
//        openTruncate = TRUNCATE_EXISTING
//    };
//    inline err open(openMode omode, shareMode smode, createMode cmode)
//    {
//        handle = CreateFile(path,
//            static_cast<DWORD>(omode),
//            static_cast<DWORD>(smode), NULL,
//            static_cast<DWORD>(cmode), FILE_ATTRIBUTE_NORMAL, NULL);
//        return (err)!handle;
//    }
//    inline uint64_t get_size()
//    {
//        DWORD fileSizeH = 0, fileSizeL = 0;
//        fileSizeL = GetFileSize(handle, &fileSizeH);
//        uint64_t fileSize = fileSizeH;
//        fileSize = (fileSize << 32);
//        fileSize += fileSizeL;
//        return fileSize;
//    }
//    inline void close()
//    {
//        CloseHandle(handle);
//    }
//    inline err read(void* dest, uint64_t& size)
//    {
//        DWORD sizeRead;
//        err ret = (err)!ReadFile(handle, dest, size, &sizeRead, NULL);
//        size = sizeRead;
//        return ret;
//    }
//    inline err write(void* dest, uint64_t size)
//    {
//        return (err)!WriteFile(handle, dest, size, NULL, NULL);
//    }
//#ifdef UNICODE
//    inline static err remove(wchar_t* path)
//    {
//        return (err)!DeleteFile(path);
//    }
//#else
//    inline static err remove(char* path)
//    {
//        return (err)!DeleteFile(path);
//    }
//#endif
//#elif defined(ESP_PLATFORM)
//    nvs_handle handle;
//    static inline bool isNvsInit = [] {
//        esp_err_t ret = nvs_flash_init();
//        if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
//            ESP_ERROR_CHECK(nvs_flash_erase());
//            return !nvs_flash_init();
//        }
//        return !ret;
//        }();
//
//public:
//    char* path;
//    inline static constexpr bool fulfilled = true;
//    using openMode = enum : std::underlying_type<nvs_open_mode_t>::type {
//        read_only = NVS_READONLY,
//        read_write = NVS_READWRITE
//    };
//    using shareMode = enum : uint8_t {
//        none = 0
//    };
//    using createMode = enum : uint8_t {
//        openAlways = 0
//    };
//    inline nvs(char* _path)
//    {
//        path = _path;
//    }
//    inline err open(openMode omode, shareMode smode, createMode cmode)
//    {
//        return (err)nvs_open(path, static_cast<nvs_open_mode_t>(omode), &handle);
//    }
//    inline void close()
//    {
//        nvs_close(handle);
//    }
//    inline uint64_t get_size()
//    {
//        uint64_t ret = 0;
//        nvs_get_u64(handle, "s", &ret);
//        return ret;
//    }
//    inline err read(void* dest, uint64_t& size)
//    {
//        size_t sizet = size;
//        err ret = (err)nvs_get_blob(handle, "c", dest, &sizet);
//        size = sizet;
//        return ret;
//    }
//    inline err write(void* dest, uint64_t size)
//    {
//        err ret;
//        if ((ret = (err)nvs_set_u64(handle, "s", size)) != ESP_OK)
//            return ret;
//        return (err)nvs_set_blob(handle, "c", dest, size);
//    }
//    inline static err remove(char* path)
//    {
//        nvs_handle handle2; // for behavior same as WIN API
//        err ret = (err)nvs_open(path, NVS_READWRITE, &handle2);
//        if (ret != ok)
//            return ret;
//        ret = (err)nvs_erase_all(handle2);
//        nvs_close(handle2);
//        return ret;
//    }
//#else
//public:
//    inline static constexpr bool fulfilled = false;
//#endif
//};

//friend class sntp_client;
//friend class wifi_sta;
//static inline bool _is_init_netif = false;
//esp_netif_t* handle;
//esp_ping_handle_t ping_handle = nullptr;
//singal cb_singal;
//        public:
//            inline netif()
//            {
//                if (false == _is_init_netif)
//                    esp_netif_init();
//            }
//            inline ~netif()
//            {
//                esp_ping_delete_session(ping_handle);
//            }
//            inline static constexpr bool fulfilled = true;
//            inline err ping(const char* test_host, int& delay, int timeout)
//            {
//                if (ping_handle == nullptr)
//                {
//                    esp_ping_config_t ping_config = ESP_PING_DEFAULT_CONFIG();
//                    ping_config.count = 1;
//                    ping_config.timeout_ms = timeout;
//                    ping_config.interval_ms = timeout;
//                    ping_config.interface = esp_netif_get_netif_impl_index(handle);
//                    esp_ping_callbacks_t cbs = {
//                        .cb_args = nullptr,
//                        .on_ping_success = on_ping_success_cb,
//                        .on_ping_timeout = nullptr,
//                        .on_ping_end = nullptr,
//                    };
//
//                    netif_ping_singal = cb_singal.handle;
//                    ip4_addr_t target_addr;
//                    inet_pton(AF_INET, test_host, &target_addr);
//                    ping_target_id_t targ = PING_TARGET_IP_ADDRESS;
//                    esp_ping_set_target(targ, &target_addr, sizeof(target_addr));
//
//                    ping_config.target_addr.u_addr.ip4 = target_addr;
//                    ping_config.target_addr.type = IPADDR_TYPE_V4;
//                    if (esp_ping_new_session(&ping_config, &cbs, &ping_handle) != ESP_OK)
//                    {
//                        ping_handle = nullptr;
//                        return failed;
//                    }
//                }
//
//                esp_ping_start(ping_handle);
//                cb_singal.wait(timeout);
//                esp_ping_stop(ping_handle);
//
//                if (is_ping_successed)
//                {
//                    is_ping_successed = false;
//                    delay = netif_ping_delay;
//                    return ok;
//                }
//                return failed;
//            }
//            // does your network way link to the Internet?
//            inline err internetLinkTest(int& avrDelay, int timeout)
//            {
//                const char* test_hosts[] = {
//                    "119.29.29.29",    // DNSPod Public DNS
//                    "182.254.116.116", // DNSPod Public DNS
//                    "223.5.5.5",       // AliDNS
//                    "223.6.6.6",       // AliDNS
//                    "180.76.76.76",    // Baidu Public DNS
//                    "101.226.4.6",     // China Telecom DNS
//                    "123.125.81.6"     // Baidu
//                };
//                const int num_hosts = sizeof(test_hosts) / sizeof(test_hosts[0]);
//                uint64_t delaySum = 0;
//                int successfulPings = 0;
//                int delay = 0;
//
//                for (int i = 0; i < num_hosts; i++)
//                {
//                    if (ping(test_hosts[i], delay, timeout) == ok)
//                    {
//                        delaySum += delay;
//                        successfulPings++;
//                        delay = 0;
//                    }
//                    watchDog::feed();
//                }
//
//                if (successfulPings > 0)
//                {
//                    avrDelay = delaySum / successfulPings;
//                    return ok;
//                }
//                else
//                {
//                    return failed;
//                }
//            }


class bluetooth_device
{
#if defined(ESP_PLATFORM)
protected:
    inline static uint _useCount = 0;
    inline static void global_bt_init()
    {
        esp_err_t ret;
        ESP_ERROR_CHECK(
            esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

        esp_bt_controller_config_t bt_cfg =
            BT_CONTROLLER_INIT_CONFIG_DEFAULT();
        ret = esp_bt_controller_init(&bt_cfg);
        if (ret)
        {
            ESP_LOGE("GATTS_TAG", "%s initialize controller failed", __func__);
            return;
        }

        ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
        if (ret)
        {
            ESP_LOGE("GATTS_TAG", "%s enable controller failed", __func__);
            return;
        }

        ret = esp_bluedroid_init();
        if (ret)
        {
            ESP_LOGE("GATTS_TAG", "%s init bluetooth failed", __func__);
            return;
        }
        ret = esp_bluedroid_enable();
        if (ret)
        {
            ESP_LOGE("GATTS_TAG", "%s enable bluetooth failed", __func__);
            return;
        }
    }
    inline static void global_bt_deinit()
    {
        esp_bluedroid_deinit();
        esp_bt_controller_deinit();
    }
public:
    inline static constexpr bool fulfilled = true;
    inline bluetooth_device()
    {
        if (_useCount == 0)
        {
            global_bt_init();
        }
        _useCount++;
    }
    inline ~bluetooth_device()
    {
        if (_useCount == 1)
        {
            global_bt_deinit();
        }
        _useCount--;
    }
#else
public:
    inline static constexpr bool fulfilled = false;
#endif
};

class bluetooth_client : public bluetooth_device {};

class bluetooth_server : public bluetooth_device
{
#if defined(ESP_PLATFORM)
public:
    inline static constexpr bool fulfilled = true;
    inline bluetooth_server()
    {
    }
    inline ~bluetooth_server()
    {
        this->close();
    }
    // todo: (Blocking) The call thread will hang up till the server open complete
    inline err open(char* deviceName)
    {
        return (err)ble_example_init(deviceName);
    }
    inline err addService()
    {
        return ok;
    }
    inline err addChar(dualBuffer<pData>* buffer)
    {
        ble_server_char_set_cache(buffer);
        return ok;
    }
    inline err sendCharNotify()
    {}
    inline err setCharReply()
    {}
    inline err removeChar()
    {}
    inline err removeService()
    {}
    inline err getMac(uint8_t(&outp)[6])
    {
        auto macScr = esp_bt_dev_get_address();
        if (macScr == nullptr)
            return failed;
        memcpy(&outp, macScr, sizeof(uint8_t) * 6);
        return ok;
    }
    inline void close()
    {
        ble_server_close();
    }
#else
public:
    inline static constexpr bool fulfilled = false;
#endif
};

class wifi_device
{
public:
    using adConfig = struct {
        std::string SSID;
        std::string password;
    };
    using linkConfig = struct {
        std::string SSID;
        std::string password;
    };
#if defined(ESP_PLATFORM)
protected:
    inline static uint _useCount = 0;
    inline static void global_wifi_init()
    {
        esp_err_t ret = esp_event_loop_create_default();
        ESP_ERROR_CHECK(ret);
        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        ret = esp_wifi_init(&cfg);
        ESP_ERROR_CHECK(ret);
    }
    inline static void global_wifi_deinit()
    {
        esp_wifi_deinit();
        esp_event_loop_delete_default();
    }

public:
    inline static constexpr bool fulfilled = true;
    inline wifi_device()
    {
        if (_useCount == 0)
        {
            global_wifi_init();
        }
        _useCount++;
    }
    inline ~wifi_device()
    {
        if (_useCount == 1)
        {
            global_wifi_deinit();
        }
        _useCount--;
    }
#else
public:
    inline static constexpr bool fulfilled = false;
#endif
};

class wifi_sta :public wifi_device
{
#if defined(ESP_PLATFORM)
    int advertising_count = 0;
    int linkSuccessful = false;
public:
    inline static constexpr bool fulfilled = true;
    duration_ms overtime_t = std::chrono::seconds(7);
    singal overtime_s;
    inline wifi_sta()
    {
        esp_netif_create_default_wifi_sta();
        esp_wifi_set_mode(WIFI_MODE_STA);
        wifi_example_regist_cb();
    }
    inline ~wifi_sta()
    {}
    inline err open()
    {
        return (err)esp_wifi_start();
    }
    inline err getAdvertisingBegin(wifi_device::adConfig& ad)
    {
    }
    inline err getAdvertisingContinue(wifi_device::adConfig& ad)
    {
    }
    inline err tryConnect(wifi_device::linkConfig& conf, netif& network_out)
    {
        wifi_config_t wifi_cfg = {};
        memcpy(wifi_cfg.sta.ssid, conf.SSID.c_str(), conf.SSID.size());
        memcpy(wifi_cfg.sta.password, conf.password.c_str(), conf.password.size());
        err ret = (err)esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg);
        if (ret != ok)
            return ret;

        wifi_example_on_connected(overtime_s.handle, &linkSuccessful);
        if (linkSuccessful)
            esp_wifi_disconnect();
        overtime_s.wait(std::chrono::milliseconds(100).count());
        ret = (err)esp_wifi_connect();
        if (ret == ok)
        {
            watchDog::feed();
            overtime_s.wait(overtime_t.count());
            watchDog::feed();
            if (linkSuccessful)
            {
                network_out.handle = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
                return ok;
            }
            else
            {
                return failed;
            }
        }
        return ret;
    }
    inline err disconnect()
    {
        return (err)esp_wifi_disconnect();
    }
    inline bool isConnected() { return linkSuccessful; }
#else
public:
    inline static constexpr bool fulfilled = false;
#endif
};

class wifi_ap :public wifi_device {};

#if defined(ESP_PLATFORM)
inline static singal cb_singal;
inline static std::atomic<bool> inited = false;
        public:
            inline static constexpr bool fulfilled = true;
            inline static err initService(netif& _netif, const char* ntp_server, uint32_t overtime)
            {
                static bool init = [&]
                    {
                        singal_inited = &inited;
                        sntp_time_singal = cb_singal.handle;
                        sntp_setoperatingmode(SNTP_OPMODE_POLL);
                        setenv("TZ", "CST-8", 1);
                        tzset();
                        return true;
                    }();
                bool a = false;
                if (inited.compare_exchange_strong(a, false))
                {
                    sntp_set_time_sync_notification_cb(time_sync_notification_cb);
                    sntp_init();
                    sntp_setservername(0, ntp_server);
                    // sntp_set_netif_index(esp_netif_get_netif_impl_index(_netif.handle));
                    if (cb_singal.wait(overtime) == singal::success)
                    {
                        return ok;
                    }
                    else
                    {
                        return failed;
                    }
                }
                return ok;
            }
            inline static void getTime(struct tm& output)
            {
                time_t now;
                time(&now);
                localtime_r(&now, &output);
            }
#else
        public:
            inline static constexpr bool fulfilled = false;
#endif