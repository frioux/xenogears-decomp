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
    /* 0x08 */ u8 unk08[0x88];
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

// The container freed by func_8008A5BC.
typedef struct {
    /* 0x000 */ u8 unk000[0xB4];
    /* 0x0B4 */ void* pUnk0B4;
    /* 0x0B8 */ u8 unk0B8[0x98];
    /* 0x150 */ void* pUnk150;
    /* 0x154 */ u8 unk154[0x98];
    /* 0x1EC */ void* pUnk1EC;
    /* 0x1F0 */ u8 unk1F0[0x94];
    /* 0x284 */ Misc13Node* pUnk284;
} Misc13Group; // Size: unknown

// The 0x1C-byte payload allocated by func_80089E74.
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

// The 0x14-byte payload allocated by func_8008A254.
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

void func_80089D5C(Misc13Node* pNode) {
    if (pNode == NULL) {
        return;
    }
    func_80089D5C(pNode->pChildB);
    func_80089D5C(pNode->pChildA);
    switch (pNode->state) {
    case 1:
        func_80089FF8(pNode->pData);
        break;
    case 2:
        func_80089EB4(pNode->pData);
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

void func_80089E2C(Misc13Node* pNode, void* pData) {
    pNode->pData = pData;
    pNode->state = 1;
}

void func_80089E3C(Misc13Node* pNode) {
    pNode->state = 3;
}

void func_80089E48(Misc13Node* pNode) {
    pNode->state = 4;
}

void func_80089E54(Misc13Node* pNode, void* pData) {
    pNode->pData = pData;
    pNode->state = 2;
}

void func_80089E64(Misc13Node* pNode, void* pData) {
    pNode->pData = pData;
    pNode->state = 6;
}

Misc13Scale* func_80089E74(void) {
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

INCLUDE_ASM("asm/battling/nonmatchings/main/misc13", func_80089EB4);

/* 90.98% - the loop shape is right, but gcc folds the entry test
   (i != -1 for i = count - 1) into a beqz on count; the original keeps
   addiu + beq against -1. s16/s32/u32 counters all fold or add sll/sra.
void func_80089EB4(Misc13EntryList* pList) {
    s32 i;

    if (pList->pData != NULL) {
        HeapFree(pList->pData);
    }
    if (pList->pEntries != NULL) {
        if (D_80091C2C == 0) {
            i = pList->count - 1;
            while (i != -1) {
                if (pList->pEntries[i].unk00 != 0) {
                    HeapFree(pList->pEntries[i].pData);
                }
                i--;
            }
        }
        HeapFree(pList->pEntries);
    }
}
*/

Misc13ModelPart* func_80089F8C(Misc13ModelPart* pPart) {
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

Misc13ModelPart* func_80089FC4(void) {
    HeapSetCurrentContentType(1);
    return func_80089F8C(HeapAlloc(0x20, 0));
}

void func_80089FF8(Misc13ModelPart* pPart) {
    if (pPart->pPacketBuffer != NULL) {
        HeapDelayedFree(pPart->pPacketBuffer, 2);
    }
    ModelPartFreeLightData(pPart->pModelPart);
}
void func_8008A040(void* pImg) {
    if (D_80092800 > 0) {
        func_8002DDE4(pImg, 1, D_80092800, D_80092804, 1, D_80092808, D_8009280C);
    } else {
        func_8002DDE4(pImg, 0, 0, 0, 0, 0, 0);
    }
}

void func_8008A0B4(Misc13Node* pNode, s32 r, s32 g, s32 b) {
    ((Misc13ModelPart*)pNode->pData)->r = r;
    ((Misc13ModelPart*)pNode->pData)->g = g;
    ((Misc13ModelPart*)pNode->pData)->b = b;
    ((Misc13ModelPart*)pNode->pData)->flags |= 0x10;
}

void func_8008A0F4(Misc13Node* pNode) {
    Misc13ModelPart* pPart = pNode->pData;

    pPart->flags &= ~0x10;
}

void func_8008A110(s16 x, s16 y) {
    D_80092800 = x;
    D_80092804 = y;
}

void func_8008A128(s16 x, s16 y) {
    D_80092808 = x;
    D_8009280C = y;
}

void func_8008A140(s16 x, s16 y, s16 clutX, s16 clutY) {
    D_80092800 = x;
    D_80092804 = y;
    D_80092808 = clutX;
    D_8009280C = clutY;
}

void func_8008A168(void) {
    D_80092808 = -1;
    D_80092800 = -1;
}
void func_8008A184(Misc13ModelPart* pPart, ModelPart* pModelPart) {
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
Misc13Sprite* func_8008A254(void) {
    Misc13Sprite* pSprite;

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
void func_8008A298(void* pData) {
    HeapFree(pData);
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc13", func_8008A2B8);

void func_8008A3A0(void) {
}

void func_8008A3A8(Misc13Node* pNode) {
    HeapDelayedFree(pNode->pData, 3);
    HeapFree(pNode);
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc13", func_8008A3E0);

void func_8008A5BC(Misc13Group* pGroup) {
    HeapFree(pGroup->pUnk0B4);
    HeapFree(pGroup->pUnk150);
    HeapFree(pGroup->pUnk1EC);
    func_8008A3A8(pGroup->pUnk284);
    HeapFree(pGroup);
}
