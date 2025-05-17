class sntp_data {
    enum class mode : uint8_t {
        client,
    };
public:
    uint8_t li_vn_mode;      // Leap Indicator, Version Number, Mode
    uint8_t stratum;         // Stratum
    uint8_t poll;            // Poll Interval
    uint8_t precision;       // Precision
    uint32_t root_delay;     // Root Delay
    uint32_t root_dispersion; // Root Dispersion
    uint32_t ref_id;         // Reference ID
    uint32_t ref_timestamp[2]; // Reference Timestamp (64-bit)
    uint32_t orig_timestamp[2]; // Originate Timestamp (64-bit)
    uint32_t recv_timestamp[2]; // Receive Timestamp (64-bit)
    uint32_t tx_timestamp[2];   // Transmit Timestamp (64-bit)

    inline void setLI_VN_Mode(uint8_t li, uint8_t vn, uint8_t mode) {
        li_vn_mode = (li << 6) | (vn << 3) | mode;
    }

    inline void setRefTimestamp(std::chrono::system_clock::time_point tp) {
        auto duration = tp.time_since_epoch();
        auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration).count();
        auto fractional = std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count() % 1000000000;

        ref_timestamp[0] = static_cast<uint32_t>(seconds);
        ref_timestamp[1] = static_cast<uint32_t>(fractional);
    }

    inline void setOrigTimestamp(std::chrono::system_clock::time_point tp) {
        auto duration = tp.time_since_epoch();
        auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration).count();
        auto fractional = std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count() % 1000000000;

        orig_timestamp[0] = static_cast<uint32_t>(seconds);
        orig_timestamp[1] = static_cast<uint32_t>(fractional);
    }

    inline uint8_t getLI() const {
        return (li_vn_mode >> 6) & 0x03;
    }

    inline uint8_t getVN() const {
        return (li_vn_mode >> 3) & 0x07;
    }

    inline uint8_t getMode() const {
        return li_vn_mode & 0x07;
    }

    inline std::chrono::system_clock::time_point getTxTimestamp() const {
        std::chrono::seconds sec(tx_timestamp[0]);
        std::chrono::nanoseconds frac(tx_timestamp[1]);
        std::chrono::system_clock::duration duration_sec = sec;
        std::chrono::system_clock::duration duration_frac = std::chrono::duration_cast<std::chrono::system_clock::duration>(frac);
        return std::chrono::system_clock::time_point(duration_sec + duration_frac);
    }

    inline std::chrono::system_clock::time_point getOrigTimestamp() const {
        std::chrono::seconds sec(tx_timestamp[0]);
        std::chrono::nanoseconds frac(tx_timestamp[1]);
        std::chrono::system_clock::duration duration_sec = sec;
        std::chrono::system_clock::duration duration_frac = std::chrono::duration_cast<std::chrono::system_clock::duration>(frac);
        return std::chrono::system_clock::time_point(duration_sec + duration_frac);
    }
};