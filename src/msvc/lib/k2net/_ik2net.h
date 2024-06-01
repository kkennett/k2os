#ifndef __IK2NET_H
#define __IK2NET_H

#include <lib/k2mem.h>
#include <lib/k2tree.h>
#include <lib/k2net.h>

typedef struct _K2NET_RUNNING_PAIR K2NET_RUNNING_PAIR;
struct _K2NET_RUNNING_PAIR
{
    K2NET_HOSTING_OID *     mpProtocolHostOid;
    K2NET_HW *              mpHw;
    K2LIST_LINK             ListLink;
    K2NET_LINK_MAP_CONFIG * mpLinkMapConfig;
};

typedef struct _K2NET_REG_PROTO K2NET_REG_PROTO;
struct _K2NET_REG_PROTO
{
    UINT32                          mProto;
    K2NET_pf_ProtocolHostFactory    mfHostFactory;
    K2LIST_LINK                     ListLink;
};

typedef struct _K2NET_REG_HW K2NET_REG_HW;
struct _K2NET_REG_HW
{
    K2NET_HW *  mpHw;
    K2LIST_LINK ListLink;
};

#endif // __IK2NET_H