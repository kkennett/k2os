/*   
//   BSD 3-Clause License
//   
//   Copyright (c) 2023, Kurt Kennett
//   All rights reserved.
//   
//   Redistribution and use in source and binary forms, with or without
//   modification, are permitted provided that the following conditions are met:
//   
//   1. Redistributions of source code must retain the above copyright notice, this
//      list of conditions and the following disclaimer.
//   
//   2. Redistributions in binary form must reproduce the above copyright notice,
//      this list of conditions and the following disclaimer in the documentation
//      and/or other materials provided with the distribution.
//   
//   3. Neither the name of the copyright holder nor the names of its
//      contributors may be used to endorse or promote products derived from
//      this software without specific prior written permission.
//   
//   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
//   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
//   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
//   DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
//   FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
//   DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
//   SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
//   CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
//   OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
//   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#ifndef __SPEC_ETHERDEF_INC
#define __SPEC_ETHERDEF_INC

//
// --------------------------------------------------------------------------------- 
//

#define ETHER_FRAME_BYTES                   1518    // 0x5EE
#define ETHER_FRAME_ALIGN_BYTES             1536    // 0x600, 64-byte aligned

#define ETHER_FRAME_MAC_LENGTH              6
#define ETHER_FRAME_TYPE_OR_LENGTH_LENGTH   2
#define ETHER_FRAME_MTU                     1500
#define ETHER_FRAME_FCS_LENGTH              4

#define ETHER_FRAME_HDR_LENGTH              ((ETHER_FRAME_MAC_LENGTH * 2) + ETHER_FRAME_TYPE_OR_LENGTH_LENGTH)
#define ETHER_FRAME_FULL_LENGTH             (ETHER_FRAME_HDR_LENGTH + ETHER_FRAME_MTU + ETHER_FRAME_FCS_LENGTH)

#define ETHER_FRAME_DESTMAC_OFFSET          0
#define ETHER_FRAME_SRCMAC_OFFSET           ETHER_FRAME_MAC_LENGTH
#define ETHER_FRAME_TYPE_OR_LENGTH_OFFSET   (ETHER_FRAME_MAC_LENGTH * 2)
#define ETHER_FRAME_DATA_OFFSET             (ETHER_FRAME_TYPE_OR_LENGTH_OFFSET + ETHER_FRAME_TYPE_OR_LENGTH_LENGTH)

#define ETHER_TYPE_IPV4         0x0800
#define ETHER_TYPE_ARP          0x0806
#define ETHER_TYPE_RARP         0x8035
#define ETHER_TYPE_IPV6         0x86DD
#define ETHER_TYPE_FLOW_CONTROL 0x8808

//
// --------------------------------------------------------------------------------- 
//

#endif // __SPEC_ETHERDEF_INC

