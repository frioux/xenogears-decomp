#include "common.h"
#include "system/memory.h"
#include "system/model.h"

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

INCLUDE_ASM("asm/battling/nonmatchings/main/misc13", func_80089D5C);

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

INCLUDE_ASM("asm/battling/nonmatchings/main/misc13", func_80089E74);
INCLUDE_ASM("asm/battling/nonmatchings/main/misc13", func_80089EB4);
INCLUDE_ASM("asm/battling/nonmatchings/main/misc13", func_80089F8C);
INCLUDE_ASM("asm/battling/nonmatchings/main/misc13", func_80089FC4);
INCLUDE_ASM("asm/battling/nonmatchings/main/misc13", func_80089FF8);
INCLUDE_ASM("asm/battling/nonmatchings/main/misc13", func_8008A040);
INCLUDE_ASM("asm/battling/nonmatchings/main/misc13", func_8008A0B4);

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
INCLUDE_ASM("asm/battling/nonmatchings/main/misc13", func_8008A184);
INCLUDE_ASM("asm/battling/nonmatchings/main/misc13", func_8008A254);
void func_8008A298(void* pData) {
    HeapFree(pData);
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc13", func_8008A2B8);

void func_8008A3A0(void) {
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc13", func_8008A3A8);
INCLUDE_ASM("asm/battling/nonmatchings/main/misc13", func_8008A3E0);
INCLUDE_ASM("asm/battling/nonmatchings/main/misc13", func_8008A5BC);
