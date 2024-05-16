#include <cstring>

#include "CHITrafficGenerator.h"

void CHITrafficGenerator::clock_posedge()
{
    if (!channels[ARM::CHI::CHANNEL_RSP].rx_queue.empty())
    {
        const CHIFlit rsp_flit = channels[ARM::CHI::CHANNEL_RSP].rx_queue.front();
        channels[ARM::CHI::CHANNEL_RSP].rx_queue.pop_front();

        switch (rsp_flit.phase.rsp_opcode)
        {
        case ARM::CHI::RSP_OPCODE_COMP_DBID_RESP:
        case ARM::CHI::RSP_OPCODE_DBID_RESP:
        case ARM::CHI::RSP_OPCODE_DBID_RESP_ORD:
            handle_dbid_resp(rsp_flit);
            break;
        case ARM::CHI::RSP_OPCODE_COMP:
            /* ignore a separate Comp */
            break;
        default:
            SC_REPORT_ERROR(name(), "unexpected response opcode received");
        }
    }

    if (!channels[ARM::CHI::CHANNEL_DAT].rx_queue.empty())
    {
        const CHIFlit dat_flit = channels[ARM::CHI::CHANNEL_DAT].rx_queue.front();
        channels[ARM::CHI::CHANNEL_DAT].rx_queue.pop_front();

        switch (dat_flit.phase.dat_opcode)
        {
        case ARM::CHI::DAT_OPCODE_COMP_DATA:
        case ARM::CHI::DAT_OPCODE_DATA_SEP_RESP:
            /* ignore returned read data */
            break;
        default:
            SC_REPORT_ERROR(name(), "unexpected read data opcode received");
        }
    }

    /* The other channels are inactive and cannot receive flits, so no need to process them. */
}

static ARM::CHI::Phase make_write_data_phase(const ARM::CHI::Phase& dbid_phase, const ARM::CHI::DatOpcode dat_opcode)
{
    ARM::CHI::Phase dat_phase;

    dat_phase.channel = ARM::CHI::CHANNEL_DAT;

    dat_phase.qos = dbid_phase.qos;
    dat_phase.tgt_id = dbid_phase.src_id;
    dat_phase.src_id = dbid_phase.tgt_id;
    dat_phase.txn_id = dbid_phase.dbid;
    dat_phase.dat_opcode = dat_opcode;
    dat_phase.resp = ARM::CHI::RESP_I;

    return dat_phase;
}

void CHITrafficGenerator::handle_dbid_resp(const CHIFlit& dbid_flit)
{
    /* Exemplar write data, filled with TxnID.  Data can be over filled, BE indicates which bytes are really enabled. */
    dbid_flit.payload.byte_enable = transaction_valid_bytes_mask(dbid_flit.payload);
    memset(dbid_flit.payload.data, dbid_flit.phase.txn_id, CHI_CACHE_LINE_SIZE_BYTES);

    /* Generate and queue write data beats. */
    ARM::CHI::Phase dat_phase = make_write_data_phase(dbid_flit.phase, ARM::CHI::DAT_OPCODE_NON_COPY_BACK_WR_DATA);

    for (const auto data_id : transaction_data_ids(dbid_flit.payload, data_width_bytes))
    {
        dat_phase.data_id = data_id;
        channels[ARM::CHI::CHANNEL_DAT].tx_queue.emplace_back(dbid_flit.payload, dat_phase);
    }
}

void CHITrafficGenerator::clock_negedge()
{
    /* Try to issue credits and send transactions on active channels. */
    for (const auto channel : {ARM::CHI::CHANNEL_REQ, ARM::CHI::CHANNEL_RSP, ARM::CHI::CHANNEL_DAT})
    {
        channels[channel].send_flits(channel, [this](ARM::CHI::Payload& payload, ARM::CHI::Phase& phase) {
            return initiator.nb_transport_fw(payload, phase);
        });
    }
}

tlm::tlm_sync_enum CHITrafficGenerator::nb_transport_bw(ARM::CHI::Payload& payload, ARM::CHI::Phase& phase)
{
    if (!channels[phase.channel].receive_flit(payload, phase))
        SC_REPORT_ERROR(name(), "flit on inactive channel received");

    return tlm::TLM_ACCEPTED;
}

CHITrafficGenerator::CHITrafficGenerator(const sc_core::sc_module_name& name, const unsigned data_width_bits) :
    sc_module(name),
    data_width_bytes{data_width_bits / 8},
    initiator("initiator", *this, &CHITrafficGenerator::nb_transport_bw, ARM::TLM::PROTOCOL_CHI_E, data_width_bits),
    clock("clock")
{
    SC_METHOD(clock_posedge);
    sensitive << clock.pos();
    dont_initialize();

    SC_METHOD(clock_negedge);
    sensitive << clock.neg();
    dont_initialize();

    /* We will need to issue link credits to our peer so that they can ... */
    for (const auto channel : {
                 ARM::CHI::CHANNEL_RSP, /* ... send responses (e.g. DBIDResp */
                 ARM::CHI::CHANNEL_DAT, /* ... send write data (e.g. CompData) */
         })
    {
        channels[channel].rx_credits_available = CHI_MAX_LINK_CREDITS;
    }
}

void CHITrafficGenerator::add_payload(
        const ARM::CHI::ReqOpcode req_opcode, const uint64_t address, const ARM::CHI::Size size)
{
    ARM::CHI::Payload& req_payload = *ARM::CHI::Payload::new_payload();
    ARM::CHI::Phase req_phase;

    req_phase.tgt_id = 2;
    req_phase.src_id = 1;
    req_phase.txn_id = txn_id++;
    req_phase.req_opcode = req_opcode;
    req_phase.order = ARM::CHI::ORDER_NO_ORDER;

    req_payload.address = address;
    req_payload.size = size;
    req_payload.mem_attr = ARM::CHI::MEM_ATTR_NORMAL_WB_A;

    channels[ARM::CHI::CHANNEL_REQ].tx_queue.emplace_back(req_payload, req_phase);

    req_payload.unref();
}
