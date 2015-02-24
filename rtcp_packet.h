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
        uint32_t fraction_lost:8;
        uint32_t cumulative_lost:24;
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
        uint8_t V:2;
        uint8_t P:1;
        uint8_t RC:5;
        uint8_t PT;
        uint16_t length;
    };

    // This class is used for both reading and writing RTCP compound packets.
    class rtcp_packet
    {
        uint32_t* header;
        char* end_ptr;
        char* write_ptr;
        char* data_ptr;

        void add_bytes(int);
        void increment_RC();
        void write_header(int pt);
        void write_sdes_item(int id, const char* data);
        size_t packet_bytes_written();
        void write32(uint32_t data);
        uint32_t read32();

    public:
        rtcp_packet(void* data, size_t size);

        // Returns the internal pointer to the compound packet data
        void* data();

        // Returns the current size of the compound packet
        size_t compound_size();

        // Returns the size of the packet currently being read/written, 
        // including the 4-byte header.  This is 4 bytes less than the 
        // length represented by the RTCP length field (the length field is 
        // also in 32-bit units).
        size_t packet_size();

        bool move_next();

        void write_sender_report(sender_report&);

        void write_sender_report_block(report_block&);
        
        // Writes the SDES packet header, starting a new RTCP packet within the compound packet.
        void write_sdes(uint32_t src);

        // Writes the given SDES item
        void write_sdes_cname(const char* user_host);
        void write_sdes_name(const char* name);
        void write_sdes_email(const char* email);

        // Writes the padding bytes and sets the correct length in the 
        // packet header.  This MUST be called after writing SDES items, or 
        // the packet won't be well-formed.
        void write_sdes_end();

        void write_bye(uint32_t src);
        void write_bye_src(uint32_t src);
        void write_bye_reason(const char* reason);

        // Functions to get data out of the packet
        void read_header(rtcp_header&);
        void read_sender_report(sender_report&);
        void read_report_block(report_block&);

        uint32_t read_ssrc();
    };
}