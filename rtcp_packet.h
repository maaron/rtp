#pragma once

#include <stdint.h>

#define RTCP_SR 200

namespace media
{
    struct sender_report
    {
        uint32_t ssrc;
        uint32_t ntp_msw;
        uint32_t ntp_lsw;
        uint32_t rtp_timestamp;
        uint32_t packet_count;
        uint32_t octet_count;
    };

    struct report_block
    {
        uint32_t ssrc;
        uint32_t fraction_cumulative_lost;
        uint32_t extended_seq_received;
        uint32_t interarrival_jitter;
        uint32_t last_sr;
        uint32_t delay_since_last_sr;
    };

    struct sdes_chunk
    {
        uint32_t src;
    };

    struct sdes_item
    {
        uint8_t type;
        uint8_t length;
    };

    struct rtcp_header
    {
        uint8_t V_P_RC;
        uint8_t PT;
        uint16_t length;
    };

    class rtcp_packet
    {
        rtcp_header* header;
        char* end_ptr;
        char* write_ptr;
        char* data_ptr;

        int get_V();
        bool get_P();
        int get_RC();
        uint8_t get_PT();

        void set_V(int);
        void set_P(bool);
        void set_RC(int);
        void set_PT(uint8_t);

        void add_bytes(int);
        void increment_RC();

    public:
        // For received packets
        rtcp_packet(void* data, size_t size);

        void* data();
        size_t size();

        bool move_next();

        void write_header();
        void write_sender_info(uint32_t ssrc, uint64_t ntp_time, uint32_t rtp_time, uint32_t octet_count, uint32_t packet_count);
        void write_sender_block(uint32_t ssrc, uint8_t fraction_lost, uint32_t packets_lost, uint32_t highest_sequence, uint32_t jitter, uint32_t last_sr, uint32_t delay_last_sr);
    };

    void copy_header(rtcp_packet& src, rtcp_packet& dst);
}