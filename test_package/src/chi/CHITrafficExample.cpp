#include <iostream>

#include "CHITrafficGenerator.h"
#include "CHIMonitor.h"
#include "CHIMemory.h"

void add_payloads_to_tg(CHITrafficGenerator& tg)
{
    tg.add_payload(ARM::CHI::REQ_OPCODE_READ_NO_SNP,  0x00001000, ARM::CHI::SIZE_4);
    tg.add_payload(ARM::CHI::REQ_OPCODE_WRITE_NO_SNP_PTL, 0x0000600c, ARM::CHI::SIZE_4);
    tg.add_payload(ARM::CHI::REQ_OPCODE_READ_NO_SNP,  0x00002008, ARM::CHI::SIZE_8);
    tg.add_payload(ARM::CHI::REQ_OPCODE_WRITE_NO_SNP_PTL, 0x00005010, ARM::CHI::SIZE_8);
    tg.add_payload(ARM::CHI::REQ_OPCODE_READ_NO_SNP,  0x00003020, ARM::CHI::SIZE_16);
    tg.add_payload(ARM::CHI::REQ_OPCODE_WRITE_NO_SNP_PTL, 0x00004030, ARM::CHI::SIZE_16);
    tg.add_payload(ARM::CHI::REQ_OPCODE_READ_NO_SNP,  0x00004000, ARM::CHI::SIZE_32);
    tg.add_payload(ARM::CHI::REQ_OPCODE_WRITE_NO_SNP_PTL, 0x00003000, ARM::CHI::SIZE_32);
    tg.add_payload(ARM::CHI::REQ_OPCODE_READ_NO_SNP,  0x00005000, ARM::CHI::SIZE_32);
    tg.add_payload(ARM::CHI::REQ_OPCODE_WRITE_NO_SNP_PTL, 0x00002000, ARM::CHI::SIZE_32);
    tg.add_payload(ARM::CHI::REQ_OPCODE_READ_NO_SNP,  0x00006000, ARM::CHI::SIZE_64);
    tg.add_payload(ARM::CHI::REQ_OPCODE_WRITE_NO_SNP_PTL, 0x00001000, ARM::CHI::SIZE_64);
}

int sc_main(int, char**)
{
    static const unsigned data_width_bits = 256;

    sc_core::sc_clock clk("clk", 2, sc_core::SC_NS, 0.5);

    CHITrafficGenerator tg("tg", data_width_bits);
    CHIMonitor mon("mon", data_width_bits);
    CHIMemory mem("mem", data_width_bits);

    tg.clock.bind(clk);
    mem.clock.bind(clk);

    tg.initiator.bind(mon.target);
    mon.initiator.bind(mem.target);

    add_payloads_to_tg(tg);

    sc_core::sc_start(2000, sc_core::SC_NS);

    ARM::CHI::Payload::debug_payload_pool(std::cout);

    return 0;
}

