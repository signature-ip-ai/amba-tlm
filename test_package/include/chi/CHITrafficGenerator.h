#ifndef ARM_CHI_TRAFFIC_GENERATOR_H
#define ARM_CHI_TRAFFIC_GENERATOR_H

#include <ARM/TLM/arm_chi.h>

#include "CHIUtilities.h"

class CHITrafficGenerator : public sc_core::sc_module
{
protected:
    SC_HAS_PROCESS(CHITrafficGenerator);

    CHIChannelState channels[CHI_NUM_CHANNELS];
    unsigned data_width_bytes;
    uint16_t txn_id = 0;

    void clock_posedge();
    void clock_negedge();

    void handle_dbid_resp(const CHIFlit& dbid_flit);

    tlm::tlm_sync_enum nb_transport_bw(ARM::CHI::Payload& payload, ARM::CHI::Phase& phase);

public:
    explicit CHITrafficGenerator(const sc_core::sc_module_name& name, unsigned data_width_bits = 128);

    /* Add a payload to the traffic queue. */
    void add_payload(ARM::CHI::ReqOpcode req_opcode, uint64_t address, ARM::CHI::Size size);

    ARM::CHI::SimpleInitiatorSocket<CHITrafficGenerator> initiator;

    sc_core::sc_in<bool> clock;
};

#endif // ARM_CHI_TRAFFIC_GENERATOR_H
