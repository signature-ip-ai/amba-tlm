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

#ifndef ARM_TLM_PROTOCOL_H
#define ARM_TLM_PROTOCOL_H

namespace ARM
{
namespace TLM
{

/** AMBA protocols possible on ARM::TLM sockets. */
enum Protocol
{
    PROTOCOL_ACE          = 0,
    PROTOCOL_ACE_LITE     = 1,
    PROTOCOL_ACE_LITE_DVM = 2,
    PROTOCOL_AXI4         = 3,
    PROTOCOL_AXI4_LITE    = 4,
    PROTOCOL_CHI          = 5,
    /* RESERVED           = 6, */
    PROTOCOL_CHIE         = 7,
    PROTOCOL_AHB          = 8,

    /*
     * Catch all value for when Protocol isn't used to distinguish one port
     * type from another.
     */
    PROTOCOL_OTHER        = 255
};

}
}

#endif // ARM_TLM_PROTOCOL_H
