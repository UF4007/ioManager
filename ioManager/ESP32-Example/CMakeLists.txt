set(requires nvs_flash esp_netif esp_timer wpa_supplicant mqtt bt json app_update)
set(priv_requires esp-tls unity esp_http_server esp_wifi esp_eth mqtt esp_http_client esp_https_ota)

if(${IDF_TARGET} STREQUAL "linux")
list(APPEND requires esp_stubs esp-tls esp_http_server  nvs_flash esp_netif mqtt esp_bt)
endif()

idf_component_register(
    SRC_DIRS 
    "."
    "wifi_extern_callback"
    "ble_extern_callback"

    INCLUDE_DIRS
    "."
    "wifi_extern_callback"
    "ble_extern_callback"

    REQUIRES ${requires}

    PRIV_REQUIRES  ${priv_requires}

    # EMBED_TXTFILES user_mqtt/ali_iot_ca.crt
    
    WHOLE_ARCHIVE
)