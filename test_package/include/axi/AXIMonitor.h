#ifndef ARM_AXI_MONITOR_H
#define ARM_AXI_MONITOR_H

#include <map>

#include <ARM/TLM/arm_axi4.h>

class AXIMonitor : public sc_core::sc_module
{
protected:
    SC_HAS_PROCESS(AXIMonitor);

    /* Beat-sized data for data printing. */
    uint8_t* beat_data;

    /* Map of burst counts for observed Payloads. */
    std::map<ARM::AXI::Payload*, unsigned> payload_burst_index;

    tlm::tlm_sync_enum nb_transport_fw(ARM::AXI::Payload& payload,
        ARM::AXI::Phase& phase);
    tlm::tlm_sync_enum nb_transport_bw(ARM::AXI::Payload& payload,
        ARM::AXI::Phase& phase);

    void print_payload(ARM::AXI::Payload& payload,
        ARM::AXI::Phase sent_phase, tlm::tlm_sync_enum reply,
        ARM::AXI::Phase reply_phase);

public:
    AXIMonitor(sc_core::sc_module_name name, unsigned port_width = 128);
    ~AXIMonitor();

    ARM::AXI::SimpleTargetSocket<AXIMonitor> target;
    ARM::AXI::SimpleInitiatorSocket<AXIMonitor> initiator;
};

#endif /* ARM_AXI_MONITOR_H */
