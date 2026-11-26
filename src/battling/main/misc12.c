#include "common.h"
#include "system/memory.h"
#include "system/archive.h"
#include "psyq/libgpu.h"
#include "psyq/libgte.h"
#include "system/controller.h"

// Bit of g_BattleOptionFlags that enables the extra entry BattleAppendExtraQueueEntry queues.
// owned by include/battle/actor.h; this unit carries its own externs and cannot include it
#define BATTLE_SLOT_BIT_COUNT 49

#define BATTLE_OPTION_EXTRA_ENTRY 0x10000
// Slot bits g_BattleSlotBits holds: BattleSetFlag10000IfAllSlotBitsSet tests every one.
// The screen size BattleSetScaleMatrix scales a requested size from.
#define BATTLE_SCREEN_BASE_WIDTH 320
#define BATTLE_SCREEN_BASE_HEIGHT 240

// Node of a singly linked list hung off a 0x98+ byte owner.
typedef struct BattleListNode {
    /* 0x00 */ u8 unk0[0x8C];
    /* 0x8C */ struct BattleListNode* pList;
    /* 0x90 */ struct BattleListNode* pNext;
    /* 0x94 */ struct BattleListNode* pHead;
} BattleListNode;

// Record pData in BattleMisc12Owner points at: the matrix BattleBuildScaledMatrixFromVectors fills.
typedef struct {
    /* 0x00 */ u8 unk0[0xC];
    /* 0x0C */ MATRIX matrix;
} BattleMisc12Data; // Size: 0x2C (only +0xC is reached)

// Owner struct whose field 4 holds the pointer to that record.
typedef struct {
    /* 0x0 */ u32 unk0;
    /* 0x4 */ BattleMisc12Data* pData;
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

// The 0x9C-byte object BattleCreateObject allocates and BattleResetObject clears;
// src/battling/main/misc14.c declares the same record as Misc14ModelNode, and the
// links at 0x8C are the ones BattleAppendListEntry walks.
typedef struct {
    /* 0x00 */ u32 state;
    /* 0x04 */ void* pData;
    /* 0x08 */ void (*pUpdate)(void);
    /* 0x0C */ MATRIX localMatrix;
    /* 0x2C */ SVECTOR sourcePos;
    /* 0x34 */ VECTOR position;
    /* 0x44 */ SVECTOR rotation;
    /* 0x4C */ MATRIX rotMatrix;
    /* 0x6C */ MATRIX matrix;
    /* 0x8C */ BattleListNode* pList;
    /* 0x90 */ BattleListNode* pNext;
    /* 0x94 */ BattleListNode* pHead;
    /* 0x98 */ u32 unk98;
} BattleMisc12Obj; // Size: 0x9C

// g_BattleOptionFlags split the way BattleApplyOptionWord decodes it: a 4-bit mode
// word (which it compares against 1), the two vibration bits, the match count and
// the difficulty level.
typedef struct {
    /* 0x0:0  */ u32 mode : 4;
    /* 0x0:4  */ u32 player1Vibration : 1;
    /* 0x0:5  */ u32 player2Vibration : 1;
    /* 0x0:6  */ u32 matchCount : 7;
    /* 0x0:13 */ u32 difficultyLevel : 3;
    /* 0x0:16 */ u32 unk16 : 16;
} BattleOptionBits; // Size: 0x4

extern MATRIX g_BattleBaseMatrix;
extern MATRIX g_BattleScaleMatrix;
extern MATRIX g_BattleBaseMatrixCopy;

extern u32 g_BattleOptionFlags;
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
extern void BattleBuildScaledMatrixFromVectors(MATRIX*, SVECTOR*, SVECTOR*, SVECTOR*);

// GTE scratch vectors BattleBuildScaledMatrixFromVectors builds the matrix from: the edge (result of
// every VectorNormal), the up vector taken from the caller's scale, and two
// working axes the OuterProduct12 calls fill.
extern VECTOR g_BattleMatrixWorkVector;
extern VECTOR g_BattleMatrixAxisY;
extern VECTOR g_BattleMatrixAxisZ;
extern VECTOR g_BattleMatrixAxisX;

void BattleSetFlag10000(void) {
    u32 value;
    u32* pFlags;

    pFlags = &g_BattleOptionFlags;
    value = *pFlags;
    *pFlags = value | BATTLE_OPTION_EXTRA_ENTRY;
}

void BattleAppendExtraQueueEntry(void) {
    if ((g_BattleOptionFlags & BATTLE_OPTION_EXTRA_ENTRY) == 0) {
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
    u32 value;
    u32* pFlags;

    for (slot = 0; slot < BATTLE_SLOT_BIT_COUNT; slot++) {
        if (slot != 0x16) {
            if (BattleTestSlotBit(slot) == 0) {
                return 0;
            }
        }
    }
    pFlags = &g_BattleOptionFlags;
    value = *pFlags;
    *pFlags = value | BATTLE_OPTION_EXTRA_ENTRY;
    BattleAppendExtraQueueEntry();
    return 0;
}



// Best attempt: objdiff 86.96% at retail's exact 184 bytes / 46 insns. With the
// bitfield body below everything is retail: the single lw, the five clearing masks
// (~0xF, ~0x10, ~0x20, ~0xFFFFC03F, ~0xFFFF1FFF), the four hoisted li masks in
// retail order, the five value registers ($a2,$a1,$a0,$v1,$t0), the four
// lbu/andi/sll/or chains and the final and t0,v1,t0. The residual is that gcc
// stores each chain value right where it completes (sw a2 after and a1,a2,a1, ...)
// while retail keeps the four dead stores together between and t0,v1,t0 and the
// last andi/sll/or.
// Round 492 CORRECTED the round 292 note: the store slots are sched2's, not source
// order. cc1 on this unit .i with -fno-schedule-insns2 puts ONE load and all FIVE
// stores at the END of the block (RTL order = stores last), i.e. retail's sched2
// hoists the four up only 3 slots while ours hoists them ~18; -fschedule-insns is
// a no-op on this body. -fno-schedule-insns2 is a diagnostic, not a preset flag.
// Also measured dead this round: -O3 does not compile the body, cdk -G0 84.13,
// 2.6.0 -G0 86.96 (identical to the unit flags), and a 3-minute permuter run
// (base 1815 best 406 by its own metric) whose winning edit is a constant-1 local
// and whose objdiff score is unchanged at 86.96.
// Round 509: the do-while region boundary (levers h-do-while-is-the-store-barrier)
// cannot move a store LATER here - wrapping each bitfield assignment in its own
// `do { } while (0);` scores 51.80% / 188 bytes / 47 insns, because the boundary
// makes cc1 copy the chain value into $a0 and re-form the 0xFFFF1FFF mask per
// region. The store placement is sched2's hoist distance and has no source handle.
// Shapes that move the OTHER way: five u32 temps then five stores makes gcc delete
// four dead stores (168 bytes / 42 insns, 73.13%), and a volatile u32* keeps all
// five stores (184 bytes) but coalesces the whole chain into one register
// (64.43%).
/*
void func_80088A40(void) {
    BattleOptionBits* pFlags;

    if (g_BattleTransitionFlag == 0) {
        return;
    }
    pFlags = (BattleOptionBits*)&g_BattleOptionFlags;
    pFlags->mode = 1;
    pFlags->player1Vibration = g_BattlePlayer1Vibration & 1;
    pFlags->player2Vibration = g_BattlePlayer2Vibration & 1;
    pFlags->matchCount = g_BattleMatchCount & 0x7F;
    pFlags->difficultyLevel = g_BattleDifficultyLevel & 7;
}*/
INCLUDE_ASM("asm/battling/nonmatchings/main/misc12", func_80088A40);



void BattleApplyOptionWord(void) {
    u32 flags;
    u32* pFlags;
    s32 i;
    u8* pSlotBits;

    if (g_BattleTransitionFlag == 0) {
        return;
    }
    pFlags = &g_BattleOptionFlags;
    flags = *pFlags;
    g_BattleExtraEntryQueued = 0;
    if ((flags & 0xF) == 1) {
        g_BattlePlayer1Vibration = (flags >> 4) & 1;
        g_BattlePlayer2Vibration = (flags >> 5) & 1;
        g_BattleMatchCount = (flags >> 6) & 0x7F;
        g_BattleDifficultyLevel = (flags >> 13) & 7;
        if (flags & BATTLE_OPTION_EXTRA_ENTRY) {
            i = 7;
            // g_BattleSlotBits is the eight-byte slot-bit array that starts just
            // below the option word, so its last byte is pFlags - 1.
            pSlotBits = (u8*)pFlags - 1;
            for (; i >= 0; i--) {
                *pSlotBits-- = 0;
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
    if (buttons & CTRL_BTN_L1) {
        g_BattleHeapDumpScroll++;
    }
    if (buttons & CTRL_BTN_L2) {
        g_BattleHeapDumpScroll--;
    }
    if (g_BattleHeapDumpScroll < 0) {
        g_BattleHeapDumpScroll = 0;
    }
    HeapDebugDump(1, g_BattleHeapDumpScroll, 10,
                  HEAP_DEBUG_PRINT_NO | HEAP_DEBUG_PRINT_ADDRESS | HEAP_DEBUG_PRINT_SIZE |
                      HEAP_DEBUG_PRINT_GETADD | HEAP_DEBUG_PRINT_CONTENTS |
                      HEAP_DEBUG_PRINT_TOTAL_FREE_SIZE);
}

void BattleInitSlotDrawEntry(s32 slot) {
    BattleEntry* pEntry;

    pEntry = &g_BattleDrawBuffers[slot];
    pEntry->len = 3;
    pEntry->code = 0x7D;
    pEntry->uv = 0x3000;
    pEntry->clut = GetClut(0x3F0, 0xC0);
}

extern void* func_80088BFC[];
extern u8 g_BattleHeapContentTypes[];
extern s32 g_DebugTable;
extern BattleEntry* g_BattleDrawSlot;
extern BattleEntry* g_BattleDisplaySlot;
extern s32 g_BattleDebugDrawMode;
extern s16 g_BattleFrameWaitCount;
extern s32 g_BattleMultiplyNodeMatrices;
extern u16 g_BattleDebugFlags;
extern s32 g_BattleDrawFlags;
extern void (*g_BattleDebugDrawCallback)(void*);
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
    HeapChangeCurrentUser(HEAP_USER_SUGI, (char**)g_BattleHeapContentTypes);
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

extern u_long* g_BattleRootOrderingTable;
extern s32 D_80050618;
extern s32 g_BattleFramePolyCount;
extern s32 g_BattleEmittedPacketCount;
extern char D_800706AC[];
extern char D_800706C0[];
extern char D_800706D4[];
extern void* BattleCreateWorkBlock(void* pEntry, u32 arg1, u32 stackTop, s32 size);
extern void func_8008BB3C(void* pWork);
extern void GameCheckAndHandleSoftReset(void);
extern void FontDrawLetters(void* pOt);
extern void FontSetColor(s32 red, s32 green, s32 blue);
extern s32 Vsync(s32 mode);
extern void BattleReleaseFinishedSoundSlots(void);
extern void BattleCompactOrderingTable(s32 limit);
extern void BattleCaptureTimerCount(void);

/* Best attempt: objdiff 96.84% at 812 bytes against retail's 816 (203 vs 204 insns),
   with the frame of 0x40, the 0x14-byte DISPENV copy to sp+0x10 as five lwl/lwr +
   swl/swr pairs, the draw-buffer toggle, every call in order and the divu/mflo rate.
   Round 397 took this from 91.13% to 96.84% with ONE source change: the byte-index
   round-trip needs its own QImode local -
       u8 drawIndex = (drawFlags + 1) & 1;
       g_BattleDrawBufferIndex = drawIndex;
       ... pSlot = &pBuf[drawIndex];
   Spelling it as g_BattleDrawBufferIndex = (drawFlags + 1) & 1; and indexing with
   the global instead lets cc1 substitute the SImode value it already holds, which
   drops retail's andi 0xFF and moves the sb after the g_BattleDisplaySlot store
   (91.13%). The u8 local keeps the value QImode, so the sb sits in retail's place
   and the index use zero-extends, giving retail's andi a0,a0,0xFF.
   What is left, in order of cost:
   - retail's preheader has a DEAD lw of g_SpuChannelMask (lui v0 + lw v0, 8 bytes,
     never read: the loop's first sll overwrites $v0). Initialising prevMask there
     makes the load live in a callee-saved register instead; making either read of
     g_SpuChannelMask plain or volatile in every combination measured here does not
     produce the dead pair, so the loop pass does not hoist it. prevMask is read
     before it is written on the first iteration (subu v1,v1,s3), so the retail
     source read an uninitialised variable and the dead load is not its initialiser.
   - prevMask and pWork hold retail's two registers SWAPPED (retail $s2 = work,
     $s3 = prevMask; here $s3/$s2). Declaring prevMask before pWork and the reverse
     changes nothing: it is a global-alloc priority tie.
   - retail re-forms the g_BattleDrawBuffers base into $v1 between the two address
     computations, ours sits at the loop top in $a2. Dropping the pBuf local and
     indexing g_BattleDrawBuffers directly measures 93.68% at 820 bytes (one
     instruction OVER retail), so the pBuf local stays, and its live use at the end
     of the loop (PutDispEnv(pBuf = &disp)) is the one unrealistic line in the body:
     it is what keeps the base out of a callee-saved register instead of costing a
     fifth saved register (88.17%).
   A 4-minute permuter run from the 91.13% body found only that embedded assignment. */

/* ROUND 420 re-measure: 96.84% at 812 bytes / 203 insns against retail's 816 / 204,
   47 differing lines. Tried the three spellings of the prevMask read that the note
   above left unquantified, on this exact body:
   - prevMask = g_SpuChannelMask (plain): 96.94% at 800 / 200 - cse MERGES it with
     the divu's read, so the loop loses both of retail's loads and gains a
     move in the beqz delay slot.
   - prevMask = *(u32 *)&g_SpuChannelMask (non-volatile pointer cast): same 800 /
     200. A standalone cc1_probe keeps TWO loads for this spelling (the pointer
     access sets MEM_IN_STRUCT_P, so cse cannot merge them), but inside this loop
     cc1 merges them anyway, so it is not a route to retail's two direct-form
     loads here.
   - prevMask = *(volatile s32 *)&g_SpuChannelMask (parked): 812 / 203, the only
     spelling that keeps both loads, but a volatile MEM makes cc1 put the address
     in a register (lui a1 / addiu a1,a1,0 / lw s2,0(a1)) where retail has the
     direct lui s3 / lw s3,%lo(s3) form. Retail's two unmerged loads in one
     basic block therefore came from a MEM-flag difference (a struct/array access
     of the mask) that this unit has no declaration for.
   The other two residuals are unchanged and independent: pWork/prevMask hold
   retail's two callee-saved registers swapped, and ours hoists the
   g_BattleDrawBuffers base into $a2 at the loop top where retail re-forms it into
   $v1 between the two address computations. Three independent allocation
   coincidences plus a compiler artifact (retail's dead preheader lw of
   g_SpuChannelMask) - not reachable from source, so the stub stays. */
// ROUND 434 re-measure: unchanged at 96.84% / 812 bytes / 203 insns against retail's
// 816 / 204, 47 differing lines. Two more measured negatives on retail's TWO unmerged
// reads of g_SpuChannelMask in one basic block, both with cc1_probe.sh on this unit's
// flags:
// - an AGGREGATE read of the same symbol does NOT block cse: g_Sym[0] with the symbol
//   declared as an array, and a struct member at offset 0, both merge to ONE load, so
//   "the unit has no aggregate declaration for the mask" is not the missing piece.
// - declaring the symbol itself volatile does keep both loads but emits the maspsx macro
//   form "lw rd,g_Sym" inside #.set volatile (maspsx expands it through $at), not
//   retail's -G0 split form "lui rd,%hi / lw rd,%lo(rd)".
// The instruction count cannot be reconciled either: the parked body is one short, and
// retail's dead preheader load (lui v0 / lw v0) plus a volatile read costing
// lui+addiu+lw lands one OVER; every measured combination is 203 or 205, never 204.
/*
void func_80088E90(void) {
    DISPENV disp;
    BattleEntry* pBuf;
    BattleEntry* pSlot;
    void* pWork;
    s32 drawFlags;
    u32 prevMask;
    u32 frameRate;
    s32 frameCount;
    u8 drawIndex;

    BattleInitGraphics();
    pWork = BattleCreateWorkBlock(func_80088BFC[D_80050618], 0, 0x801FE000, 0x400);
    for (;;) {
        drawFlags = g_BattleDrawFlags;
        g_BattleFramePolyCount = 0;
        g_BattleEmittedPacketCount = 0;
        pBuf = g_BattleDrawBuffers;
        drawIndex = (drawFlags + 1) & 1;
        g_BattleDrawBufferIndex = drawIndex;
        g_BattleDisplaySlot = &pBuf[drawFlags & 1];
        g_BattleDrawFlags = drawFlags + 1;
        pSlot = &pBuf[drawIndex];
        g_BattleDrawSlot = pSlot;
        g_BattleRootOrderingTable = &pSlot->ot;
        disp = pSlot->dispEnv;
        GameCheckAndHandleSoftReset();
        TermPrim(g_BattleRootOrderingTable);
        if ((g_BattleDebugFlags & 0x10) && (g_BattleDebugDrawCallback != 0)) {
            g_BattleDebugDrawCallback(g_BattleRootOrderingTable);
        }
        FontDrawLetters(g_BattleRootOrderingTable);
        func_8008BB3C(pWork);
        BattleReleaseFinishedSoundSlots();
        HeapTickDelayedFree();
        if (g_BattleControlFlags & 1) {
            AddPrim(g_BattleRootOrderingTable, &g_BattleDrawSlot->tile);
        }
        frameCount = Vsync(1);
        frameRate = 60 / (g_SpuChannelMask - prevMask);
        prevMask = *(volatile s32*)&g_SpuChannelMask;
        if (g_BattleDebugFlags & 8) {
            BattleDebugScrollHeapDump();
        }
        if (g_BattleDebugFlags & 1) {
            FontPrintf(D_800706AC, g_BattleEmittedPacketCount, g_BattleFramePolyCount);
        }
        if (g_BattleDebugFlags & 2) {
            FontPrintf(D_800706C0, frameCount, D_800927F0);
        }
        if (g_BattleDebugFlags & 4) {
            FontPrintf(D_800706D4, frameRate);
        }
        FontSetColor(0xFF, 0xFF, 0xFF);
        BattleCompactOrderingTable(g_BattleFrameWaitCount);
        Vsync(g_BattleFrameWaitCount);
        BattleCaptureTimerCount();
        DrawSync(0);
        PutDispEnv(pBuf = &disp);
        DrawOTagEnv(g_BattleRootOrderingTable, &g_BattleDrawSlot->drawEnv);
    }
}*/
INCLUDE_ASM("asm/battling/nonmatchings/main/misc12", func_80088E90);


void* BattleLoadArchiveEntry(s32 entryIndex) {
    void* pBuffer;

    pBuffer = HeapAlloc(ArchiveDecodeSize(entryIndex), 1);
    ArchiveReadFileToBuffer(entryIndex, pBuffer, 0, 0);
    return pBuffer;
}

void BattleSetScaleMatrix(s32 w, s32 h) {
    s32 scaleW;
    s32 scaleH;

    scaleW = (w << 12) / BATTLE_SCREEN_BASE_WIDTH * h / w;
    scaleH = (h << 12) / BATTLE_SCREEN_BASE_HEIGHT * h / w;
    g_BattleBaseMatrixCopy = g_BattleBaseMatrix;
    g_BattleScaleMatrix = g_BattleBaseMatrix;
    g_BattleScaleMatrix.m[0][0] = scaleW;
    g_BattleScaleMatrix.m[1][1] = scaleH;
}


/* Best attempt: objdiff 97.91% at 508 bytes / 127 insns against retail's
   516 / 129, so it cannot link (the note below used to say 512; re-measured round
   420). The tile packet is a BattleTilePacket at D_8009A1C0 and the 16-byte copy is
   D_8009A2B8 = D_8009A1C0; typing both under that ONE symbol is what makes the body
   work at all - with the tile fields as separate D_8009A1C3/1C4/1C8/1CA/1CC/1CE
   externs the copy's loads look independent of those stores, sched2 hoists the
   copy above them and the body is 79.77%.
   The whole residual is TWO copies: retail does "move v0,s2 / sh v0,0x1CC" and
   "move v0,s1 / ... / sh v0,0x1CE" where ours stores sh s2 / sh s1 in place. A
   sweep of rd/th truncation spellings with cc1_probe.sh on this unit's own flags
   (a snippet carrying the two SetDefDispEnv calls, the g_BattleScreenWidth store,
   the BattleSetScaleMatrix call and the whole tile tail) folds ALL of them to
   sh $sN: plain, u16 locals assigned just before the stores, u16 locals assigned
   at entry and live across the calls, u16 locals assigned at the store site, a
   u16* field pointer, w & 0xFFFF, and u16 locals declared one per store. Earlier
   rounds added a u16/s16 cast, a shared s32 temp, bitfields, a volatile field and
   2.6.0 / 2.7.2-cdk to that list. The ONE construct that makes cse build a fresh
   HImode pseudo is reading the value back out of an s16 global
   (D_8009A1C0.w = g_BattleScreenWidth after g_BattleScreenWidth = w), and that is
   not retail: it needs two more scratch registers and measures 94.77% at 524.
   Conclusion unchanged: retail's copies came from a TU-level construct this unit
   does not have, so 97.91% is the source ceiling and the stub stays. */
// ROUND 434: THE TWO MISSING MOVES ARE A COMPILER VERSION, NOT A SOURCE CEILING - under
// gcc 2.6.0 this body is instruction-for-instruction retail. Re-measured on the unit's
// own flags: 97.91% at 508 bytes / 127 insns against retail's 516 / 129, residual still
// retail's "move v0,s2" before "sh v0,0x1CC" and "move v0,s1" before "sh v0,0x1CE":
// 2.6.0 materialises the truncated halfword in a fresh register where 2.7.2 coalesces
// the subreg.
// Round-434 measurement: try_flags.sh scores this body 99.53% under gcc-2.6.0-psx -O2
// -G0, and an objdump diff of that object against expected/asm/battling/main/misc12.s.o
// (hand-built: cpp, cc1 2.6.0, maspsx --run-assembler) shows the instruction stream,
// registers, frame and branch targets IDENTICAL. The 0.47% is only that the tile stores
// name D_8009A1C0 with addends 3/4/8/A/C/E where retail's object relocs the individual
// field symbols D_8009A1C3/1C4/1C8/1CA/1CC/1CE; both resolve to the same addresses, so
// the linked bytes are identical. 2.7.2-cdk does not compile this unit. 2.6.0 at -G4 or
// -G8 scores the same 99.53 here but drops BattleSetFlag10000 to 44.86, so -G0 stays.
// The unit cannot switch wholesale: under 2.6.0 -G0 BattleSetMatrixFromPoints falls
// 100.00 -> 91.63 (real: its loads come out lhu where retail has lw) and
// BattleBuildScaledMatrixFromVectors 100.00 -> 94.71, so misc12 is a MIXED unit and this
// function needs a preset split: range [0x19840, 0x19A44) as its own 2.6.0 file, with
// the 0x18E50 head and the 0x19A44 tail left on XenoBattlingNoGp.
// Also measured round 434: the untyped spelling (separate D_8009A1C3/1C4/1C8/1CA/1CC/1CE
// externs instead of the BattleTilePacket) is 81.40% under 2.6.0 and fails to compile
// under the unit flags, so the typed tile stores plus the D_8009A2B8 copy are the body.
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
}*/
INCLUDE_ASM("asm/battling/nonmatchings/main/misc12", func_80089330);


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

void BattleBuildScaledMatrixFromVectors(MATRIX* pMat, SVECTOR* pA, SVECTOR* pB, SVECTOR* pC) {
    g_BattleMatrixWorkVector.vx = pB->vx - pA->vx;
    g_BattleMatrixWorkVector.vy = pB->vy - pA->vy;
    g_BattleMatrixWorkVector.vz = pB->vz - pA->vz;
    g_BattleMatrixAxisY.vx = pC->vx;
    g_BattleMatrixAxisY.vy = pC->vy;
    g_BattleMatrixAxisY.vz = pC->vz;
    VectorNormal(&g_BattleMatrixWorkVector, &g_BattleMatrixAxisZ);
    OuterProduct12(&g_BattleMatrixAxisY, &g_BattleMatrixAxisZ, &g_BattleMatrixWorkVector);
    VectorNormal(&g_BattleMatrixWorkVector, &g_BattleMatrixAxisX);
    OuterProduct12(&g_BattleMatrixAxisZ, &g_BattleMatrixAxisX, &g_BattleMatrixWorkVector);
    VectorNormal(&g_BattleMatrixWorkVector, &g_BattleMatrixAxisY);
    pMat->m[0][0] = g_BattleMatrixAxisX.vx;
    pMat->m[0][1] = g_BattleMatrixAxisX.vy;
    pMat->m[0][2] = g_BattleMatrixAxisX.vz;
    pMat->m[1][0] = g_BattleMatrixAxisY.vx;
    pMat->m[1][1] = g_BattleMatrixAxisY.vy;
    pMat->m[1][2] = g_BattleMatrixAxisY.vz;
    pMat->m[2][0] = g_BattleMatrixAxisZ.vx;
    pMat->m[2][1] = g_BattleMatrixAxisZ.vy;
    pMat->m[2][2] = g_BattleMatrixAxisZ.vz;
    ApplyMatrix(pMat, pA, &g_BattleMatrixWorkVector);
    MulMatrix2(&g_BattleScaleMatrix, pMat);
    pMat->t[0] = -g_BattleMatrixWorkVector.vx;
    pMat->t[1] = -g_BattleMatrixWorkVector.vy;
    pMat->t[2] = -g_BattleMatrixWorkVector.vz;
}
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
    BattleBuildScaledMatrixFromVectors(&pOwner->pData->matrix, &offset, &zero, &scale);
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
    p->rotMatrix = p->matrix;
    p->localMatrix = p->rotMatrix;
    return p;
}

void BattleCreateObject(void) {
    void* p;

    HeapSetCurrentContentType(8);
    p = HeapAlloc(sizeof(BattleMisc12Obj), 0);
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
