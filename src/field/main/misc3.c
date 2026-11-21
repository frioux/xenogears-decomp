#include "common.h"
#include "system/memory.h"
#include "field/main.h"
#include "field/actor.h"
#include "field/camera.h"
#include "psyq/libgpu.h"
#include "psyq/libgte.h"
#include "system/model.h"

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
extern s32 g_FieldDialogs;
extern s32 g_FieldWalkmesh;
extern s32 g_FieldModelFile;
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
    HeapFree(g_FieldDialogs);
    HeapFree(g_FieldCurScriptFile);
    HeapFree(g_FieldWalkmesh);
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
extern void func_80080F44(int actorIndex);
extern void func_80077268(void);
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

// The walkmesh file the loader decompresses and the views into it: its first
// word is the triangle count, then the four counts and offsets it walks, then the
// pointers the renderer reads (g_FieldWalkmeshTriangles etc.).
typedef struct {
    /* 0x00 */ void* pFile;
    /* 0x04 */ void* pSpriteData;
    /* 0x08 */ u32* pMaterialFlags;
    /* 0x0C */ void* pTriangles[4];
    /* 0x1C */ void* pVertices[4];
    /* 0x2C */ s32 unk2C[4];
    /* 0x3C */ s16 triCount;
} FieldWalkmesh; // Size: 0x3E

extern FieldWalkmesh g_FieldWalkmesh;

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

// PARKED 74.78% (3488 bytes against retail's 3484: ONE instruction over, frame
// 0xa0 and every call, branch and store in the original's order). objdiff's
// number is not the story here - the same body with the position pairs spelled
// inline (`ACTOR(i)->...;` twice per pair) measures 74.98% at 3520 bytes (9 over)
// and the same body with ONE `pActor` local for the whole loop measures 75.18%
// at 3304 bytes (45 SHORT), so the source is somewhere between: retail shares one
// `g_FieldActors` read for each of the three transform/childMatrix PAIRS and for
// the `status & 0x40` test plus the `|= 0x20` store (both reuse $a0 across the
// branch), and re-reads the base for every other store. What is left is all
// register allocation: retail's saved set is s0-s5, this body's is s0-s6, so from
// the second phase on every s-register is one off (retail $s2 = the actor byte
// offset / the offset-table pointer, here $s3).
// WHAT MATCHES BYTE-FOR-BYTE (checked instruction by instruction): the 16-byte
// rodata copy, the 0x100-byte structure copy from g_GameStagedFileBuffer to
// g_FieldSpritePlacements (both the aligned and the lwl/lwr paths - the copy type must have
// alignment 1 or 2 or cc1 drops the `or/andi 0x3` test), the 4x4 quad loop, the
// five fixed FieldInitTableQuad calls, and the whole Phase-4..12 HeapAlloc +
// FieldLZSSDecompress chain.
// FACTS ESTABLISHED THIS ROUND (keep them):
//  - misc3 owns exactly one .rodata item, D_8006FAF4 {0,-100,2000,0}: converting
//    FieldLoad DELETES it (its dlabel is in the stub's .s), so the converted body
//    must carry `const FieldVector3 D_8006FAF4 = {0, -100, 2000, 0};` at file
//    scope. It is dropped below because the parked INCLUDE_ASM would otherwise
//    define the symbol twice.
//  - 0x800B0080..0x800B00B1 is ONE object in this unit's source (the init block
//    reaches g_FieldScreenEffectParams' bytes as `0x1C($s2)` off g_FieldScreenEffectConfig and
//    the effect-setup fields as displacements too), so FieldScreenEffectConfig
//    above is one struct; misc.c's separate FieldScreenEffectSetup /
//    FieldScreenEffectParams views are the same memory.
//  - 0x800AFB18 is likewise one object (FieldWalkmesh): g_FieldSpriteData is +4,
//    g_FieldWalkmeshMaterialFlags +8, g_FieldWalkmeshTriangles +0xC (4 pointers),
//    g_FieldWalkmeshVertices +0x1C (4 pointers), the `>>1 /7` counts +0x2C,
//    g_FieldWalkmeshTriCount +0x3C. Retail re-reads `-0xC($a2)` (the file
//    pointer) per store.
//  - the actor spawn records are 8 u16 = FieldActorFileData (the loader reads
//    flags, then the rotation trio, then the position trio, then modelID, so the
//    pointer bumps +2/+6/+6 and the loop tail's +2 lands in func_80080F44's delay
//    slot). Loads are `lhu`, so the header's fields are u_short, not short.
//  - FieldLightFileData must stay 0x2A bytes: ActorFile needs 0xE of padding
//    before numEntitites (0x18C) and actorData is the inline array at 0x190.
//  - the -0x20/-0xB8 first arguments of the two FieldSetMatrix calls and the
//    RotMatrix are displacements off ONE base in retail, i.e. g_FieldModelSlotScratch
//    .. g_FieldMotionSlotSteps is one object in the source too.
INCLUDE_ASM("asm/field/nonmatchings/main/misc3", FieldLoad);

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
    for (i = 0; i < count; i++) {
        FieldLoadTIM((u_long*)((u8*)pTimData + ((s32*)pTimData)[i + 1]));
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
    pWord = (u32*)pWalk->pFile;
    pWalk->triCount = *(s32*)pWalk->pFile;
    for (i = 0; i < 4; i++) {
        pWalk->unk2C[i] = (pWord[i + 1] >> 1) / 7;
    }
    pWalk->pMaterialFlags = (u32*)((u8*)pWalk->pFile + pWord[5]);
    for (i = 0; i < pWalk->triCount; i++) {
        pWalk->pTriangles[i] = (u8*)pWalk->pFile + pWord[i * 2 + 6];
        pWalk->pVertices[i] = (u8*)pWalk->pFile + pWord[i * 2 + 7];
    }

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
    g_FieldActors = HeapAlloc(count * sizeof(FieldActor), 0);
    pWord = (u32*)g_FieldActors;
    for (i = 0; i < count * (s32)(sizeof(FieldActor) / sizeof(s32)); i++) {
        pWord[i] = 0;
    }

    for (i = 0; i < g_FieldNumActors; i++) {
        ACTOR(i)->status = *pSpawn++;
        ACTOR(i)->rotation.x = pSpawn[0];
        ACTOR(i)->rotation.y = pSpawn[1];
        ACTOR(i)->rotation.z = pSpawn[2];
        pSpawn += 3;
        pActor = ACTOR(i);
        pActor->transformMatrix.t[0] = pSpawn[0];
        pActor->childMatrix.t[0] = pSpawn[0];
        pActor = ACTOR(i);
        pActor->transformMatrix.t[1] = pSpawn[1];
        pActor->childMatrix.t[1] = pSpawn[1];
        pActor = ACTOR(i);
        pActor->transformMatrix.t[2] = pSpawn[2];
        pActor->childMatrix.t[2] = pSpawn[2];
        pSpawn += 3;
        if (ACTOR(i)->status & 0x40) {
            ACTOR(i)->status |= ACTOR_STATUS_INVISIBLE;
            ACTOR(i)->rotation.x = 0;
            ACTOR(i)->rotation.y = 0;
            ACTOR(i)->rotation.z = 0;
        } else {
            j = *pSpawn;
            pModel = HeapAlloc(sizeof(FieldActorModelData), 0);
            ACTOR(i)->pModelData = pModel;
            pModelPart = (ModelPart*)((u8*)g_FieldModelFile + ((s32*)g_FieldModelFile)[j + 1] + 0x10);
            pModel->pBounds = (FieldActorModelBounds*)pModelPart;
            ModelAllocatePackets(pModelPart, (void**)&pModel->pPrimitiveBuffers[0],
                                 (void**)&pModel->pPrimitiveBuffers[1]);
            ModelInitializePacket(pModelPart, pModel->pPrimitiveBuffers[0], (ACTOR(i)->status & 0xC) >> 2);
            memcpy(pModel->pPrimitiveBuffers[1], pModel->pPrimitiveBuffers[0], pModelPart->unk20[1].modelPacketSize);
            if (ACTOR(i)->status & 0x2000) {
                HeapChangeCurrentUser(3, 0);
                pModel->pScripts = func_800303C8(pModelPart, 0);
                HeapChangeCurrentUser(8, 0);
            }
            ModelFileTrimHeapBlock((ModelFileHeader*)pModelPart);
        }
        pSpawn++;
        func_80080F44(i);
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
    func_80077268();
    g_FieldRenderContextUseOT2 = g_FieldScreenEffectEnabled ? 1 : FieldAnyActorHasFlag8000();

    for (i = 0; i < g_FieldActiveActorCount; i++) {
        pActor = (FieldActor*)(i * sizeof(FieldActor) + (s32)g_FieldActors);
        if (pActor->status & 0x40) {
            if (pActor->pActorData->flags & 0x1000000) {
                SpriteSetDirection(pActor->pSpriteData, pActor->pActorData->rotation.vz);
            } else {
                func_800223B0(pActor->pSpriteData,
                              (s16)((u16)pActor->pActorData->rotation.vz + (u16)g_CamInterpolation.curAngleY));
            }
        }
    }
}
*/
