#include "common.h"
#include "psyq/libgpu.h"
#include "libgear/main.h"
#include "battle/actor.h"
#include "system/controller.h"

// The status and slot mask tables hold one entry per bit of a u16 mask word.
#define BATTLE_MASK_BIT_COUNT 16
// The g_BattlePrimPool records the pause menu counts down, one timer each.
#define BATTLE_PRIM_POOL_SLOT_COUNT 16
// The debug pages g_DebugPageIndex cycles through.
#define BATTLE_DEBUG_PAGE_COUNT 5
// The status flag bits that get a column sprite: BattleBuildFlaggedSlotSpriteColumn
// builds one sprite per set bit among the lowest BATTLE_STATUS_ICON_COUNT of them.
#define BATTLE_STATUS_ICON_COUNT 5
// The per-slot effect counter records BattleUpdatePauseAndEffectTimers steps: one
// D_800D32F8 slot pointer and one pair of counter entries each (stride 8 in the tables).
#define BATTLE_EFFECT_SLOT_COUNT 3
// Digit sprite ids are indices into the value glyph set that starts at this id.
#define BATTLE_VALUE_DIGIT_SPRITE_BASE 0x92
// Status icon sprite ids are indices into the icon set that starts at this id.
#define BATTLE_STATUS_ICON_SPRITE_BASE 0xC4
// The sounds the pause menu plays for a cursor move, a confirmed key and a cancelled one.
#define BATTLE_SOUND_MENU_MOVE 0x4C
#define BATTLE_SOUND_MENU_DECIDE 0x4D
#define BATTLE_SOUND_MENU_CANCEL 0x4E
// The g_BattleMenuEvent id that says the menu is gone, not a key that was pressed.
#define BATTLE_MENU_EVENT_EXIT 0xFF
// The source word offset BattleUploadImageFrame wraps at: the end of the sub-image source.
#define BATTLE_UPLOAD_SOURCE_WORD_LIMIT 199

// Partial view of the per-frame battle work block (g_BattleDisplayState). The shared
// include/battle/battle_work.h owns the type other units read; this view names
// only the sub-image upload state this unit advances each frame.
typedef struct {
    /* 0x000 */ u8 unk0[0x30];
    /* 0x030 */ u32 sourceWordOffset; // word offset into each sub-image source, 4 words per upload
    /* 0x034 */ u8 unk34[0x6C];
    /* 0x0A0 */ u8 effectTimersActive; // BattleUpdatePauseAndEffectTimers returns unless set
    /* 0x0A1 */ u8 unkA1[0x8];
    /* 0x0A9 */ u8 moveSteps; // position interpolation steps accumulated, 6 per frame
    /* 0x0AA */ u8 frameCount; // incremented every update; the upload runs on odd values
    /* 0x0AB */ u8 sizeSteps; // size increment steps accumulated, 1 per frame
    /* 0x0AC */ u8 unkAC;
    /* 0x0AD */ u8 actorDrawActive; // func_80088B80 returns unless set
    /* 0x0AE */ u8 unkAE[0x1C];
    /* 0x0CA */ u8 slotTimersActive; // set to tick the 0x10 prim pool timers down
    /* 0x0CB */ u8 unkCB[0x4];
    /* 0x0CF */ u8 battleMenuOpen;
} BattleUploadState; // Size: 0xD0 (lower bound: other units reach 0xFC)

// Partial view of the battle work block (g_BattleGraphicsWork): the sub-image descriptors
// LoadImage uploads and the scroll BattleUpdateImageUpload animates.
typedef struct {
    /* 0x0000 */ u8 unk0[0x6410];
    /* 0x6410 */ s32 scrollOffset; // sub-image scroll, steps by 4 between 0x7C and 0x81
    /* 0x6414 */ u8 scrollRowPrimStart; // prim index the scroll row's prim was built at
    /* 0x6415 */ u8 scrollActive; // while set, scrollOffset steps every update
    /* 0x6416 */ u8 scrollDir; // 0 counts scrollOffset up to 0x81, 1 counts down
    /* 0x6417 */ u8 unk6417[0x2539];
    /* 0x8950 */ RECT uploadRect[4];
} BattleUploadWork; // Size: 0x8970 (lower bound: other units reach 0xA2C8)

// The prim pool's head is a 0x38-stride table of per-slot records; only the two
// fields this unit touches are named (see BattlePrimPool for the pool tail).
typedef struct {
    /* 0x00 */ u8 unk0[0x26];
    /* 0x26 */ s16 timer; // counts down to 0 while timerActive
    /* 0x28 */ u8 timerActive;
    /* 0x29 */ u8 unk29[0xF];
} BattlePrimPoolSlot; // Size: 0x38

// Partial view of the per-slot battle actor reached through D_800D32F8: only the
// four animation-done flags at 0x15F8 are named here.
typedef struct {
    /* 0x0000 */ u8 unk0[0x15F8];
    /* 0x15F8 */ u8 unk15F8; // set while the slot has two counters to step, not one
    /* 0x15F9 */ u8 unk15F9; // cleared once both of the slot's counters are done
    /* 0x15FA */ u8 unk15FA; // first counter done
    /* 0x15FB */ u8 unk15FB; // second counter done
} BattleActorAnimFlags; // Size: 0x15FC (lower bound, the actor continues past it)

extern u16 g_BattleStatusBitMasks[];
extern u16 g_BattleSlotBitMasks[];
extern u8 g_BattleDisplayMode;
extern u8 D_800CCDC4[];
extern u8 D_800CCDC6[];
extern u8 g_BattleValueDigitSpriteIds[];
extern s32 g_BattleLerpStepsPerFrame;
extern u8 g_BattleLerpDirX;
extern s32 g_BattleLerpStepX;
extern u8 g_BattleLerpDirY;
extern s32 g_BattleLerpStepY;
extern s32 g_BattleLerpFracX;
extern s32 g_BattleLerpFracY;
extern s32 g_BattleLerpStartX;
extern s32 g_BattleLerpTargetX;
extern s32 g_BattleLerpStartY;
extern s32 g_BattleLerpTargetY;
extern u8 g_BattleLerpDone;
extern BattleActorAnimFlags* D_800D32F8[];
extern u8 g_BattleStateWork[];
extern void func_801DF270(void);
extern void func_801DF4C0(void);
extern u8 D_800D2C36;
extern u8 D_800C3CFA[];
extern u8 D_800CDCB8[];
extern u8 D_800CDCBC[];
extern u8 D_800CDCD0[];
extern u8 D_800CDCD4[];
extern u8 g_BattleAlternateSpriteFlag;
extern u16 g_BattleStatusFlags;
extern u16 D_800D2C3A;
extern s32 D_800C2054[];
extern u8 GameRandomByteRange(s32 arg0, u8 arg1);
extern void BattleSetupLinearMove(s32 arg0, s32 arg1, s32 arg2, s32 arg3);
extern s32 BattleMaskHasStatusBit(s32 mask, u8 idx);
extern u8 g_BattlePrimStartIndex;
extern u8* g_BattleSpriteColumnWork;
extern s32 BattleDrawSpritePackage(s32 arg0, s32 arg1, s32 arg2, s32 arg3);
extern void BattleSetPrimGrey80BlendAdd(u8* p);
extern Temp8* g_BattleSoundBankMain;
extern void SoundPlaySoundEffectOnLastPair(s32 arg0);
extern void BattleDrawSlotSpriteColumns(void);
extern void BattleBuildFlaggedSlotSpriteColumn(void);
extern void BattleBuildBlendedSprite(void);
extern void BattleBuildGrey80BlendSpriteColumnPair(void);
extern void BattleBuildGrey80BlendSpriteColumn5(void);
extern void BattleBuildGrey80BlendSpriteColumn6(void);
extern void BattleBuildGrey80BlendSpriteColumn7(void);
extern void BattleBuildGrey80BlendSpriteColumn2(void);
extern void BattleUpdatePauseMenu(u8 arg0);
extern void BattleUpdateImageUpload(u8 arg0);
extern void BattleUpdatePauseAndEffectTimers(u8 arg0);
extern int rand(void);
extern BattleUploadWork *g_BattleGraphicsWork;
extern BattleUploadState *g_BattleDisplayState;
extern u8 g_BattlePauseActive;
extern BattlePrimPoolSlot *g_BattlePrimPool;
extern u8 D_800C48EA;
extern u8 g_BattleMenuEvent;
extern s32 *g_DebuggerState;
extern u16 g_C1ButtonStatePressedOnce;
extern u16 g_C1ButtonStateReleased;
extern int g_SpuChannelMask;
extern int ControllerGetType(int controllerIndex);
extern int ControllerIsStateStackFull(void);
extern void ControllerResetState(void);
extern int ControllerPopState(void);
extern void GraphicsDrawPauseLetters(int x, int y);
extern void SoundMuteAllSpuChannels(void);
extern void SoundEnableAllSpuChannels(void);
// The two bytes of the menu's event history: `last` is the event the menu had last
// frame (the shared header's g_BattleLastMenuEvent), `previous` the event before it.
typedef struct {
    /* 0x0 */ u8 previous;
    /* 0x1 */ u8 last;
} BattleMenuEventHistory; // Size: 0x2

// Partial view of the command queue header (BattleCommandQueue in src/battle/main/misc4.c):
// only the byte that says a queued command has run is named here.
typedef struct {
    /* 0x000 */ u8 unk0[0x2DB];
    /* 0x2DB */ u8 commandFinished;
} BattleCommandQueueTail; // Size: 0x2DC

extern BattleCommandQueueTail* g_BattleCommandQueue;
extern BattleTableEntry g_BattleWorkBlock[];
extern u8 g_BattleSlotDisplayValue;
extern BattleMenuEventHistory g_BattleMenuEventHistory;
extern u8 g_DebugPageIndex;
extern u8 g_BattleDebugFontLoaded;
extern u8 D_800D32A8;
extern u8 D_800D32B0;
extern void BattlePlaySoundIfEnabled(u8 arg0);
extern void FontSetExternalBuffer(void* pFont);
extern void* FontLoadFont(s32 startX, s32 startY, s32 width, s32 height, s32 maxLetters, u32 flags, s32 texpageX, s32 texpageY, s32 clutX, s32 clutY, void* pCompressedFontFile);
extern u8 D_800C3CF8[];
extern u8 D_800C3CFB[];
extern u8 g_BattleSceneRunning;
extern u8 g_BattleCommandCharIndex;
extern u8 D_800D2C35;
extern u8 g_BattleCursorScrollState;
extern s32 g_BattleCursorScrollY;
extern u16 D_800D2C2A;
extern void BattleTickSlotCountdowns(void);
extern void BattlePollMenuEvent(s32 arg0);
extern void BattleParseNumberToDigits(s32 arg0);
extern void BattleSetPrimGrey80BlendSub(u8 *p);

void BattleStepTwoAxisInterpolation(void) {
    s32 i = 0;

    for (i = 0; i < g_BattleLerpStepsPerFrame; i++) {
        if (g_BattleLerpDirX != 0) {
            g_BattleLerpFracX -= g_BattleLerpStepX;
        } else {
            g_BattleLerpFracX += g_BattleLerpStepX;
        }
        if (g_BattleLerpDirY != 0) {
            g_BattleLerpFracY -= g_BattleLerpStepY;
        } else {
            g_BattleLerpFracY += g_BattleLerpStepY;
        }
    }
    if (g_BattleLerpStepX == 0x100) {
        if (g_BattleLerpDirX != 0) {
            if (g_BattleLerpFracX / 0x100 + g_BattleLerpStartX < g_BattleLerpTargetX) {
                g_BattleLerpDone = 1;
            }
        } else if (g_BattleLerpTargetX < g_BattleLerpFracX / 0x100 + g_BattleLerpStartX) {
            g_BattleLerpDone = 1;
        }
    } else if (g_BattleLerpDirY != 0) {
        if (g_BattleLerpFracY / 0x100 + g_BattleLerpStartY < g_BattleLerpTargetY) {
            g_BattleLerpDone = 1;
        }
    } else if (g_BattleLerpTargetY < g_BattleLerpFracY / 0x100 + g_BattleLerpStartY) {
        g_BattleLerpDone = 1;
    }
}
// Round-500 history (superseded by the note below), 95.80% at 1224 bytes / 306 insns, up from 92.61: (A) all seven prim-blend and grid call sites need the POINTER-ADDITION spelling (pass+0x... with the constant on the index chain, e.g. BattleSetPrimGrey80BlendAdd(g_BattleSpriteColumnWork + ((i * 2 + g_BattleSpriteColumnWork[0x5D94]) * 0x28 + 0xD20))) -- an embedded-assignment temp instead costs an extra register move per site (86.88) and a plain statement temp is the old 88.68; that class is closed. (B) BattleDrawSpritePackage is declared here with s32 coordinates, not s16: with s16 cc1 canonicalises the grid's 'sy >> 16' as an s16-in-high-half value, loop.c then HOISTS it out of the inner 10-iteration loop as 'sra v0,s5,0x10 / sll s6,v0,0x10' plus a per-use 'sra a3,s6,0x10', which is the extra s6 save/restore and the +8-byte frame (94.20 / 310); with s32 there is no conversion and retail's per-iteration 'sra a3,s5,0x10' falls out (95.80 / 306). The two s16 truncations that retail DOES have (BattleBuildGrey80BlendSpriteColumn6/7, third argument '0x11A + n * 6') are written as explicit (s16) casts and both stay 100.00; every other caller in the unit was re-measured 100.00 under the s32 prototype. Residual: +4 insns -- (1) the lerp block emits the 0x5D9C store before loading g_BattleLerpFracY, so it pays two extra nops and loses retail's [lw FracY][bgez][sh 0x5D9C in the delay slot] schedule (+2), tried: two statement orderings, a work pointer local (rotates the base to a0), (2) retail keeps the ADDRESS of g_BattlePrimStartIndex in s0 across the three straight-line 0x5D94/93/95 stores (lui+addiu once, three 'lbu 0,0(s0)') where this body re-forms lui/lbu at each (+1), (3) one insn still unaccounted. Also cosmetic: the grid preheader emits lui s5/move s1/move s4/lui s3/move s2 where retail has move s1/lui s5/move s4/move s2/lui s3. Preset checked earlier: cdk drops matched neighbours (98.75 / 79.93), so 2.6.0 is right.
// Best attempt: objdiff 96.37%, retail's EXACT 1208 bytes / 302 insns (round 500 was 95.80 / 1224 / 306). Round 501 recovered all four surplus instructions, two findings: (A) the lerp block must read BOTH fractions into locals before the two stores -- fracX = g_BattleLerpFracX; fracY = g_BattleLerpFracY; then the 0x5D9C/0x5D9E stores -- which drops the two nops and gives retail's [lw FracY][bgez FracY][sh 0x5D9C in the delay slot][sra v1,a0,0x8] tail (+2 insns, old residual 1 closed). Computing the first value into a temp and storing it after the FracY read instead scores 96.52% but 1216 bytes / 304 insns -- a wrong-size candidate, rejected. (B) the three 0x5D94/0x5D93/0x5D95 stores share the ADDRESS of g_BattlePrimStartIndex if it is taken once: 'u8 *pPrimStartIndex = &g_BattlePrimStartIndex;' then '*pPrimStartIndex' at the three sites (+1 insn, old residual 2 closed). Assigning that pointer at the first use instead is folded by cc1 (*&x -> x) and goes back to lui/lbu per site (95.90 / 304). Residual left, size-exact and count-neutral: retail forms that address AFTER the first BattleDrawSpritePackage call in $s0, this body forms it in the entry block in $s2 ($s0 is live early with the D_800C2054 index), 3 diff lines; and the lerp block pays 'move a0,v0' in FracX's bgez delay slot where retail pays a nop, and retail puts 'sra v1,v1,0x8' before the StartX lhu where this body puts it after (count-neutral swap). Grid preheader order (lui s5/move s1 vs move s1/lui s5) is also cosmetic and still differs. Preset: cdk drops matched neighbours (98.75 / 79.93), so 2.6.0 is right.
// Best attempt: objdiff 96.37%, retail's EXACT 1208 bytes / 302 insns; 42 differing lines, of which 21 are branch targets that move only because of residual (1). The rest of the body is instruction-identical. Round 520 re-measured all three residuals and closed the axes below.
// (1) THE SHARED ADDRESS OF g_BattlePrimStartIndex (the 0x5D94/0x5D93/0x5D95 stores): retail forms lui $s0/addiu $s0 AFTER the first BattleDrawSpritePackage returns and then does three lbu 0($s0); this body forms it in the ENTRY block in $s2. A function-TOP declaration initializer is the only spelling whose pointer survives copy propagation in 2.6.0 (that survival is what shares the address at all) and it is emitted where it is declared; assigning it at the first use, initializing it in a nested block that begins at the first use, and initializing it in a block that begins before the two BattleDrawSpritePackage calls all FOLD (deref of address-of becomes the global) and fall back to per-site lui v0/lbu v0 = 1216 bytes / 304 insns / 95.90%. So definition placement and address sharing are mutually exclusive here, and the shorter live range is what hands retail $s0.
// (2) LERP BLOCK: reading BOTH fractions into locals before the two 0x5D9C/0x5D9E stores is the only 302-insn spelling, but it pays 'move a0,v0' in the FracX bgez delay slot where retail has a nop, loads FracY right after FracX where retail loads it after StartX's lhu, and puts 'sra v1,v1,8' after that lhu where retail puts it before. Writing the fractions inline in each store (which interleaves like retail) and reading fracY between the two stores are both 1216 bytes / 304 insns (two unfilled nops).
// (3) GRID PREHEADER order: this body emits lui $s5, move $s1, move $s4, lui $s3, move $s2 where retail emits move $s1, lui $s5, move $s4, move $s2, lui $s3 - two independent-instruction swaps from sched2; sy's initializer precedes the for in every spelling, so source order cannot reach it.
// Permuter, two 4-minute runs (-j 2): base=1379 both times, best=940 then best=1087 and neither is portable. Run 1's only edit was 'new_var = fracX / 0x100;' - the named-division temp already measured at 1216 bytes / 304 insns / 96.52, a wrong-size candidate, rejected. Run 2's was deleting the pointer's initializer and assigning it just before the 0x5D97 store, i.e. the three 0x5D94/0x5D93/0x5D95 dereferences would read an uninitialised local - wrong code, rejected. Presets measured on this body: gcc-2.7.2 96.18, 2.6.0 -G0 96.37 (identical), 2.6.0 -O3 and cdk do not compile; cdk was already known to drop matched neighbours (98.75 / 79.93), so 2.6.0 is right.
/*
void func_80088B80(void) {
    s32 i;
    s32 j;
    s32 sx;
    s32 sy;
    s32 x;
    s32 y;
    s32 fracX;
    s32 fracY;
    u8 *pPrimStartIndex = &g_BattlePrimStartIndex;

    if (g_BattleDisplayState->actorDrawActive == 0) {
        return;
    }
    if (g_BattleLerpDone != 0) {
        if ((u8)GameRandomByteRange(0, 0x63) >= 0x60) {
            BattleSetupLinearMove(*(u16*)(g_BattleSpriteColumnWork + 0x5D9C), *(u16*)(g_BattleSpriteColumnWork + 0x5D9E),
                D_800C2054[(u8)GameRandomByteRange(0, 4)], D_800C2054[(u8)GameRandomByteRange(0, 4) + 5]);
        }
    }
    if (g_BattleLerpDone == 0) {
        BattleStepTwoAxisInterpolation();
        fracX = g_BattleLerpFracX;
        fracY = g_BattleLerpFracY;
        *(u16*)(g_BattleSpriteColumnWork + 0x5D9C) = *(u16*)&g_BattleLerpStartX + fracX / 0x100;
        *(u16*)(g_BattleSpriteColumnWork + 0x5D9E) = *(u16*)&g_BattleLerpStartY + fracY / 0x100;
    }
    g_BattleSpriteColumnWork[0x5D72] = BattleDrawSpritePackage(0xB9, (s32)(g_BattleSpriteColumnWork + 0xD20), *(s16*)(g_BattleSpriteColumnWork + 0x5D9C), *(s16*)(g_BattleSpriteColumnWork + 0x5D9E));
    g_BattleSpriteColumnWork[0x5D94] = *pPrimStartIndex;
    g_BattleSpriteColumnWork[0x5D71] = BattleDrawSpritePackage((*(u16*)(g_BattleSpriteColumnWork + 0x5D9C) & 0xF) + 0xA9, (s32)(g_BattleSpriteColumnWork + 0x3C0), 0xA0, 0x64);
    g_BattleSpriteColumnWork[0x5D93] = *pPrimStartIndex;
    g_BattleSpriteColumnWork[0x5D73] = BattleDrawSpritePackage(0x82, (s32)(g_BattleSpriteColumnWork + 0xFA0), *(s16*)(g_BattleSpriteColumnWork + 0x5D9C), *(s16*)(g_BattleSpriteColumnWork + 0x5D9E));
    g_BattleSpriteColumnWork[0x5D95] = *pPrimStartIndex;
    for (i = 0; i < g_BattleSpriteColumnWork[0x5D72]; i++) {
        BattleSetPrimGrey80BlendAdd(g_BattleSpriteColumnWork + ((i * 2 + g_BattleSpriteColumnWork[0x5D94]) * 0x28 + 0xD20));
    }
    for (i = 0; i < g_BattleSpriteColumnWork[0x5D73]; i++) {
        BattleSetPrimGrey80BlendAdd(g_BattleSpriteColumnWork + ((i * 2 + g_BattleSpriteColumnWork[0x5D95]) * 0x28 + 0xFA0));
    }
    for (i = 0; i < g_BattleSpriteColumnWork[0x5D71]; i++) {
        BattleSetPrimGrey80BlendAdd(g_BattleSpriteColumnWork + ((i * 2 + g_BattleSpriteColumnWork[0x5D93]) * 0x28 + 0x3C0));
    }
    g_BattleSpriteColumnWork[0x5D96] = BattleDrawSpritePackage((*(u16*)(g_BattleSpriteColumnWork + 0x5D9E) & 0xF) + 0xC9, (s32)(g_BattleSpriteColumnWork + 0x32A0), *(s16*)(g_BattleSpriteColumnWork + 0x5D9C), *(s16*)(g_BattleSpriteColumnWork + 0x5D9E));
    g_BattleSpriteColumnWork[0x5D97] = g_BattlePrimStartIndex;
    for (i = 0; i < g_BattleSpriteColumnWork[0x5D96]; i++) {
        BattleSetPrimGrey80BlendSub(g_BattleSpriteColumnWork + ((i * 2 + g_BattleSpriteColumnWork[0x5D97]) * 0x28 + 0x32A0));
    }
    if (g_BattleLerpDone == 0) {
        sy = 0xA << 16;
        for (i = 0; i < 2; i++) {
            sx = 0x82 << 16;
            for (j = 0; j < 0xA; j++) {
                x = sx >> 16;
                y = sy >> 16;
                BattleDrawSpritePackage(GameRandomByteRange(0, 9) + 0xBA, (s32)(g_BattleSpriteColumnWork + ((i * 0xA + j) * 0x50 + 0x46A0)), x, y);
                BattleSetPrimGrey80BlendAdd(g_BattleSpriteColumnWork + (((i * 0xA + j) * 2 + *(s32*)&g_BattlePrimStartIndex) * 0x28 + 0x46A0));
                sx += 0x60000;
            }
            sy += 0xBD << 16;
        }
        g_BattleSpriteColumnWork[0x5D98] = g_BattlePrimStartIndex;
    }
}*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc14", func_80088B80);




void BattleBuildFlaggedSlotSpriteColumn(void) {
    s32 i = 0;
    s32 pitch = 0x6E << 16;
    s32 off;

    g_BattleSpriteColumnWork[0x5DA1] = 0;
    for (i = 0; i < BATTLE_STATUS_ICON_COUNT; i++) {
        if ((u16)BattleMaskHasStatusBit(g_BattleStatusFlags, i) != 0) {
            off = g_BattleSpriteColumnWork[0x5DA1] * 0x50 + 0x4CE0;
            g_BattleSpriteColumnWork[0x5DA1] += BattleDrawSpritePackage(i + BATTLE_STATUS_ICON_SPRITE_BASE, (s32)(off + (s32)g_BattleSpriteColumnWork), 0xE0, pitch >> 16);
            pitch += 0xA0000;
        }
    }
    g_BattleSpriteColumnWork[0x5DA0] = g_BattlePrimStartIndex;
    *(u16*)(g_BattleSpriteColumnWork + 0x5DA2) = 0;
}
void BattleBuildBlendedSprite(void) {
    s32 arg0;
    s32 pEntry;
    s32 i;

    arg0 = 0xA0;
    if (g_BattleAlternateSpriteFlag != 0) {
        arg0 = 0xA1;
    }
    g_BattleSpriteColumnWork[0x5D75] = BattleDrawSpritePackage(arg0, (s32)(g_BattleSpriteColumnWork + 0x3AC0), 0xA0, 0x64);
    g_BattleSpriteColumnWork[0x5D84] = g_BattlePrimStartIndex;
    for (i = 0; i < g_BattleSpriteColumnWork[0x5D75]; i++) {
        pEntry = (i * 2 + g_BattleSpriteColumnWork[0x5D84]) * 0x28 + 0x3AC0;
        BattleSetPrimGrey80BlendAdd((u8*)(pEntry + (s32)g_BattleSpriteColumnWork));
    }
}
void BattleBuildGrey80BlendSpriteColumnPair(void) {
    u8 v;
    u8 slotA;
    u8 slotB;
    s32 i;

    v = g_BattleCommandCharIndex;
    slotA = v - 0x5D;
    slotB = v - 0x25;
    g_BattleSpriteColumnWork[0x5D77] = BattleDrawSpritePackage(slotA, (s32)(g_BattleSpriteColumnWork + 0x3E80), 0xA0, 0x64);
    g_BattleSpriteColumnWork[0x5D86] = g_BattlePrimStartIndex;
    g_BattleSpriteColumnWork[0x5D7F] = BattleDrawSpritePackage(slotB, (s32)(g_BattleSpriteColumnWork + 0x43D0), 0xA0, 0x64);
    g_BattleSpriteColumnWork[0x5D8E] = g_BattlePrimStartIndex;
    for (i = 0; i < g_BattleSpriteColumnWork[0x5D77]; i++) {
        BattleSetPrimGrey80BlendAdd(g_BattleSpriteColumnWork + ((i * 2 + g_BattleSpriteColumnWork[0x5D86]) * 0x28 + 0x3E80));
    }
    for (i = 0; i < g_BattleSpriteColumnWork[0x5D7F]; i++) {
        BattleSetPrimGrey80BlendSub(g_BattleSpriteColumnWork + ((i * 2 + g_BattleSpriteColumnWork[0x5D8E]) * 0x28 + 0x43D0));
    }
}
void BattleBuildGrey80BlendSpriteColumn5(void) {
    s32 i = 0;
    s32 pitch = 0x11A << 16;
    s32 off;
    s32 off2;

    BattleParseNumberToDigits(D_800D2C2A);
    while (i < 5) {
        s32 slot = D_800C3CF8[i++];
        if (slot != BATTLE_VALUE_DIGIT_NONE) {
            off = g_BattleSpriteColumnWork[0x5D78] * 0x50 + 0x4E70;
            g_BattleSpriteColumnWork[0x5D78] += BattleDrawSpritePackage(slot + BATTLE_VALUE_DIGIT_SPRITE_BASE, (s32)(off + (s32)g_BattleSpriteColumnWork), pitch >> 16, 0x46);
            pitch += 0x60000;
        }
    }
    g_BattleSpriteColumnWork[0x5D87] = g_BattlePrimStartIndex;
    for (i = 0; i < g_BattleSpriteColumnWork[0x5D78]; i++) {
        BattleSetPrimGrey80BlendAdd((u8*)((off2 = (i * 2 + g_BattleSpriteColumnWork[0x5D87]) * 0x28 + 0x4E70) + (s32)g_BattleSpriteColumnWork));
    }
}
void BattleBuildGrey80BlendSpriteColumn6(void) {
    s32 i;
    s32 n;
    s32 pitch;
    s32 columnBase;
    s32 off;
    s32 off2;

    columnBase = 0x4FB0;
    if (g_BattleCommandCharIndex < 4) {
        BattleParseNumberToDigits(D_800D2C3A);
        i = 0;
        n = 0;
        pitch = 0x11A << 16;
        while (i < 2) {
            s32 slot = D_800C3CFB[i++];
            if (slot != BATTLE_VALUE_DIGIT_NONE) {
                off = g_BattleSpriteColumnWork[0x5D79] * 0x50 + 0x4FB0;
                g_BattleSpriteColumnWork[0x5D79] += BattleDrawSpritePackage(slot + BATTLE_VALUE_DIGIT_SPRITE_BASE, (s32)(off + (s32)g_BattleSpriteColumnWork), pitch >> 16, 0x4E);
                pitch += 0x60000;
                n++;
            }
        }
        g_BattleSpriteColumnWork[0x5D79] += BattleDrawSpritePackage(0x9D, (s32)((g_BattleSpriteColumnWork[0x5D79] * 0x50 + columnBase) + (s32)g_BattleSpriteColumnWork), (s16)(0x11A + n * 6), 0x4E);
    } else {
        g_BattleSpriteColumnWork[0x5D79] = BattleDrawSpritePackage(0xA2, (s32)(g_BattleSpriteColumnWork + 0x4FB0), 0x11A, 0x4E);
    }
    g_BattleSpriteColumnWork[0x5D88] = g_BattlePrimStartIndex;
    for (i = 0; i < g_BattleSpriteColumnWork[0x5D79]; i++) {
        BattleSetPrimGrey80BlendAdd((u8*)((off2 = (i * 2 + g_BattleSpriteColumnWork[0x5D88]) * 0x28 + 0x4FB0) + (s32)g_BattleSpriteColumnWork));
    }
}
void BattleBuildGrey80BlendSpriteColumn7(void) {
    s32 i = 0;
    s32 columnBase;
    s32 n = 0;
    s32 pitch = 0x11A << 16;
    s32 off;
    s32 off2;

    columnBase = 0x50A0;
    BattleParseNumberToDigits(D_800D2C36);
    for (i = 0; i < 3; i++) {
        if (D_800C3CFA[i] != BATTLE_VALUE_DIGIT_NONE) {
            off = g_BattleSpriteColumnWork[0x5D7A] * 0x50 + 0x50A0;
            g_BattleSpriteColumnWork[0x5D7A] += BattleDrawSpritePackage(D_800C3CFA[i] + BATTLE_VALUE_DIGIT_SPRITE_BASE, (s32)(off + (s32)g_BattleSpriteColumnWork), pitch >> 16, 0x56);
            pitch += 0x60000;
            n++;
        }
    }
    g_BattleSpriteColumnWork[0x5D7A] += BattleDrawSpritePackage(0x9D, (s32)((g_BattleSpriteColumnWork[0x5D7A] * 0x50 + columnBase) + (s32)g_BattleSpriteColumnWork), (s16)(0x11A + n * 6), 0x56);
    g_BattleSpriteColumnWork[0x5D89] = g_BattlePrimStartIndex;
    for (i = 0; i < g_BattleSpriteColumnWork[0x5D7A]; i++) {
        off2 = (i * 2 + g_BattleSpriteColumnWork[0x5D89]) * 0x28 + 0x50A0;
        BattleSetPrimGrey80BlendAdd(&g_BattleSpriteColumnWork[off2]);
    }
}

void BattleBuildGrey80BlendSpriteColumn2(void) {
    s32 i = 0;
    s32 pitch = 0x11A << 16;
    s32 off;
    s32 off2;

    BattleParseNumberToDigits(D_800D2C35);
    while (i < 2) {
        s32 slot = D_800C3CFB[i++];
        if (slot != BATTLE_VALUE_DIGIT_NONE) {
            off = g_BattleSpriteColumnWork[0x5D7B] * 0x50 + 0x51E0;
            g_BattleSpriteColumnWork[0x5D7B] += BattleDrawSpritePackage(slot + BATTLE_VALUE_DIGIT_SPRITE_BASE, (s32)(off + (s32)g_BattleSpriteColumnWork), pitch >> 16, 0x5E);
            pitch += 0x60000;
        }
    }
    g_BattleSpriteColumnWork[0x5D8A] = g_BattlePrimStartIndex;
    for (i = 0; i < g_BattleSpriteColumnWork[0x5D7B]; i++) {
        BattleSetPrimGrey80BlendAdd((u8*)((off2 = (i * 2 + g_BattleSpriteColumnWork[0x5D8A]) * 0x28 + 0x51E0) + (s32)g_BattleSpriteColumnWork));
    }
}
// Best attempt: objdiff 97.23% at retail's exact 520 bytes / 130 insns. One residual class: at all three BattleDrawSpritePackage call sites the base sits in $a1, the count lbu in $v1 and the offset chain in $v0, where retail has base $v1 / count $v0 / chain $a1 -- a caller-saved rotation, instruction stream otherwise identical. Structural findings that landed: second loop's pitch += 0x80000 belongs INSIDE its if, loop 1's at the join. Round 500 re-measured 97.23 and added: the rotation IS allocator-order sensitive -- writing an explicit i = 0; before the 0x5D8B store (retail has move s0,zero there) rotates the two CALLEE-saved regs instead (i -> $s1, pitch -> $s0) and drops to 96.95, so retail's early i=0 is real but reaching it costs the s0/s1 pair; one 4-minute permuter run (1704 iterations) found nothing (base=360 best=360). Previously tried and byte-identical: off as u32, count local, base local, both operand orders of the arg sum, pWork local, inline offset, outer cast grouping, pitch increment inside/outside the if, for vs while, truncation probes (truncating after loop 1 allocates loop 1 exactly retail's way).
// Best attempt: objdiff 97.23% at retail's exact 520 bytes / 130 insns. One residual class: at all three BattleDrawSpritePackage call sites the base sits in $a1, the count lbu in $v1 and the offset chain in $v0, where retail has base $v1 / count $v0 / chain $a1 -- a caller-saved rotation, instruction stream otherwise identical. Round 500 re-measured 97.23: the rotation IS allocator-order sensitive (an explicit early i = 0; rotates the callee-saved pair instead, 96.95) and one 4-minute permuter run found nothing (1704 iters, base=360 best=360). Round 501: the pointer-arithmetic spelling of the arg, (s32)(g_BattleSpriteColumnWork + off), is byte-identical to the int sum (97.23, 520). Previously tried and byte-identical: off as u32, count local, base local, both operand orders of the arg sum, pWork local, inline offset, outer cast grouping, pitch increment inside/outside the if, for vs while, truncation probes.
/*
void func_800898F0(u8 arg0) {
    s32 i = 0;
    s32 pitch = 0x20 << 16;
    s32 off;

    g_BattleSpriteColumnWork[0x5D7C] = 0;
    BattleParseNumberToDigits(*(u16*)(D_800CCDC4 + arg0 * 0x170));
    while (i < 4) {
        s32 slot = g_BattleValueDigitSpriteIds[i++];
        if (slot != 0xFF) {
            off = g_BattleSpriteColumnWork[0x5D7C] * 0x50 + 0x5280;
            g_BattleSpriteColumnWork[0x5D7C] += BattleDrawSpritePackage(slot + 0x92, (s32)(off + (s32)g_BattleSpriteColumnWork), pitch >> 16, 0xCC);
        }
        pitch += 0x80000;
    }
    g_BattleSpriteColumnWork[0x5D8B] = g_BattlePrimStartIndex;
    g_BattleSpriteColumnWork[0x5D7D] = 0;
    BattleParseNumberToDigits(*(u16*)(D_800CCDC6 + arg0 * 0x170));
    pitch = 0x48 << 16;
    for (i = 0; i < 4; i++) {
        if (g_BattleValueDigitSpriteIds[i] != 0xFF) {
            off = g_BattleSpriteColumnWork[0x5D7D] * 0x50 + 0x53C0;
            g_BattleSpriteColumnWork[0x5D7D] += BattleDrawSpritePackage(g_BattleValueDigitSpriteIds[i] + 0x92, (s32)(off + (s32)g_BattleSpriteColumnWork), pitch >> 16, 0xCC);
            pitch += 0x80000;
        }
    }
    off = g_BattleSpriteColumnWork[0x5D7D] * 0x50 + 0x53C0;
    g_BattleSpriteColumnWork[0x5D7D] += BattleDrawSpritePackage(0x9C, (s32)(off + (s32)g_BattleSpriteColumnWork), 0x41, 0xCC);
    g_BattleSpriteColumnWork[0x5D8C] = g_BattlePrimStartIndex;
}*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc14", func_800898F0);




void BattleBuildSpritePrimLists(void) {
    BattleDrawSlotSpriteColumns();
    BattleBuildFlaggedSlotSpriteColumn();
    BattleBuildBlendedSprite();
    BattleBuildGrey80BlendSpriteColumnPair();
    BattleBuildGrey80BlendSpriteColumn5();
    BattleBuildGrey80BlendSpriteColumn6();
    BattleBuildGrey80BlendSpriteColumn7();
    BattleBuildGrey80BlendSpriteColumn2();
}
u16 BattleRandomRange(u16 arg0, u16 arg1) {
    s32 rem;
    s32 range;

    if (arg0 == 0xFFFF) {
        return 0xFFFF;
    }
    if (arg1 == 0) {
        return 0;
    }
    if (arg0 == arg1) {
        return arg0;
    }
    range = arg1 - arg0;
    if (range > 0xFFFE) {
        return rand();
    }
    rem = (u16)rand() % (range + 1);
    return (u16)(arg0 + rem);
}
u16 BattleStatusBitMask(u8 idx) {
    return g_BattleStatusBitMasks[idx];
}
u16 BattleSlotBitMask(u8 idx) {
    return g_BattleSlotBitMasks[idx];
}
u16 BattleStatusBitMaskInverted(u8 idx) {
    return ~g_BattleStatusBitMasks[idx];
}
u16 BattleSlotBitMaskInverted(u8 idx) {
    return ~g_BattleSlotBitMasks[idx];
}
s32 BattleMaskHasStatusBit(s32 mask, u8 idx) {
    s32 flags;

    if (idx < BATTLE_MASK_BIT_COUNT) {
        flags = g_BattleStatusBitMasks[idx] & mask;
    } else {
        flags = 0;
    }
    return flags;
}
s32 BattleMaskHasSlotBit(s32 mask, u8 idx) {
    s32 flags;

    if (idx < BATTLE_MASK_BIT_COUNT) {
        flags = g_BattleSlotBitMasks[idx] & mask;
    } else {
        flags = 0;
    }
    return flags;
}
void BattlePollMenuEvent(s32 arg0) {
    s32 channelMask;
    BattleMenuEventHistory* pEvent;
    u8* pMenuExit;
    u8* pSlotState;
    u8 event;
    u8 waiting;
    u8 muted;
    u8 prevEvent;

    event = 8;
    waiting = 1;
    muted = 0;
    do {
        if (ControllerGetType(0) == 0) {
            if (muted == 0) {
                GraphicsDrawPauseLetters(0x88, 0x64);
                GraphicsDrawPauseLetters(0x88, 0x144);
                SoundMuteAllSpuChannels();
                muted++;
                channelMask = g_SpuChannelMask;
            }
        } else {
            if (muted != 0) {
                SoundEnableAllSpuChannels();
                g_SpuChannelMask = channelMask;
            }
            waiting = 0;
        }
    } while (waiting != 0);

    pEvent = &g_BattleMenuEventHistory;
    pMenuExit = &D_800C48EA;
    pSlotState = &g_BattleWorkBlock[0].state.commandCounter;
    do {
        if (ControllerIsStateStackFull() != 0) {
            ControllerResetState();
        } else {
            while (ControllerPopState() != 0) {
                if (*pMenuExit != 0 || g_BattleCommandQueue->commandFinished != 0) {
                    event = BATTLE_MENU_EVENT_EXIT;
                    break;
                }
                if (g_BattlePauseActive != 0) {
                    if (*g_DebuggerState != -1) {
                        if ((g_C1ButtonStatePressedOnce & CTRL_BTN_L1) != 0) {
                            if ((g_C1ButtonStatePressedOnce & CTRL_BTN_R1) != 0) {
                                *pMenuExit = 1;
                                SoundEnableAllSpuChannels();
                                g_SpuChannelMask = channelMask;
                                g_BattlePauseActive = 0;
                                break;
                            }
                        }
                    }
                } else if ((g_C1ButtonStatePressedOnce & CTRL_BTN_RIGHT) != 0) {
                    BattlePlaySoundIfEnabled(BATTLE_SOUND_MENU_MOVE);
                    prevEvent = pEvent->last;
                    event = 0;
                    pEvent->last = event;
                    pEvent->previous = prevEvent;
                    break;
                } else if ((g_C1ButtonStatePressedOnce & CTRL_BTN_DOWN) != 0) {
                    BattlePlaySoundIfEnabled(BATTLE_SOUND_MENU_MOVE);
                    prevEvent = pEvent->last;
                    event = 1;
                    pEvent->last = event;
                    pEvent->previous = prevEvent;
                    break;
                } else if ((g_C1ButtonStatePressedOnce & CTRL_BTN_LEFT) != 0) {
                    BattlePlaySoundIfEnabled(BATTLE_SOUND_MENU_MOVE);
                    prevEvent = pEvent->last;
                    event = 2;
                    pEvent->last = event;
                    pEvent->previous = prevEvent;
                    break;
                } else if ((g_C1ButtonStatePressedOnce & CTRL_BTN_UP) != 0) {
                    BattlePlaySoundIfEnabled(BATTLE_SOUND_MENU_MOVE);
                    prevEvent = pEvent->last;
                    event = 3;
                    pEvent->last = event;
                    pEvent->previous = prevEvent;
                    break;
                } else if ((g_C1ButtonStateReleased & CTRL_BTN_CIRCLE) != 0) {
                    event = 4;
                    BattlePlaySoundIfEnabled(BATTLE_SOUND_MENU_DECIDE);
                    break;
                } else if ((g_C1ButtonStateReleased & CTRL_BTN_CROSS) != 0) {
                    event = 5;
                    BattlePlaySoundIfEnabled(BATTLE_SOUND_MENU_CANCEL);
                    break;
                } else if ((g_C1ButtonStateReleased & CTRL_BTN_SQUARE) != 0) {
                    event = 6;
                    BattlePlaySoundIfEnabled(BATTLE_SOUND_MENU_DECIDE);
                    break;
                } else if ((g_C1ButtonStateReleased & CTRL_BTN_TRIANGLE) != 0) {
                    event = 7;
                    BattlePlaySoundIfEnabled(BATTLE_SOUND_MENU_DECIDE);
                    break;
                } else if ((g_C1ButtonStateReleased & CTRL_BTN_L2) != 0) {
                    event = 0xC;
                    if (*g_DebuggerState != -1) {
                        g_BattleSlotDisplayValue = 0x1C;
                        D_800D32A8 = 0x1C;
                        D_800D32B0 = 0x1C;
                        /* the three per-slot records run 0x170 apart, the stride of
                           g_BattleWorkBlock's slots (BattleSetSlotFields148And162 in
                           src/battle/main/misc19.c writes these same two bytes) */
                        pSlotState[0] = 4;
                        pSlotState[0x170] = 4;
                        pSlotState[0x2E0] = 4;
                        pSlotState[0x1A] = 0xFF;
                        pSlotState[0x18A] = 0xFF;
                        pSlotState[0x2FA] = 0xFF;
                    }
                    break;
                } else if ((g_C1ButtonStateReleased & CTRL_BTN_R2) != 0) {
                    if (*g_DebuggerState != -1) {
                        if (++g_DebugPageIndex >= BATTLE_DEBUG_PAGE_COUNT) {
                            g_DebugPageIndex = 0;
                        }
                        event = 0xB;
                        if (g_BattleDebugFontLoaded == 0) {
                            FontSetExternalBuffer((void*)0x80200000);
                            FontLoadFont(0x10, 0x10, 0x140, 0x100, 0x3E8, 0, 0x340, 0, 0x340, 0x20, 0);
                            g_BattleDebugFontLoaded += 1;
                        }
                    }
                    break;
                } else if ((g_C1ButtonStateReleased & CTRL_BTN_SELECT) != 0) {
                    event = 0xD;
                    break;
                }
                if ((g_C1ButtonStateReleased & CTRL_BTN_START) == 0) {
                    continue;
                }
                event = 0xE;
                if (g_BattleSceneRunning != 0) {
                    if (g_BattlePauseActive == 0) {
                        GraphicsDrawPauseLetters(0x88, 0x64);
                        GraphicsDrawPauseLetters(0x88, 0x144);
                        SoundMuteAllSpuChannels();
                        channelMask = g_SpuChannelMask;
                        g_BattlePauseActive = 1;
                    } else {
                        SoundEnableAllSpuChannels();
                        g_SpuChannelMask = channelMask;
                        g_BattlePauseActive = 0;
                    }
                }
                break;
            }
        }
    } while (g_BattlePauseActive != 0);
    g_BattleMenuEvent = event;
}

void BattleUploadImageFrame(void) {
    /* declared out of call order: gcc's register tie for the LoadImage arguments depends on it */
    s32 vramOffset3;
    s32 vramOffset0;
    s32 vramOffset1;
    s32 vramOffset2;
    g_BattleDisplayState->frameCount += 1;
    if ((g_BattleDisplayState->frameCount & 1) == 0) {
        return;
    }
    vramOffset0 = 0x8970;
    LoadImage(&g_BattleGraphicsWork->uploadRect[0], (u_long*)(g_BattleDisplayState->sourceWordOffset * 4 + vramOffset0 + (s32)g_BattleGraphicsWork));
    vramOffset1 = 0x8FA0;
    LoadImage(&g_BattleGraphicsWork->uploadRect[1], (u_long*)(g_BattleDisplayState->sourceWordOffset * 4 + vramOffset1 + (s32)g_BattleGraphicsWork));
    vramOffset2 = 0x95D0;
    LoadImage(&g_BattleGraphicsWork->uploadRect[2], (u_long*)(g_BattleDisplayState->sourceWordOffset * 4 + vramOffset2 + (s32)g_BattleGraphicsWork));
    vramOffset3 = 0x9C00;
    LoadImage(&g_BattleGraphicsWork->uploadRect[3], (u_long*)(g_BattleDisplayState->sourceWordOffset * 4 + vramOffset3 + (s32)g_BattleGraphicsWork));
    g_BattleDisplayState->sourceWordOffset += 4;
    if (g_BattleDisplayState->sourceWordOffset >= BATTLE_UPLOAD_SOURCE_WORD_LIMIT) {
        g_BattleDisplayState->sourceWordOffset = 0;
    }
}
void BattleUpdateImageUpload(u8 arg0) {
    if (g_BattleSceneRunning != 0) {
        BattleTickSlotCountdowns();
    }
    if (arg0 == 0) {
        BattlePollMenuEvent(0);
    }
    g_BattleDisplayState->moveSteps += 6;
    g_BattleDisplayState->sizeSteps += 1;
    if (g_BattleGraphicsWork->scrollActive != 0) {
        if (g_BattleGraphicsWork->scrollDir == 0) {
            g_BattleGraphicsWork->scrollOffset += 4;
            if (g_BattleGraphicsWork->scrollOffset >= 0x81) {
                g_BattleGraphicsWork->scrollOffset = 0x7C;
                g_BattleGraphicsWork->scrollDir = 1;
            }
        } else {
            g_BattleGraphicsWork->scrollOffset -= 4;
            if (g_BattleGraphicsWork->scrollOffset < 0) {
                g_BattleGraphicsWork->scrollOffset = 4;
                g_BattleGraphicsWork->scrollDir = 0;
            }
        }
    }
    switch (g_BattleCursorScrollState) {
    case 1:
    case 3:
        g_BattleCursorScrollY += 1;
        break;
    case 2:
    case 4:
        g_BattleCursorScrollY -= 1;
        break;
    }
    BattleUploadImageFrame();
}

void BattleUpdatePauseMenu(u8 arg0) {
    s32 i;
    s32 channelMask;
    u8 muted;
    u8 waiting;

    waiting = 1;
    muted = 0;
    do {
        if (ControllerGetType(0) == 0) {
            if (muted == 0) {
                GraphicsDrawPauseLetters(0x88, 0x64);
                GraphicsDrawPauseLetters(0x88, 0x144);
                SoundMuteAllSpuChannels();
                muted++;
                channelMask = g_SpuChannelMask;
            }
        } else {
            if (muted != 0) {
                SoundEnableAllSpuChannels();
                g_SpuChannelMask = channelMask;
            }
            waiting = 0;
        }
    } while (waiting != 0);

    BattleUploadImageFrame();
    if (g_BattleDisplayState->slotTimersActive != 0) {
        for (i = 0; i < BATTLE_PRIM_POOL_SLOT_COUNT; i++) {
            if (g_BattlePrimPool[i].timerActive != 0) {
                if (--g_BattlePrimPool[i].timer < 0) {
                    g_BattlePrimPool[i].timer = 0;
                }
            }
        }
    }

    muted = 1;
    do {
        if (g_BattleDisplayState->battleMenuOpen == 0) {
            g_BattleMenuEvent = BATTLE_MENU_EVENT_EXIT;
        }
        if (ControllerIsStateStackFull() != 0) {
            ControllerResetState();
        } else {
            while (ControllerPopState() != 0) {
                if (g_BattlePauseActive != 0) {
                    if (*g_DebuggerState != -1) {
                        if ((g_C1ButtonStatePressedOnce & CTRL_BTN_L1) != 0) {
                            if ((g_C1ButtonStatePressedOnce & CTRL_BTN_R1) != 0) {
                                D_800C48EA = muted;
                                SoundEnableAllSpuChannels();
                                g_SpuChannelMask = channelMask;
                                g_BattlePauseActive = 0;
                                break;
                            }
                        }
                    }
                } else if ((g_C1ButtonStateReleased & CTRL_BTN_CIRCLE) != 0) {
                    g_BattleMenuEvent = 4;
                    break;
                }
                if ((g_C1ButtonStateReleased & CTRL_BTN_START) == 0) {
                    continue;
                }
                if (g_BattleSceneRunning == 0) {
                    break;
                }
                if (g_BattlePauseActive == 0) {
                    SoundMuteAllSpuChannels();
                    GraphicsDrawPauseLetters(0x88, 0x64);
                    GraphicsDrawPauseLetters(0x88, 0x144);
                    channelMask = g_SpuChannelMask;
                    g_BattlePauseActive = muted;
                } else {
                    SoundEnableAllSpuChannels();
                    g_SpuChannelMask = channelMask;
                    g_BattlePauseActive = 0;
                }
                break;
            }
        }
    } while (g_BattlePauseActive != 0);
}

void BattleUpdatePauseAndEffectTimers(u8 arg0) {
    u8* pWork;
    u8* pCounters;
    BattleActorAnimFlags** pSlot;
    s32 offset;
    u8 active;
    u8 done;
    u8 waiting;
    u8 muted;
    s32 channelMask;

    active = 1;
    waiting = 1;
    muted = 0;
    do {
        if (ControllerGetType(0) == 0) {
            if (muted == 0) {
                GraphicsDrawPauseLetters(0x88, 0x64);
                GraphicsDrawPauseLetters(0x88, 0x144);
                SoundMuteAllSpuChannels();
                muted++;
                channelMask = g_SpuChannelMask;
            }
        } else {
            if (muted != 0) {
                SoundEnableAllSpuChannels();
                g_SpuChannelMask = channelMask;
            }
            waiting = 0;
        }
    } while (waiting != 0);

    if (g_BattleDisplayState->battleMenuOpen == 0) {
        g_BattleMenuEvent = BATTLE_MENU_EVENT_EXIT;
    }
    do {
        if (ControllerIsStateStackFull() != 0) {
            ControllerResetState();
        } else {
            while (ControllerPopState() != 0) {
                if ((g_C1ButtonStateReleased & CTRL_BTN_CIRCLE) != 0) {
                    g_BattleMenuEvent = 4;
                    break;
                }
                if ((g_C1ButtonStateReleased & CTRL_BTN_START) == 0) {
                    continue;
                }
                if (g_BattlePauseActive == 0) {
                    SoundMuteAllSpuChannels();
                    GraphicsDrawPauseLetters(0x88, 0x64);
                    GraphicsDrawPauseLetters(0x88, 0x144);
                    channelMask = g_SpuChannelMask;
                    g_BattlePauseActive = 1;
                } else {
                    SoundEnableAllSpuChannels();
                    g_SpuChannelMask = channelMask;
                    g_BattlePauseActive = 0;
                }
                break;
            }
        }
    } while (g_BattlePauseActive != 0);

    if (g_BattleDisplayState->effectTimersActive == 0) {
        return;
    }
    if (D_800D32F8[0]->unk15F9 == 0) {
        return;
    }
    done = 1;
    pWork = g_BattleStateWork;
    pSlot = D_800D32F8;
    offset = 0;
    do {
        if ((*pSlot)->unk15FA == 0) {
            if (*(s32*)(D_800CDCD0 + offset) == 0) {
                (*pSlot)->unk15FA = done;
            } else {
                pCounters = (u8 *)(offset + (s32)pWork);
                *(s32*)(pCounters + 0x9E20) = *(s32*)(D_800CDCD0 + offset) - 1;
                *(s32*)(pCounters + 0x9E08) = *(s32*)(D_800CDCB8 + offset) + 1;
            }
        }
        if ((*pSlot)->unk15FB == 0) {
            if (*(s32*)(D_800CDCD4 + offset) == 0) {
                (*pSlot)->unk15FB = done;
            } else {
                pCounters = (u8 *)(offset + (s32)pWork);
                *(s32*)(pCounters + 0x9E24) = *(s32*)(D_800CDCD4 + offset) - 1;
                *(s32*)(pCounters + 0x9E0C) = *(s32*)(D_800CDCBC + offset) + 1;
            }
        }
        active &= (*pSlot)->unk15FA;
        if (D_800D32F8[0]->unk15F8 != 0) {
            active &= (*pSlot)->unk15FB;
        }
        offset += 8;
        pSlot++;
    } while ((int)pSlot < (int)(D_800D32F8 + BATTLE_EFFECT_SLOT_COUNT));
    if (active == 0) {
        func_801DF270();
        func_801DF4C0();
    } else {
        D_800D32F8[0]->unk15F9 = 0;
    }
}
void BattleUpdateEffectByScreenMode(u8 arg0) {
    switch (g_BattleDisplayMode) {
    case 0:
        BattleUpdatePauseAndEffectTimers(arg0);
        break;
    case 1:
        BattleUpdateImageUpload(arg0);
        break;
    case 2:
        BattleUpdatePauseMenu(arg0);
        break;
    }
}
void BattlePlaySoundWithBaseId(u8 arg0) {
    SoundPlaySoundEffectOnLastPair((g_BattleSoundBankMain->angle << 16) | arg0);
}
