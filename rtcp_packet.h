#pragma once

#include <stdint.h>

#define RTCP_SR 200
#define RTCP_RR 201
#define RTCP_SDES 202
#define RTCP_BYE 203

#define SDES_CNAME 1
#define SDES_NAME 2
#define SDES_EMAIL 3
#define SDES_PHONE 4

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

    // This class is used for both reading and writing RTCP compound packets.
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
        bool write_header(int pt);
        void write_sdes_item(int id, const char* data);

    public:
        rtcp_packet(void* data, size_t size);

        void* data();
        size_t size();

        bool move_next();

        void write_sender_report(
            uint32_t ssrc, 
            uint64_t ntp_time, 
            uint32_t rtp_time, 
            uint32_t octet_count, 
            uint32_t packet_count);

        void write_sender_report_block(
            uint32_t ssrc, 
            uint8_t fraction_lost, 
            uint32_t packets_lost, 
            uint32_t highest_sequence, 
            uint32_t jitter, 
            uint32_t last_sr, 
            uint32_t delay_last_sr);
        
        void write_sdes(uint32_t src);
        void write_sdes_cname(const char* user_host);
        void write_sdes_name(const char* name);
        void write_sdes_email(const char* email);
        void write_sdes_end();

        void write_bye(uint32_t src);
        void write_bye_src(uint32_t src);
        void write_bye_reason(const char* reason);
    };
}