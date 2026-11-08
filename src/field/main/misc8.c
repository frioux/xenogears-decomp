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
#include "field/particles.h"

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

/* 71.39%: identical size (468 bytes) and every block is the right shape; the
   residual is register allocation. gcc here keeps `actorIndex * 0x5C` in one
   pseudo across all four `g_FieldActors[actorIndex]` accesses and reuses
   `&g_FieldActors[actorIndex]`, where the original recomputes it once after the
   first HeapAlloc and once more after the loop. `D_800B2180++` is also emitted
   as `lui at; sw %lo(at)` instead of addressing the counter through one
   register (`lui v1,%hi; addiu v1,v1,%lo; lw/sw 0(v1)`); `volatile` makes it
   worse (gcc then reloads the counter). The frame (0x88) needs the unused 0x60
   local; reg assignment wants `int numWords` declared before the pointers and
   the unused array last.
INCLUDE_ASM("asm/field/nonmatchings/main/misc8", func_80080F44);
/* 71.39%: correct size (468 bytes) and frame (0x88, which needs the unused
   local to fill 0x78 bytes). Residual is address materialisation: the
   original recomputes `actorIndex * 0x5C` and reloads g_FieldActors for
   every `g_FieldActors[actorIndex].` access and addresses D_800B2180 through
   a register (`lui v1,%hi; addiu v1,v1,%lo; lw 0(v1); addiu; sw 0(v1)`),
   where gcc CSEs the index scaling and emits `lui at; sw %lo(at)`.
extern int D_800B2180;
void func_80080A74(int actorIndex);
void func_8007AA44(Quad* pQuad);

void func_80080F44(int actorIndex) {
    int numWords = sizeof(ActorData) / sizeof(u_int);
    int i;
    u_int* pData;
    ActorData* pActor;
    FieldActorModelData* pModelData;
    u8 unused[0x60]; // original has an unused stack slot; reproduces the frame

    if (actorIndex < D_800ADBFC) {
        D_800B2180++;
        pActor = HeapAlloc(0x138, 0);
        g_FieldActors[actorIndex].pActorData = pActor;
        pActor = g_FieldActors[actorIndex].pActorData;
        pData = (u_int*)pActor;
        for (i = 0; i < numWords; i++) {
            *pData++ = 0;
        }
        g_FieldActors[actorIndex].unk5A = 0;
        if (g_FieldActors[actorIndex].status & 0x2000) {
            pModelData = g_FieldActors[actorIndex].pModelData;
            pActor->unk118 = HeapAlloc(0x80, 0);
            if (pModelData->pScripts != NULL) {
                for (i = 0; i < pModelData->pScripts->numScripts; i++) {
                    pModelData->pScripts->pScripts[i].pCallback = func_80080A18;
                    ((u_int*)pActor->unk118)[i] = 0;
                }
            }
        }
        func_80080A74(actorIndex);
        g_FieldActors[actorIndex].pShadow = HeapAlloc(0x70, 0);
        func_8007AA44((Quad*)g_FieldActors[actorIndex].pShadow);
    }
}
*/

extern int D_800B2180;
void func_80080A74(int actorIndex);
void func_8007AA44(Quad* pQuad);

void func_80080F44(int actorIndex) {
    int numWords = sizeof(ActorData) / sizeof(u_int);
    int i;
    u_int* pData;
    ActorData* pActor;
    FieldActorModelData* pModelData;
    u8 unused[0x60];

    if (actorIndex < D_800ADBFC) {
        D_800B2180++;
        pActor = HeapAlloc(0x138, 0);
        g_FieldActors[actorIndex].pActorData = pActor;
        pActor = g_FieldActors[actorIndex].pActorData;
        pData = (u_int*)pActor;
        for (i = 0; i < numWords; i++) {
            *pData++ = 0;
        }
        g_FieldActors[actorIndex].unk5A = 0;
        if (g_FieldActors[actorIndex].status & 0x2000) {
            pModelData = g_FieldActors[actorIndex].pModelData;
            pActor->unk118 = HeapAlloc(0x80, 0);
            if (pModelData->pScripts != NULL) {
                for (i = 0; i < pModelData->pScripts->numScripts; i++) {
                    pModelData->pScripts->pScripts[i].pCallback = func_80080A18;
                    ((u_int*)pActor->unk118)[i] = 0;
                }
            }
        }
        func_80080A74(actorIndex);
        g_FieldActors[actorIndex].pShadow = HeapAlloc(0x70, 0);
        func_8007AA44((Quad*)g_FieldActors[actorIndex].pShadow);
    }
}
*/

INCLUDE_ASM("asm/field/nonmatchings/main/misc8", func_8008110C);

INCLUDE_ASM("asm/field/nonmatchings/main/misc8", func_800815F0);

INCLUDE_ASM("asm/field/nonmatchings/main/misc8", func_80081C54);

int func_80081F5C(ActorData* pActor) {
    u_int actorType = (pActor->scriptFlags.flags >> 9) & 3;

    return -((actorType & (pActor->curWalkmeshTriMaterial >> 3)) != 0);
}

INCLUDE_ASM("asm/field/nonmatchings/main/misc8", func_80081F80);

INCLUDE_ASM("asm/field/nonmatchings/main/misc8", func_800821F4);
/* 95.50%: everything matches except how the model-id table base is formed. The
   original keeps `&D_800B2346` in s2 and derives the table (`&D_800B2346 - 0xB1`
   shorts) with a separate `addiu a0, s2, -354`; gcc here folds the -354 into the
   %lo of the symbol, which also makes it rematerialise the compare load instead
   of reusing s2.
extern s16 D_800B2344;
extern s16 D_800B2346;
extern u8 D_800ADFB8[];
void func_800245D8(int, int);
void func_801E8330(u16, u16, s32);

void func_800821F4(int arg0, int actorId, FieldActor* pActor) {
    s16* pModelIds;
    s16* pTable;

    if (!(pActor->status & 0x40)) {
        return;
    }
    if (actorId != 3 && D_800B2344 == 0) {
        pActor->pActorData->scriptFlags.flags &= ~0x800;
    }
    if (actorId == 0xFF) {
        actorId = 0;
    }
    // The model-id table ends right before D_800B2346; the original derives its
    // base from that symbol, so index from the last entry.
    pModelIds = &D_800B2346;
    if (actorId != pModelIds[0]) {
        pActor->pActorData->scriptFlags.flags &= ~0x800;
    }
    if (!(pActor->pActorData->flags & 0x2000)) {
        if (pActor->pActorData->flags & 0x1000000) {
            return;
        }
        func_800245D8(arg0, actorId);
        return;
    }
    pTable = pModelIds - 0xB1;
    if (actorId < 0x10) {
        func_801E8330(pActor->pActorData->flags12C_0xE & 7, 0, D_800ADFB8[actorId]);
        pTable[pActor->pActorData->flags12C_0xE & 7] = D_800ADFB8[actorId];
    } else {
        actorId -= 0x10;
        func_801E8330(pActor->pActorData->flags12C_0xE & 7, 0, actorId);
        pTable[pActor->pActorData->flags12C_0xE & 7] = actorId;
    }
}
*/

// Rectangular trigger zone given as a centre and per-axis half-extents, unlike
// the quad-corner FieldTriggerZone. Centred on 0x22 (z) / 0x2A (x).
typedef struct {
    /* 0x0  */ u8 unk0[0x18];
    /* 0x18 */ u_short unk18;
    /* 0x1A */ u16 unk1A;
    /* 0x1C */ u_short unk1C;
    /* 0x1E */ u8 unk1E[0x4];
    /* 0x22 */ s16 unk22;
    /* 0x24 */ u8 unk24[0x6];
    /* 0x2A */ s16 unk2A;
} FieldExtentZone; // Size: 0x2C

void func_80281678(FieldExtentZone* pZone);

int func_8008237C(int z, int x, FieldExtentZone* pZone, int radius) {
    long actorPosition2D;
    long triggerPos1;
    long triggerPos2;
    long triggerPos3;
    long triggerPos4;

    actorPosition2D = (z << 0x10) + x;
    triggerPos1 = ((pZone->unk22 - pZone->unk18 - radius) << 0x10) + (pZone->unk2A + pZone->unk1C + radius);
    triggerPos2 = ((pZone->unk22 + pZone->unk18 + radius) << 0x10) + (pZone->unk2A + pZone->unk1C + radius);
    triggerPos3 = ((pZone->unk22 + pZone->unk18 + radius) << 0x10) + (pZone->unk2A - pZone->unk1C - radius);
    triggerPos4 = ((pZone->unk22 - pZone->unk18 - radius) << 0x10) + (pZone->unk2A - pZone->unk1C - radius);
    if (NormalClip(triggerPos1, triggerPos2, actorPosition2D) < 0) {
        return -1;
    }
    if (NormalClip(triggerPos2, triggerPos3, actorPosition2D) < 0) {
        return -1;
    }
    if (NormalClip(triggerPos3, triggerPos4, actorPosition2D) < 0) {
        return -1;
    }
    if (NormalClip(triggerPos4, triggerPos1, actorPosition2D) < 0) {
        return -1;
    }
    if (g_FieldSystemMode == 0) {
        func_80281678(pZone);
    }
    return 0;
}

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
/* 92.68%: the whole chain matches; gcc loads vy straight into v1 and stores
   `sh 0x106` into the call delay slot, where the original loads into v0 and
   copies to v1 (`move v1,v0`, one more insn: 144 vs 140 bytes) and keeps both
   stores before the reload of rotation.vx. `u_short value` for the second half
   is required: it is what makes the second half a single `lhu`.
void func_800831F4(int arg0, ActorData* pActor, int arg2, int arg3) {
    short vy;
    int rotation;
    u_short value;
    RECT rect;

    if (pActor->moveModified.vx != 0 || pActor->moveModified.vz != 0) {
        vy = pActor->rotation.vy;
        if (!(vy & 0x8000)) {
            rotation = vy + ((arg3 & 1) ? -0x400 : 0x400);
            rotation &= 0xFFF;
            pActor->rotation.vx = rotation;
            pActor->rotation.vy = rotation;
            func_80081F80(arg0, pActor->rotation.vx);
            value = pActor->rotation.vy | 0x8000;
            pActor->rotation.vx = value;
            pActor->rotation.vy = value;
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
/* 91.16%: every check matches; the only residual is the `flags & 2` early
   return. The original keeps it as its own `li v0,-1` block after the
   `bltz`-style tail (280 bytes), so the `beqz` above it keeps a nop delay slot
   and the later `beqz flags&4` gets `move v0,zero` from its target; gcc here
   cross-jumps that return into the shared epilogue and fills the delay slots
   itself (276 bytes).
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
    if (pActor->flags & 1 && pActor->walkmeshId == 0) {
        return -1;
    }
    if (pActor->flags & 2 && pActor->walkmeshId == 1) {
        return -1;
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
/* 85.56%: the original keeps `arg3 & 7` in s0 and shifts at each use
   (`andi s0,a3,7`, `sll a0,s0,1`, `sll a1,s0,1`); gcc CSEs the doubled value
   into s0 and copies it (`move a0,s0` / `move a1,s0`). Same size and registers,
   so it links, but objdiff never reaches 100%.
void func_800855C8(int arg0, int arg1, int arg2, int arg3) {
    int offset = arg3 & 7;

    func_8003A20C(offset * 2);
    func_80039F9C(arg0, offset * 2, (s16)arg1, (s16)arg2);
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

// WDS files are streamed in 0x800-byte blocks; the header occupies the first four.
typedef struct {
    /* 0x0 */ u_long unk0[0x200];
} FieldWdsBlock; // Size: 0x800

extern FieldWdsBlock* D_800C3A1C;
extern s32 D_800B2370;
extern SoundWDSEntry* g_GameCurLoadedWDS;
extern s32 D_8004F308;
extern s32 D_8004F33C;
extern s32 D_8004F354;
extern s32 D_800AFC54;
extern u8 D_800ADFCD[];
extern void ArchiveCdDataSync(int mode);
extern void func_8001B66C(void);
void func_8002945C(void* pData);
void func_8003BDFC(int arg0);
void SoundTransferWdsPart(void* pBuffer, int size);
SoundWDSEntry* func_800380D0(void* pBuffer, int size, int arg2);
void func_80085560(int archiveIndex, int unused, void* pCallback);

void func_800859DC(FieldWdsBlock* pBlock) {
    int bankIndex = D_800B2370;

    switch (bankIndex) {
        case 0:
        case 1:
        case 2:
        case 3:
            D_800C3A1C[bankIndex] = *pBlock;
            D_800B2370++;
            func_8002945C(pBlock);
            if (D_800B2370 == 4) {
                g_GameCurLoadedWDS = func_800380D0(D_800C3A1C, 0x2000, 0);
            }
            break;
        case 4:
            func_8003BDFC(0x10);
            D_800C3A1C[0] = *pBlock;
            SoundTransferWdsPart(D_800C3A1C, 0x800);
            func_8002945C(pBlock);
            break;
    }
}

INCLUDE_ASM("asm/field/nonmatchings/main/misc8", func_80085B20);
/* 95.92%: only the block index computation differs. The original computes
   `2 * streamId + 0x13` into v0 before the D_8004F33C check and copies it to a0
   at the call (`addiu v0,v0,0x13` / `move a0,v0`); gcc here computes it straight
   into a0 inside the branch body, which also leaves gcc's two delay slots empty
   and gives D_8004F33C the v0 register.
void func_80085B20(int arg0) {
    u_short streamId;

    ArchiveCdDataSync(0);
    func_8001B66C();
    if (arg0 == 0xFF) {
        D_8004F308 = 0;
        return;
    }
    ArchiveSetIndex(0x1C, 0);
    if (D_800ADFCD[arg0 * 2] == 1) {
        func_80086024();
    }
    streamId = D_800ADFCD[arg0 * 2 - 1];
    if (streamId != 0xFF) {
        if (D_8004F33C != streamId) {
            func_80085560(streamId * 2 + 0x13, 1, (void*)func_800859DC);
            D_8004F354 = 1;
            D_800B2370 = 0;
            D_800C3A1C = HeapAlloc(0x2000, 1);
        }
    }
    ArchiveSetIndex(4, 0);
    D_8004F308 = -1;
    D_800AFC54 = 1;
}
*/

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
/* 95.25%: everything matches but the base address (204 bytes, same size).
   The original materialises `&g_FieldActors` into v1 (`lui v1,%hi;
   addiu v1,v1,%lo`, offset 0, after the FieldScriptVMGetActorIndex call),
   takes the matrix from it with `addiu a0,v1,-0xAC` and loads the actor
   pointer with `lw a1,0(v1)`; gcc folds -0xAC into one %lo and emits a
   separate lui/lw for the pointer. The -0xAC is exact and identifies the
   original layout: &g_FieldActors (0x800AFB10) - 0xAC ==
   g_Scene.worldToScreenMatrix (0x800AF990 + 0xD4), so the matrix and the
   actor pointer were members of one struct in the original source. The anchor
   must stay the g_FieldActors symbol, so the fix is a parent struct that the
   symbol file can express without overlapping g_Scene. Holding the base in a
   local pointer does not help: gcc const-propagates the symbol address.
void FieldActorWorldToScreenPosition(int actorIndex, int* pScreenX, int* pScreenY) {
    SVECTOR vertex;
    MATRIX transformMatrix;
    long sxy;
    long p;
    long flag;
    FieldActor** pActorSlot = &g_FieldActors;

    CompMatrix((MATRIX*)((u_char*)pActorSlot - 0xAC), &(*pActorSlot)[actorIndex].transformMatrix, &transformMatrix);
    FieldScriptVMGetActorIndex(1);
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

void func_800862CC(int arg0, int arg1, int arg2, int arg3, int actorIndex) {
    short value;
    int screenX;
    int screenY;
    int i;
    int scaled;

    for (i = 0; i < 3; i++) {
        if (D_800AFE8A[i].unk0 == 0xFFFF) {
            D_800AFE8A[i].unk0 = arg0;
            D_800AFE88[i].unk0 = actorIndex;
            func_80086078(arg3, (int*)&value, arg1);
            FieldActorWorldToScreenPosition(actorIndex, &screenX, &screenY);
            if (screenX >= 0x141) {
                screenX = 0x13F;
            }
            if (screenX < 0) {
                screenX = 0;
            }
            scaled = screenX * 0x6666 >> 16;
            func_8003A20C(i * 2);
            func_80039F9C((void*)arg0, (short)(i * 2), value, scaled);
            break;
        }
    }
}

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

extern s32 D_800AFD1C;
void FieldInitializeDefaultParticleBanks(int targetActorID);
int FieldInitializeParticleBanks(int actorIndex);

void func_80086C34(void) {
    int arg0;
    int arg1;
    int arg2;

    switch (SCRIPT_READ_U8_REL(1)) {
        case 0:
            g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
            break;
        case 1:
            arg0 = FieldScriptVMGetArgument(2);
            arg1 = FieldScriptVMGetArgument(4);
            arg2 = FieldScriptVMGetArgument(6);
            FieldInitializeDefaultParticleBanks(D_800AFD1C);
            g_FieldDefaultParticleBanks[0].flags = 0x14;
            g_FieldDefaultParticleBanks[0].unk0 = 0x1;
            g_FieldDefaultParticleBanks[0].max = 0x10;
            g_FieldDefaultParticleBanks[0].unk72 = 0x0;
            g_FieldDefaultParticleBanks[0].unk74 = arg1;
            g_FieldDefaultParticleBanks[0].unk74 = (arg1 == 0x27) ? 0x20 : 0x22;
            g_FieldDefaultParticleBanks[0].ewait = 0x1000;
            FieldInitializeParticleBanks(D_800AFD1C);
            g_FieldScriptVMCurActor->scriptInstructionPointer += 8;
            break;
    }
}

void GameSoftReset(void);

void func_80086D4C(void) {
    GameSoftReset();
    D_800B00C0 = 1;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 1;
}
