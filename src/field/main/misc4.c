#include "common.h"
#include "main/game.h"
#include "field/actor.h"
#include "field/main.h"
#include "field/text_box.h"
#include "field/effects.h"
#include "system/memory.h"
#include "system/archive.h"
#include "system/sound.h"
#include "psyq/libetc.h"
#include "psyq/libcd.h"
#include "psyq/inline_c.h"

// The field fades between black and FIELD_FADE_COLOR_MAX, a 16.16 fixed point
// polygon colour (so >> 16 is the 0..0xFF channel value), FIELD_FADE_COLOR_STEP
// at a time
#define FIELD_FADE_COLOR_MAX 0x800000
#define FIELD_FADE_COLOR_STEP 0x40000

// Frames the field keeps rendering while it fades out, and while it waits for a
// requested disc change
#define FIELD_FADE_FRAME_COUNT 0x20
#define FIELD_DISC_CHANGE_FRAME_COUNT 8

// g_FieldZoomFadeScale is ramped by FIELD_ZOOM_FADE_SCALE_STEP per frame, up to
// FIELD_ZOOM_FADE_SCALE_MAX (0x1000 is 1:1)
#define FIELD_ZOOM_FADE_SCALE_MAX 0x22C0
#define FIELD_ZOOM_FADE_SCALE_STEP 0x20

// WDS bank the field asks for when the game is changing discs
#define FIELD_DISC_CHANGE_WDS_BANK 0xE

// FieldOrVramStripPixels converts one strip of 16-bit VRAM pixels, read two at a
// time through a u_int word
#define FIELD_SCREEN_STRIP_WIDTH 0x100
#define FIELD_SCREEN_STRIP_HEIGHT 0x20
#define FIELD_SCREEN_STRIP_WORD_COUNT (FIELD_SCREEN_STRIP_WIDTH * FIELD_SCREEN_STRIP_HEIGHT / 2)
#define FIELD_SCREEN_STRIP_SIZE (FIELD_SCREEN_STRIP_WORD_COUNT * 4)

// Number of switchable draw/display environments in g_FieldRenderContexts

// Texture UVs are byte-sized: FieldClampPolyFT4UVs pins each of them below
// FIELD_TEXTURE_UV_COUNT
#define FIELD_TEXTURE_UV_MAX 0xFF
#define FIELD_TEXTURE_UV_COUNT 0x100

// Angles are a 4096-step turn counter; every sum of angles is wrapped back into it

// Byte 1 of a controller buffer holds the device type
#define FIELD_PAD_TYPE_MOUSE 0x12

// The scratchpad is 1 KB at the top of RAM. FieldAllocScratchpad addresses it in words
#define FIELD_SCRATCHPAD_BASE 0x1F800000

// g_FieldPlayerWalkmeshMaterialOverride value that leaves the computed material alone
#define FIELD_WALKMESH_MATERIAL_OVERRIDE_NONE 0xFF

// FieldInitShadowQuad spreads its four vertices this far from the actor's origin
#define FIELD_SHADOW_QUAD_HALF_SIZE 24

// FieldInitTableQuad reads textures from six parallel short tables: the four
// GetTPage parameters then the two GetClut ones
#define FIELD_TABLE_QUAD_TEX_PARAM_COUNT 6

extern int FieldUpdateWdsAndAudioLoad(s32 arg0);
extern s32 g_GameWdsBankLoadState;
extern s32 g_GameRequestedWdsBankIndex;
extern s32 g_FieldSkipCounter;

// Ticks down the countdown timer, returning the value it held after the tick
int FieldTickSkipCounter(void) {
    int nCount;

    rand();
    if (g_GameWdsBankLoadState == -1) {
        g_GameWdsBankLoadState = FieldUpdateWdsAndAudioLoad(g_GameRequestedWdsBankIndex);
    }
    nCount = g_FieldSkipCounter;
    if (nCount != 0) {
        nCount--;
        g_FieldSkipCounter = nCount;
    }
    return nCount;
}

extern int ArchiveDataSync(void);
extern s32 g_FieldArchiveStreamActive;
extern s32 g_FieldScreenImageStored;
extern s32 g_FieldSpecialAnimLoadPending;
extern s32 g_FieldPartyModelLoadState;

INCLUDE_ASM("asm/field/nonmatchings/main/misc4", func_80078BC8);
/* Best attempt: 90.68% (144 bytes; the original is 148 / 37 insns). The original
   loads `li v0,-1` ONCE, in the delay slot of the second guard's bnez, and the
   three later guards jump to the epilogue with nop slots, i.e. one -1 definition
   serves all four failing exits. Our compile re-materialises `li v0,-1` in every
   guard's delay slot (3 instructions retail does not have) and therefore needs no
   `addu v1,v0,zero` to copy the ArchiveDataSync result out of $v0 - the net is
   one instruction short (36 vs 37).
   Shapes measured in round 487, all 144 bytes / 36 insns: five separate
   `return -1` (78.38, the last guard flips to beqz/j), a single shared
   `s32 nResult = -1;` with five `return nResult` (78.38, same flip), one
   `if (call() == 0 && g1 == 0 && g2 == 0 && g3 == 0) return -(...); return -1;`
   (79.73), the `||` chain (78.38). Earlier rounds: `nResult = -1` at declaration
   with one out label (71.76, 152 bytes - the -1 goes to a callee-saved register
   because it is live across the call), and a 4-minute permuter run (84.49).
   Presets ruled out: 2.6.0 90.68, 2.7.2-cdk 80.81, -G0 90.68.
   The 90.68 body below is the if/else-if chain (cc1 CSEs the -1 through the
   merged arms); it is the only shape that keeps all four guard slots' polarity.
   Round 500 re-measured three more: one `nResult = -1` after the call followed by
   a single `&&` chain of the four guards and `nResult = -(last)` in the else
   measures 87.57% / 148 bytes / 37 insns - retail's EXACT size, and the only
   residual is that the allocator gives `nResult` $v1 and `nSyncStatus` $v0, so
   ours tests the call result in place (no `addu v1,v0,zero`) and negates into
   $v1 then copies; retail has the pair the other way round ($v0 = nResult, hence
   the copy of the call result to $v1). Swapping the two locals' declaration order
   changes nothing, and letting the success path `return -(last)` directly drops
   back to 79.73% / 144 bytes with the first guard's -1 no longer inlined. */
/* Best attempt:
int func_80078BC8(void) {
    int nSyncStatus;
    int nResult;

    if (g_FieldArchiveStreamActive != 0) {
        return -1;
    }
    nSyncStatus = ArchiveDataSync();
    if (nSyncStatus != 0) {
        nResult = -1;
    } else if (g_GameWdsBankLoadState != 0) {
        nResult = -1;
    } else if (g_FieldSpecialAnimLoadPending != 0) {
        nResult = -1;
    } else if (g_FieldScreenImageStored != 0) {
        nResult = -1;
    } else {
        nResult = -(g_FieldPartyModelLoadState != 0xFF);
    }
    return nResult;
}
*/


extern s16 g_Unknown_800B2344;
void FieldOrVramStripPixels(void) {
    RECT rect;
    int i;
    u_int* pImage;

    if (g_Unknown_800B2344 != 0) {
        g_FieldRenderContexts[0].drawEnvs[0].dtd = 0;
        g_FieldRenderContexts[1].drawEnvs[0].dtd = 0;
        pImage = HeapAlloc(FIELD_SCREEN_STRIP_SIZE, 0x0);
        rect.y = 0x1E0;
        rect.w = FIELD_SCREEN_STRIP_WIDTH;
        rect.x = 0;
        rect.h = FIELD_SCREEN_STRIP_HEIGHT;
        StoreImage(&rect, pImage);
        DrawSync(0);

        // Process image before storing it back in VRAM
        for (i = 0; i < FIELD_SCREEN_STRIP_WORD_COUNT; i++) {
            if (pImage[i] & 0xFFFF) {
                pImage[i] |= 0xC63;
            }
            if (pImage[i] & 0xFFFF0000) {
                pImage[i] |= 0x0C630000;
            }
        }
        
        LoadImage(&rect, pImage);
        DrawSync(0);
        HeapFree(pImage);
    }
}

extern u_char g_GameDiscChangeRequest;
extern u_char g_FieldDiscChangeFade;
extern s32 g_GameStagedFileSyncCount;
extern s32 g_GamePartySkinsInitialized;
extern SoundWDSEntry* g_GameCurLoadedWDS;

extern s32 g_FieldScreenSetupDone;
extern s32 g_WorldMapAudioManagerLoaded;
extern s32 g_GameLoadedWdsBankIndex;
extern s32 g_GameReusePendingAudioManager;
extern s32 g_SoundPendingAudioManager;
extern s32 g_FieldActive;
extern s32 g_FieldScriptDataStreamActive;
extern int* g_FieldScriptDataStream; // buffer the field script data stream is read into
extern s32 g_FieldZoomFadeScale;
extern s32 g_FieldActorModelSlotCount;
extern AudioManager* g_SoundAudioManager;

extern void FieldLoadFont(void);
extern void FieldStoreScreenImage(void);
extern void FieldWaitForSceneMapFileLoad(void);
extern void FieldRenderSync(void);
extern void FieldImageConvert24BitTo15Bit(int nIndex);
extern void FieldInitializeRenderContexts(void);
extern void FieldCopyScreenToFadeTexture(void);
extern void FieldCopyScreenToVram(int x, int y);
extern void FieldClearAndSwapOTag(void);
extern void FieldZoomFadeEffectLoadTextures(int nSemiTrans, int nABR);
extern void FieldLoad(void);
extern void FieldStartScriptDataStream(void);
extern void LibGearSetDirectKeyframes(u32 shouldRender);
extern void FieldZoomFadeEffectSetPolygonColor(int color);
extern void FieldZoomFadeEffectUpdate(void);
extern void FieldDisplay(void);
extern void FieldFadeToBlack(int duration);
extern void FieldUpdateFrame(void);
extern void func_8007554C(void);
extern void FieldFreePendingAudioManager(void);
extern void FieldStartWdsBankLoad(int arg0, int arg1);
extern void FieldScriptUpdateMemoryEachFrame(void);
extern void FieldRestoreScreenImage(void);
extern void ControllerResetState(void);
extern void SoundStopAudioManager(AudioManager* manager);
extern void SoundDestroyAudioManager(AudioManager* pManager);
extern void SoundFreeWdsEntry(SoundWDSEntry* pTargetEntry);
extern void SoundFreeLoadedWdsAndAudioManager(void);
extern void func_800A24C4(void);
extern void FontFree(void);

// Loads the scene map, fades the field in, runs it until the state machine
// leaves state 1 and then tears the field down
void FieldLoadSceneAndRun(void) {
    RECT rect;
    int i;
    int nZoomScaleStep;
    int nFadeColor;

    FieldLoadFont();
    FieldStoreScreenImage();
    ArchiveSetIndex(4, 0);
    FieldWaitForSceneMapFileLoad();
    FieldRenderSync();
    if (g_FieldScreenSetupDone == 0) {
        FieldImageConvert24BitTo15Bit(0);
    }
    FieldInitializeRenderContexts();
    if (g_FieldScreenSetupDone == 0) {
        rect.x = 0;
        rect.y = 0x100;
        rect.w = SCREEN_WIDTH;
        rect.h = SCREEN_HEIGHT;
        MoveImage(&rect, 0, 0);
    }
    g_FieldCurRenderContextIndex = 1;
    FieldCopyScreenToFadeTexture();
    FieldCopyScreenToVram(0, 0x100);
    DrawSync(0);
    FieldClearAndSwapOTag();
    FieldRenderSync();
    if (g_GameDiscChangeRequest == 1 || g_FieldDiscChangeFade == 1) {
        FieldZoomFadeEffectLoadTextures(0, 0);
    } else {
        FieldZoomFadeEffectLoadTextures(1, 1);
    }
    g_FieldScreenSetupDone = 1;
    ArchiveCdDataSync(0);
    ArchiveSetIndex(4, 0);
    FieldLoad();
    FieldStartScriptDataStream();
    g_FieldActive = 1;
    if (g_FieldActorModelSlotCount != 0) {
        LibGearSetDirectKeyframes(1);
    }
    if (g_GameDiscChangeRequest == 1 || g_FieldDiscChangeFade == 1) {
        nZoomScaleStep = 0;
    } else {
        nZoomScaleStep = FIELD_ZOOM_FADE_SCALE_STEP;
    }
    nFadeColor = FIELD_FADE_COLOR_MAX;
    if (g_FieldScriptDataStreamActive == 1) {
        // A goto loop, not do-while: the do-while form makes cc1 hoist the fade
        // step and the state comparison out of the loop's preheader
    fieldLoop:
        FieldClearAndSwapOTag();
        FieldZoomFadeEffectUpdate();
        FieldDisplay();
        if (g_FieldDiscChangeFade != 1) {
            if (g_FieldZoomFadeScale < FIELD_ZOOM_FADE_SCALE_MAX) {
                g_FieldZoomFadeScale += nZoomScaleStep;
            }
        } else {
            FieldZoomFadeEffectSetPolygonColor(nFadeColor >> 16);
            nFadeColor -= FIELD_FADE_COLOR_STEP;
            if (nFadeColor < 0) {
                nFadeColor = 0;
            }
        }
        if (ArchiveDataSync() != 0) {
            goto fieldLoop;
        }
        DrawSync(0);
        HeapFree(g_FieldScriptDataStream);
        g_FieldScriptDataStreamActive = 0;
        FieldOrVramStripPixels();
    }
    if (g_FieldDiscChangeFade == 1) {
        do {
            FieldClearAndSwapOTag();
            FieldZoomFadeEffectUpdate();
            FieldDisplay();
            FieldZoomFadeEffectSetPolygonColor(nFadeColor >> 16);
            nFadeColor -= FIELD_FADE_COLOR_STEP;
        } while (nFadeColor >= 0);
    }
    if (g_GameDiscChangeRequest == 1) {
        rect.y = 0;
        rect.x = 0;
        rect.w = SCREEN_WIDTH;
        rect.h = SCREEN_HEIGHT;
        MoveImage(&rect, 0x200, 0);
    }
    if (g_WorldMapAudioManagerLoaded != 0) {
        SoundStopAudioManager(g_SoundAudioManager);
        SoundDestroyAudioManager(g_SoundAudioManager);
        SoundFreeWdsEntry(g_GameCurLoadedWDS);
        g_WorldMapAudioManagerLoaded = 0;
    }
    func_800A24C4();
    g_GameStagedFileSyncCount = 0;
    g_GamePartySkinsInitialized = 0;
    FieldRenderSync();
    ControllerResetState();
    g_GameWdsBankLoadState = 0;
    if (g_GameDiscChangeRequest == 1) {
        g_GameRequestedWdsBankIndex = FIELD_DISC_CHANGE_WDS_BANK;
        FieldFreePendingAudioManager();
    }
    if (g_GameLoadedWdsBankIndex != g_GameRequestedWdsBankIndex) {
        SoundFreeLoadedWdsAndAudioManager();
        g_GameWdsBankLoadState = -1;
        if (g_SoundPendingAudioManager != 0) {
            g_GameReusePendingAudioManager = 1;
        }
        FieldStartWdsBankLoad(g_GameRequestedWdsBankIndex, 1);
    } else {
        FieldFreePendingAudioManager();
    }
    FieldScriptUpdateMemoryEachFrame();
    if (g_GameDiscChangeRequest != 1) {
        if (g_FieldDiscChangeFade != 1) {
            FieldFadeToBlack(0x20);
            nFadeColor = FIELD_FADE_COLOR_MAX;
            for (i = 0; i < FIELD_FADE_FRAME_COUNT; i++) {
                FieldUpdateFrame();
                FieldZoomFadeEffectUpdate();
                func_8007554C();
                FieldTickSkipCounter();
                if (g_GameDiscChangeRequest != 1) {
                    FieldZoomFadeEffectSetPolygonColor(nFadeColor >> 16);
                    nFadeColor -= FIELD_FADE_COLOR_STEP;
                    if (nFadeColor < 0) {
                        nFadeColor = 0;
                    }
                    if (g_FieldZoomFadeScale < FIELD_ZOOM_FADE_SCALE_MAX) {
                        g_FieldZoomFadeScale += nZoomScaleStep;
                    }
                }
            }
        } else {
            FieldFadeToBlack(0x20);
        }
    } else {
        for (i = 0; i < FIELD_DISC_CHANGE_FRAME_COUNT; i++) {
            FieldUpdateFrame();
            FieldZoomFadeEffectUpdate();
            func_8007554C();
            FieldTickSkipCounter();
        }
    }
    if (g_FieldActorModelSlotCount != 0) {
        LibGearSetDirectKeyframes(0);
    }
    FieldRestoreScreenImage();
    HeapConsolidate();
    FontFree();
    FieldLoadFont();
    g_FieldActive = 0;
}

// The skin slot table FieldRandomizeSkinSlots (main/misc) fills: the period at
// +0x0, the number of slots in use at +0x4 and the slot values at +0x8. The
// original reaches all three through one base register, so they are one block.
#define FIELD_RANDOM_SKIN_SLOT_COUNT 32
#define FIELD_RANDOM_SKIN_SLOT_EMPTY 0xFFFF

typedef struct {
    /* 0x0 */ s32 period;
    /* 0x4 */ s32 count;
    /* 0x8 */ u16 values[FIELD_RANDOM_SKIN_SLOT_COUNT];
} FieldSkinTable; // Size: 0x48

// Number of skins the selection table covers, i.e. one weight and one value per
// skin
#define FIELD_SKIN_COUNT 16

extern FieldSkinTable D_800B2298;
extern s32 g_FieldRandomSkinSlots; // frames left before the table is regenerated
extern u16 D_800B2270[];    // per-skin value handed to the field system
extern s16 g_FieldPendingWdsBankIndex;
extern u_char D_80065ADC[]; // per-skin selection weight
extern u_char D_800ADB04;
extern s32 D_800ADBD0;
extern u_char g_GameSelectedSkinIndex;
extern u_char g_GameSelectedSkinLoadPending;
extern void FieldRandomizeSkinSlots(void);
extern void LoadGameStateOverlay(int);
extern void func_80281204(int);
extern int rand(void);
extern s32 g_FieldMapChangeGate;
extern s32 g_FieldEntryStateGate;
extern s32 g_FieldMapTransitionGate;
extern s32 g_FieldArchiveStreamActive;
extern s32 g_FieldSuppressStateChange;

// Ages the slot table and, once a slot frees up, rolls a skin against
// D_80065ADC's weights and switches the party to it.
// Best attempt: 81.59% (712 bytes; retail 684, 178 vs 171 insns).
// What this body gets exactly right: the profile of eight gate checks, the
// countdown (lui/lw + sw through $at, i.e. g_FieldRandomSkinSlots read as its own
// absolute s32, NOT as a field of the table), loop 1 in full (walk pointer, count
// re-read at -4 of the values base), the nTotal loop, the prefix-sum fill and the
// tail from D_800B2270[i] to func_80281204. The whole "three symbols through one
// base" rebase of the old note was NOT a giv problem: it is one struct, the
// period at +0x0, count at +0x4 and values at +0x8 of the block at 0x800B2298,
// and writing it as a struct reproduces retail's single s1 base and its
// "lw -4(a0)" count read.
// What is left, and it is all ONE root cause (the search loop's block layout):
// retail lays the found-slot body OUT of line, behind "beqz" from the loop top,
// so the loop itself is only "load, compare, advance" with a walk pointer, and
// loop.c therefore hoists nothing out of it. Our compile keeps the body inline,
// inverts the branch (bnez to an out-of-line copy of the advance), recomputes
// values[i] with sll/addu every iteration instead of walking, and hoists the
// nWeightSums base (sp+16) into a fifth callee-saved register, which is where
// the extra instructions and the 5-vs-4 saved set come from.
// Tried here: "if (values[i] == 0) { ...body...; break; }" (712), the same with
// "...; return;" (712), "if (values[i] != 0) continue;" + break (712, identical
// code), and a u16* pSlot walk pointer (712, worse fuzzy 77.22). Dropping the
// break/return entirely gets 692 bytes / 80.36% but is wrong code: the loop
// would scan on and re-roll after a skin was already chosen.
// Best attempt: 92.85% (692 bytes; retail 684, 173 vs 171 insns).
// The search loop is a GOTO, not a loop body: the original wrote
//     for (i = 0; i < D_800B2298.count; i++)
//         if (D_800B2298.values[i] == 0) goto foundSlot;
//     return;
// foundSlot: ...
// which is what makes gcc lay the whole found-slot body OUT of line behind the
// loop, with the "j end" of the failed search textually before it (91 -> 92.85,
// 712 -> 692 bytes). Everything up to there now matches retail exactly: the
// eight gate checks, the countdown as its own absolute s32 g_FieldRandomSkinSlots
// (not a field of the table), loop 1, the nTotal loop, the prefix-sum fill.
// Residual, all in the two loop tails:
//  - our loop 2 keeps the count in a1 instead of re-reading it at -4(a1), so the
//    loop runs "move a1,v0 / slt v0,s0,a1" where retail has "lw v0,-4(a1) /
//    slt v0,s0,v0": retail's store to values[i] still sits inside the loop's
//    block set, ours does not.
//  - the frame is 112 vs retail's 120: retail has 24 bytes more of locals than
//    the 16-int nWeightSums array (0x10..0x50) and a gap up to the saves at
//    0x68, i.e. its array block is 88 bytes, not 64.
//  - the last loop's found arm costs retail 2 more instructions (beqz/slt/beqz +
//    "j" with nSum++ in the delay slot); ours branches straight to the body.
// The -4(a1) count read means the body must still be reachable from the loop
// header; the array block being 88 bytes is the other half of the frame delta.
int FieldUpdateRandomSkinSelection(void) {
    int nWeightSums[FIELD_SKIN_COUNT + 2];
    int nTotal;
    int nSum;
    int i;
    u_char* pWeight;

    if (g_FieldMapChangeGate == 0) {
        return;
    }
    if (g_FieldEntryStateGate == 0) {
        return;
    }
    if (g_FieldMapTransitionGate == 0) {
        return;
    }
    if (g_GameWdsBankLoadState == -1) {
        return;
    }
    if (D_800B2298.period == 0) {
        return;
    }
    if (g_FieldControl.isRandomEncountersEnabled == -1) {
        return;
    }
    if (g_FieldArchiveStreamActive == 1) {
        return;
    }
    if (D_800ADB04 == 0) {
        return;
    }
    if (--g_FieldRandomSkinSlots == 0) {
        FieldRandomizeSkinSlots();
    }
    for (i = 0; i < D_800B2298.count; i++) {
        if (D_800B2298.values[i] != FIELD_RANDOM_SKIN_SLOT_EMPTY) {
            D_800B2298.values[i]--;
        }
    }
    for (i = 0; i < D_800B2298.count; i++) {
        if (D_800B2298.values[i] == 0) {
            D_800B2298.values[i] = FIELD_RANDOM_SKIN_SLOT_EMPTY;
            goto foundSlot;
        }
    }
    return;
foundSlot:
    pWeight = D_80065ADC;
    nTotal = 0;
    for (i = 0; i < FIELD_SKIN_COUNT; i++) {
        nTotal += pWeight[i];
    }
    nSum = 0;
    for (i = 0; i < FIELD_SKIN_COUNT; i++) {
        nWeightSums[i] = nSum;
        nSum += pWeight[i];
    }
    nTotal = (rand() * (nTotal + 1)) >> 15;
    nSum = 0;
    for (i = FIELD_SKIN_COUNT - 1; i >= 0; i--) {
        if (pWeight[i] != 0 && nWeightSums[i] < nTotal) {
            nSum++;
            break;
        }
    }
    if (nSum == 0) {
        return;
    }
    g_GameSelectedSkinIndex = i;
    g_GameSelectedSkinLoadPending = 0;
    g_FieldPendingWdsBankIndex = D_800B2270[i];
    if (g_FieldSuppressStateChange == 0) {
        LoadGameStateOverlay(2);
    }
    g_FieldMapChangeGate = 0;
    D_800ADBD0 = 1;
    if (g_FieldSystemMode == 0) {
        func_80281204(i);
    }
}




extern s32 g_FieldInitialWdsBankIndex;
extern s32 g_FieldSuppressStateChange;
extern s16 g_GameWdsEntryReleased;
extern s32 g_FieldNextGameState;
extern void FieldScriptSyncMemoryWithGameState(void);
extern void SoundFreeLoadedWdsAndAudioManager(void);
extern void FieldBeginWdsFileLoad(void);
extern void FieldFinishWdsFileLoad(void);
extern void GameStartSelectedSkinLoad(void);
extern void ArchiveCdDataSync(int);
extern void ChangeGameState(int);

// Field game state machine: one case per transition
void FieldAdvanceGameState(int state) {
    g_FieldDiscChangeFade = 0;
    switch (state) {
    case 0: {
        u16 nSaved;

        FieldScriptSyncMemoryWithGameState();
        nSaved = g_pGameState->fieldScriptMemory[1];
        g_GameRequestedWdsBankIndex = g_FieldInitialWdsBankIndex;
        g_pGameState->wdsBankIndex = g_FieldInitialWdsBankIndex;
        g_pGameState->nextMapFlags = nSaved;
        if (g_FieldSuppressStateChange != 0) {
            return;
        }
        ChangeGameState(2);
        break;
    }
    case 1:
        if (g_GameWdsEntryReleased == state) {
            SoundFreeLoadedWdsAndAudioManager();
            FieldBeginWdsFileLoad();
            ArchiveCdDataSync(0);
            FieldFinishWdsFileLoad();
            SoundFreeLoadedWdsAndAudioManager();
        }
        if (g_FieldSuppressStateChange != 0) {
            return;
        }
        ChangeGameState(3);
        break;
    case 2: {
        u16 nSaved;

        nSaved = g_pGameState->fieldScriptMemory[1];
        g_pGameState->wdsBankIndex = g_GameRequestedWdsBankIndex;
        g_pGameState->nextMapFlags = nSaved;
        if (g_FieldSuppressStateChange != 0) {
            return;
        }
        ChangeGameState(4);
        g_GamePartySkinsInitialized++;
        break;
    }
    case 3:
        g_GameStagedFileSyncCount = 0;
        g_GamePartySkinsInitialized = 0;
        if (g_FieldSuppressStateChange != 0) {
            return;
        }
        if (g_FieldNextGameState & 0x80) {
            GameStartSelectedSkinLoad();
        }
        ChangeGameState(g_FieldNextGameState & 0x7F);
        break;
    }

    MainLoop(0);
}

void FieldNoOp2(void) {}

void FieldSwapRenderContext(void) {
    g_FieldCurRenderContextIndex = (g_FieldCurRenderContextIndex + 1) % FIELD_RENDER_CONTEXT_COUNT;
    g_FieldCurRenderContext = &g_FieldRenderContexts[g_FieldCurRenderContextIndex];
    PutDispEnv(&g_FieldCurRenderContext->dispEnv);
    PutDrawEnv(&g_FieldCurRenderContext->drawEnvs[0]);
}

extern RECT g_FieldColorFillRect;
extern TILE g_FieldColorFillTiles[];
extern DR_MODE g_FieldColorFillDrawModes[];
	
void FieldRenderColorFill(int color) {
    FieldClearAndSwapOTag();
    setRGB(g_FieldColorFillTiles[g_FieldCurRenderContextIndex], color * 4);
    addPrim(g_FieldCurRenderContext->ot1, &g_FieldColorFillTiles[g_FieldCurRenderContextIndex]);
    addPrim(g_FieldCurRenderContext->ot1, &g_FieldColorFillDrawModes[g_FieldCurRenderContextIndex]);
    FieldRenderSync();
    MoveImage(&g_FieldColorFillRect, 0, g_FieldCurRenderContextIndex * 0x100);
    PutDispEnv(&g_FieldCurRenderContext->dispEnv);
    PutDrawEnv(&g_FieldCurRenderContext->drawEnvs[0]);
    DrawOTag(g_FieldCurRenderContext->ot1 + 1);
}


extern s32 g_FieldSceneReloadFlags;
extern u_char g_FieldPlayerWalkmeshMaterial;
extern s16 g_FieldPlayerWalkmeshMaterialOverride;
extern s32 g_PlayerActorIndex;

void FieldUpdatePlayerWalkmeshMaterial(void) {
    if (g_FieldSceneReloadFlags != 0 && (g_FieldActors[g_PlayerActorIndex].pActorData->curWalkmeshTriMaterial & 0xC0) == 0) {
        g_FieldPlayerWalkmeshMaterial = 0;
    } else {
        g_FieldPlayerWalkmeshMaterial = 1;
    }
    if (g_FieldPlayerWalkmeshMaterialOverride != FIELD_WALKMESH_MATERIAL_OVERRIDE_NONE) {
        g_FieldPlayerWalkmeshMaterial = g_FieldPlayerWalkmeshMaterialOverride;
    }
}

void FieldMoveImageAndSync(short w, short h, short x, short y, int destX, int destY) {
    RECT rect;

    rect.w = w;
    rect.h = h;
    rect.x = x;
    rect.y = y;
    MoveImage(&rect, destX, destY);
    DrawSync(0);
}

void FieldRenderSyncAndFlush(void) {
    FieldRenderSync();
    EnterCriticalSection();
    FlushCache();
    ExitCriticalSection();
}

// A request code of 0x80 is a random encounter; the low seven bits of the code
// name the field state the map is being loaded for
#define FIELD_REQUEST_RANDOM_ENCOUNTER 0x80
#define FIELD_REQUEST_CODE_MASK 0x7F

// Archive entry holding the gear overlay, which is only loaded when the map has
// actor model slots
#define FIELD_GEAR_OVERLAY_ARCHIVE_INDEX 0x6B9

// The strip table below covers the frame buffer in six 0x40 x 0x20 blocks
#define FIELD_FRAMEBUFFER_STRIP_COUNT 6

// Source and VRAM corner of one strip of the frame buffer
typedef struct {
    /* 0x0 */ s16 x;
    /* 0x2 */ s16 y;
} ScreenStripCorner; // Size: 0x4

extern s32 g_FieldRequestCode;
extern u_char g_FieldRandomEncounterFlag;
extern void* g_FieldArchiveQueueBuffer;
extern s32 g_FieldStreamDataSize;
extern void* g_MenuResourcesFile;
extern u_char g_MenuDebugEnabled;
extern u_char g_MenuOverlayIndex;
extern s32 g_GfxOTDepthShift;
extern s32 g_GameSceneMapNum;
extern u_char g_FieldScriptMemoryUpdateLocked;
extern s32 g_FieldScriptSuppressScreenEffect;
extern s32 g_FieldScreenEffectSuppressed;
extern s32 g_FieldPendingRequestCount;
extern s32 g_GamePartyMemberSkins[MAX_PARTY_MEMBERS];
extern s32 g_GamePartyUsesGearSkins;
extern s32 g_GamePartyQueuedSkinKind;
extern void* g_PartyDataBuffers[MAX_PARTY_MEMBERS];
extern u_char g_FieldDisableActorRender;
extern u_long* g_MenuGfxEnvOtBuffer0[2];
extern u16 D_8006BE2C[MAX_PARTY_MEMBERS];
extern ScreenStripCorner D_800ADCB0[FIELD_FRAMEBUFFER_STRIP_COUNT];
extern ScreenStripCorner D_800ADCC8[FIELD_FRAMEBUFFER_STRIP_COUNT];
extern void ArchiveStartStreamQueue(StreamDataQueueEntry* pQueue, s32 arg1, s32 arg2);
extern int ArchiveDecodeAlignedSize(u32 entryIndex);
extern void ArchiveReadFileToBuffer(s32 fileIndex, void* pBuffer, u32 arg2, u32 flags);
extern void LZSSDecompress(void* pSrc, void* pDst);
extern void MenuMain(void);
extern void FieldScriptMemoryWriteU16(int index, int value);
extern void FieldFreeScriptDataStream(void);
extern void FieldPartyFreeSkinDataBuffers(void);
extern void FieldPartyAllocateSkinDataBuffers(void);
extern void FieldScriptRunFirstActorRoutine3(void);
extern void GamePartySyncSkinData(void);
extern void GamePartySyncStreamedData(void);

// Loads everything a field map needs: the palette and stream data, the frame
// buffer it was entered from, the render contexts and the party skin data
// PARKED 90.03% (2644 bytes / 661 insns; retail 2680 / 670) - measured, compiles.
// Frame 0x78 with fp+s0-s7+ra, the prologue/epilogue, every call and branch is
// retail's. The residual is two things, both spread over the whole body:
//  1. HOW AN ABSOLUTE ADDRESS IS REACHED. Retail holds &g_FieldColorFillRect in
//     $s0 across four MoveImage/LoadImage calls and stores x through `0(s0)` while
//     y/w/h go through the assembler's `lui $at` pair; the compile re-forms a
//     `lui $at` for EVERY store and for every &-argument (+~1 insn per block).
//     After reload gcc's CSE only merges an address that matches a register
//     EXACTLY, and a `sh $v0,sym+2` MEM has no base register - hence the mix in
//     retail and its absence here.
//  2. The two strip loops: retail strength-reduces each field reference into its
//     own walker (s0/s1 = the +0/+2 pair of D_800ADCB0, s3/s4 the pair of
//     D_800ADCC8, +4 per iteration, 8 preheader insns); gcc gives one base per
//     table with 0/2 offsets (4 preheader insns, 2 fewer per iteration). Both
//     loads are already u16-for-rect / s16-for-the-call like retail's.
// Also link-identical but objdiff-visible (reloc names only, SECOND EXCEPTION):
//     g_FieldColorFillTiles[1] relocates as tiles+0x10 where retail names
//     D_800AFE64; g_MenuGfxEnvOtBuffer0[1] as +4 where retail names D_8005A4B0;
//     the y/w/h stores as g_FieldColorFillRect+2/+4/+6 where retail names
//     D_800AFE4E/50/52.
// PRESET (re-measured with try_flags.sh --all on THIS body): under --gp -G0 the
// function goes 90.03 -> 91.89 and every other function of the unit keeps its
// score exactly (all 100.00 except FieldUpdateRandomSkinSelection 99.83 under
// unit flags too), so the unit is a clean -G0 candidate - but even -G0 does not
// close this function; gcc-2.7.2-cdk 83.37, cdk -G0 74.26.
// TRIED, one line each (newest round first):
//  - Store order vs the `la`: in retail the `la &g_FieldColorFillRect` sits at
//    the TOP of each rect block, before the four field stores (that is what buys
//    it `sh v0,0(s0)` for x and `lui $at` for +2/+4/+6); the compile puts the la
//    after them, next to the MoveImage argument, so all four stores take the
//    assembler's `lui $at`. cse folds an offset-0 symbolic store into a register
//    only when that register is already live, and the source cannot order it.
//  - RE-MEASURED live (un-parked and diffed twice): 90.03% / 2644 bytes /
//    661 insns (retail 2680 / 670), compiles from the park text - the number is
//    not stale.
//  - RECT* pRect = &g_FieldColorFillRect used for the MoveImage arguments only
//    (field stores left on the global): flat at 90.03, gcc rematerialises the
//    address per call all the same. The s0 kept-address of retail cannot be
//    steered from the source this way.
//  - `D_800ADCB0[i].x / [i].y` for both strip loops instead of the ++ walkers:
//    87.25, and gcc still gives ONE base per table - it strength-reduces to
//    `lhu $2,D_800ADCB0($16)` / `lh $5,D_800ADCC8($16)` rather than to retail's
//    four walkers. The walkers are the better spelling, keep them.
//  - queue[2]/queue[3] zeroed BEFORE pPalette = HeapAlloc(...): 88.93 -> 90.02
//    (retail's store order), kept.
//  - `nSize = (nStreamSize & 0xFFFFFF) + ...` instead of reloading
//    g_FieldStreamDataSize: 660 insns / 89.80 - one insn closer but 0.23 lower,
//    so the reload spelling is what this park carries.
//  - rect.w/h before rect.x/y in the first LoadImage block: kept (304 -> 301
//    differing lines, flat on the percentage).
//  - RECT* pRect = &g_FieldColorFillRect used for every field store and call
//    argument: 87.74 (worse; retail only merges the offset-0 store).
//  - indexed `T[i].x` vs explicit `pSrc++` walkers: 84.84 -> 88.90 (walkers).
//  - rect/queue declaration order: what put the locals at retail's 0x18/0x20.
//  - `void* pQueueBackup = NULL` (prologue `move fp,zero`): part of the 88.90.
//  - nRequestCode local before the last request-code test: frame 0x70 -> 0x78.
// OPEN: nStreamSize sits at 0x40 where retail uses 0x48, and pStreamBuffer is
// spilled to the stack where retail keeps it in $s7 - if that spill is fixed, the
// free slot at 0x48 lands on retail's nStreamSize. Retail's 0x40/0x44/0x4C are
// never referenced (12 unaccounted bytes), so that slot is not a missing object,
// just a different allocation.
/*
void func_800799D4(void) {
    RECT rect;
    StreamDataQueueEntry queue[4];
    s32 nSize;
    s32 nRequestCode;
    s32 nStreamSize;
    void* pQueueBackup = NULL;
    void* pStreamBuffer;
    void* pPalette;
    void* pImageA;
    void* pImageB;
    void* pSkinData;
    ScreenStripCorner* pSrcCorner;
    ScreenStripCorner* pDstCorner;
    int i;

    if (g_FieldRequestCode == FIELD_REQUEST_RANDOM_ENCOUNTER && g_FieldRandomEncounterFlag != 0) {
        return;
    }

    setTile(&g_FieldColorFillTiles[0]);
    SetSemiTrans(&g_FieldColorFillTiles[0], 1);
    setRGB(g_FieldColorFillTiles[0], 0);
    g_FieldColorFillTiles[0].y0 = 0;
    g_FieldColorFillTiles[0].x0 = 0;
    g_FieldColorFillTiles[0].w = SCREEN_WIDTH;
    g_FieldColorFillTiles[0].h = SCREEN_HEIGHT;
    g_FieldColorFillTiles[1] = g_FieldColorFillTiles[0];
    SetDrawMode(&g_FieldColorFillDrawModes[0], 0, 0, GetTPage(0, 2, 0, 0), NULL);
    SetDrawMode(&g_FieldColorFillDrawModes[1], 0, 0, GetTPage(0, 2, 0, 0), NULL);

    FieldPartyFreeSkinDataBuffers();
    if (g_FieldActorModelSlotCount != 0) {
        ArchiveSetIndex(ARCHIVE_DIR_FIELD, 0);
        pQueueBackup = HeapAlloc(ArchiveDecodeAlignedSize(FIELD_GEAR_OVERLAY_ARCHIVE_INDEX), 0);
        memcpy(pQueueBackup, g_FieldArchiveQueueBuffer, ArchiveDecodeAlignedSize(FIELD_GEAR_OVERLAY_ARCHIVE_INDEX));
        HeapFree(g_FieldArchiveQueueBuffer);
    }

    nStreamSize = g_FieldStreamDataSize;
    ArchiveSetIndex(ARCHIVE_DIR_MENUS, 0);
    if (g_FieldSuppressStateChange == 1) {
        nSize = ArchiveDecodeAlignedSize((g_FieldRequestCode + 5) & FIELD_REQUEST_CODE_MASK);
    } else {
        nSize = (g_FieldStreamDataSize & 0xFFFFFF) + 0xFFE3AFF8;
    }
    pStreamBuffer = HeapAlloc(nSize, 1);

    queue[2].archiveIndex = 0;
    queue[2].pData = NULL;
    queue[3].archiveIndex = 0;
    queue[3].pData = NULL;
    pPalette = HeapAlloc(ArchiveDecodeAlignedSize(1), 1);
    g_MenuResourcesFile = pPalette;
    queue[0].archiveIndex = 1;
    queue[0].pData = pPalette;
    queue[1].archiveIndex = (g_FieldRequestCode & FIELD_REQUEST_CODE_MASK) + 5;
    queue[1].pData = pStreamBuffer;
    if ((g_FieldRequestCode & FIELD_REQUEST_CODE_MASK) == 5 && g_FieldSuppressStateChange == 0) {
        queue[2].archiveIndex = 0xC;
        queue[2].pData = (void*)0x1DC000;
    }
    ArchiveCdDataSync(0);
    ArchiveStartStreamQueue(queue, 0, 0);

    ArchiveSetIndex(ARCHIVE_DIR_FIELD, 0);
    rect.w = 0x40;
    rect.h = 0x20;
    for (i = 0, pSrcCorner = D_800ADCB0, pDstCorner = D_800ADCC8; i < FIELD_FRAMEBUFFER_STRIP_COUNT; i++, pSrcCorner++, pDstCorner++) {
        rect.x = pSrcCorner->x;
        rect.y = pSrcCorner->y;
        MoveImage(&rect, pDstCorner->x, pDstCorner->y);
        DrawSync(0);
    }
    FieldMoveImageAndSync(0x40, 0x100, 0x3C0, 0x100, 0x300, 0);
    FieldMoveImageAndSync(0x40, 0x100, 0x2C0, 0x100, 0x280, 0);

    g_FieldColorFillRect.x = 0x2C0;
    g_FieldColorFillRect.y = 0x100;
    g_FieldColorFillRect.w = SCREEN_WIDTH;
    g_FieldColorFillRect.h = SCREEN_HEIGHT;
    FieldCopyScreenToFadeTexture();
    MoveImage(&g_FieldColorFillRect, 0, 0x100);
    DrawSync(0);
    for (i = 0; i < 0x20; i++) {
        FieldRenderColorFill(i);
    }
    FieldRenderSync();
    g_FieldColorFillRect.x = 0;
    g_FieldColorFillRect.y = 0;
    FieldSwapRenderContext();
    FontFree();
    MoveImage(&g_FieldColorFillRect, 0, 0xE0);
    FieldRenderSync();

    ArchiveCdDataSync(0);
    g_GameDiscChangeRequest = 0;
    g_MenuDebugEnabled = 0;
    g_MenuOverlayIndex = g_FieldRequestCode & FIELD_REQUEST_CODE_MASK;
    for (i = 0; i < MAX_PARTY_MEMBERS; i++) {
        D_8006BE2C[i] = g_pGameState->gearRide[i];
    }
    FieldUpdatePlayerWalkmeshMaterial();
    g_MenuGfxEnvOtBuffer0[0] = g_FieldRenderContexts[0].ot1;
    g_MenuGfxEnvOtBuffer0[1] = g_FieldRenderContexts[1].ot1;
    FieldRenderSyncAndFlush();
    MenuMain();
    FieldRenderSyncAndFlush();

    g_GfxOTDepthShift = 2;
    if (g_GameDiscChangeRequest == 0) {
        if ((g_FieldRequestCode & FIELD_REQUEST_CODE_MASK) == 2) {
            g_FieldScriptMemoryUpdateLocked = 1;
            FieldScriptMemoryWriteU16(0x46, 0);
            FieldScriptMemoryWriteU16(4, 4);
            g_GameSceneMapNum = 4;
            g_pGameState->nextMapFlags = 0;
            g_pGameState->fieldScriptMemory[1] = 0;
            g_pGameState->mapNum = 4;
        }
    }
    if (g_GameDiscChangeRequest == 2) {
        g_FieldScriptMemoryUpdateLocked = 1;
        FieldScriptMemoryWriteU16(0x46, 2);
        FieldScriptMemoryWriteU16(4, g_pGameState->mapNum & 0x3FFF);
        if ((g_pGameState->mapNum & 0x3FFF) < 0x400) {
            g_pGameState->nextMapFlags = g_pGameState->fieldScriptMemory[0x2A];
        }
    }

    FieldRenderSync();
    g_FieldColorFillRect.x = 0;
    g_FieldColorFillRect.y = 0xE0;
    g_FieldColorFillRect.w = SCREEN_WIDTH;
    g_FieldColorFillRect.h = SCREEN_HEIGHT;
    MoveImage(&g_FieldColorFillRect, 0x140, 0);
    FieldRenderSync();
    g_FieldColorFillRect.x = 0x140;
    g_FieldColorFillRect.y = 0;
    MoveImage(&g_FieldColorFillRect, 0, 0);
    MoveImage(&g_FieldColorFillRect, 0, 0x100);
    FieldRenderSync();
    PutDispEnv(&g_FieldCurRenderContext->dispEnv);
    PutDrawEnv(g_FieldCurRenderContext);
    g_FieldColorFillRect.x = 0x2C0;
    g_FieldColorFillRect.y = 0x100;
    FieldRenderColorFill(0x1F);
    FieldRenderColorFill(0x1F);

    rect.x = 0x300;
    rect.y = 0;
    rect.w = 0x40;
    rect.h = 0x100;
    pImageA = HeapAlloc(0x8000, 1);
    StoreImage(&rect, pImageA);
    DrawSync(0);
    rect.x = 0x280;
    rect.y = 0;
    rect.w = 0x40;
    rect.h = 0x100;
    pImageB = HeapAlloc(0x8000, 1);
    StoreImage(&rect, pImageB);
    DrawSync(0);

    for (i = 0, pSrcCorner = D_800ADCB0, pDstCorner = D_800ADCC8; i < FIELD_FRAMEBUFFER_STRIP_COUNT; i++, pSrcCorner++, pDstCorner++) {
        rect.w = 0x40;
        rect.h = 0x20;
        rect.x = pSrcCorner->x;
        rect.y = pSrcCorner->y;
        MoveImage(&rect, pDstCorner->x, pDstCorner->y);
        DrawSync(0);
    }

    ArchiveSetIndex(ARCHIVE_DIR_FIELD, 0);
    HeapChangeCurrentUser(8, NULL);
    g_FieldScriptDataStreamActive = 0;
    FieldStartScriptDataStream();
    if (g_FieldScriptSuppressScreenEffect != 0) {
        for (i = 0x20; i < 0x3F; i++) {
            FieldRenderColorFill(i);
        }
        g_FieldScreenEffectSuppressed = 1;
    } else {
        for (i = 0x1F; i >= 0; i--) {
            FieldRenderColorFill(i);
        }
        FieldRenderColorFill(0);
        g_FieldScreenEffectSuppressed = 0;
    }
    FieldFreeScriptDataStream();
    FieldRenderSync();

    rect.w = 0x40;
    rect.h = 0x100;
    rect.x = 0x2C0;
    rect.y = 0x100;
    LoadImage(&rect, pImageB);
    DrawSync(0);
    rect.x = 0x3C0;
    LoadImage(&rect, pImageA);
    DrawSync(0);
    HeapFree(pImageB);
    HeapFree(pImageA);
    HeapFree(pStreamBuffer);

    ArchiveSetIndex(ARCHIVE_DIR_FIELD, 0);
    if (g_FieldActorModelSlotCount != 0) {
        if (g_FieldSuppressStateChange != 0) {
            g_FieldArchiveQueueBuffer = HeapAlloc(ArchiveDecodeAlignedSize(FIELD_GEAR_OVERLAY_ARCHIVE_INDEX), 1);
        } else {
            g_FieldArchiveQueueBuffer = HeapAlloc((nStreamSize & 0xFFFFFF) + 0xFFE23FF8, 1);
        }
        memcpy(g_FieldArchiveQueueBuffer, pQueueBackup, ArchiveDecodeAlignedSize(FIELD_GEAR_OVERLAY_ARCHIVE_INDEX));
        HeapFree(pQueueBackup);
    }

    SetGeomOffset(0xA0, 0x70);
    SetGeomScreen(g_Scene.sceneScrZ);
    FieldPartyAllocateSkinDataBuffers();
    nRequestCode = g_FieldRequestCode;
    if (nRequestCode == 1) {
        g_GamePartyMemberSkins[2] = 0xFF;
        g_GamePartyMemberSkins[1] = 0xFF;
        g_GamePartyMemberSkins[0] = 0xFF;
        g_GamePartyUsesGearSkins = 0;
        g_GamePartyQueuedSkinKind = 0;
        GamePartySyncSkinData();
        GamePartySyncStreamedData();
        FieldRenderSync();
        g_FieldMapTransitionGate = 0;
        g_FieldDisableActorRender = 1;
    } else {
        for (i = 0; i < MAX_PARTY_MEMBERS; i++) {
            if (g_GamePartyMemberSkins[i] == 0xFF) {
                continue;
            }
            pSkinData = HeapAlloc(ArchiveDecodeAlignedSize(g_GamePartyMemberSkins[i] + 5), 1);
            ArchiveReadFileToBuffer(g_GamePartyMemberSkins[i] + 5, pSkinData, 0, 0x80);
            ArchiveCdDataSync(0);
            LZSSDecompress(pSkinData, g_PartyDataBuffers[i]);
            HeapFree(pSkinData);
        }
        FieldScriptRunFirstActorRoutine3();
        FieldRenderSync();
    }
    g_FieldRequestCode = 0xFF;
    FieldLoadFont();
    g_FieldPendingRequestCount = 0;
}*/
INCLUDE_ASM("asm/field/nonmatchings/main/misc4", func_800799D4);



// Quad rendering
// --------------------
void FieldClampPolyFT4UVs(POLY_FT4* poly, short u0, short v0, short u1, short v1, short u2, short v2, short u3, short v3) {
    if (u0 < 0) u0 = 0;
    if (u1 < 0) u1 = 0;
    if (u2 < 0) u2 = 0;
    if (u3 < 0) u3 = 0;
    if (v0 < 0) v0 = 0;
    if (v1 < 0) v1 = 0;
    if (v2 < 0) v2 = 0;
    if (v3 < 0) v3 = 0;
    
    if (u0 >= FIELD_TEXTURE_UV_COUNT) u0 = FIELD_TEXTURE_UV_MAX;
    if (u1 >= FIELD_TEXTURE_UV_COUNT) u1 = FIELD_TEXTURE_UV_MAX;
    if (u2 >= FIELD_TEXTURE_UV_COUNT) u2 = FIELD_TEXTURE_UV_MAX;
    if (u3 >= FIELD_TEXTURE_UV_COUNT) u3 = FIELD_TEXTURE_UV_MAX;
    if (v0 >= FIELD_TEXTURE_UV_COUNT) v0 = FIELD_TEXTURE_UV_MAX;
    if (v1 >= FIELD_TEXTURE_UV_COUNT) v1 = FIELD_TEXTURE_UV_MAX;
    if (v2 >= FIELD_TEXTURE_UV_COUNT) v2 = FIELD_TEXTURE_UV_MAX;
    if (v3 >= FIELD_TEXTURE_UV_COUNT) v3 = FIELD_TEXTURE_UV_MAX;

    poly->u0 = u0;
    poly->v0 = v0;
    poly->u1 = u1;
    poly->v1 = v1;
    poly->u2 = u2;
    poly->v2 = v2;
    poly->u3 = u3;
    poly->v3 = v3;
}

extern u_short D_800ADE30[]; // X0,Z0 -> X3,Z3 positions per direction
extern u_char D_800ADE70[];  // U0,V0 -> U3,V3s per direction
extern Quad D_800B0FEC[];

// Best attempt: 77.28% (568 bytes; retail 560, 142 vs 140 insns).
// The body is structurally retail's. The residual is ONE extra strength-reduced
// giv, the block move's LIMIT (pPoly + 0x20 == quad + 0x40). Retail keeps two
// givs, s0 = the Quad and s1 = &quad->polys[0], both +0x70, and recomputes
// "addiu t0,s1,32" inside the loop; our compile gives the limit its own register
// (s1 = quad+0x40, +0x70), bases the vertex stores off it with negative offsets
// and needs a tenth callee-saved slot, so the uv+7 pointer is spilled
// (frame 0x58 vs 0x50).
// Tried, each measured: pQuad->polys[0] inline (77.28), a POLY_FT4* pPoly local
// (77.28, same code), a POLY_FT4* pNextPoly for the copy (77.28), a SVECTOR*
// pVertex for the vertices (77.28), vy=0 before vx, retail's store order (73.31),
// D_800B0FEC[i] indexed (73.51, 668 bytes), pPoly++ in the for-update (rejected:
// POLY_FT4's stride is 0x28, not the quad's 0x70, so that body is wrong code).
// One 4-minute permuter run reached 81.72 by taking &pQuad in the last vertex
// store, which puts pQuad in memory: gcc then stops strength-reducing it and
// derives &polys[0] from the reload instead, so it is not retail either.
//
// Round 426 (redo): the m2c draft and the retail asm agree that retail stores
// vy = 0 FIRST in each vertex group (this body stores it second); that order
// measures 73.31 / 572 bytes / 143 insns. New mechanism fact: gcc re-bases EVERY
// quad-relative store in the loop onto the LAST address it assigns, not onto the
// quad - deleting the copy loop alone makes the base "quad+0x2E", i.e. the
// pPoly->clut store, with all 12 vertex stores as negative displacements off it,
// so the limit giv is not the cause, it is the latest link in one combine_givs
// chain. Retail keeps add_val 0 (the quad) plus a second chain at +0x20, which is
// what this C cannot express: pPoly is folded into "quad + 0x20" and every
// address in the loop then shares one base. Least-bad variant measured: replacing
// the eight inline uv arguments with "u_char* pUV = &D_800ADE70[i * 8];" and
// pUV[0..7] gives 78.04 / frame 0x48 (one saved register fewer, D_800ADE70
// rematerialised per iteration) but loses retail's second ADE70 pointer, so the
// body below (retail-shaped everywhere but the one giv) is the one parked.
/*
void func_8007A5C4(void) {
    int i;
    Quad* pQuad;
    POLY_FT4* pPoly;

    for (i = 0, pQuad = D_800B0FEC; i < 4; i++, pQuad++) {
        pPoly = &pQuad->polys[0];
        SetPolyFT4(pPoly);

        pQuad->vertices[0].vx = D_800ADE30[i * 8 + 0];
        pQuad->vertices[0].vy = 0;
        pQuad->vertices[0].vz = D_800ADE30[i * 8 + 1];
        pQuad->vertices[1].vx = D_800ADE30[i * 8 + 2];
        pQuad->vertices[1].vy = 0;
        pQuad->vertices[1].vz = D_800ADE30[i * 8 + 3];
        pQuad->vertices[2].vx = D_800ADE30[i * 8 + 4];
        pQuad->vertices[2].vy = 0;
        pQuad->vertices[2].vz = D_800ADE30[i * 8 + 5];
        pQuad->vertices[3].vx = D_800ADE30[i * 8 + 6];
        pQuad->vertices[3].vy = 0;
        pQuad->vertices[3].vz = D_800ADE30[i * 8 + 7];

        setRGB0(pPoly, 0x80, 0x80, 0x80);

        FieldClampPolyFT4UVs(pPoly,
            D_800ADE70[i * 8 + 0], D_800ADE70[i * 8 + 1] + 0xC0,
            D_800ADE70[i * 8 + 2], D_800ADE70[i * 8 + 3] + 0xC0,
            D_800ADE70[i * 8 + 4], D_800ADE70[i * 8 + 5] + 0xC0,
            D_800ADE70[i * 8 + 6], D_800ADE70[i * 8 + 7] + 0xC0);

        SetSemiTrans(pPoly, 1);
        pPoly->tpage = GetTPage(0, 2, 0x280, 0x1C0);
        pPoly->clut = GetClut(0x100, 0xF2);

        pPoly[1] = pPoly[0];
    }
}*/
INCLUDE_ASM("asm/field/nonmatchings/main/misc4", func_8007A5C4);


extern SVec4 g_FieldTableQuadVertexXs[]; // X0 -> X3 positions
extern SVec4 g_FieldTableQuadVertexZs[]; // Y0 -> Y3 positions
extern SVec4 g_FieldTableQuadTextureUs[]; // U0 -> U3s
extern SVec4 g_FieldTableQuadTextureVs[]; // V0 -> V3s

// Texture / Clut stuff
extern short g_FieldTableQuadTexParams[];
extern short D_800ADE02[];
extern short D_800ADE04[];
extern short D_800ADE06[];
extern short D_800ADE08[];
extern short D_800ADE0A[];

// Compass primitive initialization?
void FieldInitTableQuad(Quad* pPart, int x, int y, int tex) {
    int nIndexTex;
    POLY_FT4* pNextPoly;
    char _pad[0x88];

    pNextPoly = &pPart->polys[1]; 
    SetPolyFT4(&pPart->polys[0]);
    
    nIndexTex = tex * FIELD_TABLE_QUAD_TEX_PARAM_COUNT,

    // Set Position 0, 1, 2, and 3
    pPart->vertices[0].vx = g_FieldTableQuadVertexXs[x].x;
    pPart->vertices[0].vy = 0x0;
    pPart->vertices[0].vz = g_FieldTableQuadVertexZs[y].x;

    pPart->vertices[1].vx = g_FieldTableQuadVertexXs[x].y;
    pPart->vertices[1].vy = 0;
    pPart->vertices[1].vz = g_FieldTableQuadVertexZs[y].y;

    pPart->vertices[2].vx = g_FieldTableQuadVertexXs[x].z;
    pPart->vertices[2].vy = 0;
    pPart->vertices[2].vz = g_FieldTableQuadVertexZs[y].z;

    pPart->vertices[3].vx = g_FieldTableQuadVertexXs[x].w;
    pPart->vertices[3].vy = 0;
    pPart->vertices[3].vz = g_FieldTableQuadVertexZs[y].w;
    
    setRGB0(&pPart->polys[0], 0x80, 0x80, 0x80);
    
    pPart->polys[0].tpage = GetTPage(
        g_FieldTableQuadTexParams[nIndexTex], 
        D_800ADE02[nIndexTex], 
        D_800ADE04[nIndexTex], 
        D_800ADE06[nIndexTex]
    );
    
    pPart->polys[0].clut = GetClut(D_800ADE08[nIndexTex], D_800ADE0A[nIndexTex]);
    
    FieldClampPolyFT4UVs(&pPart->polys[0], 
        g_FieldTableQuadTextureUs[x].x, g_FieldTableQuadTextureVs[y].x, 
        g_FieldTableQuadTextureUs[x].y, g_FieldTableQuadTextureVs[y].y, 
        g_FieldTableQuadTextureUs[x].z, g_FieldTableQuadTextureVs[y].z, 
        g_FieldTableQuadTextureUs[x].w, g_FieldTableQuadTextureVs[y].w
    );

    *pNextPoly = pPart->polys[0];
}

// Actor primitive initialization
void FieldInitShadowQuad(Quad* pQuad) {
    POLY_FT4* pPoly;
    POLY_FT4* pPoly2;

    pPoly = &pQuad->polys[0];
    pPoly2 = &pQuad->polys[1];
    
    SetPolyFT4(pPoly);
    pQuad->vertices[3].vx = -FIELD_SHADOW_QUAD_HALF_SIZE;
    pQuad->vertices[3].vz = -FIELD_SHADOW_QUAD_HALF_SIZE;
    pQuad->vertices[3].vy = 0;

    pQuad->vertices[2].vx = FIELD_SHADOW_QUAD_HALF_SIZE;
    pQuad->vertices[2].vy = 0;
    pQuad->vertices[2].vz = -FIELD_SHADOW_QUAD_HALF_SIZE;

    pQuad->vertices[1].vx = -FIELD_SHADOW_QUAD_HALF_SIZE;
    pQuad->vertices[1].vy = 0;
    pQuad->vertices[1].vz = FIELD_SHADOW_QUAD_HALF_SIZE;

    pQuad->vertices[0].vx = FIELD_SHADOW_QUAD_HALF_SIZE;
    pQuad->vertices[0].vy = 0;
    pQuad->vertices[0].vz = FIELD_SHADOW_QUAD_HALF_SIZE;

    setRGB0(&pQuad->polys[0], 0x80, 0x80, 0x80);
    pQuad->polys[0].tpage = GetTPage(0, 2, 0x280, 0x1E0);
    pQuad->polys[0].clut = GetClut(0x100, 0xF3);
    SetSemiTrans(&pQuad->polys[0], 1);

    pPoly->v0 = 0xE0;
    pPoly->v1 = 0xE0;
    pPoly->u0 = 0;
    pPoly->u1 = 0xF;
    pPoly->u2 = 0;
    pPoly->v2 = 0xEF;
    pPoly->u3 = 0xF;
    pPoly->v3 = 0xEF;
    
    *pPoly2 = *pPoly;
}

void FieldRenderQuad(u_long* ot, Quad* pQuad, MATRIX* pMatTransform, int renderContext) {
    long nInterpolation;
    long nFlag;
    POLY_FT4* pPoly;

    pPoly = &pQuad->polys[renderContext];
    PushMatrix();

    // Set our GTE transform matrix (RTM and TRV)
    SetRotMatrix(pMatTransform);
    SetTransMatrix(pMatTransform);

    // Transform our vertices to screen coordinates
    RotAverage4(
        &pQuad->vertices[0], &pQuad->vertices[1], &pQuad->vertices[2], &pQuad->vertices[3], 
        (long*)&pPoly->x0, (long*)&pPoly->x1, (long*)&pPoly->x2, (long*)&pPoly->x3, 
        &nInterpolation, &nFlag
    );

    // Queue our primitive for rendering
    addPrim(ot + 1, pPoly);

    PopMatrix();
}

void FieldRenderQuadScreenRect(u_long* ot, Quad* pQuad, MATRIX* pMatTransform, int renderContext) {
    int nInterpolation;
    int nFlag;
    int nPosY;
    int nPosX1;
    POLY_FT4* pPoly;
    int nPosX2;

    pPoly = &pQuad->polys[renderContext];
    PushMatrix();

    // Set our GTE transform matrix (RTM and TRV)
    SetRotMatrix(pMatTransform);
    SetTransMatrix(pMatTransform);

    // Transform our vertices to screen coordinates
    RotAverage4(
        &pQuad->vertices[0], &pQuad->vertices[1], &pQuad->vertices[2], &pQuad->vertices[3], 
        (long*)&pPoly->x0, (long*)&pPoly->x1, (long*)&pPoly->x2, (long*)&pPoly->x3, 
        &nInterpolation, &nFlag
    );
    
    nPosX1 = (pPoly->x3 + pPoly->x2) / 2;
    nPosY = pPoly->y3;

    nPosX2 = nPosX1 + 8;
    nPosX1 = nPosX1 - 8;
    setXY4(pPoly,
        nPosX1, nPosY - 10,
        nPosX2, nPosY - 10,
        nPosX1, nPosY,
        nPosX2, nPosY
    );
    
    // Queue our primitive for rendering
    addPrim(ot + 1, pPoly);

    PopMatrix();
}


// Controller stuff
// --------------------
extern s32 g_pFieldControllerBuffer1;
extern s32 g_pFieldControllerBuffer2;

extern u_short g_FieldMouseSpeedX;
extern u_short g_FieldMouseSpeedY;
extern s32 g_FieldMouseLimitMinX;
extern s32 g_FieldMouseLimitMinY;
extern s32 g_FieldMouseLimitMaxX;
extern s32 g_FieldMouseLimitMaxY;
extern int g_FieldMousePositionsX[2];
extern int g_FieldMousePositionsY[2];

void FieldSetControllerBuffers(void* controllerBuffer1, void* controllerBuffer2) {
    g_pFieldControllerBuffer1 = controllerBuffer1;
    g_pFieldControllerBuffer2 = controllerBuffer2;
}

void FieldSetMouseLimits(int arg0, int arg1, int arg2, int arg3) {
    g_FieldMouseLimitMinX = arg0 * g_FieldMouseSpeedX;
    g_FieldMouseLimitMaxX = arg1 * g_FieldMouseSpeedX;
    g_FieldMouseLimitMinY = arg2 * g_FieldMouseSpeedY;
    g_FieldMouseLimitMaxY = arg3 * g_FieldMouseSpeedY;
}

void FieldSetMouseSpeed(u_short xSpeed, u_short ySpeed) {
    g_FieldMouseSpeedX = xSpeed;
    g_FieldMouseSpeedY = ySpeed;
}

void FieldSetMousePosition(int mouseIndex, int xMovement, int yMovement) {
    g_FieldMousePositionsX[mouseIndex] = xMovement * g_FieldMouseSpeedX;
    g_FieldMousePositionsY[mouseIndex] = yMovement * g_FieldMouseSpeedY;
}

void FieldUpdateMousePosition(int controllerIndex);

int FieldGetMouseState(int controllerIndex, int* pOut) {
    s8** pPad;

    FieldUpdateMousePosition(controllerIndex);
    pOut[0] = g_FieldMousePositionsX[controllerIndex] / g_FieldMouseSpeedX;
    pPad = (s8**)&g_pFieldControllerBuffer1;
    pOut[1] = g_FieldMousePositionsY[controllerIndex] / g_FieldMouseSpeedY;
    pOut[2] = -0x100;
    pOut[3] = ((s8**)&g_pFieldControllerBuffer1)[controllerIndex][4];
    pOut[4] = pPad[controllerIndex][5];
    if (!((s8**)&g_pFieldControllerBuffer1)[controllerIndex][0]
        && ((s8**)&g_pFieldControllerBuffer1)[controllerIndex][1] == FIELD_PAD_TYPE_MOUSE) {
        pOut[2] = ~pPad[controllerIndex][3] & 0xC;
    }
}

void FieldUpdateMousePosition(int controllerIndex) {
    if (((s8**)&g_pFieldControllerBuffer1)[controllerIndex][0] == 0
        && ((s8**)&g_pFieldControllerBuffer1)[controllerIndex][1] == FIELD_PAD_TYPE_MOUSE) {
        g_FieldMousePositionsX[controllerIndex] += ((s8**)&g_pFieldControllerBuffer1)[controllerIndex][4];
        g_FieldMousePositionsY[controllerIndex] += ((s8**)&g_pFieldControllerBuffer1)[controllerIndex][5];
        if (g_FieldMousePositionsX[controllerIndex] > g_FieldMouseLimitMaxX) {
            g_FieldMousePositionsX[controllerIndex] = g_FieldMouseLimitMaxX;
        } else if (g_FieldMousePositionsX[controllerIndex] < g_FieldMouseLimitMinX) {
            g_FieldMousePositionsX[controllerIndex] = g_FieldMouseLimitMinX;
        }
        if (g_FieldMousePositionsY[controllerIndex] > g_FieldMouseLimitMaxY) {
            g_FieldMousePositionsY[controllerIndex] = g_FieldMouseLimitMaxY;
        } else if (g_FieldMousePositionsY[controllerIndex] < g_FieldMouseLimitMinY) {
            g_FieldMousePositionsY[controllerIndex] = g_FieldMouseLimitMinY;
        }
    }
}

// Returns the Y of the point on the plane spanned by pA/pB/pC whose X/Z is pOut's
void FieldGetTrianglePlaneHeight(SVECTOR* pA, SVECTOR* pB, SVECTOR* pC, SVECTOR* pOut, VECTOR* pNormal) {
    VECTOR nB;
    VECTOR nC;
    VECTOR v;

    v.vx = pB->vx - pA->vx;
    v.vy = pB->vy - pA->vy;
    v.vz = pB->vz - pA->vz;
    VectorNormal(&v, &nB);

    v.vx = pC->vx - pA->vx;
    v.vy = pC->vy - pA->vy;
    v.vz = pC->vz - pA->vz;
    VectorNormal(&v, &nC);

    OuterProduct12(&nB, &nC, pNormal);
    if (pNormal->vy == 0) {
        pOut->vy = 0;
    } else {
        pOut->vy = pA->vy + (-(pNormal->vx * (pOut->vx - pA->vx)) - pNormal->vz * (pOut->vz - pA->vz)) / pNormal->vy;
    }
}

INCLUDE_ASM("asm/field/nonmatchings/main/misc4", func_8007B1C4);
/* Best attempt: 95.27% (696 bytes; the original is 692). The loop body, the
   three NCLIP rejection tests (sxy3 n0,n1,nRef / n1,n2,nRef / n2,n0,nRef) and
   the nine halfword copies into v[] are instruction-identical; needs
   psyq/inline_c.h for the 4.x gte_ldsxy3/gte_nclip/gte_stopz spellings, whose
   mtc2 $12/$14/$13 order, double nop and swc2 $24 match retail exactly.
   Remaining deltas, all in the prologue and impossible to reach by guessing
   source shape: (1) gcc copies pVerts a3->t0 for the three header uses, one
   extra instruction; (2) the original frame is 0xA0 with locals at 0x48
   (v[3]), 0x60 (ref) and 0x68/0x6C/0x70 (the three stopz temporaries), i.e. 48
   bytes of locals allocated before v[3] that no source line uses, and it saves
   $s2/$s3/$s4 without ever using them - the retail object came from a source
   with three more call-crossing locals, or is a patched object (splat marks
   this stub "Handwritten function", and so does the jtbl-owning func_8007CD80).
   Passing x/z as s16 instead of int adds an sll/sra pair; u16 vertex components
   turn the eight lh into lhu; pTri[i] indexing (instead of pTri++) is required
   or gcc rebases the reads onto pTri+4.
typedef struct {
    s16 v0;
    s16 v1;
    s16 v2;
    undefined8 pad6[0x8];
} FieldWalkmeshTriIdx; // Size: 0xE

typedef struct {
    s16 vx;
    s16 vy;
    s16 vz;
    u16 pad6;
} FieldWalkmeshVert; // Size: 0x8

extern FieldWalkmeshTriIdx* g_FieldWalkmeshTriangles[];
extern FieldWalkmeshVert* g_FieldWalkmeshVertices[];
extern s32 D_800AFB44[];

// Index of the walkmesh triangle that contains pRef's x/z, 0 when none does
int func_8007B1C4(int x, int z, int nWalkmesh, SVECTOR* pOut, VECTOR* pNormal) {
    SVECTOR v[3];
    SVECTOR ref;
    FieldWalkmeshTriIdx* pTri;
    FieldWalkmeshVert* pVerts;
    u_short* pComp;
    int nCount;
    int nRef;
    int n0;
    int n1;
    int n2;
    long nTemp;
    long nTemp2;
    long nTemp3;
    int i;

    ref.vx = x;
    ref.vy = 0;
    ref.vz = z;
    nRef = (x << 16) + z;
    pTri = g_FieldWalkmeshTriangles[nWalkmesh];
    nCount = D_800AFB44[nWalkmesh];
    pVerts = g_FieldWalkmeshVertices[nWalkmesh];
    pComp = (u_short*)pVerts;

    for (i = 0; i < nCount; i++) {
        n0 = (pVerts[pTri[i].v0].vx << 16) + pVerts[pTri[i].v0].vz;
        n1 = (pVerts[pTri[i].v1].vx << 16) + pVerts[pTri[i].v1].vz;
        n2 = (pVerts[pTri[i].v2].vx << 16) + pVerts[pTri[i].v2].vz;

        gte_ldsxy3(n0, n1, nRef);
        gte_nclip();
        gte_stopz(&nTemp);
        if (nTemp < 0) {
            continue;
        }
        gte_ldsxy3(n1, n2, nRef);
        gte_nclip();
        gte_stopz(&nTemp2);
        if (nTemp2 < 0) {
            continue;
        }
        gte_ldsxy3(n2, n0, nRef);
        gte_nclip();
        gte_stopz(&nTemp3);
        if (nTemp3 < 0) {
            continue;
        }

        v[0].vx = pComp[pTri[i].v0 * 4 + 0];
        v[0].vy = pComp[pTri[i].v0 * 4 + 1];
        v[0].vz = pComp[pTri[i].v0 * 4 + 2];
        v[1].vx = pComp[pTri[i].v1 * 4 + 0];
        v[1].vy = pComp[pTri[i].v1 * 4 + 1];
        v[1].vz = pComp[pTri[i].v1 * 4 + 2];
        v[2].vx = pComp[pTri[i].v2 * 4 + 0];
        v[2].vy = pComp[pTri[i].v2 * 4 + 1];
        v[2].vz = pComp[pTri[i].v2 * 4 + 2];

        FieldGetTrianglePlaneHeight(&v[0], &v[1], &v[2], &ref, pNormal);
        pOut->vx = ref.vx;
        pOut->vy = ref.vy;
        pOut->vz = ref.vz;
        return i;
    }

    pOut->vx = 0;
    pOut->vy = 0;
    pOut->vz = 0;
    pNormal->vx = 0;
    pNormal->vy = 0;
    pNormal->vz = 0;
    return 0;
}
*/

// 1 when the triangle pA/pB/pC winds consistently seen from pRef, -1 when it folds back
int FieldGetTriangleWindingSign(SVECTOR* pA, SVECTOR* pB, SVECTOR* pC, SVECTOR* pRef) {
    VECTOR v;
    VECTOR edge;
    VECTOR cross;

    v.vx = pB->vx - pA->vx;
    v.vy = 0;
    v.vz = pB->vz - pA->vz;
    edge.vx = pRef->vx - pA->vx;
    edge.vy = 0;
    edge.vz = pRef->vz - pA->vz;
    OuterProduct0(&v, &edge, &cross);
    if (cross.vy < 0) {
        return -1;
    }

    v.vx = pC->vx - pB->vx;
    v.vy = 0;
    v.vz = pC->vz - pB->vz;
    edge.vx = pRef->vx - pB->vx;
    edge.vy = 0;
    edge.vz = pRef->vz - pB->vz;
    OuterProduct0(&v, &edge, &cross);
    if (cross.vy >= 0) {
        v.vx = pA->vx - pC->vx;
        v.vy = 0;
        v.vz = pA->vz - pC->vz;
        edge.vx = pRef->vx - pC->vx;
        edge.vy = 0;
        edge.vz = pRef->vz - pC->vz;
        OuterProduct0(&v, &edge, &cross);
        return cross.vy >> 31;
    }
    return -1;
}

extern s16 g_FieldPolarVectorScale;

void FieldGetVec2FromPolar(IVEC3* pVec, int scale, int angle) {
    int nAngle = angle & FIELD_ANGLE_MASK;
    int nScale = (scale << 4) * g_FieldPolarVectorScale >> 12;
    int nScaled;

    pVec->x = rsin(nAngle) * nScale;
    /* dropping nScaled and folding it into the rcos multiply costs the s0/s1/s2 assignment, 97.88% */
    nScaled = nScale;
    nScale = rcos(nAngle);
    pVec->z = -(nScale * nScaled);
    pVec->y = 0;
}

int FieldGetVec2Angle(IVEC3* pVec) {
    return -ratan2(pVec->z, pVec->x) & FIELD_ANGLE_MASK;
}

long FieldGetVec2Magnitude(long, long);

int FieldGetWallEdgeDirection(s16 angle, SVECTOR* pPoints, VECTOR* pOut, s32 nFlags) {
    VECTOR v;
    VECTOR normal;
    int nAngle;
    short nWallAngle;
    short nResult;
    long nMagnitude;

    nAngle = (0xC00 - angle) & FIELD_ANGLE_MASK;
    nWallAngle = -ratan2(pPoints[1].vz - pPoints[0].vz, pPoints[1].vx - pPoints[0].vx);
    nWallAngle &= FIELD_ANGLE_MASK;
    nAngle = (nAngle + nWallAngle) & FIELD_ANGLE_MASK;
    nResult = nWallAngle;
    if ((u_int)(nAngle - 0x80) >= 0xF01) {
        pOut->vx = 0;
        pOut->vy = 0;
        pOut->vz = 0;
        return nWallAngle;
    }
    if (nAngle < 0x800) {
        v.vx = pPoints[0].vx - pPoints[1].vx;
        v.vy = 0;
        v.vz = pPoints[0].vz - pPoints[1].vz;
        nResult = (nWallAngle + 0x800) & FIELD_ANGLE_MASK;
    } else {
        v.vx = pPoints[1].vx - pPoints[0].vx;
        v.vy = 0;
        v.vz = pPoints[1].vz - pPoints[0].vz;
    }
    VectorNormal(&v, &normal);

    nMagnitude = FieldGetVec2Magnitude(pOut->vx >> 12, pOut->vz >> 12);
    pOut->vx = normal.vx * nMagnitude;
    pOut->vy = 0;
    pOut->vz = normal.vz * nMagnitude;

    return nResult;
}
int func_8007C694(VECTOR* pVec, VECTOR* pPos, ActorData* pActor, SVECTOR* pPoints, SVECTOR* pOut, int nMode);
int FieldGetWallEdgeDirection(s16 angle, SVECTOR* pPoints, VECTOR* pOut, s32 nFlags);
extern s32 D_800ADB98;

int FieldProbeWalkmeshAtAngle(VECTOR* pVec, ActorData* pActor, SVECTOR* pPoints, s16 angle) {
    VECTOR v;
    SVECTOR out;
    VECTOR pad[2];  // dead: reproduces the original's 0x78 frame
    int nAngle;

    nAngle = angle & FIELD_ANGLE_MASK;
    v.vx = pVec->vx + rsin(nAngle) * 64;
    v.vz = pVec->vz - rcos(nAngle) * 64;
    if (func_8007C694(&v, &pActor->position, pActor, pPoints, &out, -1) == -1) {
        v.vx = pVec->vx;
        v.vy = pVec->vy;
        v.vz = pVec->vz;
        FieldGetWallEdgeDirection(angle, pPoints, &v, 0);
    } else {
        nAngle = angle - 0x100;
        nAngle &= FIELD_ANGLE_MASK;
        v.vx = pVec->vx + rsin(nAngle) * 64;
        v.vz = pVec->vz - rcos(nAngle) * 64;
        if (func_8007C694(&v, &pActor->position, pActor, pPoints, &out, -1) == -1) {
            v.vx = pVec->vx;
            v.vy = pVec->vy;
            v.vz = pVec->vz;
            FieldGetWallEdgeDirection(angle, pPoints, &v, 0);
        } else {
            nAngle = angle + 0x100;
            nAngle &= FIELD_ANGLE_MASK;
            v.vx = pVec->vx + rsin(nAngle) * 64;
            v.vz = pVec->vz - rcos(nAngle) * 64;
            if (func_8007C694(&v, &pActor->position, pActor, pPoints, &out, -1) == -1) {
                v.vx = pVec->vx;
                v.vy = pVec->vy;
                v.vz = pVec->vz;
                FieldGetWallEdgeDirection(angle, pPoints, &v, 0);
            } else {
                v.vx = pVec->vx;
                v.vy = pVec->vy;
                v.vz = pVec->vz;
            }
        }
    }

    if (func_8007C694(&v, &pActor->position, pActor, pPoints, &out, 0) == -1) {
        return -1;
    }
    if (pActor->scriptFlags.flags & 0x40000) {
        out.vy = *(u_short*)&pActor->scriptYPos;
    } else if ((out.vy << 16) < pActor->position.vy && D_800ADB98 == 0) {
        return -1;
    }
    v.vy = (out.vy << 16) - pActor->position.vy;
    pVec->vx = v.vx;
    pVec->vy = v.vy;
    pVec->vz = v.vz;
    pActor->curYPos = (pActor->position.vy + pVec->vy) >> 16;
    return 0;
}
int FieldProbeWalkmeshSlideAlongWall(VECTOR* pVec, ActorData* pActor, SVECTOR* pPoints, int angle) {
    VECTOR v;
    VECTOR savedV;
    SVECTOR out;
    SVECTOR savedOut;
    VECTOR dir;
    VECTOR normal;
    s32 nFlags;
    s16 nDirection;
    int nPosY;
    long nMagnitude;
    int nAngle;

    nDirection = angle;
    nAngle = nDirection - 0x100;
    nAngle &= 0xFFF;
    v.vx = pVec->vx + rsin(nAngle) * 64;
    v.vz = pVec->vz - rcos(nAngle) * 64;
    if (func_8007BEF4(&v, &pActor->position, pActor, pPoints, &out, -1, &nFlags) == -1) {
        v.vx = pVec->vx;
        v.vy = pVec->vy;
        v.vz = pVec->vz;
        FieldGetWallEdgeDirection(nDirection, pPoints, &v, nFlags);
    } else {
        nAngle = nDirection + 0x100;
        nAngle &= 0xFFF;
        v.vx = pVec->vx + rsin(nAngle) * 64;
        v.vz = pVec->vz - rcos(nAngle) * 64;
        if (func_8007BEF4(&v, &pActor->position, pActor, pPoints, &out, -1, &nFlags) == -1) {
            v.vx = pVec->vx;
            v.vy = pVec->vy;
            v.vz = pVec->vz;
            FieldGetWallEdgeDirection(nDirection, pPoints, &v, nFlags);
        } else {
            nAngle = angle & 0xFFF;
            v.vx = pVec->vx + rsin(nAngle) * 64;
            v.vz = pVec->vz - rcos(nAngle) * 64;
            if (func_8007BEF4(&v, &pActor->position, pActor, pPoints, &out, -1, &nFlags) == -1) {
                v.vx = pVec->vx;
                v.vy = pVec->vy;
                v.vz = pVec->vz;
                FieldGetWallEdgeDirection(nDirection, pPoints, &v, nFlags);
            } else {
                v.vx = pVec->vx;
                v.vy = pVec->vy;
                v.vz = pVec->vz;
            }
        }
    }

    if (func_8007BEF4(&v, &pActor->position, pActor, pPoints, &out, 0, &nFlags) == -1) {
        return -1;
    }

    // Snapshot of the accepted probe, put back when the retry loop gives up
    savedV.vx = v.vx;
    savedV.vy = v.vy;
    savedV.vz = v.vz;
    savedOut.vx = out.vx;
    savedOut.vy = out.vy;
    savedOut.vz = out.vz;
    nPosY = ((s16*)&pActor->position.vy)[1];

    if (out.vy < nPosY) {
    retry:
        dir.vx = -v.vx >> 8;
        dir.vy = ((out.vy << 16) - pActor->position.vy) >> 8;
        dir.vz = -v.vz >> 8;
        VectorNormal(&dir, &normal);
        nMagnitude = FieldGetVec2Magnitude(v.vx >> 8, v.vz >> 8);
        v.vx = -(nMagnitude * normal.vx) >> 4;
        v.vy = (nMagnitude * normal.vy) >> 4;
        v.vz = -(nMagnitude * normal.vz) >> 4;
        if (func_8007BEF4(&v, &pActor->position, pActor, pPoints, &out, 0, &nFlags) == -1) {
            return -1;
        }
        pActor->scriptFlags.flags |= 0x4000000;
        goto storeback;
    } else if ((nFlags & 0x200000) != 0) {
        goto retry;
    } else if ((nFlags & 0x420000) != 0) {
        if ((pActor->curWalkmeshTriMaterial & 0x420000) == 0) {
            goto restore;
        }
        goto retry;
    } else if (out.vy < nPosY + 0x40) {
        goto retry;
    }
restore:
    v.vx = savedV.vx;
    v.vy = savedV.vy;
    v.vz = savedV.vz;
    out.vx = savedOut.vx;
    out.vy = savedOut.vy;
    out.vz = savedOut.vz;
storeback:
    v.vy = (out.vy << 16) - pActor->position.vy;
    pVec->vx = v.vx;
    pVec->vy = v.vy;
    pVec->vz = v.vz;
    pActor->curYPos = (pActor->position.vy + pVec->vy) >> 16;
    return 0;
}

typedef struct {
    /* 0x0 */ s16 v0;
    /* 0x2 */ s16 v1;
    /* 0x4 */ s16 v2;
    /* 0x6 */ s16 n0; // Neighbour across the v0-v1 edge
    /* 0x8 */ s16 n1; // ... across v1-v2
    /* 0xA */ s16 n2; // ... across v2-v0
    /* 0xC */ u_char material;
    /* 0xD */ s8 stepHeight;
} FieldWalkmeshTriangleRecord; // Size: 0xE

typedef struct {
    /* 0x0 */ s16 vx;
    /* 0x2 */ s16 vy;
    /* 0x4 */ s16 vz;
    /* 0x6 */ u_short pad6;
} FieldWalkmeshVertexRecord; // Size: 0x8

extern u_int* g_FieldWalkmeshMaterialFlags;
extern FieldWalkmeshTriangleRecord* g_FieldWalkmeshTriangles[];
extern FieldWalkmeshVertexRecord* g_FieldWalkmeshVertices[];
extern u_char g_FieldPartyMoveInProgress;

// Best attempt: 92.80% (1916 bytes, retail's exact size, 479 insns each). Everything from
// the loop head down is instruction-identical: the loop body, the eight-arm switch and its
// jump table, the three shared arm tails, the material-flag re-read, the three failure
// tests, the loop tail, both FieldGetTrianglePlaneHeight call sites and all three
// crossed-edge copies. Round 498 fixed three things the older notes had wrong:
// 1. nSavedTri was READ BUT NEVER ASSIGNED (UB in the fail switch). `nSavedTri = nTri;` at
//    the loop head restores retail's `sw s0,0x38(sp)`.
// 2. the tail was written inverted (`if (nTri == -1 || i == 0x20) { switch; return -1; }`),
//    which made gcc duplicate the epilogue - 1924 bytes. Written the other way round
//    (`if (nTri != -1 && i != 0x20) { if (nMode != -1) { call } return 0; }`, then the nClip
//    switch, then `return -1` falling into the epilogue) the body hits retail's 1916 bytes.
// 3. the spill-slot order is source-declaration order: declaring nSavedTri, nFlagMask, nPos,
//    i, nStop first gives retail's 0x38/0x40/0x48/0x50/0x58, confirmed by the per-offset
//    access counts 4,4,4,5,3.
// WHAT IS LEFT:
// a. THE FRAME, and it is the whole residual: retail is 0x90, saves at 0x68..0x8C, stack
//    args read at 0xA0/0xA4/0xA8; this body is 0x88, saves 0x60..0x84, args 0x98/0x9C/0xA0.
//    The 8 bytes are a SIXTH reload spill at 0x60..0x67 that nothing in retail's .s reads
//    (its only sp accesses are 0x10,0x18,0x28,0x30,0x38,0x40,0x48,0x50,0x58), so the slot
//    is allocated and never used. An expand-time local cannot produce it: it lands at 0x28
//    and pushes all five real spills +8. Every save/restore line, both stack-arg reads and
//    most branch displacements follow from that one slot.
// b. prologue: retail keeps the full word of pPos->vz (`lw a3,8(pPos)` then `sra a3`) for
//    nPos's z half because that load is shared with the (pPos->vz + pVec->vz) sum. With
//    nPos written first - needed for the x half's `sra/sll` and worth 2 insns, i.e. the
//    1916-byte shape - cc1 narrows the z half to `lh 10(pPos)`. Writing nPos last scores
//    93.14% but is 1924 bytes, so it could never link; the exact-size shape was kept.
// Tried and measured this round: nPos last (93.14, 1924); the older declaration order
// (92.77, same 1916). Not tried: the permuter - the lane rule gates it at 99% and no random
// edit can add a stack slot.
/*
int func_8007BEF4(VECTOR* pVec, VECTOR* pPos, ActorData* pActor, SVECTOR* pPoints, SVECTOR* pOut, int nMode, s32* pFlags) {
    FieldWalkmeshTriangleRecord* pTri;
    FieldWalkmeshVertexRecord* pVerts;
    VECTOR normal;
    s32 nSavedTri;
    s32 nFlagMask;
    s32 nPos;
    s32 i;
    s32 nStop;
    s32 nRef;
    s32 nClip;
    u_int nFlags;
    s32 nTri;
    s32 vA;
    s32 vB;
    s32 vC;

    nTri = pActor->walkmeshTriIds[pActor->walkmeshId];
    pTri = g_FieldWalkmeshTriangles[pActor->walkmeshId];
    pVerts = g_FieldWalkmeshVertices[pActor->walkmeshId];
    if (nTri == -1) {
        return -1;
    }

    nPos = ((pPos->vx >> 16) << 16) + (pPos->vz >> 16);
    pOut->vx = (pPos->vx + pVec->vx) >> 16;
    nRef = (pOut->vx << 16) + ((pPos->vz + pVec->vz) >> 16);
    nFlagMask = 0;
    pOut->vy = 0;
    pOut->vz = (pPos->vz + pVec->vz) >> 16;
    if (!((pActor->flags >> (pActor->walkmeshId + 3)) & 1)) {
        nFlagMask = -(g_FieldPartyMoveInProgress < 1);
    }

    nFlags = g_FieldWalkmeshMaterialFlags[pTri[nTri].material] & nFlagMask;
    nStop = 0;
    if ((nFlags & 0x400000) || nMode == 0x80) {
        nStop = 1;
    }

    for (i = 0; i < 0x20; i++) {
        nSavedTri = nTri;
        vA = (pVerts[pTri[nTri].v0].vx << 16) + pVerts[pTri[nTri].v0].vz;
        vB = (pVerts[pTri[nTri].v1].vx << 16) + pVerts[pTri[nTri].v1].vz;
        vC = (pVerts[pTri[nTri].v2].vx << 16) + pVerts[pTri[nTri].v2].vz;

        nClip = NormalClip(vA, vB, nRef) < 0;
        if (NormalClip(vB, vC, nRef) < 0) {
            nClip |= 2;
        }
        if (NormalClip(vC, vA, nRef) < 0) {
            nClip |= 4;
        }

        switch (nClip) {
        case 0:
            i = 0xFF;
            break;
        case 1:
            nTri = pTri[nTri].n0;
            break;
        case 2:
            nTri = pTri[nTri].n1;
            break;
        case 3:
            if (NormalClip(vB, nRef, nPos) < 0) {
                nTri = pTri[nTri].n0;
                nClip = 1;
            } else {
                nTri = pTri[nTri].n1;
                nClip = 2;
            }
            break;
        case 4:
            nTri = pTri[nTri].n2;
            break;
        case 5:
            if (NormalClip(vA, nRef, nPos) < 0) {
                nTri = pTri[nTri].n2;
                nClip = 4;
            } else {
                nTri = pTri[nTri].n0;
                nClip = 1;
            }
            break;
        case 6:
            if (NormalClip(vC, nRef, nPos) >= 0) {
                nTri = pTri[nTri].n2;
                nClip = 4;
            } else {
                nTri = pTri[nTri].n1;
                nClip = 2;
            }
            break;
        case 7:
            nTri = -1;
            break;
        }

        nFlags = g_FieldWalkmeshMaterialFlags[pTri[nTri].material] & nFlagMask;
        *pFlags = nFlags;
        if ((((pActor->scriptFlags.flags >> 8) & 7) & (nFlags >> 5)) != 0) {
            nTri = -1;
            break;
        }
        if ((nFlags & 0x800000) && pActor->walkmeshId == 0) {
            nTri = -1;
            break;
        }
        if (nFlags & 0x400000) {
            if (!nStop) {
                FieldGetTrianglePlaneHeight(&pVerts[pTri[nTri].v0], &pVerts[pTri[nTri].v1], &pVerts[pTri[nTri].v2], pOut, &normal);
                if (pOut->vy < ((s16*)&pPos->vy)[1]) {
                    nTri = -1;
                    break;
                }
            }
        }
        if (nTri == -1) {
            break;
        }
    }

    if (nTri != -1 && i != 0x20) {
        if (nMode != -1) {
            FieldGetTrianglePlaneHeight(&pVerts[pTri[nTri].v0], &pVerts[pTri[nTri].v1], &pVerts[pTri[nTri].v2], pOut, &normal);
        }
        return 0;
    }

    switch (nClip) {
    case 1:
        pPoints[0].vx = pVerts[pTri[nSavedTri].v0].vx;
        pPoints[0].vy = pVerts[pTri[nSavedTri].v0].vy;
        pPoints[0].vz = pVerts[pTri[nSavedTri].v0].vz;
        pPoints[1].vx = pVerts[pTri[nSavedTri].v1].vx;
        pPoints[1].vy = pVerts[pTri[nSavedTri].v1].vy;
        pPoints[1].vz = pVerts[pTri[nSavedTri].v1].vz;
        break;
    case 2:
        pPoints[0].vx = pVerts[pTri[nSavedTri].v1].vx;
        pPoints[0].vy = pVerts[pTri[nSavedTri].v1].vy;
        pPoints[0].vz = pVerts[pTri[nSavedTri].v1].vz;
        pPoints[1].vx = pVerts[pTri[nSavedTri].v2].vx;
        pPoints[1].vy = pVerts[pTri[nSavedTri].v2].vy;
        pPoints[1].vz = pVerts[pTri[nSavedTri].v2].vz;
        break;
    case 4:
        pPoints[0].vx = pVerts[pTri[nSavedTri].v2].vx;
        pPoints[0].vy = pVerts[pTri[nSavedTri].v2].vy;
        pPoints[0].vz = pVerts[pTri[nSavedTri].v2].vz;
        pPoints[1].vx = pVerts[pTri[nSavedTri].v0].vx;
        pPoints[1].vy = pVerts[pTri[nSavedTri].v0].vy;
        pPoints[1].vz = pVerts[pTri[nSavedTri].v0].vz;
        break;
    }
    return -1;
}*/
INCLUDE_ASM("asm/field/nonmatchings/main/misc4", func_8007BEF4);



int FieldSetRangeByDelta(int* pCur, int* pNext, int n) {
    int value = *pCur;

    if (n < 0) {
        *pCur = value;
        *pNext = value;
        return value;
    }
    *pCur = value;
    value += n;
    *pNext = value;
    return value;
}

// Best attempt: 91.87% (1684 bytes; the original is 1704, five instructions short, 421 vs 426).
//    The frame now matches retail exactly: 0x80 with pActor 0x28, nSavedTri 0x30,
//    nFlagMask 0x38, nPos 0x40, i 0x48 and the working slot s? 0x50, so all twenty
//    save/restore offsets, the prologue and the epilogue agree. That came from
//    declaring the locals in retail's slot order (nSavedTri, nFlagMask, nPos, i) and
//    from one extra spilled local (nWalkmeshId at 0x50) - retail's frame is 8 bytes
//    bigger than its five visible spilled locals need, so some sixth value was
//    spilled there and never read. The earlier note's 0x78 frame and its 'i stays in
//    a register' claim were both stale.
//    Case 6 is now the right way round (NormalClip < 0 takes n1/nMask 2 and falls
//    through, else n2/nMask 4 - retail bgez's to the n2 arm) and the prologue has
//    retail's instruction set including the two reloads of pPos->vz/pVec->vz for the
//    pRef->vz store. What is left, all measured:
//    1. prologue order: retail loads pPos->vx, pVec->vx, pPos->vz, pVec->vz, then
//       pRef, then forms nRef from those registers and computes nPos from the SAME
//       live words (sra a1,a1,16 / sll a1,a1,16) where this body emits lh 2(pPos)
//       because gcc rematerialises the narrowed load. This body computes both sums
//       before the stores and interleaves them.
//    2. the two nFlags/scriptFlags tests: retail keeps the constant on the
//       scriptFlags side (srl v0,a1,9 / andi v0,v0,3 / and v0,v0,v1); gcc here
//       reassociates so the mask lands on the nFlags side. Swapping the C operands
//       only moves the mask to the other side (155 differing lines, 91.81).
//    3. the loop break: retail's flag tests jump to a shared block that sets
//       nTri = -1 and falls into the i == 0x20 test, while gcc here merges every
//       exit into one block at the end and beq's to it.
//    4. retail's stack scratch is t2 (dead pRef); here it is t1 (dead pVec), so every
//       spill reload picks a different register (8 lines).
//    Tried and measured: nX/nZ locals so pRef->vx is not reloaded as lh (92.34%,
//    1664 bytes, but the pRef->vz reload disappears and the frame has to come from
//    nWalkmeshId alone); case 6 swapped the other way (n1 arm first: bltz, ~1% worse);
//    nPos moved above the pRef->vy store (no change to the lh narrowing).
//    Direct indexing with no pointer locals, as before: a pCurTri/pVert pointer pair
//    costs ~30%.
// Best attempt: 92.61% (1696 bytes; retail is 1704 = two instructions more, 424 vs 426).
//    This is the HONEST spelling of the body: the 91.87% note above got its frame from an
//    uninitialised local (nWalkmeshId read in the 0x800000 test), which is UB. With
//    pActor->walkmeshId in that test the body scores 92.00, and with the nMode tail
//    written as two returns (retail has TWO copies of 'j epilogue / move v0,zero')
//    it reaches 92.61 - so the phantom slot was worth nothing. Everything the note
//    above lists as 'fixed' still holds; the loop body, the switch table use, the
//    plane-height call and the crossed-edge copies are instruction-identical.
//    WHAT IS LEFT, measured on this body:
//    1. THE FRAME, and it is arithmetic, not guesswork: vars = frame(0x80) - args(0x18)
//       - saves(0x28) = 0x40 = VECTOR normal (0x10) + SIX 8-byte reload spill slots
//       (retail spills at 0x28,0x30,0x38,0x40,0x48 plus an unread 0x50). This body has
//       FIVE (vars = 0x38, frame 0x78, saves at 0x50..0x74), so every save offset, both
//       incoming-stack-arg reads (0x90/0x94 vs 0x88/0x8C) and the prologue/epilogue
//       differ. An EXPAND-era dead array cannot fill that slot: expand-era slots are
//       handed out BELOW every reload spill, so a dead u_long[2] lands at 0x20 and
//       pushes the five spills +8 (86.66, measured last round). A sixth RELOAD spill
//       would need a store+load pair, and 'grep -o '[0-9A-F]*(sp)' of retail's .s shows
//       nothing at all at 0x50. Treat this as unrequestable from source.
//    2. two instructions: retail's early exit is 'bne s0,-1,body' plus an out-of-line
//       'j epilogue / li v0,-1', cc1 emits 'beq s0,-1,<epilogue>'; and in the flag test
//       retail's 'beqz v0,fail' reuses the 'li v0,-1' in its delay slot for the
//       following 'beq s0,v0', where this body re-loads it.
//    3. nPos: retail narrows neither half (full-word 'sra 16 / sll 16' off the loads
//       taken before the pRef stores); this body reloads 'lh 2(pPos) / lh 10(pPos)'
//       because the pRef->vx store kills the loaded words.
//    4. the '& 3' / '& 7' mask sits on the scriptFlags side in retail and on the
//       nFlags side here (same instruction count, 2 lines).
//    Keep: case 6 must be written '< 0 -> n1 else n2' (retail's bgez) and the nMode
//    tail as 'if (nMode == -1) return 0; ... return 0;'.
//
// Best attempt: 93.64% (1692 bytes; retail is 1704, three instructions more, 423 vs 426 insns).
//    The prologue is now retail's shape: four word loads (pPos->vx, pVec->vx, pPos->vz, pVec->vz) before any store, nRef formed from them, and a fresh reload of the vz pair for pRef->vz. What bought it, measured in order on this body: reading pPos->vx and pPos->vz into the posX/posZ locals BEFORE the pRef stores (92.61 -> 93.64) removes cc1's lh 2(pPos) / lh 10(pPos) narrowing of the nPos halves, because the two uses no longer share one load; writing the nPos statement first without the locals is 92.73.
//    KEEP: case 6 as NormalClip(vC,nRef,nPos) < 0 -> n1 else n2 (retail bgez's to the n2 arm), the nMode tail as if (nMode == -1) return 0; ... return 0;.
//    WHAT IS LEFT, all allocator or scheduler:
//    1. frame 0x78 against retail's 0x80, and it is the whole save block plus both incoming stack-arg reads. Retail's var area is 0x18 normal + SIX slots 8 apart (0x28..0x50) and NOTHING in the .s reads or writes 0x50, so the sixth is a phantom reload slot. Decl locals cannot reach it: they pack below the reload spills (a dead u_long[2] measured 86.66 in an earlier round), so treat the frame as unrequestable from source.
//    2. sched2 (on in this preset, see levers h-sched2) places the pRef->vy store and the vz reload pair differently from retail inside the same statement list, so the prologue order is the scheduler's choice, not the source's.
//    3. the flag test: retail masks the FLAGS side (srl a1,9 / andi 3 / and with nFlags>>3); cc1 folds the mask onto the nFlags side (same instruction count, 4 lines). Swapping the C operands only mirrors it (91.81 on the older body).
//    4. the fail path: retail beqz's to the shared .L8007C710 block (j .L8007CA30 / addiu s0,-1, placed near the top of the function); cc1 emits bnez past it plus an inline j / li s0,-1 copy.
//    5. the early exit: retail is bne s0,-1 over an out-of-line j epilogue / li v0,-1; cc1 threads the branch straight to the tail's li v0,-1.
//    Measured dead in earlier rounds on the same body: nX/nZ locals instead of posX/posZ (92.34), case 6 swapped (bltz, about 1% worse), nPos moved above the pRef->vy store, direct indexing with no pointer locals for pTri/pVerts (about 30% worse), a dead array for the phantom frame slot (86.66), the uninitialised-local frame trick (91.87, and it is UB - do not ship it).
/*
int func_8007C694(VECTOR* pVec, VECTOR* pPos, ActorData* pActor, SVECTOR* pPoints, SVECTOR* pRef, int nMode) {
    FieldWalkmeshTriangleRecord* pTri;
    FieldWalkmeshVertexRecord* pVerts;
    VECTOR normal;
    s32 nSavedTri;
    s32 nFlagMask;
    s32 nPos;
    s32 i;
    s32 nRef;
    s32 nTri;
    s32 nMask;
    s32 vA;
    s32 vB;
    s32 vC;
    s32 posX;
    s32 posZ;
    u_int nFlags;

    nTri = pActor->walkmeshTriIds[pActor->walkmeshId];
    pTri = g_FieldWalkmeshTriangles[pActor->walkmeshId];
    pVerts = g_FieldWalkmeshVertices[pActor->walkmeshId];
    if (nTri == -1) {
        return -1;
    }

    posX = pPos->vx;
    posZ = pPos->vz;
    pRef->vx = (posX + pVec->vx) >> 16;
    nRef = (pRef->vx << 16) + ((posZ + pVec->vz) >> 16);
    pRef->vy = 0;
    pRef->vz = (pPos->vz + pVec->vz) >> 16;
    nPos = ((posX >> 16) << 16) + (posZ >> 16);

    nFlagMask = 0;
    if (!((pActor->flags >> (pActor->walkmeshId + 3)) & 1)) {
        nFlagMask = -(g_FieldPartyMoveInProgress < 1);
    }

    for (i = 0; i < 0x20; i++) {
        nSavedTri = nTri;
        vA = (pVerts[pTri[nTri].v0].vx << 16) + pVerts[pTri[nTri].v0].vz;
        vB = (pVerts[pTri[nTri].v1].vx << 16) + pVerts[pTri[nTri].v1].vz;
        vC = (pVerts[pTri[nTri].v2].vx << 16) + pVerts[pTri[nTri].v2].vz;

        nMask = NormalClip(vA, vB, nRef) < 0;
        if (NormalClip(vB, vC, nRef) < 0) {
            nMask |= 2;
        }
        if (NormalClip(vC, vA, nRef) < 0) {
            nMask |= 4;
        }

        switch (nMask) {
        case 0:
            i = 0xFF;
            break;
        case 1:
            nTri = pTri[nTri].n0;
            break;
        case 2:
            nTri = pTri[nTri].n1;
            break;
        case 3:
            if (NormalClip(vB, nRef, nPos) < 0) {
                nTri = pTri[nTri].n0;
                nMask = 1;
            } else {
                nTri = pTri[nTri].n1;
                nMask = 2;
            }
            break;
        case 4:
            nTri = pTri[nTri].n2;
            break;
        case 5:
            if (NormalClip(vA, nRef, nPos) < 0) {
                nTri = pTri[nTri].n2;
                nMask = 4;
            } else {
                nTri = pTri[nTri].n0;
                nMask = 1;
            }
            break;
        case 6:
            if (NormalClip(vC, nRef, nPos) < 0) {
                nTri = pTri[nTri].n1;
                nMask = 2;
            } else {
                nTri = pTri[nTri].n2;
                nMask = 4;
            }
            break;
        case 7:
            nTri = -1;
            break;
        }

        nFlags = g_FieldWalkmeshMaterialFlags[pTri[nTri].material] & nFlagMask;
        if ((((pActor->scriptFlags.flags >> 9) & 3) & (nFlags >> 3)) != 0
            || (((pActor->scriptFlags.flags >> 8) & 7) & (nFlags >> 5)) != 0
            || ((nFlags & 0x800000) != 0 && pActor->walkmeshId == 0)) {
            nTri = -1;
            break;
        }
        if (nTri == -1) {
            break;
        }
    }

    if (nTri != -1 && i != 0x20) {
        if (nMode == -1) {
            return 0;
        }
        FieldGetTrianglePlaneHeight(&pVerts[pTri[nTri].v0], &pVerts[pTri[nTri].v1], &pVerts[pTri[nTri].v2], pRef, &normal);
        return 0;
    }

    switch (nMask) {
    case 1:
        pPoints[0].vx = pVerts[pTri[nSavedTri].v0].vx;
        pPoints[0].vy = pVerts[pTri[nSavedTri].v0].vy;
        pPoints[0].vz = pVerts[pTri[nSavedTri].v0].vz;
        pPoints[1].vx = pVerts[pTri[nSavedTri].v1].vx;
        pPoints[1].vy = pVerts[pTri[nSavedTri].v1].vy;
        pPoints[1].vz = pVerts[pTri[nSavedTri].v1].vz;
        break;
    case 2:
        pPoints[0].vx = pVerts[pTri[nSavedTri].v1].vx;
        pPoints[0].vy = pVerts[pTri[nSavedTri].v1].vy;
        pPoints[0].vz = pVerts[pTri[nSavedTri].v1].vz;
        pPoints[1].vx = pVerts[pTri[nSavedTri].v2].vx;
        pPoints[1].vy = pVerts[pTri[nSavedTri].v2].vy;
        pPoints[1].vz = pVerts[pTri[nSavedTri].v2].vz;
        break;
    case 4:
        pPoints[0].vx = pVerts[pTri[nSavedTri].v2].vx;
        pPoints[0].vy = pVerts[pTri[nSavedTri].v2].vy;
        pPoints[0].vz = pVerts[pTri[nSavedTri].v2].vz;
        pPoints[1].vx = pVerts[pTri[nSavedTri].v0].vx;
        pPoints[1].vy = pVerts[pTri[nSavedTri].v0].vy;
        pPoints[1].vz = pVerts[pTri[nSavedTri].v0].vz;
        break;
    }
    return -1;
}*/
INCLUDE_ASM("asm/field/nonmatchings/main/misc4", func_8007C694);





extern int g_FieldScratchpadOffset;

// Pointer into the scratchpad area for the script VM
int FieldAllocScratchpad(int size) {
    int offset = g_FieldScratchpadOffset;

    g_FieldScratchpadOffset = offset + size;
    return FIELD_SCRATCHPAD_BASE + offset * 4;
}

void FieldFreeScratchpad(int size) {
    g_FieldScratchpadOffset -= size;
}

INCLUDE_ASM("asm/field/nonmatchings/main/misc4", func_8007CD80);

INCLUDE_ASM("asm/field/nonmatchings/main/misc4", func_8007D3D4);
/* Best attempt: 86.66%, 1092 bytes = retail's EXACT size, and every block is in
   retail's order: the three NCLIP tests, the eight-arm switch (each arm's
   neighbour field, cases 3/5/6 calling NormalClip(vB|vA|vC, nRef, nPos)), the
   loop tail's `i == 0x20` exhaustion test, the plane-height call, the shared
   0x7FFFFFFF "not steppable" arm reached from both material-flag copies, and the
   single `*pY = nY; FieldSetRangeByDelta(pY, pYMax, nStep);` join whose store
   retail keeps in the jal's delay slot.
   WHAT IS MEASURED AND STILL WRONG:
   1. frame 0x78 against retail's 0x80: retail has an 8-byte local slot at
      0x50..0x57 that NOTHING in the function references (the whole .s touches
      0x18,0x1A,0x1C,0x20,0x28,0x30,0x38,0x40,0x48,0x90,0x94 and no other sp
      offset), i.e. the original source held one more in-memory local than the
      code uses - the sibling func_8007BEF4/func_8007C694 parks carry the same
      8-byte gap. A dead SCALAR leftover (`nSavedTri = nTri;`) allocates no slot
      (gcc keeps it in a register and drops the copy), but levers.md
      j-an-unused-local-array-is says an ARRAY local takes its slot at expand
      time: a dead 8-byte array (or a second SVECTOR) is the next thing to try.
   2. `(pActor->position.vx >> 16) << 16` comes out as `lh 0x22(a0); sll 16`
      (combine narrows it) where retail keeps the full word loaded for the x sum
      and does `sra 16; sll 16`. Identical residual to func_8007BEF4's note.
   3. prologue: retail issues all four position/moveModified loads before the
      first store to `out`; here gcc stores out.vx first, and a store to an
      addressable local blocks the CSE of the later pos.vz/mod.vz loads.
   4. the third moveModified test compiles to `beqz` where retail's three tests
      are all `bnez` (arm layout only, same instruction count).
   NOT to be undone: case 6 must be written `< 0 -> n1 else n2` (that is the
   polarity retail's `bgez n2` branch and its n1-before-n2 block order show);
   writing it `>= 0 -> n2 else n1` inverts the branch and swaps the two shared
   neighbour blocks. An earlier body that duplicated `*pY = out.vy;
   FieldSetRangeByDelta(...)` in both arms measured 89.29% but only 1080 bytes
   (12 short of retail, so it could never link) because gcc merged the two
   copies; the nY-local shape above is the one that reproduces retail's tail.
   Permuter: 4 min, base=2598 best=2353 - the only real edit was hoisting the
   pVerts load above nTri, which measured 86.16% / 1096 bytes on port (worse);
   the rest of the candidate was `if (1) {}` noise.
   Round 487, item 1 measured (unparked, re-scored 86.66% / 1092 bytes, 160
   differing lines on the shape below): retail's frame is 0x80 = vars 0x40
   (eight 8-byte slots at 0x18..0x57) + the ten saves at 0x58..0x7C, and the
   slots are out 0x18, nSetIndex 0x20, pY 0x28, pNormal 0x30, pVerts 0x38,
   nFlagMask 0x40, nPos 0x48, then 0x50..0x57 with no access at all; the 5th and
   6th parameters are read at 0x90/0x94, i.e. frame 0x80 + 0x10. A dead
   `u_long unusedFrame[2]` does close the frame to 0x80 and puts the saves and
   the 0x90/0x94 argument reads on retail's exact offsets, but an expand-time
   object takes the FIRST var slot: the array lands at 0x20 and pushes all six
   spill slots +8 (still 86.66%, 160 differing lines). So the phantom has to be
   a reload-era spill slot ABOVE the six spills, which levers.md
   j-an-expand-time-slot-is-always-the-lowest says is not source-reachable from
   a body of this shape - the same conclusion j-a-small-aggregate-mid-list has.
   Round 487, item 3 measured: moving `nPos = ...` above `nFlagMask = 0;` (retail
   computes nPos's two halves right after nRef but stores it after out.vy) is
   86.59%, and it does not restore retail's full-word `sra 16 / sll 16` for the x
   half - our compile still reloads it narrow as `lh 0x22(fp)`, because the store
   to out.vx has already killed the loaded word.
   Round 500: the body had been parked WITHOUT its 's32 nSavedTri;' line, so it
   could not compile and the 86.66% above came from a stale object. Re-measured
   with the declaration: 86.66% / 1092 bytes / 273 insns (retail 273 insns /
   1092 bytes - the byte count IS retail's), 178 differing lines. The dead
   `u_long unusedFrame[2]` was also tried as the FIRST declaration, not only as a
   mid-list one: it lands at 0x20 in either position (an expand-time object takes
   the lowest slot regardless of declaration order), which does close the frame to
   retail's 0x80 but pushes a1/a2/a3 from 0x20/0x28/0x30 to 0x28/0x30/0x38;
   differing lines 178 -> 160, score still 86.66%. So the phantom slot at 0x50 is
   a reload-era slot above the six spills on both positions, as the two levers
   above say. Not re-run: the still-undiagnosed bulk of the 160 lines (items 2 and
   3: the narrow `lh 0x22(fp)` x half and the prologue load/store interleave). */
/*
// Height of the walkmesh set nSetIndex under the point the actor's next step
// reaches: walks the triangle graph from the actor's current triangle until the
// point is inside one, then reports that triangle, the plane height there and
// the range the actor may be lifted through
s32 func_8007D3D4(ActorData* pActor, s32 nSetIndex, s32* pY, VECTOR* pNormal, s16* pTriIndex, s32* pYMax) {
    FieldWalkmeshTriangleRecord* pTri;
    FieldWalkmeshVertexRecord* pVerts;
    SVECTOR out;
    s32 nRef;
    s32 nFlagMask;
    s32 nPos;
    s32 nClip;
    s32 nTri;
    s32 vA;
    s32 vB;
    s32 vC;
    s32 nSavedTri;
    s32 nStep;
    s32 nY;
    s32 i;

    nTri = pActor->walkmeshTriIds[nSetIndex];
    pTri = g_FieldWalkmeshTriangles[nSetIndex];
    pVerts = g_FieldWalkmeshVertices[nSetIndex];
    if (nTri == -1) {
        return -1;
    }

    out.vx = (pActor->position.vx + pActor->moveModified.vx) >> 16;
    nRef = (out.vx << 16) + ((pActor->position.vz + pActor->moveModified.vz) >> 16);
    nFlagMask = 0;
    out.vy = 0;
    nPos = ((pActor->position.vx >> 16) << 16) + (pActor->position.vz >> 16);
    out.vz = (pActor->position.vz + pActor->moveModified.vz) >> 16;
    if (!((pActor->flags >> (nSetIndex + 3)) & 1)) {
        nFlagMask = -(g_FieldPartyMoveInProgress < 1);
    }

    for (i = 0; i < 0x20; i++) {
        nSavedTri = nTri;
        vA = (pVerts[pTri[nTri].v0].vx << 16) + pVerts[pTri[nTri].v0].vz;
        vB = (pVerts[pTri[nTri].v1].vx << 16) + pVerts[pTri[nTri].v1].vz;
        vC = (pVerts[pTri[nTri].v2].vx << 16) + pVerts[pTri[nTri].v2].vz;

        nClip = NormalClip(vA, vB, nRef) < 0;
        if (NormalClip(vB, vC, nRef) < 0) {
            nClip |= 2;
        }
        if (NormalClip(vC, vA, nRef) < 0) {
            nClip |= 4;
        }

        switch (nClip) {
        case 0:
            i = 0xFF;
            break;
        case 1:
            nTri = pTri[nTri].n0;
            break;
        case 2:
            nTri = pTri[nTri].n1;
            break;
        case 3:
            if (NormalClip(vB, nRef, nPos) < 0) {
                nTri = pTri[nTri].n0;
            } else {
                nTri = pTri[nTri].n1;
            }
            break;
        case 4:
            nTri = pTri[nTri].n2;
            break;
        case 5:
            if (NormalClip(vA, nRef, nPos) < 0) {
                nTri = pTri[nTri].n2;
            } else {
                nTri = pTri[nTri].n0;
            }
            break;
        case 6:
            if (NormalClip(vC, nRef, nPos) < 0) {
                nTri = pTri[nTri].n1;
            } else {
                nTri = pTri[nTri].n2;
            }
            break;
        case 7:
            nTri = -1;
            break;
        }
        if (nTri == -1) {
            return -1;
        }
    }
    if (i == 0x20) {
        return -1;
    }

    FieldGetTrianglePlaneHeight(&pVerts[pTri[nTri].v0], &pVerts[pTri[nTri].v1], &pVerts[pTri[nTri].v2], &out, pNormal);
    *pTriIndex = nTri;
    nStep = pTri[nTri].stepHeight * 4;
    if (nStep < 0) {
        nStep = 0;
    }
    if (pActor->walkmeshId != nSetIndex) {
        if ((g_FieldWalkmeshMaterialFlags[pTri[nTri].material] & 0x800000 & nFlagMask) != 0) {
            *pY = 0x7FFFFFFF;
            *pYMax = 0x7FFFFFFF;
            return 0;
        }
        nY = out.vy;
    } else if ((g_FieldWalkmeshMaterialFlags[pTri[nTri].material] & 0x800000 & nFlagMask) != 0) {
        *pY = 0x7FFFFFFF;
        *pYMax = 0x7FFFFFFF;
        return 0;
    } else if (pActor->moveModified.vx != 0 || pActor->moveModified.vy != 0 || pActor->moveModified.vz != 0) {
        nY = (s16)pActor->curYPos;
    } else {
        nY = out.vy;
    }
    *pY = nY;
    FieldSetRangeByDelta(pY, pYMax, nStep);
    return 0;
}

*/

int FieldGetDominantComponent(int x, int y, int z);

void FieldNormalizeVecByDominantAxis(VECTOR* pVec, VECTOR* pOut) {
    int nDominantAxis;

    nDominantAxis = FieldGetDominantComponent(pVec->vx, pVec->vy, pVec->vz);
    pVec->vx >>= 12;
    pVec->vy >>= 12;
    pVec->vz >>= 12;
    if (nDominantAxis < 0) {
        pVec->vx = -pVec->vx;
        pVec->vy = -pVec->vy;
        pVec->vz = -pVec->vz;
    }
    VectorNormal(pVec, pOut);
}

// Returns whichever component has the largest absolute value
int FieldGetDominantComponent(int x, int y, int z) {
    int nAbsX = x;
    int nAbsY = y;
    int nAbsZ = z;

    if (nAbsX < 0) {
        nAbsX = -nAbsX;
    }
    if (nAbsY < 0) {
        nAbsY = -nAbsY;
    }
    if (nAbsZ < 0) {
        nAbsZ = -nAbsZ;
    }

    if (nAbsX >= nAbsY && nAbsX >= nAbsZ) {
        return x;
    }
    if (nAbsY >= nAbsX && nAbsY >= nAbsZ) {
        return y;
    }
    if (nAbsZ >= nAbsX && nAbsZ >= nAbsY) {
        return z;
    }
    return 0;
}




