#include <string>
#include <sstream>
#include <iostream>
#include <iomanip>

#include "CHIMonitor.h"
#include "CHIUtilities.h"

tlm::tlm_sync_enum CHIMonitor::nb_transport_fw(ARM::CHI::Payload& payload, ARM::CHI::Phase& phase)
{
    initiator.nb_transport_fw(payload, phase);
    print_payload(true, payload, phase);
    return tlm::TLM_ACCEPTED;
}

tlm::tlm_sync_enum CHIMonitor::nb_transport_bw(ARM::CHI::Payload& payload, ARM::CHI::Phase& phase)
{
    target.nb_transport_bw(payload, phase);
    print_payload(false, payload, phase);
    return tlm::TLM_ACCEPTED;
}

template <typename T, size_t N>
static const char* enum_value_to_string(T value, const char* const (&names)[N])
{
    return value >= 0 && value < N ? names[value] : "???";
}

static const char* channel_to_string(const ARM::CHI::Channel channel)
{
    static const char* const names[] = {"Req", "Snp", "Rsp", "Dat"};

    return enum_value_to_string(channel, names);
}

static const char* phase_opcode_to_string(const ARM::CHI::Phase &phase)
{
    if (phase.channel == ARM::CHI::CHANNEL_DAT) {
        static const char* const names[] = {
            "DatLCrdReturn", // 0x00
            "SnpRespData",
            "CopyBackWrData",
            "NonCopyBackWrData",
            "CompData",
            "SnpRespDataPtl",
            "SnpRespDataFwded",
            "WriteDataCancel",
            "DatOpcode0x08",
            "DatOpcode0x09",
            "DatOpcode0x0a",
            "DataSepResp",
            "NCBWrDataCompAck",
        };

        return enum_value_to_string(phase.dat_opcode, names);
    } else if (phase.channel == ARM::CHI::CHANNEL_REQ) {
        static const char* const names[] = {
            "ReqLCrdReturn", // 0x00
            "ReadShared",
            "ReadClean",
            "ReadOnce",
            "ReadNoSnp",
            "PCrdReturn",
            "ReqOpcode0x06",
            "ReadUnique",
            "CleanShared",
            "CleanInvalid",
            "MakeInvalid",
            "CleanUnique",
            "MakeUnique",
            "Evict",
            "ReqOpcode0x0e",
            "ReqOpcode0x0f",
            "ReqOpcode0x10", // 0x10
            "ReadNoSnpSep",
            "ReqOpcode0x12",
            "CleanSharedPersistSep",
            "DVMOp",
            "WriteEvictFull",
            "ReqOpcode0x16",
            "WriteCleanFull",
            "WriteUniquePtl",
            "WriteUniqueFull",
            "WriteBackPtl",
            "WriteBackFull",
            "WriteNoSnpPtl",
            "WriteNoSnpFull",
            "ReqOpcode0x1e",
            "ReqOpcode0x1f",
            "WriteUniqueFullStash", // 0x20
            "WriteUniquePtlStash",
            "StashOnceShared",
            "StashOnceUnique",
            "ReadOnceCleanInvalid",
            "ReadOnceMakeInvalid",
            "ReadNotSharedDirty",
            "CleanSharedPersist",
            "AtomicStoreAdd",
            "AtomicStoreClr",
            "AtomicStoreEor",
            "AtomicStoreSet",
            "AtomicStoreSMax",
            "AtomicStoreSMin",
            "AtomicStoreUMax",
            "AtomicStoreUMin",
            "AtomicLoadAdd", // 0x30
            "AtomicLoadClr",
            "AtomicLoadEor",
            "AtomicLoadSet",
            "AtomicLoadSMax",
            "AtomicLoadSMin",
            "AtomicLoadUMax",
            "AtomicLoadUMin",
            "AtomicSwap",
            "AtomicCompare",
            "PrefetchTgt",
            "ReqOpcode0x3b",
            "ReqOpcode0x3c",
            "ReqOpcode0x3d",
            "ReqOpcode0x3e",
            "ReqOpcode0x3f",
            "ReqOpcode0x40", // 0x40
            "MakeReadUnique",
            "WriteEvictOrEvict",
            "WriteUniqueZero",
            "WriteNoSnpZero",
            "ReqOpcode0x45",
            "ReqOpcode0x46",
            "StashOnceSepShared",
            "StashOnceSepUnique",
            "ReqOpcode0x49",
            "ReqOpcode0x4a",
            "ReqOpcode0x4b",
            "ReadPreferUnique",
            "ReqOpcode0x4d",
            "ReqOpcode0x4e",
            "ReqOpcode0x4f",
            "WriteNoSnpFullCleanSh", // 0x50
            "WriteNoSnpFullCleanInv",
            "WriteNoSnpFullCleanShPerSep",
            "ReqOpcode0x53",
            "WriteUniqueFullCleanSh",
            "ReqOpcode0x55",
            "WriteUniqueFullCleanShPerSep",
            "ReqOpcode0x57",
            "WriteBackFullCleanSh",
            "WriteBackFullCleanInv",
            "WriteBackFullCleanShPerSep",
            "ReqOpcode0x5b",
            "WriteCleanFullCleanSh",
            "ReqOpcode0x5d",
            "WriteCleanFullCleanShPerSep",
            "ReqOpcode0x5f",
            "WriteNoSnpPtlCleanSh", // 0x60
            "WriteNoSnpPtlCleanInv",
            "WriteNoSnpPtlCleanShPerSep",
            "ReqOpcode0x63",
            "WriteUniquePtlCleanSh",
            "ReqOpcode0x65",
            "WriteUniquePtlCleanShPerSep",
        };

        return enum_value_to_string(phase.req_opcode, names);
    } else if (phase.channel == ARM::CHI::CHANNEL_RSP) {
        static const char* names[] = {
            "RspLCrdReturn", // 0x00
            "SnpResp",
            "CompAck",
            "RetryAck",
            "Comp",
            "CompDBIDResp",
            "DBIDResp",
            "PCrdGrant",
            "ReadReceipt",
            "SnpRespFwded",
            "TagMatch",
            "RespSepData",
            "Persist",
            "CompPersist",
            "DBIDRespOrd",
            "RspOpcode0x0f",
            "StashDone", // 0x10
            "CompStashDone",
            "RspOpcode0x12",
            "RspOpcode0x13",
            "CompCMO",
        };

        return enum_value_to_string(phase.rsp_opcode, names);
    } else if (phase.channel == ARM::CHI::CHANNEL_SNP) {
        static const char* names[] = {
            "SnpLCrdReturn", // 0x00
            "SnpShared",
            "SnpClean",
            "SnpOnce",
            "SnpNotSharedDirty",
            "SnpUniqueStash",
            "SnpMakeInvalidStash",
            "SnpUnique",
            "SnpCleanShared",
            "SnpCleanInvalid",
            "SnpMakeInvalid",
            "SnpStashUnique",
            "SnpStashShared",
            "SnpDVMOp",
            "SnpOpcode0x0e",
            "SnpOpcode0x0f",
            "SnpQuery", // 0x10
            "SnpSharedFwd",
            "SnpCleanFwd",
            "SnpOnceFwd",
            "SnpNotSharedDirtyFwd",
            "SnpPreferUnique",
            "SnpPreferUniqueFwd",
            "SnpUniqueFwd",
        };

        return enum_value_to_string(phase.snp_opcode, names);
    } else {
        return "????";
    }
}

static const char* resp_err_to_string(const ARM::CHI::RespErr resp_err)
{
    static const char* const names[] = {"Ok   ", "ExOk ", "DErr ", "NDErr"};

    return enum_value_to_string(resp_err, names);
}

static bool phase_is_pcrd_grant(const ARM::CHI::Phase& phase)
{
    return phase.channel == ARM::CHI::CHANNEL_RSP && phase.rsp_opcode == ARM::CHI::RSP_OPCODE_PCRD_GRANT;
}

static bool phase_is_group_message(const ARM::CHI::Phase& phase)
{
    if (phase.channel != ARM::CHI::CHANNEL_RSP)
        return false;

    switch (phase.rsp_opcode) {
    case ARM::CHI::RSP_OPCODE_PERSIST:
    case ARM::CHI::RSP_OPCODE_STASH_DONE:
    case ARM::CHI::RSP_OPCODE_TAG_MATCH: return true;

    default: return false;
    }
}

static bool rsp_opcode_has_resp_err(const ARM::CHI::RspOpcode rsp_opcode)
{
    switch (rsp_opcode) {
    case ARM::CHI::RSP_OPCODE_COMP_ACK:
    case ARM::CHI::RSP_OPCODE_RETRY_ACK:
    case ARM::CHI::RSP_OPCODE_DBID_RESP:
    case ARM::CHI::RSP_OPCODE_DBID_RESP_ORD:
    case ARM::CHI::RSP_OPCODE_PCRD_GRANT:
    case ARM::CHI::RSP_OPCODE_READ_RECEIPT: return false;

    default: return true;
    }
}

void CHIMonitor::print_payload(const bool fw, const ARM::CHI::Payload& payload, const ARM::CHI::Phase& phase)
{
    std::ostringstream stream;

    stream << sc_core::sc_time_stamp() << ' ' << name() << ": " << channel_to_string(phase.channel);

    if (phase.lcrd) {
        stream << (fw ? " ---->   " : "    <----");
        stream << " link-credit";
    } else {
        bool print_endpoints = true;
        bool print_address = false;
        bool print_resp_err = false;
        bool print_data = false;

        if (phase.raw_opcode == 0) {
            // L-Credit return, no fields
            print_endpoints = false;
        } else {
            if (phase_is_pcrd_grant(phase)) {
            } else if (phase_is_group_message(phase)) {
                print_endpoints = true;
            } else {
                print_endpoints = true;
                print_address = true;
                print_resp_err = phase.channel == ARM::CHI::CHANNEL_DAT ||
                                 (phase.channel == ARM::CHI::CHANNEL_RSP && rsp_opcode_has_resp_err(phase.rsp_opcode));
                print_data = phase.channel == ARM::CHI::CHANNEL_DAT;
            }
        }

        if (print_endpoints) {
            stream << std::setfill('0');

            if (fw)
                stream << ' '
                    << std::setw(3) << std::hex << phase.src_id
                    << "->"
                    << std::setw(3) << phase.tgt_id;
            else
                stream << ' '
                    << std::setw(3) << std::hex << phase.tgt_id
                    << "<-"
                    << std::setw(3) << phase.src_id;
        }

        stream << ' ' << std::setw(20) << std::setfill(' ') << std::left << phase_opcode_to_string(phase) << std::right;

        if (print_address) {
            // Basic transaction information, valid from the request onwards

            stream << " TxnID:" << std::setw(3) << std::setfill('0') << std::hex << phase.txn_id;
            stream << " @" << std::setw(12) << std::setfill('0') << std::hex << payload.address << std::dec << ' ';

            stream << std::setw(3) << std::setfill(' ') << ((1 << payload.size) * 8) << "-bit";
        }

        if (print_resp_err)
            stream << ' ' << resp_err_to_string(phase.resp_err);

        if (print_data) {
            const unsigned data_offset = phase.data_id * 128 / 8;
            const uint8_t* const beat_data = payload.data + data_offset;

            const uint64_t valid_mask = transaction_valid_bytes_mask(payload) >> data_offset;
            const uint64_t enable_mask = (!fw ? ~uint64_t(0) : payload.byte_enable) >> data_offset & valid_mask;

            stream << std::uppercase << std::hex << std::setfill('0');

            stream << ' ' << unsigned{phase.data_id} << ':';
            for (int i = int(data_width_bytes) - 1; i >= 0; i--) {
                if ((enable_mask >> i & 1) != 0)
                    stream << std::setw(2) << unsigned{beat_data[i]};
                else if ((valid_mask >> i & 1) != 0)
                    stream << "xx"; // byte not updated
                else
                    stream << ".."; // byte not part of the transaction

                if (i != 0 && (i % 8) == 0)
                    stream << '_';
            }
        }
    }

    stream << '\n';
    std::cout << stream.str();
}

CHIMonitor::CHIMonitor(const sc_core::sc_module_name& name, unsigned data_width_bits) :
    sc_core::sc_module(name),
    data_width_bytes(data_width_bits / 8),
    target("target", *this, &CHIMonitor::nb_transport_fw, ARM::TLM::PROTOCOL_CHI_E, data_width_bits),
    initiator("initiator", *this, &CHIMonitor::nb_transport_bw, ARM::TLM::PROTOCOL_CHI_E, data_width_bits)
{}
