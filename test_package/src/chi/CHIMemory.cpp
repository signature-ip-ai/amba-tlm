#include "CHIMemory.h"

#include "CHIUtilities.h"

#include <algorithm>
#include <cstdint>
#include <cstring>

void CHIMemory::clock_posedge()
{
    if (!channels[ARM::CHI::CHANNEL_REQ].rx_queue.empty())
    {
        const CHIFlit req_flit = channels[ARM::CHI::CHANNEL_REQ].rx_queue.front();
        channels[ARM::CHI::CHANNEL_REQ].rx_queue.pop_front();

        switch (req_flit.phase.req_opcode)
        {
        case ARM::CHI::REQ_OPCODE_READ_NO_SNP:
            handle_read_req(req_flit);
            break;
        case ARM::CHI::REQ_OPCODE_WRITE_NO_SNP_PTL:
        case ARM::CHI::REQ_OPCODE_WRITE_NO_SNP_FULL:
            handle_write_req(req_flit);
            break;
        default:
            SC_REPORT_ERROR(name(), "unexpected request opcode received");
        }
    }

    if (!channels[ARM::CHI::CHANNEL_RSP].rx_queue.empty())
    {
        const CHIFlit rsp_flit = channels[ARM::CHI::CHANNEL_RSP].rx_queue.front();

        switch (rsp_flit.phase.rsp_opcode)
        {
        case ARM::CHI::RSP_OPCODE_COMP_ACK:
            /* ignore the CompAck */
            break;
        default:
            SC_REPORT_ERROR(name(), "unexpected opcode response received");
        }
    }

    if (!channels[ARM::CHI::CHANNEL_DAT].rx_queue.empty())
    {
        const CHIFlit dat_flit = channels[ARM::CHI::CHANNEL_DAT].rx_queue.front();
        channels[ARM::CHI::CHANNEL_DAT].rx_queue.pop_front();

        switch (dat_flit.phase.dat_opcode)
        {
        case ARM::CHI::DAT_OPCODE_NON_COPY_BACK_WR_DATA:
        case ARM::CHI::DAT_OPCODE_NCB_WR_DATA_COMP_ACK:
        case ARM::CHI::DAT_OPCODE_WRITE_DATA_CANCEL:
            handle_write_dat(dat_flit);
            break;
        default:
            SC_REPORT_ERROR(name(), "unexpected write data opcode received");
        }
    }

    /* The other channels are inactive and cannot receive flits, so no need to process them. */
}

static ARM::CHI::Phase make_response_phase(
        const ARM::CHI::Phase& fw_phase, const ARM::CHI::RspOpcode rsp_opcode, const unsigned dbid = 0)
{
    ARM::CHI::Phase rsp_phase;

    rsp_phase.channel = ARM::CHI::CHANNEL_RSP;

    rsp_phase.qos = fw_phase.qos;
    rsp_phase.tgt_id = fw_phase.src_id;
    rsp_phase.src_id = fw_phase.tgt_id;
    rsp_phase.txn_id = fw_phase.txn_id;
    rsp_phase.home_nid = fw_phase.tgt_id;
    rsp_phase.rsp_opcode = rsp_opcode;
    rsp_phase.dbid = dbid;

    return rsp_phase;
}

static ARM::CHI::Phase make_read_data_phase(const ARM::CHI::Phase& fw_phase, const ARM::CHI::DatOpcode dat_opcode)
{
    ARM::CHI::Phase dat_phase;

    dat_phase.channel = ARM::CHI::CHANNEL_DAT;

    dat_phase.qos = fw_phase.qos;
    dat_phase.tgt_id = fw_phase.src_id;
    dat_phase.src_id = fw_phase.tgt_id;
    dat_phase.txn_id = fw_phase.txn_id;
    dat_phase.home_nid = fw_phase.tgt_id;
    dat_phase.dat_opcode = dat_opcode;
    dat_phase.resp = ARM::CHI::RESP_UC;
    dat_phase.dbid = 0;

    return dat_phase;
}

void CHIMemory::handle_read_req(const CHIFlit& req_flit)
{
    static_assert(MEMORY_SIZE % CHI_CACHE_LINE_SIZE_BYTES == 0, "");

    if (req_flit.phase.order != ARM::CHI::ORDER_NO_ORDER)
    {
        channels[ARM::CHI::CHANNEL_RSP].tx_queue.emplace_back(
                req_flit.payload, make_response_phase(req_flit.phase, ARM::CHI::RSP_OPCODE_READ_RECEIPT));
    }

    /* Fill all the response data in one go.  We don't need to be precise and can fill in the whole "cache line".  The
     * requester will pick out the bytes it needs later. */
    memcpy(req_flit.payload.data,
            mem_data + (req_flit.payload.address & CHI_CACHE_LINE_ADDRESS_MASK) % MEMORY_SIZE,
            CHI_CACHE_LINE_SIZE_BYTES);

    /* Generate and queue read data beats.  These do need to be precise. */
    ARM::CHI::Phase dat_phase = make_read_data_phase(req_flit.phase, ARM::CHI::DAT_OPCODE_COMP_DATA);

    for (auto data_id : transaction_data_ids(req_flit.payload, data_width_bytes))
    {
        dat_phase.data_id = data_id;
        channels[ARM::CHI::CHANNEL_DAT].tx_queue.emplace_back(req_flit.payload, dat_phase);
    }
}

static ARM::CHI::Phase make_tag_match_phase(const ARM::CHI::Phase& fw_phase)
{
    ARM::CHI::Phase rsp_phase;

    rsp_phase.channel = ARM::CHI::CHANNEL_RSP;

    rsp_phase.qos = fw_phase.qos;
    rsp_phase.tgt_id = fw_phase.src_id;
    rsp_phase.src_id = fw_phase.tgt_id;
    rsp_phase.rsp_opcode = ARM::CHI::RSP_OPCODE_TAG_MATCH;
    rsp_phase.resp = ARM::CHI::RESP_I; /* Fail */

    return rsp_phase;
}

void CHIMemory::handle_write_req(const CHIFlit& req_flit)
{
    channels[ARM::CHI::CHANNEL_RSP].tx_queue.emplace_back(
            req_flit.payload,
            make_response_phase(req_flit.phase, ARM::CHI::RSP_OPCODE_DBID_RESP, allocate_dbid_for_write(req_flit)));

    if (req_flit.phase.tag_op == ARM::CHI::TAG_OP_MATCH)
        channels[ARM::CHI::CHANNEL_RSP].tx_queue.emplace_back(req_flit.payload, make_tag_match_phase(req_flit.phase));

    /* Now we wait for write data beats to accumulate separately before committing the write. */
}

uint16_t CHIMemory::allocate_dbid_for_write(const CHIFlit& req_flit)
{
    auto it = std::find(write_data_beats_remaining.begin(), write_data_beats_remaining.end(), 0);

    if (it == write_data_beats_remaining.end())
        it = write_data_beats_remaining.insert(it, 0);

    const unsigned size_bytes = 1 << req_flit.payload.size;
    const unsigned beat_count = size_bytes <= data_width_bytes ? 1 : data_width_bytes / size_bytes;

    *it = beat_count;

    return it - write_data_beats_remaining.begin();
}

void CHIMemory::handle_write_dat(const CHIFlit& dat_flit)
{
    if (dat_flit.phase.dbid >= write_data_beats_remaining.size())
        SC_REPORT_ERROR(name(), "write data with invalid DBID received");

    uint8_t& data_beats_remaining = write_data_beats_remaining[dat_flit.phase.dbid];

    data_beats_remaining--;

    if (data_beats_remaining == 0)
    {
        /* All write data has been received, we can now commit the write. */

        static_assert(MEMORY_SIZE % CHI_CACHE_LINE_SIZE_BYTES == 0, "");

        const uint64_t be = dat_flit.payload.byte_enable & transaction_valid_bytes_mask(dat_flit.payload);
        uint8_t* const cache_line = mem_data + (dat_flit.payload.address & CHI_CACHE_LINE_ADDRESS_MASK) % MEMORY_SIZE;

        for (unsigned i = 0; i < CHI_CACHE_LINE_SIZE_BYTES; i++)
        {
            if ((be >> i & 1) != 0)
                cache_line[i] = dat_flit.payload.data[i];
        }

        channels[ARM::CHI::CHANNEL_RSP].tx_queue.emplace_back(
                dat_flit.payload, make_response_phase(dat_flit.phase, ARM::CHI::RSP_OPCODE_COMP));
    }
}

void CHIMemory::clock_negedge()
{
    /* Try to issue credits and send transactions on active channels. */
    for (const auto channel : {ARM::CHI::CHANNEL_REQ, ARM::CHI::CHANNEL_RSP, ARM::CHI::CHANNEL_DAT})
    {
        channels[channel].send_flits(channel, [this](ARM::CHI::Payload& payload, ARM::CHI::Phase& phase) {
            return target.nb_transport_bw(payload, phase);
        });
    }
}

tlm::tlm_sync_enum CHIMemory::nb_transport_fw(ARM::CHI::Payload& payload, ARM::CHI::Phase& phase)
{
    if (!channels[phase.channel].receive_flit(payload, phase))
        SC_REPORT_ERROR(name(), "flit on inactive channel received");

    return tlm::TLM_ACCEPTED;
}

CHIMemory::CHIMemory(const sc_core::sc_module_name& name, const unsigned data_width_bits) :
    sc_core::sc_module(name),
    write_data_beats_remaining(10),
    data_width_bytes{data_width_bits / 8},
    target("target", *this, &CHIMemory::nb_transport_fw, ARM::TLM::PROTOCOL_CHI_E, data_width_bits),
    clock("clock")
{
    memset(mem_data, 0xdf, MEMORY_SIZE);

    SC_METHOD(clock_posedge);
    sensitive << clock.pos();
    dont_initialize();

    SC_METHOD(clock_negedge);
    sensitive << clock.neg();
    dont_initialize();

    /* We will need to issue link credits to our peer so that they can ... */
    for (const auto channel : {
                 ARM::CHI::CHANNEL_REQ, /* ... send requests (e.g. ReadNoSnp) */
                 ARM::CHI::CHANNEL_RSP, /* ... send responses (e.g. CompAck) */
                 ARM::CHI::CHANNEL_DAT, /* ... send write data (e.g. NonCopyBackWrData) */
         })
    {
        channels[channel].rx_credits_available = CHI_MAX_LINK_CREDITS;
    }
}

