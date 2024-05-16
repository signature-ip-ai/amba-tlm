#include <iostream>

#include "AXITrafficGenerator.h"
#include "AXIMonitor.h"
#include "AXIMemory.h"
#include "AXITransactors.h"

void add_payloads_to_tg(AXITrafficGenerator& tg)
{
    tg.add_payload(ARM::AXI::COMMAND_READ,  0x00001000, ARM::AXI::SIZE_16, 3);
    tg.add_payload(ARM::AXI::COMMAND_WRITE, 0x00006000, ARM::AXI::SIZE_16, 3);
    tg.add_payload(ARM::AXI::COMMAND_READ,  0x00002000, ARM::AXI::SIZE_16, 3);
    tg.add_payload(ARM::AXI::COMMAND_WRITE, 0x00005000, ARM::AXI::SIZE_16, 3);
    tg.add_payload(ARM::AXI::COMMAND_READ,  0x00003000, ARM::AXI::SIZE_16, 3);
    tg.add_payload(ARM::AXI::COMMAND_WRITE, 0x00004000, ARM::AXI::SIZE_16, 3);
    tg.add_payload(ARM::AXI::COMMAND_READ,  0x00004000, ARM::AXI::SIZE_16, 3);
    tg.add_payload(ARM::AXI::COMMAND_WRITE, 0x00003000, ARM::AXI::SIZE_16, 3);
    tg.add_payload(ARM::AXI::COMMAND_READ,  0x00005000, ARM::AXI::SIZE_16, 3);
    tg.add_payload(ARM::AXI::COMMAND_WRITE, 0x00002000, ARM::AXI::SIZE_16, 3);
    tg.add_payload(ARM::AXI::COMMAND_READ,  0x00006000, ARM::AXI::SIZE_16, 3);
    tg.add_payload(ARM::AXI::COMMAND_WRITE, 0x00001000, ARM::AXI::SIZE_16, 3);
}

int sc_main(int, char**)
{
    sc_core::sc_clock clk("clk", 2, sc_core::SC_NS, 0.5);

    AXITrafficGenerator tg("tg");
    AXIMonitor mon1("mon1");
    TransAXIToGeneric<128> a_to_g("ARM_to_generic", ARM::TLM::PROTOCOL_ACE);
    TransGenericToAXI<128> g_to_a("generic_to_ARM", ARM::TLM::PROTOCOL_ACE);
    AXIMonitor mon2("mon2");
    AXIMemory mem("mem");

    tg.clock.bind(clk);
    mem.clock.bind(clk);
    a_to_g.clock.bind(clk);
    g_to_a.clock.bind(clk);

    tg.initiator.bind(mon1.target);
    mon1.initiator.bind(a_to_g.target);
    a_to_g.initiator.bind(g_to_a.target);
    g_to_a.initiator.bind(mon2.target);

    mon2.initiator.bind(mem.target);

    add_payloads_to_tg(tg);

    sc_core::sc_start(2000, sc_core::SC_NS);

    ARM::AXI::Payload::debug_payload_pool(std::cout);

    return 0;
}
