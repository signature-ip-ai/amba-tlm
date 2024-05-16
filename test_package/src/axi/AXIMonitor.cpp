#include <string>
#include <sstream>
#include <iostream>
#include <iomanip>

#include "AXIMonitor.h"

tlm::tlm_sync_enum AXIMonitor::nb_transport_fw(ARM::AXI::Payload& payload, ARM::AXI::Phase& phase)
{
    ARM::AXI::Phase prev_phase = phase;
    tlm::tlm_sync_enum reply = initiator.nb_transport_fw(payload, phase);

    print_payload(payload, prev_phase, reply, phase);

    return reply;
}

tlm::tlm_sync_enum AXIMonitor::nb_transport_bw(ARM::AXI::Payload& payload, ARM::AXI::Phase& phase)
{
    ARM::AXI::Phase prev_phase = phase;
    tlm::tlm_sync_enum reply = target.nb_transport_bw(payload, phase);

    print_payload(payload, prev_phase, reply, phase);

    return reply;
}

void AXIMonitor::print_payload(ARM::AXI::Payload& payload, ARM::AXI::Phase phase,
    tlm::tlm_sync_enum reply, ARM::AXI::Phase /* reply_phase */)
{
    std::ostringstream stream;

    ARM::AXI::Command command = payload.get_command();

    const char* phase_name = "?";
    bool show_addr = true;
    bool show_data = false;
    bool show_resp = false;
    bool inc_beat = false;
    bool first_beat = false;
    bool last_beat = false;

    bool updated = reply == tlm::TLM_UPDATED;

    switch (phase)
    {
    case ARM::AXI::PHASE_UNINITIALIZED:
        phase_name = "PHASE_UNINITIALIZED";
        show_addr = false;
        break;
    case ARM::AXI::AW_VALID:
        phase_name = (updated ? "AW VALID READY" : "AW VALID -----");
        break;
    case ARM::AXI::AW_READY:
        phase_name = "AW ----- READY";
        break;
    case ARM::AXI::W_VALID:
    case ARM::AXI::W_VALID_LAST:
        phase_name = (updated ? "W  VALID READY" : "W  VALID -----");
        inc_beat = updated;
        show_data = true;
        first_beat = true;
        last_beat = updated;
        break;
    case ARM::AXI::W_READY:
        inc_beat = true;
        phase_name = "W  ----- READY";
        show_data = true;
        last_beat = true;
        break;
    case ARM::AXI::B_VALID:
        phase_name = (updated ? "B  VALID READY" : "B  VALID -----");
        show_resp = true;
        last_beat = updated;
        break;
    case ARM::AXI::B_READY:
        phase_name = "B  ----- READY";
        show_resp = true;
        last_beat = true;
        break;
    case ARM::AXI::AR_VALID:
        phase_name = (updated ? "AR VALID READY" : "AR VALID -----");
        break;
    case ARM::AXI::AR_READY:
        phase_name = "AR ----- READY";
        break;
    case ARM::AXI::R_VALID:
    case ARM::AXI::R_VALID_LAST:
        phase_name = (updated ? "R  VALID READY" : "R  VALID -----");
        inc_beat = updated;
        show_data = true;
        show_resp = true;
        first_beat = true;
        last_beat = updated;
        break;
    case ARM::AXI::R_READY:
        inc_beat = true;
        phase_name = "R  ----- READY";
        show_data = true;
        show_resp = true;
        last_beat = true;
        break;
    case ARM::AXI::AC_VALID:
        phase_name = (updated ? "AC VALID READY" : "AC VALID -----");
        break;
    case ARM::AXI::AC_READY:
        phase_name = "AC ----- READY";
        break;
    case ARM::AXI::CR_VALID:
        phase_name = (updated ? "CR VALID READY" : "CR VALID -----");
        break;
    case ARM::AXI::CR_READY:
        phase_name = "CR ----- READY";
        break;
    case ARM::AXI::CD_VALID:
    case ARM::AXI::CD_VALID_LAST:
        phase_name = (updated ? "CD VALID READY" : "CD VALID -----");
        inc_beat = updated;
        show_data = true;
        first_beat = true;
        last_beat = updated;
        break;
    case ARM::AXI::CD_READY:
        inc_beat = true;
        phase_name = "CD ----- READY";
        show_data = true;
        last_beat = true;
        break;
    case ARM::AXI::WACK:
        phase_name = "WACK";
        show_addr = false;
        break;
    case ARM::AXI::RACK:
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
        stream << "@" << std::setw(12) << std::setfill('0') << std::hex
            << payload.get_address() << std::dec << ' ';

        if (command != ARM::AXI::COMMAND_SNOOP)
        {
            const static char* burst_types[] = { "FIXED", "INCR ", "WRAP " };
            ARM::AXI::Burst burst = payload.get_burst();

            stream << payload.get_beat_count() << "x" <<
                (8 * (1 << payload.get_size())) << "bits ";
            stream << (burst <= ARM::AXI::BURST_WRAP ? burst_types[burst] : "?????")
                << ' ';
        }
    }

    if (show_resp)
    {
        ARM::AXI::Resp resp = payload.get_resp();
        const static char* resp_types[] =
            { "OKAY  ", "EXOKAY", "SLVERR", "DECERR" };
        stream << (resp <= ARM::AXI::RESP_DECERR ? resp_types[resp] : "??????") << ' ';
    } else
    {
        stream << "       ";
    }

    if (show_data)
    {
        unsigned burst_index = payload_burst_index[&payload];

        stream << (burst_index == payload.get_len() ? "LAST " : "     ")
            << "DATA:";

        uint64_t byte_strobe(uint64_t(~0));

        switch (payload.get_command())
        {
        case ARM::AXI::COMMAND_WRITE:
            payload.write_out_beat(burst_index, beat_data);
            byte_strobe = payload.write_out_beat_strobe(burst_index);
            break;
        case ARM::AXI::COMMAND_READ:
            payload.read_out_beat(burst_index, beat_data);
            break;
        case ARM::AXI::COMMAND_SNOOP:
            payload.snoop_out_beat(burst_index, beat_data);
            break;
        default:
            assert(0);
            break;
        }

        stream << std::uppercase << std::hex;
        unsigned size = 1 << payload.get_size();
        for (int i = size - 1; i >= 0; i--)
        {
            if ((byte_strobe >> (i % 8)) & 1)
                stream << std::setw(2) << std::setfill('0') << unsigned(beat_data[i]);
            else
                stream << "XX";
            if (i != 0 && !(i % 8))
                stream << "_";
        }
        stream << std::dec;

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
    std::cout << stream.str();
}

AXIMonitor::AXIMonitor(sc_core::sc_module_name name, unsigned port_width) :
    sc_core::sc_module(name),
    beat_data(new uint8_t[port_width >> 3]),
    target("target", *this, &AXIMonitor::nb_transport_fw, ARM::TLM::PROTOCOL_ACE,
        port_width),
    initiator("initiator", *this, &AXIMonitor::nb_transport_bw, ARM::TLM::PROTOCOL_ACE,
        port_width)
{}

AXIMonitor::~AXIMonitor()
{
    delete[] beat_data;
}

