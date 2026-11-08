#include "common.h"
#include "system/memory.h"
#include "field/main.h"
#include "field/camera.h"
#include "psyq/libgpu.h"
#include "psyq/libgte.h"

extern void ArchiveCdDataSync(int mode);

// Light data stuff
INCLUDE_ASM("asm/field/nonmatchings/main/misc3", func_8006FDEC);

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

