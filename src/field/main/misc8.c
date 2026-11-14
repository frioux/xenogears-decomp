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
void FieldFreeActorData(int actorIndex) {
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

u_int FieldActorGetWalkmeshTriMaterial(ActorData* pActor) {
    if ((pActor->flags >> (pActor->walkmeshId + 3)) & 1) {
        return 0;
    }
    return D_800AFB20[D_800AFB24[pActor->walkmeshId][pActor->walkmeshTriIds[pActor->walkmeshId]].material];
}

extern u_short D_800B14AC;

s32 FieldSumRampSteps(s32* pData) {
    s32 value = -0x14D000;
    s32 step = pData[7];
    s32 acc = 0;
    u_short count;

    D_800B14AC = 0;
    do {
        count = D_800B14AC;
        count += 2;
        D_800B14AC = count;
        acc += value;
    } while ((value += step) <= 0);
    return acc >> 16;
}

extern s32 D_800ADB58;
extern s32 D_800ADB5C;

u_int FieldPopActorScriptWorkWord(void) {
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
*/
INCLUDE_ASM("asm/field/nonmatchings/main/misc8", func_80080F44);
/* 71.39%: correct size (468 bytes) and frame (0x88, which needs the unused
   local to fill 0x78 bytes). Residual is address materialisation: the
   original recomputes `actorIndex * 0x5C` and reloads g_FieldActors for
   every `g_FieldActors[actorIndex].` access and addresses D_800B2180 through
   a register (`lui v1,%hi; addiu v1,v1,%lo; lw 0(v1); addiu; sw 0(v1)`),
   where gcc CSEs the index scaling and emits `lui at; sw %lo(at)`.
extern int D_800B2180;
void func_80080A74(int actorIndex);
void FieldInitShadowQuad(Quad* pQuad);

void func_80080F44(int actorIndex) {
    int numWords = sizeof(ActorData) / sizeof(u_int);
    int i;
    u_int* pData;
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
                    pModelData->pScripts->pScripts[i].pCallback = FieldPopActorScriptWorkWord;
                    ((u_int*)pActor->unk118)[i] = 0;
                }
            }
        }
        func_80080A74(actorIndex);
        g_FieldActors[actorIndex].pShadow = HeapAlloc(0x70, 0);
        FieldInitShadowQuad((Quad*)g_FieldActors[actorIndex].pShadow);
    }
}

extern int D_800B2180;
void func_80080A74(int actorIndex);
void FieldInitShadowQuad(Quad* pQuad);

void func_80080F44(int actorIndex) {
    int numWords = sizeof(ActorData) / sizeof(u_int);
    int i;
    u_int* pData;
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
                    pModelData->pScripts->pScripts[i].pCallback = FieldPopActorScriptWorkWord;
                    ((u_int*)pActor->unk118)[i] = 0;
                }
            }
        }
        func_80080A74(actorIndex);
        g_FieldActors[actorIndex].pShadow = HeapAlloc(0x70, 0);
        FieldInitShadowQuad((Quad*)g_FieldActors[actorIndex].pShadow);
    }
}
*/

INCLUDE_ASM("asm/field/nonmatchings/main/misc8", func_8008110C);

INCLUDE_ASM("asm/field/nonmatchings/main/misc8", func_800815F0);

INCLUDE_ASM("asm/field/nonmatchings/main/misc8", func_80081C54);
/* 63.38% (772 vs 776 bytes). What is settled, from the assembly:
   - It fills one entry of a 0x48-byte ring at D_800B14F0 indexed by a volatile
     s32 D_800B2360 (the original re-reads the index 13 times, so the global must
     be declared volatile), then clears D_800C3910 and does
     D_800B2360 = (D_800B2360 - 1) & 0x1F.
   - Sources: one &g_FieldActors[actorIndex] read gives pActorData (FieldActor
     0x4C) and pSpriteData (0x4); two early returns
     (actorIndex != g_PlayerActorIndex, D_800B21CC != 0).
   - Destination offsets from 0x800B14F0 (splat has a bss label at each of
     0x14F0/14F4/14F8/14FA/14FC/1500/1502/1504/1510/1514/1518/1530/1534):
     0x00 ActorData scriptFlags.flags, 0x04 ActorData flags,
     0x08/0x0A/0x0C high halves of ActorData position (s16 at 0x22/0x26/0x2A),
     0x10 SpriteData field_0x84, 0x12 ActorData curAnimationId,
     0x14 rotation.vy & 0xFFF, 0x16..0x1C walkmeshTriIds[4] (copied by a loop),
     0x20/0x24/0x28 SpriteData step.x/y/z, 0x30/0x34/0x38 curTriNormal vx/vy/vz,
     0x40 curWalkmeshTriMaterial, 0x44 walkmeshId (byte store).
   - Residue: (1) the 0x18 frame is never used by the body, so the original
     source must have a dead ~0x18 local; (2) pActorData/pSpriteData land in
     a3/a1 here instead of t0/a2; (3) address CSE - the original materialises
     sym+0x20 once (la a1, 0x800B1510) and derives +0x10 and -0xA from it with
     runtime addius, where gcc re-forms the symbolic address per statement
     (sw $4,D_800B14F0+32($2)); both link to the same address, so the %hi/%lo
     addend split can never read 100% in objdiff - gate this one with
     tools/scripts/check_bin.sh field once the shape is right.
*/
/* attempt:
typedef struct {
    0x00    u_int scriptFlags;
    0x04    u_int flags;
    0x08    s16 positionX;
    0x0A    s16 positionY;
    0x0C    s16 positionZ;
    0x0E    s16 pad0E;
    0x10    s16 spriteUnk84;
    0x12    s16 animationId;
    0x14    s16 rotation;
    0x16    s16 walkmeshTriIds[4];
    0x1E    s16 pad1E;
    0x20    s32 stepX;
    0x24    s32 stepY;
    0x28    s32 stepZ;
    0x2C    s32 pad2C;
    0x30    s32 normalX;
    0x34    s32 normalY;
    0x38    s32 normalZ;
    0x3C    s32 pad3C;
    0x40    u_int walkmeshTriMaterial;
    0x44    u8 walkmeshId;
    0x45    undefined8 pad45[3];
} FieldActorRenderPacket; // Size: 0x48

extern FieldActorRenderPacket D_800B14F0[];
extern volatile s32 D_800B2360;
extern s32 D_800C3910;
extern u_char D_800B21CC;
extern s32 g_PlayerActorIndex;

void func_80081C54(int actorIndex) {
    FieldActor* pEntry = &g_FieldActors[actorIndex];
    ActorData* pActor = pEntry->pActorData;
    SpriteData* pSprite = (SpriteData*)pEntry->pSpriteData;
    int i;

    if (actorIndex != g_PlayerActorIndex) {
        return;
    }
    if (D_800B21CC != 0) {
        return;
    }
    D_800B14F0[D_800B2360].stepX = pSprite->step.x;
    D_800B14F0[D_800B2360].stepY = pSprite->step.y;
    D_800B14F0[D_800B2360].stepZ = pSprite->step.z;
    D_800B14F0[D_800B2360].normalX = pActor->curTriNormal.vx;
    D_800B14F0[D_800B2360].normalY = pActor->curTriNormal.vy;
    D_800B14F0[D_800B2360].normalZ = pActor->curTriNormal.vz;
    D_800B14F0[D_800B2360].rotation = *(u_short*)&pActor->rotation.vy & 0xFFF;
    D_800B14F0[D_800B2360].spriteUnk84 = *(u_short*)&pSprite->field_0x84;
    D_800B14F0[D_800B2360].positionX = ((s16*)&pActor->position)[1];
    D_800B14F0[D_800B2360].positionY = ((s16*)&pActor->position)[3];
    D_800B14F0[D_800B2360].positionZ = ((s16*)&pActor->position)[5];
    D_800B14F0[D_800B2360].animationId = *(u_short*)&pActor->curAnimationId;
    D_800B14F0[D_800B2360].walkmeshTriMaterial = pActor->curWalkmeshTriMaterial;
    D_800B14F0[D_800B2360].scriptFlags = pActor->scriptFlags.flags;
    D_800B14F0[D_800B2360].flags = pActor->flags;
    for (i = 0; i < 4; i++) {
        D_800B14F0[D_800B2360].walkmeshTriIds[i] = pActor->walkmeshTriIds[i];
    }
    D_800B14F0[D_800B2360].walkmeshId = *(u_short*)&pActor->walkmeshId;
    D_800C3910 = 0;
    D_800B2360 = (D_800B2360 - 1) & 0x1F;
}
*/

int FieldActorTestMaterialScriptMask(ActorData* pActor) {
    u_int actorType = (pActor->scriptFlags.flags >> 9) & 3;

    return -((actorType & (pActor->curWalkmeshTriMaterial >> 3)) != 0);
}

INCLUDE_ASM("asm/field/nonmatchings/main/misc8", func_80081F80);
/* 58.17% (644 bytes vs 644): the structure is right - status & 0x40 selects the
   two halves, the scale is `(N / pActorData->moveSpeed >> 8) << 5` with
   N = 0x40000 (status bit clear, and the 0x20000-set path stores
   moveSpeed = 0x4000000 / moveSpeed) or 0x80000 (flags & 0x2000, 0x20000 clear),
   and the shared step.x/step.z computation is duplicated in the original: one
   copy inline after the 0x40000 case, one at 0x800820FC that the 0x40000-free
   paths jump into. arg0 is SpriteData* (step at 0xC/0x14, moveSpeed at 0x18),
   arg2 is FieldActor*, and scaleX/scaleZ must be read signed (`lh`, so cast the
   header's u_short to s16); the 0x12C word is read whole through
   ActorFlags12CView. Residual: gcc lays the `flags & 0x2000` then-block out as
   fall-through where the original jumps forward over the 0x80000 block, uses
   `at` to form `D_801E8670 + ((flags12C >> 11) & 0x1C)` where the original keeps
   it in v1 via `lui/addiu`, and spills one extra register (frame 0x38 vs 0x28).
   `else if (!(flags & 0x2000))` first gives 55.87%.
void func_80081F80(SpriteData* pSprite, s16 angle, FieldActor* pActor) {
    int scale;
    int a;

    if (!(pActor->status & 0x40)) {
        scale = (0x40000 / pActor->pActorData->moveSpeed >> 8) << 5;
        if (angle & 0x8000) {
            pSprite->step.x = 0;
            pSprite->step.z = 0;
        } else {
            a = angle & 0xFFF;
            pSprite->step.x = (rsin(a) * scale >> 12) * (s16)pActor->pActorData->scaleX;
            pSprite->step.z = (-(rcos(a) * scale) >> 12) * (s16)pActor->pActorData->scaleZ;
        }
    } else if (angle & 0x8000) {
        pSprite->step.x = 0;
        pSprite->step.z = 0;
    } else if (pActor->pActorData->flags & 0x2000) {
        if (pActor->pActorData->flags & 0x20000) {
            pSprite->step.x = -((s32*)D_801E8670[(((ActorFlags12CView*)pActor->pActorData)->flags12C >> 13) & 7])[0x128 / 4] << 16;
            pSprite->step.z = -((s32*)D_801E8670[(((ActorFlags12CView*)pActor->pActorData)->flags12C >> 13) & 7])[0x130 / 4] << 16;
        } else {
            scale = (0x80000 / pActor->pActorData->moveSpeed >> 8) << 5;
            a = angle & 0xFFF;
            pSprite->step.x = (rsin(a) * scale >> 12) * (s16)pActor->pActorData->scaleX;
            pSprite->step.z = (-(rcos(a) * scale) >> 12) * (s16)pActor->pActorData->scaleZ;
        }
    } else if (pActor->pActorData->flags & 0x80000) {
        scale = (0x40000 / pActor->pActorData->moveSpeed >> 8) << 5;
        a = angle & 0xFFF;
        pSprite->step.x = (rsin(a) * scale >> 12) * (s16)pActor->pActorData->scaleX;
        pSprite->step.z = (-(rcos(a) * scale) >> 12) * (s16)pActor->pActorData->scaleZ;
        pSprite->moveSpeed = 0x4000000 / pActor->pActorData->moveSpeed;
    } else {
        SpriteSetDirection(pSprite, angle);
    }
    pSprite->step.x &= -0x1000;
    pSprite->step.z &= -0x1000;
}
*/

extern s16 D_800B2344;
extern s16 D_800B2346;
extern u8 D_800ADFB8[];
void func_800245D8(int, int);
void LibGearSetModelAnimation(u16, u16, s32);

// This unit reads the 0x12C word whole; the ActorData bitfields there make gcc
// narrow the load to a byte.
typedef struct {
    /* 0x0   */ u8 unk0[0x12C];
    /* 0x12C */ u_int flags12C;
} ActorFlags12CView; // Size: 0x130

INCLUDE_ASM("asm/field/nonmatchings/main/misc8", func_800821F4);
/* 95.91%, same size, register base only. Two findings. (1) The 0x12C word must
   be read whole - the ActorData bitfields there make gcc narrow it to
   `lbu 0x12D; srl 5`, so this unit reads it through ActorFlags12CView.
   (2) The model-id base: the original does `la s2, D_800B2346` for the compare
   and derives the table with a per-branch `addiu a0, s2, -0x162`; gcc here
   rematerialises the compare inline and keeps the -0x162 address (used in both
   branches) in s2 instead. Computing pTable after the compare is what gets
   gcc to share one base at all; who owns the register is the remaining gap.
   Verified this round: indexing the array directly with no local
   (`((s16*)&D_800B2346)[i - 0xB1]`, recomputing the 0x12C read at each store)
   drops to 91.23% and loses the s2 slot entirely (gcc folds each address into
   `lui at; addu at,at,v0`), so the pointer local below is required.
void func_800821F4(int arg0, int actorId, FieldActor* pActor) {
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
    if (actorId != (&D_800B2346)[0]) {
        pActor->pActorData->scriptFlags.flags &= ~0x800;
    }
    pTable = &D_800B2346 - 0xB1;
    if (!(pActor->pActorData->flags & 0x2000)) {
        if (pActor->pActorData->flags & 0x1000000) {
            return;
        }
        func_800245D8(arg0, actorId);
        return;
    }
    if (actorId < 0x10) {
        LibGearSetModelAnimation((((ActorFlags12CView*)pActor->pActorData)->flags12C >> 13) & 7, 0, D_800ADFB8[actorId]);
        pTable[(((ActorFlags12CView*)pActor->pActorData)->flags12C >> 13) & 7] = D_800ADFB8[actorId];
    } else {
        actorId -= 0x10;
        LibGearSetModelAnimation((((ActorFlags12CView*)pActor->pActorData)->flags12C >> 13) & 7, 0, actorId);
        pTable[(((ActorFlags12CView*)pActor->pActorData)->flags12C >> 13) & 7] = actorId;
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

int FieldCheckPointInExtentZone(int z, int x, FieldExtentZone* pZone, int radius) {
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

int FieldCheckActorInZone2D(IVEC3* pOffset, ActorData* pActor) {
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

int FieldGetActorDistance2D(int actorIndexA, int actorIndexB) {
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

int FieldClampAngleDelta(int angle, int limit) {
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

void FieldConvertIVec3ToSVector(SVECTOR* pDst, IVEC3* pSrc) {
    pDst->vx = pSrc->x >> 0x10;
    pDst->vy = pSrc->y >> 0x10;
    pDst->vz = pSrc->z >> 0x10;
}

extern void* D_801E8670[];
void SpriteSetDirection(SpriteData*, s16);

INCLUDE_ASM("asm/field/nonmatchings/main/misc8", func_800831F4);
/* 92.68% (148 vs 144 bytes): frame and every other insn match, two residuals.
   The branch shape only comes out right when the ternary sits *inside* the
   arithmetic: `angle = (rotation.vy + ((direction & 1) ? -0x400 : 0x400)) &
   0xFFF` gives the original's `bnez $v0,.L; addiu $v0,$v1,-0x400` cond-add with
   the delay slot filled (an if/else or a ternary split from the `& 0xFFF`
   gives `beqz` + `j` instead). The second store reads `vy` unsigned (`lhu`), so
   the `| 0x8000` uses the field type, not a signed local.
   Frame: do NOT pad it. This body compiles to a 0x18 frame (140 bytes vs the
   original 148); the original's extra 8 unused bytes and the `move $v1,$v0`
   below arrive together with the right instruction stream, so an unused local
   only moves the frame and leaves the diff where it is.
   Residuals: (1) the original loads vy into v0, copies it (`move $v1,$v0`) and
   tests in place (`andi $v0,$v0,0x8000`) so vy lives in v1 for the addu; gcc
   loads straight into v1 (`lh $v1`; `andi $v0,$v1,0x8000`) and is 4 bytes
   short. Reading the field into a `s16 current` local first changes nothing.
   (2) gcc sinks the second store (`sh $v0,0x106($s0)`) into the `jal
   func_80081F80` delay slot because the reload `lh $a1,0x104($s0)` is moved
   before it; the original stores both halves, reloads, and keeps a nop in the
   slot.
void func_80081F80(SpriteData*, s16, FieldActor*);

void func_800831F4(SpriteData* pSprite, ActorData* pActorData, FieldActor* pActor, int direction) {
    s16 angle;

    if (pActorData->moveModified.vx != 0 || pActorData->moveModified.vz != 0) {
        if (!(pActorData->rotation.vy & 0x8000)) {
            angle = (pActorData->rotation.vy + ((direction & 1) ? -0x400 : 0x400)) & 0xFFF;
            pActorData->rotation.vx = angle;
            pActorData->rotation.vy = angle;
            func_80081F80(pSprite, pActorData->rotation.vx, pActor);
            angle = pActorData->rotation.vy | 0x8000;
            pActorData->rotation.vx = angle;
            pActorData->rotation.vy = angle;
        }
    }
}
*/
INCLUDE_ASM("asm/field/nonmatchings/main/misc8", func_80083288);

void FieldNoOp(void) {
}

INCLUDE_ASM("asm/field/nonmatchings/main/misc8", func_8008399C);

INCLUDE_ASM("asm/field/nonmatchings/main/misc8", func_80084158);

INCLUDE_ASM("asm/field/nonmatchings/main/misc8", func_8008492C);
/* 91.16%: every check matches; the only residual is the `flags & 2` early
   return. The original keeps it as its own `li v0,-1` block after the
   `bltz`-style tail (280 bytes), so the `beqz` above it keeps a nop delay slot
   and the later `beqz flags&4` gets `move v0,zero` from its target; gcc here
   cross-jumps that return into the shared epilogue and fills the delay slots
   itself (276 bytes). Re-measured: the merge also costs the ordering of the
   `flags & 2` block (ours hoists `li v0,0x1` above `lh v1,0x10($a0)`, the
   original loads first) and leaves the `flags & 4` load in v0, where the
   original uses v1 - v1 is only dead there once the `flags & 2` return is its
   own block. Same body, same 276 bytes.
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
int* ArchiveAllocStreamFile(int numEntries, int allocMode);
void ArchiveReadFileToBuffer(int fileIndex, void* pBuffer, int arg2, int size);

int FieldStepArchiveStream(void) {
    int result = func_80028B14();

    D_800ADBBC = result;
    if (result != 0) {
        ((void (*)(int))D_800AFEA4)(result);
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

void FieldStartArchiveStream(int archiveIndex, int allocMode, void* pCallback) {
    int* pStreamFile;

    D_800ADB2C = 1;
    pStreamFile = ArchiveAllocStreamFile(8, allocMode);
    D_800ADBB8 = pStreamFile;
    ArchiveReadFileToBuffer(archiveIndex, pStreamFile, 0, 0x100);
    D_800AFEA4 = pCallback;
}

void func_8003A20C(int);
void SoundPlaySoundEffectOnPairWithLevels(void*, int, int, int);

INCLUDE_ASM("asm/field/nonmatchings/main/misc8", func_800855C8);
/* 85.56%: the original keeps `arg3 & 7` in s0 and shifts at each use
   (`andi s0,a3,7`, `sll a0,s0,1`, `sll a1,s0,1`); gcc CSEs the doubled value
   into s0 and copies it (`move a0,s0` / `move a1,s0`), which also keeps it out
   of the first jal's delay slot. Same size and registers, so it links, but
   objdiff never reaches 100%. Writing `offset + offset`, `2 * (arg3 & 7)`, or
   the two expressions inline with no local all still fold into one product;
   re-tried here: both inline (85.56%, `sll s0,s0,1` hoisted before the call
   plus two `move`s) and `offset*2` + `offset+offset` (85.56%, same shape - the
   front end canonicalises `+` to a shift before cse, so the two uses still
   merge); the 100% shape needs a second `offset * 2` that gcc cannot CSE.
void func_800855C8(int arg0, int arg1, int arg2, int arg3) {
    int offset = arg3 & 7;

    func_8003A20C(offset * 2);
    SoundPlaySoundEffectOnPairWithLevels(arg0, offset * 2, (s16)arg1, (s16)arg2);
}
*/

extern s32 D_800B21B8;

void FieldSetSoundSlotElement(int arg0, int arg1) {
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
void SoundPlaySoundEffectOnPair(s32 arg0, s32 arg1);

typedef struct {
    /* 0x0 */ u_short unk0;
    /* 0x2 */ u_short unk2;
} FieldUnk800AE060; // Size: 0x4

extern FieldUnk800AE060 D_800AE060[];

void FieldAdvanceSedsSequence(void) {
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
    SoundPlaySoundEffectOnPair((value & 0xFF) | (D_800B235C->sedId << 16), (value >> 7) & 0xE);
    D_800C3A64++;
    goto loop;
}

void FieldStopSedsPlayback(void) {
    if (D_800C3A38 != 0xFF) {
        SoundStopAllElements();
        SoundRemoveSedsEntry(D_800B235C);
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
    SoundStopAllElements();
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

void FieldLoadSedsFile(void) {
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

void FieldFreeSedsFile(void) {
    SoundRemoveSedsEntry(D_8006259C);
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
extern void SoundFreeLoadedWdsAndAudioManager(void);
void func_8002945C(void* pData);
void func_8003BDFC(int arg0);
void SoundTransferWdsPart(void* pBuffer, int size);
SoundWDSEntry* SoundLoadPartialWdsFile(void* pBuffer, int size, int arg2);
void FieldStartArchiveStream(int archiveIndex, int unused, void* pCallback);

void FieldWdsStreamCallback(FieldWdsBlock* pBlock) {
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
                g_GameCurLoadedWDS = SoundLoadPartialWdsFile(D_800C3A1C, 0x2000, 0);
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

void FieldStartWdsBankLoad(int arg0) {
    u_short streamId;
    int index;

    ArchiveCdDataSync(0);
    SoundFreeLoadedWdsAndAudioManager();
    if (arg0 == 0xFF) {
        D_8004F308 = 0;
        return;
    }
    ArchiveSetIndex(0x1C, 0);
    if (D_800ADFCD[arg0 * 2] == 1) {
        FieldReleaseWdsEntry();
    }
    streamId = D_800ADFCD[arg0 * 2 - 1];
    if (streamId != 0xFF) {
        index = streamId * 2 + 0x13;
        if (D_8004F33C != streamId) {
            FieldStartArchiveStream(index, 1, (void*)FieldWdsStreamCallback);
            D_8004F354 = 1;
            D_800B2370 = 0;
            D_800C3A1C = HeapAlloc(0x2000, 1);
        }
    }
    ArchiveSetIndex(4, 0);
    D_8004F308 = -1;
    D_800AFC54 = 1;
}

int FieldRunArchiveStreamSteps(void) {
    int i;

    for (i = 0; i < 5; i++) {
        if (FieldStepArchiveStream() == -1) {
            return 0;
        }
    }
    return -1;
}

extern s32 D_8004F364;
extern s32 D_8004F338;
extern s32 D_8004F340;
extern s32 D_8004F348;
extern s32 D_8004F358;
extern s32 D_8004F35C;
extern s32 D_8004F36C;
extern s32 g_GameHasLoadedWDS;
extern u8 D_800ADFCC[];
extern u8 D_80062648[];
extern AudioManager* D_80062528;
extern AudioManager* D_8004F2FC;
void FieldBeginWdsFileLoad(void);
int FieldFinishWdsFileLoad(void);
int ArchiveDataSync(void);
void ArchiveReadFileToBuffer(int fileIndex, void* pBuffer, int arg2, int size);
AudioManager* SoundCreateAudioManagerOnHeap(void* pData);
void SoundPlayAudioManager(AudioManager* pManager, s32 arg1, s32 arg2);
void SoundSetAudioManagerVolumeWithFade(AudioManager* pManager, s32 arg1, s32 arg2);
void SoundReloadAudioManagerSamples(AudioManager* pManager, s32 arg1, s32 arg2);

int FieldUpdateWdsAndAudioLoad(s32 arg0) {
    s32 state = D_8004F354;

    if (state == 1) {
        if (FieldRunArchiveStreamSteps() == -1) {
            return -1;
        }
        func_8003BDFC(0x10);
        HeapFree(D_800C3A1C);
        D_8004F354 = 0;
        g_GameHasLoadedWDS = state;
        D_8004F33C = D_800ADFCC[arg0 * 2];
    }
    if (D_800ADFCD[arg0 * 2] == 0) {
        if (D_8004F364 == 0) {
            FieldBeginWdsFileLoad();
            return -1;
        }
        if (D_8004F364 & 0x80) {
            if (FieldFinishWdsFileLoad() == -1) {
                return -1;
            }
        }
    }
    if (D_800AFC54 == 1) {
        if (D_8004F338 != arg0) {
            ArchiveSetIndex(0x1C, 0);
            ArchiveReadFileToBuffer(arg0 * 2 + 0x14, D_80062648, 0, 0x80);
            D_8004F358 = 1;
            ArchiveSetIndex(4, 0);
        }
        D_800AFC54 = 0;
        return -1;
    }
    if (ArchiveDataSync() != 0) {
        return -1;
    }
    if (D_8004F358 == 1) {
        if (D_8004F348 == 0) {
            D_80062528 = SoundCreateAudioManagerOnHeap(D_80062648);
            if (D_8004F340 == -1) {
                SoundPlayAudioManager(D_80062528, 0x7F, 0);
            } else {
                SoundPlayAudioManager(D_80062528, 0, 0);
                SoundSetAudioManagerVolumeWithFade(D_80062528, 0, 0);
            }
        } else {
            D_80062528 = D_8004F2FC;
            SoundReloadAudioManagerSamples(D_8004F2FC, 0x7F, 0xF0);
            D_8004F348 = 0;
            D_8004F2FC = NULL;
        }
        D_8004F358 = 0;
        D_8004F35C = 1;
        D_8004F338 = arg0;
    }
    D_8004F340 = -1;
    D_8004F36C = 1;
    return 0;
}

void FieldFreePendingAudioManager(void) {
    if (D_8004F2FC != NULL) {
        SoundStopAudioManager(D_8004F2FC);
        SoundDestroyAudioManager(D_8004F2FC);
        D_8004F2FC = NULL;
    }
}

extern s32 D_8004F364;
extern s32 D_8004F368;
extern s16 D_8004F384;
extern s32 D_80059560;
extern SoundWDSEntry* D_8006251C;
extern void* D_800B00E0; // WDS File Buffer

int FieldFinishWdsFileLoad(void) {
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

void FieldBeginWdsFileLoad(void) {
    void* pWdsFileBuffer;

    ArchiveSetIndex(0x1C, 0x0);
    pWdsFileBuffer = HeapAlloc(ArchiveDecodeAlignedSize(3), 1);
    D_800B00E0 = pWdsFileBuffer;
    ArchiveReadFileToBuffer(3, pWdsFileBuffer, 0, CdlModeSpeed);
    ArchiveSetIndex(4, 0);
    D_8004F364 = 0x80;
}

void FieldReleaseWdsEntry(void) {
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

void FieldGetDistanceScaledValue(int arg0, int* pOut, int arg2) {
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

void FieldUpdateActorSoundSlot(int arg0, int arg1, int arg2, int arg3, int actorIndex) {
    int i;
    int value;
    int screenX;
    int screenY;
    int scaled;

    for (i = 0; i < 3; i++) {
        if (D_800AFE88[i].unk0 == actorIndex) {
            FieldGetDistanceScaledValue(arg3, &value, arg1);
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
   local pointer, in a `u_int` local, or writing `&g_FieldActors - 0xAC`
   directly all give 95.25%: gcc const-propagates and never reuses the base
   register for both addresses.
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

void FieldAssignActorSoundSlot(int arg0, int arg1, int arg2, int arg3, int actorIndex) {
    short value;
    int screenX;
    int screenY;
    int i;
    int scaled;

    for (i = 0; i < 3; i++) {
        if (D_800AFE8A[i].unk0 == 0xFFFF) {
            D_800AFE8A[i].unk0 = arg0;
            D_800AFE88[i].unk0 = actorIndex;
            FieldGetDistanceScaledValue(arg3, (int*)&value, arg1);
            FieldActorWorldToScreenPosition(actorIndex, &screenX, &screenY);
            if (screenX >= 0x141) {
                screenX = 0x13F;
            }
            if (screenX < 0) {
                screenX = 0;
            }
            scaled = screenX * 0x6666 >> 16;
            func_8003A20C(i * 2);
            SoundPlaySoundEffectOnPairWithLevels((void*)arg0, (short)(i * 2), value, scaled);
            break;
        }
    }
}

void FieldReleaseActorSoundSlot(u_int arg0) {
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

int FieldFindActorSoundSlot(int arg0, u_int arg1) {
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

void FieldClearActorSoundSlots(void) {
    int i;

    for (i = 0; i < 3; i++) {
        D_800AFE88[i].unk0 = 0xFFFF;
        D_800AFE8A[i].unk0 = 0xFFFF;
    }
}

extern u_short D_800B233C;

void FieldResetActorSoundSlots(void) {
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
/* 85.55% (864 bytes vs 872): the struct and all five loops are right, and the
   whole first block (0x10D test, the three magnitude args, the slot pick and
   the distance compare) is instruction-for-instruction. What got it there:
   g_FieldActors is volatile, so the one pActorData load the original shares
   between the 0x10D test and the three position reads needs a loop-body local
   (ActorData* pActor = g_FieldActors[i].pActorData), while every access after
   the magnitude call must spell the global out inline to reproduce the
   original's per-store reloads. gcc then keeps pActor in a2, as retail does.
   Residuals: (1) the delta stores come out lhu+subu+sh, the original
   lh+subu+sh (gcc narrows the loads once only the low halfword is live).
   (2) gcc hoists the g_FieldActors reload above the actorIndex/distance
   stores; retail stores first. (3) the slot*8 and distance-slot base land in
   a1/a0 the other way round. (4) the last loop: retail duplicates the
   increments after the FieldUpdateActorSoundSlot call (addiu s0,s0,8 in the jal delay
   slot, addiu s1,s1,1 in the j delay slot) and keeps them again at the shared
   continue block; gcc merges them into one copy, 8 bytes short.
typedef struct {
    s32 distance[4]; // 0x00
    s32 unk10[4]; // 0x10
    s32 unk20[4]; // 0x20
    s32 inUse[4]; // 0x30
    s32 unk40[4]; // 0x40
    s32 actorIndex[4]; // 0x50
    s16 delta[3][4]; // 0x60
} FieldActorProximitySlots; // Size: 0x78

void func_80086590(VECTOR* pPosition) {
    FieldActorProximitySlots slots;
    s16* pDelta;
    s16* pPos = (s16*)pPosition;
    s16* pActorPos;
    s32 distance;
    s32 slot;
    int i;

    for (i = 0; i < 3; i++) {
        slots.distance[i] = 0xFFFF;
        slots.unk10[i] = -1;
        slots.unk40[i] = 0;
        slots.inUse[i] = 0;
        slots.actorIndex[i] = 0;
        slots.unk20[i] = 0;
    }

    for (i = 0; i < D_800ADBFC; i++) {
        ActorData* pActor = g_FieldActors[i].pActorData;

        if (pActor->unk10D != 0xFF) {
            pActorPos = (s16*)&pActor->position;
            distance = FieldGetVec3Magnitude(
                pPos[1] - pActorPos[1],
                pPos[3] - pActorPos[3],
                pPos[5] - pActorPos[5]);
            if (slots.distance[0] < slots.distance[1]) {
                slot = (slots.distance[1] < slots.distance[2]) ? 2 : 1;
            } else {
                slot = (slots.distance[0] < slots.distance[2]) ? 2 : 0;
            }
            if (distance < slots.distance[slot]) {
                slots.actorIndex[slot] = i;
                slots.distance[slot] = distance;
                slots.unk10[slot] = g_FieldActors[i].pActorData->unk10A;
                slots.unk20[slot] = g_FieldActors[i].pActorData->unk10C;
                slots.delta[slot][0] = pPos[1] - ((s16*)&g_FieldActors[i].pActorData->position)[1];
                slots.delta[slot][1] = pPos[3] - ((s16*)&g_FieldActors[i].pActorData->position)[3];
                slots.delta[slot][2] = pPos[5] - ((s16*)&g_FieldActors[i].pActorData->position)[5];
            }
        } else {
            pActor->unk10D = 0xFF;
        }
    }

    for (i = 0; i < 3; i++) {
        slot = FieldFindActorSoundSlot(slots.unk10[i], slots.actorIndex[i]);
        if (slot != -1) {
            slots.inUse[slot] = 1;
            slots.unk40[i] = 1;
        }
    }

    for (i = 0; i < 3; i++) {
        if (slots.inUse[i] == 0) {
            if (D_800AFE8A[i].unk0 != 0xFFFF) {
                func_8003A20C(i * 2);
                D_800AFE8A[i].unk0 = 0xFFFF;
                D_800AFE88[i].unk0 = 0xFFFF;
            }
        }
    }

    pDelta = slots.delta[0];
    for (i = 0; i < 3; i++, pDelta += 4) {
        if (slots.unk10[i] == -1) {
            continue;
        }
        if (slots.unk40[i] == 1) {
            FieldUpdateActorSoundSlot(slots.unk10[i], slots.unk20[i], pDelta[0], slots.distance[i],
                slots.actorIndex[i]);
        } else {
            FieldAssignActorSoundSlot(slots.unk10[i], slots.unk20[i], pDelta[0], slots.distance[i],
                slots.actorIndex[i]);
        }
    }
}
*/

extern VECTOR g_CameraEye;
extern VECTOR g_CameraAt;
extern s16 D_800B22E0;
extern s32 g_PlayerActorIndex;

void FieldUpdateProximitySoundSlots(void) {
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

void FieldScriptVM2Run(void) {
    char *script = (char *)g_FieldScriptVMCurScriptData;
    size_t vmIP = ++(g_FieldScriptVMCurActor->scriptInstructionPointer);

    ScriptVMHandler *handler = &g_FieldScriptVMHandlers2[script[vmIP]];
    (*handler)();
}

extern s16 D_800B22E2[];

extern s16 D_800B22E8[];
extern s16 D_800B22EA[];
extern s16 D_800B22EC[];
extern s16 D_800B2300[];
extern s16 D_800B2302[];
extern s16 D_800B2304[];
extern s32 D_800B2318[];
extern s16 D_800B2324[];
extern s16 D_800B2326[];
extern s16 D_800B2328[];
long FieldGetVec3Magnitude(long x, long y, long z);

void FieldUpdateMotionSlotSteps(int index, VECTOR* pPosition) {
    s16* pPos = (s16*)pPosition;
    s32 stepX = ((D_800B22E8[index * 4] - D_800B2300[index * 4]) << 16) / D_800B2318[index];
    s32 stepY = ((D_800B22EA[index * 4] - D_800B2302[index * 4]) << 16) / D_800B2318[index];
    s32 stepZ = ((D_800B22EC[index * 4] - D_800B2304[index * 4]) << 16) / D_800B2318[index];
    s32 remaining;

    remaining = D_800B2318[index] - FieldGetVec3Magnitude(
        D_800B2324[index * 4] - pPos[1],
        D_800B2326[index * 4] - pPos[3],
        D_800B2328[index * 4] - pPos[5]);

    ((s16*)D_800B2318)[index - 0x6E] = (stepX * remaining) >> 16;
    ((s16*)D_800B2318)[index - 0x6B] = (stepY * remaining) >> 16;
    ((s16*)D_800B2318)[index - 0x68] = (stepZ * remaining) >> 16;
}

void FieldUpdateMotionSlots(void) {
    int i;

    for (i = 0; i < 3; i++) {
        if (D_800B22E2[i] != -1) {
            FieldUpdateMotionSlotSteps(i, &g_FieldActors[D_800B22E2[i]].pActorData->position);
        }
    }
}

extern s32 D_800AFD1C;
void FieldInitializeDefaultParticleBanks(int targetActorID);
int FieldInitializeParticleBanks(int actorIndex);

void FieldScriptConfigureDefaultParticleBank(void) {
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

void FieldScriptSoftReset(void) {
    GameSoftReset();
    D_800B00C0 = 1;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 1;
}
