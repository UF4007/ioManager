#include "esp_timer.h"
#include "esp_task_wdt.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_ping.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_ping.h"
#include "esp_mac.h"
#include "esp_http_client.h"
#include "esp_ota_ops.h"
#include "esp_https_ota.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include <arpa/inet.h>
#include "socket.h"
#include "ping/ping.h"
#include "ping/ping_sock.h"
#include "wifi_extern_callback.h"

static int* _linkSuccessful = NULL;
static int s_retry_num = 0;
static SemaphoreHandle_t _handle;        //singal

static void event_handler(void* arg, esp_event_base_t event_base,
                          int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < 0) {
            esp_wifi_connect();
            s_retry_num++;
        } else {
            s_retry_num = 0;
            if (_linkSuccessful)
                *_linkSuccessful = 0;
            if (_handle)
                xSemaphoreGive(_handle);
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        s_retry_num = 0;
        if (_linkSuccessful)
            *_linkSuccessful = 1;
        if (_handle)
            xSemaphoreGive(_handle);
    }
}

void wifi_example_regist_cb()
{
    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, &instance_any_id);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, &instance_got_ip);
}
void wifi_example_on_connected(SemaphoreHandle_t handle, int* linkSuccessful)
{
    _handle = handle;
    _linkSuccessful = linkSuccessful;
}