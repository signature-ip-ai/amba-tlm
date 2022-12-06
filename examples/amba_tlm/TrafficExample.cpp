#include <iostream>

#include "TrafficGenerator.h"
#include "Monitor.h"
#include "Memory.h"

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
    Monitor mon("mon");
    Memory mem("mem");

    tg.clock.bind(clk);
    mem.clock.bind(clk);

    tg.master.bind(mon.slave);
    mon.master.bind(mem.slave);

    add_payloads_to_tg(tg);

    sc_start(2000, SC_NS);

    Payload::debug_payload_pool(cout);

    return 0;
}

