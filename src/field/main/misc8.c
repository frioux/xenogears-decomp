#include "common.h"
#include "field/actor.h"
#include "field/main.h"
#include "field/text_box.h"
#include "field/effects.h"
#include "field/script_vm.h"
#include "system/memory.h"
#include "system/archive.h"
#include "system/sound.h"
#include "psyq/libetc.h"
#include "psyq/libcd.h"

extern int D_800ADBFC;
void func_8008083C(int actorIndex) {
    ActorData* pActor;

    if (actorIndex < D_800ADBFC) {
        pActor = g_FieldActors[actorIndex].pActorData;
        if (pActor->flags134 & 0x80) {
            HeapFree(pActor->unk110);
        }
        if (pActor->flags12C_0xD) {
            HeapFree(pActor->unk114);
        }
        if (g_FieldActors[actorIndex].status & 0x2000) {
            HeapFree(pActor->unk118);
        }
        if (pActor->unk124 != -1) {
            HeapFree(pActor->unk120);
        }
        HeapFree(pActor);
        HeapFree(g_FieldActors[actorIndex].pShadow);
        func_800230A8(g_FieldActors[actorIndex].pSpriteData);
    }
}

typedef struct {
    /* 0x0 */ undefined8 unk0[0xC];
    /* 0xC */ u8 material;
    /* 0xD */ undefined8 padD;
} FieldWalkmeshTriangle; // Size: 0xE

extern FieldWalkmeshTriangle* D_800AFB24[];
extern u_int* D_800AFB20;

u_int func_80080968(ActorData* pActor) {
    if ((pActor->flags >> (pActor->walkmeshId + 3)) & 1) {
        return 0;
    }
    return D_800AFB20[D_800AFB24[pActor->walkmeshId][pActor->walkmeshTriIds[pActor->walkmeshId]].material];
}

extern u_short D_800B14AC;

INCLUDE_ASM("asm/field/nonmatchings/main/misc8", func_800809D0);
/* 85.00%: pre-loop `acc = 0; acc += value` is folded by gcc into a copy of the
   constant (no `addu a0,zero,zero`), so acc lands in a1 instead of a0.
int func_800809D0(s32* pData) {
    int value = -0x14D000;
    int step = pData[7];
    int acc = 0;

    D_800B14AC = 0;
    acc += value;
    do {
        D_800B14AC += 2;
        value += step;
        acc += value;
    } while (value <= 0);
    acc -= value;
    return acc >> 16;
}
*/

extern s32 D_800ADB58;
extern s32 D_800ADB5C;

u_int func_80080A18(void) {
    ActorData* pActor = g_FieldActors[D_800ADB58].pActorData;

    return ((u_int*)pActor->unk118)[D_800ADB5C++];
}

INCLUDE_ASM("asm/field/nonmatchings/main/misc8", func_80080A74);

INCLUDE_ASM("asm/field/nonmatchings/main/misc8", func_80080F44);

INCLUDE_ASM("asm/field/nonmatchings/main/misc8", func_8008110C);

INCLUDE_ASM("asm/field/nonmatchings/main/misc8", func_800815F0);

INCLUDE_ASM("asm/field/nonmatchings/main/misc8", func_80081C54);

int func_80081F5C(ActorData* pActor) {
    u_int actorType = (pActor->scriptFlags.flags >> 9) & 3;

    return -((actorType & (pActor->curWalkmeshTriMaterial >> 3)) != 0);
}

INCLUDE_ASM("asm/field/nonmatchings/main/misc8", func_80081F80);

INCLUDE_ASM("asm/field/nonmatchings/main/misc8", func_800821F4);

INCLUDE_ASM("asm/field/nonmatchings/main/misc8", func_8008237C);

INCLUDE_ASM("asm/field/nonmatchings/main/misc8", func_80082494);

INCLUDE_ASM("asm/field/nonmatchings/main/misc8", func_800825AC);

INCLUDE_ASM("asm/field/nonmatchings/main/misc8", func_80082620);

INCLUDE_ASM("asm/field/nonmatchings/main/misc8", func_80082BB8);

int func_80083178(int angle, int limit) {
    if (angle < 0) {
        angle += 0x4000;
        limit = -limit;
        if (angle < limit) {
            angle = limit;
        }
        if (angle > 0) {
            angle = 0;
        }
    } else {
        angle -= 0x4000;
        if (limit < angle) {
            angle = limit;
        }
        if (angle < 0) {
            angle = 0;
        }
    }
    return angle;
}

void func_800831D0(SVECTOR* pDst, IVEC3* pSrc) {
    pDst->vx = pSrc->x >> 0x10;
    pDst->vy = pSrc->y >> 0x10;
    pDst->vz = pSrc->z >> 0x10;
}

INCLUDE_ASM("asm/field/nonmatchings/main/misc8", func_800831F4);

INCLUDE_ASM("asm/field/nonmatchings/main/misc8", func_80083288);

void func_80083994(void) {
}

INCLUDE_ASM("asm/field/nonmatchings/main/misc8", func_8008399C);

INCLUDE_ASM("asm/field/nonmatchings/main/misc8", func_80084158);

INCLUDE_ASM("asm/field/nonmatchings/main/misc8", func_8008492C);

INCLUDE_ASM("asm/field/nonmatchings/main/misc8", func_80084A40);

INCLUDE_ASM("asm/field/nonmatchings/main/misc8", func_800854D0);

extern s32 D_800ADB2C;
extern void* D_800ADBB8;
extern void* D_800AFEA4;

INCLUDE_ASM("asm/field/nonmatchings/main/misc8", func_80085560);
/* 81.88%: only the prologue schedule and the temp register differ (gcc stores
   the stream file from a1 in the original, from v0 here).
void func_80085560(int archiveIndex, int unused, void* pCallback) {
    void* pStreamFile;

    D_800ADB2C = 1;
    pStreamFile = ArchiveAllocStreamFile(8);
    D_800ADBB8 = pStreamFile;
    ArchiveReadFileToBuffer(archiveIndex, pStreamFile, 0, 0x100);
    D_800AFEA4 = pCallback;
}
*/

void func_8003A20C(int);
void func_80039F9C(void*, int, int, int);

INCLUDE_ASM("asm/field/nonmatchings/main/misc8", func_800855C8);

extern s32 D_800B21B8;

void func_80085634(int arg0, int arg1) {
    int offset = arg1 & 7;

    if (arg0 == 0) {
        func_8003A20C(offset * 2);
    } else {
        D_800B21B8 = arg0;
        func_800855C8(arg0, 0x7F, 0x40, offset);
    }
}

INCLUDE_ASM("asm/field/nonmatchings/main/misc8", func_80085678);

extern s16 D_800C3A38;
extern void* D_800B235C;

void func_80085738(void) {
    if (D_800C3A38 != 0xFF) {
        func_80039FF8();
        func_8003852C(D_800B235C);
        HeapFree(D_800B235C);
    }
}

INCLUDE_ASM("asm/field/nonmatchings/main/misc8", func_80085788);

INCLUDE_ASM("asm/field/nonmatchings/main/misc8", func_80085890);

extern void* D_8006259C;
extern s32 D_8004F32C;

void func_80085988(void) {
    func_8003852C(D_8006259C);
    HeapUnpinBlock(D_8006259C);
    HeapFree(D_8006259C);
    D_8004F32C = -1;
}

INCLUDE_ASM("asm/field/nonmatchings/main/misc8", func_800859DC);

INCLUDE_ASM("asm/field/nonmatchings/main/misc8", func_80085B20);

int func_80085C3C(void) {
    int i;

    for (i = 0; i < 5; i++) {
        if (func_800854D0() == -1) {
            return 0;
        }
    }
    return -1;
}

INCLUDE_ASM("asm/field/nonmatchings/main/misc8", func_80085C90);

extern AudioManager* D_8004F2FC;

void func_80085EEC(void) {
    if (D_8004F2FC != NULL) {
        func_80039C4C(D_8004F2FC);
        func_800399D4(D_8004F2FC);
        D_8004F2FC = NULL;
    }
}

extern s32 D_8004F364;
extern s32 D_8004F368;
extern s16 D_8004F384;
extern s32 D_80059560;
extern SoundWDSEntry* D_8006251C;
extern void* D_800B00E0; // WDS File Buffer

int func_80085F30(void) {
    void* pWdsEntry;

    if (ArchiveDataSync()) {
        return -1;
    }
    
    pWdsEntry = SoundLoadWdsFile(D_800B00E0, 0);
    D_8006251C = pWdsEntry;
    D_80059560 = pWdsEntry;
    func_8003BDFC(0x10);
    HeapFree(D_800B00E0);
    D_8004F364 = 1;
    D_8004F384 = 0;
    D_8004F368 = 0;
    return 0;
}

void func_80085FB8(void) {
    void* pWdsFileBuffer;

    ArchiveSetIndex(0x1C, 0x0);
    pWdsFileBuffer = HeapAlloc(ArchiveDecodeAlignedSize(3), 1);
    D_800B00E0 = pWdsFileBuffer;
    ArchiveReadFileToBuffer(3, pWdsFileBuffer, 0, CdlModeSpeed);
    ArchiveSetIndex(4, 0);
    D_8004F364 = 0x80;
}

void func_80086024(void) {
    if (D_8004F368 == 0) {
        D_8004F384 = 1;
        SoundFreeWdsEntry(D_8006251C);
        D_8004F368 = 1;
    }
    D_8004F364 = 0;
}


INCLUDE_ASM("asm/field/nonmatchings/main/misc8", func_80086078);

INCLUDE_ASM("asm/field/nonmatchings/main/misc8", func_800860F0);

INCLUDE_ASM("asm/field/nonmatchings/main/misc8", FieldActorWorldToScreenPosition);

INCLUDE_ASM("asm/field/nonmatchings/main/misc8", func_800862CC);

INCLUDE_ASM("asm/field/nonmatchings/main/misc8", func_800863E8);

// Three 6-byte entries; splat names the second field's address D_800AFE8A.
typedef struct {
    /* 0x0 */ u_short unk0;
    /* 0x2 */ u_short unk2;
    /* 0x4 */ u_short unk4;
} FieldUnk800AFE88; // Size: 0x6

extern FieldUnk800AFE88 D_800AFE88[3];
extern FieldUnk800AFE88 D_800AFE8A[3];

int func_80086470(int arg0, u_int arg1) {
    int i;

    if (arg0 == -1) {
        return -1;
    }
    for (i = 0; i < 3; i++) {
        if (D_800AFE88[i].unk0 == arg1) {
            return i;
        }
    }
    return -1;
}

void func_800864B4(void) {
    int i;

    for (i = 0; i < 3; i++) {
        D_800AFE88[i].unk0 = 0xFFFF;
        D_800AFE8A[i].unk0 = 0xFFFF;
    }
}

INCLUDE_ASM("asm/field/nonmatchings/main/misc8", func_800864F0);

INCLUDE_ASM("asm/field/nonmatchings/main/misc8", func_80086590);

INCLUDE_ASM("asm/field/nonmatchings/main/misc8", func_80086908);
/*
extern VECTOR g_CameraEye;
extern VECTOR g_CameraAt;
extern s16 D_800B22E0;
extern s32 g_PlayerActorIndex;

void func_80086908(void) {
    switch (D_800B22E0) {
        case 0:
            func_80086590(&g_FieldActors[g_PlayerActorIndex].pActorData->position);
            return;
        case 1:
            func_80086590(&g_CameraEye);
            return;
        case 2:
            func_80086590(&g_CameraAt);
            return;
    }
}
*/

void FieldScriptVM2Run(void) {
    char *script = (char *)g_FieldScriptVMCurScriptData;
    size_t vmIP = ++(g_FieldScriptVMCurActor->scriptInstructionPointer);

    ScriptVMHandler *handler = &g_FieldScriptVMHandlers2[script[vmIP]];
    (*handler)();
}

INCLUDE_ASM("asm/field/nonmatchings/main/misc8", func_80086A1C);

INCLUDE_ASM("asm/field/nonmatchings/main/misc8", func_80086BA8);

INCLUDE_ASM("asm/field/nonmatchings/main/misc8", func_80086C34);

void GameSoftReset(void);

void func_80086D4C(void) {
    GameSoftReset();
    D_800B00C0 = 1;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 1;
}
