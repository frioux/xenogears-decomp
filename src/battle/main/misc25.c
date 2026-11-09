#include "common.h"

extern u8 D_800C37C8;
extern u16 D_800C3E30;
extern u8 D_800C3D6C;
extern u8 *D_800C3BAC[];
extern s32 D_800C3BCC[];
extern s32 D_800C3BEC;
extern s32 D_800C3BF0;
extern void *D_800658C8;

extern void HeapFree(void *pMem);
extern s32 func_800A2330(void);
extern void func_800A23E8(s32, u8 *);
extern s16 func_800A579C(s16 *);
extern void func_800A5870(s16 *, s16, void *);
extern s32 func_800A5914(s16 *, s16, s32);
extern void func_8003852C(s32);
extern void func_800B00D0(void);

typedef struct BattleModelNode BattleModelNode;

// Node of a 0x7C-stride hierarchy whose element 0 heads the list.
typedef struct {
    /* 0x0 */ u8 pad0[0x4];
    /* 0x4 */ BattleModelNode *pNodes;
} BattleModel;

struct BattleModelNode {
    /* 0x0 */ BattleModelNode *pParent;
    /* 0x4 */ u8 pad4[0x3];
    /* 0x7 */ u8 unk7;
    /* 0x8 */ u8 pad8[0x2];
    /* 0xA */ u16 unkA;
    /* 0xC */ u8 padC[0x70];
}; // Size: 0x7C

// Current frame published to the rest of the battle code.
typedef struct {
    /* 0x0 */ mem_addr unk0;
    /* 0x4 */ mem_addr unk4;
    /* 0x8 */ mem_addr unk8;
    /* 0xC */ mem_addr unkC;
    /* 0x10 */ mem_addr unk10;
    /* 0x14 */ s32 unk14;
    /* 0x18 */ s32 unk18;
} BattleFrame; // Size: 0x1C

// Streamed battle data; only the fields this unit reads are named.
typedef struct {
    /* 0x0 */ u8 pad0[0x34C];
    /* 0x34C */ s16 unk34C;
    /* 0x34E */ s16 unk34E;
    /* 0x350 */ s16 unk350;
    /* 0x352 */ s16 unk352;
} BattleStreamData;

extern BattleFrame D_800C3BD0;

INCLUDE_ASM("asm/battle/nonmatchings/main/misc25", func_800AF2C4);

s32 func_800AF400(void) {
    s32 i;

    for (i = 0; i < 13; i++) {
        if ((D_800C3E30 >> i) & 1) {
            break;
        }
    }
    return i;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc25", func_800AF438);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc25", func_800AF518);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc25", func_800AF678);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc25", func_800AFA98);
/* 89.22%: the loop is right, gcc picks an offset induction variable where the
   original keeps a pointer one (base in a1 reloaded per iteration vs s0). A
   pointer-local variant reaches 95.11% but starts at element 0, not 1.
void func_800AFA98(BattleModel *pModel, BattleModelNode *pNode, s32 flags) {
    s32 i;

    pNode->unk7 = flags & 1;
    if (flags & 0x80) {
        for (i = 1; i < pModel->pNodes->unkA; i++) {
            if (pModel->pNodes[i].pParent == pNode) {
                func_800AFA98(pModel, &pModel->pNodes[i], flags);
            }
        }
    }
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc25", func_800AFB4C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc25", func_800AFC68);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc25", func_800AFD98);

void func_800AFF9C(u8 *p) {
    u8 unk10[0x10];
    s16 vec[3];
    s16 index;

    vec[0] = *(u16 *)(*(u8 **)(p + 4) + 0x5C);
    vec[1] = *(u16 *)(*(u8 **)(p + 4) + 0x60);
    vec[2] = *(u16 *)(*(u8 **)(p + 4) + 0x64);
    index = func_800A5914(vec, *(s16 *)(p + 0x1E), 4);
    *(s16 *)(p + 0x1E) = index;
    if (index < 0) {
        index = func_800A579C(vec);
        *(s16 *)(p + 0x1E) = index;
    }
    func_800A5870(vec, *(s16 *)(p + 0x1E), unk10);
    *(u16 *)(p + 0x60) = vec[1];
    if (*(u8 *)(p + 0x36) == 0) {
        *(s32 *)(*(u8 **)(p + 4) + 0x60) = vec[1];
    }
}

void func_800B0060(u8 *p) {
    if (*(s32 *)(p + 0xC) != 0) {
        if (*(u8 *)(p + 0x63) != 0) {
            func_8003852C(*(s32 *)(*(u8 **)(p + 0xB4) + 8));
            *(u8 *)(p + 0x63) = 0;
        }
        HeapFree(*(void **)(p + 0xC));
        *(s32 *)(p + 0xC) = 0;
        *(s32 *)(p + 0x18) = 0;
    }
}

void func_800B00D0(void) {
    s32 i = 8;
    s32 *p = D_800C3BCC;

    for (; i >= 0; i--) {
        *p-- = 0;
    }
}

void func_800B00F4(s32 arg0) {
    s32 i = 0;
    u8 **p = D_800C3BAC;
    u8 *pEntry;

    for (; i < 9; i++) {
        pEntry = *p++;
        if (pEntry != NULL) {
            func_800A23E8(arg0, pEntry);
        }
    }
    func_800B00D0();
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc25", func_800B0164);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc25", func_800B026C);

s32 func_800B0AB4(s16 *pPos) {
    BattleStreamData *pData = D_800658C8;

    if (pPos[0] > pData->unk34E && pPos[0] < pData->unk34C && pPos[2] > pData->unk350 &&
        pPos[2] < pData->unk352) {
        return 1;
    }
    return 0;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc25", func_800B0B14);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc25", func_800B0D70);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc25", func_800B0FF4);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc25", func_800B10EC);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc25", func_800B12D0);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc25", func_800B136C);
void func_800B14B8(void) {
    D_800C3D6C = 1;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc25", func_800B14CC);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc25", func_800B15D8);
/* 71.84%: the frame address and the 7-word struct copy are right; gcc swaps the
   frame pointer / D_800C3BD0 register roles and blocks the copy 4+3 instead of
   3+3+1, and re-forms D_800C3BD0+8/+16 instead of using one register.
s32 func_800B15D8(u8 *p, s32 index) {
    BattleFrame *pFrame = (BattleFrame *)(p + index * 0x1C + 0xC);

    D_800C3BD0 = *pFrame;
    if ((*(s32 *)(p + 4) & 1) == 0) {
        D_800C3BD0.unk0 += (mem_addr)pFrame;
        D_800C3BD0.unk8 += (mem_addr)pFrame;
        D_800C3BD0.unk10 += (mem_addr)pFrame;
    }
    D_800C3BF0 = 0;
    D_800C3BEC = D_800C3BD0.unk10;
    return pFrame->unk14;
}
*/
