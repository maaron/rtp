
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
        header->length = htons(ntohs(header->length) + bytes / 4);
    }

    void rtcp_packet::increment_RC()
    {
        set_RC(get_RC() + 1);
    }

    size_t rtcp_packet::size()
    {
        return (ntohs(header->length) + 1) * sizeof(uint32_t);
    }

    void rtcp_packet::write_header()
    {
        if (write_ptr >= end_ptr) return;

        header = (rtcp_header*)write_ptr;

        set_V(2);
        set_RC(0);
        set_P(0);
        header->length = 0;

        write_ptr += sizeof(rtcp_header);
    }

    void rtcp_packet::write_sender_info(uint32_t ssrc, uint64_t ntp_time, uint32_t rtp_time, uint32_t octet_count, uint32_t packet_count)
    {
        if (write_ptr >= end_ptr) return;

        set_PT(RTCP_SR);

        auto sr = (sender_report*)(write_ptr);
        sr->ssrc = htonl(ssrc);
        sr->ntp_lsw = htonl((uint32_t)ntp_time);
        sr->ntp_msw = htonl((uint32_t)(ntp_time >> 32));
        sr->rtp_timestamp = htonl(rtp_time);
        sr->octet_count = htonl(octet_count);
        sr->packet_count = htonl(packet_count);

        add_bytes(sizeof(sender_report));
    }

    void rtcp_packet::write_sender_block(uint32_t ssrc, uint8_t fraction_lost, uint32_t packets_lost, uint32_t highest_sequence, uint32_t jitter, uint32_t last_sr, uint32_t delay_last_sr)
    {
        if (write_ptr >= end_ptr) return;

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
}