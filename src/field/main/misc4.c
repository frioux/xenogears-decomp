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

// FieldOrVramStripPixels converts one strip of 16-bit VRAM pixels, read two at a
// time through a u_int word
#define FIELD_SCREEN_STRIP_WIDTH 0x100
#define FIELD_SCREEN_STRIP_HEIGHT 0x20
#define FIELD_SCREEN_STRIP_WORD_COUNT (FIELD_SCREEN_STRIP_WIDTH * FIELD_SCREEN_STRIP_HEIGHT / 2)
#define FIELD_SCREEN_STRIP_SIZE (FIELD_SCREEN_STRIP_WORD_COUNT * 4)

// Number of switchable draw/display environments in g_FieldRenderContexts
#define FIELD_RENDER_CONTEXT_COUNT 2

// Texture UVs are byte-sized: FieldClampPolyFT4UVs pins each of them below
// FIELD_TEXTURE_UV_COUNT
#define FIELD_TEXTURE_UV_MAX 0xFF
#define FIELD_TEXTURE_UV_COUNT 0x100

// Angles are a 4096-step turn counter; every sum of angles is wrapped back into it
#define FIELD_ANGLE_MASK 0xFFF

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
/* Best attempt: 90.68% (144 bytes; the original is 148). The original keeps its
   return value -1 in v0 across the ArchiveDataSync call and the three guard
   tests, so the call's result is copied to v1 (`addu v1,v0,zero`), the -1 is
   re-materialised in that branch's delay slot and the three later tests read
   into v1 with nop delay slots. Our compile tests v0 directly and re-emits
   `li v0,-1` in each of the three branch delay slots, i.e. it is exactly the
   `addu v1,v0,zero` (4 bytes) that is missing; every shape below folds the
   constant instead. Tried: five independent `return -1`s, a `result = -1;`
   before the call with `return result`/`goto out`, nested if/else + single
   nResult (this one, 90.68), `nResult = -1` initialised at declaration with a
   single out label (71.76, 152 bytes), and one permuter run (4 min, 705 iters)
   whose best candidate is 84.49% under objdiff - the permuter's own score is
   not objdiff's, so it picked a body that is worse here.
extern int ArchiveDataSync(void);
extern s32 g_FieldArchiveStreamActive;
extern s32 g_FieldScreenImageStored;
extern s32 g_FieldSpecialAnimLoadPending;
extern s32 g_FieldPartyModelLoadState;

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

extern s16 D_800B2344;
void FieldOrVramStripPixels(void) {
    RECT rect;
    int i;
    u_int* pImage;

    if (D_800B2344 != 0) {
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
extern s32 D_800AFD04;
extern s32 g_FieldScriptDataStreamActive;
extern s32 D_800ADC14;
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

INCLUDE_ASM("asm/field/nonmatchings/main/misc4", func_80078D44);
/* Best attempt: 97.50% (1344 bytes; the original is 1348). Every call, loop and
   branch is in retail's order; what is left is one allocator decision and one
   block shape.
   1. Retail keeps nFade in $s0 and the loop counter in $s1; all our shapes give
      nFade $s1 and i $s0, because gcc CSEs the -0x40000 decrement into a saved
      register ($s0) here, while retail rematerialises it (lui $v0,0xfffc;
      addu $s0,$s0,$v0) inside the g_FieldScriptDataStreamActive loop and only parks it in a saved
      register ($s1, then $s3) for the later loops. Declaration orders tried:
      rect/i/nFadeStep/nFade, nFade first, nFadeStep first; all four give the
      same allocation.
   2. nFadeStep: retail is [lbu $v0,g_GameDiscChangeRequest][beq -> end][move $s2,zero]
      [lbu $v0,g_FieldDiscChangeFade][bne -> end][li $s2,0x20][move $s2,zero], i.e. a
      store of 0, then 0x20, then 0 again on the Y==1 path. The "0x20 unless
      either flag is 1" spelling (body below) reaches the same instruction
      multiset but fills the delay slots the other way round; the
      `nFadeStep = 0; if (X != 1) { nFadeStep = 0x20; if (Y == 1) { ... } }`
      spellings all make gcc fold the two constants into a setcc
      (xori/sltu/sll $s2,$v0,0x5) and come out 4-8 bytes short.
   Also open: the rect stores in the second MoveImage block are [w][y][x][h]
   here and [w][y][x][h] in retail but with the two zero stores swapped.
   One permuter run (4 min, 913 iters) reported PERMUTER_BETTER 1553 -> 1042 for
   a body that duplicates the fade block under `if (i)`, but that body scores
   94.16% under objdiff (1332 bytes), so its metric is not objdiff's here.
void func_80078D44(void) {
    // Loads the scene map, fades the field in and runs it until the state machine
    // leaves state 1, then tears the field down

        RECT rect;
        int i;
        int nFadeStep;
        int nFade;

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
            rect.w = 0x140;
            rect.h = 0xE0;
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
        D_800AFD04 = 1;
        if (g_FieldActorModelSlotCount != 0) {
            LibGearSetDirectKeyframes(1);
        }
        nFadeStep = 0x20;
        if (g_GameDiscChangeRequest == 1 || g_FieldDiscChangeFade == 1) {
            nFadeStep = 0;
        }
        nFade = 0x800000;
        if (g_FieldScriptDataStreamActive == 1) {
            do {
                FieldClearAndSwapOTag();
                FieldZoomFadeEffectUpdate();
                FieldDisplay();
                if (g_FieldDiscChangeFade != 1) {
                    if (g_FieldZoomFadeScale < 0x22C0) {
                        g_FieldZoomFadeScale += nFadeStep;
                    }
                } else {
                    FieldZoomFadeEffectSetPolygonColor(nFade >> 16);
                    nFade -= 0x40000;
                    if (nFade < 0) {
                        nFade = 0;
                    }
                }
            } while (ArchiveDataSync() != 0);
            DrawSync(0);
            HeapFree((void*)D_800ADC14);
            g_FieldScriptDataStreamActive = 0;
            FieldOrVramStripPixels();
        }
        if (g_FieldDiscChangeFade == 1) {
            do {
                FieldClearAndSwapOTag();
                FieldZoomFadeEffectUpdate();
                FieldDisplay();
                FieldZoomFadeEffectSetPolygonColor(nFade >> 16);
                nFade -= 0x40000;
            } while (nFade >= 0);
        }
        if (g_GameDiscChangeRequest == 1) {
            rect.x = 0;
            rect.y = 0;
            rect.w = 0x140;
            rect.h = 0xE0;
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
            g_GameRequestedWdsBankIndex = 0xE;
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
                for (i = 0; i < 0x20; i++) {
                    FieldUpdateFrame();
                    FieldZoomFadeEffectUpdate();
                    func_8007554C();
                    FieldTickSkipCounter();
                    if (g_GameDiscChangeRequest != 1) {
                        FieldZoomFadeEffectSetPolygonColor(nFade >> 16);
                        nFade -= 0x40000;
                        if (nFade < 0) {
                            nFade = 0;
                        }
                        if (g_FieldZoomFadeScale < 0x22C0) {
                            g_FieldZoomFadeScale += nFadeStep;
                        }
                    }
                }
            } else {
                FieldFadeToBlack(0x20);
            }
        } else {
            for (i = 0; i < 8; i++) {
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
        D_800AFD04 = 0;
    }
*/

INCLUDE_ASM("asm/field/nonmatchings/main/misc4", func_80079288);
/* Best attempt: 79.46% (the form below scores 78.37%, 728 bytes against the
   original's 684). splat splits this block per word, so the original's
   relocations name D_800B2298 (period), D_800B229C (count) and D_800B22A0
   (values) separately while reading all three through one base register
   (s1 = &D_800B2298; count at -4(a0), values at 0(a0) for a0 = s1 + 8, i.e.
   combine_givs re-basing three symbols onto one pointer). Remaining deltas:
   that rebase, and rand()'s product landing in a2/a3 instead of s1/a2.
extern s32 g_FieldMapChangeGate;
extern s32 g_FieldEntryStateGate;
extern s32 g_FieldMapTransitionGate;
extern s32 g_FieldArchiveStreamActive;
extern u_char D_800ADB04;
extern s32 D_800ADBD0;
extern s32 g_FieldSuppressStateChange;
extern s32 g_FieldRandomSkinSlots;   // countdown to the next regeneration of the slot table
extern s32 D_800B2298;   // regeneration period, 0 disables the random skin
extern s32 D_800B229C;   // number of slot table entries in use
extern u16 D_800B22A0[]; // slot table; 0xFFFF marks a free slot
extern u16 D_800B2270[]; // per-skin value handed to the field system
extern s16 D_800B2290;
extern u_char D_80065ADC[]; // per-skin selection weight
extern u_char g_GameSelectedSkinIndex;
extern u_char g_GameSelectedSkinLoadPending;
extern void FieldRandomizeSkinSlots(void);
extern void LoadGameStateOverlay(int);
extern void func_80281204(int);
extern int rand(void);

// Ages the slot table and, once a slot frees up, rolls a skin against
// D_80065ADC's weights and switches the party to it.
void func_80079288(void) {
    int aWeightSums[16];
    int nTotal;
    int nTarget;
    int i;
    int nSum;
    s32* pPeriod;
    s32* pCount;
    u16* pSlots;
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
    pPeriod = &D_800B2298;
    if (*pPeriod == 0) {
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
    pSlots = (u16*)&pPeriod[2];
    for (i = 0; i < pPeriod[1]; i++) {
        if (pSlots[i] != 0xFFFF) {
            pSlots[i] = pSlots[i] - 1;
        }
    }
    pCount = &D_800B229C;
    pSlots = (u16*)&pCount[1];
    for (i = 0; i < *pCount; i++) {
        if (pSlots[i] != 0) {
            continue;
        }
        D_800B22A0[i] = 0xFFFF;
        pWeight = D_80065ADC;
        nTotal = 0;
        for (i = 0; i < 16; i++) {
            nTotal += pWeight[i];
        }
        nSum = 0;
        for (i = 0; i < 16; i++) {
            aWeightSums[i] = nSum;
            nSum += pWeight[i];
        }
        nTarget = (rand() * (nTotal + 1)) >> 15;
        nSum = 0;
        for (i = 15; i >= 0; i--) {
            if (pWeight[i] != 0 && aWeightSums[i] < nTarget) {
                nSum++;
                break;
            }
        }
        if (nSum == 0) {
            return;
        }
        g_GameSelectedSkinIndex = i;
        g_GameSelectedSkinLoadPending = 0;
        D_800B2290 = D_800B2270[i];
        if (g_FieldSuppressStateChange == 0) {
            LoadGameStateOverlay(2);
        }
        g_FieldMapChangeGate = 0;
        D_800ADBD0 = 1;
        if (g_FieldSystemMode == 0) {
            func_80281204(i);
        }
        break;
    }
}
*/

extern s32 g_FieldInitialWdsBankIndex;
extern s32 g_FieldSuppressStateChange;
extern s16 g_GameWdsEntryReleased;
extern s32 g_FieldNextGameState;
extern void FieldScriptSyncMemoryWithGameState(void);
extern void SoundFreeLoadedWdsAndAudioManager(void);
extern void FieldBeginWdsFileLoad(void);
extern void FieldFinishWdsFileLoad(void);
extern void func_8001BB50(void);
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
        g_pGameState->unk2320 = nSaved;
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
        g_pGameState->unk2320 = nSaved;
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
            func_8001BB50();
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

INCLUDE_ASM("asm/field/nonmatchings/main/misc4", func_8007A5C4);
/* Best attempt: 77.28% (568 vs 560 bytes): vx/vz interleave reproduced, but gcc
   strength-reduces &polys[1] (the copy at the end of the body) into an induction
   register and addresses polys[0] off it with negative offsets, spilling the
   uv[7] pointer to the stack: 10 callee-saved slots vs the original 9.
   Round 43: spelling the whole body as D_800B0FEC[i].vertices[k] (indexed access,
   no pointer local) is far worse - 73.51%, 668 bytes, because gcc then reaches
   every field through `lui $at; addu $at,$at,$s0; sh` off a folded HI16 addend.
   The pointer form below is the right shape; the extra biv is what costs the
   register.
extern u_short D_800ADE30[]; // X0,Z0 -> X3,Z3 positions per direction
extern u_char D_800ADE70[];  // U0,V0 -> U3,V3s per direction
extern Quad D_800B0FEC[];

void func_8007A5C4(void) {
    int i;
    Quad* pQuad;

    for (i = 0, pQuad = D_800B0FEC; i < 4; i++, pQuad++) {
        SetPolyFT4(&pQuad->polys[0]);

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

        setRGB0(&pQuad->polys[0], 0x80, 0x80, 0x80);

        FieldClampPolyFT4UVs(&pQuad->polys[0],
            D_800ADE70[i * 8 + 0], D_800ADE70[i * 8 + 1] + 0xC0,
            D_800ADE70[i * 8 + 2], D_800ADE70[i * 8 + 3] + 0xC0,
            D_800ADE70[i * 8 + 4], D_800ADE70[i * 8 + 5] + 0xC0,
            D_800ADE70[i * 8 + 6], D_800ADE70[i * 8 + 7] + 0xC0);

        SetSemiTrans(&pQuad->polys[0], 1);
        pQuad->polys[0].tpage = GetTPage(0, 2, 0x280, 0x1C0);
        pQuad->polys[0].clut = GetClut(0x100, 0xF2);

        pQuad->polys[1] = pQuad->polys[0];
    }
}
   Previous note: 75.64% (568 vs 560 bytes). Indexing the flat arrays by symbol
   reproduces the six middle entries and the first/last pointers correctly.
*/


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

INCLUDE_ASM("asm/field/nonmatchings/main/misc4", func_8007B6C4);
/* Best attempt: 84.11% but the exact 332-byte size (earlier attempts were 336
   bytes, which can never link). Seeding nResult with nWallAngle before the range
   test is the only form that puts `move s3,a0` in the bnez delay slot and
   `addiu a0,a0,0x800` in the beqz delay slot like the original; the remaining
   delta is one extra copy of nWallAngle (the original has `move v1,a0` feeding
   both the angle sum and the early `return nWallAngle`).
long FieldGetVec2Magnitude(long, long);

int func_8007B6C4(s16 angle, SVECTOR* pPoints, VECTOR* pOut) {
    VECTOR v;
    VECTOR normal;
    int nAngle;
    int nWallAngle;
    long nResult;
    long nMagnitude;

    nAngle = (0xC00 - angle) & 0xFFF;
    nWallAngle = -ratan2(pPoints[1].vz - pPoints[0].vz, pPoints[1].vx - pPoints[0].vx) & 0xFFF;
    nAngle = (nAngle + nWallAngle) & 0xFFF;
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
        nResult = (nWallAngle + 0x800) & 0xFFF;
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
*/

int func_8007C694(VECTOR* pVec, VECTOR* pPos, ActorData* pActor, SVECTOR* pPoints, SVECTOR* pOut, int nMode);
int func_8007B6C4(s16 angle, SVECTOR* pPoints, VECTOR* pOut);
extern s32 D_800ADB98;

INCLUDE_ASM("asm/field/nonmatchings/main/misc4", func_8007B814);
/* Best attempt: 96.33% (684 bytes, same size as the original). Two differences:
   1. gcc hoists the tail's first `lw v0,0(pVec)` out of the shared func_8007B6C4
      block into the block that jumps there, so its `j` target is 4 bytes later.
   2. the probe angles: the original computes (angle -/+ 0x100) into s0 itself
      (`addiu s0,s2,-0x100` / `andi s0,s0,0xfff`), gcc computes the subtraction
      into v0 and only the mask into s0. Note the `sra s2,v0,16` at the top of the
      else branch is not an assignment from the probe result: the bne delay slot
      is `sll v0,s2,16`, so it is the sign extension of `angle` for `angle-0x100`,
      and both objects agree on it. Prototypes below are the ones this attempt
      needs; D_800ADB98 = 0x800ADB98 is in symbol_addrs.field.txt.
// Probes the walkmesh around `angle` and drops pVec onto the floor it finds there
int func_8007B814(VECTOR* pVec, ActorData* pActor, SVECTOR* pPoints, s16 angle) {
    VECTOR v;
    SVECTOR out;
    VECTOR pad[2];
    int nAngle;

    nAngle = angle & 0xFFF;
    v.vx = pVec->vx + rsin(nAngle) * 64;
    v.vz = pVec->vz - rcos(nAngle) * 64;
    if (func_8007C694(&v, &pActor->position, pActor, pPoints, &out, -1) == -1) {
        v.vx = pVec->vx;
        v.vy = pVec->vy;
        v.vz = pVec->vz;
        func_8007B6C4(angle, pPoints, &v);
    } else {
        nAngle = (angle - 0x100) & 0xFFF;
        v.vx = pVec->vx + rsin(nAngle) * 64;
        v.vz = pVec->vz - rcos(nAngle) * 64;
        if (func_8007C694(&v, &pActor->position, pActor, pPoints, &out, -1) == -1) {
            v.vx = pVec->vx;
            v.vy = pVec->vy;
            v.vz = pVec->vz;
            func_8007B6C4(angle, pPoints, &v);
        } else {
            nAngle = (angle + 0x100) & 0xFFF;
            v.vx = pVec->vx + rsin(nAngle) * 64;
            v.vz = pVec->vz - rcos(nAngle) * 64;
            if (func_8007C694(&v, &pActor->position, pActor, pPoints, &out, -1) == -1) {
                v.vx = pVec->vx;
                v.vy = pVec->vy;
                v.vz = pVec->vz;
                func_8007B6C4(angle, pPoints, &v);
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
        out.vy = *(u_short*)&pActor->unkEC;
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
*/

INCLUDE_ASM("asm/field/nonmatchings/main/misc4", func_8007BAC0);
/* Best attempt: 76.42% (1036 vs 1032 bytes). The probe sequence, the second
   func_8007BEF4 call and the tail (v.vy, pVec stores, curYPos) are
   instruction-identical; four deltas remain:
   1. the shared fallback (copy *pVec, func_8007B6C4) loses retail's leading
      dead `lw a3,0x70(sp)`, so every later label sits 4 bytes low. Retail's
      call passes nFlags as a 4th argument that func_8007B6C4 never reads;
      spelling it needs a 4-argument prototype of func_8007B6C4, which the
      file already declares with three.
   2. the retry dispatch is laid out inverted: retail puts the recompute body
      FIRST (cond1 inverts to the rest of the chain, which sits after the body
      and jumps back to it) and the restore last, while the || chain below
      compiles body-last. Duplicating the body per arm so cross-jumping can
      merge it does not work: gcc keeps all four copies (1264 bytes, 62.19%).
   3. (nDirection - 0x100) & 0xFFF must be two statements, else gcc masks into
      the local through v0 instead of computing into s0.
   4. the restore copies sit in temporaries (t0..t4) instead of r-tail regs,
      i.e. downstream of 1 and 2.
   nPosY is retail's `lh 0x26(pActor)`, the high half of position.vy, which no
   field of ActorData names.
int func_8007BEF4(VECTOR* pVec, VECTOR* pPos, ActorData* pActor, SVECTOR* pPoints, SVECTOR* pOut, int nMode, s32* pFlags);

int func_8007BAC0(VECTOR* pVec, ActorData* pActor, SVECTOR* pPoints, int angle) {
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
        func_8007B6C4(nDirection, pPoints, &v);
    } else {
        nAngle = nDirection + 0x100;
        nAngle &= 0xFFF;
        v.vx = pVec->vx + rsin(nAngle) * 64;
        v.vz = pVec->vz - rcos(nAngle) * 64;
        if (func_8007BEF4(&v, &pActor->position, pActor, pPoints, &out, -1, &nFlags) == -1) {
            v.vx = pVec->vx;
            v.vy = pVec->vy;
            v.vz = pVec->vz;
            func_8007B6C4(nDirection, pPoints, &v);
        } else {
            nAngle = angle & 0xFFF;
            v.vx = pVec->vx + rsin(nAngle) * 64;
            v.vz = pVec->vz - rcos(nAngle) * 64;
            if (func_8007BEF4(&v, &pActor->position, pActor, pPoints, &out, -1, &nFlags) == -1) {
                v.vx = pVec->vx;
                v.vy = pVec->vy;
                v.vz = pVec->vz;
                func_8007B6C4(nDirection, pPoints, &v);
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

    // Keep the probe's result so a retry that fails leaves it untouched
    savedV.vx = v.vx;
    savedV.vy = v.vy;
    savedV.vz = v.vz;
    savedOut.vx = out.vx;
    savedOut.vy = out.vy;
    savedOut.vz = out.vz;
    nPosY = ((s16*)&pActor->position.vy)[1];

    if (out.vy < nPosY
        || (nFlags & 0x200000) != 0
        || ((nFlags & 0x420000) != 0 && (pActor->curWalkmeshTriMaterial & 0x420000) != 0)
        || out.vy < nPosY + 0x40) {
        dir.vx = -v.vx >> 8;
        dir.vz = -v.vz >> 8;
        dir.vy = ((out.vy << 16) - pActor->position.vy) >> 8;
        VectorNormal(&dir, &normal);
        nMagnitude = FieldGetVec2Magnitude(v.vx >> 8, v.vz >> 8);
        v.vx = -(normal.vx * nMagnitude) >> 4;
        v.vy = (normal.vy * nMagnitude) >> 4;
        v.vz = -(normal.vz * nMagnitude) >> 4;
        if (func_8007BEF4(&v, &pActor->position, pActor, pPoints, &out, 0, &nFlags) == -1) {
            return -1;
        }
        pActor->scriptFlags.flags |= 0x4000000;
    } else {
        v.vx = savedV.vx;
        v.vy = savedV.vy;
        v.vz = savedV.vz;
        out.vx = savedOut.vx;
        out.vy = savedOut.vy;
        out.vz = savedOut.vz;
    }

    v.vy = (out.vy << 16) - pActor->position.vy;
    pVec->vx = v.vx;
    pVec->vy = v.vy;
    pVec->vz = v.vz;
    pActor->curYPos = (pActor->position.vy + pVec->vy) >> 16;
    return 0;
}
*/

typedef struct {
    /* 0x0 */ s16 v0;
    /* 0x2 */ s16 v1;
    /* 0x4 */ s16 v2;
    /* 0x6 */ s16 n0; // Neighbour across the v0-v1 edge
    /* 0x8 */ s16 n1; // ... across v1-v2
    /* 0xA */ s16 n2; // ... across v2-v0
    /* 0xC */ u_char material;
    /* 0xD */ u_char stepHeight;
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
extern u_char D_800B21CC;

INCLUDE_ASM("asm/field/nonmatchings/main/misc4", func_8007BEF4);
/* Best attempt: 83.66% (1916 bytes, the original's exact size). The body at
   retail's size and every block is in retail's order (the eight-arm switch, the
   flag tests, the 0x400000 height probe and the crossed-edge copy / second
   switch). What is left is ordering and allocation, all of it measured:
   1. prologue: retail loads pVec->vx/pVec->vz/pPos->vx/pPos->vz once, forms
      nRef from those registers (`addu s8,v1,a0` right after the sra of the z
      sum) and reloads pVec->vz/pPos->vz only for the pOut->vz store; gcc
      splits the loads into two clusters and forms nRef after the second.
   2. `(pPos->vx >> 16) << 16` is narrowed by gcc to `lh 2(pPos); sll 16` where
      retail does `sra/sll` on the full word (nPos, 0x48(sp)).
   3. nSavedTri stays in a register here: the frame is 0x88 against retail's
      0x90, and retail keeps nSavedTri 0x38, nPos 0x48, nStop 0x58, i 0x50
      while this body keeps nPos 0x38, nStop 0x48, i 0x50. Forcing the spill
      would fix every saved-register offset in the function.
   4. case 6 written as `if (NormalClip(...) >= 0) {n2; nClip=4} else {n1;
      nClip=2}` (this body) matches retail's bgez; the swapped form scores
      84.01% but is 1912 bytes, so the 4-byte-short position is the one inside
      that arm.
   5. i = 0xFF in case 0 is retail's "triangle found" sentinel: the loop tail
      tests `i == 0x20` for "exhausted", and the exit tests nTri == -1 || i ==
      0x20.
   Six of the seven arguments are known from the call sites; pOut is arg 5
   (0xA0(sp)), nMode arg 6 (0xA4(sp), 0x80 and -1 are the two values tested) and
   pFlags arg 7 (0xA8(sp)), which is where `*pFlags = nFlags` stores. */
/*
// Drops pOut onto the walkmesh under the actor: walks the triangle graph from
// the actor's current triangle toward pOut's x/z, then fills pPoints with the
// two vertices of the edge it crossed when no triangle is found. -1 on failure.
int func_8007BEF4(VECTOR* pVec, VECTOR* pPos, ActorData* pActor, SVECTOR* pPoints, SVECTOR* pOut, int nMode, s32* pFlags) {
    FieldWalkmeshTriangleRecord* pTri;
    FieldWalkmeshVertexRecord* pVerts;
    VECTOR normal;
    s32 nRef;
    s32 nPos;
    s32 nFlagMask;
    s32 nClip;
    u_int nFlags;
    s32 nStop;
    s32 nTri;
    s32 vA;
    s32 vB;
    s32 vC;
    s32 nSavedTri;
    s32 i;

    nTri = pActor->walkmeshTriIds[pActor->walkmeshId];
    pTri = g_FieldWalkmeshTriangles[pActor->walkmeshId];
    pVerts = g_FieldWalkmeshVertices[pActor->walkmeshId];
    if (nTri == -1) {
        return -1;
    }

    pOut->vx = (pPos->vx + pVec->vx) >> 16;
    nRef = (pOut->vx << 16) + ((pPos->vz + pVec->vz) >> 16);
    nFlagMask = 0;
    pOut->vy = 0;
    pOut->vz = (pPos->vz + pVec->vz) >> 16;
    nPos = ((pPos->vx >> 16) << 16) + (pPos->vz >> 16);
    if (!((pActor->flags >> (pActor->walkmeshId + 3)) & 1)) {
        nFlagMask = -(D_800B21CC < 1);
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

    if (nTri == -1 || i == 0x20) {
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
    }

    if (nMode != -1) {
        FieldGetTrianglePlaneHeight(&pVerts[pTri[nTri].v0], &pVerts[pTri[nTri].v1], &pVerts[pTri[nTri].v2], pOut, &normal);
    }
    return 0;
}
*/

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

INCLUDE_ASM("asm/field/nonmatchings/main/misc4", func_8007C694);
/* Best attempt: 92.29% (1696 bytes; the original is 1704, so two instructions
   are missing). Every call, all eight jump-table arms, the material/flag test
   and the edge-copy dispatch are in retail's order. What is left:
   1. the frame is 0x78 here against retail's 0x80: retail has one more 4-byte
      local (it keeps pActor 0x28, nSavedTri 0x30, nFlagMask 0x38, nPos 0x40 and
      the loop counter i 0x48 in memory); here `i` stays in a register, so both
      the frame and every saved-register offset sit 8 bytes low.
   2. the prologue: retail computes nRef from the sra'd register value
      (`sh v1,0(t2); sll v1,v1,0x10; ... addu s8,v1,a0`) and reloads
      pPos->vz/pVec->vz for the pRef->vz store, while this body reloads
      pRef->vx (`lh a0,0(t2)`) for nRef and computes the vz store first, and it
      stores pRef->vy before pRef->vx.  Swapping nRef above the pRef->vz store
      was tried: 91.33% (1672 bytes), worse.
   3. the `nTri == -1` early exit is inverted: retail bne's into the body and
      falls into its own `return -1`, gcc here merges all three `return -1`s
      into one block at the end and beq's to it.
   4. cases 3/5/6 compute vB/nRef/nPos into a0/a1/a2 with `nop` delay slots
      where retail has the argument moves there (a0 = s3, a1 = s2, a2 = fp).
   Written with direct indexing (pTri[nTri].v0, pVerts[...].v0) and NO pointer
   locals: a pCurTri/pVert pointer pair cost 30% (51.58%, 1244 bytes), because
   gcc then re-bases the induction register onto the verts and spills both the
   tri table and pPoints. That is the single biggest lever in this function.
   The vertex fields must be signed (s16) for the packing reads, which emit `lh`;
   the edge copies read the same records as u_short and emit `lhu`, so retail's
   source had an unsigned view for the copies and a signed one for the math.
   Six argument call, verified against func_8007B814's site: (pVec, pPos, pActor, pPoints SVECTOR[2] out,
   pRef SVECTOR out of (x,0,z), nMode) with
   nMode == -1 meaning "stop when the triangle is found".
   func_8007CD80 is the same walk with a 3-argument front end (it finds the first
   triangle with func_8007B1C4 and clamps to g_Scene.unk4C..unk52); it inlines
   the three NCLIPs with gte_ldsxy3/gte_nclip/gte_stopz into one stack word
   instead of calling NormalClip. Not attempted. */
/*
// Walks the actor's walkmesh toward pVec, then drops pRef onto the floor of the
// triangle it stopped in. -1 with pPoints holding the edge that was crossed.
int func_8007C694(VECTOR* pVec, VECTOR* pPos, ActorData* pActor, SVECTOR* pPoints, SVECTOR* pRef, int nMode) {
    FieldWalkmeshTriangleRecord* pTri;
    FieldWalkmeshVertexRecord* pVerts;
    VECTOR normal;
    s32 nRef;
    s32 nPos;
    s32 nMask;
    s32 nFlagMask;
    s32 nTri;
    s32 nSavedTri;
    s32 vA;
    s32 vB;
    s32 vC;
    u_int nFlags;
    s32 i;

    nTri = pActor->walkmeshTriIds[pActor->walkmeshId];
    pTri = g_FieldWalkmeshTriangles[pActor->walkmeshId];
    pVerts = g_FieldWalkmeshVertices[pActor->walkmeshId];
    if (nTri == -1) {
        return -1;
    }

    pRef->vx = (pPos->vx + pVec->vx) >> 16;
    pRef->vy = 0;
    pRef->vz = (pPos->vz + pVec->vz) >> 16;
    nRef = (pRef->vx << 16) + ((pPos->vz + pVec->vz) >> 16);
    nPos = ((pPos->vx >> 16) << 16) + (pPos->vz >> 16);

    nFlagMask = 0;
    if (!((pActor->flags >> (pActor->walkmeshId + 3)) & 1)) {
        nFlagMask = -(D_800B21CC < 1);
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
            if (NormalClip(vC, nRef, nPos) >= 0) {
                nTri = pTri[nTri].n2;
                nMask = 4;
            } else {
                nTri = pTri[nTri].n1;
                nMask = 2;
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
        if (nMode != -1) {
            FieldGetTrianglePlaneHeight(&pVerts[pTri[nTri].v0], &pVerts[pTri[nTri].v1], &pVerts[pTri[nTri].v2], pRef, &normal);
        }
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
}
*/

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




