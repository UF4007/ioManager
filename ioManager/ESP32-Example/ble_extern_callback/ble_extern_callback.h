#pragma once

#ifdef __cplusplus
extern "C"
{
#endif
    void ble_server_char_set_cache(void* cache);
    void ble_server_char_rec_isr(char* data, int size, int offset);
#ifdef __cplusplus
}
#endif