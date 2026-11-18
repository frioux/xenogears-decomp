#include "common.h"
#include "system/memory.h"
#include "system/model.h"

extern s32 g_BattleSkipEntryFree;
extern s16 g_BattleTpageX;
extern s16 g_BattleTpageY;
extern s16 g_BattleClutX;
extern s16 g_BattleClutY;

// A model node: a state tag plus a payload whose type depends on the state,
// the matrices it is placed with, and two child nodes of the same type.
typedef struct Misc13Node {
    /* 0x00 */ s32 state;
    /* 0x04 */ void* pData;
    /* 0x08 */ void (*pUpdate)(void);
    /* 0x0C */ MATRIX localMatrix;
    /* 0x2C */ SVECTOR sourcePos;
    /* 0x34 */ s32 positionX;
    /* 0x38 */ s32 positionY;
    /* 0x3C */ s32 positionZ;
    /* 0x40 */ u32 pad40;
    /* 0x44 */ SVECTOR rotation;
    /* 0x4C */ MATRIX rotMatrix;
    /* 0x6C */ MATRIX worldMatrix;
    /* 0x8C */ void* pMatrices;
    /* 0x90 */ struct Misc13Node* pChildA;
    /* 0x94 */ struct Misc13Node* pChildB;
    /* 0x98 */ u32 pad98;
} Misc13Node; // Size: 0x9C

// The payload used by state 1: one model part and the packets allocated for it.
typedef struct {
    /* 0x00 */ s32 flags;
    /* 0x04 */ s32 renderMode;
    /* 0x08 */ void* pPacketBuffer;
    /* 0x0C */ void* pCurrentPacket;
    /* 0x10 */ s32 unk10;
    /* 0x14 */ ModelPart* pModelPart;
    /* 0x18 */ u8 r;
    /* 0x19 */ u8 g;
    /* 0x1A */ u8 b;
    /* 0x1B */ u8 pad1B;
    /* 0x1C */ s32 unk1C;
} Misc13ModelPart; // Size: 0x20

// One entry of the list owned by a state-2 payload: the animation of one part,
// which the animation reader declares as Misc14Anim.
typedef struct {
    /* 0x00 */ void* pConfig;
    /* 0x04 */ void* pData;
    /* 0x08 */ u8* pTracks;
    /* 0x0C */ u32 unk0C;
    /* 0x10 */ u16 unk10;
    /* 0x12 */ s16 position;
} Misc13Entry; // Size: 0x14

// The payload used by state 2.
typedef struct {
    /* 0x00 */ u16 nodeCount; // nodes in the table at 0x04
    /* 0x02 */ s16 count;
    /* 0x04 */ void* pData;
    /* 0x08 */ Misc13Entry* pEntries;
    /* 0x0C */ s32 unk0C;
    /* 0x10 */ s16 scaleX; // 0x1000
    /* 0x12 */ s16 scaleY;
    /* 0x14 */ s16 scaleZ;
    /* 0x16 */ u16 pad16;
    /* 0x18 */ s32 unk18;
} Misc13EntryList; // Size: 0x1C

// The 0x28C-byte container allocated by BattleCreateLightGroup: four 0x9C-byte nodes
// at the head of the block, plus the parent node it was built for.
typedef struct {
    /* 0x000 */ s32 unk000;
    /* 0x004 */ Misc13Node* pNode0;
    /* 0x008 */ Misc13Node* pNode1;
    /* 0x00C */ Misc13Node* pNode2;
    /* 0x010 */ Misc13Node* pNode3;
    /* 0x014 */ Misc13Node nodes[4];
    /* 0x284 */ Misc13Node* pParent;
    /* 0x288 */ u8 r;
    /* 0x289 */ u8 g;
    /* 0x28A */ u8 b;
    /* 0x28B */ u8 pad28B;
} Misc13Group; // Size: 0x28C

// The 0x68-byte buffer allocated by BattleCreateOrderingTablePair: two parallel u32 arrays
// carved out of one block, plus pointers to their last elements.
typedef struct {
    /* 0x00 */ s32 unk00;
    /* 0x04 */ u32* pArrayA;
    /* 0x08 */ u32* pArrayB;
    /* 0x0C */ u32* pLastA;
    /* 0x10 */ u32* pLastB;
    /* 0x14 */ u16 count;
    /* 0x16 */ u8 flags;
    /* 0x17 */ u8 depthShift;
    /* 0x18 */ u8 unk18[0x50];
} Misc13Buffer; // Size: 0x68

// The 0x14-byte light payload allocated by BattleCreateLight: the direction
// func_80030A30 normalizes into the light vector table, and its colour.
typedef struct {
    /* 0x00 */ s32 dirX;
    /* 0x04 */ s32 dirY;
    /* 0x08 */ s32 dirZ;
    /* 0x0C */ s16 r;
    /* 0x0E */ s16 g;
    /* 0x10 */ s16 b;
    /* 0x12 */ u16 pad12;
} Misc13Light; // Size: 0x14

extern void HeapSetCurrentContentType(u_short contentTag);
extern void ModelPartFreeLightData(ModelPart* pModelPart);
extern void HeapDelayedFree(void* pMem, u_int delay);

void BattleFreeNodeTree(Misc13Node* pNode) {
    if (pNode == NULL) {
        return;
    }
    BattleFreeNodeTree(pNode->pChildB);
    BattleFreeNodeTree(pNode->pChildA);
    switch (pNode->state) {
    case 1:
        BattleFreeModelPart(pNode->pData);
        break;
    case 2:
        BattleFreeEntryList(pNode->pData);
        break;
    case 5:
        BattleFreeWorkBlock(pNode->pData);
        break;
    }
    if (pNode->pData != NULL) {
        HeapFree(pNode->pData);
    }
    HeapFree(pNode);
}

void BattleSetNodeModelPart(Misc13Node* pNode, void* pData) {
    pNode->pData = pData;
    pNode->state = 1;
}

void BattleSetNodeState3(Misc13Node* pNode) {
    pNode->state = 3;
}

void BattleSetNodeState4(Misc13Node* pNode) {
    pNode->state = 4;
}

void BattleSetNodeEntryList(Misc13Node* pNode, void* pData) {
    pNode->pData = pData;
    pNode->state = 2;
}

void BattleSetNodeLight(Misc13Node* pNode, void* pData) {
    pNode->pData = pData;
    pNode->state = 6;
}

Misc13EntryList* BattleCreateEntryList(void) {
    Misc13EntryList* pList;

    HeapSetCurrentContentType(4);
    pList = HeapAlloc(sizeof(Misc13EntryList), 0);
    pList->scaleZ = 0x1000;
    pList->scaleY = 0x1000;
    pList->scaleX = 0x1000;
    pList->pData = 0;
    pList->pEntries = 0;
    return pList;
}

void BattleFreeEntryList(Misc13EntryList* pList) {
    s32 i;

    if (pList->pData != NULL) {
        HeapFree(pList->pData);
    }
    if (pList->pEntries != NULL) {
        if (g_BattleSkipEntryFree == 0) {
            i = pList->count;
            while (--i != -1) {
                if (pList->pEntries[i].pConfig != 0) {
                    HeapFree(pList->pEntries[i].pData);
                }
            }
        }
        HeapFree(pList->pEntries);
    }
}

Misc13ModelPart* BattleInitModelPart(Misc13ModelPart* pPart) {
    pPart->renderMode = 1;
    pPart->flags = 0;
    pPart->pPacketBuffer = NULL;
    pPart->pCurrentPacket = NULL;
    pPart->unk10 = 0;
    pPart->pModelPart = NULL;
    pPart->unk1C = 0;
    pPart->b = 0x40;
    pPart->g = 0x40;
    pPart->r = 0x40;
    return pPart;
}

Misc13ModelPart* BattleCreateModelPart(void) {
    HeapSetCurrentContentType(1);
    return BattleInitModelPart(HeapAlloc(sizeof(Misc13ModelPart), 0));
}

void BattleFreeModelPart(Misc13ModelPart* pPart) {
    if (pPart->pPacketBuffer != NULL) {
        HeapDelayedFree(pPart->pPacketBuffer, 2);
    }
    ModelPartFreeLightData(pPart->pModelPart);
}
void BattleLoadImageWithTpageClut(void* pImg) {
    if (g_BattleTpageX > 0) {
        func_8002DDE4(pImg, 1, g_BattleTpageX, g_BattleTpageY, 1, g_BattleClutX, g_BattleClutY);
    } else {
        func_8002DDE4(pImg, 0, 0, 0, 0, 0, 0);
    }
}

void BattleSetNodeColor(Misc13Node* pNode, s32 r, s32 g, s32 b) {
    // the original reloads pData for each store, so the cast stays at every use
    ((Misc13ModelPart*)pNode->pData)->r = r;
    ((Misc13ModelPart*)pNode->pData)->g = g;
    ((Misc13ModelPart*)pNode->pData)->b = b;
    ((Misc13ModelPart*)pNode->pData)->flags |= 0x10;
}

void BattleClearNodeColor(Misc13Node* pNode) {
    Misc13ModelPart* pPart = pNode->pData;

    pPart->flags &= ~0x10;
}

void BattleSetTpage(s16 x, s16 y) {
    g_BattleTpageX = x;
    g_BattleTpageY = y;
}

void BattleSetClut(s16 x, s16 y) {
    g_BattleClutX = x;
    g_BattleClutY = y;
}

void BattleSetTpageAndClut(s16 x, s16 y, s16 clutX, s16 clutY) {
    g_BattleTpageX = x;
    g_BattleTpageY = y;
    g_BattleClutX = clutX;
    g_BattleClutY = clutY;
}

void BattleClearTpageAndClut(void) {
    g_BattleClutX = -1;
    g_BattleTpageX = -1;
}
void BattleAttachModelPart(Misc13ModelPart* pPart, ModelPart* pModelPart) {
    pPart->pModelPart = pModelPart;
    pPart->unk10 = func_800303C8(pModelPart, 1);
    ModelAllocatePackets(pPart->pModelPart, &pPart->pPacketBuffer, &pPart->pCurrentPacket);
    if (g_BattleTpageX >= 0) {
        ModelPacketSetTexPageOverride(GetTPage(0, 1, g_BattleTpageX, g_BattleTpageY) & 0xFFFF);
    }
    if (g_BattleClutX >= 0) {
        ModelPacketSetBaseClut(g_BattleClutX, g_BattleClutY);
    }
    ModelInitializePacket(pPart->pModelPart, pPart->pPacketBuffer, 2);
    func_800732AC(pPart->pCurrentPacket, pPart->pPacketBuffer, pPart->pModelPart->unk20[1].modelPacketSize);
    pPart->flags |= 0x2;
}
Misc13Light* BattleCreateLight(void) {
    Misc13Light* pLight;

    HeapSetCurrentContentType(0xB);
    pLight = HeapAlloc(sizeof(Misc13Light), 0);
    pLight->dirZ = 0x10;
    pLight->dirY = 0x10;
    pLight->dirX = 0x10;
    pLight->b = 0;
    pLight->g = 0;
    pLight->r = 0;
    return pLight;
}
void BattleFreeLight(void* pData) {
    HeapFree(pData);
}

Misc13Buffer* BattleCreateOrderingTablePair(u16 count) {
    Misc13Buffer* pBuffer;
    s32 i;

    HeapSetCurrentContentType(0xC);
    pBuffer = HeapAlloc(sizeof(Misc13Buffer), 0);
    HeapSetCurrentContentType(0xC);
    pBuffer->pArrayA = HeapAlloc(count * 2 * sizeof(u32), 0);
    pBuffer->pArrayB = pBuffer->pArrayA + count;
    pBuffer->flags = 1;
    pBuffer->depthShift = 0xE;
    pBuffer->unk00 = 0;
    pBuffer->count = count;
    pBuffer->pLastA = &pBuffer->pArrayA[count - 1];
    pBuffer->pLastB = &pBuffer->pArrayB[count - 1];
    i = 1;
    if (count != i) {
        while (i != count) {
            i <<= 1;
            if (i > 0x4000) {
                pBuffer->depthShift = 0xE;
                break;
            }
            pBuffer->depthShift--;
        }
    }
    return pBuffer;
}

void BattlingNoOp(void) {
}

void BattleFreeNodeDelayed(Misc13Node* pNode) {
    HeapDelayedFree(pNode->pData, 3);
    HeapFree(pNode);
}

Misc13Group* BattleCreateLightGroup(Misc13Node* pParent) {
    Misc13Group* pGroup;
    Misc13Node* pNode0;
    Misc13Node* pNode1;
    Misc13Node* pNode2;
    Misc13Node* pNode3;
    Misc13Light* pLight;
    Misc13Light* pLightB;
    Misc13Node* pA;
    Misc13Node* pB;
    Misc13Node* pC;

    HeapSetCurrentContentType(9);
    pGroup = HeapAlloc(sizeof(Misc13Group), 0);
    pNode0 = &pGroup->nodes[0];
    pNode1 = &pGroup->nodes[1];
    pNode2 = &pGroup->nodes[2];
    pNode3 = &pGroup->nodes[3];
    pGroup->unk000 = 0;
    pGroup->pNode0 = pNode0;
    pGroup->pNode1 = pNode1;
    pGroup->pNode2 = pNode2;
    pGroup->pNode3 = pNode3;
    BattleResetObject(pNode0);
    BattleResetObject(pNode1);
    BattleResetObject(pNode2);
    BattleResetObject(pNode3);
    BattleSetNodeLight(pGroup->pNode1, BattleCreateLight());
    BattleSetNodeLight(pGroup->pNode2, BattleCreateLight());
    BattleSetNodeLight(pGroup->pNode3, BattleCreateLight());
    pA = pGroup->pNode1;
    pB = pGroup->pNode2;
    pC = pGroup->pNode3;
    pC->positionY = -2;
    pB->positionY = -2;
    pA->positionY = -2;
    pGroup->pNode1->positionX = 0;
    pGroup->pNode1->positionZ = 1;
    pGroup->pNode2->positionX = -1;
    pGroup->pNode2->positionZ = -1;
    pGroup->pNode3->positionX = 1;
    pGroup->pNode3->positionZ = -1;
    pLight = (Misc13Light*)pGroup->pNode1->pData;
    pLight->b = 0x800;
    pLight->g = 0x800;
    pLight->r = 0x800;
    pLightB = (Misc13Light*)pGroup->pNode2->pData;
    pLightB->b = 0;
    pLightB->g = 0;
    pLightB->r = 0;
    *(Misc13Light*)pGroup->pNode3->pData = *(Misc13Light*)pGroup->pNode2->pData;
    pGroup->pParent = pParent;
    pGroup->b = 0;
    pGroup->g = 0;
    pGroup->r = 0;
    pGroup->b = 0x10;
    pGroup->g = 0x10;
    pGroup->r = 0x10;
    BattleSetModelLights(&pGroup->pNode1);
    return pGroup;
}

void BattleFreeLightGroup(Misc13Group* pGroup) {
    HeapFree(pGroup->nodes[1].pData);
    HeapFree(pGroup->nodes[2].pData);
    HeapFree(pGroup->nodes[3].pData);
    BattleFreeNodeDelayed(pGroup->pParent);
    HeapFree(pGroup);
}
