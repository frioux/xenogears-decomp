#include "common.h"
#include "field/actor.h"
#include "field/main.h"
#include "psyq/libgpu.h"
#include "psyq/libetc.h"
#include "system/archive.h"
#include "system/memory.h"
#include "system/controller.h"
#include "field/effects.h"

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

INCLUDE_ASM("asm/field/nonmatchings/main/main", FieldLoadUITextures);
// 87.32% (432 bytes vs 428; the frame is now exact: 0x40 with s0-s5+ra, and the
// loop prologue's `move s5,zero` / `lui s2,%hi(D_800ADC44)` order matches). The
// remaining gap is the middle four arguments of FieldLoadTIMWithClut: the original
// computes `addu $vN,$s2,$s0` FOUR times (destinations v0/v1/a0/t0) and reads
// `lh 2/4/6/8($vN)`, i.e. it keeps the table base in a register and re-creates the
// base+offset sum per argument; every C form tried here makes cse merge them into
// one `addu` (this version: `addu v1,s5,s3` then all four loads off v1), which is
// 3 insns short and shifts every later offset. Tried: struct array indexed
// (pTable[i].y, 85.72%, also made the base walk), byte base + s32 offset variable
// and `*(s16*)(pBase + offset + K)` (this one), `D_800ADC44[i*6+K]` flat s16, and
// pointer locals for all six. NOTE for the next attempt: D_800ADC44 must be
// declared once (a struct array; the old `extern s16 D_800ADC44[];` at the top of
// the file conflicts with it, cc1 exits 33), and the height walker starts at
// table + 0xA (`(s16*)D_800ADC44 + 5`), not at entry 5.
// Also still open: the D_800B004C RECT stores - the original writes x through the
// a0 it reuses for StoreImage and y/w/h through `lui at`, in that order; this body
// gets the same relocations but emits the x store after w.
/*
void FieldLoadUITextures(void) {
    u8* pBase;
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
    HeapUnpinBlock(g_GameUITextureFileBuffer);
    g_GameUITextureFileStaged = 0;
    D_800C2692 = 0;
    D_800C2690 = 0;
    D_800C38FE = 0;
    D_800C38FC = 0;
    ResolveArchiveEntryPointers(g_GameUITextureFileBuffer);
    pBase = (u8*)D_800ADC44;
    pX = (s16*)pBase;
    pHeight = (s16*)pBase + 5;
    pEntry = (s32*)((u8*)g_GameUITextureFileBuffer + 4);
    offset = 0;
    for (i = 0; i < 8; i++) {
        FieldLoadTIMWithClut((u_long*)pEntry[i], *pX, *(s16*)(pBase + offset + 2),
            *(s16*)(pBase + offset + 4), *(s16*)(pBase + offset + 6),
            *(s16*)(pBase + offset + 8), *pHeight);
        pX += 6;
        offset += 0xC;
        pHeight += 6;
        DrawSync(0);
    }
    D_800B004C.x = 0;
    D_800B004C.y = 0xFB;
    D_800B004C.w = 0x10;
    D_800B004C.h = 1;
    StoreImage(&D_800B004C, D_800AFC08);
    DrawSync(0);
    HeapFree(g_GameUITextureFileBuffer);
}
*/

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
extern s32 g_FieldActorModelSlotCount;
extern void* g_FieldArchiveQueueBuffer;
extern s32 g_FieldStreamDataSize;
extern s32 g_FieldSuppressStateChange;
extern void FieldWaitForArchiveSync(void);
extern void FieldMoveStoredImageToHeap(int nHeapType);
extern void ArchiveStartStreamQueue(StreamDataQueueEntry* pQueue, int arg1, int arg2);

INCLUDE_ASM("asm/field/nonmatchings/main/main", func_80077884);
// 90.25%: structure, frame (0x38, s0-s4 + ra) and size (560 bytes, same as the
// original) are right; the two remaining gaps are both gcc rematerialising where the
// original reuses a materialised address:
//  1. the original builds &g_FieldActorModelSlotCount into $v1 (lui+addiu), loads the count with
//     `lw 0($v1)` and then `addiu $s4,$v1,-0x88` for the s16 table at g_FieldActorModelSlotArchiveBase,
//     reloading the count inside the loop as `lw 0x88($s4)`. This source computes the
//     base as one constant (`lui %hi(g_FieldActorModelSlotCount); addiu -0x88`) and re-forms a second
//     lui/%lo pair for the count instead; gcc's fold_rtx always folds the +0x88 back to
//     the bare symbol, so the two accesses are never expressed off one register. Same
//     wall as FieldRenderGearFrame (misc2) and func_800A0EE8 (misc6), just in the other
//     direction. Pointer locals (pCount/pTable, and `(s32*)((u8*)pTable + 0x88)`) give
//     90.56% but 580 bytes.
//  2. the `g_FieldSuppressStateChange` size select: the original hoists `lw $v1,%lo(g_FieldStreamDataSize)` above
//     the branch and keeps the constant path as the fall-through with a `j` at the end;
//     both `if (x == 0) const else call` and a pre-computed `nSize` give the same size
//     but reorder the block.
// Note the pData stores below relocate against D_800B2394+4, splat prints them as
// D_800B2398; that one is link-identical and only costs objdiff points.
/*
void func_80077884(void) {
    int i;
    int nSize;
    FieldSoundList* pSoundList;
    void* pBuffer;

    if (g_FieldActorModelSlotCount == 0) {
        return;
    }

    FieldWaitForArchiveSync();
    ArchiveSetIndex(ARCHIVE_DIR_FIELD, 0);
    FieldMoveStoredImageToHeap(0);

    nSize = (g_FieldStreamDataSize & 0xFFFFFF) + 0xFFE23FF8;
    if (g_FieldSuppressStateChange != 0) {
        nSize = ArchiveDecodeAlignedSize(0x6B9);
    }
    g_FieldArchiveQueueBuffer = HeapAlloc(nSize, 1);
    FieldMoveStoredImageToHeap(1);

    pSoundList = (FieldSoundList*)((u8*)&g_FieldActorModelSlotCount - 0x88);
    for (i = 0; i < pSoundList->count; i++) {
        D_800B2394[i * 2 + 1].archiveIndex = pSoundList->entries[i] + 0x6BB;
        pBuffer = HeapAlloc(ArchiveDecodeAlignedSize((s16)pSoundList->entries[i] + 0x6BB), 1);
        g_FieldActorModelSlotSpriteBuffers[i] = (s32)pBuffer;
        D_800B2394[i * 2 + 1].pData = pBuffer;
    }

    pSoundList = (FieldSoundList*)((u8*)&g_FieldActorModelSlotCount - 0x88);
    for (i = 0; i < pSoundList->count; i++) {
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
}
*/
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

// PARKED 66.92% (460 bytes / 115 insns; retail 428 bytes / 107). Re-measured from
// scratch this round: the old 64.39 note is superseded and its body is replaced by
// the one below, which takes its loop shape from the twin misc6
// FieldScriptVMHandlerUpdateActorModelSlot case 2 (same func_801E742C call:
// (s16)(0x240 - (offset << 6)), 0x100, 0, (s16)(nSlot + 0xFC), &vectors[nSlot]).
// VERIFIED STRUCTURE (keep): count guard, FieldWaitForArchiveSync,
// FieldRenderSyncAndFlush, func_801E738C(g_FieldGearObjectCapacity),
// D_801E8644 = g_FieldMotionSlotSteps, SetBackColor, the slot loop,
// HeapChangeCurrentUser(8, NULL). Layout, in words off &g_FieldActorModelSlotCount
// (0x800B2264): count at +0x00, g_FieldMotionSlotSteps at -0x28 (0x800B223C),
// g_FieldModelSlotScratch at -0x78 (0x800B21EC) with the SVECTOR the loop zeroes at
// its +0x00 and the s32 animation ids it fills at its +0x20 (i.e. count - 0x58),
// i.e. misc6's FieldModelSlotScratch. The third argument's row byte is
// g_FieldActorModelSpriteRow[i]: retail's lbu %lo(g_FieldEffects+0xEB)(at) with
// at = hi + 0xFC + i resolves to 0x800B225F + i = g_FieldActorModelSpriteRow, so the
// source keeps ONE counter that starts at 0xFC and is used both as that byte index
// and as clut >> 16 (clut starting at 0xFC0000, += 0x10000 per slot). The data-buffer
// argument is a POST-INCREMENTED pointer (lw a2,0(s6) then addiu s6,s6,4 in the jal
// delay slot); the sprite buffer is read twice from one indexed address (argument and
// HeapFree).
// THE WALL, and it is the whole remaining cost: retail materialises &count into $s0
// in the prologue (its sw s0,0x30(sp) comes before sw ra) and does everything off that
// ONE register - lw 0(s0) for the count twice, addiu v0,s0,-40 for D_801E8644,
// addiu s7,s0,-120 for the scratch, and inside the loop lw 0x78(s7) plus
// addiu v1,s7,32. This build folds every *pCount load back to the maspsx
// lui %hi / lw %lo macro and re-forms lui/addiu for the two address constants, which
// is +1 instruction per count read and per address (the budget above).
// MEASURED DEAD this round, all landing on 66.92 / 460 except where noted:
// pCount = &g_FieldActorModelSlotCount with *pCount; the same symbol re-declared as an
// unbounded array (extern s32 g_FieldActorModelSlotCount[]; pCount = symbol; pCount[0],
// 63.28 at 464 - the array giv also lands at -2(s2)); the pointer assigned after a
// direct-global guard; (s32*)(mem_addr)&g_FieldActorModelSlotCount; the loop count
// reloaded through the scratch base as ((s32*)(pScratch + 0x78))[0], misc3's round-306
// spelling that did reproduce a lw 0x80(s2) there. Presets: gcc-2.6.0 66.92, -O3 66.92,
// -G0 63.28, 2.7.2-cdk compile-fails (mixed declarations) - not a preset question.
// This is the same fold_rtx / SCALAR_ADDR wall that holds func_80077884 above, misc3
// FieldFree and virtual_machine func_800A24C4.
// STILL SHAPED WRONG once the address is solved: retail indexes the sprite buffer and
// the animation-ids array with a shared i*4 (sll s1,s3,2 then addu, s1 recomputed every
// iteration) where this body strength-reduces both into walking pointers, which is why
// it needs a ninth saved register (sw s8,80(sp); frame 0x58 matches retail, saved set
// s0-s8 against retail's s0-s7); and retail's vector zero-fill walks a pointer with
// stores at 0/2/4 where this body's giv ends up at -2(s2)/0(s2). Both are downstream of
// the address wall, so fix that first.
/*
void func_80077AB4(void) {
    s32* pCount;
    SVECTOR* pVector;
    s32* pData;
    u8* pScratch;
    s32 i;
    s32 clut;
    int texOffset;

    if (g_FieldActorModelSlotCount == 0) {
        return;
    }
    pCount = &g_FieldActorModelSlotCount;

    FieldWaitForArchiveSync();
    FieldRenderSyncAndFlush();
    func_801E738C(g_FieldGearObjectCapacity);
    D_801E8644 = pCount - 10;
    SetBackColor(g_FieldBackgroundR, g_FieldBackgroundG, g_FieldBackgroundB);
    if (*pCount > 0) {
        pScratch = (u8*)pCount - 0x78;
        pVector = (SVECTOR*)pScratch;
        clut = 0xFC0000;
        texOffset = 0xFC;
        pData = g_FieldActorModelSlotDataBuffers;
        for (i = 0; i < ((s32*)(pScratch + 0x78))[0]; i++) {
            pVector->vx = 0;
            pVector->vy = 0;
            pVector->vz = 0;
            func_801E742C(i, 0, *pData++, g_FieldActorModelSlotSpriteBuffers[i],
                (s16)(0x240 - ((i + ((u8*)&g_FieldEffects)[0xEB + texOffset]) * 0x40)), 0x100, 0,
                clut >> 16, pVector++);
            texOffset++;
            HeapFree((void*)g_FieldActorModelSlotSpriteBuffers[i]);
            ((s32*)(pScratch + 0x20))[i] = *(s16*)((u8*)g_FieldActorModelPointers[i] + 0x1C);
            clut += 0x10000;
        }
    }
    HeapChangeCurrentUser(8, NULL);
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
/*
s32 func_80077E10(void) {
    int new_var;
    new_var = (D_800ADBD0 != 1) || (g_Unknown_800B2344 != 0);
    if (D_800ADBD0 != 1 || g_Unknown_800B2344 != 0) {
        return 0;
    }
    return -((FIELD_ACTORS[g_PlayerActorIndex].pActorData->scriptFlags.flags & 0x800) != 0);
}
*/
INCLUDE_ASM("asm/field/nonmatchings/main/main", func_80077E10);

INCLUDE_ASM("asm/field/nonmatchings/main/main", FieldMain);
