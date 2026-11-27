#include "common.h"
#include "field/actor.h"
#include "field/main.h"
#include "psyq/libgpu.h"
#include "psyq/libetc.h"
#include "system/archive.h"
#include "system/memory.h"
#include "system/controller.h"
#include "field/effects.h"
#include "field/particles.h"
#include "field/script_vm.h"
#include "main/game.h"

extern int g_FrameDeltaTime;

extern void ArchiveCdDataSync(int mode);
extern s32 D_800ADBD0;
extern s32 g_PlayerActorIndex;
extern s16 g_Unknown_800B2344;

// g_FieldActors is volatile; reading it through a non-volatile lvalue reproduces
// the original's delay-slot filling.
#define FIELD_ACTORS (*((FieldActor**)(void*)&g_FieldActors))

extern s32 g_GameSceneMapNum;
extern int ArchiveLoadStagedFileIfChanged(int fileIndex, s32 arg1);
void FieldInitializeControllersAndMouse(void);
void func_80077884(void);
void func_80077AB4(void);

void FieldInitializeControllers(void) {
    HeapChangeCurrentUser(HEAP_USER_YOSI, NULL);
    ArchiveSetIndex(ARCHIVE_DIR_FIELD, 0);
    FieldInitializeControllersAndMouse();
}

void FieldRenderSync(void) {
    DrawSync(0);
    Vsync(0);
}

extern s32 g_GameUITextureFileStaged;
extern void* g_GameUITextureFileBuffer;
extern s16 D_800C2690;
extern s16 D_800C2692;
extern s16 D_800C38FC;
extern s16 D_800C38FE;
extern RECT D_800B004C;
extern u_long D_800AFC08[];

extern int ArchiveDecodeAlignedSize(unsigned int entryIndex);
extern void ArchiveReadFileToBuffer(s32 index, void* pBuffer, u32 arg2, u32 flags);
extern void ResolveArchiveEntryPointers(u32* header);
extern void FieldLoadTIMWithClut(u_long* pTimData, short x, short y, short clutX, short clutY,
                                 short clutWidth, short clutHeight);

typedef struct {
    s16 x; s16 y; s16 clutX; s16 clutY; s16 clutWidth; s16 clutHeight;
} FieldUITextureRect; // Size: 0xC

extern FieldUITextureRect D_800ADC44[];

// 88.91% (432 bytes / 108 insns vs retail's 428 / 107). ROUND 508 took it 87.32 -> 88.91 by matching
// retail's STATEMENT ORDER, which no earlier round had tried: this body assigns i = 0 first, then the
// table base, then pHeight (base + 5 s16), then pX (base), and only then calls HeapUnpinBlock - retail's
// `move s5,zero` / `lui+addiu base` / `addiu s4,base,0xA` / [HeapUnpinBlock jal slot] `move pX,base`.
// The loop walks pEntry with an explicit `pEntry++` statement (retail increments it in the bnez delay
// slot) and keeps pX/offset/pHeight as three walking values. Two runs measured the same bytes with pX
// assigned either side of HeapUnpinBlock, so that one line is not load-bearing.
// RESIDUAL 1 - the 6-way callee-saved ROTATION, and it is closed on the allocator's own arithmetic, so
// do not spend spellings on it. cc1 -da/-dl/-dg on this body (Default flags): processing order
// `6 regs to allocate: 75 76 73 74 77 72`, dispositions `75 in 16 (s0) 76 in 17 73 in 18 74 in 19 77
// in 20 72 in 21` = pEntry, offset, pX, pHeight, i, base in $s0..$s5; retail needs offset, pEntry,
// base, pX, pHeight, i. lreg: base 5 refs / 76 insns, i 7/40, pHeight 7/37, pX 7/34, offset 7/26,
// pEntry 7/25. The priority is floor_log2(refs)*refs*size/live, so base must clear pX's quotient, which
// takes base to 16 references at a 76-insn live range (or a live range under 24 insns): unreachable
// from any spelling, since retail's own stream pins base's one def and its 4-5 uses. The offset/pEntry
// pair is 1 insn of live range apart and would be the near-miss half if base could move at all.
// RESIDUAL 2 - the middle four arguments, unchanged from earlier rounds: retail re-creates
// base+offset FOUR times (`addu v0,s2,s0` / `addu v1` / `addu a0` / `addu t0`, then lh 2/4/6/8 off
// each) where every C spelling cse-merges to one `addu` and reads all four off it. Six pointer/offset
// spellings cleared with cc1_probe in round 476 plus four locals / int locals / struct-array indexing
// (73-85%) before that; base+offset is ONE value in C, so this is not a spelling question.
// PRESET: -O1 65.89, -O3 and gcc-2.7.2-cdk -G0 do not compile, gcc-2.6.0 identical to Default,
// -G0 scores 92.09 BUT its body is 16 bytes SHORTER than retail and still has a single add, so that
// number is diff-side (the RECT tail's base+disp stores), not progress on residual 2.
// TAIL: retail stores rect.x through $a0 (the address it reuses for StoreImage) and y/w/h through the
// FIELD symbols D_800B004E/50/52 - our struct spelling reaches those by folding, and the store order
// differs only because reorg fills the loop's bnez slot with `li v0,0xFB`. Retail's object also carries
// a dead `addiu s1,s1,-4` in the StoreImage delay slot that no C emits.
// ROUND 530 re-measure: unparked and diffed, still exactly 88.91% / 432 bytes / 108 insns, so the number above is current.
// ROUND 530 also cleared four more spellings of residual 2, all still ONE addu and all worse: the typed
// form 'D_800ADC44[i].y/.clutX/.clutY/.clutWidth' kept alongside the pX/pHeight walkers is 80.39% / 452
// bytes / 113 insns (it loses a saved word, frame 56) and 83.14 under -G0; the same four reads behind a
// 'static inline s16 FieldUITextureHalf(u8* pBase, s32 offset, s32 disp)' helper is 85.75% with 61 diff
// lines (cse folds the sum anyway). cc1 probes on this live body: -fno-rerun-cse-after-loop and
// -fno-strength-reduce both leave FieldLoadUITextures BYTE-IDENTICAL (they do change other functions in
// the unit), so the three missing adds are not a pass a flag switches off. The mechanism, for the next
// lane: under -G8 cc1 keeps 'D_800ADC44 + index' as a symbolic MEM operand ('lh v0,D_800ADC44+2(s0)')
// and never forms a register-register sum for loop.c to duplicate, while retail's four sums come from a
// source-level 'base + offset' that cse1 folds here before loop.c sees it. Coupling worth recording: even
// with the four adds, base's ref count only reaches about 8 (priority 3*8/76 = 0.32) against pX's
// 2*7/34 = 0.41, so base would still land 5th - retail's own six-register hand-out is not predicted by
// the refs/live formula either, i.e. residual 1 is not a ref-count question. Park stands.
/*
void FieldLoadUITextures(void) {
    u8* pBase;
    s16* pX;
    s16* pHeight;
    s32* pEntry;
    s32 offset;
    int i;

    if (g_GameUITextureFileStaged == 0) {
        g_GameUITextureFileBuffer = HeapAlloc(ArchiveDecodeAlignedSize(0xA7), 1);
        HeapPinBlock(g_GameUITextureFileBuffer);
        ArchiveReadFileToBuffer(0xA7, g_GameUITextureFileBuffer, 0, 0x80);
        ArchiveCdDataSync(0);
    }
    i = 0;
    pBase = (u8*)D_800ADC44;
    pHeight = (s16*)pBase + 5;
    pX = (s16*)pBase;
    HeapUnpinBlock(g_GameUITextureFileBuffer);
    g_GameUITextureFileStaged = 0;
    D_800C2692 = 0;
    D_800C2690 = 0;
    D_800C38FE = 0;
    D_800C38FC = 0;
    ResolveArchiveEntryPointers(g_GameUITextureFileBuffer);
    offset = 0;
    pEntry = (s32*)((u8*)g_GameUITextureFileBuffer + 4);
    for (; i < 8; i++) {
        FieldLoadTIMWithClut((u_long*)pEntry[0], *pX, *(s16*)(pBase + offset + 2),
            *(s16*)(pBase + offset + 4), *(s16*)(pBase + offset + 6),
            *(s16*)(pBase + offset + 8), *pHeight);
        pX += 6;
        offset += 0xC;
        pHeight += 6;
        pEntry++;
        DrawSync(0);
    }
    D_800B004C.x = 0;
    D_800B004C.y = 0xFB;
    D_800B004C.w = 0x10;
    D_800B004C.h = 1;
    StoreImage(&D_800B004C, D_800AFC08);
    DrawSync(0);
    HeapFree(g_GameUITextureFileBuffer);
}*/
INCLUDE_ASM("asm/field/nonmatchings/main/main", FieldLoadUITextures);




void FieldWaitForSceneMapFileLoad(void) {
    ArchiveCdDataSync(0);
    while (ArchiveLoadStagedFileIfChanged((g_GameSceneMapNum & 0xFFF) * 2, 0) != 0) {
    }
}

void FieldUpdateDeltaTime(void) {
    g_FrameDeltaTime = Vsync(1);
}

void FieldSetMatrix(s16* pOut, int x0, int x1, int x2, int x3, int x4, int x5, int x6, int x7, int x8) {
    pOut[0] = x0;
    pOut[1] = x1;
    pOut[2] = x2;
    pOut[3] = x3;
    pOut[4] = x4;
    pOut[5] = x5;
    pOut[6] = x6;
    pOut[7] = x7;
    pOut[8] = x8;
}

extern s32 g_FieldActorModelSlotDataBuffers[];
extern s32 g_FieldActorModelSlotSpriteBuffers[];
// The s16 entry table at g_FieldActorModelSlotArchiveBase is followed immediately by its entry count at
// g_FieldActorModelSlotCount; the original reaches both through one base register.
typedef struct {
    /* 0x000 */ s16 entries[0x44];
    /* 0x088 */ s32 count;
} FieldSoundList;

extern StreamDataQueueEntry D_800B2394[];
// Unbounded-array extern on purpose: retail materialises the count's ADDRESS once per
// loop ('lui v1,%hi / addiu v1,v1,%lo') and reaches the count through that register
// ('lw v0,0(v1)'). The scalar form folds 'base + 0x88' back to the bare symbol and then
// forms the reopened base as its own 'la count-136' constant - two values, not one.
extern s32 g_FieldActorModelSlotCount[];
extern void* g_FieldArchiveQueueBuffer;
extern s32 g_FieldStreamDataSize;
extern s32 g_FieldSuppressStateChange;
extern void FieldWaitForArchiveSync(void);
extern void FieldMoveStoredImageToHeap(int nHeapType);
extern void ArchiveStartStreamQueue(StreamDataQueueEntry* pQueue, int arg1, int arg2);

// Round 450 re-measure: 96.38% at retail's exact 560 bytes / 140 insns. Everything matches
// except BOTH loop preheaders: retail emits 'lui v1,hi(count)' / 'addiu v1,v1,lo(count)' /
// 'lw v0,0(v1)' / ... / 'addiu s4,v1,-136', ours emits the same four instructions as
// 'lui v0' / 'addiu v0,v0,0' / 'addiu s4,v0,-136' / 'lw v0,0(v0)' - the base is derived
// BEFORE the load, so local-alloc is free to reuse the address register for the count.
// Nothing in the source reorders those two: the pSoundList assignment has to precede the
// 'for', and gcc emits it first. Everything else in the body is instruction-identical.
// WHAT TOOK IT 90.25 -> 96.38 (keep all three):
//  1. the size select must be TWO complete HeapAlloc calls in an if/else, const arm in the
//     THEN and the ArchiveDecodeAlignedSize arm in the ELSE. gcc then computes the size
//     straight into HeapAlloc's $a0 in both arms and cross-jumps the 'jal HeapAlloc' block,
//     which is exactly retail's 'and a0,v1,a0' / 'j' / 'addu a0,a0,v0' / 'addu a0,v0,zero'
//     and puts retail's 'lui v0,0xFFE2' in the bnez delay slot. A single nSize local
//     (const-then-if, or the note's old call-first if/else) is 90.25 and lands it in $v0.
//  2. 'nStreamSize = g_FieldStreamDataSize;' hoisted above the branch is REQUIRED for
//     retail's 'lw v1,lo(g_FieldStreamDataSize)' above the bnez; read inline it sinks into
//     the arm.
//  3. the array extern + 'pCount = g_FieldActorModelSlotCount;' (decay) is what makes
//     retail's la/lw preheader at all, and the loop bound spelled
//     '((s32*)((u8*)pSoundList + 0x88))[0]' is what produces retail's in-loop
//     'lw v0,0x88(s4)' reload ('pSoundList->count' folds the reload to the symbol).
// Measured worse this round, do not repeat: pSoundList computed inside the loop body with
// the bound inline (90.44, 80 differing lines - it destroys the s2/s4 walker pair), the
// same with the bound as pCount[0] (90.44), the bound as pCount[0] with the base hoisted
// (96.33, 26 differing lines), scalar declaration with a pCount pointer local (94.35), and
// the array extern on the old single-nSize body (86.54 / 142 insns / 568 bytes).
// If the preheader is ever closed, the same change is owed to func_80077AB4's park below,
// which is BETTER on the array extern too (its note's 75.93 vs 68.07) and is now parked on
// the same declaration this file has.
// Round 463 re-measured and closed four more spellings of the preheader (all against this body):
//  - loop bound 'pCount[0]' (the count read through the count SYMBOL): the count address then
//    needs its own callee-saved register across the calls, so the frame grows to 0x40 with an
//    extra 'sw s5' and the body is 141 insns / 564 bytes (95.21 with the pSoundList statement
//    dropped and the base taken from the symbol; 96.62 with it kept, see next line).
//  - 'pCount[0]' bound + the pSoundList assignment moved INSIDE the body: THIS IS RETAIL'S
//    PREHEADER ORDER (count load first, then the base), 96.62%, but 139 insns / 556 bytes -
//    with the count anchored at &count, gcc gives the entries base $s4 and derives the cursor
//    with one 'addiu s2,s4,-136', so retail's second instruction ('move s2,s4') never exists.
//    It cannot link, so it is not the park.
//  - a struct-typed extern at the entries base ('FieldSoundList g_FieldActorModelSlotArchiveBase'
//    + 'pSoundList->count' as the bound): 96.26 / 560 / 30 differing lines - the count load is
//    re-formed from the symbol with a register of its own ('lui v0' + 'lw v0,136(v0)') instead
//    of being reached at 0x88 of the entries base.
// So the two halves of retail's preheader are exclusive from source: the count must be anchored
// at the entries base (0x88(s4), which the parked body reproduces exactly), and the base then
// has to be born BEFORE the test - which is the one instruction order left. Do not re-run these;
// the next idea has to make one instruction serve both roles.
// ROUND 476 re-measure: 96.38%, 560 bytes / 140 insns, retail's exact size. The single residual is
// where 'addiu s4,v1,-136' sits: retail emits it after the count load AND the rotated test (as the
// loop preheader), ours right after the la. Two more spellings measured, both worse:
//  - pSoundList assigned INSIDE the loop body from pCount and a 's16* pEntry = pSoundList->entries'
//    cursor: 95.46 / 552 / 138; the count address ends up in $s2 and entries are reached as
//    '-136(s2)', so the s4 base and the 'move s2,s4' copy both vanish.
//  - base derived straight from the symbol (no pCount local), bound still through the base:
//    96.26 / 560 / 140; correct instruction set, but the hi/lo of the count symbol is formed TWICE
//    ('lui s4' + 'addiu s4,s4,-136' and a separate 'lui v0' + 'lw') because &count and &count-136
//    are not CSE'd when only one spelling mentions pCount. So the base must be derived FROM the
//    pCount register (as the parked body does) - which is what forces it before the test.
/*
void func_80077884(void) {
    int i;
    int nStreamSize;
    FieldSoundList* pSoundList;
    s32* pCount;
    void* pBuffer;

    if (g_FieldActorModelSlotCount[0] == 0) {
        return;
    }

    FieldWaitForArchiveSync();
    ArchiveSetIndex(ARCHIVE_DIR_FIELD, 0);
    FieldMoveStoredImageToHeap(0);

    nStreamSize = g_FieldStreamDataSize;
    if (g_FieldSuppressStateChange == 0) {
        g_FieldArchiveQueueBuffer = HeapAlloc((nStreamSize & 0xFFFFFF) + 0xFFE23FF8, 1);
    } else {
        g_FieldArchiveQueueBuffer = HeapAlloc(ArchiveDecodeAlignedSize(0x6B9), 1);
    }
    FieldMoveStoredImageToHeap(1);

    pCount = g_FieldActorModelSlotCount;
    pSoundList = (FieldSoundList*)((u8*)pCount - 0x88);
    for (i = 0; i < ((s32*)((u8*)pSoundList + 0x88))[0]; i++) {
        D_800B2394[i * 2 + 1].archiveIndex = pSoundList->entries[i] + 0x6BB;
        pBuffer = HeapAlloc(ArchiveDecodeAlignedSize((s16)pSoundList->entries[i] + 0x6BB), 1);
        g_FieldActorModelSlotSpriteBuffers[i] = (s32)pBuffer;
        D_800B2394[i * 2 + 1].pData = pBuffer;
    }

    pCount = g_FieldActorModelSlotCount;
    pSoundList = (FieldSoundList*)((u8*)pCount - 0x88);
    for (i = 0; i < ((s32*)((u8*)pSoundList + 0x88))[0]; i++) {
        D_800B2394[i * 2].archiveIndex = pSoundList->entries[i] + 0x6BA;
        pBuffer = HeapAlloc(ArchiveDecodeAlignedSize((s16)pSoundList->entries[i] + 0x6BA), 0);
        g_FieldActorModelSlotDataBuffers[i] = (s32)pBuffer;
        D_800B2394[i * 2].pData = pBuffer;
    }

    D_800B2394[i * 2].archiveIndex = 0x6B9;
    D_800B2394[i * 2].pData = g_FieldArchiveQueueBuffer;
    D_800B2394[i * 2 + 1].archiveIndex = 0;
    D_800B2394[i * 2 + 1].pData = NULL;

    FieldWaitForArchiveSync();
    ArchiveStartStreamQueue(D_800B2394, 0, 0);
}*/
INCLUDE_ASM("asm/field/nonmatchings/main/main", func_80077884);





extern s32* D_801E8644;
extern void* g_FieldActorModelPointers[];
extern u8 g_FieldBackgroundR;
extern u8 g_FieldBackgroundG;
extern u8 g_FieldBackgroundB;
extern s16 g_FieldGearObjectCapacity;

extern void FieldRenderSyncAndFlush(void);
extern void func_801E738C(s16 arg0);
extern void func_801E742C(s32 arg0, s32 arg1, s32 arg2, s32 arg3, s16 arg4, s32 arg5, s32 arg6,
                          s32 arg7, SVECTOR* pVector);

// The per-slot model working area at 0x800B21EC: an SVECTOR per slot, the
// animation ids the loader stores beside them, the motion steps handed to the
// gear renderer through D_801E8644, and the slot count word that ends it
// (g_FieldActorModelSlotCount). Retail reaches the vectors at +0x0, the ids at
// +0x20, the steps at +0x50 and the count at +0x78 all off one register.
typedef struct {
    /* 0x00 */ SVECTOR vectors[4];
    /* 0x20 */ s32 animationIds[4];
    /* 0x30 */ u8 unk30[0x20];
    /* 0x50 */ s32 motionSteps[10];
    /* 0x78 */ s32 slotCount;
} FieldActorModelSlots; // Size: 0x7C

// PARKED 68.07% (424 bytes / 106 insns; retail 428 / 107) - redo lane, re-measured this round.
// THE STRUCT SPELLING IS THE WIN (keep it): the count reached as its own address and every
// other part of the working area as a byte displacement off it, with the vectors zeroed as
// `pSlots->vectors[i]` and passed as `&pSlots->vectors[i]`, reproduces retail's prologue,
// frame 0x58, both guards (`lw v0,0(s0)` twice off the count address) and ONE pVector walker
// (`sh zero,0/2/4(sN)` + `addiu sN,sN,8` in the jal delay slot). The older `pVector++` body
// split that pointer into two induction registers 8 apart and is strictly worse.
// THE DECLARATION OF g_FieldActorModelSlotCount CHANGES THE SCORE: this body with
// `pCount = g_FieldActorModelSlotCount;` (array decay) is 75.93% at the same 424 bytes /
// 106 insns against 68.07 for the `&g_FieldActorModelSlotCount` form below it. The file now
// declares the symbol as an unbounded ARRAY because round 450 found that is what
// func_80077884 needs, so this park must be un-parked with the decay form; its body below is
// still the scalar spelling and will not compile as-is.
// RESIDUAL, named - TWO loop.c MULT GIVs, which is the whole remaining gap. Retail holds SIX
// values in its loop (i s3, pVector s2, pData s6, nRow s4, clut s5, pSlots s7) plus two
// transients (s0 = i*4, reused as the sprite element address; s1 = i*4, then
// `&animationIds[0] + i*4`) = s0-s7, its exact saved set. It re-forms both array bases every
// iteration: `sll s1,s3,2` once, `lui s0,%hi(SpriteBuffers); addiu; addu s0,s1,s0`, then
// `lw a3,0(s0)` for the argument AND `lw a0,0(s0)` for HeapFree, and
// `addiu v1,s7,0x20; addu s1,s1,v1` for the animation-id store, with the loop bound read as
// `lw v0,0x78(s7)`. Our compile turns the double-use `sprite[i]` and the single-use
// `animationIds[i]` into walkers (`la sN,sym` in the preheader, `addiu sN,sN,4` in the
// latch), keeps only `i*4` unreduced (the ModelPointers access does come out as retail's
// `lw v0,sym($reg)`), and so needs 8 held + 1 = s0-s8. Per lever
// g-qimode-index-blocks-the-mult-giv this is not source-reachable: retail has neither an
// `andi` on the index nor an `sltiu` test, so its counter is s32 and the multiply cannot be
// kept unreduced from C.
// TRIED THIS ROUND, one line each (scalar declaration unless the line says otherwise):
// - byte-offset casts for both sprite uses and for the animation-id store
//   (`*(s32*)((u8*)g_FieldActorModelSlotSpriteBuffers + i * 4)`, `*(s32*)((u8*)pSlots + 0x20 + i * 4)`):
//   byte-identical to the `[i]` spelling, cc1 canonicalises them (75.93 under the array decl).
// - named `offset = i * 4;` used for the three array addresses: 67.56 / 448 / 112, the offsets
//   themselves get givs.
// - source biv `offset = 0;` with `offset += 4;` at the loop bottom: 71.36 but retail's exact
//   428 bytes / 107 insns - the sprite base still walks, pSlots still takes the 9th register,
//   and it adds a `move sN,zero` init retail does not have.
// - `pSprite = sprite[i]` loaded once into a local and passed to BOTH calls: 67.92 / 428 / 107,
//   gcc re-loads it from the walking base anyway.
// - try_flags this round: gcc-2.6.0 -O2 -G8 75.93 (equal to the array-decl body, so not a
//   preset answer), gcc-2.7.2-cdk -O2 -G8 70.60. Previous round: -G0 65.49, cdk -G0 60.94.
// PARKED 68.07% (424 bytes / 106 insns; retail 428 / 107) - redo lane, re-measured this round.
// THE STRUCT SPELLING IS THE WIN (keep it): the count reached as its own address and every
// other part of the working area as a byte displacement off it, with the vectors zeroed as
// `pSlots->vectors[i]` and passed as `&pSlots->vectors[i]`, reproduces retail's prologue,
// frame 0x58, both guards (`lw v0,0(s0)` twice off the count address) and ONE pVector walker
// (`sh zero,0/2/4(sN)` then `addiu sN,sN,8` in the jal delay slot). The `pVector++` body this
// park replaces split that pointer into two induction registers 8 apart and is 3.5 points
// worse (see the declaration note) - do not go back to it.
// THE DECLARATION OF g_FieldActorModelSlotCount CHANGES THE SCORE of this body by 8 points:
// with the unbounded-array extern (`extern s32 g_FieldActorModelSlotCount[];` and
// `pCount = g_FieldActorModelSlotCount;`, array decay) it is 75.93% at the same 424 bytes /
// 106 insns; with the SCALAR extern the file has now it is 68.07% (`pCount =
// &g_FieldActorModelSlotCount` - at 65.88% with the older pVector++ body). It is parked on the
// scalar form because round 437 established it as the one func_80077884 needs (that stub is
// parked at 90.25 and measures 87.24 under the array form). If func_80077884 is settled some
// other way, flip this file to the array form and this body gains 8 points for free.
// CAUTION: the park this replaces carried 72.42% from the ARRAY declaration that round 437
// removed, and its `pCount = g_FieldActorModelSlotCount;` now assigns an int to a pointer, so
// re-measuring it verbatim reads 69.75% through a deref of the count VALUE - undefined
// behaviour, not a valid base. The 68.07 above is well-defined C.
// RESIDUAL, named - TWO loop.c MULT GIVs, which is the whole remaining gap. Retail holds SIX
// values in its loop (i s3, pVector s2, pData s6, nRow s4, clut s5, pSlots s7) plus two
// transients (s0 = i*4, reused as the sprite element address; s1 = i*4, then
// `&animationIds[0] + i*4`) = s0-s7, its exact saved set. It re-forms both array bases every
// iteration: one `sll s1,s3,2`, then `lui s0,%hi(SpriteBuffers); addiu; addu s0,s1,s0` with
// `lw a3,0(s0)` for the argument AND `lw a0,0(s0)` for HeapFree, and
// `addiu v1,s7,0x20; addu s1,s1,v1` for the animation-id store, the loop bound read as
// `lw v0,0x78(s7)`. Our compile turns the double-use `sprite[i]` and the single-use
// `animationIds[i]` into walkers (`la sN,sym` in the preheader, `addiu sN,sN,4` in the
// latch); only `i*4` stays unreduced (the ModelPointers access does come out as retail's
// `lw v0,sym($reg)`), so the body needs 8 held + 1 = s0-s8. Per lever
// g-qimode-index-blocks-the-mult-giv this is not source-reachable: retail has neither an
// `andi` on the index nor an `sltiu` test, so its counter is s32 and the multiply cannot be
// kept unreduced from C.
// TRIED THIS ROUND, one line each (scalar declaration unless the line says otherwise):
// - byte-offset casts for both sprite uses and for the animation-id store
//   (`*(s32*)((u8*)g_FieldActorModelSlotSpriteBuffers + i * 4)`, `*(s32*)((u8*)pSlots + 0x20 + i * 4)`):
//   same 68.07 / 424 / 106, cc1 canonicalises them (75.93 under the array decl), so write the
//   `[i]` form.
// - named `offset = i * 4;` used for the three array addresses: 67.56 / 448 / 112, the offsets
//   themselves get givs.
// - source biv `offset = 0;` with `offset += 4;` at the loop bottom: 71.36 but retail's exact
//   428 bytes / 107 insns - the sprite base still walks, pSlots still takes the 9th register,
//   and it adds a `move sN,zero` init retail does not have.
// - `pSprite = sprite[i]` loaded once into a local and passed to BOTH calls: 67.92 / 428 / 107,
//   gcc re-loads it from the walking base anyway.
// - try_flags this round: gcc-2.6.0 -O2 -G8 75.93 (equal to the array-decl body, so not a
//   preset answer), gcc-2.7.2-cdk -O2 -G8 70.60. Previous round: -G0 65.49, cdk -G0 60.94.
/*
void func_80077AB4(void) {
    FieldActorModelSlots* pSlots;
    s32* pData;
    s32* pCount;
    s32 i;
    s32 clut;
    s32 nRow;

    pCount = g_FieldActorModelSlotCount;
    if (pCount[0] != 0) {
        FieldWaitForArchiveSync();
        FieldRenderSyncAndFlush();
        func_801E738C(g_FieldGearObjectCapacity);
        D_801E8644 = (s32*)((u8*)pCount - 0x28);
        SetBackColor(g_FieldBackgroundR, g_FieldBackgroundG, g_FieldBackgroundB);
        pSlots = (FieldActorModelSlots*)((u8*)pCount - 0x78);
        pData = g_FieldActorModelSlotDataBuffers;
        clut = 0xFC0000;
        nRow = 0xFC;
        for (i = 0; i < pSlots->slotCount; i++) {
            pSlots->vectors[i].vx = 0;
            pSlots->vectors[i].vy = 0;
            pSlots->vectors[i].vz = 0;
            func_801E742C(i, 0, *pData++,
                          g_FieldActorModelSlotSpriteBuffers[i],
                          (s16)(0x240 - ((i + ((u8*)&g_FieldEffects)[0xEB + nRow]) << 6)), 0x100, 0,
                          clut >> 16, &pSlots->vectors[i]);
            nRow++;
            HeapFree((void*)g_FieldActorModelSlotSpriteBuffers[i]);
            pSlots->animationIds[i] = *(s16*)((u8*)g_FieldActorModelPointers[i] + 0x1C);
            clut += 0x10000;
        }
        HeapChangeCurrentUser(8, NULL);
    }
}*/
INCLUDE_ASM("asm/field/nonmatchings/main/main", func_80077AB4);

void FieldLoadAndCreateObjectModels(void) {
    func_80077884();
    func_80077AB4();
}

extern void* g_PartyDataBuffers[];

void FieldPartyAllocateSkinDataBuffers(void) {
    HeapChangeCurrentUser(HEAP_USER_YOSI, NULL);
    g_PartyDataBuffers[0] = HeapAlloc(0x14000, 0);
    g_PartyDataBuffers[1] = HeapAlloc(0x14000, 0);
    g_PartyDataBuffers[2] = HeapAlloc(0x14000, 0);
    HeapPinBlock(g_PartyDataBuffers[0]);
    HeapPinBlock(g_PartyDataBuffers[1]);
    HeapPinBlock(g_PartyDataBuffers[2]);
}

void FieldPartyFreeSkinDataBuffers(void) {
    HeapUnpinBlock(g_PartyDataBuffers[0]);
    HeapUnpinBlock(g_PartyDataBuffers[1]);
    HeapUnpinBlock(g_PartyDataBuffers[2]);
    HeapFree(g_PartyDataBuffers[0]);
    HeapFree(g_PartyDataBuffers[1]);
    HeapFree(g_PartyDataBuffers[2]);
}

extern s32 D_800ADB9C;

// .rodata
// Sized to the 0xC bytes splat assigns this unit's rodata subsegment. The last byte
// (0x6B) sits past the string's terminator but is present in the original binary.
const char D_8006FB80[0xC] = { 'C', 'l', 'e', 'a', 'r', ' ', 'O', 'T', 'A', 'G', 0, 0x6B };

void FieldUpdateFrame(void) {
    D_800ADB9C = Vsync(1);
    FieldClearAndSwapOTag();
    FieldPollControllers();
    if (g_FieldSystemMode == SYSTEM_MODE_PC_HDD) {
        func_80281B00(D_8006FB80);
    }
    FieldScriptUpdateMemoryEachFrame();
}

// FieldMain-related predicate used by callers in misc6; returns -1 while the
// player actor's script flags have 0x800 set and the field is idle.
// 96.67% (permuter score 240): the early-return shape is right and the whole
// tail (gcc 2.7.2 gives the original's negu in $v0) matches; only the first
// two compares differ. The original allocates result=0 in $v0 at entry, the
// literal 1 in $v1 and the D_800ADBD0 load in $a0, and keeps $v0=0 live across
// the second bnez (whose delay slot is a nop, not the extra move v0,zero this
// body emits). That means local-alloc saw the result pseudo live over the load;
// gcc's sched2 only moves that move v0,zero down into the first bne delay slot.
// Not a preset problem: gcc 2.6.0 gives the identical 96.33 shape and
// 2.7.2-cdk 85.67.
// The dead 'new_var' assignment above is load-bearing: it shifts the load from
// $v1 to $v0 (96.33 -> 96.67); a second dead var does nothing more.
// 15th-16th attempts, same shape family, both worse: one merged
// return -(a == 1 && b == 0 && flags) scores 87.50% (the && forces the sltu into
// $a0 and leaves negu v0,a0, the coalescing wall), and if (a == 1 && b == 0)
// return -(...); return 0; scores 91.50% (extra j, the zero return lands after
// the compute block instead of being the branch target).
// Tried and all <= this (13 attempts): s32 result = 0; if (a && b) result =
// ...; return result; (96.00%, leaves negu a0,v0; move v0,a0), two separate
// return 0; early exits, nested ifs (91.50%, extra j), a flags local, a mode
// local, goto out with a result local (95.33%), (u32) and 1 != x operand
// forms, a helper function (44.30%), and a ?: return (91.50%, 128 bytes).
// Permuter: 3 min / 2076 iterations from this body, base 240 best 240 - the
// shape family is exhausted, the next idea has to materialise the 0 before the
// first compare without constant-folding it away.
// Round 450 re-measure: un-parked the body above, still 96.67% / 120 bytes (retail's size). 's32 result = 0; ... return result;' on the guard path is 96.33% (worse: the zero return lands in $v0 but the compute rematerialises), so the new_var body stays the best.
// Round 463 re-measured the if/else form (it IS retail's block shape: 'if (a != 1 || b != 0) result = 0; else result = ...; return result;' puts the 0 in the first bne delay slot exactly as retail does) but the guard's registers come out swapped: retail has the D_800ADBD0 load in $a0 and the constant 1 in $v1, this shape has the load in $v1 and the 1 in $v0 (96.33%, 120 bytes, 16 differing lines). 's32 mode = D_800ADBD0;' as a statement above the test is byte-identical to it, and the 'result = 0; if (a == 1 && b == 0) return -(...); return result;' two-return form costs an extra j (91.50 / 128 bytes). So the guard's allocation is the whole residual, not the exit-block shape.
/*
s32 func_80077E10(void) {
    int new_var;
    new_var = (D_800ADBD0 != 1) || (g_Unknown_800B2344 != 0);
    if (D_800ADBD0 != 1 || g_Unknown_800B2344 != 0) {
        return 0;
    }
    return -((FIELD_ACTORS[g_PlayerActorIndex].pActorData->scriptFlags.flags & 0x800) != 0);
}*/
INCLUDE_ASM("asm/field/nonmatchings/main/main", func_80077E10);




// FieldMain's field-state globals; none of them have a home header yet.
extern s32 g_DebugTable;
extern s32 g_FieldWdsFileEntry;
extern s32 g_SoundWdsEntryFile3;
extern s32 g_GameWdsEntrySlot3;
extern s32 D_80062524;
extern s32 g_GamePartySkinsInitialized;
extern s32 g_GamePartyMemberSkins[3];
extern s32 g_FieldStagedFileSyncHold;
extern s32 g_FieldMapChangeRequestGate;
extern s32 g_FieldMapChangeGate;
extern s32 g_FieldGameStateChangeRequest;
extern s32 g_GameWdsBankHeaderReadPending;
extern s32 g_GameWdsStreamInProgress;
extern s32 g_FieldScratchpadOffset;
extern s32 g_FieldScriptDataStreamActive;
extern s32 g_FieldScreenImageStored;
extern s32 g_FieldStreamReadyFlag;
extern s32 D_800ADC04;
extern u16 g_NextMapId;
extern u16 g_NextMapFlags;
extern u16 g_NextMapRotationY;
extern u16 g_GamePendingWdsBankIndex;
extern s32 g_GameSceneMapNum;
extern s8 g_GameDiscChangeRequest;
extern s32 g_GameRequestedWdsBankIndex;
extern s32 g_GamePartyUsesGearSkins;
extern s32 g_FieldInitialWdsBankIndex;
extern u8 g_SoundEffectBankIndex;
extern u8 g_FieldWdsSoundEffectBankIndex;
extern s32 g_GameLoadedWdsBankIndex;
extern s32 g_GameReusePendingAudioManager;
extern s32 g_GameWdsBankLoadState;
extern s16 g_FieldPendingWdsBankIndex;
extern s32 g_FieldMapNumChanged;
extern s32 g_FieldPartyModelLoadState;
extern s32 g_FieldSpecialAnimLoadPending;
extern s32 g_FieldDisableCompassRender;
extern s32 g_FieldP2RenderToggle;
extern s32 g_FieldDisableGearRender;
extern u16 g_FieldButtonsPressedOnce;
extern u16 g_FieldP1ButtonsPressed;
extern u16 g_FieldP2ButtonsPressedOnce;
extern u8 g_FieldPauseDisabled;
extern s32 g_FieldRequestCode;
extern u8 g_FieldRandomEncounterFlag;
extern u16 g_FieldScriptShopIndex;
extern s8 g_ShopIndex;
extern s32 g_GameStagedFileTag;
extern HeapBlock* g_GameStagedFileBuffer;
extern s32 g_GameStagedFileSyncCount;
extern s32 g_FieldStreamSceneActive;
extern s32 g_FieldScreenSetupDone;
extern s32 g_SoundAudioManager;
extern s32 g_GamePartyQueuedSkinKind;
extern u8 g_FieldPlayerWalkmeshMaterial;
extern u8 D_800ADB04;
extern s32 D_800ADB68;
extern s32 g_GameAudioManagerVolumeRestorePending;

extern void FieldUpdateDeltaTime(void);
extern void FieldRenderSyncAndFlush(void);
extern void FieldLoadSedsFile(s32 flags);
extern void FieldInitializeDefaultParticleBanks(s32 actorIndex);
extern void FieldLoadSceneAndRun(void);
extern void FieldRunScriptFadeEffect(void);
extern void FieldUnloadSceneAndFade(void);
extern void FieldFreeSedsFile(void);
extern void FieldScriptSyncMemoryWithGameState(void);
extern void FieldTextBoxFreeAll(void);
extern void FieldParticlesFreeAll(void);
extern void FieldResetActorSoundSlots(void);
extern void FieldUpdatePlayerWalkmeshMaterial(void);
extern void FieldRestoreScreenImage(void);
extern void FieldScriptUpdateMemoryEachFrame(void);
extern void FieldTickSkipCounter(void);
extern void FieldRunStreamedSequence(void);
extern void FieldAdvanceGameState(s32 state);
extern void FieldFree(void);
extern s32 FieldScriptMemoryWriteU16(s32 index, u16 value);
extern void FieldStartWdsBankLoad(s16 bankIndex, s32 arg1);
extern s32 func_80078BC8(void);
extern void func_802811EC(void);
extern void func_8007554C(void);
extern void func_800A3F4C(void);
extern void func_800799D4(void);
extern void func_800ABA98(void);
extern void func_800ACE90(void);
extern void ControllerResetState(void);
extern void GameCheckAndHandleSoftReset(void);
extern void SoundMuteAllSpuChannels(void);
extern void SoundEnableAllSpuChannels(void);
extern void GraphicsDrawPauseLetters(s16 x, s16 y);
extern void SoundSetAudioManagerVolumeWithFade(s32 audioManager, s32 volume, s32 frames);
// Called with two arguments in one place and with none in another, so the
// original compiled against a declaration with no prototype.
extern void SoundFreeLoadedWdsAndAudioManager();
extern s32 ArchiveDataSync(void);
extern s32 ControllerGetType(s32 port);
extern void FieldPollControllers(void);

void FieldMain(void) {
    // Never touched: retail's frame spends 8 bytes below the saves (0x10-0x17) on
    // a local that the optimiser emptied out, and only an addressable local buys them.
    u_long scratch[2];
    s32 voiceBankLoaded;
    s32 walkmeshUpdateStarted;
    s32 advanceState;

    if (g_DebugTable != -1) {
        g_FieldSystemMode = SYSTEM_MODE_PC_HDD;
    } else {
        g_FieldSystemMode = SYSTEM_MODE_CD_ROM;
    }
    FieldRenderSyncAndFlush();
    if (g_FieldSystemMode == SYSTEM_MODE_PC_HDD) {
        DrawSyncCallback(FieldUpdateDeltaTime);
    }
    g_FieldWdsFileEntry = g_SoundWdsEntryFile3;
    D_80062524 = g_GameWdsEntrySlot3;
    HeapChangeCurrentUser(HEAP_USER_YOSI, NULL);
    if (g_FieldSystemMode == SYSTEM_MODE_PC_HDD && g_FieldSuppressStateChange == 0) {
        ArchiveSetIndex(ARCHIVE_DIR_FIELD, 0);
        ArchiveReadFileToBuffer(0xAD, (void*)0x80280000, 0, 0x80);
        ArchiveCdDataSync(0);
        FieldRenderSyncAndFlush();
    }
    if (g_GamePartySkinsInitialized == 0) {
        g_GamePartyMemberSkins[2] = CHARACTER_ID_NONE;
        g_GamePartyMemberSkins[1] = CHARACTER_ID_NONE;
        g_GamePartyMemberSkins[0] = CHARACTER_ID_NONE;
    }
    FieldLoadSedsFile(0);
    walkmeshUpdateStarted = 0;
    FieldPartyAllocateSkinDataBuffers();
    g_FieldStagedFileSyncHold = -1;
    g_FieldEntryStateGate = -1;
    g_FieldMapChangeRequestGate = -1;
    g_FieldMapChangeGate = -1;
    g_FieldGameStateChangeRequest = -1;
    g_GameWdsBankHeaderReadPending = 0;
    g_GameWdsStreamInProgress = 0;
    g_FieldScratchpadOffset = 0;
    g_FieldScriptDataStreamActive = 0;
    g_FieldScreenImageStored = 0;
    g_FieldStreamReadyFlag = 0;
    D_800ADC04 = 2;
    if (g_FieldSystemMode == SYSTEM_MODE_PC_HDD) {
        func_802811EC();
    }
    FieldInitializeControllers();
    g_pGameState = &g_GameState;
    g_GameSceneMapNum = g_NextMapId;
    g_GameState.fieldScriptMemory[1] = g_NextMapFlags;
    g_GameState.fieldScriptMemory[4] = g_NextMapRotationY >> 9;
    if (g_FieldScreenSetupDone == 0) {
        g_GameDiscChangeRequest = 0;
        g_GameRequestedWdsBankIndex = 0xFF;
    } else {
        g_GameRequestedWdsBankIndex = g_GamePendingWdsBankIndex;
    }
    if (g_FieldSystemMode == SYSTEM_MODE_CD_ROM) {
        g_pGameState->fieldScriptMemory[0x28] = 1;
        FieldScriptMemoryWriteU16(0x50, 1);
    }
    FieldLoadUITextures();
    g_GamePartyUsesGearSkins = 0;
    GamePartySyncSkinData();
    GamePartySyncStreamedData();
    g_FieldStreamDataSize = HeapAlloc(4, 1);
    if (g_FieldSystemMode == SYSTEM_MODE_PC_HDD) {
        asm("break 0x400");
        FieldInitializeDefaultParticleBanks(g_PlayerActorIndex);
        g_FieldDefaultParticleBanks[0].unk0 = 1;
        g_FieldDefaultParticleBanks[0].max = 0x10;
    }
    voiceBankLoaded = 0;
    FieldLoadSceneAndRun();
    D_800ADB04 = 1;
    for (;;) {
        if (ControllerGetType(0) == 0) {
            advanceState = g_SpuChannelMask;
            SoundMuteAllSpuChannels();
            GraphicsDrawPauseLetters(0x88, (((g_FieldCurRenderContextIndex + 1) & 1) << 8) | 0x64);
            do {
                DrawSync(0);
                Vsync(2);
                FieldPollControllers();
                GameCheckAndHandleSoftReset();
            } while (ControllerGetType(0) == 0);
            SoundEnableAllSpuChannels();
            g_SpuChannelMask = advanceState;
        }
        if ((g_FieldButtonsPressedOnce & 0x800) && !(g_FieldP1ButtonsPressed & 0x40) &&
            g_FieldPauseDisabled == 0) {
            advanceState = g_SpuChannelMask;
            SoundMuteAllSpuChannels();
            GraphicsDrawPauseLetters(0x88, (((g_FieldCurRenderContextIndex + 1) & 1) << 8) | 0x64);
            do {
                DrawSync(0);
                Vsync(2);
                FieldPollControllers();
                GameCheckAndHandleSoftReset();
            } while (!(g_FieldButtonsPressedOnce & 0x800));
            SoundEnableAllSpuChannels();
            g_SpuChannelMask = advanceState;
        }
        if (g_FieldSystemMode == SYSTEM_MODE_CD_ROM) {
            FieldScriptMemoryWriteU16(0x50, 1);
        }
        GameCheckAndHandleSoftReset();
        FieldUpdateFrame();
        func_8007554C();
        FieldRunScriptFadeEffect();
        if (g_FieldCurRenderContextIndex == 1 && g_FieldMapChangeGate == 0 && func_80078BC8() == 0 &&
            func_80077E10() == 0) {
            if (g_GameStagedFileTag != -1) {
                HeapUnpinBlock(g_GameStagedFileBuffer);
                HeapFree(g_GameStagedFileBuffer);
            }
            if (!voiceBankLoaded) {
                voiceBankLoaded = 1;
                g_FieldInitialWdsBankIndex = g_GameRequestedWdsBankIndex;
            }
            FieldTextBoxFreeAll();
            if (D_800ADBD0 == 1) {
                g_SoundEffectBankIndex = g_FieldWdsSoundEffectBankIndex;
                g_FieldInitialWdsBankIndex = g_GameRequestedWdsBankIndex;
                if (g_GameLoadedWdsBankIndex != g_FieldPendingWdsBankIndex) {
                    if (g_GameLoadedWdsBankIndex != -1) {
                        g_GameReusePendingAudioManager = 1;
                    }
                    SoundFreeLoadedWdsAndAudioManager(g_FieldPendingWdsBankIndex, g_GameLoadedWdsBankIndex);
                    g_GameWdsBankLoadState = -1;
                    g_GameRequestedWdsBankIndex = g_FieldPendingWdsBankIndex;
                    FieldStartWdsBankLoad(g_FieldPendingWdsBankIndex, 1);
                }
                D_800ADBD0 = 0;
                g_GameAudioManagerVolumeRestorePending = 1;
            } else {
                if (g_FieldMapNumChanged == 0) {
                    g_GamePartySkinsInitialized += 1;
                    func_800A3F4C();
                }
                advanceState = 0;
                if (g_GameAudioManagerVolumeRestorePending == 1) {
                    SoundSetAudioManagerVolumeWithFade(g_SoundAudioManager, 0x7F, 0);
                }
                g_GameAudioManagerVolumeRestorePending = 0;
                break;
            }
        }
        if (g_FieldMapTransitionGate == 0 && g_GameWdsBankLoadState == 0 &&
            g_FieldPartyModelLoadState == 0xFF && g_FieldSpecialAnimLoadPending == 0 &&
            ArchiveLoadStagedFileIfChanged((g_GameSceneMapNum & 0xFFF) * 2, 0) == 0 &&
            ArchiveDataSync() == 0 && g_FieldEffects.fades[0].duration == 0) {
            D_800ADB04 = 0;
            FieldScriptSyncMemoryWithGameState();
            ArchiveCdDataSync(0);
            FieldUnloadSceneAndFade();
            ControllerResetState();
            D_800ADB04 = 1;
        }
        if (g_FieldCurRenderContextIndex == 1 && g_FieldEntryStateGate == 0 && func_80078BC8() == 0) {
            ArchiveCdDataSync(0);
            advanceState = 1;
            if (g_GameStagedFileTag != -1) {
                HeapUnpinBlock(g_GameStagedFileBuffer);
                HeapFree(g_GameStagedFileBuffer);
            }
            break;
        }
        if (g_FieldCurRenderContextIndex == 1 && g_FieldStagedFileSyncHold == 0 && func_80078BC8() == 0) {
            ArchiveCdDataSync(0);
            if (g_GameStagedFileTag != -1) {
                HeapUnpinBlock(g_GameStagedFileBuffer);
                HeapFree(g_GameStagedFileBuffer);
            }
            g_GameStagedFileSyncCount += 1;
            advanceState = 2;
            func_800A3F4C();
            break;
        }
        if (g_FieldCurRenderContextIndex == 1 && g_FieldGameStateChangeRequest == 0 &&
            func_80078BC8() == 0) {
            ArchiveCdDataSync(0);
            if (g_GameStagedFileTag != -1) {
                HeapUnpinBlock(g_GameStagedFileBuffer);
                HeapFree(g_GameStagedFileBuffer);
            }
            advanceState = 3;
            SoundFreeLoadedWdsAndAudioManager();
            break;
        }
        if (g_FieldSystemMode == SYSTEM_MODE_PC_HDD) {
            if (g_FieldP2ButtonsPressedOnce & 0x40) {
                g_FieldDisableCompassRender = (g_FieldDisableCompassRender + 1) & 1;
            }
            if (g_FieldP2ButtonsPressedOnce & 0x10) {
                g_FieldP2RenderToggle = (g_FieldP2RenderToggle + 1) & 1;
            }
            if (g_FieldP2ButtonsPressedOnce & 0x80) {
                g_FieldDisableGearRender = (g_FieldDisableGearRender + 1) & 1;
            }
            if ((g_FieldP1ButtonsPressed & 0x40) && (g_FieldButtonsPressedOnce & 0x100) &&
                g_FieldMapTransitionGate == -1 && g_GameWdsBankLoadState == 0 &&
                g_FieldScreenImageStored == 0) {
                g_GameSceneMapNum = 0;
                g_FieldMapTransitionGate = 0;
                FieldScriptMemoryWriteU16(2, 0);
            }
        }
        if (g_FieldGameStateChangeRequest == -1 && g_FieldMapChangeGate == -1 &&
            g_FieldEntryStateGate == -1 && func_80078BC8() == 0 && g_FieldMapTransitionGate == -1) {
            if (!(g_FieldP1ButtonsPressed & 3)) {
                walkmeshUpdateStarted = 0;
            }
            if ((g_FieldP1ButtonsPressed & 1) && D_800ADB68 == 1 && (g_FieldP1ButtonsPressed & 2) &&
                walkmeshUpdateStarted == 0) {
                walkmeshUpdateStarted = 1;
                FieldUpdatePlayerWalkmeshMaterial();
                if (g_FieldRequestCode == 0xFF &&
                    !(FIELD_ACTORS[g_PlayerActorIndex].pActorData->scriptFlags.flags & 0x1800) &&
                    g_FieldPlayerWalkmeshMaterial == 0) {
                    func_800ACE90();
                }
            }
            if ((g_FieldButtonsPressedOnce & 0x100) && g_FieldMapTransitionGate == -1 && D_800ADB68 == 1) {
                func_800ABA98();
            }
            if (g_FieldStreamSceneActive != 0 && g_FieldCurRenderContextIndex == 1) {
                FieldRunStreamedSequence();
                g_FieldStreamSceneActive = 0;
            }
            if (g_FieldRequestCode != 0xFF && g_FieldCurRenderContextIndex == 0 &&
                !(FIELD_ACTORS[g_PlayerActorIndex].pActorData->scriptFlags.flags & 0x1800)) {
                FieldTextBoxFreeAll();
                func_800799D4();
                g_FieldRequestCode = 0xFF;
            }
            if ((g_FieldButtonsPressedOnce & 0x10) && g_FieldRandomEncounterFlag == 0 &&
                g_FieldRequestCode == 0xFF && D_800ADB68 == 1) {
                g_FieldRequestCode = 0x80;
                g_ShopIndex = g_FieldScriptShopIndex;
            }
        }
        FieldTickSkipCounter();
    }
    FieldUpdatePlayerWalkmeshMaterial();
    FieldRestoreScreenImage();
    FieldScriptUpdateMemoryEachFrame();
    FieldParticlesFreeAll();
    FieldResetActorSoundSlots();
    FieldTextBoxFreeAll();
    DrawSync(0);
    Vsync(0);
    FieldFree();
    FieldPartyFreeSkinDataBuffers();
    FieldFreeSedsFile();
    g_GamePartyQueuedSkinKind = 0;
    HeapFree((void*)g_FieldStreamDataSize);
    FieldAdvanceGameState(advanceState);
}
