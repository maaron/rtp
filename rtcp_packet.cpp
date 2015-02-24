
#include "stdafx.h"
#include <WinSock2.h>
#include "rtcp_packet.h"
#include "bits.h"

namespace media
{
    rtcp_packet::rtcp_packet(void* data, size_t size)
        : data_ptr((char*)data), header((uint32_t*)data),
        write_ptr((char*)data),
        end_ptr((char*)data + size)
    {
    }

    void* rtcp_packet::data() { return data_ptr; }

    void rtcp_packet::add_bytes(int bytes)
    {
        write_ptr += bytes;
        uint16_t* length_ptr = (uint16_t*)header + 1;
        *length_ptr = htons((packet_bytes_written() - sizeof(uint32_t)) / 4);
    }

    void rtcp_packet::increment_RC()
    {
        uint32_t h = ntohl(*header);
        int rc = bf_get(h, 24, 5);
        bf_set(h, rc + 1, 24, 5);
        *header = htonl(h);
    }

    size_t rtcp_packet::compound_size()
    {
        return write_ptr - data_ptr;
    }

    size_t rtcp_packet::packet_size()
    {
        uint16_t* length = (uint16_t*)write_ptr + 1;
        return (ntohs(*length) + 1) * sizeof(uint32_t);
    }

    void rtcp_packet::write32(uint32_t data)
    {
        *(uint32_t*)write_ptr = htonl(data);
        add_bytes(sizeof(uint32_t));
    }

    uint32_t rtcp_packet::read32()
    {
        if (write_ptr >= end_ptr) return 0;

        uint32_t value = htonl(*(uint32_t*)write_ptr);
        write_ptr += sizeof(uint32_t);
        return value;
    }

    void rtcp_packet::write_header(int pt)
    {
        header = (uint32_t*)write_ptr;
        *header = htonl(0x80000000 | (pt << 16));
        write_ptr += sizeof(uint32_t);
    }

    void rtcp_packet::write_sender_report(sender_report& sr)
    {
        write_header(RTCP_SR);

        write32(sr.ssrc);
        write32(sr.ntp_msw);
        write32(sr.ntp_lsw);
        write32(sr.rtp_timestamp);
        write32(sr.octet_count);
        write32(sr.packet_count);
    }

    void rtcp_packet::write_sender_report_block(report_block& block)
    {
        write32(block.ssrc);
        write32((block.fraction_lost << 24) | (block.cumulative_lost & 0x00ffffff));
        write32(block.extended_seq_received);
        write32(block.interarrival_jitter);
        write32(block.last_sr);
        write32(block.delay_since_last_sr);

        increment_RC();
    }

    void rtcp_packet::write_sdes(uint32_t src)
    {
        write_header(RTCP_SDES);
        write32(src);
        increment_RC();
    }

    void rtcp_packet::write_sdes_item(int id, const char* user_host)
    {
        unsigned char length = 0;
        while (user_host[length++] != 0 && length != 255);

        if (write_ptr + sizeof(sdes_item)+length >= end_ptr) return;

        auto item = (sdes_item*)write_ptr;
        item->type = id;
        item->length = length;

        memcpy(item + 1, user_host, length);

        add_bytes(sizeof(sdes_item)+length);
    }

    void rtcp_packet::write_sdes_cname(const char* cname) { write_sdes_item(SDES_CNAME, cname); }
    void rtcp_packet::write_sdes_name(const char* name) { write_sdes_item(SDES_NAME, name); }
    void rtcp_packet::write_sdes_email(const char* email) { write_sdes_item(SDES_EMAIL, email); }

    void rtcp_packet::write_sdes_end()
    {
        // Careful... this pad requires at least one byte
        int num_pad_bytes = 4 - (packet_bytes_written() & 0x03);
        
        if (write_ptr + num_pad_bytes >= end_ptr) return;

        memset(write_ptr, 0, num_pad_bytes);

        add_bytes(num_pad_bytes);
    }

    void rtcp_packet::write_bye(uint32_t src)
    {
        write_header(RTCP_BYE);
        write32(src);
        increment_RC();
    }

    void rtcp_packet::write_bye_src(uint32_t src)
    {
        write32(src);
        increment_RC();
    }

    void rtcp_packet::write_bye_reason(const char* reason)
    {
        int length = 0;
        while (reason[length++] != 0 && length < 256);
        int num_pad_bytes = (4 - (packet_bytes_written() & 0x03)) & 0x03;

        if (write_ptr + 1 + length + num_pad_bytes >= end_ptr) return;

        *write_ptr = length;
        memcpy(write_ptr + 1, reason, length);
        memset(write_ptr + 1 + length, 0, num_pad_bytes);

        add_bytes(1 + length + num_pad_bytes);
    }

    size_t rtcp_packet::packet_bytes_written()
    {
        return write_ptr - (char*)header;
    }

    bool rtcp_packet::move_next()
    {
        auto next_ptr = write_ptr + packet_size();
        if (next_ptr >= end_ptr) return false;
        else
        {
            write_ptr = next_ptr;
            return true;
        }
    }

    void rtcp_packet::read_header(rtcp_header& h)
    {
        uint32_t dword = read32();
        
        h.V = bf_get(dword, 30, 2);
        h.P = bf_get(dword, 29, 1);
        h.RC = bf_get(dword, 24, 8);
        h.PT = bf_get(dword, 16, 8);
        h.length = bf_get(dword, 0, 16);
    }

    void rtcp_packet::read_sender_report(sender_report& sr)
    {
        sr.ssrc = read32();
        sr.ntp_msw = read32();
        sr.ntp_lsw = read32();
        sr.rtp_timestamp = read32();
        sr.packet_count = read32();
        sr.octet_count = read32();
    }

    void rtcp_packet::read_report_block(report_block& block)
    {
        block.ssrc = read32();
        uint32_t lost = read32();
        block.fraction_lost = (lost >> 24) & 0x00ff;
        block.cumulative_lost = lost & 0x00ffffff;
        block.extended_seq_received = read32();
        block.interarrival_jitter = read32();
        block.last_sr = read32();
        block.delay_since_last_sr = read32();
    }

    uint32_t rtcp_packet::read_ssrc()
    {
        return read32();
    }
}