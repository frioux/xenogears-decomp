#ifndef _XENO_MAIN_H
#define _XENO_MAIN_H

#define NUM_GAME_STATE_OVERLAYS 0x7

#define ERR_KERNEL 0x80

typedef struct {
    void (*pFnMain)(void);
    void* pMemStart;
    void* pHeapStart;
    int hasOverlay;
} MainGameState;

// Header of one entry of the packet-node stream at g_BattlePacketNodeCursor.
typedef struct {
    u_char flags;
    u_char pad1;
    short count; // passed to the handler as its second argument
} UnknownObject;

// The leading 0x1C bytes of include/system/model.h's ModelPart, as
// ModelRenderPacket/ModelInitializePacket read a model part.
typedef struct {
    /* 0x00 */ u_short flags;
    /* 0x02 */ u_short pad2;
    /* 0x04 */ u_short polyCount;
    /* 0x06 */ u_short nodeCount;
    /* 0x08 */ void *pVertices;
    /* 0x0C */ void *pNormals;
    /* 0x10 */ UnknownObject *pNodes;
    /* 0x14 */ void *pColors;
    /* 0x18 */ void *pLightData;
} UnknownObjectContainer; // size: 0x1C


typedef void (*UnknownHandler_t)(UnknownObject *, short);

// One packet-node type: ModelRenderPacket picks handlers[renderMode] and calls it per
// node; ModelInitializePacket calls pFnInitPrims per node and then advances the three
// cursors (packet-node stream, model packet, primitive list) by the strides below.
// The packet node's first byte picks the entry (0 = triangles, 8 = quads).
typedef struct {
    /* 0x00 */ UnknownHandler_t handlers[6];
    /* 0x18 */ int pFnInitPrims; // ModelPacketInit*: fills the node's primitives in the packet
    /* 0x1C */ int nodeStride; // bytes to advance g_BattlePacketNodeCursor per node
    /* 0x20 */ int packetStride; // bytes to advance g_pCurModelPacket per node
    /* 0x24 */ int primsStride; // bytes to advance the primitive list handed to pFnInitPrims
} UnknownHandlerBlock;

/*
 * Entry 0:
 * handlers[0] = 0x8002E038
 * handlers[1] = func_8002ED20
 * handlers[2] = 0x8002E470
 * handlers[3] = 0x8002E8DC
 * handlers[4] = 0x8002E038
 * handlers[5] = 0x8002E038
 * pFnInitPrims = func_8002CDCC (ModelPacketInitPolyF3)
 * nodeStride = 0x00000008
 * packetStride = 0x00000004
 * primsStride = 0x00000014
 */

extern UnknownHandlerBlock g_UnknownHandlerBlocks[17];

extern void* g_MainGameStateOverlayBuffer;
extern MainGameState g_MainGameStates[];

extern int g_GameStateOverlayArchiveOffsets[NUM_GAME_STATE_OVERLAYS];

extern void ClearMemory(u32* pStart, u32* pEnd);
extern void MainLoop(int) __attribute__((noreturn));
extern void GameHandleError(unsigned int errorCode, unsigned int sourceAddress) __attribute__((noreturn));


#endif