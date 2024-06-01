#ifndef __IK2NET_H
#define __IK2NET_H

#include <lib/k2win32.h>

#include <lib/k2mem.h>
#include <lib/k2list.h>
#include <lib/k2tree.h>
#include <lib/k2nethost.h>

typedef struct _K2NET_NOTIFY K2NET_NOTIFY;
struct _K2NET_NOTIFY
{
    K2NET_LAYER *       mpLayer;
    K2NET_NotifyType    mNotifyType;
    UINT32              mNotifyData;
    K2NET_NOTIFY *      mpNext;
};

typedef struct _K2NET_REGPHYS_TRACK K2NET_REGPHYS_TRACK;
struct _K2NET_REGPHYS_TRACK
{
    K2NET_PHYS_LAYER *  mpPhysLayer;
    K2LIST_LINK         ListLink;
};

typedef struct _K2NET_REGPROTO_TRACK K2NET_REGPROTO_TRACK;
struct _K2NET_REGPROTO_TRACK
{
    UINT32                      mProtocolId;
    K2NET_pf_ProtocolFactory    mfFactory;
    K2LIST_LINK                 ListLink;
};

typedef struct _K2NET_PROTO_INSTANCE K2NET_PROTO_INSTANCE;
struct _K2NET_PROTO_INSTANCE
{
    K2NET_PROTO_PROP    Prop;
    K2NET_HOST_LAYER *  mpHostLayer;
    K2LIST_LINK         ListLink;
};

extern UINT32           gNextProtoInstanceId;
extern K2LIST_ANCHOR    gProtoList;
extern K2TREE_ANCHOR    gOpenInstTree;
extern K2LIST_ANCHOR    gRegProtoList;
extern K2LIST_ANCHOR    gRegPhysList;
extern K2NET_NOTIFY *   gpNotifyList;
extern K2NET_NOTIFY *   gpNotifyListLast;

K2NET_HOST_LAYER * K2IPV4_ProtocolFactory(UINT32 aProtocolId, K2NET_STACK_PROTO_CHUNK_HDR const *apConfig);

#endif // __IK2NET_H