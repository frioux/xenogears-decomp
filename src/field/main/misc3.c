#include "common.h"
#include "system/memory.h"
#include "field/main.h"
#include "field/actor.h"
#include "field/camera.h"
#include "field/effects.h"
#include "main/game.h"
#include "psyq/libgpu.h"
#include "psyq/libgte.h"
#include "system/model.h"

extern void ArchiveCdDataSync(int mode);


// Best attempt, 98.45% at 668 bytes against retail's 672 (ONE instruction short,
// so it can never link). Structure, every call and the whole instruction
// multiset match; the residual is one scheduler placement. Levers that DID land,
// and are in the parked body:
//  - an empty `do { ... } while (0);` around each of the three trailing ambient
//    read+store statements: sched1 otherwise hoists the third `lhu` into the
//    second one's load-delay slot with a fresh $v1 (89.37 -> 95.24, and that
//    whole block then matches retail instruction for instruction).
//  - an empty `do { ... } while (0);` around the two 0x14-byte light copies:
//    without it sched1 floats the 22-instruction copy block up into the third
//    light's `lhu` load-delay slot (95.24 -> 98.45).
// Give each light its own
// pointer local (`pLight1 = &pLights[1]`) and use it for BOTH the `vx` store and
// the func_80030A30 argument - that is what makes the store `sw $v0,332($s1)`
// and the argument `addu` instead of a rematerialised `lui $at` + `sw
// %lo(g_Scene+332)` (92.40 -> 93.66). Facts: the three 0x14-byte lights sit
// directly after g_Scene at +0x138/+0x14C/+0x160 (g_WorldScale is g_Scene+0x134)
// and the ambient colours at +0x174/+0x176/+0x178; the camera vectors retail
// reaches as `addiu $a1,$s1,-0x110` are g_CameraEye/At/Up at g_Scene-0x110,
// -0x100, -0xF0, and plain &g_CameraEye gives a separate lui/addiu pair that
// cannot match, so the casts off &g_Scene stay.
typedef struct {
    s32 vx;
    s32 vy;
    s32 vz;
    s16 r;
    s16 g;
    s16 b;
    s16 pad12;
} FieldLight; // Size: 0x14

extern VECTOR g_CameraEye;
extern VECTOR g_CameraAt;
extern VECTOR g_CameraUp;
extern void func_80030A30(int index, FieldLight* pLight);
extern void func_80030B14(MATRIX* pMatrix);

// .bss
extern s32 D_800AFACC;
extern s32 D_800AFAD0;
extern s16 D_800AFAD4;
extern s16 D_800AFAD6;
extern s16 D_800AFAD8;
extern s32 D_800AFAE0;
extern s32 D_800AFAE4;
extern s16 D_800AFAE8;
extern s16 D_800AFAEA;
extern s16 D_800AFAEC;
extern s32 D_800AFAF4;
extern s32 D_800AFAF8;
extern s16 D_800AFAFC;
extern s16 D_800AFAFE;
extern s16 D_800AFB00;
extern s16 D_800AFB04;
extern s16 D_800AFB06;
extern s16 D_800AFB08;

// Round 471: 98.45% / 668 bytes, ONE instruction short of retail's 672. The two
// residuals round 394 left are both closed (see the lever list above), and the
// remaining row is a three-way contest between the third light's `pData++` skip,
// the call's `li a0,0x2` and `addiu a1,s1,352`, over one load-delay slot and the
// jal slot. Retail: `lhu v0,0(s0) / nop / sll / sh` then the copies then
// `jal / addiu s0,s0,4`, with `li a0,0x2` + `addiu a1,s1,352` hoisted to the head
// of the light's block and the copies in $v0/$v1/$a2/$a3. The parked body has the
// copies in retail's place but pays for it: the skip's `addiu s0,s0,4` fills the
// `lhu` load-delay slot (retail's `nop`), which pushes `li a0,0x2` into the jal
// slot and frees $a0, so the copies land in $v0/$v1/$a0/$a2. Measured this round,
// all 668 bytes / 167 insns and all still short by one, i.e. each variant just
// moves the missing slot around: the skip after the copy block (natural source
// order, no barrier) 95.24 at 43 differing lines - the copies hoist and the skip
// lands in the jal slot as retail has it; skip after the barrier block 97.26;
// skip as the barrier block's last statement 97.26; skip as its first statement
// 97.26 (there `addiu a1,s1,352` takes the load slot and `li a0,0x2` the jal
// slot). build/src/field/main/misc3.c.i compiled with the unit cc1 plus
// -fno-schedule-insns (and
// -fno-schedule-insns2 on top) is retail's exact stream INCLUDING the load-delay
// nops, so the mover is sched1's forward delay-slot fill, not the source order.
// gcc-2.7.2-cdk measures the same 98.45 and 2.6.0 the same 93.66 as before, so it
// is not a preset question. Dead earlier: element-wise field copies (70.00), a
// volatile copy source (turns it into memcpy calls), a volatile D_800AFB00.
// Next lever: something that gives those two call-argument insns a home before the
// load slot, or closes the slot with a conflicting instruction.
// Best 98.45% / 668 bytes against retail's 672 (one instruction short). Every
// variant measured this round is in the Round 471 note above; the mover is sched1's
// forward delay-slot fill.
// Round 479: re-measured 98.45% / 668 bytes against retail's 672 (retail 168 insns,
// this body 167), the same residual round 471 left, and the permuter is out: a
// 3-minute run reports PERMUTER_NONE base=260 best=260 over 1624 iterations.
// THE RESIDUAL IS ONE INSN, and its cause is register allocation, not statement order:
// retail's third light block hoists the call's argument constant (`li a0,0x2`) to the
// block head, which keeps a0 live across the 22-instruction light copy, so the copy
// loads take a2/a3 and the pData skip fills the jal slot (`addiu s0,s0,4`) while the
// third colour `lhu` pays a load-delay nop. This body sinks the constant to the jal slot
// instead, which frees $a0 for the copy (loads in $v0/$v1/$a0/$a2), puts the skip in
// the `lhu` load-delay slot and leaves the function 4 bytes short. cc1's own .s shows
// the same split at -O2 (blocks 1 and 2 emit their `li $4` at the head, block 3 does
// not) while `addu $5,$17,352` IS hoisted in block 3, so the hoist machinery works on the
// second argument's computation and not on the constant: once local-alloc gives $a0 to a
// copy load, sched2 can never move the constant above it (WAW). Round 479 measured dead
// on this body, all 668 bytes / 167 insns and the same 21 differing lines: a named
// `s32 lightIndex = 2;` passed to the call (copy-propagated, byte-identical), and a single
// do-while barrier wrapping the copies AND the call (byte-identical). Round 471's four
// skip-position variants (95.24 / 97.26 x3) stand. Not a preset question (round 471:
// gcc-2.7.2-cdk 98.45, 2.6.0 93.66).
/*
void func_8006FDEC(s16* pData) {
    FieldLight* pLights;
    FieldLight* pLight1;
    FieldLight* pLight2;
    long flag;
    u_short color;

    FieldMatrixLookAt(
        &g_Scene.viewMatrix,
        (VECTOR*)((u8*)&g_Scene - 0x110),
        (VECTOR*)((u8*)&g_Scene - 0x100),
        (VECTOR*)((u8*)&g_Scene - 0xF0)
    );
    RotMatrix(&g_Scene.worldRotation, &g_Scene.worldToScreenMatrix);
    MulMatrix2(&g_Scene.viewMatrix, &g_Scene.worldToScreenMatrix);

    pLights = (FieldLight*)((u8*)&g_Scene + 0x138);
    pLight1 = &pLights[1];
    pLight2 = &pLights[2];
    pLights[0].vx = *pData++;
    D_800AFACC = *pData++;
    D_800AFAD0 = *pData++;
    pData++;
    D_800AFAD4 = (u_short)*pData++ << 3;
    D_800AFAD6 = (u_short)*pData++ << 3;
    D_800AFAD8 = (u_short)*pData++ << 3;
    pData++;
    func_80030A30(0, &pLights[0]);

    pLight1->vx = *pData++;
    D_800AFAE0 = *pData++;
    D_800AFAE4 = *pData++;
    pData++;
    D_800AFAE8 = (u_short)*pData++ << 3;
    D_800AFAEA = (u_short)*pData++ << 3;
    D_800AFAEC = (u_short)*pData++ << 3;
    pData++;
    func_80030A30(1, pLight1);

    pLight2->vx = *pData++;
    D_800AFAF4 = *pData++;
    D_800AFAF8 = *pData++;
    pData++;
    D_800AFAFC = (u_short)*pData++ << 3;
    D_800AFAFE = (u_short)*pData++ << 3;
    D_800AFB00 = (u_short)*pData++ << 3;
    pData++;

    do {
        pLights[1] = pLights[0];
        pLights[2] = pLights[0];
    } while (0);
    func_80030A30(2, pLight2);

    do {
        color = (u_short)pData[0] << 4;
        D_800AFB04 = color;
    } while (0);
    do {
        color = (u_short)pData[1] << 4;
        D_800AFB06 = color;
    } while (0);
    do {
        color = (u_short)pData[2] << 4;
        D_800AFB08 = color;
    } while (0);
    pData += 3;

    SetRotMatrix(&g_Scene.viewMatrix);
    SetTransMatrix(&g_Scene.viewMatrix);
    RotTrans(&g_Scene.worldTranslation, &g_Scene.worldToScreenMatrix.t, &flag);
    func_80030B14(&g_Scene.worldToScreenMatrix);
    SetRotMatrix(&g_Scene.worldToScreenMatrix);
    SetTransMatrix(&g_Scene.worldToScreenMatrix);
}*/
INCLUDE_ASM("asm/field/nonmatchings/main/misc3", func_8006FDEC);







void FieldLZSSDecompress(void* _unused, void* pCompressed, void* pDecompressed) {
    LZSSDecompress(pCompressed, pDecompressed);
}


// FieldFree's render-context loop computes `i + 1` before the flush pair and
// copies it into `i` after it: written as `i++` gcc coalesces the increment into
// one `addiu` and reorg steals it into GfxFlushImageList's jal delay slot, where
// the original has a `nop`.
// g_FieldActorModelSlotCount is an unbounded-array extern on purpose (the address
// is materialised once for the read and the store), and the actor loop indexes as
// `i * sizeof(FieldActor) + base` because that is the spelling whose addu is
// index-first, as the original's is.
extern s32 g_FieldActorModelSlotCount[];
extern void* g_FieldArchiveQueueBuffer;
extern void* g_FieldDialogs;
extern void* g_FieldModelFile;
extern s16 g_FieldScreenEffectEnabled;
extern void* g_FieldScreenEffectData;

// The walkmesh file the loader decompresses and the views into it: its first
// word is the triangle count, then the four counts and offsets it walks, then the
// pointers the renderer reads (g_FieldWalkmeshTriangles etc.).
typedef struct {
    /* 0x00 */ void* pFile;
    /* 0x04 */ void* pSpriteData;
    /* 0x08 */ u32* pMaterialFlags;
    /* 0x0C */ u32* pTriangles[4];
    /* 0x1C */ u32* pVertices[4];
    /* 0x2C */ s32 unk2C[4];
    /* 0x3C */ s16 triCount;
} FieldWalkmesh; // Size: 0x3E

extern FieldWalkmesh g_FieldWalkmesh;

extern void WorkListsFreeAllEntries(void);
extern void GfxFlushImageList(void);
extern void GfxSetRenderContext(int context);
extern void GfxFreeWorkBuffers(void);
extern void FieldFreeActorData(int actorIndex);
extern void ModelRestorePartVertices(void* pBackup);
extern void ModelPartFreeLightData(void* pModelPart);
extern void FieldDistortionFree(void);
extern void GfxLineScrollFree(void* pLineScroll);
extern void FontFree(void);
extern void LibGearShutdown(void);
extern void FieldRenderSyncAndFlush(void);
extern void FieldFreeSpritePrimitives(void);
extern void HeapFreeIfNotNull(void* pData);

typedef struct {
    s16 numLineScrolls;
    s16 unk2;
    void* pLineScrolls[32]; // 0x4
    u8* pDatas[32];         // 0x84
    s16 lengths[32];        // 0x104
} LineScrollTable; // Size: 0x144

extern LineScrollTable g_FieldLineScrolls;

void FieldFree(void) {
    FieldActor* pActor;
    s32* pCount;
    FieldActorModelData* pModelData;
    u_short status;
    int i;
    int next;

    ResetGraph(1);
    WorkListsFreeAllEntries();

    i = 0;
    do {
        next = i + 1;
        GfxFlushImageList();
        DrawSync(0);
        i = next;
        GfxSetRenderContext((g_FieldCurRenderContextIndex + i) & 1);
        GfxFlushImageList();
        DrawSync(0);
        GfxFreeWorkBuffers();
    } while (i < FIELD_RENDER_CONTEXT_COUNT);

    for (i = 0; i < g_FieldNumActors; i++) {
        FieldFreeActorData(i);
        pActor = (FieldActor*)(i * sizeof(FieldActor) + (int)g_FieldActors);
        status = pActor->status;
        if (!(status & 0x40)) {
            pModelData = pActor->pModelData;
            if (status & 0x2000) {
                ModelRestorePartVertices(pModelData->pScripts);
            }
            ModelPartFreeLightData(pModelData->pBounds);
            HeapFree(pModelData->pPrimitiveBuffers[0]);
            HeapFree(g_FieldActors[i].pModelData);
        }
    }

    FieldDistortionFree();
    HeapFree(g_FieldActors);
    HeapFree(g_pFieldTriggerZones);
    HeapFree(g_FieldDialogs);
    HeapFree(g_FieldCurScriptFile);
    HeapFree(g_FieldWalkmesh.pFile);
    HeapFree(g_FieldModelFile);
    HeapFree(g_FieldSpriteData);

    if (g_FieldScreenEffectEnabled != 0) {
        HeapFreeIfNotNull(g_FieldScreenEffectData);
    }

    for (i = 0; i < g_FieldLineScrolls.numLineScrolls; i++) {
        GfxLineScrollFree(g_FieldLineScrolls.pLineScrolls[i]);
        HeapFree(g_FieldLineScrolls.pDatas[i]);
        HeapFree(g_FieldLineScrolls.pLineScrolls[i]);
    }

    FontFree();
    pCount = g_FieldActorModelSlotCount;
    g_FieldLineScrolls.numLineScrolls = 0;
    if (*pCount != 0) {
        LibGearShutdown();
        HeapFree(g_FieldArchiveQueueBuffer);
        FieldRenderSyncAndFlush();
    }
    pCount[0] = 0;
    HeapFreeBlocksWithFlag(3);
    FieldFreeSpritePrimitives();
}

void FieldLoadTIMWithClut(u_long *pTimData, short x, short y, short clutX, short clutY, short clutWidth, short clutHeight) {
    TIM_IMAGE* pTIM;
    TIM_IMAGE tim;   

    OpenTIM(pTimData);
    pTIM = ReadTIM(&tim);

    if (pTIM) {
        if (tim.caddr) {
            if (clutY != -1) {
                tim.crect->x = clutX;
                tim.crect->y = clutY;
            }

            if (clutWidth)
                tim.crect->w = clutWidth;
            
            if (clutHeight)
                tim.crect->h = clutHeight;

            LoadImage(tim.crect, tim.caddr);
        }

        if (tim.paddr) {
            tim.prect->x = x;
        }
        tim.prect->y = y;
        LoadImage(tim.prect, tim.paddr);
    }
}

extern s32 g_FieldScriptDataStreamActive; // Field script data stream allocated?
extern int* g_FieldScriptDataStream; // Field script data stream
extern s32 g_GameSceneMapNum;

extern int* ArchiveAllocStreamFile(int numEntries, int allocMode);
extern void func_80029EB0(int, int*, int, int, short, short, short, short, short, short);

extern void FieldOrVramStripPixels(void);

void FieldStartScriptDataStream(void) {
    int* pStreamFile;

    if (!g_FieldScriptDataStreamActive) {
        g_FieldScriptDataStreamActive = 1;
        pStreamFile = ArchiveAllocStreamFile(4, 1);
        g_FieldScriptDataStream = pStreamFile;
        func_80029EB0(((g_GameSceneMapNum & 0xFFF) * 2) + 0xB9, pStreamFile, 0, 0, 0, 0, 0, 0, 0, 0);
    }
}

void FieldFreeScriptDataStream(void) {
    if (g_FieldScriptDataStreamActive == 1) {
        ArchiveCdDataSync(0);
        DrawSync(0);
        HeapFree(g_FieldScriptDataStream);
        g_FieldScriptDataStreamActive = 0;
    }

    FieldOrVramStripPixels();
}

void FieldConvertSVectorToVector(VECTOR* pDest, SVECTOR* pSrc) {
    pDest->vx = pSrc->vx << 16;
    pDest->vy = pSrc->vy << 16;
    pDest->vz = pSrc->vz << 16;
}

void FieldMatrixSetIdentity(MATRIX* dest) {
    SVECTOR rotation;

    rotation.vx = 0;
    rotation.vy = 0;
    rotation.vz = 0;
    RotMatrix(&rotation, dest);
    dest->t[2] = 0;
    dest->t[1] = 0;
    dest->t[0] = 0;
}

// Puts the whole field system back into its power-on state: every field global
// gets its default, the clip rectangle and the pad are reset, and the camera
// matrices are rebuilt. FieldLoad calls it once while it builds a scene.
// The 3-entry motion-slot actor id table sits 0x74 bytes below
// g_FieldScriptSoundEffectBank (g_FieldMotionSlotActorIds); the 16-entry table
// 0xBA bytes into g_FieldScriptRenderContextIndex and the 32-entry one ending at
// D_800B22DE have no symbol of their own, so they are reached from those bases.
extern u_short g_FieldButtonPollMask;
extern s16 g_FieldP1ButtonsPressed;
extern s16 g_FieldP2ButtonsPressed;
extern s16 g_FieldP1ButtonsReleased;
extern s16 g_FieldP2ButtonsReleased;
extern s16 g_FieldButtonsPressedOnce;
extern s16 g_FieldP2ButtonsPressedOnce;
extern u8 g_FieldScriptSoundEffectBank;
extern s16 g_FieldCurrentModelAnimationId;
extern s16 g_FieldGearObjectCapacity;
extern s16 g_FieldPlayerWalkmeshMaterialOverride;
extern s16 g_FieldStreamId;
extern s32 g_FieldScreenScrollY;
extern s32 g_FieldScreenScrollX;
extern s32 D_800B21BC;
extern s32 D_800B21C0;
extern s32 D_800B21C4;
extern s32 g_FieldSoundSlotId;
extern u8 g_FieldCompassDisabled;
extern u8 g_FieldRandomEncounterFlag;
extern s32 g_FieldSceneReloadFlags;
extern u8 g_FieldPartyMoveStepRan;
extern s16 D_800B21B0;
extern s16 D_800B21B2;
extern SVECTOR D_800B21AE;
extern s16 D_800B21A2;
extern s16 D_800B21A4;
extern s16 D_800B21A6;
extern s16 D_800B21A8;
extern s16 D_800B21AA;
extern s16 D_800B2186;
extern s16 D_800B2188;
extern u8 g_FieldCameraAtFollowTargetY;
extern u8 g_FieldPartySlotMask;
extern u8 g_FieldClearColorB;
extern u8 g_FieldClearColorG;
extern u8 g_FieldClearColorR;
extern u8 g_FieldPartyMoveInProgress;
extern u8 D_800B21CF;
extern u8 g_FieldScreenShakeMode;
extern s32 g_FieldMode;
extern s32 g_FieldSpecialAnimLoadPending;
extern s32 D_800ADB24;
extern s32 D_800ADB98;
extern s32 D_800ADB94;
extern s32 g_FieldStreamSceneActive;
extern s32 g_FieldArchiveStreamActive;
extern s32 D_800ADB68;
extern s32 g_FieldTransitionBusyFlag;
extern s32 g_FieldCameraAtYLatched;
extern s32 g_FieldNextGameState;
extern s32 g_FieldMapNumChanged;
extern s32 g_FieldScreenEffectSuppressed;
extern s32 g_FieldScriptSuppressScreenEffect;
extern s32 g_FieldActive;
extern u8 g_FieldScriptMemoryUpdateLocked;
extern s32 g_GameAudioManagerVolumeRestorePending;
extern s32 D_800ADBD0;
extern s16 g_FieldSoundProximitySource;
extern s16 g_FieldSoundElementMask;
extern s32 D_800B14A4;
extern s16 g_FieldScriptShopIndex;
extern s32 g_FieldScriptFadeMode;
extern s32 g_FieldScriptFadeDuration;
extern s32 g_FieldFadeDuration;
extern s32 g_FieldFadeMode;
extern s16 g_FieldSoundMaxDistance;
extern s32 g_FieldSkipCounter;
extern s32 g_FieldParticleCurActor;
extern s16 g_FieldTalkIdleFrames;
extern s16 g_Unknown_800B2344;
extern s16 D_800B2342;
extern s16 g_FieldWalkAbortCount;
extern s16 g_FieldPlayerActorPartySlot;
extern u8 g_FieldWdsSoundEffectBankIndex;
extern u8 D_800ADB04;
extern u8 g_FieldDisableActorRender;
extern u8 g_FieldFogDisabled;
extern u8 g_FieldAlternatePadAngles;
extern s32 D_800ADBB4;
extern s32 g_FieldSavedPlayerScriptFlags;
extern s16 g_FieldScreenStripSpritesEnabled;
extern u8 g_FieldPauseDisabled;
extern s32 g_FieldScriptCounter;
extern s32 g_FieldStreamReadyFlag;
extern s32 g_FieldScriptRoutineRunning;
extern s32 g_FieldRequestCode;
extern s16 D_800AF93A;
extern s16 g_FieldScriptRenderContextIndex;
extern s16 g_FieldActorUpdateAngle;
extern s16 g_FieldPolarVectorScale;
extern s32 g_FieldPartyModelLoadState;
extern s32 g_FieldStreamPlayerInitialized;
extern s16 g_FieldPendingWdsBankIndex;
extern s32 g_FieldMapTransitionGate;
extern s32 g_FieldCameraSnapFrames;
extern u8 g_FieldFogNearB;
extern u8 g_FieldFogNearG;
extern u8 g_FieldFogNearR;
extern u8 g_FieldFogFarB;
extern u8 g_FieldFogFarG;
extern u8 g_FieldFogFarR;
extern s16 g_FieldFogNearDistance;
extern s16 g_FieldFogFarDistance;
extern s32 D_800ADC0C;
extern s32 g_PlayerActorIndex;
extern s16 g_FieldTextBoxInUseMask;
extern s32 g_FieldActorAllocCount;
extern s32 g_FieldFrameDelay;
extern s16 g_FieldFogEnabled;
extern s16 g_FieldTextScaleDivisor;
extern s32 g_GamePartySkinsInitialized;
extern s32 g_FieldPartyActorSlots[];
extern s32 g_FieldPartyActorIndices[];
extern u_short D_800B22DE[];
extern s32 D_800B2298;
extern s32 D_800B229C;
extern s32 g_GfxOTDepthShift;
extern MATRIX D_800AF85C;
extern MATRIX g_FieldModelScreenMatrix;
extern SVECTOR g_FieldRotationVector;
extern void func_8028125C(void);
extern void func_8007254C(void);
extern void FieldClearActorSoundSlots(void);
extern void FieldInitializeParticles(void);
extern void FieldSetClipDimensions(short x, short y, short w, short h);
extern void ControllerResetState(void);
extern s16 g_FieldFadeIsWhite;
extern u_short g_FieldPlayerActorIndex16;

// The paired rotation op writes two 0x6-byte vectors, vx/vz/vy order (see
// FieldScriptSetRotationVectorPair in misc.c).
typedef struct {
    /* 0x0 */ s16 vx;
    /* 0x2 */ s16 vy;
    /* 0x4 */ s16 vz;
} FieldRotationVector; // Size: 0x6

typedef struct {
    /* 0x0 */ FieldRotationVector first;
    /* 0x6 */ FieldRotationVector second;
} FieldRotationVectorPair; // Size: 0xC

extern FieldRotationVectorPair g_FieldRotationVectorPair;
extern void FieldResetPortraitSlots(void);
extern void FieldInitScreenStripSprites(void);

// 426 insns, huge straight-line global-reset block (field scene reset). Parked at 84.61%,
// 426 insns / 1704 bytes - RETAIL'S EXACT SIZE - with the frame and prologue/epilogue exact.
// The old 89.55% note was a 427-insn / 1708-byte body; this one is the better base.
// Fixed here (do not re-derive): (a) the frame is 0x38 because of a DEAD 16-byte array local -
// `u_long unusedFrame[4];` declared BEFORE `SVECTOR rotation;` puts rotation at sp+0x20 and the
// saves at 0x28/0x2C/0x30 (lever j-an-unused-local-array-is; no callee takes a stack argument).
// (b) the copy loop needs three walking pointers (pMemory, pZero = pMemory + 0x200, and a
// GameState* local feeding `pState->fieldScriptMemory[i]`, which gcc strength-reduces into
// `lhu 0x1930(a0)` + addiu - a u16* pSource gives 427 insns and loses that displacement).
// (c) D_800B22DE must be u_short: the 32-entry 0xFFFF fill is retail's `ori v1,zero,0xffff`.
// NAMED RESIDUAL (1): cse folds every global pointer base, so ours re-forms symbol+offset where
// retail keeps the base in a register for 200 instructions. Retail loads &bank into $s0 at insn
// 20 (across two calls), stores `sb v0,0(s0)` and derives the first fill loop with
// `addiu s0,s0,-0x70`; retail's 16-loop likewise holds the base in $v1 (`lui/addiu %lo(sym)`,
// `sh v0,0(v1)`) and derives the loop pointer with `addiu a2,v1,0xBA`. The parked body spells the
// bank as `u8* pBank = &g_FieldScriptSoundEffectBank;` (initializer at the declaration, tried), as
// an assignment before the store (same bytes) and as the raw expression
// (s16*)((u8*)&g_FieldScriptSoundEffectBank - 0x70) (427 insns); all three fold to
// `lui at; sb %lo(bank)` plus a fresh `%hi(bank-0x70)`, so no callee-saved base is ever live. That
// is the root of the rest: without it $s0/$s1 end up on g_Scene only in the tail, and the whole
// allocation/schedule shifts (our loop counters land in $v1 where retail's are $a1 in all five
// loops; retail keeps 0x1000 in $a0 and reuses it for D_800AF93A and g_FieldPolarVectorScale where
// we re-`li` it; the 200-store block's li/store pairing differs).
// NAMED RESIDUAL (2): include/field/main.h declares g_FieldControl.controllerBtnMask as `short`,
// so `= 0xFFFF` emits `addiu v0,-1` where retail has `ori v0,zero,0xffff` (unsigned field); the
// header is shared, so it was not changed here.
// Everything else in the diff is that schedule/permutation; the ~131 differing instruction rows
// contain no structural difference left that was found.
/*
void func_800705DC(void) {
    u_long unusedFrame[4];
    SVECTOR rotation;
    u8* pBank = &g_FieldScriptSoundEffectBank;
    s16* pIndex;
    u_short* pMemory;
    u_short* pZero;
    GameState* pState;
    s16* pSlot;
    int i;

    if (g_FieldSystemMode == 0) {
        func_8028125C();
    }

    FieldSetClipDimensions(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    ControllerResetState();

    g_FieldButtonPollMask = 0xFFFF;
    g_FieldP1ButtonsPressed = 0;
    g_FieldP2ButtonsPressed = 0;
    g_FieldP1ButtonsReleased = 0;
    g_FieldP2ButtonsReleased = 0;
    g_FieldButtonsPressedOnce = 0;
    g_FieldP2ButtonsPressedOnce = 0;
    *pBank = 5;
    g_FieldCurrentModelAnimationId = 3;
    g_FieldGearObjectCapacity = 0x40;
    g_FieldPlayerWalkmeshMaterialOverride = 0xFF;
    g_FieldStreamId = 0xFF;
    g_FieldScreenScrollY = 0;
    g_FieldScreenScrollX = 0;
    D_800B21BC = 0;
    D_800B21C0 = 0;
    D_800B21C4 = 0;
    g_FieldActorModelSlotCount[0] = 0;
    g_FieldSoundSlotId = 0;
    g_FieldCompassDisabled = 0;
    g_FieldRandomEncounterFlag = 0;
    g_FieldEffects.distortion.v6 = 0;
    g_FieldEffects.distortion.v5 = 0;
    g_FieldEffects.distortion.v4 = 0;
    g_FieldEffects.distortion.v3 = 0;
    g_FieldEffects.distortion.v2 = 0;
    g_FieldEffects.distortion.v1 = 0;
    g_FieldEffects.distortion.unk3C = 0;
    g_FieldEffects.distortion.unk38 = 0;
    g_FieldSceneReloadFlags = 0;
    g_FieldPartyMoveStepRan = 0;
    D_800B21B0 = 0;
    D_800B21B2 = 0;
    D_800B21AE.vx = 0;
    g_FieldEffects.distortion.isActive = 0;
    g_FieldCameraAtFollowTargetY = 0;
    g_FieldPartySlotMask = 0;
    g_FieldClearColorB = 0;
    g_FieldClearColorG = 0;
    g_FieldClearColorR = 0;
    g_FieldPartyMoveInProgress = 0;
    D_800B21CF = 0;
    g_FieldScreenShakeMode = 0;
    g_FieldMode = 0;
    g_FieldRenderContextUseOT2 = 0;
    g_FieldSpecialAnimLoadPending = 0;
    D_800ADB24 = 0;
    D_800ADB98 = 0;
    D_800ADB94 = 0;
    g_FieldStreamSceneActive = 0;
    g_FieldArchiveStreamActive = 0;
    D_800ADB68 = 0;
    g_FieldTransitionBusyFlag = 0;
    g_FieldCameraAtYLatched = 0;
    g_FieldNextGameState = 0;
    g_FieldMapNumChanged = 0;
    g_FieldScreenEffectSuppressed = 0;
    g_FieldScriptSuppressScreenEffect = 0;
    g_FieldActive = 0;
    g_FieldScriptMemoryUpdateLocked = 0;
    g_GameAudioManagerVolumeRestorePending = 0;
    D_800ADBD0 = 0;
    g_FieldSoundProximitySource = 0;
    g_FieldSoundElementMask = 0;
    D_800B14A4 = 0;
    g_FieldScriptShopIndex = 0;
    g_FieldScriptFadeMode = 0;
    g_FieldScriptFadeDuration = 0;
    g_FieldFadeDuration = 0x20;
    g_FieldFadeMode = 2;
    g_FieldSoundMaxDistance = 0x3FF;
    g_FieldSkipCounter = 4;
    g_FieldParticleCurActor = 0;
    g_FieldTalkIdleFrames = 0;
    g_FieldPlayerActorIndex16 = 0;
    g_Unknown_800B2344 = 0;
    D_800B2342 = 0;
    g_FieldWalkAbortCount = 0;
    g_FieldPlayerActorPartySlot = 0;
    g_FieldWdsSoundEffectBankIndex = 0;
    D_800ADB04 = 0;
    g_FieldDisableActorRender = 0;
    g_FieldFogDisabled = 0;
    g_FieldAlternatePadAngles = 0;
    D_800ADBB4 = 0;
    g_FieldSavedPlayerScriptFlags = 0;
    g_FieldScreenStripSpritesEnabled = 0;
    g_FieldPauseDisabled = 0;
    g_FieldScriptCounter = 0;
    g_FieldStreamReadyFlag = 0;
    g_FieldScriptRoutineRunning = 0;
    g_FieldRequestCode = 0xFF;

    rotation.vx = 0;
    rotation.vy = 0;
    rotation.vz = 0;
    RotMatrix(&rotation, &g_FieldModelScreenMatrix);

    pSlot = (s16*)(pBank - 0x70);
    for (i = 2; i >= 0; i--) {
        *pSlot-- = -1;
    }

    g_FieldFadeIsWhite = 1;
    D_800AF93A = 0x1000;
    pIndex = &g_FieldScriptRenderContextIndex;
    *pIndex = 0x720;

    pSlot = pIndex + 0x5D;
    for (i = 0xF; i >= 0; i--) {
        *pSlot-- = 0x1D;
    }

    D_800B21A4 = 0x100;
    D_800B21A2 = 0x100;
    g_FieldRotationVectorPair.first.vx = 0x100;
    D_800B21AA = 0x200;
    D_800B21A8 = 0x200;
    D_800B21A6 = 0x200;
    g_FieldActorUpdateAngle = 0x80;
    g_FieldPartyModelLoadState = 0xFF;
    g_FieldPolarVectorScale = 0x1000;
    g_FieldRotationVector.vx = 0;
    D_800B2186 = 0;
    D_800B2188 = 0;
    g_FieldStreamPlayerInitialized = -1;

    g_FieldPendingWdsBankIndex = 0x1D;
    g_FieldMapTransitionGate = -1;
    g_FieldCameraSnapFrames = 2;
    g_FieldControl.controllerBtnMask = 0xFFFF;
    g_FieldFogNearB = 0x80;
    g_FieldFogNearG = 0x80;
    g_FieldFogNearR = 0x80;
    g_FieldFogFarB = 0xFF;
    g_FieldFogFarG = 0xFF;
    g_FieldFogFarR = 0xFF;
    g_FieldFogNearDistance = 0x15E0;
    g_FieldFogFarDistance = 0x300C;
    g_FieldCurRenderContextIndex = 0;
    D_800ADC0C = 0;
    g_FieldLineScrolls.numLineScrolls = 0;
    g_PlayerActorIndex = 0;
    g_FieldControl.angle = 0;
    g_FieldTextBoxInUseMask = 0;
    g_FieldControl.isRandomEncountersEnabled = 0;
    g_FieldActorAllocCount = 0;
    g_FieldFrameDelay = 0;
    g_FieldFogEnabled = 0;
    g_FieldTextScaleDivisor = 8;

    if (g_GamePartySkinsInitialized == 0) {
        for (i = 0; i < 3; i++) {
            g_FieldPartyActorSlots[i] = 0xFF;
            g_FieldPartyActorIndices[i] = 0xFF;
        }
    }

    pMemory = D_800B22DE;
    for (i = 0x1F; i >= 0; i--) {
        *pMemory-- = 0xFFFF;
    }

    D_800B229C = 0;
    D_800B2298 = 0;
    g_GfxOTDepthShift = 2;

    pMemory = (u_short*)&g_FieldScriptMemory;
    pZero = pMemory + 0x200;
    pState = g_pGameState;
    for (i = 0; i < 0x200; i++) {
        *pMemory++ = pState->fieldScriptMemory[i];
        *pZero++ = 0;
    }

    SetGeomScreen(0x200);

    FieldMatrixSetIdentity(&g_Scene.viewMatrix);
    FieldMatrixSetIdentity(&D_800AF85C);
    FieldMatrixSetIdentity(&g_Scene.worldToScreenMatrix);
    FieldMatrixSetIdentity(&g_Scene.worldRotationMatrix);

    g_Scene.worldRotation.vx = 0;
    g_Scene.worldRotation.vy = 0;
    g_Scene.worldRotation.vz = 0;
    g_Scene.worldTranslation.vx = 0;
    g_Scene.worldTranslation.vy = 0;
    g_Scene.worldTranslation.vz = 0;
    g_WorldScale = 0x3000;
    RotMatrix(&g_Scene.worldRotation, &g_Scene.worldToScreenMatrix);
    g_FieldCurRenderContext = g_FieldRenderContexts;

    func_8007254C();
    FieldResetPortraitSlots();
    FieldClearActorSoundSlots();
    FieldInitializeParticles();
    FieldInitScreenStripSprites();
}*/
INCLUDE_ASM("asm/field/nonmatchings/main/misc3", func_800705DC);



extern s16 g_FieldPortraitSlots[3][3];
extern s16 D_800B06A6[3][3];
extern s32 g_FieldPortraitSlotCursor;

void FieldResetPortraitSlots(void) {
    int i;

    for (i = 0; i < 3; i++) {
        g_FieldPortraitSlots[i][0] = 0xFF;
        D_800B06A6[i][0] = 0xFF;
    }

    g_FieldPortraitSlotCursor = 0;
}

// --- FieldLoad: reads the staged field scene file into the field globals -----

extern void* g_GameStagedFileBuffer;
extern s32 g_GameStagedFileTag;
extern s32 g_GameStagedFileIndex;
extern s32 g_FieldSystemMode;
extern s32 g_FieldEntryRoutineFinished;
extern s32 g_FieldRenderContextUseOT2;
extern s32 g_FieldActiveActorCount;
extern u_short g_FieldPlayerActorIndex16;
extern s16 g_FieldScreenEffectEnabled;
extern void* g_FieldScreenEffectData;
extern s16 g_FieldMotionSlotSteps[];
extern void* g_FieldModelFile;
extern void* g_FieldDialogs;
extern u8 D_800658DC[];
extern VECTOR g_CameraAt2;
extern SVECTOR g_FieldRotationVector;
extern u8 g_FieldBackgroundR;
extern u8 g_FieldBackgroundG;
extern u8 g_FieldBackgroundB;

extern void func_800705DC(void);
extern void func_8007A5C4(void);
extern void func_8006FDEC(s16* pData);
extern void FieldAllocActorData(int actorIndex);
extern void FieldSnapPartyToPlayer(void);
extern void func_800A28D4(void);
extern void func_802812A4(void);
extern void FieldLoadTIM(u_long* pTimData);
extern void FieldInitTableQuad(Quad* pPart, int x, int y, int tex);
extern void FieldSetMatrix(s16* pOut, int x0, int x1, int x2, int x3, int x4, int x5, int x6, int x7, int x8);
extern void GfxLoadClutsAccelerated(u8* pData, int x, u16 y);
extern void GfxAllocateWorkBuffers(int workBufferSize, unsigned int allocFlag);
extern void WorkListsReset(void);
extern void FieldTextBoxInitialize(void);
extern void FieldFadeInitialize(void);
extern void FieldLoadAndCreateObjectModels(void);
extern void FieldLoadActorAnimationFiles(void);
extern void FieldUpdateActorRenderModes(void);
extern int FieldAnyActorHasFlag8000(void);
extern void SpriteSetDirection(SpriteData* pSpriteData, s16 direction);
extern void func_800223B0(SpriteData* pSpriteData, s32 animId);
extern void ModelResolvePointers(ModelFileHeader* pModelFile);
extern void ModelAllocatePackets(ModelPart* pModelPart, void** pPacket1, void** pPacket2);
extern void ModelInitializePacket(ModelPart* pPart, void* pPacket, s32 packetIndex);
extern int ModelFileTrimHeapBlock(ModelFileHeader* pModelFile);
extern void* func_800303C8(ModelPart* pModelPart, s32 arg1);

typedef struct {
    /* 0x0 */ s32 vx;
    /* 0x4 */ s32 vy;
    /* 0x8 */ s32 vz;
    /* 0xC */ s32 pad;
} FieldVector3; // Size: 0x10

// The default spawn orientation the loader copies onto the stack for the actors
// it creates (misc3's only .rodata item). Commented out while FieldLoad is a
// stub, because the stub's own .s supplies this object; an unpark must restore
// it or the unit loses its 0x10 bytes of .rodata.
// const FieldVector3 D_8006FAF4 = {0, -100, 2000, 0};

// The 0x100-byte block the loader copies into the sprite placement table
typedef struct {
    /* 0x00 */ u8 unk0[0x100];
} FieldSceneFileHeader; // Size: 0x100

// Per-sprite-id placement table, stride 8 (the table misc2.c walks too)
typedef struct {
    /* 0x0 */ u16 texX;
    /* 0x2 */ u16 texY;
    /* 0x4 */ u16 clut;
    /* 0x6 */ s16 clutLoaded;
} FieldSpritePlacement; // Size: 0x8

extern FieldSpritePlacement g_FieldSpritePlacements[];

// Quad table the field renders its table-mode primitives from: the 4x4 grid
// FieldLoad fills by index, then 5 fixed quads at ids 4..8
typedef struct {
    /* 0x0 */ u8 unk0[0x70];
} FieldTableQuad; // Size: 0x70

extern FieldTableQuad D_800B06BC[];
extern FieldTableQuad D_800B0DBC[];

// The header pointers the renderer reads (field/main/misc4.c uses the same names)
extern u32* g_FieldWalkmeshMaterialFlags;
extern s16 g_FieldWalkmeshTriCount;
extern u32* g_FieldWalkmeshTriangles[];
extern u32* g_FieldWalkmeshVertices[];
extern s32 D_800AFD10;

// The screen effect setup record at 0x800B0080 and the parameter record at
// 0x800B00A0 that follows it (FieldScriptSetParams80 / FieldScriptSetParamsA0
// fill the same memory a field at a time).
typedef struct {
    /* 0x00 */ s16 unk0;
    /* 0x02 */ s16 unk2;
    /* 0x04 */ s16 unk4;
    /* 0x06 */ s16 unk6;
    /* 0x08 */ s16 unk8;
    /* 0x0A */ s16 unkA;
    /* 0x0C */ s16 unkC;
    /* 0x0E */ s16 unkE;
    /* 0x10 */ s32 unk10;
    /* 0x14 */ s32 unk14;
    /* 0x18 */ s32 unk18;
    /* 0x1C */ u8 unk1C;
    /* 0x1D */ u8 unk1D;
    /* 0x1E */ u8 unk1E;
    /* 0x1F */ u8 unk1F;
    /* 0x20 */ u8 unk20;
    /* 0x21 */ u8 unk21;
    /* 0x22 */ u8 unk22;
    /* 0x23 */ u8 unk23;
    /* 0x24 */ u8 unk24;
    /* 0x25 */ u8 unk25;
    /* 0x26 */ u8 unk26;
    /* 0x27 */ u8 unk27;
    /* 0x28 */ s16 unk28;
    /* 0x2A */ s16 unk2A;
    /* 0x2C */ s16 unk2C;
} FieldScreenEffectConfig; // Size: 0x2E

extern FieldScreenEffectConfig g_FieldScreenEffectConfig;

extern MATRIX D_800AFC30;
extern s32 D_800AFC44;
extern s32 D_800AFC48;
extern s32 D_800AFC4C;

// The motion slot working area g_FieldMotionSlotSteps lives in; the second
// FieldSetMatrix layout sits 0x20 bytes in front of it
extern u8 g_FieldModelSlotScratch[];

#define STAGED_FILE ((ActorFile*)((u8*)g_GameStagedFileBuffer + 0x10C))
// The actor table is a volatile pointer, so retail re-reads it at every use
#define ACTOR(index) ((FieldActor*)((index) * sizeof(FieldActor) + (s32)g_FieldActors))

// PARKED 77.89% (3484 bytes = retail's exact instruction count; the frame (160)
// and the prologue's saved set s0-s5 are byte-identical to retail).
// WHAT MATCHED, AND WHY (keep these):
//  - the actor-table malloc must store the LOCAL, not re-read the global:
//    pWord = HeapAlloc(...); g_FieldActors = (FieldActor*)pWord;  -- g_FieldActors
//    is a VOLATILE pointer, so pWord = (u32*)g_FieldActors; after the store
//    reloads it and leaves the body one instruction over (74.78 -> 75.80).
//  - every use of the model part AFTER ModelAllocatePackets goes through
//    pModel->pBounds, not the local: retail does lw a0,0x4(s0) before
//    ModelInitializePacket, func_800303C8 and ModelFileTrimHeapBlock, and reloads
//    both primitive buffers from pModel as well. pModelPart is dead after the
//    first call, which is what frees the extra callee-saved register (75.80 ->
//    77.89, retail's exact prologue).
//  - the TIM offset loop is pOffsets = (s32*)pTimData + 1; ... *pOffsets++
//    (retail puts the +4 in the loop-back branch delay slot); measured neutral.
//  - the model-header loop is ((s32*)g_FieldModelFile)[i + 1], indexed, NOT
//    *pOffsets++: the pointer form is retail's shape but leaves the body 8 bytes
//    SHORT (3476) because retail also re-reads g_FieldModelFile into a saved
//    register in the preheader (lw s2,0(s0)), and the indexed form's sll/addu per
//    iteration is what makes up the two instructions.
// RESIDUAL: the actor spawn loop forms a SECOND induction register for pSpawn.
// Retail reaches ry/rz with 2(s5)/4(s5) off one register; this body keeps a
// parallel pointer (pSpawn+6) and reads -2/0 off it. Reproduced in isolation
// with tools/scripts/cc1_probe.sh on a 40-line replica, so it is not the
// surrounding function; dropping the transform/childMatrix read group from the
// replica makes the split go away, so the record's second read group is what
// triggers loop.c's giv combining. Spellings measured and dead: *pSpawn++ vs
// pSpawn[0] + pSpawn += 1, reading offsets 3..5 with a single += 6, u16
// temporaries, one ACTOR(i) reload for all three pairs, gcc 2.6.0 (74.65) and
// 2.7.2-cdk (68.46). It is not a register count: the saved set already matches.
// STRUCT FACTS (keep): misc3 owns exactly one .rodata item, D_8006FAF4
// {0,-100,2000,0} -- converting FieldLoad must carry
// const FieldVector3 D_8006FAF4 = {0, -100, 2000, 0}; at file scope, and it is
// dropped here because the stub's .s owns that dlabel. 0x800B0080..0x800B00B1 is
// ONE object (FieldScreenEffectConfig). 0x800AFB18 is one object (FieldWalkmesh:
// +4 spriteData, +8 materialFlags, +0xC triangles[4], +0x1C vertices[4], +0x2C
// the >>1 /7 counts, +0x3C triCount; retail re-reads -0xC(a2) per store). The
// spawn records are 8 u16, so the pointer bumps +2/+6/+6 and the tail's +2 lands
// in FieldAllocActorData's delay slot; the loads are lhu, so they are u_short.
// FieldLightFileData must stay 0x2A bytes (ActorFile needs 0xE of padding before
// numEntitites at 0x18C; actorData is the inline array at 0x190). The -0x20/-0xB8
// first arguments of the two FieldSetMatrix calls and the RotMatrix are
// displacements off ONE base, i.e. g_FieldModelSlotScratch .. g_FieldMotionSlotSteps
// is one object in the source too.
// PARKED 80.99% at retail's EXACT 3484 bytes and instruction count (871/871).
// The frame is 0x98 against retail's 0xA0 and the saved set is s0-s4 against
// retail's s0-s5, so one long-lived value is still in a caller-saved register
// (that one 8-byte frame gap plus its sw/lw pair is the whole remaining byte
// budget: everything else is register choice or store order inside twenty small
// regions). Nothing structural is left - every jal count, every branch and the
// total instruction count match retail. Region residuals, largest first:
//   - walkmesh header: retail derives &triangles/&vertices from ONE base
//     register (&g_FieldWalkmeshMaterialFlags, +4/+0x14) where we materialise the
//     g_FieldWalkmeshTriangles and g_FieldWalkmeshVertices symbols separately
//     (2 instructions). Spelling the three pointer fields as a struct whose base
//     IS g_FieldWalkmeshMaterialFlags reproduces it, but then D_800AFD10 loses
//     the two %hi/%lo loads retail has, so both spellings measure 3484.
//   - screen-effect config zeroing (~0x1C..0x2C): retail mixes s2-relative stores
//     with D_800B00xx absolute ones; our order emits two of the byte clears
//     (unk1D/unk1E) absolute where retail reaches them off s2.
//   - final actor loop: retail copies i into $a0 in the model arm's `j` delay
//     slot and leaves pSpawn's +2 in the jal slot; ours swaps the two.
//   - the spawn loop's t[]/childMatrix pairs need the `value` local (one load,
//     two stores - without it gcc reloads *pSpawn per store, +3 instructions).
// MEASURED LEVERS that closed the last lane's residuals, keep them:
//   - the three t[] pairs want a value temp, not two pSpawn[] reads;
//   - `if ((status & 0x40) == 0) { model } else { invisible }` (retail's
//     invisible arm is OUT of line, so the test must be the inverted form);
//   - the walkmesh counts divide is `*pWord++ / 14`, NOT `(x >> 1) / 7`: retail's
//     0x92492493 magic is gcc's /14 expansion (pre-shift by 1 then mulhi >> 2);
//   - D_800AFD10 = g_FieldWalkmeshTriangles[0] - g_FieldWalkmeshMaterialFlags
//     (a pointer difference, so gcc emits subu + sra 2) is a STATEMENT the
//     previous body was missing entirely - worth 8 instructions;
//   - the walkmesh header words 1..7 are one continuous *pWord++ walk.
/*
void FieldLoad(void) {
    FieldActor* pActor;
    FieldActorModelData* pModel;
    ModelPart* pModelPart;
    ActorFile* pFile;
    FieldWalkmesh* pWalk;
    u16* pSpawn;
    s32* pOffsets;
    u32* pWord;
    void* pTimData;
    void* pClutData;
    s32 size;
    s32 count;
    s32 i;
    s32 j;
    u16 value;
    FieldVector3 defaultVector;

    defaultVector = D_8006FAF4;
    func_800705DC();

    *(FieldSceneFileHeader*)g_FieldSpritePlacements = *(FieldSceneFileHeader*)g_GameStagedFileBuffer;

    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            FieldInitTableQuad((Quad*)&D_800B06BC[i * 4 + j], j, i, 0);
        }
    }
    FieldInitTableQuad((Quad*)&D_800B0DBC[0], 4, 4, 1);
    FieldInitTableQuad((Quad*)&D_800B0DBC[1], 5, 5, 1);
    FieldInitTableQuad((Quad*)&D_800B0DBC[2], 6, 6, 1);
    FieldInitTableQuad((Quad*)&D_800B0DBC[3], 7, 7, 1);
    FieldInitTableQuad((Quad*)&D_800B0DBC[4], 8, 8, 1);
    func_8007A5C4();

    size = STAGED_FILE->timPackageSize + 0x10;
    pTimData = HeapAlloc(size, 1);
    FieldLZSSDecompress((void*)size, (u8*)g_GameStagedFileBuffer + STAGED_FILE->timPackageOffset, pTimData);
    count = *(s32*)pTimData;
    pOffsets = (s32*)pTimData + 1;
    for (i = 0; i < count; i++) {
        FieldLoadTIM((u_long*)((u8*)pTimData + *pOffsets++));
    }

    size = STAGED_FILE->clutDataSize + 0x10;
    pClutData = HeapAlloc(size, 0);
    FieldLZSSDecompress((void*)size, (u8*)g_GameStagedFileBuffer + STAGED_FILE->clutDataOffset, pClutData);
    count = *(s32*)pClutData;
    pOffsets = (s32*)pClutData + 1;
    for (i = 0; i < count; i++) {
        if (g_FieldSpritePlacements[i].clutLoaded == 0) {
            GfxLoadClutsAccelerated((u8*)pClutData + *pOffsets, g_FieldSpritePlacements[i].texX, g_FieldSpritePlacements[i].texY);
        }
        pOffsets++;
    }
    DrawSync(0);
    HeapFree(pTimData);
    HeapFree(pClutData);

    size = STAGED_FILE->modelDataSize + 0x10;
    g_FieldModelFile = HeapAlloc(size, 0);
    FieldLZSSDecompress((void*)size, (u8*)g_GameStagedFileBuffer + STAGED_FILE->modelDataOffset, g_FieldModelFile);
    for (i = 0; i < *(s32*)g_FieldModelFile; i++) {
        ModelResolvePointers((ModelFileHeader*)((u8*)g_FieldModelFile + ((s32*)g_FieldModelFile)[i + 1]));
    }

    FieldLZSSDecompress((void*)(STAGED_FILE->unk3Size + 0x10),
                        (u8*)g_GameStagedFileBuffer + STAGED_FILE->pUnk3Data, D_800658DC);

    size = STAGED_FILE->scriptsSize + 0x10;
    g_FieldCurScriptFile = HeapAlloc(size, 0);
    FieldLZSSDecompress((void*)size, (u8*)g_GameStagedFileBuffer + STAGED_FILE->scriptsOffset, g_FieldCurScriptFile);
    g_FieldActiveActorCount = g_FieldCurScriptFile->numScripts;
    g_FieldScriptVMCurScriptData = (u8*)g_FieldCurScriptFile->metadata + g_FieldActiveActorCount * 0x40;

    size = STAGED_FILE->triggersSize + 0x10;
    g_pFieldTriggerZones = HeapAlloc(size, 0);
    FieldLZSSDecompress((void*)size, (u8*)g_GameStagedFileBuffer + STAGED_FILE->triggersOffset, g_pFieldTriggerZones);

    size = STAGED_FILE->dialogsSize + 0x10;
    g_FieldDialogs = HeapAlloc(size, 0);
    FieldLZSSDecompress((void*)size, (u8*)g_GameStagedFileBuffer + STAGED_FILE->dialogsOffset, g_FieldDialogs);

    size = STAGED_FILE->walkmeshSize + 0x10;
    g_FieldWalkmesh.pFile = HeapAlloc(size, 0);
    FieldLZSSDecompress((void*)size, (u8*)g_GameStagedFileBuffer + STAGED_FILE->walkmeshDataOffset, g_FieldWalkmesh.pFile);

    pWalk = &g_FieldWalkmesh;
    pWord = (u32*)pWalk->pFile + 1;
    g_FieldWalkmeshTriCount = *(s32*)pWalk->pFile;
    for (i = 0; i < 4; i++) {
        pWalk->unk2C[i] = *pWord++ / 14;
    }
    g_FieldWalkmeshMaterialFlags = (u32*)((u8*)pWalk->pFile + *pWord++);
    for (i = 0; i < g_FieldWalkmeshTriCount; i++) {
        g_FieldWalkmeshTriangles[i] = (u32*)((u8*)pWalk->pFile + *pWord++);
        g_FieldWalkmeshVertices[i] = (u32*)((u8*)pWalk->pFile + *pWord++);
    }
    D_800AFD10 = g_FieldWalkmeshTriangles[0] - g_FieldWalkmeshMaterialFlags;

    size = STAGED_FILE->spriteDataSize + 0x10;
    g_FieldSpriteData = HeapAlloc(size, 0);
    FieldLZSSDecompress((void*)size, (u8*)g_GameStagedFileBuffer + STAGED_FILE->spriteDataOffset, g_FieldSpriteData);

    g_Scene.scrollLimits[0] = 1;
    g_Scene.scrollLimits[1] = 1;
    g_Scene.scrollLimits[2] = 1;
    g_Scene.scrollLimits[3] = 1;
    func_8006FDEC((s16*)&STAGED_FILE->lightData);

    count = STAGED_FILE->numEntitites;
    pSpawn = STAGED_FILE->actorData;
    g_FieldNumActors = count;
    pWord = HeapAlloc(count * sizeof(FieldActor), 0);
    g_FieldActors = (FieldActor*)pWord;
    for (i = 0; i < count * (s32)(sizeof(FieldActor) / sizeof(s32)); i++) {
        pWord[i] = 0;
    }

    for (i = 0; i < g_FieldNumActors; i++) {
        ACTOR(i)->status = *pSpawn++;
        ACTOR(i)->rotation.x = pSpawn[0];
        ACTOR(i)->rotation.y = pSpawn[1];
        ACTOR(i)->rotation.z = pSpawn[2];
        pSpawn += 3;
        value = pSpawn[0];
        pActor = ACTOR(i);
        pActor->transformMatrix.t[0] = value;
        pActor->childMatrix.t[0] = value;
        value = pSpawn[1];
        pActor = ACTOR(i);
        pActor->transformMatrix.t[1] = value;
        pActor->childMatrix.t[1] = value;
        value = pSpawn[2];
        pActor = ACTOR(i);
        pActor->transformMatrix.t[2] = value;
        pActor->childMatrix.t[2] = value;
        pSpawn += 3;
        if ((ACTOR(i)->status & 0x40) == 0) {
            j = *pSpawn;
            pModel = HeapAlloc(sizeof(FieldActorModelData), 0);
            ACTOR(i)->pModelData = pModel;
            pModelPart = (ModelPart*)((u8*)g_FieldModelFile + ((s32*)g_FieldModelFile)[j + 1] + 0x10);
            pModel->pBounds = (FieldActorModelBounds*)pModelPart;
            ModelAllocatePackets(pModelPart, (void**)&pModel->pPrimitiveBuffers[0],
                                 (void**)&pModel->pPrimitiveBuffers[1]);
            ModelInitializePacket((ModelPart*)pModel->pBounds, pModel->pPrimitiveBuffers[0],
                                  (ACTOR(i)->status & 0xC) >> 2);
            memcpy(pModel->pPrimitiveBuffers[1], pModel->pPrimitiveBuffers[0],
                   ((ModelPart*)pModel->pBounds)->unk20[1].modelPacketSize);
            if (ACTOR(i)->status & 0x2000) {
                HeapChangeCurrentUser(3, 0);
                pModel->pScripts = func_800303C8((ModelPart*)pModel->pBounds, 0);
                HeapChangeCurrentUser(8, 0);
            }
            ModelFileTrimHeapBlock((ModelFileHeader*)pModel->pBounds);
        } else {
            ACTOR(i)->status |= ACTOR_STATUS_INVISIBLE;
            ACTOR(i)->rotation.x = 0;
            ACTOR(i)->rotation.y = 0;
            ACTOR(i)->rotation.z = 0;
        }
        pSpawn++;
        FieldAllocActorData(i);
    }

    if (g_FieldSystemMode == 0) {
        func_802812A4();
    }
    FieldTextBoxInitialize();
    FieldFadeInitialize();
    HeapUnpinBlock(g_GameStagedFileBuffer);
    HeapFree(g_GameStagedFileBuffer);
    HeapChangeCurrentUser(5, 0);
    GfxAllocateWorkBuffers(0x3C00, 0);
    WorkListsReset();
    HeapChangeCurrentUser(8, 0);
    FieldSetMatrix(g_FieldMotionSlotSteps, 0x800, 0, 0, 0x800, 0, 0, 0x800, 0, 0);
    FieldSetMatrix((s16*)(g_FieldModelSlotScratch + 0x30), 0x1F8, -0xFC1, -0x1F8, 0, 0, 0, 0, 0, 0);

    g_FieldBackgroundB = 0x1E;
    g_FieldBackgroundG = 0x1E;
    g_FieldBackgroundR = 0x1E;
    g_FieldScreenEffectConfig.unk4 = 0x140;
    g_FieldScreenEffectConfig.unkE = 0;
    g_FieldScreenEffectConfig.unk27 = 0;
    g_FieldScreenEffectConfig.unk26 = 0;
    g_FieldScreenEffectConfig.unk24 = 0;
    g_FieldScreenEffectConfig.unk23 = 0;
    g_FieldScreenEffectConfig.unk22 = 0;
    g_FieldScreenEffectConfig.unk20 = 0;
    g_FieldScreenEffectConfig.unk1F = 0;
    g_FieldScreenEffectConfig.unk1E = 0;
    g_FieldScreenEffectConfig.unk1D = 0;
    g_FieldScreenEffectConfig.unk1C = 0;
    g_FieldScreenEffectConfig.unk10 = 0;
    g_FieldScreenEffectConfig.unk18 = 0x1000;
    g_FieldScreenEffectConfig.unk2C = 0;
    g_FieldScreenEffectConfig.unk2A = 0;
    g_FieldScreenEffectConfig.unk28 = 0;
    g_FieldScreenEffectConfig.unkC = 0;
    g_FieldScreenEffectConfig.unkA = 0;
    g_FieldScreenEffectConfig.unk8 = 0;
    g_FieldScreenEffectConfig.unk6 = 0;
    g_FieldScreenEffectConfig.unk2 = 0;
    g_FieldScreenEffectConfig.unk0 = 0;
    g_FieldScreenEffectEnabled = 0;
    g_FieldScreenEffectConfig.unk14 = 0;
    g_FieldScreenEffectConfig.unk25 = 0x20;
    g_FieldEntryRoutineFinished = 0;

    func_800A28D4();
    g_FieldEntryRoutineFinished = 1;
    RotMatrix(&g_FieldRotationVector, &D_800AFC30);
    D_800AFC4C = 0;
    D_800AFC48 = 0;
    D_800AFC44 = 0;
    if (g_FieldScreenEffectEnabled != 0) {
        g_FieldScreenEffectData = func_8002709C(g_FieldScreenEffectConfig.unk0, g_FieldScreenEffectConfig.unk2, g_FieldScreenEffectConfig.unk4, g_FieldScreenEffectConfig.unk6,
                                                g_FieldScreenEffectConfig.unk8, g_FieldScreenEffectConfig.unkA, g_FieldScreenEffectConfig.unkC, g_FieldScreenEffectConfig.unkE,
                                                (VECTOR*)&g_FieldScreenEffectConfig.unk10, &g_FieldScreenEffectConfig.unk1C, g_FieldScreenEffectConfig.unk28,
                                                g_FieldScreenEffectConfig.unk2A, g_FieldScreenEffectConfig.unk2C);
    }
    HeapChangeCurrentUser(8, 0);

    pActor = (FieldActor*)(g_FieldPlayerActorIndex16 * sizeof(FieldActor) + (s32)g_FieldActors);
    g_CameraAt2.vx = pActor->transformMatrix.t[0] << 16;
    g_CameraAt2.vy = pActor->transformMatrix.t[1] << 16;
    g_CameraAt2.vz = pActor->transformMatrix.t[2] << 16;
    for (i = 0; i < g_FieldNumActors; i++) {
        pActor = (FieldActor*)(i * sizeof(FieldActor) + (s32)g_FieldActors);
        RotMatrix(&pActor->rotation, &pActor->transformMatrix);
        pActor->childMatrix = pActor->transformMatrix;
    }

    FieldLoadAndCreateObjectModels();
    FieldLoadActorAnimationFiles();
    g_GameStagedFileTag = -1;
    g_GameStagedFileIndex = -1;
    FieldUpdateActorRenderModes();
    FieldSnapPartyToPlayer();
    g_FieldRenderContextUseOT2 = g_FieldScreenEffectEnabled ? 1 : FieldAnyActorHasFlag8000();

    for (i = 0; i < g_FieldActiveActorCount; i++) {
        pActor = (FieldActor*)(i * sizeof(FieldActor) + (s32)g_FieldActors);
        if (pActor->status & 0x40) {
            if ((pActor->pActorData->flags & 0x1000000) == 0) {
                func_800223B0(pActor->pSpriteData,
                              (s16)((u16)pActor->pActorData->rotation.vz + (u16)g_CamInterpolation.curAngleY));
            } else {
                SpriteSetDirection(pActor->pSpriteData, pActor->pActorData->rotation.vz);
            }
        }
    }
}*/
INCLUDE_ASM("asm/field/nonmatchings/main/misc3", FieldLoad);

