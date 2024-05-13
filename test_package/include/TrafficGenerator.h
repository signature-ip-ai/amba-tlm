#ifndef ARM_TRAFFICGENERATOR_H
#define ARM_TRAFFICGENERATOR_H

#include <deque>

#include <ARM/TLM/arm_axi4.h>

class TrafficGenerator : public sc_core::sc_module
{
protected:
    SC_HAS_PROCESS(TrafficGenerator);

    enum ChannelState
    {
        CLEAR,
        REQ,
        ACK
    };

    /* Outgoing queues per channel. */
    std::deque<ARM::AXI4::Payload*> aw_queue;
    std::deque<ARM::AXI4::Payload*> w_queue;
    std::deque<ARM::AXI4::Payload*> ar_queue;
    std::deque<ARM::AXI4::Payload*> cr_queue;
    std::deque<ARM::AXI4::Payload*> wack_queue;
    std::deque<ARM::AXI4::Payload*> rack_queue;

    /* Current state of each fw channel. */
    ChannelState aw_state;
    ChannelState w_state;
    ChannelState ar_state;
    ChannelState cr_state;

    /* Incoming communications to push at posedge. */
    ARM::AXI4::Payload* cr_incoming;
    ARM::AXI4::Payload* wack_incoming;
    ARM::AXI4::Payload* rack_incoming;

    /* Number of beats (of the front Payload of w_queue) already sent. */
    unsigned w_beat_count;

    void clock_posedge();
    void clock_negedge();

    tlm::tlm_sync_enum nb_transport_bw(ARM::AXI4::Payload& payload,
        ARM::AXI4::Phase& phase);

public:
    TrafficGenerator(sc_core::sc_module_name name);

    /* Add a payload to the traffic queue. */
    void add_payload(ARM::AXI4::Command command, uint64_t address,
        ARM::AXI4::Size size, uint8_t len, ARM::AXI4::Burst burst =
        ARM::AXI4::BURST_INCR);

    ARM::AXI4::SimpleMasterSocket<TrafficGenerator> master;

    sc_core::sc_in<bool> clock;
};

#endif // ARM_TRAFFICGENERATOR_H
