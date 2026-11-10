#include "common.h"
#include "system/memory.h"
#include "system/archive.h"
#include "psyq/libgpu.h"
#include "psyq/libgte.h"
#include "system/controller.h"

// Node of a singly linked list hung off a 0x98+ byte owner.
typedef struct BattleListNode {
    /* 0x00 */ u8 unk0[0x8C];
    /* 0x8C */ struct BattleListNode* unk8C;
    /* 0x90 */ struct BattleListNode* unk90;
    /* 0x94 */ struct BattleListNode* unk94;
} BattleListNode;

// Owner struct whose field 4 holds the pointer this unit offsets by 0xC.
typedef struct {
    /* 0x0 */ u32 unk0;
    /* 0x4 */ void* unk4;
} BattleMisc12Owner; // Size: 0x8

// Slot table entry, 0xF8 bytes, indexed by slot number.
typedef struct {
    /* 0x00 */ u8 unk0[0x77];
    /* 0x77 */ u8 unk77;
    /* 0x78 */ u8 unk78[3];
    /* 0x7B */ u8 unk7B;
    /* 0x7C */ u8 unk7C[4];
    /* 0x80 */ u16 unk80;
    /* 0x82 */ u16 unk82;
    /* 0x84 */ u8 unk84[0x74];
} BattleEntry; // Size: 0xF8

extern s32 D_8006F980;
extern VECTOR D_80096FA8;
extern u8 D_8005061C;
extern u8 D_80099D98;
extern u8 D_80099D9B;
extern u8 D_80099D9C;
extern u8 D_80099D9F;
extern s16 D_8009285C;
extern s16 D_8009286C;
extern u8 D_80091A6C[];
extern u8 D_800927EC;
extern s32 D_80092888;
extern s32 D_800927F4;
extern s32 D_800927F0;
extern u8** D_800928EC;
extern BattleEntry D_8009A0D8[];

extern void ArchiveReadFileToBuffer(s32 index, void* pBuffer, u32 arg2, u32 flags);

extern void func_800888B0(void);
extern s32 func_800888E4(s32 slot);
extern void func_8008895C(void);
extern s32 func_800889C8(void);
extern void func_80089330(s32, s32);
extern void func_80089534(s32, s32);
extern void func_80089B44(void* p);
extern void func_8002DFF0(s32, s32);
extern void func_80089210(s32, s32);
extern void func_800898BC(void*, SVECTOR*, SVECTOR*, SVECTOR*);

void func_80088940(void) {
    s32 value;
    s32* pFlags;

    pFlags = &D_8006F980;
    value = *pFlags;
    *pFlags = value | 0x10000;
}

void func_8008895C(void) {
    if ((D_8006F980 & 0x10000) == 0) {
        return;
    }
    if (D_800927EC != 0) {
        return;
    }
    D_800927EC = 1;
    D_800928EC[D_80092888++] = D_80091A6C;
}

s32 func_800889C8(void) {
    s32 slot;
    s32 value;
    s32* pFlags;

    for (slot = 0; slot < 0x31; slot++) {
        if (slot != 0x16) {
            if (func_800888E4(slot) == 0) {
                return 0;
            }
        }
    }
    pFlags = &D_8006F980;
    value = *pFlags;
    *pFlags = value | 0x10000;
    func_8008895C();
    return 0;
}


INCLUDE_ASM("asm/battling/nonmatchings/main/misc12", func_80088A40);

/* Best attempt: objdiff 47.28% (identical under `--gp -G0`, so this is not the
   -G8/-G0 question that func_80088940 has). The source is five statements
   `D_8006F980 = (D_8006F980 & ~MASK) | ((D_80099D9x & m) << shift);` with a
   `if (D_8005061C == 0) return;` guard: the single load, the five masks, the
   five field loads and the shifts all match. Two differences remain: gcc
   rematerialises the store address into `$at` instead of keeping it in `$a3`,
   and the original keeps all five statement values live in five registers
   (`$a2,$a1,$a0,$v1,$t0`) storing them together after the last field load,
   where gcc stores each value immediately and reuses one register. */
/*
void func_80088A40(void) {
    if (D_8005061C == 0) {
        return;
    }
    D_8006F980 = (D_8006F980 & 0xFFFFFFF0) | 1;
    D_8006F980 = (D_8006F980 & 0xFFFFFFEF) | ((D_80099D9B & 1) << 4);
    D_8006F980 = (D_8006F980 & 0xFFFFFFDF) | ((D_80099D9C & 1) << 5);
    D_8006F980 = (D_8006F980 & 0xFFFFE03F) | ((D_80099D9F & 0x7F) << 6);
    D_8006F980 = (D_8006F980 & 0xFFFF1FFF) | ((D_80099D98 & 7) << 13);
}
*/

void func_80088AF8(void) {
    u32 flags;
    u32* pFlags;
    s32 i;
    u8* pBytes;

    if (D_8005061C == 0) {
        return;
    }
    pFlags = (u32*)&D_8006F980;
    flags = *pFlags;
    D_800927EC = 0;
    if ((flags & 0xF) == 1) {
        D_80099D9B = (flags >> 4) & 1;
        D_80099D9C = (flags >> 5) & 1;
        D_80099D9F = (flags >> 6) & 0x7F;
        D_80099D98 = (flags >> 13) & 7;
        if (flags & 0x10000) {
            i = 7;
            pBytes = (u8*)pFlags - 1;
            for (; i >= 0; i--) {
                *pBytes-- = 0;
            }
        }
    } else {
        D_80099D9B = 0;
        D_80099D9C = 0;
        D_80099D9F = 2;
        D_80099D98 = 0;
        func_80088A40();
    }
}

void func_80088BD4(void) {
    func_800888B0();
    func_800889C8();
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc12", func_80088BFC);

void func_80088C28(void) {
    u16 buttons;

    buttons = g_C1ButtonState;
    if (buttons & 4) {
        D_800927F4++;
    }
    if (buttons & 1) {
        D_800927F4--;
    }
    if (D_800927F4 < 0) {
        D_800927F4 = 0;
    }
    HeapDebugDump(1, D_800927F4, 0xA, 0x80AD);
}

void func_80088CBC(s32 slot) {
    BattleEntry* pEntry;

    pEntry = &D_8009A0D8[slot];
    pEntry->unk77 = 3;
    pEntry->unk7B = 0x7D;
    pEntry->unk80 = 0x3000;
    pEntry->unk82 = GetClut(0x3F0, 0xC0);
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc12", func_80088D1C);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc12", func_80088E90);

void* func_800891C0(s32 entryIndex) {
    void* pBuffer;

    pBuffer = HeapAlloc(ArchiveDecodeSize(entryIndex), 1);
    ArchiveReadFileToBuffer(entryIndex, pBuffer, 0, 0);
    return pBuffer;
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc12", func_80089210);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc12", func_80089330);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc12", func_80089534);

void func_800896C4(s32 a0, s32 a1) {
    SetGeomOffset(a0 / 2, a1 / 2);
    SetGeomScreen((a0 << 8) / a0);
    func_8002DFF0(a0, a1);
    func_80089210(a0, a1);
}

void func_8008973C(void) {
    func_80089534(D_8009285C, D_8009286C);
}

void func_8008976C(s32 a0, s32 a1) {
    func_80089330(a0, a1);
    func_80089534(a0, a1);
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc12", func_800897AC);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc12", func_800898BC);

/*@func_80089A98*/
void func_80089A98(BattleMisc12Owner* pOwner, VECTOR* pFrom, VECTOR* pTo) {
    SVECTOR scale;
    SVECTOR offset;
    SVECTOR zero;

    scale.vy = 0x1000;
    scale.vz = 0;
    scale.vx = 0;
    D_80096FA8 = *pTo;
    offset.vx = pFrom->vx - pTo->vx;
    offset.vy = pFrom->vy - pTo->vy;
    offset.vz = pFrom->vz - pTo->vz;
    zero.vz = 0;
    zero.vx = 0;
    zero.vy = 0;
    func_800898BC((u8*)pOwner->unk4 + 0xC, &offset, &zero, &scale);
}
/*@@*/

INCLUDE_ASM("asm/battling/nonmatchings/main/misc12", func_80089B44);

void func_80089C54(void) {
    void* p;

    HeapSetCurrentContentType(8);
    p = HeapAlloc(0x9C, 0);
    func_80089B44(p);
}

void func_80089C88(BattleListNode* pList, BattleListNode* pNode) {
    BattleListNode* pTail;
    BattleListNode* pCur;

    pNode->unk8C = pList;
    pTail = pList->unk94;
    if (pTail == 0) {
        pList->unk94 = pNode;
        return;
    }
    pCur = pTail;
    while (pCur->unk90 != 0) {
        pCur = pCur->unk90;
    }
    pCur->unk90 = pNode;
}

void func_80089CD8(BattleListNode* pNode) {
    BattleListNode* pList;
    BattleListNode* pPrev;
    BattleListNode* pCur;

    if (pNode == 0) {
        return;
    }
    pList = pNode->unk8C;
    if (pList == 0) {
        return;
    }
    pNode->unk8C = 0;
    pPrev = pList->unk94;
    if (pPrev == 0) {
        return;
    }
    if (pPrev == pNode) {
        pList->unk94 = pNode->unk90;
    } else {
        pCur = pPrev;
        if (pCur->unk90 != pNode) {
            do {
                pCur = pCur->unk90;
            } while (pCur->unk90 != pNode);
        }
        pCur->unk90 = pNode->unk90;
    }
    pNode->unk90 = 0;
}
