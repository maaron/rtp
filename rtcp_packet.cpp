
#include "stdafx.h"
#include <WinSock2.h>
#include "rtcp_packet.h"
#include "bits.h"

namespace media
{
    rtcp_packet::rtcp_packet(void* data, size_t size)
        : data_ptr((char*)data), header((rtcp_header*)data),
        write_ptr((char*)data),
        end_ptr((char*)data + size)
    {
    }

    int rtcp_packet::get_V() { return bf_get(header->V_P_RC, 6, 2); }
    bool rtcp_packet::get_P() { return bf_get(header->V_P_RC, 5, 1); }
    int rtcp_packet::get_RC() { return bf_get(header->V_P_RC, 0, 5); }
    uint8_t rtcp_packet::get_PT() { return header->PT; }

    void rtcp_packet::set_V(int v) { bf_set(header->V_P_RC, v, 6, 2); }
    void rtcp_packet::set_P(bool p) { bf_set(header->V_P_RC, p, 5, 1); }
    void rtcp_packet::set_RC(int rc) { bf_set(header->V_P_RC, rc, 0, 5); }
    void rtcp_packet::set_PT(uint8_t pt) { header->PT = pt; }

    void rtcp_packet::add_bytes(int bytes)
    {
        write_ptr += bytes;
        header->length = htons((packet_bytes_written() - sizeof(header)) / 4);
    }

    void rtcp_packet::increment_RC()
    {
        set_RC(get_RC() + 1);
    }

    size_t rtcp_packet::compound_size()
    {
        return write_ptr - data_ptr;
    }

    size_t rtcp_packet::packet_size()
    {
        return (ntohs(header->length) + 1) * sizeof(uint32_t);
    }

    bool rtcp_packet::write_header(int pt)
    {
        if (write_ptr + sizeof(rtcp_header) >= end_ptr) return false;

        header = (rtcp_header*)write_ptr;

        header->V_P_RC = 0x80;
        header->PT = pt;
        header->length = 0;

        write_ptr += sizeof(rtcp_header);

        return true;
    }

    void rtcp_packet::write_sender_report(
        uint32_t ssrc, 
        uint64_t ntp_time, 
        uint32_t rtp_time, 
        uint32_t octet_count, 
        uint32_t packet_count)
    {
        if (write_ptr + sizeof(sender_report) >= end_ptr) return;

        if (!write_header(RTCP_SR)) return;

        auto sr = (sender_report*)(write_ptr);
        sr->ssrc = htonl(ssrc);
        sr->ntp_lsw = htonl((uint32_t)ntp_time);
        sr->ntp_msw = htonl((uint32_t)(ntp_time >> 32));
        sr->rtp_timestamp = htonl(rtp_time);
        sr->octet_count = htonl(octet_count);
        sr->packet_count = htonl(packet_count);

        add_bytes(sizeof(sender_report));
    }

    void rtcp_packet::write_sender_report_block(
        uint32_t ssrc, 
        uint8_t fraction_lost, uint32_t packets_lost, 
        uint32_t highest_sequence, 
        uint32_t jitter, 
        uint32_t last_sr, 
        uint32_t delay_last_sr)
    {
        if (write_ptr + sizeof(report_block) >= end_ptr) return;

        auto block = (report_block*)write_ptr;
        block->ssrc = ssrc;
        block->fraction_cumulative_lost = (fraction_lost << 24) | (packets_lost & 0x00ffffff);
        block->extended_seq_received = highest_sequence;
        block->interarrival_jitter = jitter;
        block->last_sr = last_sr;
        block->delay_since_last_sr = delay_last_sr;

        increment_RC();
        add_bytes(sizeof(report_block));
    }

    void rtcp_packet::write_sdes(uint32_t src)
    {
        if (!write_header(RTCP_SDES)) return;

        if (write_ptr + sizeof(src) >= end_ptr) return;

        increment_RC();

        *(uint32_t*)write_ptr = htonl(src);

        add_bytes(sizeof(src));
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
        if (!write_header(RTCP_BYE)) return;
        
        if (write_ptr + sizeof(src) >= end_ptr) return;

        increment_RC();
        
        *(uint32_t*)write_ptr = htonl(src);

        add_bytes(sizeof(src));
    }

    void rtcp_packet::write_bye_src(uint32_t src)
    {
        if (write_ptr + sizeof(src) >= end_ptr) return;

        increment_RC();
        
        *(uint32_t*)write_ptr = htonl(src);

        add_bytes(sizeof(src));
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
}