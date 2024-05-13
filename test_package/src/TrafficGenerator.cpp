#include <cstring>

#include "TrafficGenerator.h"

using namespace std;
using namespace sc_core;
using namespace tlm;
using namespace ARM::AXI4;

void TrafficGenerator::clock_posedge()
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
        cr_incoming = NULL;
    }

    if (rack_incoming)
    {
        rack_queue.push_back(rack_incoming);
        rack_incoming = NULL;
    }

    if (wack_incoming)
    {
        wack_queue.push_back(wack_incoming);
        wack_incoming = NULL;
    }
}

void TrafficGenerator::clock_negedge()
{
    /* Send next payload AWVALID */
    if (aw_state == CLEAR && !aw_queue.empty())
    {
        Payload* payload = aw_queue.front();
        Phase phase = AW_VALID;

        w_queue.push_back(payload);
        aw_queue.pop_front();

        aw_state = REQ;
        tlm_sync_enum reply = master.nb_transport_fw(*payload, phase);
        if (reply == TLM_UPDATED)
        {
            sc_assert(phase == AW_READY);
            aw_state = ACK;
        }
    }

    /* Send next payload ARVALID */
    if (ar_state == CLEAR && !ar_queue.empty())
    {
        Payload* payload = ar_queue.front();
        Phase phase = AR_VALID;

        ar_queue.pop_front();

        ar_state = REQ;
        tlm_sync_enum reply = master.nb_transport_fw(*payload, phase);
        if (reply == TLM_UPDATED)
        {
            sc_assert(phase == AR_READY);
            ar_state = ACK;
        }
    }

    /* Send write beat WVALID */
    if (w_state == CLEAR && !w_queue.empty())
    {
        Payload* payload = w_queue.front();
        Phase phase = W_VALID;

        /* Example beat data using the beat count as data */
        uint8_t data_beat[128 / 8];
        memset(data_beat, w_beat_count, 128 / 8);
        w_beat_count++;
        payload->write_in_beat(data_beat);

        if (w_beat_count == payload->get_beat_count())
        {
            phase = W_VALID_LAST;
            w_queue.pop_front();
            w_beat_count = 0;
        }

        w_state = REQ;
        tlm_sync_enum reply = master.nb_transport_fw(*payload, phase);
        if (reply == TLM_UPDATED)
        {
            sc_assert(phase == W_READY);
            w_state = ACK;
        }
    }

    /* Send CRVALID response but no snoop data */
    if (cr_state == CLEAR && !cr_queue.empty())
    {
        Phase phase = CR_VALID;
        Payload* payload = cr_queue.front();

        cr_queue.pop_front();

        cr_state = REQ;
        tlm_sync_enum reply = master.nb_transport_fw(*payload, phase);
        if (reply == TLM_UPDATED)
        {
            sc_assert(phase == CR_READY);
            cr_state = ACK;
            payload->unref();
        }
    }

    /* Send WACK */
    if (!wack_queue.empty())
    {
        Phase phase = WACK;
        Payload* payload = wack_queue.front();

        wack_queue.pop_front();

        tlm_sync_enum reply = master.nb_transport_fw(*payload, phase);
        sc_assert(reply == TLM_ACCEPTED);

        payload->unref();
    }

    /* Send RACK */
    if (!rack_queue.empty())
    {
        Phase phase = RACK;
        Payload* payload = rack_queue.front();

        rack_queue.pop_front();

        tlm_sync_enum reply = master.nb_transport_fw(*payload, phase);
        sc_assert(reply == TLM_ACCEPTED);

        payload->unref();
    }
}

tlm_sync_enum TrafficGenerator::nb_transport_bw(Payload& payload,
    Phase& phase)
{
    switch (phase)
    {
    case AW_READY:
        aw_state = ACK;
        return TLM_ACCEPTED;
    case W_READY:
        w_state = ACK;
        return TLM_ACCEPTED;
    case B_VALID:
        wack_incoming = &payload;
        phase = B_READY;
        return TLM_UPDATED;
    case AR_READY:
        ar_state = ACK;
        return TLM_ACCEPTED;
    case R_VALID_LAST:
        /* Move to RACK queue after last beat. */
        rack_incoming = &payload;
    /* Fall through */
    case R_VALID:
        phase = R_READY;
        return TLM_UPDATED;
    case AC_VALID:
        cr_incoming = &payload;
        /* Keep the AC payload to use with CR response. */
        payload.ref();
        phase = AC_READY;
        return TLM_UPDATED;
    case CR_READY:
        cr_state = ACK;
        payload.unref();
        return TLM_ACCEPTED;
    default:
        sc_assert(!"Unrecognised phase");
        return TLM_ACCEPTED;
    }
}

TrafficGenerator::TrafficGenerator(sc_core::sc_module_name name) :
    sc_module(name),
    aw_state(CLEAR),
    w_state(CLEAR),
    ar_state(CLEAR),
    cr_state(CLEAR),
    cr_incoming(NULL),
    wack_incoming(NULL),
    rack_incoming(NULL),
    w_beat_count(0),
    master("master", *this, &TrafficGenerator::nb_transport_bw,
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

void TrafficGenerator::add_payload(Command command, uint64_t address, Size size,
    uint8_t len, Burst burst)
{
    Payload* payload = Payload::new_payload(command, address, size, len, burst);

    payload->cache = CacheBitEnum() | CACHE_AW_B;

    switch (payload->get_command())
    {
    case COMMAND_WRITE:
        aw_queue.push_back(payload);
        break;
    case COMMAND_READ:
        ar_queue.push_back(payload);
        break;
    default:
        sc_assert(!"Can only generate read and write traffic");
    }
}
