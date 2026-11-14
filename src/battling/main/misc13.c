#include "common.h"
#include "system/memory.h"
#include "system/model.h"

extern s32 D_80091C2C;
extern s16 D_80092800;
extern s16 D_80092804;
extern s16 D_80092808;
extern s16 D_8009280C;

// A model node: a state tag plus a payload whose type depends on the state,
// plus two child nodes of the same type.
typedef struct Misc13Node {
    /* 0x00 */ s32 state;
    /* 0x04 */ void* pData;
    /* 0x08 */ u8 unk08[0x2C];
    /* 0x34 */ s32 unk34;
    /* 0x38 */ s32 unk38;
    /* 0x3C */ s32 unk3C;
    /* 0x40 */ u8 unk40[0x50];
    /* 0x90 */ struct Misc13Node* pChildA;
    /* 0x94 */ struct Misc13Node* pChildB;
} Misc13Node; // Size: unknown

// The payload used by state 1: one model part and the packets allocated for it.
typedef struct {
    /* 0x00 */ s32 flags;
    /* 0x04 */ s32 unk04;
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

// One entry of the list owned by a state-2 payload.
typedef struct {
    /* 0x00 */ s32 unk00;
    /* 0x04 */ void* pData;
    /* 0x08 */ u8 unk08[0xC];
} Misc13Entry; // Size: 0x14

// The payload used by state 2.
typedef struct {
    /* 0x00 */ u16 unk00;
    /* 0x02 */ s16 count;
    /* 0x04 */ void* pData;
    /* 0x08 */ Misc13Entry* pEntries;
} Misc13EntryList; // Size: unknown

// The 0x28C-byte container allocated by BattleCreateLightGroup: four 0x9C-byte nodes
// carved out of the block, plus the parent node it was built for.
typedef struct {
    /* 0x000 */ s32 unk000;
    /* 0x004 */ Misc13Node* pNode0;
    /* 0x008 */ Misc13Node* pNode1;
    /* 0x00C */ Misc13Node* pNode2;
    /* 0x010 */ Misc13Node* pNode3;
    /* 0x014 */ u8 unk014[0xA0];
    /* 0x0B4 */ void* pSprite1; // pNode1->pData
    /* 0x0B8 */ u8 unk0B8[0x98];
    /* 0x150 */ void* pSprite2; // pNode2->pData
    /* 0x154 */ u8 unk154[0x98];
    /* 0x1EC */ void* pSprite3; // pNode3->pData
    /* 0x1F0 */ u8 unk1F0[0x94];
    /* 0x284 */ Misc13Node* pParent;
    /* 0x288 */ u8 unk288;
    /* 0x289 */ u8 unk289;
    /* 0x28A */ u8 unk28A;
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
    /* 0x16 */ u8 unk16;
    /* 0x17 */ u8 unk17;
    /* 0x18 */ u8 unk18[0x50];
} Misc13Buffer; // Size: 0x68

// The 0x1C-byte payload allocated by BattleCreateEntryList.
typedef struct {
    /* 0x00 */ s32 unk00;
    /* 0x04 */ s32 unk04;
    /* 0x08 */ s32 unk08;
    /* 0x0C */ s32 unk0C;
    /* 0x10 */ s16 unk10;
    /* 0x12 */ s16 unk12;
    /* 0x14 */ s16 unk14;
    /* 0x16 */ u16 pad16;
    /* 0x18 */ s32 unk18;
} Misc13Scale; // Size: 0x1C

// The 0x14-byte payload allocated by BattleCreateLight.
typedef struct {
    /* 0x00 */ s32 unk00;
    /* 0x04 */ s32 unk04;
    /* 0x08 */ s32 unk08;
    /* 0x0C */ s16 unk0C;
    /* 0x0E */ s16 unk0E;
    /* 0x10 */ s16 unk10;
    /* 0x12 */ u16 pad12;
} Misc13Sprite; // Size: 0x14

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
        func_8008C120(pNode->pData);
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

Misc13Scale* BattleCreateEntryList(void) {
    Misc13Scale* pSprite;

    HeapSetCurrentContentType(4);
    pSprite = HeapAlloc(0x1C, 0);
    pSprite->unk14 = 0x1000;
    pSprite->unk12 = 0x1000;
    pSprite->unk10 = 0x1000;
    pSprite->unk04 = 0;
    pSprite->unk08 = 0;
    return pSprite;
}

void BattleFreeEntryList(Misc13EntryList* pList) {
    s32 i;

    if (pList->pData != NULL) {
        HeapFree(pList->pData);
    }
    if (pList->pEntries != NULL) {
        if (D_80091C2C == 0) {
            i = pList->count;
            while (--i != -1) {
                if (pList->pEntries[i].unk00 != 0) {
                    HeapFree(pList->pEntries[i].pData);
                }
            }
        }
        HeapFree(pList->pEntries);
    }
}

Misc13ModelPart* BattleInitModelPart(Misc13ModelPart* pPart) {
    pPart->unk04 = 1;
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
    return BattleInitModelPart(HeapAlloc(0x20, 0));
}

void BattleFreeModelPart(Misc13ModelPart* pPart) {
    if (pPart->pPacketBuffer != NULL) {
        HeapDelayedFree(pPart->pPacketBuffer, 2);
    }
    ModelPartFreeLightData(pPart->pModelPart);
}
void BattleLoadImageWithTpageClut(void* pImg) {
    if (D_80092800 > 0) {
        func_8002DDE4(pImg, 1, D_80092800, D_80092804, 1, D_80092808, D_8009280C);
    } else {
        func_8002DDE4(pImg, 0, 0, 0, 0, 0, 0);
    }
}

void BattleSetNodeColor(Misc13Node* pNode, s32 r, s32 g, s32 b) {
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
    D_80092800 = x;
    D_80092804 = y;
}

void BattleSetClut(s16 x, s16 y) {
    D_80092808 = x;
    D_8009280C = y;
}

void BattleSetTpageAndClut(s16 x, s16 y, s16 clutX, s16 clutY) {
    D_80092800 = x;
    D_80092804 = y;
    D_80092808 = clutX;
    D_8009280C = clutY;
}

void BattleClearTpageAndClut(void) {
    D_80092808 = -1;
    D_80092800 = -1;
}
void BattleAttachModelPart(Misc13ModelPart* pPart, ModelPart* pModelPart) {
    pPart->pModelPart = pModelPart;
    pPart->unk10 = func_800303C8(pModelPart, 1);
    ModelAllocatePackets(pPart->pModelPart, &pPart->pPacketBuffer, &pPart->pCurrentPacket);
    if (D_80092800 >= 0) {
        func_8002CC54(GetTPage(0, 1, D_80092800, D_80092804) & 0xFFFF);
    }
    if (D_80092808 >= 0) {
        ModelPacketSetBaseClut(D_80092808, D_8009280C);
    }
    ModelInitializePacket(pPart->pModelPart, pPart->pPacketBuffer, 2);
    func_800732AC(pPart->pCurrentPacket, pPart->pPacketBuffer, pPart->pModelPart->unk20[1].modelPacketSize);
    pPart->flags |= 0x2;
}
Misc13Sprite* BattleCreateLight(void) {
    Misc13Sprite* pSprite;
    Misc13Sprite* pSprite2;
    Misc13Node* pA;
    Misc13Node* pB;
    Misc13Node* pC;

    HeapSetCurrentContentType(0xB);
    pSprite = HeapAlloc(0x14, 0);
    pSprite->unk08 = 0x10;
    pSprite->unk04 = 0x10;
    pSprite->unk00 = 0x10;
    pSprite->unk10 = 0;
    pSprite->unk0E = 0;
    pSprite->unk0C = 0;
    return pSprite;
}
void BattleFreeLight(void* pData) {
    HeapFree(pData);
}

Misc13Buffer* BattleCreateOrderingTablePair(u16 count) {
    Misc13Buffer* pBuffer;
    s32 i;

    HeapSetCurrentContentType(0xC);
    pBuffer = HeapAlloc(0x68, 0);
    HeapSetCurrentContentType(0xC);
    pBuffer->pArrayA = HeapAlloc(count * 8, 0);
    pBuffer->pArrayB = pBuffer->pArrayA + count;
    pBuffer->unk16 = 1;
    pBuffer->unk17 = 0xE;
    pBuffer->unk00 = 0;
    pBuffer->count = count;
    pBuffer->pLastA = &pBuffer->pArrayA[count - 1];
    pBuffer->pLastB = &pBuffer->pArrayB[count - 1];
    i = 1;
    if (count != i) {
        while (i != count) {
            i <<= 1;
            if (i > 0x4000) {
                pBuffer->unk17 = 0xE;
                break;
            }
            pBuffer->unk17--;
        }
    }
    return pBuffer;
}

void func_8008A3A0(void) {
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
    Misc13Sprite* pSprite;
    Misc13Sprite* pSprite2;
    Misc13Node* pA;
    Misc13Node* pB;
    Misc13Node* pC;

    HeapSetCurrentContentType(9);
    pGroup = HeapAlloc(0x28C, 0);
    pNode0 = (Misc13Node*)((u8*)pGroup + 0x14);
    pNode1 = (Misc13Node*)((u8*)pGroup + 0xB0);
    pNode2 = (Misc13Node*)((u8*)pGroup + 0x14C);
    pNode3 = (Misc13Node*)((u8*)pGroup + 0x1E8);
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
    pC->unk38 = -2;
    pB->unk38 = -2;
    pA->unk38 = -2;
    pGroup->pNode1->unk34 = 0;
    pGroup->pNode1->unk3C = 1;
    pGroup->pNode2->unk34 = -1;
    pGroup->pNode2->unk3C = -1;
    pGroup->pNode3->unk34 = 1;
    pGroup->pNode3->unk3C = -1;
    pSprite = (Misc13Sprite*)pGroup->pNode1->pData;
    pSprite->unk10 = 0x800;
    pSprite->unk0E = 0x800;
    pSprite->unk0C = 0x800;
    pSprite2 = (Misc13Sprite*)pGroup->pNode2->pData;
    pSprite2->unk10 = 0;
    pSprite2->unk0E = 0;
    pSprite2->unk0C = 0;
    *(Misc13Sprite*)pGroup->pNode3->pData = *(Misc13Sprite*)pGroup->pNode2->pData;
    pGroup->pParent = pParent;
    pGroup->unk28A = 0;
    pGroup->unk289 = 0;
    pGroup->unk288 = 0;
    pGroup->unk28A = 0x10;
    pGroup->unk289 = 0x10;
    pGroup->unk288 = 0x10;
    func_8008ABAC(&pGroup->pNode1);
    return pGroup;
}

void BattleFreeLightGroup(Misc13Group* pGroup) {
    HeapFree(pGroup->pSprite1);
    HeapFree(pGroup->pSprite2);
    HeapFree(pGroup->pSprite3);
    BattleFreeNodeDelayed(pGroup->pParent);
    HeapFree(pGroup);
}
