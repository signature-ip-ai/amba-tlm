#include <string>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <assert.h>

#include "Transactors.h"
#include "tlm_utils/peq_with_cb_and_phase.h"
using namespace std;
using namespace sc_core;

tlm::tlm_generic_payload* TransARMToGenericImp::mm::allocate()
{
    return new tlm::tlm_generic_payload(this);
}

void TransARMToGenericImp::mm::free(tlm::tlm_generic_payload* trans)
{
    if (trans->get_data_ptr())
        delete[] trans->get_data_ptr();
    if (trans->get_byte_enable_ptr())
        delete[] trans->get_byte_enable_ptr();
    delete trans;
}

bool TransARMToGenericImp::process_req()
{
    if (next_req_payload && !cur_req_payload)
    {
        cur_req_payload = next_req_payload;
        next_req_payload = NULL;

        bool is_write = (cur_req_payload->get_command() == ARM::AXI4::COMMAND_WRITE);

        tlm::tlm_generic_payload* gen_payload = m_mm.allocate();
        gen_payload->acquire();

        gen_payload->set_address(cur_req_payload->get_base_address());
        gen_payload->set_data_ptr( reinterpret_cast<unsigned char*>(new char[cur_req_payload->get_data_length()]) );
        gen_payload->set_data_length(cur_req_payload->get_data_length());
        gen_payload->set_streaming_width(cur_req_payload->get_beat_data_length());

        if (is_write)
        {
            gen_payload->set_command(tlm::TLM_WRITE_COMMAND);
            gen_payload->set_byte_enable_ptr(reinterpret_cast<unsigned char*>(new char[cur_req_payload->get_data_length() / 8 + 1]));

            cur_req_payload->write_out(gen_payload->get_data_ptr());
            cur_req_payload->write_out_strobes(gen_payload->get_byte_enable_ptr());
        }
        else
        {
            gen_payload->set_command(tlm::TLM_READ_COMMAND);
        }

        gen_payload->set_response_status( tlm::TLM_INCOMPLETE_RESPONSE );
        gen_payload->set_extension(new arm_tlm_extension(cur_req_payload));

        tlm::tlm_phase gen_phase = tlm::BEGIN_REQ;
        sc_core::sc_time t = sc_core::SC_ZERO_TIME;
        tlm::tlm_sync_enum reply = master_nb_transport_fw(*gen_payload, gen_phase, t);
        if (reply == tlm::TLM_UPDATED)
        {
            cur_req_payload = NULL;
            return true;
        }
    }
    return false;
}

tlm::tlm_sync_enum TransARMToGenericImp::nb_transport_fw(ARM::AXI4::Payload& arm_payload, ARM::AXI4::Phase& arm_phase)
{
    switch (arm_phase)
    {
        case ARM::AXI4::AW_VALID:
            arm_phase = ARM::AXI4::AW_READY;
            return tlm::TLM_UPDATED;
        case ARM::AXI4::W_VALID:
            arm_phase = ARM::AXI4::W_READY;
            return tlm::TLM_UPDATED;
        case ARM::AXI4::W_VALID_LAST:
            {
                assert(!next_req_payload);
                next_req_payload = &arm_payload;
                if (process_req())
                {
                    arm_phase = ARM::AXI4::W_READY;
                    return tlm::TLM_UPDATED;
                }
                else
                {
                    return tlm::TLM_ACCEPTED;
                }
            }
        case ARM::AXI4::AR_VALID:
            {
                assert(!next_req_payload);
                next_req_payload = &arm_payload;
                if (process_req())
                {
                    arm_phase = ARM::AXI4::AR_READY;
                    return tlm::TLM_UPDATED;
                }
                else
                {
                    return tlm::TLM_ACCEPTED;
                }
            }
        case ARM::AXI4::RACK:
        case ARM::AXI4::WACK:
            got_ack = true;
            return tlm::TLM_ACCEPTED;

        default:
            assert(!"Unknown phase");
    }

    return tlm::TLM_ACCEPTED;
}

tlm::tlm_sync_enum TransARMToGenericImp::nb_transport_bw(tlm::tlm_generic_payload& gen_payload, tlm::tlm_phase& gen_phase, sc_core::sc_time& t)
{
    peq.notify(gen_payload, gen_phase, t);
    return tlm::TLM_ACCEPTED;
}

void TransARMToGenericImp::nb_transport_bw_untimed(tlm::tlm_generic_payload& gen_payload, const tlm::tlm_phase& gen_phase)
{
    switch (gen_phase)
    {
        case tlm::BEGIN_RESP:
        {
            arm_tlm_extension* arm_ext;
            gen_payload.get_extension(arm_ext);
            rsp_payload = arm_ext->arm_payload;

            assert(rsp_payload);
            rsp_gen_payload = &gen_payload;

            got_ack = false;
            if (gen_payload.is_read())
            {
                rsp_to_send = rsp_payload->get_beat_count();
                rsp_payload->read_in(gen_payload.get_data_ptr());
            }
            else
            {
                rsp_to_send = 1;
            }
            break;
        }
        case tlm::END_REQ:
        {
            if (gen_payload.is_read())
                read_req_to_ack = cur_req_payload;
            else
                write_req_to_ack = cur_req_payload;
            cur_req_payload = NULL;
            if (process_req())
            {
                if (gen_payload.is_read())
                    read_req_to_ack = cur_req_payload;
                else
                    write_req_to_ack = cur_req_payload;
                cur_req_payload = NULL;
            }
            break;
        }
        default:
            assert(!"Unknown phase");
    }
}

void TransARMToGenericImp::clock_posedge()
{
    if (state_r == TRANS_PORT_STATE_ACK)
        state_r = TRANS_PORT_STATE_CLEAR;
    if (state_b == TRANS_PORT_STATE_ACK)
        state_b = TRANS_PORT_STATE_CLEAR;
}

void TransARMToGenericImp::clock_negedge()
{
    if (read_req_to_ack)
    {
        ARM::AXI4::Phase arm_phase = ARM::AXI4::AR_READY;
        slave.nb_transport_bw(*read_req_to_ack, arm_phase);
        read_req_to_ack = NULL;
    }

    if (write_req_to_ack)
    {
        ARM::AXI4::Phase arm_phase = ARM::AXI4::W_READY;
        slave.nb_transport_bw(*write_req_to_ack, arm_phase);
        write_req_to_ack = NULL;
    }

    if (rsp_payload)
    {
        if (rsp_payload->get_command() == ARM::AXI4::COMMAND_READ)
        {
            if (rsp_to_send && state_r == TRANS_PORT_STATE_CLEAR)
            {
                ARM::AXI4::Phase arm_phase = ARM::AXI4::R_VALID;
                rsp_to_send--;
                if (rsp_to_send == 0)
                    arm_phase = ARM::AXI4::R_VALID_LAST;
                tlm::tlm_sync_enum resp = slave.nb_transport_bw(*rsp_payload, arm_phase);

                if (resp == tlm::TLM_ACCEPTED)
                    state_r = TRANS_PORT_STATE_REQ;
                else
                    state_r = TRANS_PORT_STATE_ACK;
            }

        }
        else
        {
            if (rsp_to_send && state_b == TRANS_PORT_STATE_CLEAR)
            {
                ARM::AXI4::Phase arm_phase = ARM::AXI4::B_VALID;
                rsp_to_send--;
                tlm::tlm_sync_enum resp = slave.nb_transport_bw(*rsp_payload, arm_phase);

                if (resp == tlm::TLM_ACCEPTED)
                    state_b = TRANS_PORT_STATE_REQ;
                else
                    state_b = TRANS_PORT_STATE_ACK;
            }

        }

        if (state_b != TRANS_PORT_STATE_REQ &&
            state_r  != TRANS_PORT_STATE_REQ &&
            rsp_to_send == 0 &&
            got_ack == true)
        {
            rsp_payload = NULL;
            tlm::tlm_phase gen_phase = tlm::END_RESP;
            sc_core::sc_time t = sc_core::SC_ZERO_TIME;
            tlm::tlm_sync_enum reply = master_nb_transport_fw(*rsp_gen_payload, gen_phase, t);
            assert(reply == tlm::TLM_COMPLETED);
            rsp_gen_payload->release();
        }
    }

}

TransARMToGenericImp::TransARMToGenericImp(sc_module_name name, ARM::TLM::Protocol protocol, unsigned width) :
    sc_module(name),
    peq(this, &TransARMToGenericImp::nb_transport_bw_untimed),
    next_req_payload(NULL),
    cur_req_payload(NULL),
    read_req_to_ack(NULL),
    write_req_to_ack(NULL),
    rsp_payload(NULL),
    rsp_gen_payload(NULL),
    state_r(TRANS_PORT_STATE_CLEAR),
    state_b(TRANS_PORT_STATE_CLEAR),
    slave("slave", *this, &TransARMToGenericImp::nb_transport_fw, protocol, width),
    clock("clock")
{
    SC_METHOD(clock_posedge);
    sensitive << clock.pos();
    dont_initialize();

    SC_METHOD(clock_negedge);
    sensitive << clock.neg();
    dont_initialize();
}

TransARMToGenericImp::~TransARMToGenericImp()
{
}

tlm::tlm_sync_enum TransGenericToARMImp::nb_transport_fw(tlm::tlm_generic_payload& gen_payload, tlm::tlm_phase& gen_phase, sc_core::sc_time& t)
{
    peq.notify(gen_payload, gen_phase, t);
    if (gen_phase == tlm::END_RESP)
        return tlm::TLM_COMPLETED;
    return tlm::TLM_ACCEPTED;
}

void TransGenericToARMImp::process_resp()
{
    if (next_resp_payload && !cur_resp_payload)
    {
        cur_resp_payload = next_resp_payload;
        next_resp_payload = NULL;

        tlm::tlm_phase gen_phase = tlm::BEGIN_RESP;
        sc_core::sc_time t = sc_core::SC_ZERO_TIME;
        tlm::tlm_sync_enum reply = slave_nb_transport_bw(*cur_resp_payload, gen_phase, t);

        if (reply == tlm::TLM_UPDATED)
            peq.notify(*cur_resp_payload, gen_phase, t);

        if (reply == tlm::TLM_COMPLETED)
        {
            gen_phase = tlm::END_RESP;
            peq.notify(*cur_resp_payload, gen_phase, t);
        }
    }
}

void TransGenericToARMImp::nb_transport_fw_untimed(tlm::tlm_generic_payload& gen_payload, const tlm::tlm_phase& gen_phase)
{
    switch (gen_phase)
    {
        case tlm::BEGIN_REQ:
        {
            arm_tlm_extension* arm_ext;
            gen_payload.get_extension(arm_ext);
            ARM::AXI4::Payload* parent_payload = NULL;
            if (arm_ext != NULL)
                parent_payload = arm_ext->arm_payload;

            ARM::AXI4::Command command = gen_payload.is_read() ?
                ARM::AXI4::COMMAND_READ : ARM::AXI4::COMMAND_WRITE;

            uint64_t address = gen_payload.get_address();
            ARM::AXI4::Size size = 0;

            unsigned beat_width = gen_payload.get_streaming_width();
            if (beat_width == 0)
                beat_width = 1;
            while (beat_width >>= 1)
                size = size + 1;
            beat_width = 1 << size;
            uint8_t len = gen_payload.get_data_length() / beat_width - 1;

            if (parent_payload)
                req_payload = parent_payload->descend(command, address, size, len);
            else
                req_payload = ARM::AXI4::Payload::new_payload(command, address, size, len);

            data_to_send = 0;
            req_to_send = 1;

            if (command == ARM::AXI4::COMMAND_WRITE)
            {
                data_to_send = req_payload->get_beat_count();
                req_payload->write_in(gen_payload.get_data_ptr(), gen_payload.get_byte_enable_ptr());
            }

            arm_to_gen_map[req_payload] = &gen_payload;
            gen_to_arm_map[&gen_payload] = req_payload;
            req_gen_payload = &gen_payload;
            break;
        }
        case tlm::END_RESP:
        {
            cur_resp_payload = NULL;
            ARM::AXI4::Payload* rsp_payload = gen_to_arm_map[&gen_payload];
            ARM::AXI4::Phase arm_phase = rsp_payload->get_command() ==  ARM::AXI4::COMMAND_READ ?
                ARM::AXI4::R_READY : ARM::AXI4::B_READY;
            master.nb_transport_fw(*rsp_payload, arm_phase);

            arm_phase = rsp_payload->get_command() ==  ARM::AXI4::COMMAND_READ ?
                ARM::AXI4::RACK : ARM::AXI4::WACK;
            master.nb_transport_fw(*rsp_payload, arm_phase);

            process_resp();
            rsp_payload->unref();
            gen_to_arm_map.erase(&gen_payload);
            break;
        }
        default:
            assert(!"Unknown phase");
    }
}

tlm::tlm_sync_enum TransGenericToARMImp::nb_transport_bw(ARM::AXI4::Payload& arm_payload, ARM::AXI4::Phase& arm_phase)
{
    switch (arm_phase)
    {
        case ARM::AXI4::AR_READY:
            state_ar = TRANS_PORT_STATE_ACK;
            break;

        case ARM::AXI4::AW_READY:
            state_aw = TRANS_PORT_STATE_ACK;
            break;

        case ARM::AXI4::W_READY:
            state_w = TRANS_PORT_STATE_ACK;
            break;

        case ARM::AXI4::R_VALID:
            arm_phase = ARM::AXI4::R_READY;
            return tlm::TLM_UPDATED;

        case ARM::AXI4::R_VALID_LAST:
        {
            tlm::tlm_generic_payload* gen_payload = arm_to_gen_map[&arm_payload];
            arm_to_gen_map.erase(&arm_payload);
            assert(gen_payload);
            assert(&arm_payload);

            arm_payload.read_out(gen_payload->get_data_ptr());
            if (arm_payload.get_resp() == 0)
                gen_payload->set_response_status(tlm::TLM_OK_RESPONSE);
            next_resp_payload = gen_payload;
            process_resp();
            return tlm::TLM_ACCEPTED;
        }
        case ARM::AXI4::B_VALID:
        {
            tlm::tlm_generic_payload* gen_payload = arm_to_gen_map[&arm_payload];
            arm_to_gen_map.erase(&arm_payload);
            assert(gen_payload);
            assert(&arm_payload);

            next_resp_payload = gen_payload;
            if (arm_payload.get_resp() == 0)
                gen_payload->set_response_status(tlm::TLM_OK_RESPONSE);
            process_resp();
            return tlm::TLM_ACCEPTED;
        }
        default:
            assert(!"Unknown phase");
    }

    tlm::tlm_generic_payload gen_payload;
    tlm::tlm_phase gen_phase;
    sc_core::sc_time t = sc_core::SC_ZERO_TIME;

    tlm::tlm_sync_enum reply = slave_nb_transport_bw(gen_payload, gen_phase, t);

    return reply;
}

void TransGenericToARMImp::clock_posedge()
{
    if (state_ar == TRANS_PORT_STATE_ACK)
        state_ar = TRANS_PORT_STATE_CLEAR;
    if (state_aw == TRANS_PORT_STATE_ACK)
        state_aw = TRANS_PORT_STATE_CLEAR;
    if (state_w == TRANS_PORT_STATE_ACK)
        state_w = TRANS_PORT_STATE_CLEAR;
}

void TransGenericToARMImp::clock_negedge()
{
    if (!req_payload)
        return;
    if (req_payload->get_command() == ARM::AXI4::COMMAND_READ)
    {
        if (req_to_send && state_ar == TRANS_PORT_STATE_CLEAR)
        {
            ARM::AXI4::Phase arm_phase = ARM::AXI4::AR_VALID;
            tlm::tlm_sync_enum resp = master.nb_transport_fw(*req_payload, arm_phase);
            req_to_send--;

            if (resp == tlm::TLM_ACCEPTED)
                state_ar = TRANS_PORT_STATE_REQ;
            else
                state_ar = TRANS_PORT_STATE_ACK;
        }
    }
    else
    {
        if (req_to_send && state_aw == TRANS_PORT_STATE_CLEAR)
        {
            ARM::AXI4::Phase arm_phase = ARM::AXI4::AW_VALID;
            tlm::tlm_sync_enum resp = master.nb_transport_fw(*req_payload, arm_phase);
            req_to_send--;

            if (resp == tlm::TLM_ACCEPTED)
                state_aw = TRANS_PORT_STATE_REQ;
            else
                state_aw = TRANS_PORT_STATE_ACK;
        }

        if (data_to_send && state_w == TRANS_PORT_STATE_CLEAR)
        {
            ARM::AXI4::Phase arm_phase = ARM::AXI4::W_VALID;
            data_to_send--;
            if (data_to_send == 0)
                arm_phase = ARM::AXI4::W_VALID_LAST;
            tlm::tlm_sync_enum resp = master.nb_transport_fw(*req_payload, arm_phase);

            if (resp == tlm::TLM_ACCEPTED)
                state_w = TRANS_PORT_STATE_REQ;
            else
                state_w = TRANS_PORT_STATE_ACK;
        }
    }
    if (state_ar != TRANS_PORT_STATE_REQ &&
        state_aw != TRANS_PORT_STATE_REQ && req_to_send == 0 &&
        state_w != TRANS_PORT_STATE_REQ && data_to_send == 0)
    {
        tlm::tlm_phase gen_phase = tlm::END_REQ;
        sc_core::sc_time t = sc_core::SC_ZERO_TIME;
        slave_nb_transport_bw(*req_gen_payload, gen_phase, t);
        req_payload = NULL;
    }
}

TransGenericToARMImp::TransGenericToARMImp(sc_module_name name, ARM::TLM::Protocol protocol, unsigned width) :
    sc_module(name),
    peq(this, &TransGenericToARMImp::nb_transport_fw_untimed),
    req_payload(NULL),
    req_gen_payload(NULL),
    state_ar(TRANS_PORT_STATE_CLEAR),
    state_aw(TRANS_PORT_STATE_CLEAR),
    state_w(TRANS_PORT_STATE_CLEAR),
    next_resp_payload(NULL),
    cur_resp_payload(NULL),
    master("master", *this, &TransGenericToARMImp::nb_transport_bw, protocol, width),
    clock("clock")
{
    SC_METHOD(clock_posedge);
    sensitive << clock.pos();
    dont_initialize();

    SC_METHOD(clock_negedge);
    sensitive << clock.neg();
    dont_initialize();
}

TransGenericToARMImp::~TransGenericToARMImp()
{
}
