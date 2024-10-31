#pragma once

extern "C"
{
    inline std::atomic<bool>* singal_inited = nullptr;
    inline SemaphoreHandle_t sntp_time_singal = nullptr;
    inline void time_sync_notification_cb(struct timeval *tv)
    {
        if (sntp_time_singal)
        {
            singal_inited->store(true);
            xSemaphoreGive(sntp_time_singal);
        }
    }
}