#include <iostream>

#include "TrafficGenerator.h"
#include "Monitor.h"
#include "Memory.h"
#include "Transactors.h"

using namespace std;
using namespace sc_core;
using namespace ARM::AXI4;

void add_payloads_to_tg(TrafficGenerator& tg)
{
    tg.add_payload(COMMAND_READ,  0x00001000, SIZE_16, 3);
    tg.add_payload(COMMAND_WRITE, 0x00006000, SIZE_16, 3);
    tg.add_payload(COMMAND_READ,  0x00002000, SIZE_16, 3);
    tg.add_payload(COMMAND_WRITE, 0x00005000, SIZE_16, 3);
    tg.add_payload(COMMAND_READ,  0x00003000, SIZE_16, 3);
    tg.add_payload(COMMAND_WRITE, 0x00004000, SIZE_16, 3);
    tg.add_payload(COMMAND_READ,  0x00004000, SIZE_16, 3);
    tg.add_payload(COMMAND_WRITE, 0x00003000, SIZE_16, 3);
    tg.add_payload(COMMAND_READ,  0x00005000, SIZE_16, 3);
    tg.add_payload(COMMAND_WRITE, 0x00002000, SIZE_16, 3);
    tg.add_payload(COMMAND_READ,  0x00006000, SIZE_16, 3);
    tg.add_payload(COMMAND_WRITE, 0x00001000, SIZE_16, 3);
}

int sc_main(int argc, char* argv[])
{
    sc_clock clk("clk", 2, SC_NS, 0.5);

    TrafficGenerator tg("tg");
    Monitor mon1("mon1");
    TransARMToGeneric<128> a_to_g("ARM_to_generic", ARM::TLM::PROTOCOL_ACE);
    TransGenericToARM<128> g_to_a("generic_to_ARM", ARM::TLM::PROTOCOL_ACE);
    Monitor mon2("mon2");
    Memory mem("mem");

    tg.clock.bind(clk);
    mem.clock.bind(clk);
    a_to_g.clock.bind(clk);
    g_to_a.clock.bind(clk);

    tg.master.bind(mon1.slave);
    mon1.master.bind(a_to_g.slave);
    a_to_g.master.bind(g_to_a.slave);
    g_to_a.master.bind(mon2.slave);

    mon2.master.bind(mem.slave);

    add_payloads_to_tg(tg);

    sc_start(2000, SC_NS);

    Payload::debug_payload_pool(cout);

    return 0;
}
