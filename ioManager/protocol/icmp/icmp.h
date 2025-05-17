struct icmp_data {
    std::byte type;
    std::byte code;
    uint16_t checksum;
    uint16_t identifier;
    uint16_t sequence;
    inline static icmp_data ping_header(uint16_t identifier) {
        icmp_data icmpHeader;
        icmpHeader.type = (std::byte)8;
        icmpHeader.code = (std::byte)0;
        icmpHeader.checksum = 0;
        icmpHeader.identifier = identifier;
        icmpHeader.sequence = 1;
        icmpHeader.checksum = computeChecksum(reinterpret_cast<uint16_t*>(&icmpHeader), sizeof(icmpHeader));
        return icmpHeader;
    }
    inline static uint16_t computeChecksum(uint16_t* pBuf, int length) {
        uint32_t sum = 0;
        for (int i = 0; i < length / 2; ++i) {
            sum += pBuf[i];
        }
        if (length % 2) {
            sum += static_cast<uint16_t>(reinterpret_cast<char*>(pBuf)[length - 1]);
        }
        while (sum >> 16) {
            sum = (sum & 0xFFFF) + (sum >> 16);
        }
        return static_cast<uint16_t>(~sum);
    }
};