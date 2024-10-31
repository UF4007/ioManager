#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"

#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"
#include "esp_timer.h"

#ifdef __cplusplus
extern "C"
{
#endif

    void ble_server_close();
    esp_err_t ble_example_init(char *deviceName);
    void ble_char_send_notify(void *notify, unsigned int size);
    void ble_set_char_rec_isr(void *cb);
    
#ifdef __cplusplus
}
#else
struct bt_sever_rec
{
    uint8_t *bytes;
    uint32_t size;
    uint32_t offset;
};
#endif