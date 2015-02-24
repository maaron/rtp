#include "rtp_packet.h"
#include "bits.h"

#include <assert.h>
#include <WinSock2.h>

namespace media
{
    rtp_packet::rtp_packet(void* buffer, size_t size)
        : header((rtp_header*)buffer), packet_size(size)
    {
        // Make sure size is at least large enough for the header, including 
        // CSRC list.
        assert(size >= (sizeof(rtp_header) + 4 * get_CC()));

        csrc = (uint32_t*)(header + 1);
    }
        
    int rtp_packet::get_V() { return bf_get(header->V_P_X_CC, 6, 2); }
    bool rtp_packet::get_P() { return bit_get(header->V_P_X_CC, 5); }
    bool rtp_packet::get_X() { return bit_get(header->V_P_X_CC, 4); }
    int rtp_packet::get_CC() { return bf_get(header->V_P_X_CC, 0, 4); }
    bool rtp_packet::get_M() { return bit_get(header->M_PT, 7); }
    int rtp_packet::get_PT() { return bf_get(header->M_PT, 0, 7); }
    uint16_t rtp_packet::get_sequence_number() { return ntohs(header->sequence_number); }
    int rtp_packet::get_timestamp() { return ntohl(header->timestamp); }
    int rtp_packet::get_ssrc() { return ntohl(header->ssrc); }
    int rtp_packet::get_csrc(int index)
    {
        assert(index < get_CC());
        return ntohl(csrc[index]);
    }

    void rtp_packet::set_V(int v) { bf_set(header->V_P_X_CC, v, 6, 2); }
    void rtp_packet::set_P(bool p) { bit_set(header->V_P_X_CC, 5); }
    void rtp_packet::set_X(bool x) { bit_set(header->V_P_X_CC, 4); }
    void rtp_packet::set_CC(int cc) { bf_set(header->V_P_X_CC, cc, 0, 4); }
    void rtp_packet::set_M(bool m) { bit_set(header->M_PT, 7); }
    void rtp_packet::set_PT(int pt) { bf_set(header->M_PT, pt, 0, 7); }
    void rtp_packet::set_sequence_number(uint16_t seq) { header->sequence_number = htons(seq); }
    void rtp_packet::set_timestamp(int timestamp) { header->timestamp = htonl(timestamp); }
    void rtp_packet::set_ssrc(int src) { header->ssrc = htonl(src); }
    void rtp_packet::set_csrc(int index, int src)
    {
        assert(index < get_CC());
        csrc[index] = htonl(src);
    }

    void* rtp_packet::data()
    {
        return header;
    }

    size_t rtp_packet::size()
    {
        return packet_size;
    }

    size_t rtp_packet::payload_size()
    {
        int ret = packet_size - sizeof(rtp_header) - 4 * get_CC();
        if (get_P())
        {
            char* data = (char*)header;
            ret -= data[packet_size - 1];
        }

        return ret;
    }
}