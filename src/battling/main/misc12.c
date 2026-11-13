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

// 0x10-byte GPU tile packet: tag word (with its length byte), the primitive
// code word, then the position and size halfwords.
typedef struct {
    /* 0x0 */ u8 tag[3];
    /* 0x3 */ u8 len;
    /* 0x4 */ u32 code;
    /* 0x8 */ u16 x;
    /* 0xA */ u16 y;
    /* 0xC */ u16 w;
    /* 0xE */ u16 h;
} BattleTilePacket; // Size: 0x10

// Sprite set up by func_800897AC: four drawing-mode packets and two tiles.
typedef struct {
    /* 0x00 */ u8 unk0[0x16];
    /* 0x16 */ u8 unk16;
    /* 0x17 */ u8 unk17;
    /* 0x18 */ DR_MODE unk18;
    /* 0x24 */ DR_MODE unk24;
    /* 0x30 */ DR_MODE unk30;
    /* 0x3C */ DR_MODE unk3C;
    /* 0x48 */ BattleTilePacket unk48;
    /* 0x58 */ BattleTilePacket unk58;
} BattleSprite; // Size: 0x68

// 0x10-byte block moved around inside the object built by func_80089B44.
typedef struct {
    /* 0x0 */ u32 unk0;
    /* 0x4 */ u32 unk4;
    /* 0x8 */ u32 unk8;
    /* 0xC */ u32 unkC;
} BattleBlk16; // Size: 0x10

// 0x20-byte template copied wholesale into the two env globals below.
typedef struct {
    /* 0x00 */ u32 unk0;
    /* 0x04 */ u32 unk4;
    /* 0x08 */ u32 unk8;
    /* 0x0C */ u32 unkC;
    /* 0x10 */ u32 unk10;
    /* 0x14 */ u32 unk14;
    /* 0x18 */ u32 unk18;
    /* 0x1C */ u32 unk1C;
} BattleEnv; // Size: 0x20

// The 0x9C-byte object func_80089C54 allocates and func_80089B44 clears.
typedef struct {
    /* 0x00 */ u32 unk0;
    /* 0x04 */ u32 unk4;
    /* 0x08 */ u32 unk8;
    /* 0x0C */ BattleBlk16 unkC;
    /* 0x1C */ BattleBlk16 unk1C;
    /* 0x2C */ u16 unk2C;
    /* 0x2E */ u16 unk2E;
    /* 0x30 */ u16 unk30;
    /* 0x32 */ u16 unk32;
    /* 0x34 */ u32 unk34;
    /* 0x38 */ u32 unk38;
    /* 0x3C */ u32 unk3C;
    /* 0x40 */ u32 unk40;
    /* 0x44 */ u16 unk44;
    /* 0x46 */ u16 unk46;
    /* 0x48 */ u16 unk48;
    /* 0x4A */ u16 unk4A;
    /* 0x4C */ BattleBlk16 unk4C;
    /* 0x5C */ BattleBlk16 unk5C;
    /* 0x6C */ BattleEnv unk6C;
    /* 0x8C */ u32 unk8C;
    /* 0x90 */ u32 unk90;
    /* 0x94 */ u32 unk94;
    /* 0x98 */ u32 unk98;
} BattleMisc12Obj; // Size: 0x9C

extern BattleEnv D_80091C0C;
extern BattleEnv D_80096FE0;
extern BattleEnv D_8009A2D8;
extern s16 D_80096FE8;

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
extern void func_800453E8(void* pDrmode, void* pRect);
extern void func_8004546C(void* pDrmode, void* pRect);

extern void func_800888B0(void);
extern s32 func_800888E4(s32 slot);
extern void func_8008895C(void);
extern s32 func_800889C8(void);
extern void func_80089330(s32, s32);
extern void func_80089534(s32, s32);
extern BattleMisc12Obj* func_80089B44(BattleMisc12Obj* p);
extern void func_8002DFF0(s32, s32);
extern void func_80089210(s32, s32);
extern void func_800898BC(void*, SVECTOR*, SVECTOR*, SVECTOR*);

extern s32 D_8009A0C8;
extern s32 D_8009A0CC;
extern s32 D_8009A0D0;
extern s32 D_8009A918;
extern s32 D_8009A91C;
extern s32 D_8009A920;
extern s32 D_80096F98;
extern s32 D_80096F9C;
extern s32 D_80096FA0;
extern s32 D_80097000;
extern s32 D_80097004;
extern s32 D_80097008;

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

/* Best attempt: objdiff 61.85% (up from 47.28% with the pointer-local shape that
   func_80088940 needs; the store address now stays in a register instead of
   being rematerialised into $at). What is left is the original's five DEFERRED
   stores: it loads D_8006F980 once, keeps all five statement values live in
   $a2,$a1,$a0,$v1,$t0 and stores them together after the last field load, while
   gcc stores each value as it is computed and reuses one register. The masks,
   the five field loads and the shifts all match. Unclear how C source defers
   stores past the following global loads (they must alias), so this may not be
   reachable from C at all. */
/*
void func_80088A40(void) {
    s32* pFlags;
    s32 v0;
    s32 v1;
    s32 v2;
    s32 v3;
    s32 v4;

    if (D_8005061C == 0) {
        return;
    }
    pFlags = &D_8006F980;
    v0 = *pFlags;
    v0 = (v0 & 0xFFFFFFF0) | 1;
    v1 = (v0 & 0xFFFFFFEF) | ((D_80099D9B & 1) << 4);
    v2 = (v1 & 0xFFFFFFDF) | ((D_80099D9C & 1) << 5);
    v3 = (v2 & 0xFFFFE03F) | ((D_80099D9F & 0x7F) << 6);
    v4 = (v3 & 0xFFFF1FFF) | ((D_80099D98 & 7) << 13);
    *pFlags = v0;
    *pFlags = v1;
    *pFlags = v2;
    *pFlags = v3;
    *pFlags = v4;
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

/* Not attempted. The original starts with a dead `lb $t0,0x52C4($zero)` (a read
   of RAM address 0x52C4, which has no symbol in symbol_addrs or
   undefined_syms_auto and is never used again); the rest is
   `D_800927F0 = Vsync(1)`. No C statement produces that dead load, so the stub
   is probably not convertible as a single function. */

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

void func_80089210(s32 w, s32 h) {
    s32 scaleW;
    s32 scaleH;

    scaleW = (w << 12) / 320 * h / w;
    scaleH = (h << 12) / 240 * h / w;
    D_8009A2D8 = D_80091C0C;
    D_80096FE0 = D_80091C0C;
    ((s16*)&D_80096FE0)[0] = scaleW;
    D_80096FE8 = scaleH;
}

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

void func_800897AC(BattleSprite* p, s32 x, s32 y, s32 w, s32 h, s32 v) {
    RECT rect;
    RECT rect2;

    rect.x = x;
    rect.y = y;
    rect.w = w;
    rect.h = h;
    func_800453E8(&p->unk18, &rect);
    rect.y = y + v;
    func_800453E8(&p->unk24, &rect);
    rect2.x = x;
    rect2.y = y;
    func_8004546C(&p->unk30, &rect2);
    rect2.y = y + v;
    func_8004546C(&p->unk3C, &rect2);
    p->unk48.len = 3;
    p->unk48.code = 0x60000000;
    p->unk48.x = 0;
    p->unk48.y = 0;
    p->unk48.w = w;
    p->unk48.h = h;
    p->unk58 = p->unk48;
    p->unk16 |= 0xC;
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc12", func_800898BC);

/* Best attempt: objdiff 50.18%. The statement order and the four VectorNormal/
   OuterProduct12 calls are right, but gcc 2.7.2 hoists the materialisation of all
   four base addresses (&D_8009A0C8, &D_80096F98, &D_8009A918, &D_80097000) to the
   top of the function, while retail materialises each one right before its first
   use, after the three D_8009A0C8/CC/D0 difference stores. That shifts every later
   block. Signatures: (MATRIX*, SVECTOR* pFrom, SVECTOR* pTo, SVECTOR* pScale);
   pMat->m rows come from the D_80097000 / D_8009A918 / D_80096F98 triples loaded
   as words and stored as halves, pMat->t = negated D_8009A0C8/CC/D0. */
/*
void func_800898BC(MATRIX* pMat, SVECTOR* pFrom, SVECTOR* pTo, SVECTOR* pScale) {
    D_8009A0C8 = pTo->vx - pFrom->vx;
    D_8009A0CC = pTo->vy - pFrom->vy;
    D_8009A0D0 = pTo->vz - pFrom->vz;
    D_8009A918 = pScale->vx;
    D_8009A91C = pScale->vy;
    D_8009A920 = pScale->vz;
    VectorNormal((VECTOR*)&D_8009A0C8, (VECTOR*)&D_80096F98);
    OuterProduct12((VECTOR*)&D_8009A918, (VECTOR*)&D_80096F98, (VECTOR*)&D_8009A0C8);
    VectorNormal((VECTOR*)&D_8009A0C8, (VECTOR*)&D_80097000);
    OuterProduct12((VECTOR*)&D_80096F98, (VECTOR*)&D_80097000, (VECTOR*)&D_8009A0C8);
    VectorNormal((VECTOR*)&D_8009A0C8, (VECTOR*)&D_8009A918);
    pMat->m[0][0] = D_80097000;
    pMat->m[0][1] = D_80097004;
    pMat->m[0][2] = D_80097008;
    pMat->m[1][0] = D_8009A918;
    pMat->m[1][1] = D_8009A91C;
    pMat->m[1][2] = D_8009A920;
    pMat->m[2][0] = D_80096F98;
    pMat->m[2][1] = D_80096F9C;
    pMat->m[2][2] = D_80096FA0;
    ApplyMatrix(pMat, pFrom, (VECTOR*)&D_8009A0C8);
    MulMatrix2((MATRIX*)&D_80096FE0, pMat);
    pMat->t[0] = -D_8009A0C8;
    pMat->t[1] = -D_8009A0CC;
    pMat->t[2] = -D_8009A0D0;
}
*/


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

BattleMisc12Obj* func_80089B44(BattleMisc12Obj* p) {
    p->unk0 = 0;
    p->unk8C = 0;
    p->unk94 = 0;
    p->unk90 = 0;
    p->unk8 = 0;
    p->unk4 = 0;
    p->unk3C = 0;
    p->unk38 = 0;
    p->unk34 = 0;
    p->unk48 = 0;
    p->unk46 = 0;
    p->unk44 = 0;
    p->unk30 = 0;
    p->unk2E = 0;
    p->unk2C = 0;
    p->unk6C = D_80091C0C;
    p->unk4C = *(BattleBlk16*)&p->unk6C;
    p->unk5C = *((BattleBlk16*)&p->unk6C + 1);
    p->unkC = p->unk4C;
    p->unk1C = p->unk5C;
    return p;
}

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
