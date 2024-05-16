#ifndef ARM_CHI_UTILITIES_H
#define ARM_CHI_UTILITIES_H

#include <ARM/TLM/arm_chi_payload.h>
#include <ARM/TLM/arm_chi_phase.h>

#include <cstdint>
#include <deque>
#include <vector>

/* Calculate a mask of bytes that may be transferred in the transaction. */
inline uint64_t transaction_valid_bytes_mask(const ARM::CHI::Payload& payload)
{
    const unsigned size_bytes = 1 << payload.size;
    const uint64_t align_mask = size_bytes - 1;

    const unsigned first_byte =
            ((payload.mem_attr & ARM::CHI::MEM_ATTR_DEVICE) != 0 ? payload.address : payload.address & ~align_mask) &
            0x3f;
    const unsigned last_byte = (payload.address & 0x3f) | align_mask;

    const uint64_t all_ones = ~uint64_t(0);
    const uint64_t valid_bytes = all_ones >> (63 - last_byte) & all_ones << first_byte;

    return valid_bytes;
}

static const unsigned CHI_CACHE_LINE_SIZE_LOG2_BYTES = 6;
static const unsigned CHI_CACHE_LINE_SIZE_BYTES = 1 << CHI_CACHE_LINE_SIZE_LOG2_BYTES;
static const uint64_t CHI_CACHE_LINE_ADDRESS_MASK = ~((UINT64_C(1) << CHI_CACHE_LINE_SIZE_LOG2_BYTES) - 1);

/* Generate the DataIDs that will be seen in the transaction. */
inline std::vector<uint8_t> transaction_data_ids(const ARM::CHI::Payload& payload, const unsigned data_width_bytes)
{
    const unsigned size_bytes = 1 << payload.size;
    const uint64_t align_mask = size_bytes - 1;

    const unsigned aligned_offset = payload.address & ~align_mask & ~CHI_CACHE_LINE_ADDRESS_MASK;

    const unsigned beat_inc = data_width_bytes / 16;
    const unsigned beat_count = size_bytes <= data_width_bytes ? 1 : size_bytes / data_width_bytes;
    const unsigned first_data_id = (aligned_offset >> 4) & ~(beat_inc - 1);

    std::vector<uint8_t> data_ids;
    data_ids.reserve(4);

    for (unsigned data_id = first_data_id, i = 0; i < beat_count; data_id += beat_inc, i++)
        data_ids.push_back(data_id);

    return data_ids;
}

/* Package up a payload and associated phase, managing the ref counting on the payload. */
struct CHIFlit
{
    CHIFlit(ARM::CHI::Payload& payload_, const ARM::CHI::Phase& phase_) : payload(payload_), phase(phase_)
    {
        payload.ref();
    }

    ~CHIFlit() { payload.unref(); }

    CHIFlit(const CHIFlit& other) : CHIFlit(other.payload, other.phase) { }
    CHIFlit& operator=(const CHIFlit&) = delete;

    ARM::CHI::Payload& payload;
    ARM::CHI::Phase phase;
};

static const unsigned CHI_NUM_CHANNELS = 4;

/* Maximum number of credits to issue.  The CHI architecture limits this to 15. */
static const unsigned CHI_MAX_LINK_CREDITS = 15;

struct CHIChannelState
{
    std::deque<CHIFlit> tx_queue;
    std::deque<CHIFlit> rx_queue;

    /* Link credits issued to us by our peer that we can use. */
    uint8_t tx_credits_available = 0;

    /* If >= 0, link credits we can issue to our peer for them to use.  If < 0, channel disabled. */
    int8_t rx_credits_available = -1;

    bool receive_flit(ARM::CHI::Payload& payload, ARM::CHI::Phase& phase)
    {
        if (phase.lcrd)
        {
            tx_credits_available++;
        } else
        {
            if (rx_credits_available < 0)
                return false;

            rx_credits_available++;
            rx_queue.emplace_back(payload, phase);
        }

        return true;
    }

    template <typename F>
    void send_flits(const ARM::CHI::Channel channel, F nb_transporter)
    {
        if (rx_credits_available > 0)
        {
            ARM::CHI::Payload* const payload = ARM::CHI::Payload::get_dummy();
            ARM::CHI::Phase phase;

            phase.channel = channel;
            phase.lcrd = true;

            rx_credits_available--;
            nb_transporter(*payload, phase);
        }

        if (!tx_queue.empty() && tx_credits_available > 0)
        {
            CHIFlit tx_flit = tx_queue.front();
            tx_queue.pop_front();

            tx_credits_available--;
            nb_transporter(tx_flit.payload, tx_flit.phase);
        }
    }
};

#endif // ARM_CHI_UTILITIES_H
