#include "common.h"
#include "system/memory.h"
#include "system/archive.h"
#include "psyq/libgpu.h"
#include "psyq/libgte.h"
#include "system/controller.h"

// Node of a singly linked list hung off a 0x98+ byte owner.
typedef struct BattleListNode {
    /* 0x00 */ u8 unk0[0x8C];
    /* 0x8C */ struct BattleListNode* pList;
    /* 0x90 */ struct BattleListNode* pNext;
    /* 0x94 */ struct BattleListNode* pHead;
} BattleListNode;

// Owner struct whose field 4 holds the pointer this unit offsets by 0xC.
typedef struct {
    /* 0x0 */ u32 unk0;
    /* 0x4 */ void* pData;
} BattleMisc12Owner; // Size: 0x8

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

// Slot table entry, 0xF8 bytes, indexed by slot number: the draw environment
// BattleInitDrawEnvs fills with SetDefDrawEnv, the display environment
// func_80089330 fills with SetDefDispEnv (a copy of it goes to PutDispEnv), and
// the one-word root ordering table g_BattleRootOrderingTable points at. At +0x74
// it carries the semi-transparent 16x16 sprite prim BattleInitSlotDrawEntry
// initialises: tag with length 3 and code 0x7D (what setSprt16 does, plus the
// semi-transparency bit), x0/y0, then the u0/v0 halfword and the clut. The same
// prim is BattleTexBlock D_8009A14C in src/battling/main/misc1.c, which names
// the colour bytes r0/g0/b0.
typedef struct {
    /* 0x00 */ DRAWENV drawEnv;
    /* 0x5C */ DISPENV dispEnv;
    /* 0x70 */ u_long ot;
    /* 0x74 */ u8 tagAddr[3];
    /* 0x77 */ u8 len;
    /* 0x78 */ u8 r0;
    /* 0x79 */ u8 g0;
    /* 0x7A */ u8 b0;
    /* 0x7B */ u8 code;
    /* 0x7C */ u16 x0;
    /* 0x7E */ u16 y0;
    /* 0x80 */ u16 uv;
    /* 0x82 */ u16 clut;
    /* 0x84 */ u8 unk84[0x4C];
    /* 0xD0 */ DR_AREA drawAreaMode;   // draw area, from drawEnv.clip
    /* 0xDC */ DR_MODE drawOffsetMode; // draw offset, from drawEnv.ofs
    /* 0xE8 */ BattleTilePacket tile;  // the tile func_80089330 sets to the screen size
} BattleEntry; // Size: 0xF8

// Sprite set up by BattleInitSpritePrims: a drawing-area and a drawing-offset draw
// mode per draw buffer (0/1), plus a tile per buffer. The head is the record
// src/battling/main/misc14.c declares as Misc14DisplayWork (and misc13.c as
// Misc13Buffer): the double-buffered ordering table, prim area and their length.
typedef struct {
    /* 0x00 */ s32 unk0;
    /* 0x04 */ u_long* pOt[2];     // ordering table, one per draw buffer
    /* 0x0C */ u_long* pPrim[2];   // prim area, one per draw buffer
    /* 0x14 */ s16 count;          // ordering table length in words
    /* 0x16 */ u8 flags;
    /* 0x17 */ u8 depthShift;
    /* 0x18 */ DR_MODE drawAreaMode0;
    /* 0x24 */ DR_MODE drawAreaMode1;
    /* 0x30 */ DR_MODE drawOffsetMode0;
    /* 0x3C */ DR_MODE drawOffsetMode1;
    /* 0x48 */ BattleTilePacket tile0;
    /* 0x58 */ BattleTilePacket tile1;
} BattleSprite; // Size: 0x68

// 0x10-byte block moved around inside the object built by BattleResetObject.
typedef struct {
    /* 0x0 */ u32 unk0;
    /* 0x4 */ u32 unk4;
    /* 0x8 */ u32 unk8;
    /* 0xC */ u32 unkC;
} BattleBlk16; // Size: 0x10

// 0x20-byte matrix template copied wholesale into the two env globals below: the
// MATRIX layout src/battling/main/misc1.c and misc2.c declare at the same
// addresses and read .t[0..2] of (the two pad bytes step t to a word boundary).
typedef struct {
    /* 0x00 */ s16 m[3][3];
    /* 0x12 */ u16 pad12;
    /* 0x14 */ s32 t[3];
} BattleEnv; // Size: 0x20

// The 0x9C-byte object BattleCreateObject allocates and BattleResetObject clears;
// src/battling/main/misc14.c declares the same record as Misc14ModelNode, and the
// links at 0x8C are the ones BattleAppendListEntry walks.
typedef struct {
    /* 0x00 */ u32 state;
    /* 0x04 */ void* pData;
    /* 0x08 */ void (*pUpdate)(void);
    /* 0x0C */ BattleBlk16 localMatrixLo; // first half of the local matrix
    /* 0x1C */ BattleBlk16 localMatrixHi; // second half
    /* 0x2C */ SVECTOR sourcePos;
    /* 0x34 */ VECTOR position;
    /* 0x44 */ SVECTOR rotation;
    /* 0x4C */ BattleBlk16 rotMatrixLo; // first half of the rotation matrix
    /* 0x5C */ BattleBlk16 rotMatrixHi; // second half
    /* 0x6C */ BattleEnv matrix;
    /* 0x8C */ BattleListNode* pList;
    /* 0x90 */ BattleListNode* pNext;
    /* 0x94 */ BattleListNode* pHead;
    /* 0x98 */ u32 unk98;
} BattleMisc12Obj; // Size: 0x9C

extern BattleEnv g_BattleBaseMatrix;
extern BattleEnv g_BattleScaleMatrix;
extern BattleEnv g_BattleBaseMatrixCopy;
extern s16 D_80096FE8;

extern s32 g_BattleOptionFlags;
extern VECTOR g_BattleViewOrigin;
extern u8 g_BattleTransitionFlag;
extern u8 g_BattleDifficultyLevel;
extern u8 g_BattlePlayer1Vibration;
extern u8 g_BattlePlayer2Vibration;
extern u8 g_BattleMatchCount;
extern s16 g_BattleScreenWidth;
extern s16 g_BattleScreenHeight;
extern u8 g_BattleExtraEntries[];
extern u8 g_BattleExtraEntryQueued;
extern s32 g_BattleEntryCount;
extern s32 g_BattleHeapDumpScroll;
extern s32 D_800927F0;
extern u8** g_BattleEntries;
extern BattleEntry g_BattleDrawBuffers[];

extern void ArchiveReadFileToBuffer(s32 index, void* pBuffer, u32 arg2, u32 flags);
extern void func_8004546C(void* pDrmode, void* pRect);
extern void func_80045534(void* pDrmode, RECT* pRect);

extern u8 D_8009A134[];
extern u16 D_8009A13C;
extern s16 D_8009A13E;
extern u16 D_8009A140;
extern s16 D_8009A142;
extern u8 D_8009A144;
extern u16 D_8009A234;
extern s16 D_8009A236;
extern u16 D_8009A238;
extern s16 D_8009A23A;
extern u8 D_8009A23C;
extern BattleTilePacket D_8009A1C0;
extern BattleTilePacket D_8009A2B8;

extern void BattleSetSlotBit(void);
extern s32 BattleTestSlotBit(s32 slot);
extern void BattleAppendExtraQueueEntry(void);
extern s32 BattleSetFlag10000IfAllSlotBitsSet(void);
extern void func_80089330(s32, s32);
extern void BattleInitDrawEnvs(s32, s32);
extern BattleMisc12Obj* BattleResetObject(BattleMisc12Obj* p);
extern void ModelSetScreenSize(s32, s32);
extern void BattleSetScaleMatrix(s32, s32);
extern void func_800898BC(MATRIX*, SVECTOR*, SVECTOR*, SVECTOR*);

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

void BattleSetFlag10000(void) {
    s32 value;
    s32* pFlags;

    pFlags = &g_BattleOptionFlags;
    value = *pFlags;
    *pFlags = value | 0x10000;
}

void BattleAppendExtraQueueEntry(void) {
    if ((g_BattleOptionFlags & 0x10000) == 0) {
        return;
    }
    if (g_BattleExtraEntryQueued != 0) {
        return;
    }
    g_BattleExtraEntryQueued = 1;
    g_BattleEntries[g_BattleEntryCount++] = g_BattleExtraEntries;
}

s32 BattleSetFlag10000IfAllSlotBitsSet(void) {
    s32 slot;
    s32 value;
    s32* pFlags;

    for (slot = 0; slot < 0x31; slot++) {
        if (slot != 0x16) {
            if (BattleTestSlotBit(slot) == 0) {
                return 0;
            }
        }
    }
    pFlags = &g_BattleOptionFlags;
    value = *pFlags;
    *pFlags = value | 0x10000;
    BattleAppendExtraQueueEntry();
    return 0;
}



INCLUDE_ASM("asm/battling/nonmatchings/main/misc12", func_80088A40);

/* Best attempt: objdiff 86.96%. Bitfield stores on (BattleFlagBits*)&g_BattleOptionFlags
   reproduce everything - the single lw, the five clearing masks (~0xF, ~0x10, ~0x20,
   ~0xFFFFE03F, ~0xFFFF1FFF), the five value registers ($a2,$a1,$a0,$v1,$t0) and the
   lbu/andi/sll/or chain - except store placement: retail defers the first four stores
   until after the fourth lbu (sw a2/a1/a0/v1 together), gcc stores each value as its
   bitfield assignment completes. Measurements: five plain locals with a cached s32
   61.85%; chained s32 through a pointer local 70.96%; gcc-2.7.2-cdk 84.13; 2.6.0 does
   not compile it; cc1_probe of this body with -fschedule-insns -fschedule-insns2 is
   byte-identical to the plain unit flags (it does hoist the four mask constants, which
   retail also hoists, but leaves the stores inline). Retail keeps four values live at
   once, which no source order tried so far produces. */
/*
typedef struct {
    u32 mode : 4;
    u32 flagB : 1;
    u32 flagC : 1;
    u32 value : 7;
    u32 scale : 3;
    u32 pad : 16;
} BattleFlagBits;

void func_80088A40(void) {
    BattleFlagBits* pFlags;

    if (g_BattleTransitionFlag == 0) {
        return;
    }
    pFlags = (BattleFlagBits*)&g_BattleOptionFlags;
    pFlags->mode = 1;
    pFlags->flagB = g_BattlePlayer1Vibration & 1;
    pFlags->flagC = g_BattlePlayer2Vibration & 1;
    pFlags->value = g_BattleMatchCount & 0x7F;
    pFlags->scale = g_BattleDifficultyLevel & 7;
}
*/

void BattleApplyOptionWord(void) {
    u32 flags;
    u32* pFlags;
    s32 i;
    u8* pBytes;

    if (g_BattleTransitionFlag == 0) {
        return;
    }
    pFlags = (u32*)&g_BattleOptionFlags;
    flags = *pFlags;
    g_BattleExtraEntryQueued = 0;
    if ((flags & 0xF) == 1) {
        g_BattlePlayer1Vibration = (flags >> 4) & 1;
        g_BattlePlayer2Vibration = (flags >> 5) & 1;
        g_BattleMatchCount = (flags >> 6) & 0x7F;
        g_BattleDifficultyLevel = (flags >> 13) & 7;
        if (flags & 0x10000) {
            i = 7;
            pBytes = (u8*)pFlags - 1;
            for (; i >= 0; i--) {
                *pBytes-- = 0;
            }
        }
    } else {
        g_BattlePlayer1Vibration = 0;
        g_BattlePlayer2Vibration = 0;
        g_BattleMatchCount = 2;
        g_BattleDifficultyLevel = 0;
        func_80088A40();
    }
}

void BattleMarkSlotAndQueueIfAllMarked(void) {
    BattleSetSlotBit();
    BattleSetFlag10000IfAllSlotBitsSet();
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc12", func_80088BFC);

/* Not attempted. The original starts with a dead `lb $t0,0x52C4($zero)` (a read
   of RAM address 0x52C4, which has no symbol in symbol_addrs or
   undefined_syms_auto and is never used again); the rest is
   `D_800927F0 = Vsync(1)`. No C statement produces that dead load, so the stub
   is probably not convertible as a single function.
   Round 42: the dead word is 0x800852C4, i.e. the address of func_800852C4, and
   func_80088E90 reads this label as an array of pointers through
   %lo(func_80088BFC)($at). So 0x80088BFC is a pointer datum whose code starts at
   +4; our build cannot define it (the bytes come from splat's asm range, and the
   file already has an extern u8 func_80088BFC[] for the reader above). */

void BattleDebugScrollHeapDump(void) {
    u16 buttons;

    buttons = g_C1ButtonState;
    if (buttons & 4) {
        g_BattleHeapDumpScroll++;
    }
    if (buttons & 1) {
        g_BattleHeapDumpScroll--;
    }
    if (g_BattleHeapDumpScroll < 0) {
        g_BattleHeapDumpScroll = 0;
    }
    HeapDebugDump(1, g_BattleHeapDumpScroll, 0xA, 0x80AD);
}

void BattleInitSlotDrawEntry(s32 slot) {
    BattleEntry* pEntry;

    pEntry = &g_BattleDrawBuffers[slot];
    pEntry->len = 3;
    pEntry->code = 0x7D;
    pEntry->uv = 0x3000;
    pEntry->clut = GetClut(0x3F0, 0xC0);
}

extern u8 func_80088BFC[];
extern u8 g_BattleHeapContentTypes[];
extern s32 g_DebugTable;
extern void* g_BattleDrawSlot;
extern void* g_BattleDisplaySlot;
extern s32 g_BattleDebugDrawMode;
extern u16 g_BattleFrameWaitCount;
extern s32 g_BattleMultiplyNodeMatrices;
extern u16 g_BattleDebugFlags;
extern s32 g_BattleDrawFlags;
extern s32 g_BattleDebugDrawCallback;
extern u8 g_BattleDrawBufferIndex;
extern u8 g_BattleControlFlags;
extern void* FontLoadFont(s32, s32, s32, s32, s32, u32, s32, s32, s32, s32, void*);
extern void BattleSetTpage(s32, s32);
extern void BattleSetClut(s32, s32);
extern void BattleInitSoundSlots(void);

void BattleInitGraphics(void) {
    RECT rect;


    DrawSyncCallback((void (*)())((u8*)func_80088BFC + 4));
    InitGeom();
    HeapChangeCurrentUser(6, (char**)g_BattleHeapContentTypes);
    ArchiveSetIndex(ARCHIVE_DIR_BATTLE_ENTRIES, 0);
    FontLoadFont(4, 2, 0x138, 0xDA, 0x14, 1, 0x3C0, 0x1F0, 0x3C0, 0x1EF, 0);
    BattleInitSlotDrawEntry(0);
    BattleInitSlotDrawEntry(1);
    switch (g_DebugTable) {
    case -1:
        g_BattleDebugDrawMode = 2;
        break;
    case 0:
        g_BattleDebugDrawMode = 1;
        break;
    default:
        g_BattleDebugDrawMode = 0;
        break;
    }
    g_BattleDrawSlot = g_BattleDrawBuffers;
    g_BattleDisplaySlot = &g_BattleDrawBuffers[1];
    BattleSetTpage(-1, -1);
    BattleSetClut(-1, -1);
    g_BattleFrameWaitCount = 2;
    g_BattleMultiplyNodeMatrices = 1;
    g_BattleDebugDrawCallback = 0;
    g_BattleDrawFlags = 0;
    g_BattleDrawBufferIndex = 0;
    g_BattleControlFlags = 0;
    g_BattleDebugDrawCallback = 0;
    g_BattleDebugFlags = 7;
    BattleInitSoundSlots();
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc12", func_80088E90);

/* Not attempted. 204 instructions: the battle draw loop. Calls BattleInitGraphics,
   then BattleCreateWorkBlock(handlers[D_80050618], 0, 0x801FE000, 0x400) and
   Vsync(1) in a loop that toggles g_BattleDrawFlags and re-points g_BattleDrawSlot/70 at
   g_BattleDrawBuffers + g_BattleDrawFlags * 0x70, then TermPrim, FontDrawLetters,
   BattleReleaseFinishedSoundSlots, AddPrim, BattleDebugScrollHeapDump,
   FontPrintf x3, BattleCompactOrderingTable, DrawSync, PutDispEnv, DrawOTagEnv.
   The 4-word copies at +0x5C..+0x6B use lwl/lwr (unaligned 32-bit struct
   assignment into a stack local, so a `BattleBlk16` local copied field by field
   will not reproduce it). Needs a session of its own. */

void* BattleLoadArchiveEntry(s32 entryIndex) {
    void* pBuffer;

    pBuffer = HeapAlloc(ArchiveDecodeSize(entryIndex), 1);
    ArchiveReadFileToBuffer(entryIndex, pBuffer, 0, 0);
    return pBuffer;
}

void BattleSetScaleMatrix(s32 w, s32 h) {
    s32 scaleW;
    s32 scaleH;

    scaleW = (w << 12) / 320 * h / w;
    scaleH = (h << 12) / 240 * h / w;
    g_BattleBaseMatrixCopy = g_BattleBaseMatrix;
    g_BattleScaleMatrix = g_BattleBaseMatrix;
    g_BattleScaleMatrix.m[0][0] = scaleW;
    D_80096FE8 = scaleH;
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc12", func_80089330);

/* Best attempt: objdiff 97.91%, but 508 bytes against retail's 512, so it cannot
   link yet. The tile packet is a BattleTilePacket at D_8009A1C0 (tag/len at +0/+3,
   code at +4, x/y at +8/+0xA, w/h at +0xC/+0xE) and the 16-byte copy is
   D_8009A2B8 = D_8009A1C0. Typing both under that ONE symbol is what makes the
   body work at all: with the tile fields declared as separate D_8009A1C3/1C4/1C8/
   1CA/1CC/1CE externs (the same body, same store order) gcc treats the copy's loads
   as independent of those stores and sched2 hoists the copy above them (79.77%, the
   old park); through the shared D_8009A1C0 base the loads and stores alias, the copy
   stays last, and the two truncating copies retail has for w/h come back too.
   Still missing: retail copies the w and h parameters into $v0 before each sh
   (addu v0,s2,zero / addu v0,s1,zero), which is the whole 4-byte gap. No spelling
   found that makes cc1 2.7.2 emit that copy - probed u16/s16 locals, an explicit
   (u16) cast, w & 0xFFFF, a u16* view, a shared s32 temp, bitfields for w/h, a
   volatile field, and a struct-pointer field store: every one stores $s2/$s1
   straight into the sh. */
/*
void func_80089330(s32 w, s32 h) {
    DISPENV* pDisp;

    if (h >= 0x101) {
        pDisp = (DISPENV*)D_8009A134;
        SetDefDispEnv(pDisp, 0, 0, w, h);
        SetDefDispEnv((DISPENV*)((u8*)pDisp + 0xF8), 0, 0, w, h);
        D_8009A23C = 1;
        D_8009A144 = 1;
        D_8009A13C = 0;
        D_8009A13E = 0x10;
        D_8009A140 = 0x100;
        D_8009A142 = 0xD4;
        D_8009A234 = 0;
        D_8009A236 = 0x10;
        D_8009A238 = 0x100;
        D_8009A23A = 0xD4;
    } else {
        pDisp = (DISPENV*)D_8009A134;
        SetDefDispEnv(pDisp, 0, 0x100, w, h);
        SetDefDispEnv((DISPENV*)((u8*)pDisp + 0xF8), 0, 0, w, h);
        D_8009A23C = 0;
        D_8009A144 = 0;
        D_8009A13C = 0;
        D_8009A13E = 0xA;
        D_8009A140 = 0x100;
        D_8009A142 = h;
        D_8009A234 = 0;
        D_8009A236 = 0xA;
        D_8009A238 = 0x100;
        D_8009A23A = h;
    }
    g_BattleScreenWidth = w;
    g_BattleScreenHeight = h;
    BattleSetScaleMatrix((s16)w, (s16)h);
    D_8009A1C0.len = 3;
    D_8009A1C0.code = 0x60000000;
    D_8009A1C0.w = w;
    D_8009A1C0.x = 0;
    D_8009A1C0.y = 0;
    D_8009A1C0.h = h;
    D_8009A2B8 = D_8009A1C0;
}

*/
void BattleInitDrawEnvs(s32 w, s32 h) {
    u8* pDtd;
    u16 tpage;

    SetGeomOffset(w / 2, h / 2);
    SetGeomScreen(0x180);
    ModelSetScreenSize(w, h);
    if (h >= 0x101) {
        SetDefDrawEnv(&g_BattleDrawBuffers[0].drawEnv, 0, 0, w, h);
        SetDefDrawEnv(&g_BattleDrawBuffers[1].drawEnv, 0, 0, w, h);
        g_BattleDrawBuffers[1].drawEnv.dfe = 0;
        g_BattleDrawBuffers[0].drawEnv.dfe = 0;
    } else {
        SetDefDrawEnv(&g_BattleDrawBuffers[0].drawEnv, 0, 0, w, h);
        SetDefDrawEnv(&g_BattleDrawBuffers[1].drawEnv, 0, 0x100, w, h);
    }
    pDtd = &g_BattleDrawBuffers[0].drawEnv.dtd;
    g_BattleDrawBuffers[1].drawEnv.dtd = 1;
    *pDtd = 1;
    g_BattleDrawBuffers[1].drawEnv.isbg = 0;
    g_BattleDrawBuffers[0].drawEnv.isbg = 0;
    tpage = GetTPage(0, 2, 0x280, 0);
    g_BattleDrawBuffers[1].drawEnv.tpage = tpage;
    g_BattleDrawBuffers[0].drawEnv.tpage = tpage;
    func_80045534(&g_BattleDrawBuffers[0].drawEnv.dr_env, &g_BattleDrawBuffers[0].drawEnv.clip);
    func_80045534(&g_BattleDrawBuffers[1].drawEnv.dr_env, &g_BattleDrawBuffers[1].drawEnv.clip);
    SetDrawArea(&g_BattleDrawBuffers[0].drawAreaMode, &g_BattleDrawBuffers[0].drawEnv.clip);
    SetDrawArea(&g_BattleDrawBuffers[1].drawAreaMode, &g_BattleDrawBuffers[1].drawEnv.clip);
    func_8004546C(&g_BattleDrawBuffers[0].drawOffsetMode, (RECT*)&g_BattleDrawBuffers[0].drawEnv.ofs);
    func_8004546C(&g_BattleDrawBuffers[1].drawOffsetMode, (RECT*)&g_BattleDrawBuffers[1].drawEnv.ofs);
}

void BattleSetViewport(s32 a0, s32 a1) {
    SetGeomOffset(a0 / 2, a1 / 2);
    SetGeomScreen((a0 << 8) / a0);
    ModelSetScreenSize(a0, a1);
    BattleSetScaleMatrix(a0, a1);
}

void BattleRestoreDrawEnvs(void) {
    BattleInitDrawEnvs(g_BattleScreenWidth, g_BattleScreenHeight);
}

void BattleSetScreenResolution(s32 a0, s32 a1) {
    func_80089330(a0, a1);
    BattleInitDrawEnvs(a0, a1);
}

void BattleInitSpritePrims(BattleSprite* p, s32 x, s32 y, s32 w, s32 h, s32 v) {
    RECT rect;
    RECT rect2;

    rect.x = x;
    rect.y = y;
    rect.w = w;
    rect.h = h;
    SetDrawArea(&p->drawAreaMode0, &rect);
    rect.y = y + v;
    SetDrawArea(&p->drawAreaMode1, &rect);
    rect2.x = x;
    rect2.y = y;
    func_8004546C(&p->drawOffsetMode0, &rect2);
    rect2.y = y + v;
    func_8004546C(&p->drawOffsetMode1, &rect2);
    p->tile0.len = 3;
    p->tile0.code = 0x60000000;
    p->tile0.x = 0;
    p->tile0.y = 0;
    p->tile0.w = w;
    p->tile0.h = h;
    p->tile1 = p->tile0;
    p->flags |= 0xC;
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc12", func_800898BC);

/* Best attempt: objdiff 50.18%. The statement order and the five VectorNormal/
   OuterProduct12 calls are right, but gcc 2.7.2 hoists the materialisation of all
   four base addresses (&D_8009A0C8, &D_80096F98, &D_8009A918, &D_80097000) into
   the prologue - it even interleaves their lui/addiu with the register saves -
   while retail materialises each one right before its first use, after the three
   D_8009A0C8/CC/D0 difference stores. Retail's first store also reuses $s2
   (&D_8009A0C8) that way. Signatures: (MATRIX*, SVECTOR* pFrom, SVECTOR* pTo,
   SVECTOR* pScale); pMat->m rows come from the D_80097000 / D_8009A918 /
   D_80096F98 triples loaded as words and stored as halves, pMat->t = negated
   D_8009A0C8/CC/D0. */
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
    MulMatrix2((MATRIX*)&g_BattleScaleMatrix, pMat);
    pMat->t[0] = -D_8009A0C8;
    pMat->t[1] = -D_8009A0CC;
    pMat->t[2] = -D_8009A0D0;
}
*/
/*@BattleSetMatrixFromPoints*/
void BattleSetMatrixFromPoints(BattleMisc12Owner* pOwner, VECTOR* pFrom, VECTOR* pTo) {
    SVECTOR scale;
    SVECTOR offset;
    SVECTOR zero;

    scale.vy = 0x1000;
    scale.vz = 0;
    scale.vx = 0;
    g_BattleViewOrigin = *pTo;
    offset.vx = pFrom->vx - pTo->vx;
    offset.vy = pFrom->vy - pTo->vy;
    offset.vz = pFrom->vz - pTo->vz;
    zero.vz = 0;
    zero.vx = 0;
    zero.vy = 0;
    func_800898BC((MATRIX*)((u8*)pOwner->pData + 0xC), &offset, &zero, &scale);
}
/*@@*/

BattleMisc12Obj* BattleResetObject(BattleMisc12Obj* p) {
    p->state = 0;
    p->pList = 0;
    p->pHead = 0;
    p->pNext = 0;
    p->pUpdate = 0;
    p->pData = 0;
    p->position.vz = 0;
    p->position.vy = 0;
    p->position.vx = 0;
    p->rotation.vz = 0;
    p->rotation.vy = 0;
    p->rotation.vx = 0;
    p->sourcePos.vz = 0;
    p->sourcePos.vy = 0;
    p->sourcePos.vx = 0;
    p->matrix = g_BattleBaseMatrix;
    p->rotMatrixLo = *(BattleBlk16*)&p->matrix;
    p->rotMatrixHi = *((BattleBlk16*)&p->matrix + 1);
    p->localMatrixLo = p->rotMatrixLo;
    p->localMatrixHi = p->rotMatrixHi;
    return p;
}

void BattleCreateObject(void) {
    void* p;

    HeapSetCurrentContentType(8);
    p = HeapAlloc(0x9C, 0);
    BattleResetObject(p);
}

void BattleAppendListEntry(BattleListNode* pList, BattleListNode* pNode) {
    BattleListNode* pTail;
    BattleListNode* pCur;

    pNode->pList = pList;
    pTail = pList->pHead;
    if (pTail == 0) {
        pList->pHead = pNode;
        return;
    }
    pCur = pTail;
    while (pCur->pNext != 0) {
        pCur = pCur->pNext;
    }
    pCur->pNext = pNode;
}

void BattleRemoveListEntry(BattleListNode* pNode) {
    BattleListNode* pList;
    BattleListNode* pPrev;
    BattleListNode* pCur;

    if (pNode == 0) {
        return;
    }
    pList = pNode->pList;
    if (pList == 0) {
        return;
    }
    pNode->pList = 0;
    pPrev = pList->pHead;
    if (pPrev == 0) {
        return;
    }
    if (pPrev == pNode) {
        pList->pHead = pNode->pNext;
    } else {
        pCur = pPrev;
        if (pCur->pNext != pNode) {
            do {
                pCur = pCur->pNext;
            } while (pCur->pNext != pNode);
        }
        pCur->pNext = pNode->pNext;
    }
    pNode->pNext = 0;
}
