#include <string>
#include <sstream>
#include <iostream>
#include <iomanip>

#include "Monitor.h"

using namespace std;
using namespace tlm;
using namespace sc_core;
using namespace ARM::AXI4;

tlm_sync_enum Monitor::nb_transport_fw(Payload& payload, Phase& phase)
{
    Phase prev_phase = phase;
    tlm_sync_enum reply = master.nb_transport_fw(payload, phase);

    print_payload(payload, prev_phase, reply, phase);

    return reply;
}

tlm_sync_enum Monitor::nb_transport_bw(Payload& payload, Phase& phase)
{
    Phase prev_phase = phase;
    tlm_sync_enum reply = slave.nb_transport_bw(payload, phase);

    print_payload(payload, prev_phase, reply, phase);

    return reply;
}

void Monitor::print_payload(Payload& payload, Phase phase,
    tlm_sync_enum reply, Phase reply_phase)
{
    ostringstream stream;

    Command command = payload.get_command();

    const char* phase_name = "?";
    bool show_addr = true;
    bool show_data = false;
    bool show_resp = false;
    bool inc_beat = false;
    bool first_beat = false;
    bool last_beat = false;

    bool updated = reply == TLM_UPDATED;

    switch (phase)
    {
    case PHASE_UNINITIALIZED:
        phase_name = "PHASE_UNINITIALIZED";
        show_addr = false;
        break;
    case AW_VALID:
        phase_name = (updated ? "AW VALID READY" : "AW VALID -----");
        break;
    case AW_READY:
        phase_name = "AW ----- READY";
        break;
    case W_VALID:
    case W_VALID_LAST:
        phase_name = (updated ? "W  VALID READY" : "W  VALID -----");
        inc_beat = updated;
        show_data = true;
        first_beat = true;
        last_beat = updated;
        break;
    case W_READY:
        inc_beat = true;
        phase_name = "W  ----- READY";
        show_data = true;
        last_beat = true;
        break;
    case B_VALID:
        phase_name = (updated ? "B  VALID READY" : "B  VALID -----");
        show_resp = true;
        last_beat = updated;
        break;
    case B_READY:
        phase_name = "B  ----- READY";
        show_resp = true;
        last_beat = true;
        break;
    case AR_VALID:
        phase_name = (updated ? "AR VALID READY" : "AR VALID -----");
        break;
    case AR_READY:
        phase_name = "AR ----- READY";
        break;
    case R_VALID:
    case R_VALID_LAST:
        phase_name = (updated ? "R  VALID READY" : "R  VALID -----");
        inc_beat = updated;
        show_data = true;
        show_resp = true;
        first_beat = true;
        last_beat = updated;
        break;
    case R_READY:
        inc_beat = true;
        phase_name = "R  ----- READY";
        show_data = true;
        show_resp = true;
        last_beat = true;
        break;
    case AC_VALID:
        phase_name = (updated ? "AC VALID READY" : "AC VALID -----");
        break;
    case AC_READY:
        phase_name = "AC ----- READY";
        break;
    case CR_VALID:
        phase_name = (updated ? "CR VALID READY" : "CR VALID -----");
        break;
    case CR_READY:
        phase_name = "CR ----- READY";
        break;
    case CD_VALID:
    case CD_VALID_LAST:
        phase_name = (updated ? "CD VALID READY" : "CD VALID -----");
        inc_beat = updated;
        show_data = true;
        first_beat = true;
        last_beat = updated;
        break;
    case CD_READY:
        inc_beat = true;
        phase_name = "CD ----- READY";
        show_data = true;
        last_beat = true;
        break;
    case WACK:
        phase_name = "WACK";
        show_addr = false;
        break;
    case RACK:
        phase_name = "RACK";
        show_addr = false;
        break;
    default:
        show_addr = false;
        break;
    }

    /* Remember which beat we're up to in observed payloads. */
    if (first_beat && payload_burst_index.find(&payload) ==
        payload_burst_index.end())
    {
        payload_burst_index[&payload] = 0;
    }

    stream << sc_core::sc_time_stamp() << ' ' << name() << ": "
        << phase_name << ' ';

    if (show_addr)
    {
        stream << "@" << setw(12) << setfill('0') << hex
            << payload.get_address() << dec << ' ';

        if (command != COMMAND_SNOOP)
        {
            const static char* burst_types[] = { "FIXED", "INCR ", "WRAP " };
            Burst burst = payload.get_burst();

            stream << payload.get_beat_count() << "x" <<
                (8 * (1 << payload.get_size())) << "bits ";
            stream << (burst <= BURST_WRAP ? burst_types[burst] : "?????")
                << ' ';
        }
    }

    const static char* resp_types[] =
        { "OKAY  ", "EXOKAY", "SLVERR", "DECERR" };
    Resp resp = payload.get_resp();

    if (show_resp)
        stream << (resp <= RESP_DECERR ? resp_types[resp] : "??????") << ' ';
    else
        stream << "       ";

    if (show_data)
    {
        unsigned burst_index = payload_burst_index[&payload];

        stream << (burst_index == payload.get_len() ? "LAST " : "     ")
            << "DATA:";

        uint64_t byte_strobe(uint64_t(~0));

        switch (payload.get_command())
        {
        case COMMAND_WRITE:
            payload.write_out_beat(burst_index, beat_data);
            byte_strobe = payload.write_out_beat_strobe(burst_index);
            break;
        case COMMAND_READ:
            payload.read_out_beat(burst_index, beat_data);
            break;
        case COMMAND_SNOOP:
            payload.snoop_out_beat(burst_index, beat_data);
            break;
        default:
            assert(0);
            break;
        }

        stream << uppercase << hex;
        unsigned size = 1 << payload.get_size();
        for (int i = size - 1; i >= 0; i--)
        {
            if ((byte_strobe >> (i % 8)) & 1)
                stream << setw(2) << setfill('0') << unsigned(beat_data[i]);
            else
                stream << "XX";
            if (i != 0 && !(i % 8))
                stream << "_";
        }
        stream << dec;

        /* Increment beat index on data valid. */
        if (inc_beat)
            payload_burst_index[&payload] = burst_index + 1;
    }

    /*
     * Drop the payload_burst_index for this payload once all data has been
     * shown.
     * */
    if (last_beat && payload_burst_index[&payload] == payload.get_beat_count())
        payload_burst_index.erase(&payload);

    stream << '\n';
    cout << stream.str();
}

Monitor::Monitor(sc_module_name name, unsigned port_width) :
    sc_module(name),
    beat_data(new uint8_t[port_width >> 3]),
    slave("slave", *this, &Monitor::nb_transport_fw, ARM::TLM::PROTOCOL_ACE,
        port_width),
    master("master", *this, &Monitor::nb_transport_bw, ARM::TLM::PROTOCOL_ACE,
        port_width)
{}

Monitor::~Monitor()
{
    delete[] beat_data;
}

