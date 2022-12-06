#ifndef ARM_MEMORY_H
#define ARM_MEMORY_H

#include <map>
#include <stdint.h>

#include <ARM/TLM/arm_axi4.h>

#define MEMORY_SIZE (0x10000)

class Memory : public sc_core::sc_module
{
protected:
    SC_HAS_PROCESS(Memory);

    enum ChannelState
    {
        CLEAR,
        REQ,
        ACK
    };

    /* Backing store for memory. */
    uint8_t mem_data[MEMORY_SIZE];

    /* Incoming queues per channel. */
    std::deque<ARM::AXI4::Payload*> aw_queue;
    std::deque<ARM::AXI4::Payload*> w_queue;
    std::deque<ARM::AXI4::Payload*> ar_queue;

    /* Current state of each bw channel. */
    ChannelState b_state;
    ChannelState r_state;

    /* Outgoing communications to send at negedge. */
    ARM::AXI4::Payload* b_outgoing;
    ARM::AXI4::Payload* r_outgoing;

    /* Number of beats (of r_outgoing) already sent. */
    unsigned r_beat_count;

    void clock_posedge();
    void clock_negedge();

    tlm::tlm_sync_enum nb_transport_fw(ARM::AXI4::Payload& payload,
        ARM::AXI4::Phase& phase);

public:
    Memory(sc_core::sc_module_name name);

    ARM::AXI4::SimpleSlaveSocket<Memory> slave;

    sc_core::sc_in<bool> clock;
};

#endif // ARM_MEMORY_H

