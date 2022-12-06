#ifndef ARM_TRANSACTORS_H
#define ARM_TRANSACTORS_H

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

class arm_tlm_extension : public tlm::tlm_extension<arm_tlm_extension>
{
public:
    ARM::AXI4::Payload* arm_payload;

    arm_tlm_extension(ARM::AXI4::Payload* arm_payload_ = NULL) :
        arm_payload(arm_payload_)
    {
        if (arm_payload)
            arm_payload->ref();
    };

    virtual ~arm_tlm_extension()
    {
        if (arm_payload)
            arm_payload->unref();
    };

    virtual tlm::tlm_extension_base* clone() const
    {
        return new arm_tlm_extension(arm_payload);
    };

    virtual void copy_from ( const tlm_extension_base &extension)
    {
        if (arm_payload)
            arm_payload->unref();
        arm_payload = static_cast<arm_tlm_extension const &>(extension).arm_payload;
        arm_payload->ref();
    };
};

class TransARMToGenericImp : public sc_core::sc_module
{
protected:
    SC_HAS_PROCESS(TransARMToGenericImp);

    class mm: public tlm::tlm_mm_interface
    {
    public:
        mm(){}
        tlm::tlm_generic_payload* allocate();
        void  free(tlm::tlm_generic_payload* trans);
    };

    tlm_utils::peq_with_cb_and_phase<TransARMToGenericImp> peq;
    mm m_mm;

    ARM::AXI4::Payload* next_req_payload;
    ARM::AXI4::Payload* cur_req_payload;
    ARM::AXI4::Payload* read_req_to_ack;
    ARM::AXI4::Payload* write_req_to_ack;

    ARM::AXI4::Payload* rsp_payload;
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

    virtual tlm::tlm_sync_enum master_nb_transport_fw(tlm::tlm_generic_payload& gen_payload,
        tlm::tlm_phase& gen_phase, sc_core::sc_time& t) = 0;

    void clock_posedge();
    void clock_negedge();

public:
    TransARMToGenericImp(sc_core::sc_module_name name, ARM::TLM::Protocol protocol, unsigned width);
    ~TransARMToGenericImp();

    tlm::tlm_sync_enum nb_transport_fw(ARM::AXI4::Payload& arm_payload,
        ARM::AXI4::Phase& arm_phase);
    tlm::tlm_sync_enum nb_transport_bw(tlm::tlm_generic_payload& gen_payload,
        tlm::tlm_phase& gen_phase, sc_core::sc_time& t);

    ARM::AXI4::SimpleSlaveSocket<TransARMToGenericImp> slave;
    sc_core::sc_in<bool> clock;
};

class TransGenericToARMImp : public sc_core::sc_module
{
protected:
    SC_HAS_PROCESS(TransGenericToARMImp);

    tlm_utils::peq_with_cb_and_phase<TransGenericToARMImp> peq;
    ARM::AXI4::Payload* req_payload;
    tlm::tlm_generic_payload* req_gen_payload;
    unsigned req_to_send;
    unsigned data_to_send;

    TransPortState state_ar;
    TransPortState state_aw;
    TransPortState state_w;

    std::map <tlm::tlm_generic_payload*, ARM::AXI4::Payload*> gen_to_arm_map;
    std::map <ARM::AXI4::Payload*, tlm::tlm_generic_payload*> arm_to_gen_map;

    tlm::tlm_generic_payload* next_resp_payload;
    tlm::tlm_generic_payload* cur_resp_payload;

protected:
    void process_resp();

    void nb_transport_fw_untimed(tlm::tlm_generic_payload& gen_payload,
        const tlm::tlm_phase& gen_phase);

    virtual tlm::tlm_sync_enum slave_nb_transport_bw(tlm::tlm_generic_payload& gen_payload,
        tlm::tlm_phase& gen_phase, sc_core::sc_time& t) = 0;

    void clock_posedge();
    void clock_negedge();

public:
    TransGenericToARMImp(sc_core::sc_module_name name, ARM::TLM::Protocol protocol, unsigned width);
    ~TransGenericToARMImp();

    tlm::tlm_sync_enum nb_transport_fw(tlm::tlm_generic_payload& gen_payload,
        tlm::tlm_phase& gen_phase, sc_core::sc_time& t);
    tlm::tlm_sync_enum nb_transport_bw(ARM::AXI4::Payload& arm_payload,
        ARM::AXI4::Phase& arm_phase);

    ARM::AXI4::SimpleMasterSocket<TransGenericToARMImp> master;
    sc_core::sc_in<bool> clock;
};

template <unsigned DataWidth = 0>
class TransARMToGeneric : public TransARMToGenericImp
{
protected:
    tlm::tlm_sync_enum master_nb_transport_fw(tlm::tlm_generic_payload& gen_payload,
        tlm::tlm_phase& gen_phase, sc_core::sc_time& t)
    { return master->nb_transport_fw(gen_payload, gen_phase, t); }

public:
    TransARMToGeneric(sc_core::sc_module_name name, ARM::TLM::Protocol protocol) :
        TransARMToGenericImp(name, protocol, DataWidth),
        master("master")
    { master.register_nb_transport_bw(this, &TransARMToGenericImp::nb_transport_bw); }

    tlm_utils::simple_initiator_socket<TransARMToGenericImp, DataWidth, tlm::tlm_base_protocol_types> master;
};

template <unsigned DataWidth = 0>
class TransGenericToARM : public TransGenericToARMImp
{
protected:
    tlm::tlm_sync_enum slave_nb_transport_bw(tlm::tlm_generic_payload& gen_payload,
        tlm::tlm_phase& gen_phase, sc_core::sc_time& t)
    { return slave->nb_transport_bw(gen_payload, gen_phase, t); }

public:
    TransGenericToARM(sc_core::sc_module_name name, ARM::TLM::Protocol protocol) :
        TransGenericToARMImp(name, protocol, DataWidth),
        slave("slave")
    { slave.register_nb_transport_fw(this, &TransGenericToARMImp::nb_transport_fw); }

    tlm_utils::simple_target_socket<TransGenericToARMImp, DataWidth, tlm::tlm_base_protocol_types> slave;
};

#endif // ARM_TRANSACTORS_H
