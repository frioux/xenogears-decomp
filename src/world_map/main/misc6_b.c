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
extern u32 g_WorldMapRotationMatrixValid;
extern s32 g_WorldMapPlayerPosition;
extern u32 g_WorldMapPlayerPositionY;
extern u32 g_WorldMapPlayerPositionZ;
extern u32 g_WorldMapFadeLevel;
extern s32 g_WorldMapCameraTargetY;
extern s32 g_WorldMapCameraTargetZ;
extern u32 g_WorldMapScrollTargetX;
extern u32 g_WorldMapScrollTargetY;
extern u32 g_WorldMapScrollTargetZ;
extern s16 g_WorldMapCameraRot[];
extern s16 g_WorldMapViewRotationY;
extern u16 g_WorldMapViewRotationZ;
extern s32 g_WorldMapCameraHeight;
extern s32 g_WorldMapCameraTarget[];
extern u16 g_WorldMapPathFrame;
extern u16 D_8009BD42[];
extern u16 D_8009BD4A;
extern int rand(void);
extern void func_80096F18(void *, void *, s32, void *);

/* This unit exists to hold this function's 65-word jump table: the table must sit
   at overlay 0x6F0, and misc6's .rodata starts at 0x29C, so there it would land at
   object offset 0x454 (4 mod 8), which cc1's hardcoded `.align 3` cannot emit.
   misc6_b's .rodata starts at 0x6F0 (8-aligned), so the table is at offset 0. The
   explicit `case 0:` is what keeps the table's slot 0: without it gcc folds the
   index to `state - 1` and emits 64 entries. */
int WorldMapUpdateSlotActorScriptState(int index) {
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
   ROUND 4xx (this lane): the register rotation IS reachable, and it is a
   block-boundary effect, not an allocator coin flip. If pActor's whole life
   sits in ONE basic block - `pActor = &g_WorldMapSlots[index];` assigned
   INSIDE the do-block, or no do-block at all - local-alloc gives retail's
   exact set ($v1 pActor, $a0/$a1/$a2 for x/y/z). With pActor assigned before
   the do-block it crosses into global-alloc, which runs after the local pass,
   and lands in $a2 (this body). Measured with cc1_probe.sh on a faithful
   replica of the function.
   BUT every one of those forms loses the instruction ORDER this body has
   (retail's `li v0,1` hoisted six slots above its `sh v0,4`, the RMV store at
   slot 4), and objdiff scores a displaced insn block WORSE than a pure
   register rotation: pActor assigned inside the do-block = 84.71 (the RMV
   store then hoists to slot 1, and no do-block form reorders the command
   store), and no-block + volatile stores = 88.24 (only 4 differing lines -
   retail's `sh v0,4` / `li v0,1` sit at slots 14/15 in ours and 19/20 in
   retail, everything else exact, including retail's $v1/$a0/$a1/$a2 and its
   jr delay slot). So 94.71 stays the best score.
   What remains is one body with BOTH, and the two requirements conflict:
   retail's order needs pActor to cross a block boundary, retail's registers
   need it not to. The RMV store position is the coupling - it belongs inside
   the block for retail's order and hoists to slot 1 whenever pActor is
   assigned inside the block.
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

/* The rand tail needs its own short-lived local (`nRand`, not the shared
   `nValue`): reusing the function-wide nValue gave a 3-register rotation
   that sat at 99.69% through three rounds and two permuter runs, while a
   fresh local makes the tail allocnos come out in retail's order ($a1
   value, $a3 D_8009BD42 base, $a2 scratch reload). */
int WorldMapUpdateCameraSlot(int index) {
    u8 *pScratch = (u8 *)PSX_SCRATCH;
    s16 *pAngle;
    WorldMapActor *pActor = &g_WorldMapSlots[index];
    s32 nValue;
    s32 nDelta;
    s32 nAngle;
    s32 nRand;

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
    nRand = rand() % (pActor->unk7C >> 12);
    nRand -= pActor->unk7C >> 13;
    *(s16 *)(0xA2 + pScratch) = (s16)nRand;
    D_8009BD42[0] += nRand;
    D_8009BD4A += *(u16 *)(0xA2 + pScratch);
    return 1;
}
