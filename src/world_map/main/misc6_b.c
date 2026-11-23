#include "common.h"

typedef struct {
    /* 0x00 */ undefined32 unk00;
    /* 0x04 */ s16 command;
    /* 0x06 */ u16 unk06;
    /* 0x08 */ undefined32 unk08[6]; // 0x08..0x1F
    /* 0x20 */ s16 state;
    /* 0x22 */ u16 stateTimer;
    /* 0x24 */ undefined32 unk24;
    /* 0x28 */ s32 x;
    /* 0x2C */ s32 y;
    /* 0x30 */ s32 z;
    /* 0x34 */ undefined32 unk34;
    /* 0x38 */ s32 dirX;
    /* 0x3C */ s32 dirY;
    /* 0x40 */ s32 dirZ;
    /* 0x44 */ undefined32 unk44;
    /* 0x48 */ undefined32 unk48;
    /* 0x4C */ undefined32 unk4C;
    /* 0x50 */ int stateIndex; // into the per-state tables; other map modes reuse these four words
    /* 0x54 */ u16 *pStateIds;
    /* 0x58 */ u16 *pStateFrames;
    /* 0x5C */ s32 scale;
    /* 0x60 */ s32 unk60[7]; // 0x60..0x7B
    /* 0x7C */ s32 unk7C;
} WorldMapActor; // Size: 0x80

typedef struct {
    /* 0x00 */ u8 unk0[0x14];
    /* 0x14 */ u16 sedId;
} SoundSedsBank; // Size: unknown, only +0x14 is read

extern WorldMapActor *g_WorldMapSlots;
extern u16 g_WorldMapScriptStateIds[];
extern u16 g_WorldMapScriptStateFrames[];
extern SoundSedsBank *g_MenuSedsFile;
extern s32 g_WorldMapTpageAbr;
extern s32 g_WorldMapDrawMode;
extern s32 g_WorldMapLoopActive;
extern s32 g_WorldMapAreaMode;
extern void WorldMapStartSlot(s32, s32);
extern void SoundPlaySoundEffectOnFreePair(s32);

/* This unit exists to hold this function's 65-word jump table: the table must sit
   at overlay 0x6F0, and misc6's .rodata starts at 0x29C, so there it would land at
   object offset 0x454 (4 mod 8), which cc1's hardcoded `.align 3` cannot emit.
   misc6_b's .rodata starts at 0x6F0 (8-aligned), so the table is at offset 0. The
   explicit `case 0:` is what keeps the table's slot 0: without it gcc folds the
   index to `state - 1` and emits 64 entries. */
int func_80080370(int index) {
    WorldMapActor *pActor = &g_WorldMapSlots[index];

    switch (pActor->state) {
    case 1:
        pActor->stateTimer = pActor->stateTimer - 1;
        if ((s16)pActor->stateTimer < 0) {
            pActor->state = g_WorldMapScriptStateIds[pActor->stateIndex];
            pActor->stateTimer = g_WorldMapScriptStateFrames[pActor->stateIndex];
            pActor->stateIndex = pActor->stateIndex + 1;
        }
        break;
    case 2:
        WorldMapStartSlot(3, 1);
        pActor->state = 1;
        break;
    case 3:
        WorldMapStartSlot(2, 2);
        pActor->state = 1;
        break;
    case 4:
        WorldMapStartSlot(0, 0xD);
        g_WorldMapTpageAbr = 1;
        g_WorldMapDrawMode = 0x80;
        pActor->state = 1;
        break;
    case 5:
        WorldMapStartSlot(0, 0xC);
        WorldMapStartSlot(4, 1);
        g_WorldMapTpageAbr = 1;
        g_WorldMapDrawMode = 0x80;
        pActor->state = 1;
        break;
    case 6:
        WorldMapStartSlot(2, 3);
        pActor->state = 1;
        break;
    case 7:
        WorldMapStartSlot(0, 0xD);
        g_WorldMapTpageAbr = 2;
        g_WorldMapDrawMode = 4;
        pActor->state = 1;
        break;
    case 8:
        SoundPlaySoundEffectOnFreePair((g_MenuSedsFile->sedId << 16) | 0x16);
        SoundPlaySoundEffectOnFreePair((g_MenuSedsFile->sedId << 16) | 0x17);
        SoundPlaySoundEffectOnFreePair((g_MenuSedsFile->sedId << 16) | 0x18);
        pActor->state = 1;
        break;
    case 0x40:
        g_WorldMapLoopActive = 0;
        g_WorldMapAreaMode = 0;
        pActor->state = 0;
        break;
    case 0:
    default:
        break;
    }
    return 1;
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc6_b", func_80080578);
/* 94.71% (136 bytes = retail's 34 insns), round 311. SOLVED: the
   constant-before-store-group question. Retail's first `li v0,1` sits six
   instructions above the `sh v0,4(v1)` that reads it. The source shape that
   reproduces it is the command store as the LAST statement INSIDE the
   do-block, not after the block. Measured on a faithful probe replica of this
   function (tools/scripts/cc1_probe.sh with the same externs and struct):
   store after the block -> the `li` is emitted adjacent to its `sh` (the old
   92.21 body); store as the block's last statement -> the `li` is hoisted to
   the top of the six-store group and the `sh` follows them, retail's shape.
   So the constant was never what needed keeping alive - the STORE's position
   was.
   Remaining deltas: (1) the register rotation - retail keeps pActor in v1 with
   the position values in a0/a1/a2, ours takes a2 for pActor and v1/a0/a1 for
   the values; (2) reorg's delay-slot choice at the end: retail ends
   `sh v0,4(v1)` / `li v0,1` / `jr ra` / `sh zero,0x20(v1)`, ours keeps the two
   halfword stores adjacent and steals the `li`. Its twin func_8007E450 above
   gets retail's tail for free because its two trailing zero-stores (state,
   stateIndex) leave a store next to the jr, which this body does not have.
   A 4-minute permuter run from the 94.71 body only proposed moving the command
   store back out of the block (i.e. back to 92.21), so it is exhausted here.
   Round 333 re-measured this body at 94.71 (136 bytes) and added: the plain
   `posX = *(u32 *)g_WorldMapPlayerPosition;` load (no posZ alias) is also
   94.71 here, and moving the tail `pActor->state = 0;` inside the do-block
   changes nothing. Putting the `return 1;` INSIDE the block with it (so
   loop.c could hoist the return constant into the preheader the way it hoists
   the command's `li v0,1`) is 86.47 and DROPS the command store - do not retry.
   ROUND 397: the register rotation is the same unfixable artifact as the twin
   func_8007E450 above, and it is now proven rather than guessed. The -da dump of
   this unit shows pActor's pseudo is tied to the `lw g_WorldMapSlots` result by
   the two-address `addu` (addsi3 ties operand 0 to operand 1), so the one real
   decision is which register that load result gets: local-alloc's find_free_reg
   hands it `$a2` where retail has `$v1` (it only skips `$a0` because the index
   parameter still holds it at the load's birth). -G0 leaves it at 94.71, 2.6.0 is
   48.62 and 2.7.2-cdk 92.03 on the twin, so the preset is not the lever either.
   Round 397 also measured `nResult` (an `int nResult = 1;` before the state store,
   `return nResult;`) to try to keep the return's `li v0,1` ahead of `sh zero,0x20`
   the way retail has it: gcc folds it and the tail is unchanged at 94.71, because
   our RTL ends `sh ... / sh zero / li v0,1 / jr` so reorg takes the li into the
   delay slot. Re-measured 94.71 / 136 bytes after the -da and -G0 screens.
   ROUND 420 re-measured this body live at 94.71 / 136 bytes: the two deltas are
   still (1) the twin func_8007E450's register rotation (pActor $a2 here, $v1 in
   retail; posZ $v1 here, $a2 in retail) whose -dl dispositions and quantity
   analysis are in that twin's note above, and (2) the tail reorg - retail emits
   `sh v0,4(v1)` / `li v0,1` / `jr ra` / `sh zero,0x20(v1)` while this body emits
   `sh v0,4` / `sh zero,0x20` / `jr ra` / `li v0,1`, i.e. retail's return
   constant sits before the state store in its RTL and ours after it, so reorg
   steals a different instruction into the delay slot. The twin gets retail's
   tail for free because its `pStateFrames = 0x40` store forces a v0 reload
   between the command store and the return.
*/
/*
int func_80080578(int index) {
    WorldMapActor *pActor = &g_WorldMapSlots[index];
    u32 posX;
    u32 posY;
    u32 posZ;

    g_WorldMapRotationMatrixValid = 0;
    pActor->unk7C = 0x1000;
    do {
        posZ = *(u32 *)g_WorldMapPlayerPosition;
        posX = posZ;
        posY = g_WorldMapPlayerPositionY;
        posZ = g_WorldMapPlayerPositionZ;
        g_WorldMapFadeLevel = 0x78;
        g_WorldMapCameraTarget[0] = posX;
        g_WorldMapScrollTargetX = posX;
        g_WorldMapCameraTargetY = posY;
        g_WorldMapScrollTargetY = posY;
        g_WorldMapCameraTargetZ = posZ;
        g_WorldMapScrollTargetZ = posZ;
        pActor->command = 1;
    } while (0);
    pActor->state = 0;
    return 1;
}
*/

/* 99.69%, round 325 (was 96.01%). objdiff reports 768 bytes on both sides, so the
   body below is exactly retail's size, and the WHOLE function now matches byte
   for byte except ten instructions of the rand tail, where only the register
   NAMES differ (identical instruction multiset).
   Round 325 took 96.01 -> 99.69 with four finds, all in the body:
   - case 1 of the first switch: the angle base landed in $v1 (retail: $a0),
     which dragged the 0x800000 constant into $v0 and scrambled the rest of the
     case. What fixes it is that `pAngle = g_WorldMapCameraRot; pActor->state = 1;`
     is written TWICE, once per arm of an if/else on g_WorldMapViewRotationY:
     98.65 -> 99.69, and 0x800000 then takes $v1 as retail has it. It reads like
     debris and is load-bearing - the same family as the repo's empty do-while
     and unreachable-use levers. Without it case 1 is 98.65.
   - the case-1 constants land in retail's positions only with TWO SEQUENTIAL
     do-blocks, not one: block 1 = command, camRot; block 2 = ViewY, ViewZ,
     stateIndex. A do-block's preheader is where cc1's loop.c hoists one loop
     invariant constant per register, so ONE block puts `li v0,16` and
     `lui v1,0x80` both at its head, while two put `li v0,16` before the command
     store and `lui v1,0x80` after the camRot store - retail's exact placement.
     `stateIndex = 0x800000` must be INSIDE block 2: its `lui` is hoisted above
     the two volatile stores while its `sw` stays last.
   - the pStateFrames update wants the EMBEDDED assignment, like its unk60 twin:
     nValue = (s32)(pActor->pStateFrames = (void *)((s32)pActor->pStateFrames + (nDelta >> 4)));
     Split as `nValue = ...; pActor->pStateFrames = nValue;` the sum takes a third
     register, the sra runs before the sw, and it parks at 97.10.
   - the two `pAngle[0] << 12` / `g_WorldMapViewRotationY << 12` shifts want
     SEPARATE locals (nAngle); sharing `nValue` ties them to one register and the
     second sll lands in $a3 instead of retail's $v0.
   Still needed from earlier rounds, do not undo: the pStateFrames/pStateIds
   stores stay volatile (88.41 without), the three case-1 stores stay volatile
   (86.95 without the do-block), g_WorldMapCameraRot is declared as an unbounded
   ARRAY so cc1 keeps its address in a register (round 316), `pScratch` is
   declared before `pActor` so its save lands in retail's prologue slot, and the
   second switch needs its `case 0: break;` (four nodes make retail's chain).
   Residual: the rand tail. Retail assigns nValue=$a1, the D_8009BD42 base=$a3,
   the reloaded scratchpad value=$a2; we get $a3/$a2/$a1 - a cyclic rotation of
   the same three registers over an identical instruction stream, i.e. global
   allocation's tie-break, which it decides from the whole function's allocno
   order. Measured dead in round 325: a pointer local for D_8009BD42 (declared at
   the top or at the use), `D_8009BD42[0] = D_8009BD42[0] + nValue`, base-first
   vs index-first scratch addressing, a separate local for the >>13 shift (764
   bytes, one instruction short), a separate variable for the scratch reload (760
   bytes), a do-block around the tail (98.39, rotates the other way), a do-block
   around only the three stores (96.38), `if (1) { }` (no change), and two local
   declaration orders (no change).
   Two permuter runs from this body (2011 then 2388 iterations) got base=60 and
   found nothing further; the earlier run's only find was the if/else above.
   gcc 2.7.2-cdk is NOT this unit's preset: this body scores 90.11 under it and
   the unit's matched neighbours fall to 96.27-99.67.
   ROUND 398 re-measured 99.69 (768 bytes = retail's) and now PROVES the tail
   is not a spelling question. The -da dump of this unit (mips-linux-gnu-cpp
   -DSKIP_ASM, then tools/gcc-2.7.2-psx/cc1 with the unit flags plus -da) prints
   "79 conflicts: 73 79 2 3 4 5 6 29 64 65 66" for the nValue pseudo, i.e. v0,
   v1, a0, a1 AND a2 are all marked conflicting, so find_reg has only a3 left and
   MUST put nValue there, where retail has a1. Retail's RTL therefore differs
   in a way no shape of this tail reaches, while the RTL we already have is
   retail's down to mfhi going straight into the value, sra a0,13 for the second
   shift and li v0,1 in between. Also measured dead in round 398: the ONE
   statement form `nValue = rand() % (unk7C >> 12) - (unk7C >> 13);` splits the
   value into two pseudos (mfhi v1, subu a3,v1,a0) and stays 99.69 with an extra
   move, so the two statement split in the body below is the shape retail has -
   keep it. -da recipe: it is also the cheap way to settle any rotation here.
   measure: tools/scripts/diff_func.sh world_map/main/misc6 func_80080600
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc6_b", func_80080600);
/*
int func_80080600(int index) {
    u8 *pScratch = (u8 *)PSX_SCRATCH;
    s16 *pAngle;
    WorldMapActor *pActor = &g_WorldMapSlots[index];
    s32 nValue;
    s32 nDelta;
    s32 nAngle;

    switch (pActor->command) {
    case 1:
        if (g_WorldMapViewRotationY) {
            pAngle = g_WorldMapCameraRot;
            pActor->state = 1;
        } else {
            pAngle = g_WorldMapCameraRot;
            pActor->state = 1;
        }
        do {
            pActor->command = 0;
            *(volatile s16 *)pAngle = 0x10;
        } while (0);
        do {
            *(volatile s16 *)&g_WorldMapViewRotationY = 0x8E0;
            *(volatile u16 *)&g_WorldMapViewRotationZ = 0;
            pActor->stateIndex = 0x800000;
        } while (0);
        nAngle = pAngle[0] << 12;
        *(volatile void **)&pActor->pStateFrames = (void *)nAngle;
        *(volatile void **)&pActor->pStateIds = (void *)nAngle;
        g_WorldMapCameraHeight = 0x800000;
        nAngle = g_WorldMapViewRotationY << 12;
        pActor->unk60[0] = nAngle;
        pActor->scale = nAngle;
        break;
    case 2:
        pActor->state = 2;
        pActor->command = 0;
        pActor->unk7C = 0x8000;
        break;
    case 3:
        pActor->state = 3;
        pActor->command = 0;
        pActor->unk7C = 0x80000;
        break;
    }
    if (g_WorldMapRotationMatrixValid == 0) {
        func_80096F18(&g_WorldMapPathFrame, g_WorldMapCameraTarget, g_WorldMapCameraHeight, &g_WorldMapCameraRot[0]);
    }
    switch (pActor->state) {
    case 1:
        nDelta = (pActor->stateIndex = pActor->stateIndex - 0x10000);
        if (nDelta <= 0x2FFFFF) {
            pActor->stateIndex = 0x300000;
        }
        nDelta = pActor->stateIndex - g_WorldMapCameraHeight;
        if (nDelta != 0) {
            g_WorldMapCameraHeight += nDelta >> 5;
        }
        nDelta = (s32)pActor->pStateIds - 0x4000;
        *(volatile void **)&pActor->pStateIds = (void *)nDelta;
        if (nDelta < -0x100000) {
            pActor->pStateIds = (void *)-0x100000;
        }
        nDelta = (s32)pActor->pStateIds - (s32)pActor->pStateFrames;
        if (nDelta != 0) {
            nValue = (s32)(pActor->pStateFrames = (void *)((s32)pActor->pStateFrames + (nDelta >> 4)));
            g_WorldMapCameraRot[0] = nValue >> 12;
        }
        nDelta = (pActor->scale = pActor->scale - 0x10000);
        if (nDelta <= 0x2DFFFF) {
            pActor->scale = 0x2E0000;
        }
        nDelta = pActor->scale - pActor->unk60[0];
        if (nDelta != 0) {
            nValue = (pActor->unk60[0] = pActor->unk60[0] + (nDelta >> 5));
            g_WorldMapViewRotationY = nValue >> 12;
        }
        break;
    case 0:
        break;
    case 2:
        break;
    case 3:
        pActor->unk7C -= 0x4000;
        if (pActor->unk7C < 0x1000) {
            pActor->unk7C = 0x1000;
            pActor->state = 0;
        }
        break;
    }
    nValue = rand() % (pActor->unk7C >> 12);
    nValue -= pActor->unk7C >> 13;
    *(s16 *)(0xA2 + pScratch) = (s16)nValue;
    D_8009BD42[0] += nValue;
    D_8009BD4A += *(u16 *)(0xA2 + pScratch);
    return 1;
}
*/
