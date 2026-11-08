#include "common.h"
#include "field/actor.h"
#include "field/main.h"
#include "field/text_box.h"
#include "field/effects.h"
#include "field/script_vm.h"
#include "system/math.h"
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

// Four 2D trigger-zone corners packed one per short pair as (z << 16) + x.
typedef struct {
    /* 0x0 */ s16 z0;
    /* 0x2 */ s16 x0;
    /* 0x4 */ s16 z1;
    /* 0x6 */ s16 x1;
    /* 0x8 */ s16 z2;
    /* 0xA */ s16 x2;
    /* 0xC */ s16 z3;
    /* 0xE */ s16 x3;
} FieldZone2D; // Size: 0x10

int func_80082494(IVEC3* pOffset, ActorData* pActor) {
    long actorPosition2D;
    long triggerPos1;
    long triggerPos2;
    long triggerPos3;
    long triggerPos4;
    FieldZone2D* pZone;

    if (!pActor->flags12C_0xD) {
        return 0;
    }
    actorPosition2D = (CONV_TO_GTE(pActor->position.vx + pOffset->x) << 0x10) + CONV_TO_GTE(pActor->position.vz + pOffset->z);
    pZone = pActor->unk114;
    triggerPos1 = (pZone->z0 << 0x10) + pZone->x0;
    triggerPos2 = (pZone->z1 << 0x10) + pZone->x1;
    triggerPos3 = (pZone->z2 << 0x10) + pZone->x2;
    triggerPos4 = (pZone->z3 << 0x10) + pZone->x3;
    if (NormalClip(triggerPos1, triggerPos2, actorPosition2D) < 0 ||
        NormalClip(triggerPos2, triggerPos3, actorPosition2D) < 0) {
        return -1;
    }
    if (NormalClip(triggerPos3, triggerPos4, actorPosition2D) >= 0) {
        return NormalClip(triggerPos4, triggerPos1, actorPosition2D) >> 31;
    }
    return -1;
}

long FieldGetVec2Magnitude(long, long);

int func_800825AC(int actorIndexA, int actorIndexB) {
    FieldActor* pActors = g_FieldActors;
    ActorData* pActorB = pActors[actorIndexB].pActorData;
    ActorData* pActorA = pActors[actorIndexA].pActorData;
    int posXB = CONV_TO_GTE(pActorB->position.vx);
    int posZB = CONV_TO_GTE(pActorB->position.vz);
    int posXA = CONV_TO_GTE(pActorA->position.vx);
    int posZA = CONV_TO_GTE(pActorA->position.vz);

    return FieldGetVec2Magnitude(posXB - posXA, posZB - posZA);
}

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

void func_80081F80(int, int);

INCLUDE_ASM("asm/field/nonmatchings/main/misc8", func_800831F4);
/* 92.68%: an unused 8-byte local (`RECT rect;`) reproduces the 0x20 frame, but
   gcc keeps the loaded rotation in v1 (original loads v0 and copies to v1) and
   sinks the second `sh 0x106` into the call delay slot.
void func_800831F4(int arg0, ActorData* pActor, int arg2, int arg3) {
    short rotation;
    RECT rect;

    if (pActor->moveModified.vx != 0 || pActor->moveModified.vz != 0) {
        if (!(pActor->rotation.vy & 0x8000)) {
            rotation = pActor->rotation.vy;
            rotation += (arg3 & 1) ? -0x400 : 0x400;
            rotation &= 0xFFF;
            pActor->rotation.vx = rotation;
            pActor->rotation.vy = rotation;
            func_80081F80(arg0, pActor->rotation.vx);
            rotation = pActor->rotation.vy | 0x8000;
            pActor->rotation.vx = rotation;
            pActor->rotation.vy = rotation;
        }
    }
}
*/

INCLUDE_ASM("asm/field/nonmatchings/main/misc8", func_80083288);

void func_80083994(void) {
}

INCLUDE_ASM("asm/field/nonmatchings/main/misc8", func_8008399C);

INCLUDE_ASM("asm/field/nonmatchings/main/misc8", func_80084158);

INCLUDE_ASM("asm/field/nonmatchings/main/misc8", func_8008492C);
/* 91.16%: the whole chain of early rejections matches; the tail differs only in
   block placement (gcc merges this `return -1` into the epilogue with a
   `li v0,-1` delay slot, the original keeps it as its own block, which in turn
   lets the final `return 0` merge into the epilogue).
   extern s32 D_800ADB98;
   extern s32 D_800ADC0C;

   int func_8008492C(ActorData* pActor) {
       if (pActor->curWalkmeshTriMaterial & 0x420000) {
           return -1;
       }
       if (D_800ADB98 != 0) {
           return -1;
       }
       if (pActor->moveModified.vx != 0) {
           return -1;
       }
       if (pActor->moveModified.vy != 0) {
           return -1;
       }
       if (pActor->moveModified.vz != 0) {
           return -1;
       }
       if (D_800ADC0C != 1) {
           return -1;
       }
       if (pActor->canInteract != 0xFF) {
           return -1;
       }
       if (pActor->scriptFlags.flags & 0x401800) {
           return -1;
       }
       if (pActor->flags & 1) {
           if (pActor->walkmeshId == 0) {
               return -1;
           }
       }
       if (pActor->flags & 2) {
           if (pActor->walkmeshId == 1) {
               return -1;
           }
       }
       if (pActor->flags & 4) {
           return -(pActor->walkmeshId == 2);
       }
       return 0;
   }
*/

INCLUDE_ASM("asm/field/nonmatchings/main/misc8", func_80084A40);

extern s32 D_800ADBBC;
extern s32 D_800ADB2C;
extern void* D_800ADBB8;
extern void* D_800AFEA4;
int func_80028B14(void);

INCLUDE_ASM("asm/field/nonmatchings/main/misc8", func_800854D0);
/* 96.94%: only the call result's register differs (original copies it to a0
   with `move a0,v0` and stores/tests a0; gcc here keeps it in v0).
int func_800854D0(void) {
    int result = func_80028B14();

    D_800ADBBC = result;
    if (result != 0) {
        ((void (*)(void))D_800AFEA4)();
        return 0;
    }
    if (ArchiveDataSync() != 0) {
        return 0;
    }
    if (D_800ADBBC != 0) {
        return 0;
    }
    HeapFree(D_800ADBB8);
    D_800ADB2C = 0;
    return -1;
}
*/

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
/* 85.56%: gcc hoists `(arg3 & 7) * 2` into s0 and copies it (move a0,s0 /
   move a1,s0); the original keeps `arg3 & 7` in s0 and shifts at each use.
void func_800855C8(int arg0, int arg1, int arg2, int arg3) {
    func_8003A20C((arg3 & 7) * 2);
    func_80039F9C(arg0, (arg3 & 7) * 2, (s16)arg1, (s16)arg2);
}
*/

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

extern u_short D_800C3A2C;
extern s32 D_800C3A64;
extern s32 D_800B06A0;
extern s16 D_800C3A38;
extern SoundFile* D_800B235C;
void func_80039EC4(s32 arg0, s32 arg1);

typedef struct {
    /* 0x0 */ u_short unk0;
    /* 0x2 */ u_short unk2;
} FieldUnk800AE060; // Size: 0x4

extern FieldUnk800AE060 D_800AE060[];

void func_80085678(void) {
    u_short* pDuration;
    u_short* pValue;
    u_short value;

    if (D_800C3A38 == 0xFF) {
        return;
    }
    pDuration = &D_800AE060[0].unk0;
    pValue = &D_800AE060[0].unk2;
loop:
    if (D_800B06A0 < pDuration[D_800C3A64 * 2] + D_800C3A2C) {
        return;
    }
    value = pValue[D_800C3A64 * 2];
    func_80039EC4((value & 0xFF) | (D_800B235C->sedId << 16), (value >> 7) & 0xE);
    D_800C3A64++;
    goto loop;
}

void func_80085738(void) {
    if (D_800C3A38 != 0xFF) {
        func_80039FF8();
        func_8003852C(D_800B235C);
        HeapFree(D_800B235C);
    }
}

INCLUDE_ASM("asm/field/nonmatchings/main/misc8", func_80085788);
/* 85.42%: the inner scan matches (pointer + index incremented together) but
   gcc inverts the inner loop (test at both ends, `bne` back edge) where the
   original keeps the test at the top and a `j` back edge; that also makes gcc
   hoist `streamId + 1` out of the outer loop into s0 and shuffle the base
   pointer/0xFFFF into t0/a3 instead of a3/a2.
void func_80085788(void) {
    int count;
    int i;
    int streamId;
    FieldUnk800AE060* pEntry;

    streamId = D_800C3A38;
    if (streamId == 0xFF) {
        return;
    }
    func_80039FF8();
    ArchiveSetIndex(0x1C, 0);
    D_800B235C = HeapAlloc(ArchiveDecodeAlignedSize(streamId + 0x115), 1);
    ArchiveReadFileToBuffer(streamId + 0x115, D_800B235C, 0, 0x80);
    ArchiveCdDataSync(0);
    SoundAddSedsEntry(D_800B235C);
    func_8003BDFC(0x10);
    ArchiveSetIndex(4, 0);
    i = 0;
    for (count = 0; count < streamId + 1; count++) {
        pEntry = &D_800AE060[i];
        while (pEntry->unk0 != 0xFFFF) {
            pEntry++;
            i++;
        }
        i++;
        D_800C3A64 = i;
    }
}
*/

extern void* D_8006259C;
extern s32 D_8004F32C;
extern void* D_8005A4BC;
void func_8003BDFC(int arg0);
void SoundAddSedsEntry(SoundFile* pSoundFile);

void func_80085890(void) {
    int size;

    ArchiveSetIndex(4, 0);
    size = ArchiveDecodeAlignedSize(0xA8);
    D_8006259C = HeapAlloc(size, 0);
    HeapPinBlock(D_8006259C);
    if (D_8004F32C == -1) {
        ArchiveReadFileToBuffer(0xA8, D_8006259C, 0, 0x80);
        ArchiveCdDataSync(0);
    } else {
        memcpy(D_8006259C, D_8005A4BC, size);
        HeapUnpinBlock(D_8005A4BC);
        HeapFree(D_8005A4BC);
    }
    SoundAddSedsEntry(D_8006259C);
    func_8003BDFC(0x10);
    ArchiveSetIndex(4, 0);
    D_8004F32C = -1;
}

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


// Three 6-byte entries; splat names the second field's address D_800AFE8A.
typedef struct {
    /* 0x0 */ u_short unk0;
    /* 0x2 */ u_short unk2;
    /* 0x4 */ u_short unk4;
} FieldUnk800AFE88; // Size: 0x6

extern FieldUnk800AFE88 D_800AFE88[3];
extern FieldUnk800AFE88 D_800AFE8A[3];

extern s16 D_800B21AC;

void func_80086078(int arg0, int* pOut, int arg2) {
    int limit = D_800B21AC;
    int factor;
    u_int diff;
    int scaled;

    if (limit < arg0) {
        arg0 = limit;
    }
    factor = (0x7F0000 / limit * arg0) >> 16;
    diff = (0x80 - factor) << 16;
    scaled = diff / 127;
    *pOut = (u_int)(scaled * arg2) >> 16;
}

extern void func_8003A344(int, int);
extern void func_8003A55C(int, int);
void FieldActorWorldToScreenPosition(int, int*, int*);

void func_800860F0(int arg0, int arg1, int arg2, int arg3, int actorIndex) {
    int i;
    int value;
    int screenX;
    int screenY;
    int scaled;

    for (i = 0; i < 3; i++) {
        if (D_800AFE88[i].unk0 == actorIndex) {
            func_80086078(arg3, &value, arg1);
            FieldActorWorldToScreenPosition(actorIndex, &screenX, &screenY);
            if (screenX >= 0x141) {
                screenX = 0x13F;
            }
            if (screenX < 0) {
                screenX = 0;
            }
            scaled = screenX * 0x6666 >> 16;
            func_8003A344(i * 2, value);
            func_8003A55C(i * 2, scaled);
        }
    }
}

INCLUDE_ASM("asm/field/nonmatchings/main/misc8", FieldActorWorldToScreenPosition);
/* 80.00%: everything but the first CompMatrix argument matches. The original
   builds the world-to-screen matrix address as `&g_FieldActors - 0xAC` (lui +
   addiu of g_FieldActors, then a separate `addiu a0, v1, -0xAC`, the same v1 it
   reloads the volatile actor pointer from), i.e. the matrix and the actor list
   were members of one global struct in the original source. Writing
   `&g_Scene.worldToScreenMatrix` gives the two-instruction %hi/%lo form instead.
   A `(MATRIX*)((u_char*)&g_FieldActors - 0xAC)` hack reaches 95.25% but gcc
   rematerialises the address twice. Matching needs the shared parent struct.
void FieldActorWorldToScreenPosition(int actorIndex, int* pScreenX, int* pScreenY) {
    SVECTOR vertex;
    MATRIX transformMatrix;
    long sxy;
    long p;
    long flag;

    FieldScriptVMGetActorIndex(1);
    CompMatrix(&g_Scene.worldToScreenMatrix, &g_FieldActors[actorIndex].transformMatrix, &transformMatrix);
    vertex.vx = 0;
    vertex.vy = 0;
    vertex.vz = 0;
    SetRotMatrix(&transformMatrix);
    SetTransMatrix(&transformMatrix);
    RotTransPers(&vertex, &sxy, &p, &flag);
    *pScreenY = ((short*)&sxy)[1];
    *pScreenX = ((short*)&sxy)[0];
}
*/

INCLUDE_ASM("asm/field/nonmatchings/main/misc8", func_800862CC);

void func_800863E8(u_int arg0) {
    int i;

    for (i = 0; i < 3; i++) {
        if (D_800AFE88[i].unk0 == arg0) {
            func_8003A20C(i * 2);
            D_800AFE8A[i].unk0 = 0xFFFF;
            D_800AFE88[i].unk0 = 0xFFFF;
            break;
        }
    }
}

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

extern u_short D_800B233C;

void func_800864F0(void) {
    u_short* pFlags;
    int i;

    for (i = 0; i < 3; i++) {
        D_800AFE8A[i].unk0 = 0xFFFF;
        D_800AFE88[i].unk0 = 0xFFFF;
    }
    i = 0;
    pFlags = &D_800B233C;
    for (; i < 4; i++) {
        if (!(*pFlags & 1)) {
            func_8003A20C(i * 2);
        }
        *pFlags >>= 1;
    }
}

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

extern s16 D_800B22E2[];

void func_80086A1C(int, VECTOR*);

void func_80086BA8(void) {
    int i;

    for (i = 0; i < 3; i++) {
        if (D_800B22E2[i] != -1) {
            func_80086A1C(i, &g_FieldActors[D_800B22E2[i]].pActorData->position);
        }
    }
}

INCLUDE_ASM("asm/field/nonmatchings/main/misc8", func_80086C34);

void GameSoftReset(void);

void func_80086D4C(void) {
    GameSoftReset();
    D_800B00C0 = 1;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 1;
}
