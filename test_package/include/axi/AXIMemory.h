#ifndef ARM_AXI_MEMORY_H
#define ARM_AXI_MEMORY_H

#include <map>
#include <stdint.h>

#include <ARM/TLM/arm_axi4.h>

#define MEMORY_SIZE (0x10000)

class AXIMemory : public sc_core::sc_module
{
protected:
    SC_HAS_PROCESS(AXIMemory);

    enum ChannelState
    {
        CLEAR,
        REQ,
        ACK
    };

    /* Backing store for memory. */
    uint8_t mem_data[MEMORY_SIZE];

    /* Incoming queues per channel. */
    std::deque<ARM::AXI::Payload*> aw_queue;
    std::deque<ARM::AXI::Payload*> w_queue;
    std::deque<ARM::AXI::Payload*> ar_queue;

    /* Current state of each bw channel. */
    ChannelState b_state;
    ChannelState r_state;

    /* Outgoing communications to send at negedge. */
    ARM::AXI::Payload* b_outgoing;
    ARM::AXI::Payload* r_outgoing;

    /* Number of beats (of r_outgoing) already sent. */
    unsigned r_beat_count;

    void clock_posedge();
    void clock_negedge();

    tlm::tlm_sync_enum nb_transport_fw(ARM::AXI::Payload& payload,
        ARM::AXI::Phase& phase);

public:
    explicit AXIMemory(sc_core::sc_module_name name);

    ARM::AXI::SimpleTargetSocket<AXIMemory> target;

    sc_core::sc_in<bool> clock;
};

#endif /* ARM_AXI_MEMORY_H */
