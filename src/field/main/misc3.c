#include "common.h"
#include "system/memory.h"
#include "field/main.h"
#include "field/actor.h"
#include "field/camera.h"
#include "psyq/libgpu.h"
#include "psyq/libgte.h"

extern void ArchiveCdDataSync(int mode);

INCLUDE_ASM("asm/field/nonmatchings/main/misc3", func_8006FDEC);

// Best attempt (93.61%): every instruction matches except that gcc's scheduler hoists
// the two 0x14-byte light copies above the `sh` to D_800AFB00 (10 instructions moved),
// and the three trailing ambient reads need pointer increments here where the original
// indexes 0/2/4 off a fixed base without them. Also learned: the three model lights sit
// directly after g_Scene in .bss (asm/field/data/3FAFC.bss.s) and the camera vectors sit
// before it, and the original reaches all of them as byte offsets off one base register
// holding &g_Scene - `addiu $a1,$s1,-0x110`, `sw $v0,0x138($s1)` - so the C has to spell
// them that way (plain &g_CameraEye gives a separate lui/addiu pair and cannot match).
// Retried: the trailing ambient reads as `pData[0]`, `pData[1]`, `pData[2]` do give the
// original's `lhu 0/2/4($s0)` with no increments, but gcc then hoists the third `lhu`
// into the previous load-delay `nop` (into $v1) and the 0x14-byte copies still hoist above
// the `sh` to D_800AFB00, so the indexed form scores no better (91.6-92.4%). The copies
// are the real blocker: our compile batches 4 loads + 4 stores per copy with $t0 as a
// fourth scratch where retail interleaves them right after the $at store.
/*
typedef struct {
     s32 vx;
     s32 vy;
     s32 vz;
     s16 r;
     s16 g;
     s16 b;
     undefined16 unk12;
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

// Light data stuff
void func_8006FDEC(s16* pData) {
    FieldLight* pLights;
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
    pLights[0].vx = *pData++;
    D_800AFACC = *pData++;
    D_800AFAD0 = *pData++;
    pData++;
    D_800AFAD4 = (u_short)*pData++ << 3;
    D_800AFAD6 = (u_short)*pData++ << 3;
    D_800AFAD8 = (u_short)*pData++ << 3;
    pData++;
    func_80030A30(0, &pLights[0]);

    pLights[1].vx = *pData++;
    D_800AFAE0 = *pData++;
    D_800AFAE4 = *pData++;
    pData++;
    D_800AFAE8 = (u_short)*pData++ << 3;
    D_800AFAEA = (u_short)*pData++ << 3;
    D_800AFAEC = (u_short)*pData++ << 3;
    pData++;
    func_80030A30(1, &pLights[1]);

    pLights[2].vx = *pData++;
    D_800AFAF4 = *pData++;
    D_800AFAF8 = *pData++;
    pData++;
    D_800AFAFC = (u_short)*pData++ << 3;
    D_800AFAFE = (u_short)*pData++ << 3;
    D_800AFB00 = (u_short)*pData++ << 3;
    pData++;

    pLights[1] = pLights[0];
    pLights[2] = pLights[0];
    func_80030A30(2, &pLights[2]);

    color = (u_short)*pData++ << 4;
    D_800AFB04 = color;
    color = (u_short)*pData++ << 4;
    D_800AFB06 = color;
    color = (u_short)*pData++ << 4;
    D_800AFB08 = color;

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

// Best attempt, 97.34%: the body is structurally right (ResetGraph,
// WorkListsFreeAllEntries, the two-pass render context loop, the actor loop with
// status & 0x40 / 0x2000 and its four frees, the eight HeapFrees, the line scroll
// loop and the sound list teardown) and only two spots differ.
//  1. The render context loop counter: the original emits `addiu $v0,$s1,1` and
//     then a `move $s1,$v0` copy after the g_FieldCurRenderContextIndex load;
//     every spelling tried here (separate `i++`, `i += 1`, `++i` inside the
//     argument, `context = ++i`, assigning through a temp) gives a single
//     `addiu $s1,$s1,1` that gcc's scheduler then hoists into the earlier call's
//     delay slot, so this is a combine/scheduling choice, not the expression.
//  2. D_800B2264: the original materialises `&D_800B2264` into $s0 with
//     lui+addiu once and reuses it across LibGearShutdown/HeapFree/
//     FieldRenderSyncAndFlush, gcc rematerialises `lui` + `lw %lo` for the read
//     and `lui at` + `sw %lo` for the store instead. An explicit pointer local,
//     hoisting that local above FontFree, and plain symbol read/write all give
//     the same rematerialised form.
//  3. Tried `tools/scripts/try_flags.sh field/main/misc3 FieldFree --gp -G0`: 98.66%,
//     so the -G0/gp lead is not what closes this one either; the D_800B2264 remat and
//     the missing `move` both survive it.
// Facts recovered for whoever retries: the unit needs field/actor.h for
// g_FieldActors/g_FieldNumActors/g_FieldCurScriptFile/g_FieldSpriteData; the
// actor loop must index as `(i * sizeof(FieldActor) + (int)g_FieldActors)` to get
// the original's index-first `addu`, with one status read feeding both masks; the
// line scroll table at D_800AFEA8 (0x144 bytes, same layout as misc11.c) and the
// `lh -0x4` count loop match byte for byte.
/*
extern s32 D_800B2264;
extern void* D_800ADB20;
extern s32 D_800ADBF0;
extern s32 D_800AFB18;
extern s32 D_800AFB14;
extern s16 D_800B00B2;
extern s32 D_800B007C;

extern void WorkListsFreeAllEntries(void);
extern void func_80025044(void);
extern void func_800250E0(int context);
extern void GfxFreeWorkBuffers(void);
extern void func_8008083C(int actorIndex);
extern void func_800306D0(void* pBackup);
extern void ModelPartFreeLightData(void* pModelPart);
extern void FieldDistortionFree(void);
extern void GfxLineScrollFree(void* pLineScroll);
extern void FontFree(void);
extern void LibGearShutdown(void);
extern void FieldRenderSyncAndFlush(void);
extern void func_800A83B4(void);
extern void func_80027D40(void* pData);

typedef struct {
    s16 numLineScrolls;
    s16 unk2;
    void* pLineScrolls[32]; // 0x4
    u8* pDatas[32];         // 0x84
    s16 lengths[32];        // 0x104
} LineScrollTable; // Size: 0x144

extern LineScrollTable D_800AFEA8;

void FieldFree(void) {
    FieldActor* pActor;
    s32* pCount;
    void* pModelData;
    u_short status;
    int i;

    ResetGraph(1);
    WorkListsFreeAllEntries();

    i = 0;
    do {
        func_80025044();
        DrawSync(0);
        func_800250E0((g_FieldCurRenderContextIndex + ++i) & 1);
        func_80025044();
        DrawSync(0);
        GfxFreeWorkBuffers();
    } while (i < 2);

    for (i = 0; i < g_FieldNumActors; i++) {
        func_8008083C(i);
        pActor = (FieldActor*)(i * sizeof(FieldActor) + (int)g_FieldActors);
        status = *(u_short*)&pActor->status;
        if (!(status & 0x40)) {
            pModelData = pActor->pModelData;
            if (status & 0x2000) {
                func_800306D0(*(void**)((u8*)pModelData + 0x14));
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

    if (D_800B00B2 != 0) {
        func_80027D40(D_800B007C);
    }

    for (i = 0; i < D_800AFEA8.numLineScrolls; i++) {
        GfxLineScrollFree(D_800AFEA8.pLineScrolls[i]);
        HeapFree(D_800AFEA8.pDatas[i]);
        HeapFree(D_800AFEA8.pLineScrolls[i]);
    }

    FontFree();
    pCount = &D_800B2264;
    D_800AFEA8.numLineScrolls = 0;
    if (*pCount != 0) {
        LibGearShutdown();
        HeapFree(D_800ADB20);
        FieldRenderSyncAndFlush();
    }
    *pCount = 0;
    HeapFreeBlocksWithFlag(3);
    func_800A83B4();
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

extern s32 D_800ADB60; // Field script data stream allocated?
extern int* D_800ADC14; // Field script data stream
extern s32 g_GameSceneMapNum;

extern int* ArchiveAllocStreamFile(int numEntries, int allocMode);
extern void func_80029EB0(int, int*, int, int, short, short, short, short, short, short);

extern void func_80078C5C(void);

void func_80070488(void) {
    int* pStreamFile;

    if (!D_800ADB60) {
        D_800ADB60 = 1;
        pStreamFile = ArchiveAllocStreamFile(4, 1);
        D_800ADC14 = pStreamFile;
        func_80029EB0(((g_GameSceneMapNum & 0xFFF) * 2) + 0xB9, pStreamFile, 0, 0, 0, 0, 0, 0, 0, 0);
    }
}

void func_80070508(void) {
    if (D_800ADB60 == 1) {
        ArchiveCdDataSync(0);
        DrawSync(0);
        HeapFree(D_800ADC14);
        D_800ADB60 = 0;
    }

    func_80078C5C();
}

void func_80070560(VECTOR* pDest, SVECTOR* pSrc) {
    pDest->vx = pSrc->vx << 16;
    pDest->vy = pSrc->vy << 16;
    pDest->vz = pSrc->vz << 16;
}

void func_80070594(MATRIX* dest) {
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

void func_80070C84(void) {
    int i;

    for (i = 0; i < 3; i++) {
        D_800B06A4[i][0] = 0xFF;
        D_800B06A6[i][0] = 0xFF;
    }

    D_800ADB0C = 0;
}

INCLUDE_ASM("asm/field/nonmatchings/main/misc3", FieldLoad);

