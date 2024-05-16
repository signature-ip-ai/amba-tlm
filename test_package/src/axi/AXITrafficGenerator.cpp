#include <cstring>

#include "AXITrafficGenerator.h"

void AXITrafficGenerator::clock_posedge()
{
    if (aw_state == ACK)
        aw_state = CLEAR;

    if (w_state == ACK)
        w_state = CLEAR;

    if (ar_state == ACK)
        ar_state = CLEAR;

    if (cr_state == ACK)
        cr_state = CLEAR;

    if (cr_incoming)
    {
        cr_queue.push_back(cr_incoming);
        cr_incoming = nullptr;
    }

    if (rack_incoming)
    {
        rack_queue.push_back(rack_incoming);
        rack_incoming = nullptr;
    }

    if (wack_incoming)
    {
        wack_queue.push_back(wack_incoming);
        wack_incoming = nullptr;
    }
}

void AXITrafficGenerator::clock_negedge()
{
    /* Send next payload AWVALID */
    if (aw_state == CLEAR && !aw_queue.empty())
    {
        ARM::AXI::Payload* payload = aw_queue.front();
        ARM::AXI::Phase phase = ARM::AXI::AW_VALID;

        w_queue.push_back(payload);
        aw_queue.pop_front();

        aw_state = REQ;
        tlm::tlm_sync_enum reply = initiator.nb_transport_fw(*payload, phase);
        if (reply == tlm::TLM_UPDATED)
        {
            sc_assert(phase == ARM::AXI::AW_READY);
            aw_state = ACK;
        }
    }

    /* Send next payload ARVALID */
    if (ar_state == CLEAR && !ar_queue.empty())
    {
        ARM::AXI::Payload* payload = ar_queue.front();
        ARM::AXI::Phase phase = ARM::AXI::AR_VALID;

        ar_queue.pop_front();

        ar_state = REQ;
        tlm::tlm_sync_enum reply = initiator.nb_transport_fw(*payload, phase);
        if (reply == tlm::TLM_UPDATED)
        {
            sc_assert(phase == ARM::AXI::AR_READY);
            ar_state = ACK;
        }
    }

    /* Send write beat WVALID */
    if (w_state == CLEAR && !w_queue.empty())
    {
        ARM::AXI::Payload* payload = w_queue.front();
        ARM::AXI::Phase phase = ARM::AXI::W_VALID;

        /* Example beat data using the beat count as data */
        uint8_t data_beat[128 / 8];
        memset(data_beat, w_beat_count, 128 / 8);
        w_beat_count++;
        payload->write_in_beat(data_beat);

        if (w_beat_count == payload->get_beat_count())
        {
            phase = ARM::AXI::W_VALID_LAST;
            w_queue.pop_front();
            w_beat_count = 0;
        }

        w_state = REQ;
        tlm::tlm_sync_enum reply = initiator.nb_transport_fw(*payload, phase);
        if (reply == tlm::TLM_UPDATED)
        {
            sc_assert(phase == ARM::AXI::W_READY);
            w_state = ACK;
        }
    }

    /* Send CRVALID response but no snoop data */
    if (cr_state == CLEAR && !cr_queue.empty())
    {
        ARM::AXI::Phase phase = ARM::AXI::CR_VALID;
        ARM::AXI::Payload* payload = cr_queue.front();

        cr_queue.pop_front();

        cr_state = REQ;
        tlm::tlm_sync_enum reply = initiator.nb_transport_fw(*payload, phase);
        if (reply == tlm::TLM_UPDATED)
        {
            sc_assert(phase == ARM::AXI::CR_READY);
            cr_state = ACK;
            payload->unref();
        }
    }

    /* Send WACK */
    if (!wack_queue.empty())
    {
        ARM::AXI::Phase phase = ARM::AXI::WACK;
        ARM::AXI::Payload* payload = wack_queue.front();

        wack_queue.pop_front();

        tlm::tlm_sync_enum reply = initiator.nb_transport_fw(*payload, phase);
        sc_assert(reply == tlm::TLM_ACCEPTED);

        payload->unref();
    }

    /* Send RACK */
    if (!rack_queue.empty())
    {
        ARM::AXI::Phase phase = ARM::AXI::RACK;
        ARM::AXI::Payload* payload = rack_queue.front();

        rack_queue.pop_front();

        tlm::tlm_sync_enum reply = initiator.nb_transport_fw(*payload, phase);
        sc_assert(reply == tlm::TLM_ACCEPTED);

        payload->unref();
    }
}

tlm::tlm_sync_enum AXITrafficGenerator::nb_transport_bw(ARM::AXI::Payload& payload,
    ARM::AXI::Phase& phase)
{
    switch (phase)
    {
    case ARM::AXI::AW_READY:
        aw_state = ACK;
        return tlm::TLM_ACCEPTED;
    case ARM::AXI::W_READY:
        w_state = ACK;
        return tlm::TLM_ACCEPTED;
    case ARM::AXI::B_VALID:
        wack_incoming = &payload;
        phase = ARM::AXI::B_READY;
        return tlm::TLM_UPDATED;
    case ARM::AXI::AR_READY:
        ar_state = ACK;
        return tlm::TLM_ACCEPTED;
    case ARM::AXI::R_VALID_LAST:
        /* Move to RACK queue after last beat. */
        rack_incoming = &payload;
    /* Fall through */
    case ARM::AXI::R_VALID:
        phase = ARM::AXI::R_READY;
        return tlm::TLM_UPDATED;
    case ARM::AXI::AC_VALID:
        cr_incoming = &payload;
        /* Keep the AC payload to use with CR response. */
        payload.ref();
        phase = ARM::AXI::AC_READY;
        return tlm::TLM_UPDATED;
    case ARM::AXI::CR_READY:
        cr_state = ACK;
        payload.unref();
        return tlm::TLM_ACCEPTED;
    default:
        SC_REPORT_ERROR(name(), "unrecognised phase");
        return tlm::TLM_ACCEPTED;
    }
}

AXITrafficGenerator::AXITrafficGenerator(sc_core::sc_module_name name) :
    sc_module(name),
    aw_state(CLEAR),
    w_state(CLEAR),
    ar_state(CLEAR),
    cr_state(CLEAR),
    cr_incoming(nullptr),
    wack_incoming(nullptr),
    rack_incoming(nullptr),
    w_beat_count(0),
    initiator("initiator", *this, &AXITrafficGenerator::nb_transport_bw,
        ARM::TLM::PROTOCOL_ACE, 128),
    clock("clock")
{
    SC_METHOD(clock_posedge);
    sensitive << clock.pos();
    dont_initialize();

    SC_METHOD(clock_negedge);
    sensitive << clock.neg();
    dont_initialize();
}

void AXITrafficGenerator::add_payload(ARM::AXI::Command command, uint64_t address, ARM::AXI::Size size,
    uint8_t len, ARM::AXI::Burst burst)
{
    ARM::AXI::Payload* payload = ARM::AXI::Payload::new_payload(command, address, size, len, burst);

    payload->cache = ARM::AXI::CacheBitEnum() | ARM::AXI::CACHE_AW_B;

    switch (payload->get_command())
    {
    case ARM::AXI::COMMAND_WRITE:
        aw_queue.push_back(payload);
        break;
    case ARM::AXI::COMMAND_READ:
        ar_queue.push_back(payload);
        break;
    default: SC_REPORT_ERROR(name(), "can only generate read and write traffic");
    }
}
