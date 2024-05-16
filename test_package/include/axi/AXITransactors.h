#ifndef ARM_AXI_TRANSACTORS_H
#define ARM_AXI_TRANSACTORS_H

#include <map>

#include <ARM/TLM/arm_axi4.h>
#include <tlm_utils/simple_target_socket.h>
#include <tlm_utils/simple_initiator_socket.h>
#include <tlm_utils/peq_with_cb_and_phase.h>

enum TransPortState
{
    TRANS_PORT_STATE_CLEAR,
    TRANS_PORT_STATE_ACK,
    TRANS_PORT_STATE_REQ
};

class axi_tlm_extension : public tlm::tlm_extension<axi_tlm_extension>
{
public:
    ARM::AXI::Payload* arm_payload;

    explicit axi_tlm_extension(ARM::AXI::Payload* arm_payload_ = nullptr) :
        arm_payload(arm_payload_)
    {
        if (arm_payload)
            arm_payload->ref();
    }

    virtual ~axi_tlm_extension()
    {
        if (arm_payload)
            arm_payload->unref();
    }

    virtual tlm::tlm_extension_base* clone() const
    {
        return new axi_tlm_extension(arm_payload);
    }

    virtual void copy_from ( const tlm_extension_base &extension)
    {
        if (arm_payload)
            arm_payload->unref();
        arm_payload = static_cast<axi_tlm_extension const &>(extension).arm_payload;
        arm_payload->ref();
    }
};

class TransAXIToGenericImp : public sc_core::sc_module
{
protected:
    SC_HAS_PROCESS(TransAXIToGenericImp);

    class mm : public tlm::tlm_mm_interface
    {
    public:
        mm(){}
        tlm::tlm_generic_payload* allocate();
        void  free(tlm::tlm_generic_payload* trans);
    };

    tlm_utils::peq_with_cb_and_phase<TransAXIToGenericImp> peq;
    mm m_mm;

    ARM::AXI::Payload* next_req_payload;
    ARM::AXI::Payload* cur_req_payload;
    ARM::AXI::Payload* read_req_to_ack;
    ARM::AXI::Payload* write_req_to_ack;

    ARM::AXI::Payload* rsp_payload;
    tlm::tlm_generic_payload* rsp_gen_payload;
    unsigned rsp_to_send;
    bool got_ack;
    unsigned data_to_send;

    bool waiting_for_end_req;

    TransPortState state_r;
    TransPortState state_b;

protected:
    bool process_req();

    void nb_transport_bw_untimed(tlm::tlm_generic_payload& gen_payload,
        const tlm::tlm_phase& gen_phase);

    virtual tlm::tlm_sync_enum initiator_nb_transport_fw(tlm::tlm_generic_payload& gen_payload,
        tlm::tlm_phase& gen_phase, sc_core::sc_time& t) = 0;

    void clock_posedge();
    void clock_negedge();

public:
    TransAXIToGenericImp(sc_core::sc_module_name name, ARM::TLM::Protocol protocol, unsigned width);
    ~TransAXIToGenericImp();

    tlm::tlm_sync_enum nb_transport_fw(ARM::AXI::Payload& arm_payload,
        ARM::AXI::Phase& arm_phase);
    tlm::tlm_sync_enum nb_transport_bw(tlm::tlm_generic_payload& gen_payload,
        tlm::tlm_phase& gen_phase, sc_core::sc_time& t);

    ARM::AXI::SimpleTargetSocket<TransAXIToGenericImp> target;
    sc_core::sc_in<bool> clock;
};

class TransGenericToAXIImp : public sc_core::sc_module
{
protected:
    SC_HAS_PROCESS(TransGenericToAXIImp);

    tlm_utils::peq_with_cb_and_phase<TransGenericToAXIImp> peq;
    ARM::AXI::Payload* req_payload;
    tlm::tlm_generic_payload* req_gen_payload;
    unsigned req_to_send;
    unsigned data_to_send;

    TransPortState state_ar;
    TransPortState state_aw;
    TransPortState state_w;

    std::map <tlm::tlm_generic_payload*, ARM::AXI::Payload*> gen_to_arm_map;
    std::map <ARM::AXI::Payload*, tlm::tlm_generic_payload*> arm_to_gen_map;

    tlm::tlm_generic_payload* next_resp_payload;
    tlm::tlm_generic_payload* cur_resp_payload;

protected:
    void process_resp();

    void nb_transport_fw_untimed(tlm::tlm_generic_payload& gen_payload,
        const tlm::tlm_phase& gen_phase);

    virtual tlm::tlm_sync_enum target_nb_transport_bw(tlm::tlm_generic_payload& gen_payload,
        tlm::tlm_phase& gen_phase, sc_core::sc_time& t) = 0;

    void clock_posedge();
    void clock_negedge();

public:
    TransGenericToAXIImp(sc_core::sc_module_name name, ARM::TLM::Protocol protocol, unsigned width);
    ~TransGenericToAXIImp();

    tlm::tlm_sync_enum nb_transport_fw(tlm::tlm_generic_payload& gen_payload,
        tlm::tlm_phase& gen_phase, sc_core::sc_time& t);
    tlm::tlm_sync_enum nb_transport_bw(ARM::AXI::Payload& arm_payload,
        ARM::AXI::Phase& arm_phase);

    ARM::AXI::SimpleInitiatorSocket<TransGenericToAXIImp> initiator;
    sc_core::sc_in<bool> clock;
};

template <unsigned DataWidth = 0>
class TransAXIToGeneric : public TransAXIToGenericImp
{
protected:
    tlm::tlm_sync_enum initiator_nb_transport_fw(tlm::tlm_generic_payload& gen_payload,
        tlm::tlm_phase& gen_phase, sc_core::sc_time& t)
    { return initiator->nb_transport_fw(gen_payload, gen_phase, t); }

public:
    TransAXIToGeneric(sc_core::sc_module_name name, ARM::TLM::Protocol protocol) :
        TransAXIToGenericImp(name, protocol, DataWidth),
        initiator("initiator")
    { initiator.register_nb_transport_bw(this, &TransAXIToGenericImp::nb_transport_bw); }

    tlm_utils::simple_initiator_socket<TransAXIToGenericImp, DataWidth, tlm::tlm_base_protocol_types> initiator;
};

template <unsigned DataWidth = 0>
class TransGenericToAXI : public TransGenericToAXIImp
{
protected:
    tlm::tlm_sync_enum target_nb_transport_bw(tlm::tlm_generic_payload& gen_payload,
        tlm::tlm_phase& gen_phase, sc_core::sc_time& t)
    { return target->nb_transport_bw(gen_payload, gen_phase, t); }

public:
    TransGenericToAXI(sc_core::sc_module_name name, ARM::TLM::Protocol protocol) :
        TransGenericToAXIImp(name, protocol, DataWidth),
        target("target")
    { target.register_nb_transport_fw(this, &TransGenericToAXIImp::nb_transport_fw); }

    tlm_utils::simple_target_socket<TransGenericToAXIImp, DataWidth, tlm::tlm_base_protocol_types> target;
};

#endif /* ARM_AXI_TRANSACTORS_H */
