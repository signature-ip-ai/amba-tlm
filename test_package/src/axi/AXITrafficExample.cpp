#include <iostream>

#include "AXITrafficGenerator.h"
#include "AXIMonitor.h"
#include "AXIMemory.h"

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
    AXIMonitor mon("mon");
    AXIMemory mem("mem");

    tg.clock.bind(clk);
    mem.clock.bind(clk);

    tg.initiator.bind(mon.target);
    mon.initiator.bind(mem.target);

    add_payloads_to_tg(tg);

    sc_core::sc_start(2000, sc_core::SC_NS);

    ARM::AXI::Payload::debug_payload_pool(std::cout);

    return 0;
}

