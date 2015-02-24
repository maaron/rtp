#pragma once

#include <stdint.h>

namespace media
{
    struct rtp_header
    {
        uint8_t V_P_X_CC;
        uint8_t M_PT;
        uint16_t sequence_number;
        uint32_t timestamp;
        uint32_t ssrc;
    };

    class rtp_packet
    {
        rtp_header* header;
        size_t packet_size;
        uint32_t* csrc;

    public:
        static const size_t min_size;

        rtp_packet(void* buffer, size_t size);

        void* data();
        size_t size();

        size_t payload_size();
        
        int get_V();
        bool get_P();
        bool get_X();
        int get_CC();
        bool get_M();
        int get_PT();
        uint16_t get_sequence_number();
        int get_timestamp();
        int get_ssrc();
        int get_csrc(int index);

        void set_V(int v);
        void set_P(bool p);
        void set_X(bool x);
        void set_CC(int cc);
        void set_M(bool m);
        void set_PT(int pt);
        void set_sequence_number(uint16_t seq);
        void set_timestamp(int timestamp);
        void set_ssrc(int ssrc);
        void set_csrc(int index, int csrc);
    };
}