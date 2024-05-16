#ifndef ARM_CHI_MONITOR_H
#define ARM_CHI_MONITOR_H

#include <map>

#include <ARM/TLM/arm_chi.h>

class CHIMonitor : public sc_core::sc_module
{
protected:
    SC_HAS_PROCESS(CHIMonitor);

    unsigned data_width_bytes;

    /* Map of burst counts for observed Payloads. */
    std::map<ARM::CHI::Payload*, unsigned> payload_burst_index;

    tlm::tlm_sync_enum nb_transport_fw(ARM::CHI::Payload& payload,
        ARM::CHI::Phase& phase);
    tlm::tlm_sync_enum nb_transport_bw(ARM::CHI::Payload& payload,
        ARM::CHI::Phase& phase);

    void print_payload(bool fw, const ARM::CHI::Payload& payload, const ARM::CHI::Phase& phase);

public:
    CHIMonitor(const sc_core::sc_module_name& name, unsigned data_width_bits = 128);

    ARM::CHI::SimpleTargetSocket<CHIMonitor> target;
    ARM::CHI::SimpleInitiatorSocket<CHIMonitor> initiator;
};

#endif // ARM_CHI_MONITOR_H
