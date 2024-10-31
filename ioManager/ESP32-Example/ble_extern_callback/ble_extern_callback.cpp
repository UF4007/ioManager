#include "../../ioManager.h"
#include "ble_extern_callback.h"

io::dualCache<io::pData>* ble_isr_buffer;
extern "C"
{
    void ble_server_char_set_cache(void* cache)
    {
        ble_isr_buffer = (io::dualCache<io::pData>*)cache;
    }
    void ble_server_char_rec_isr(char* data, int size, int offset)
    {
        std::queue<io::pData>* pQ = ble_isr_buffer->inbound_get();

        io::pData pdata((io::byte_t *)data, size + 1, (uint32_t)io::netMessage::bluetooth_client_rec);
        if (pQ->size() < 10)
            pQ->push(pdata);

        ble_isr_buffer->inbound_unlock(pQ);
    }
}