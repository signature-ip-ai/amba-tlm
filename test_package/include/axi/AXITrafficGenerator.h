#ifndef ARM_AXI_TRAFFICGENERATOR_H
#define ARM_AXI_TRAFFICGENERATOR_H

#include <deque>

#include <ARM/TLM/arm_axi4.h>

class AXITrafficGenerator : public sc_core::sc_module
{
protected:
    SC_HAS_PROCESS(AXITrafficGenerator);

    enum ChannelState
    {
        CLEAR,
        REQ,
        ACK
    };

    /* Outgoing queues per channel. */
    std::deque<ARM::AXI::Payload*> aw_queue;
    std::deque<ARM::AXI::Payload*> w_queue;
    std::deque<ARM::AXI::Payload*> ar_queue;
    std::deque<ARM::AXI::Payload*> cr_queue;
    std::deque<ARM::AXI::Payload*> wack_queue;
    std::deque<ARM::AXI::Payload*> rack_queue;

    /* Current state of each fw channel. */
    ChannelState aw_state;
    ChannelState w_state;
    ChannelState ar_state;
    ChannelState cr_state;

    /* Incoming communications to push at posedge. */
    ARM::AXI::Payload* cr_incoming;
    ARM::AXI::Payload* wack_incoming;
    ARM::AXI::Payload* rack_incoming;

    /* Number of beats (of the front Payload of w_queue) already sent. */
    unsigned w_beat_count;

    void clock_posedge();
    void clock_negedge();

    tlm::tlm_sync_enum nb_transport_bw(ARM::AXI::Payload& payload,
        ARM::AXI::Phase& phase);

public:
    explicit AXITrafficGenerator(sc_core::sc_module_name name);

    /* Add a payload to the traffic queue. */
    void add_payload(ARM::AXI::Command command, uint64_t address,
        ARM::AXI::Size size, uint8_t len, ARM::AXI::Burst burst =
        ARM::AXI::BURST_INCR);

    ARM::AXI::SimpleInitiatorSocket<AXITrafficGenerator> initiator;

    sc_core::sc_in<bool> clock;
};

#endif /* ARM_AXI_TRAFFICGENERATOR_H */
