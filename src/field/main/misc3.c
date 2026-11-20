#include "common.h"
#include "system/memory.h"
#include "field/main.h"
#include "field/actor.h"
#include "field/camera.h"
#include "psyq/libgpu.h"
#include "psyq/libgte.h"

extern void ArchiveCdDataSync(int mode);

INCLUDE_ASM("asm/field/nonmatchings/main/misc3", func_8006FDEC);

// Best attempt, 93.66% at 660 bytes against retail's 672 (three instructions
// short). Structure and every call match; two sched2/allocator spots differ.
//  1. The two 0x14-byte light copies (`pLights[1] = pLights[0]`) are hoisted ten
//     instructions above the third light's `sh ... D_800AFB00`: retail's copy
//     loads land in $v0/$v1/$a2/$a3 and so cannot move above the `sh $v0`, our
//     compile gives them $v1/$a2/$a3/$t0 and they float up (retail's two
//     load-delay `nop`s go with them). Making the copy source or destination
//     volatile does not help (a volatile source turns the copy into two memcpy
//     calls).
//  2. The three trailing ambient reads: our compile hoists the third `lhu` into
//     the second one's load-delay slot with a fresh $v1; retail reads all three
//     into $v0 with `nop` after each. Keeping the `color` temp for all three is
//     what gets the rest of the shape right (writing the shift straight into the
//     globals loses an instruction: 89.37%).
// Levers that DID land, and are in the parked body: give each light its own
// pointer local (`pLight1 = &pLights[1]`) and use it for BOTH the `vx` store and
// the func_80030A30 argument - that is what makes the store `sw $v0,332($s1)`
// and the argument `addu` instead of a rematerialised `lui $at` + `sw
// %lo(g_Scene+332)` (92.40 -> 93.66). Facts: the three 0x14-byte lights sit
// directly after g_Scene at +0x138/+0x14C/+0x160 (g_WorldScale is g_Scene+0x134)
// and the ambient colours at +0x174/+0x176/+0x178; the camera vectors retail
// reaches as `addiu $a1,$s1,-0x110` are g_CameraEye/At/Up at g_Scene-0x110,
// -0x100, -0xF0, and plain &g_CameraEye gives a separate lui/addiu pair that
// cannot match, so the casts off &g_Scene stay.
/*
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
    *(volatile s16*)&D_800AFB00 = (u_short)*pData++ << 3;
    pData++;

    pLights[1] = pLights[0];
    pLights[2] = pLights[0];
    func_80030A30(2, pLight2);

    color = (u_short)pData[0] << 4;
    D_800AFB04 = color;
    color = (u_short)pData[1] << 4;
    D_800AFB06 = color;
    color = (u_short)pData[2] << 4;
    D_800AFB08 = color;
    pData += 3;

    SetRotMatrix(&g_Scene.viewMatrix);
    SetTransMatrix(&g_Scene.viewMatrix);
    RotTrans(&g_Scene.worldTranslation, &g_Scene.worldToScreenMatrix.t, &flag);
    func_80030B14(&g_Scene.worldToScreenMatrix);
    SetRotMatrix(&g_Scene.worldToScreenMatrix);
    SetTransMatrix(&g_Scene.worldToScreenMatrix);
}
*/




void FieldLZSSDecompress(void* _unused, void* pCompressed, void* pDecompressed) {
    LZSSDecompress(pCompressed, pDecompressed);
}

INCLUDE_ASM("asm/field/nonmatchings/main/misc3", FieldFree);

// Best attempt, 98.66% at 652 bytes against retail's 656 (one instruction short).
// The body is otherwise identical: ResetGraph, WorkListsFreeAllEntries, the
// two-pass render context loop, the actor loop with status & 0x40 / 0x2000 and
// its four frees, the eight HeapFrees, the line scroll loop and the sound list
// teardown. The single remaining difference is the render context loop counter:
// retail emits `addiu $v0,$s1,1` then `move $s1,$v0` after the
// g_FieldCurRenderContextIndex load, our compile a single `addiu $s1,$s1,1`
// which reorg steals into the previous `jal GfxFlushImageList` delay slot (retail
// has a `nop` there). Every spelling measured the same: `++i` inside the
// argument, `(i = i + 1)`, `next = i + 1; i = next;` plus `i` in the argument
// (all coalesce to one addiu), separate `i++`, `i += 1`. A non-void return type
// did not bring retail's `nop` back either. This is the two-live-copies lever
// and it needs the permuter; the permuter cannot parse the body as written
// (base.c does not compile), so it is unrun here.
// Two levers that DID land, and are in the parked body:
//  - g_FieldActorModelSlotCount wants the unbounded-ARRAY extern
//    (`extern s32 g_FieldActorModelSlotCount[];` + `pCount[0] = 0`), which
//    materialises &symbol in $s0 once (`lui`/`addiu`) and reuses it for the read
//    and the store; the scalar declaration rematerialises `lui`+`lw %lo` /
//    `lui at`+`sw %lo` instead (this alone took 97.34 -> 98.66).
//  - the actor loop must index as `(i * sizeof(FieldActor) + (int)g_FieldActors)`
//    for the original's index-first `addu`, with one status read feeding both
//    masks; g_FieldLineScrolls is 0x144 bytes, same layout as misc11.c.
/*
extern s32 g_FieldActorModelSlotCount[];
extern void* g_FieldArchiveQueueBuffer;
extern s32 D_800ADBF0;
extern s32 D_800AFB18;
extern s32 D_800AFB14;
extern s16 g_FieldScreenEffectEnabled;
extern s32 g_FieldScreenEffectData;

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
    void* pModelData;
    u_short status;
    int i;
    int next;

    ResetGraph(1);
    WorkListsFreeAllEntries();

    i = 0;
    do {
        GfxFlushImageList();
        DrawSync(0);
        next = i + 1;
        i = next;
        GfxSetRenderContext((g_FieldCurRenderContextIndex + i) & 1);
        GfxFlushImageList();
        DrawSync(0);
        GfxFreeWorkBuffers();
    } while (i < 2);

    for (i = 0; i < g_FieldNumActors; i++) {
        FieldFreeActorData(i);
        pActor = (FieldActor*)(i * sizeof(FieldActor) + (int)g_FieldActors);
        status = *(u_short*)&pActor->status;
        if (!(status & 0x40)) {
            pModelData = pActor->pModelData;
            if (status & 0x2000) {
                ModelRestorePartVertices(*(void**)((u8*)pModelData + 0x14));
            }
            ModelPartFreeLightData(*(void**)((u8*)pModelData + 0x4));
            HeapFree(*(void**)((u8*)pModelData + 0x8));
            HeapFree(g_FieldActors[i].pModelData);
        }
    }

    FieldDistortionFree();
    HeapFree(g_FieldActors);
    HeapFree(g_pFieldTriggerZones);
    HeapFree(D_800ADBF0);
    HeapFree(g_FieldCurScriptFile);
    HeapFree(D_800AFB18);
    HeapFree(D_800AFB14);
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
*/




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

INCLUDE_ASM("asm/field/nonmatchings/main/misc3", func_800705DC);

extern s16 D_800B06A4[3][3];
extern s16 D_800B06A6[3][3];
extern s32 D_800ADB0C;

void FieldResetPortraitSlots(void) {
    int i;

    for (i = 0; i < 3; i++) {
        D_800B06A4[i][0] = 0xFF;
        D_800B06A6[i][0] = 0xFF;
    }

    D_800ADB0C = 0;
}

INCLUDE_ASM("asm/field/nonmatchings/main/misc3", FieldLoad);

