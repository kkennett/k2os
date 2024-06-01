#ifndef __IK2NET_H
#define __IK2NET_H

#include <lib/k2win32.h>

#include <lib/k2atomic.h>
#include <lib/k2mem.h>
#include <lib/k2list.h>
#include <lib/k2tree.h>
#include <lib/k2nethost.h>

typedef struct _K2NET_MSGLINK K2NET_MSGLINK;
struct _K2NET_MSGLINK
{
    K2NET_MSG       Msg;
    K2NET_MSGLINK * mpNext;
};

typedef struct _K2NET_REGPROTO K2NET_REGPROTO;
struct _K2NET_REGPROTO
{
    K2TREE_NODE             TreeNode;       // mUserVal is protocol Id
    K2NET_PROTO_pf_Factory  mfFactory;
};

typedef struct _K2NET_PROTO K2NET_PROTO;
struct _K2NET_PROTO
{
    INT32 volatile          mRefs;
    K2NET_PROTO *           mpParent;
    K2LIST_ANCHOR           ChildList;
    K2LIST_LINK             ChildListLink;
    K2TREE_NODE             TreeNode;       // mUserVal is instance id
    K2NET_PROTO_INSTANCE    Instance;
    K2NET_PROTO_CONTEXT     mProtoContext;
    K2TREE_ANCHOR           TreeAnchor;     // K2NET_OPEN tree
};

typedef struct _K2NET_OPEN K2NET_OPEN;
struct _K2NET_OPEN
{
    INT32 volatile      mRefs;
    K2NET_PROTO *       mpProto;
    K2TREE_NODE         TreeNode;           // mUserVal is pointer to K2NET_OPEN
    K2NET_USAGE_CONTEXT mUsageContext;
    K2NET_OPEN_CONTEXT  mOpenContext;
    K2NET_pf_Doorbell   mfDoorbell;
    K2NET_MSGLINK *     mpFirstMsg;
    K2NET_MSGLINK *     mpLastMsg;
};

typedef struct _K2NET_BUFFER_TRACK K2NET_BUFFER_TRACK;
struct _K2NET_BUFFER_TRACK
{
    K2TREE_NODE     TreeNode;   // mUserVal is buffer pointer
    UINT32          mMTU;
    K2NET_PROTO *   mpProto;
};

extern UINT32           gNextProtoInstanceId;
extern K2TREE_ANCHOR    gRegProtoTree;
extern K2TREE_ANCHOR    gProtoInstTree;
extern K2TREE_ANCHOR    gBufferTree;

void K2NET_Proto_Release(K2NET_PROTO *apProto);

#endif // __IK2NET_H