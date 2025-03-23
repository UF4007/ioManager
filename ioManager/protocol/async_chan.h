#pragma once
namespace io {
    inline namespace IO_LIB_VERSION___ {
        //async channel
        // Thread safe.
        template <typename T>
        struct async_chan {};

        // receive only channel
        template <typename T>
        struct async_chan_r {};

        // send only channel
        template <typename T>
        struct async_chan_s {};
    };
};