#include <cstring>

#include "Memory.h"

using namespace std;
using namespace tlm;
using namespace sc_core;
using namespace ARM::AXI4;

void Memory::clock_posedge()
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

        b_outgoing->set_resp(RESP_OKAY);

        /* Write write data into backing store all in one go. */
        uint64_t addr = b_outgoing->get_base_address();
        sc_assert(addr + b_outgoing->get_data_length() <= MEMORY_SIZE);
        b_outgoing->write_out(&mem_data[addr]);

        /* Unref for the w_queue beat ref call. */
        b_outgoing->unref();
    }
}

void Memory::clock_negedge()
{
    if (r_state == CLEAR && r_outgoing)
    {
        Phase phase = R_VALID;

        r_beat_count--;
        if (r_beat_count == 0)
            phase = R_VALID_LAST;

        r_state = REQ;
        tlm_sync_enum reply = slave.nb_transport_bw(*r_outgoing, phase);
        if (reply == TLM_UPDATED)
        {
            sc_assert(phase == R_READY);
            r_state = ACK;
        }

        /* Unref for the ar_queue.push_back() ref. */
        if (r_beat_count == 0)
        {
            r_outgoing->unref();
            r_outgoing = NULL;
        }
    }

    if (b_outgoing)
    {
        Phase phase = B_VALID;

        b_state = REQ;
        tlm_sync_enum reply = slave.nb_transport_bw(*b_outgoing, phase);
        if (reply == TLM_UPDATED)
        {
            sc_assert(phase == B_READY);
            b_state = ACK;
        }

        /* Unref for a aw_queue.push_back() ref. */
        b_outgoing->unref();
        b_outgoing = NULL;
    }
}

tlm_sync_enum Memory::nb_transport_fw(Payload& payload, Phase& phase)
{
    switch (phase)
    {
    case AW_VALID:
        aw_queue.push_back(&payload);
        payload.ref();
        phase = AW_READY;
        return TLM_UPDATED;
    case W_VALID_LAST:
        w_queue.push_back(&payload);
        payload.ref();
        phase = W_READY;
        return TLM_UPDATED;
    case W_VALID:
        phase = W_READY;
        return TLM_UPDATED;
    case B_READY:
        b_state = ACK;
        return TLM_ACCEPTED;
    case AR_VALID:
        ar_queue.push_back(&payload);
        payload.ref();
        phase = AR_READY;
        return TLM_UPDATED;
    case R_READY:
        r_state = ACK;
        return TLM_ACCEPTED;
    case RACK:
    case WACK:
        return TLM_ACCEPTED;
    default:
        sc_assert(!"Unrecognised phase");
        return TLM_ACCEPTED;
    }
}

Memory::Memory(sc_module_name name) :
    sc_module(name),
    b_state(CLEAR),
    r_state(CLEAR),
    b_outgoing(NULL),
    r_outgoing(NULL),
    slave("slave", *this, &Memory::nb_transport_fw,
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

