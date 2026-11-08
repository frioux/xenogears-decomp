#include "common.h"
#include "system/memory.h"
#include "field/main.h"
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

