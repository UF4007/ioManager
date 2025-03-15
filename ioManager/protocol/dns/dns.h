//library: libhv
//referenced from: https://github.com/ithewei/libhv/blob/master/protocol/dns.c
//
//under BSD-3 license
//2024.10.15

#define DNS_PORT        53

#define DNS_QUERY       0
#define DNS_RESPONSE    1

#define DNS_TYPE_A      1   // ipv4
#define DNS_TYPE_NS     2
#define DNS_TYPE_CNAME  5
#define DNS_TYPE_SOA    6
#define DNS_TYPE_WKS    11
#define DNS_TYPE_PTR    12
#define DNS_TYPE_HINFO  13
#define DNS_TYPE_MX     15
#define DNS_TYPE_AAAA   28  // ipv6
#define DNS_TYPE_AXFR   252
#define DNS_TYPE_ANY    255

#define DNS_CLASS_IN    1

#define DNS_NAME_MAXLEN 256
#define DNS_ALLOCATE_MAX 8196

namespace dns_internal {

    // sizeof(dnshdr_t) = 12
    struct dnshdr_little {
        uint16_t    transaction_id = 0x1234;

        uint8_t     rd : 1 = 1;
        uint8_t     tc : 1 = 0;
        uint8_t     aa : 1 = 0;
        uint8_t     opcode : 4 = 0;
        uint8_t     qr : 1 = 0;

        uint8_t     rcode : 4 = 0;
        uint8_t     cd : 1 = 0;
        uint8_t     ad : 1 = 0;
        uint8_t     res : 1 = 0;
        uint8_t     ra : 1 = 0;

        uint16_t    nquestion = htons(1);
        uint16_t    nanswer = 0;
        uint16_t    nauthority = 0;
        uint16_t    naddtional = 0;
    };
    struct dnshdr_big {
        uint16_t    transaction_id = 0x1234;

        uint8_t    qr : 1;   // DNS_QUERY or DNS_RESPONSE
        uint8_t    opcode : 4;
        uint8_t    aa : 1;   // authoritative
        uint8_t    tc : 1;   // truncated
        uint8_t    rd : 1;   // recursion desired

        uint8_t    ra : 1;   // recursion available
        uint8_t    res : 1;  // reserved
        uint8_t    ad : 1;   // authenticated data
        uint8_t    cd : 1;   // checking disable
        uint8_t    rcode : 4;

        uint16_t    nquestion = htons(1);
        uint16_t    nanswer = 0;
        uint16_t    nauthority = 0;
        uint16_t    naddtional = 0;
    };

    constexpr auto endian = std::endian::native;

    using dnshdr_t = std::conditional_t<endian == std::endian::big, dnshdr_big, dnshdr_little>;

    struct dns_rr_s {
        char        name[DNS_NAME_MAXLEN]; // original domain, such as www.example.com
        uint16_t    rtype;
        uint16_t    rclass;
        uint32_t    ttl;
        uint16_t    datalen;
        char* data;
    };

    using dns_rr_t = dns_rr_s;

    struct dns_s {
        dnshdr_t  hdr;
        dns_rr_t* questions;
        dns_rr_t* answers;
        dns_rr_t* authorities;
        dns_rr_t* addtionals;
    };

    using dns_t = dns_s;

    inline void safe_free(void* ptr) {
        ::operator delete(ptr);
    }

    inline io::err safe_alloc(void*& ptr,size_t size) {
        if (size > DNS_ALLOCATE_MAX)
            return io::err::failed;
        ptr = ::operator new(size);
        return io::err::ok;
    }

    inline void dns_free(dns_t* dns) {
        safe_free(dns->questions);
        safe_free(dns->answers);
        safe_free(dns->authorities);
        safe_free(dns->addtionals);
    }

    // www.example.com => 3www7example3com
    inline int dns_name_encode(const char* domain, char* buf) {
        const char* p = domain;
        char* plen = buf++;
        int buflen = 1;
        int len = 0;
        while (*p != '\0') {
            if (*p != '.') {
                ++len;
                *buf = *p;
            }
            else {
                *plen = len;
                //printf("len=%d\n", len);
                plen = buf;
                len = 0;
            }
            ++p;
            ++buf;
            ++buflen;
        }
        *plen = len;
        //printf("len=%d\n", len);
        *buf = '\0';
        if (len != 0) {
            ++buflen; // include last '\0'
        }
        return buflen;
    }

    // 3www7example3com => www.example.com
    inline int dns_name_decode(const char* buf, char* domain) {
        const char* p = buf;
        int len = *p++;
        //printf("len=%d\n", len);
        int buflen = 1;
        while (*p != '\0') {
            if (len-- == 0) {
                len = *p;
                //printf("len=%d\n", len);
                *domain = '.';
            }
            else {
                *domain = *p;
            }
            ++p;
            ++domain;
            ++buflen;
        }
        *domain = '\0';
        ++buflen; // include last '\0'
        return buflen;
    }

    inline int dns_rr_pack(dns_rr_t* rr, char* buf, int len) {
        char* p = buf;
        char encoded_name[256];
        int encoded_namelen = dns_name_encode(rr->name, encoded_name);
        int packetlen = encoded_namelen + 2 + 2 + (rr->data ? (4 + 2 + rr->datalen) : 0);
        if (len < packetlen) {
            return -1;
        }

        memcpy(p, encoded_name, encoded_namelen);
        p += encoded_namelen;
        uint16_t* pushort = (uint16_t*)p;
        *pushort = htons(rr->rtype);
        p += 2;
        pushort = (uint16_t*)p;
        *pushort = htons(rr->rclass);
        p += 2;

        // ...
        if (rr->datalen && rr->data) {
            uint32_t* puint = (uint32_t*)p;
            *puint = htonl(rr->ttl);
            p += 4;
            pushort = (uint16_t*)p;
            *pushort = htons(rr->datalen);
            p += 2;
            memcpy(p, rr->data, rr->datalen);
            p += rr->datalen;
        }
        return packetlen;
    }

    inline int dns_rr_unpack(char* buf, int len, dns_rr_t* rr, int is_question) {
        char* p = buf;
        int off = 0;
        int namelen = 0;
        if (*(uint8_t*)p >= 192) {
            // name off, we ignore
            namelen = 2;
            //uint16_t nameoff = (*(uint8_t*)p - 192) * 256 + *(uint8_t*)(p+1);
        }
        else {
            namelen = dns_name_decode(buf, rr->name);
        }
        if (namelen < 0) return -1;
        p += namelen;
        off += namelen;

        if (len < off + 4) return -1;
        uint16_t* pushort = (uint16_t*)p;
        rr->rtype = ntohs(*pushort);
        p += 2;
        pushort = (uint16_t*)p;
        rr->rclass = ntohs(*pushort);
        p += 2;
        off += 4;

        if (!is_question) {
            if (len < off + 6) return -1;
            uint32_t* puint = (uint32_t*)p;
            rr->ttl = ntohl(*puint);
            p += 4;
            pushort = (uint16_t*)p;
            rr->datalen = ntohs(*pushort);
            p += 2;
            off += 6;
            if (len < off + rr->datalen) return -1;
            rr->data = p;
            p += rr->datalen;
            off += rr->datalen;
        }
        return off;
    }

    inline int dns_pack(dns_t* dns, char* buf, int len) {
        if (len < sizeof(dnshdr_t)) return -1;
        int off = 0;
        dnshdr_t* hdr = &dns->hdr;
        dnshdr_t htonhdr = dns->hdr;
        htonhdr.transaction_id = htons(hdr->transaction_id);
        htonhdr.nquestion = htons(hdr->nquestion);
        htonhdr.nanswer = htons(hdr->nanswer);
        htonhdr.nauthority = htons(hdr->nauthority);
        htonhdr.naddtional = htons(hdr->naddtional);
        memcpy(buf, &htonhdr, sizeof(dnshdr_t));
        off += sizeof(dnshdr_t);
        int i;
        for (i = 0; i < hdr->nquestion; ++i) {
            int packetlen = dns_rr_pack(dns->questions + i, buf + off, len - off);
            if (packetlen < 0) return -1;
            off += packetlen;
        }
        for (i = 0; i < hdr->nanswer; ++i) {
            int packetlen = dns_rr_pack(dns->answers + i, buf + off, len - off);
            if (packetlen < 0) return -1;
            off += packetlen;
        }
        for (i = 0; i < hdr->nauthority; ++i) {
            int packetlen = dns_rr_pack(dns->authorities + i, buf + off, len - off);
            if (packetlen < 0) return -1;
            off += packetlen;
        }
        for (i = 0; i < hdr->naddtional; ++i) {
            int packetlen = dns_rr_pack(dns->addtionals + i, buf + off, len - off);
            if (packetlen < 0) return -1;
            off += packetlen;
        }
        return off;
    }

    inline io::err dns_unpack(char* buf, int len, dns_t* dns) {
        int phase = 0;
        while (1)
        {
            memset(dns, 0, sizeof(dns_t));
            if (len < sizeof(dnshdr_t)) return io::err::failed;
            int off = 0;
            dnshdr_t* hdr = &dns->hdr;
            memcpy(hdr, buf, sizeof(dnshdr_t));
            off += sizeof(dnshdr_t);
            hdr->transaction_id = ntohs(hdr->transaction_id);
            hdr->nquestion = ntohs(hdr->nquestion);
            hdr->nanswer = ntohs(hdr->nanswer);
            hdr->nauthority = ntohs(hdr->nauthority);
            hdr->naddtional = ntohs(hdr->naddtional);
            int i;
            if (hdr->nquestion) {
                int bytes = hdr->nquestion * sizeof(dns_rr_t);
                if (safe_alloc((void*&)dns->questions, bytes) == io::err::failed)break;
                for (i = 0; i < hdr->nquestion; ++i) {
                    int packetlen = dns_rr_unpack(buf + off, len - off, dns->questions + i, 1);
                    if (packetlen < 0) break;
                    off += packetlen;
                }
                phase++;
            }
            if (hdr->nanswer) {
                int bytes = hdr->nanswer * sizeof(dns_rr_t);
                if (safe_alloc((void*&)dns->answers, bytes) == io::err::failed)break;
                for (i = 0; i < hdr->nanswer; ++i) {
                    int packetlen = dns_rr_unpack(buf + off, len - off, dns->answers + i, 0);
                    if (packetlen < 0) break;
                    off += packetlen;
                }
                phase++;
            }
            if (hdr->nauthority) {
                int bytes = hdr->nauthority * sizeof(dns_rr_t);
                if (safe_alloc((void*&)dns->authorities, bytes) == io::err::failed)break;
                for (i = 0; i < hdr->nauthority; ++i) {
                    int packetlen = dns_rr_unpack(buf + off, len - off, dns->authorities + i, 0);
                    if (packetlen < 0) break;
                    off += packetlen;
                }
                phase++;
            }
            if (hdr->naddtional) {
                int bytes = hdr->naddtional * sizeof(dns_rr_t);
                if (safe_alloc((void*&)dns->addtionals, bytes) == io::err::failed)break;
                for (i = 0; i < hdr->naddtional; ++i) {
                    int packetlen = dns_rr_unpack(buf + off, len - off, dns->addtionals + i, 0);
                    if (packetlen < 0) break;
                    off += packetlen;
                }
                phase++;
            }
            return io::err::ok;
        }
        switch (phase)
        {
        case 4:
            safe_free(dns->addtionals);
        case 3:
            safe_free(dns->authorities);
        case 2:
            safe_free(dns->answers);
        case 1:
            safe_free(dns->questions);
        default:
            break;
        }
        return io::err::failed;
    }
}

struct data {
    io::dns::dns_internal::dns_t content;
    bool allocated = false;
    data() = default;
    data(const data&) = delete;
    void operator =(const data&) = delete;
    inline ~data() {
        clear();
    }
    inline void clear() {
        if (allocated)
        {
            io::dns::dns_internal::dns_free(&content);
            allocated = false;
        }
    }
    inline io::err fromChar(char* rawData, size_t size)
    {
        clear();
        auto ret = dns_unpack(rawData, size, &content);
        if (ret == io::err::ok)
            allocated = true;
        return ret;
    }
    inline io::err getIp(sockaddr_in& addr4)
    {
        if (allocated)
        {
            io::dns::dns_internal::dns_rr_t* rr = content.answers;
            for (int i = 0; i < content.hdr.nanswer; i++)
            {
                if (rr->rtype == 1) { // A record
                    addr4.sin_family = AF_INET;
                    memcpy(&addr4.sin_addr, rr->data, sizeof(addr4.sin_addr));
                    return io::err::ok;
                }
                rr ++;
            }
        }
        return io::err::failed;
    }
};
inline io::dns::dns_internal::dnshdr_t header;
inline std::string query(const std::string& domain) {
    std::ostringstream stream;

    stream.write(reinterpret_cast<const char*>(&header), sizeof(io::dns::dns_internal::dnshdr_t));

    size_t pos = 0;

    char buf[DNS_NAME_MAXLEN];
    size_t siz = io::dns::dns_internal::dns_name_encode(domain.c_str(), buf);

    stream.write(buf, siz);

    uint16_t qType = htons(1);  // A record
    uint16_t qClass = htons(1); // IN (Internet)

    stream.write(reinterpret_cast<const char*>(&qType), sizeof(qType));
    stream.write(reinterpret_cast<const char*>(&qClass), sizeof(qClass));

    return stream.str();
}