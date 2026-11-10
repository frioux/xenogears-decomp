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

INCLUDE_ASM("asm/battling/nonmatchings/main/misc12", func_80088940);

/* Best attempt: objdiff 44.86%. gcc gives the store a rematerialised symbol
   address (`lui at` + `sw %lo(at)`) and reuses the address register for the
   loaded value, while the original keeps the address live in one register and
   stores through it (`lui v0,%hi` + `addiu v0,v0,%lo` + `lw v1,0(v0)`). Pointer
   local, separate value local, `*p = *p | K`, `p[0]`, a bitfield write and
   swapped OR operands all compile to the same two-address shape. The whole
   function is 100% under `-G0` (tools/scripts/try_flags.sh ... --gp -G0), so
   this is the same `-G8` vs `-G0` question as func_80088A40. */
/*
void func_80088940(void) {
    s32 value;
    s32* pFlags;

    pFlags = &D_8006F980;
    value = *pFlags;
    *pFlags = value | 0x10000;
}
*/

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

INCLUDE_ASM("asm/battling/nonmatchings/main/misc12", func_800889C8);

/* Best attempt: objdiff 87.13%. The loop and both returns match exactly; the
   only difference is `D_8006F980 |= 0x10000;`, which gcc compiles with a
   rematerialised address (`lui at` + `sw %lo(at)`) while the original keeps the
   address live and stores it in the `jal` delay slot. Same root cause as
   func_80088940. */
/*
s32 func_800889C8(void) {
    s32 slot;

    for (slot = 0; slot < 0x31; slot++) {
        if (slot != 0x16) {
            if (func_800888E4(slot) == 0) {
                return 0;
            }
        }
    }
    D_8006F980 |= 0x10000;
    func_8008895C();
    return 0;
}
*/

INCLUDE_ASM("asm/battling/nonmatchings/main/misc12", func_80088A40);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc12", func_80088AF8);

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

INCLUDE_ASM("asm/battling/nonmatchings/main/misc12", func_80089A98);

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
