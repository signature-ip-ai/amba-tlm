#ifndef ARM_CHI_MEMORY_H
#define ARM_CHI_MEMORY_H

#include <ARM/TLM/arm_chi.h>

#include "CHIUtilities.h"

#include <cstdint>

class CHIMemory : public sc_core::sc_module
{
protected:
    SC_HAS_PROCESS(CHIMemory);

    static const size_t MEMORY_SIZE = 0x10000;

    /* Backing store for memory. */
    uint8_t mem_data[MEMORY_SIZE];

    CHIChannelState channels[CHI_NUM_CHANNELS];

    /* Track the number of write data beats remaining for in-progress writes, indexed by DBID. 0 indicates an unused
     * DBID. */
    std::vector<uint8_t> write_data_beats_remaining;

    unsigned data_width_bytes;

    void clock_posedge();
    void clock_negedge();

    void send_link_credit(ARM::CHI::Channel channel);
    void send_flit(CHIChannelState& channel);

    void handle_read_req(const CHIFlit& req_flit);
    void handle_write_req(const CHIFlit& req_flit);

    void handle_write_dat(const CHIFlit& dat_flit);

    uint16_t allocate_dbid_for_write(const CHIFlit& req_flit);

    tlm::tlm_sync_enum nb_transport_fw(ARM::CHI::Payload& payload, ARM::CHI::Phase& phase);

public:
    explicit CHIMemory(const sc_core::sc_module_name& name, unsigned data_width_bits = 128);

    ARM::CHI::SimpleTargetSocket<CHIMemory> target;

    sc_core::sc_in<bool> clock;
};

#endif // ARM_CHI_MEMORY_H
