#include <cstring>

#include "AXIMemory.h"

void AXIMemory::clock_posedge()
{
    if (b_state == ACK)
        b_state = CLEAR;

    if (r_state == ACK)
        r_state = CLEAR;

    /* Is there a read still to perform? */
    if (!r_outgoing && !ar_queue.empty())
    {
        r_outgoing = ar_queue.front();
        ar_queue.pop_front();
        r_beat_count = r_outgoing->get_beat_count();

        /* Populate read data in one go. */
        uint64_t addr = r_outgoing->get_base_address();
        sc_assert(addr + r_outgoing->get_data_length() <= MEMORY_SIZE);
        r_outgoing->read_in(&mem_data[addr]);
    }

    /* Is there a write to respond to? */
    if (!b_outgoing && !aw_queue.empty() && !w_queue.empty())
    {
        sc_assert(aw_queue.front() == w_queue.front());
        b_outgoing = w_queue.front();
        aw_queue.pop_front();
        w_queue.pop_front();

        b_outgoing->set_resp(ARM::AXI::RESP_OKAY);

        /* Write write data into backing store all in one go. */
        uint64_t addr = b_outgoing->get_base_address();
        sc_assert(addr + b_outgoing->get_data_length() <= MEMORY_SIZE);
        b_outgoing->write_out(&mem_data[addr]);

        /* Unref for the w_queue beat ref call. */
        b_outgoing->unref();
    }
}

void AXIMemory::clock_negedge()
{
    if (r_state == CLEAR && r_outgoing)
    {
        ARM::AXI::Phase phase = ARM::AXI::R_VALID;

        r_beat_count--;
        if (r_beat_count == 0)
            phase = ARM::AXI::R_VALID_LAST;

        r_state = REQ;
        tlm::tlm_sync_enum reply = target.nb_transport_bw(*r_outgoing, phase);
        if (reply == tlm::TLM_UPDATED)
        {
            sc_assert(phase == ARM::AXI::R_READY);
            r_state = ACK;
        }

        /* Unref for the ar_queue.push_back() ref. */
        if (r_beat_count == 0)
        {
            r_outgoing->unref();
            r_outgoing = nullptr;
        }
    }

    if (b_outgoing)
    {
        ARM::AXI::Phase phase = ARM::AXI::B_VALID;

        b_state = REQ;
        tlm::tlm_sync_enum reply = target.nb_transport_bw(*b_outgoing, phase);
        if (reply == tlm::TLM_UPDATED)
        {
            sc_assert(phase == ARM::AXI::B_READY);
            b_state = ACK;
        }

        /* Unref for a aw_queue.push_back() ref. */
        b_outgoing->unref();
        b_outgoing = nullptr;
    }
}

tlm::tlm_sync_enum AXIMemory::nb_transport_fw(ARM::AXI::Payload& payload, ARM::AXI::Phase& phase)
{
    switch (phase)
    {
    case ARM::AXI::AW_VALID:
        aw_queue.push_back(&payload);
        payload.ref();
        phase = ARM::AXI::AW_READY;
        return tlm::TLM_UPDATED;
    case ARM::AXI::W_VALID_LAST:
        w_queue.push_back(&payload);
        payload.ref();
        phase = ARM::AXI::W_READY;
        return tlm::TLM_UPDATED;
    case ARM::AXI::W_VALID:
        phase = ARM::AXI::W_READY;
        return tlm::TLM_UPDATED;
    case ARM::AXI::B_READY:
        b_state = ACK;
        return tlm::TLM_ACCEPTED;
    case ARM::AXI::AR_VALID:
        ar_queue.push_back(&payload);
        payload.ref();
        phase = ARM::AXI::AR_READY;
        return tlm::TLM_UPDATED;
    case ARM::AXI::R_READY:
        r_state = ACK;
        return tlm::TLM_ACCEPTED;
    case ARM::AXI::RACK:
    case ARM::AXI::WACK:
        return tlm::TLM_ACCEPTED;
    default:
        SC_REPORT_ERROR(name(), "unrecognised phase");
        return tlm::TLM_ACCEPTED;
    }
}

AXIMemory::AXIMemory(sc_core::sc_module_name name) :
    sc_core::sc_module(name),
    b_state(CLEAR),
    r_state(CLEAR),
    b_outgoing(nullptr),
    r_outgoing(nullptr),
    target("target", *this, &AXIMemory::nb_transport_fw,
        ARM::TLM::PROTOCOL_ACE, 128),
    clock("clock")
{
    memset(mem_data, 0xdf, MEMORY_SIZE);

    SC_METHOD(clock_posedge);
    sensitive << clock.pos();
    dont_initialize();

    SC_METHOD(clock_negedge);
    sensitive << clock.neg();
    dont_initialize();
}

