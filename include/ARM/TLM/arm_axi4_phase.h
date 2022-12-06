//-------------------------------------------------------------------
// The Clear BSD License
//
// Copyright (c) 2015-2019 Arm Limited.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted (subject to the limitations in the disclaimer
// below) provided that the following conditions are met:
//
//      * Redistributions of source code must retain the above copyright notice,
//      this list of conditions and the following disclaimer.
//
//      * Redistributions in binary form must reproduce the above copyright
//      notice, this list of conditions and the following disclaimer in the
//      documentation and/or other materials provided with the distribution.
//
//      * Neither the name of the copyright holder nor the names of its
//      contributors may be used to endorse or promote products derived from this
//      software without specific prior written permission.
//
// NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY
// THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
// CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
// PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
// BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
// IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//-------------------------------------------------------------------

#ifndef ARM_AXI4_PHASE_H
#define ARM_AXI4_PHASE_H

#include <stdint.h>

namespace ARM
{
namespace AXI4
{

/**
 * Communication phases for AXI4.
 *
 * Phases correspond to events on AXI4 channels.
 *
 * VALID phases are used when offering a Payload to another agent. READY is
 * used as a response (either immediately/in the same cycle with: phase =
 * ...READY; return TLM_UPDATED, or later with another communicating function
 * call) to a VALID phase. VALID_LAST phases are VALID phases used for last
 * data beats.
 */
enum Phase
{
    /*
     * Bit position meaning:
     * [0] == 1: READY
     * [7:4]: Channel number
     * [7:4] == 0: UNINITIALIZED
     * [7:4] == 1: AW
     * [7:4] == 2: W
     * [7:4] == 3: B
     * [7:4] == 4: AR
     * [7:4] == 5: R
     * [7:4] == 6: AC
     * [7:4] == 7: CR
     * [7:4] == 8: CD
     * [7:4] == 9: WACK
     * [7:4] == A: RACK
     * [8] == 1: LAST
     */

    PHASE_UNINITIALIZED = 0x000,

    AW_VALID      = 0x010,
    AW_READY      = 0x011,

    W_VALID       = 0x020,
    W_VALID_LAST  = 0x120,
    W_READY       = 0x021,

    B_VALID       = 0x030,
    B_READY       = 0x031,

    AR_VALID      = 0x040,
    AR_READY      = 0x041,

    R_VALID       = 0x050,
    R_VALID_LAST  = 0x150,
    R_READY       = 0x051,

    AC_VALID      = 0x060,
    AC_READY      = 0x061,

    CR_VALID      = 0x070,
    CR_READY      = 0x071,

    CD_VALID      = 0x080,
    CD_VALID_LAST = 0x180,
    CD_READY      = 0x081,

    WACK          = 0x090,
    RACK          = 0x0A0

};

}
}

#endif // ARM_AXI4_PHASE_H
