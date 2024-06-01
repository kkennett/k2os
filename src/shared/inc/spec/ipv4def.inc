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
#ifndef __SPEC_IPV4DEF_INC
#define __SPEC_IPV4DEF_INC

//
// --------------------------------------------------------------------------------- 
//

#define ARP_OPER_REQUEST                    1
#define ARP_OPER_REPLY                      2

#define ARP_PACKET_OFFSET_HTYPE             0
#define ARP_PACKET_OFFSET_PTYPE             2
#define ARP_PACKET_OFFSET_HLEN              4
#define ARP_PACKET_OFFSET_PLEN              5
#define ARP_PACKET_OFFSET_OPER              6
#define ARP_PACKET_OFFSET_SHA               8   // length is HLEN
// followed by SPA[PLEN]
// followed by THA[HLEN]
// followed by TPA[PLEN]

#define IPV4_PROTO_ICMP                     1
#define IPV4_PROTO_IGMP                     2
#define IPV4_PROTO_TCP                      6
#define IPV4_PROTO_UDP                      17
#define IPV4_PROTO_OSPF                     89
#define IPV4_PROTO_SCTP                     132

#define IPV4_MIN_PACKET_LENGTH              576
#define IPV4_MAX_PACKET_LENGTH              0xFFFF

#define IPV4_HDR_FLAGS_RESERVED_0           0x80
#define IPV4_HDR_FLAGS_DF                   0x40
#define IPV4_HDR_FLAGS_MF                   0x20

#define IPV4_HDR_OFFSET_VERSION_IHL         0
#define IPV4_HDR_OFFSET_DSCP_ECN            1
#define IPV4_HDR_OFFSET_TOTLEN_HI           2
#define IPV4_HDR_OFFSET_TOTLEN_LO           3
#define IPV4_HDR_OFFSET_IDENT_HI            4
#define IPV4_HDR_OFFSET_IDENT_LO            5
#define IPV4_HDR_OFFSET_FLAGS_FRAGOFF_HI    6
#define IPV4_HDR_OFFSET_FRAGOFF_LO          7
#define IPV4_HDR_OFFSET_TTL                 8
#define IPV4_HDR_OFFSET_PROTOCOL            9
#define IPV4_HDR_OFFSET_CHKSUM_HI           10
#define IPV4_HDR_OFFSET_CHKSUM_LO           11
#define IPV4_HDR_OFFSET_SRC_IPADDR          12
#define IPV4_HDR_OFFSET_DST_IPADDR          16
#define IPV4_HDR_STD_LENGTH                 20

#define UDP_HDR_OFFSET_PORT_SRC_HI          0
#define UDP_HDR_OFFSET_PORT_SRC_LO          1
#define UDP_HDR_OFFSET_PORT_DST_HI          2
#define UDP_HDR_OFFSET_PORT_DST_LO          3
#define UDP_HDR_OFFSET_LENGTH_HI            4
#define UDP_HDR_OFFSET_LENGTH_LO            5
#define UDP_HDR_OFFSET_CHKSUM_HI            6
#define UDP_HDR_OFFSET_CHKSUM_LO            7
#define UDP_HDR_LENGTH                      8

#define UDP_PSEUDO_OFFSET_SRC_IPADDR        0
#define UDP_PSEUDO_OFFSET_DST_IPADDR        4
#define UDP_PSEUDO_OFFSET_ZERO              8
#define UDP_PSEUDO_OFFSET_PROTO_NUM         9
#define UDP_PSEUDO_OFFSET_LENGTH_HI         10
#define UDP_PSEUDO_OFFSET_LENGTH_LO         11
#define UDP_PSEUDO_LENGTH                   12

#define UDP_PORT_DNS                        53
#define UDP_PORT_DHCP_SERVER                67
#define UDP_PORT_DHCP_CLIENT                68

#define DHCP_FLAGS_HI_BROADCAST             0x80    // RFC 1542

#define DHCP_OP_REQUEST                     1
#define DHCP_OP_RESPONSE                    2

#define DHCP_OPTIONS_COOKIE_0               99
#define DHCP_OPTIONS_COOKIE_1               130
#define DHCP_OPTIONS_COOKIE_2               83
#define DHCP_OPTIONS_COOKIE_3               99

#define DHCP_OPTION_PAD                     0       // has no length field
#define DHCP_OPTION_END                     255     // has no length field

#define DHCP_OPTION_SUBNET_MASK             1
#define DHCP_OPTION_TIME_OFFSET             2
#define DHCP_OPTION_ROUTERS                 3
#define DHCP_OPTION_TIME_SERVERS            4
#define DHCP_OPTION_DNS_SERVERS             6
#define DHCP_OPTION_HOST_NAME               12
#define DHCP_OPTION_BOOT_FILE_SIZE          13
#define DHCP_OPTION_DOMAIN_NAME             15
#define DHCP_OPTION_IP_FORWARDING           19
#define DHCP_OPTION_IP_TTL                  23
#define DHCP_OPTION_INTERFACE_MTU           26
#define DHCP_OPTION_ALL_SUBS_LOCAL          27
#define DHCP_OPTION_BROADCAST_ADDR          28
#define DHCP_OPTION_STATIC_ROUTES           33
#define DHCP_OPTION_ARP_TIMEOUT             35
#define DHCP_OPTION_TCP_TTL                 37
#define DHCP_OPTION_TCP_KEEPALIVE           38
#define DHCP_OPTION_REQUEST_IP              50
#define DHCP_OPTION_LEASE_TIME              51
#define DHCP_OPTION_OPT_OVERLOAD            52
#define DHCP_OPTION_MSGTYPE                 53
#define DHCP_OPTION_SERVER_ID               54
#define DHCP_OPTION_PARAM_REQ               55
#define DHCP_OPTION_ERROR_MSG               56
#define DHCP_OPTION_MAX_DHCPMSG_SIZE        57
#define DHCP_OPTION_T1_VALUE                58
#define DHCP_OPTION_T2_VALUE                59
#define DHCP_OPTION_CLIENT_ID               61
#define DHCP_OPTION_TFTP_SERVER             66
#define DHCP_OPTION_BOOTFILE                67

#define DHCP_MSGTYPE_INVALID                0
#define DHCP_MSGTYPE_DHCPDISCOVER           1
#define DHCP_MSGTYPE_DHCPOFFER              2
#define DHCP_MSGTYPE_DHCPREQUEST            3
#define DHCP_MSGTYPE_DHCPDECLINE            4
#define DHCP_MSGTYPE_DHCPACK                5
#define DHCP_MSGTYPE_DHCPNAK                6
#define DHCP_MSGTYPE_DHCPRELEASE            7
#define DHCP_MSGTYPE_DHCPINFORM             8
#define DHCP_MSGTYPE_DHCPFORCERENEW         9
#define DHCP_MSGTYPE_DHCPLEASEQUERY         10
#define DHCP_MSGTYPE_DHCPLEASEUNASSIGNED    11
#define DHCP_MSGTYPE_DHCPLEASEUNKNOWN       12
#define DHCP_MSGTYPE_DHCPLEASEACTIVE        13
#define DHCP_MSGTYPE_DHCPBULKLEASEQUERY     14
#define DHCP_MSGTYPE_DHCPLEASEQUERYDONE     15
#define DHCP_MSGTYPE_DHCPACTIVELEASEQUERY   16
#define DHCP_MSGTYPE_DHCPLEASEQUERYSTATUS   17
#define DHCP_MSGTYPE_DHCPTLS                18
#define DHCP_MSGTYPE_COUNT                  19

#define DHCP_PACKET_OFFSET_OP               0
#define DHCP_PACKET_OFFSET_HTYPE            1
#define DHCP_PACKET_OFFSET_HLEN             2
#define DHCP_PACKET_OFFSET_HOPS             3
#define DHCP_PACKET_OFFSET_XID              4
#define DHCP_PACKET_OFFSET_SECS             8
#define DHCP_PACKET_OFFSET_FLAGS_HI         10
#define DHCP_PACKET_OFFSET_FLAGS_LO         11
#define DHCP_PACKET_OFFSET_CIADDR           12
#define DHCP_PACKET_OFFSET_YIADDR           16
#define DHCP_PACKET_OFFSET_SIADDR           20
#define DHCP_PACKET_OFFSET_GIADDR           24
#define DHCP_PACKET_OFFSET_CHADDR           28
#define DHCP_PACKET_OFFSET_SNAME            44
#define DHCP_PACKET_OFFSET_FILE             108
#define DHCP_PACKET_OFFSET_OPTIONS          236

#define DHCP_PACKET_CHADDR_LENGTH           16
#define DHCP_PACKET_SNAME_LENGTH            64
#define DHCP_PACKET_FILE_LENGTH             128

#define DNS_RRCLASS_INET                    1

#define DNS_RRTYPE_ADDRESS                  1   // A
#define DNS_RRTYPE_NAME_SERVER              2   // NS
#define DNS_RRTYPE_COMP_NAME                5   // CNAME
#define DNS_RRTYPE_AUTHOR_START             6   // SOA
#define DNS_RRTYPE_POINTER                  12  // PTR
#define DNS_RRTYPE_MAIL_EX                  14  // MX
#define DNS_RRTYPE_TEXT                     16  // TXT

#define DNS_HDR_OFFSET_IDENT                0
#define DNS_HDR_OFFSET_FLAGCODE_HI          2
#define DNS_HDR_OFFSET_FLAGCODE_LO          3
#define DNS_HDR_OFFSET_QUEST_COUNT          4
#define DNS_HDR_OFFSET_ANS_COUNT            6
#define DNS_HDR_OFFSET_NSREC_COUNT          8
#define DNS_HDR_OFFSET_ADD_COUNT            10
#define DNS_HDR_OFFSET_QUEST1               12

#define DNS_FLAGCODE_RESPONSE               0x8000
#define DNS_FLAGCODE_OPCODE_MASK            0x7800
#define DNS_FLAGCODE_OPCODE_SHL             11
#define DNS_FLAGCODE_AA                     0x0400
#define DNS_FLAGCODE_TRUNC                  0x0280
#define DNS_FLAGCODE_REC_DESIRED            0x0100
#define DNS_FLAGCODE_REC_SUPPORTED          0x0080
#define DNS_FLAGCODE_RCODE_MASK             0x000F

#define DNS_OPCODE_QUERY                    0
#define DNS_OPCODE_IQUERY                   1
#define DNS_OPCODE_STATUS                   2
#define DNS_OPCODE_NOTIFY                   4
#define DNS_OPCODE_UPDATE                   5

#define TCP_PORT_DNS                        53

#define ICMP_HDR_OFFSET_TYPE                0
#define ICMP_HDR_OFFSET_CODE                1
#define ICMP_HDR_OFFSET_CHKSUM_HI           2
#define ICMP_HDR_OFFSET_CHKSUM_LO           3
#define ICMP_HDR_OFFSET_REST                4

#define ICMP_TYPE_ECHO_REPLY                0
#define ICMP_TYPE_DEST_UNREACH              3
#define ICMP_TYPE_SOURCE_QUENCY             4
#define ICMP_TYPE_REDIRECT                  5
#define ICMP_TYPE_ECHO_REQUEST              8
#define ICMP_TYPE_ROUTER_ADVERT             9
#define ICMP_TYPE_ROUTER_SOLICIT            10
#define ICMP_TYPE_TIME_EXCEEDED             11
#define ICMP_TYPE_BAD_HEADER                12
#define ICMP_TYPE_TIMESTAMP                 13
#define ICMP_TYPE_TIMESTAMP_REPLY           14

#define ICMP_UNREACH_NETWORK                0
#define ICMP_UNREACH_HOST                   1
#define ICMP_UNREACH_PROTOCOL               2
#define ICMP_UNREACH_PORT                   3
#define ICMP_UNREACH_DATA_TOO_BIG           4
#define ICMP_UNREACH_FAILED_SOURCE          5
#define ICMP_UNREACH_UNK_DEST_NET           6
#define ICMP_UNREACH_UNK_DEST_HOST          7
#define ICMP_UNREACH_SRC_ISOLATED           8
#define ICMP_UNREACH_DEST_NET_NO            9
#define ICMP_UNREACH_DEST_HOST_NO           10
#define ICMP_UNREACH_NET_TOS                11
#define ICMP_UNREACH_HOST_TOS               12
#define ICMP_UNREACH_DENIED                 13
#define ICMP_UNREACH_PRECED_BAD             14
#define ICMP_UNREACH_PRECED_CUTOFF          15

#define ICMP_PING_OFFSET_IDENT              4
#define ICMP_PING_OFFSET_SEQNO              6
#define ICMP_PING_OFFSET_PAYLOAD            8
#define ICMP_PING_MIN_LENGTH                12


//
// --------------------------------------------------------------------------------- 
//

#endif // __SPEC_IPV4DEF_INC

