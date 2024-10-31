#pragma once

extern "C"
{
    inline unsigned int netif_ping_delay = 0;
    inline bool is_ping_successed = false;
    inline SemaphoreHandle_t netif_ping_singal;
    inline void on_ping_success_cb(esp_ping_handle_t hdl, void *args)
    {
        esp_ping_get_profile(hdl, ESP_PING_PROF_TIMEGAP, &netif_ping_delay, sizeof(netif_ping_delay));
        is_ping_successed = true;
        xSemaphoreGive(netif_ping_singal);
    }
}