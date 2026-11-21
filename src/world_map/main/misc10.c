#include "common.h"
#include "main/game.h"
#include "psyq/libgpu.h"
#include "psyq/libgte.h"
#include "psyq/inline_c.h"

extern s32 g_WorldMapLevelIndex;
extern u16 g_WorldMapLevelSpriteSetIdsHeightOffset[];
extern u16 g_WorldMapLevelSpriteSetIds[];
extern void* g_WorldMapPrimTemplateBuffer[];
extern void* g_WorldMapPrimTemplateBufferCopy;
extern s16 g_WorldMapSpriteSetIds[];

extern void* g_WorldMapPartyActorAnimFile0;

extern void HeapFree(void* pMem);
extern void* SpriteAllocFromAnimFile(void* pArg0, s16 arg1, s16 arg2, s16 arg3, s16 arg4, s16 arg5);
extern void SpritePlayAnimation(void* pSprite, s32 arg1);
extern void WorldMapSetSpriteSetParent(s32 arg0, s32 arg1);
extern void SpriteSetScale(void* pSprite, s16 scale);
extern void WorldMapQueueZoneEntry(s32 arg0, s32* pVec, s32 arg2, s32 arg3);
extern s32 WorldMapGetTilePlaneHeight(s32 x, s32 z);
extern void func_80089580(void);

#define WORLD_MAP_PRIM_TEMPLATE_COUNT 0x100
#define WORLD_MAP_PRIM_TEMPLATE_POOL_SIZE (WORLD_MAP_PRIM_TEMPLATE_COUNT * sizeof(POLY_FT4))

#define WORLD_MAP_SPRITE_COUNT 512
#define WORLD_MAP_SPRITES_PER_GROUP 8
#define WORLD_MAP_TRAIL_ENTRY_COUNT 256
#define WORLD_MAP_SPRITE_FLAG_ACTIVE 0x80

typedef struct {
    /* 0x0 */ s32 w[4];
} Block16; // Size: 0x10

typedef struct {
    /* 0x00 */ undefined32 unk00;
    /* 0x04 */ u16 command;
    /* 0x06 */ u16 targetSlot;
    /* 0x08 */ undefined32 unk08[6]; // 0x08..0x1F
    /* 0x20 */ u16 state;
    /* 0x22 */ u16 stateTimer;
    /* 0x24 */ s16 hidden; // nonzero: the slot update and draw loops skip this actor
    /* 0x26 */ undefined16 unk26;
    /* 0x28 */ s32 x;
    /* 0x2C */ s32 y;
    /* 0x30 */ s32 z;
    /* 0x34 */ s32 w; // fourth word of the x/y/z vector, copied whole into a WorldMapVec
    /* 0x38 */ s32 dirX;
    /* 0x3C */ u32 dirY;
    /* 0x40 */ s32 dirZ;
    /* 0x44 */ undefined32 unk44;
    /* 0x48 */ s16 angle;
    /* 0x4A */ s16 moveSpeed;
    /* 0x4C */ void* pSprite;
    /* 0x50 */ s32 targetX;
    /* 0x54 */ s32 targetZ;
    /* 0x58 */ s32 trailDelay;
    /* 0x5C */ s32 drawAngle;
    /* 0x60 */ undefined32 unk60[8]; // 0x60..0x7F
} WorldMapActor; // Size: 0x80

typedef struct {
    /* 0x0 */ s32 x;
    /* 0x4 */ s32 y;
    /* 0x8 */ s32 z;
    /* 0xC */ s32 w;
} WorldMapVec; // Size: 0x10

typedef struct {
    /* 0x00 */ WorldMapVec vec;
    /* 0x10 */ u16 angle;
    /* 0x12 */ u16 pad12;
} WorldMapTrail; // Size: 0x14

typedef struct {
    /* 0x0 */ s8 r;
    /* 0x1 */ s8 g;
    /* 0x2 */ s8 b;
    /* 0x3 */ s8 a;
} WorldMapColorStep; // Size: 0x4

typedef union {
    /* 0x0 */ s32 flags;
    /* 0x0 */ struct {
        /* 0x0 */ u16 count;
        /* 0x2 */ s16 active;
    } status;
} WorldMapTrailFlags; // Size: 0x4

typedef struct {
    /* 0x00 */ s16 spriteIndex;
    /* 0x02 */ s16 angle;
    /* 0x04 */ WorldMapTrailFlags trailFlags;
    /* 0x08 */ s32 x;
    /* 0x0C */ s32 y;
    /* 0x10 */ s32 z;
    /* 0x14 */ undefined32 unk14;
    /* 0x18 */ s32 velX;
    /* 0x1C */ s32 velY;
    /* 0x20 */ s32 velZ;
    /* 0x24 */ undefined32 unk24;
    /* 0x28 */ s32 accelX;
    /* 0x2C */ s32 accelY;
    /* 0x30 */ s32 accelZ;
    /* 0x34 */ undefined32 unk34;
    /* 0x38 */ u16 scaleX;
    /* 0x3A */ u16 scaleY;
    /* 0x3C */ u16 scaleStepX;
    /* 0x3E */ u16 scaleStepY;
    /* 0x40 */ u32 color;
    /* 0x44 */ WorldMapColorStep colorStep;
    /* 0x48 */ u16 tpage; // ABR field taken from the sprite's flags, copied into the prim's tpage
    /* 0x4A */ undefined16 pad4A;
} WorldMapTrailEntry; // Size: 0x4C

typedef struct {
    /* 0x00 */ u32 lifeTimerInit; // what lifeTimer is armed with when the slot is claimed
    /* 0x04 */ u32 lifeTimer;
    /* 0x08 */ s16 maxCount;
    /* 0x0A */ u16 count;
    /* 0x0C */ s16 trailCount;
    /* 0x0E */ s16 trailActive;
    /* 0x10 */ u16 interval;
    /* 0x12 */ u16 timer;
    /* 0x14 */ u16 x;
    /* 0x16 */ u16 y;
    /* 0x18 */ u16 z;
    /* 0x1A */ undefined16 pad1A;
    /* 0x1C */ u16 rotX;
    /* 0x1E */ u16 rotY;
    /* 0x20 */ u16 rotZ;
    /* 0x22 */ undefined16 pad22;
    /* 0x24 */ SVECTOR trailStart; // local point a trail is emitted from; rotated by rot, added to x/y/z
    /* 0x2C */ SVECTOR trailEnd; // local point a trail heads for; (trailEnd - trailStart) is its direction
    /* 0x34 */ s32 speed;
    /* 0x38 */ u16 accelX;
    /* 0x3A */ u16 accelY;
    /* 0x3C */ u16 accelZ;
    /* 0x3E */ undefined16 unk3E;
    /* 0x40 */ u16 trailStartSpread; // random spread of trailStart; flags & 0x4 uses it unrandomized
    /* 0x42 */ u16 trailEndSpread; // random spread of trailEnd; flags & 0x8 uses it unrandomized
    /* 0x44 */ u32 scale;
    /* 0x48 */ u32 scaleStep;
    /* 0x4C */ u8 color[0x3];
    /* 0x4F */ u8 flags;
    /* 0x50 */ u32 colorStep;
} WorldMapSprite; // Size: 0x54

// g_WorldMapSpriteGroups holds WORLD_MAP_SPRITES_PER_GROUP sprites per group
// (0x2A0 = 8 * 0x54).
typedef struct {
    /* 0x000 */ WorldMapSprite sprites[WORLD_MAP_SPRITES_PER_GROUP];
} WorldMapSpriteGroup; // Size: 0x2A0

// Same layouts as misc5.c, which declares them locally too.
typedef struct {
    /* 0x00 */ u8 unk0[0x4];
    /* 0x04 */ s16 command;
    /* 0x06 */ s16 targetSlot;
    /* 0x08 */ undefined8 unk08[0x18]; // 0x08..0x1F
    /* 0x20 */ s16 state;
    /* 0x22 */ u16 stateTimer;
    /* 0x24 */ s16 hidden; // nonzero: the slot update and draw loops skip this actor
    /* 0x26 */ u16 pad26;
    /* 0x28 */ s32 x;
    /* 0x2C */ s32 y;
    /* 0x30 */ s32 z;
    /* 0x34 */ s32 w; // fourth word of the x/y/z vector, copied whole into a WorldMapVec
    /* 0x38 */ u32 dirX;
    /* 0x3C */ u32 dirY;
    /* 0x40 */ u32 dirZ;
    /* 0x44 */ u8 pad44[0x4];
    /* 0x48 */ s16 angle;
    /* 0x4A */ s16 moveSpeed;
    /* 0x4C */ void* pSprite;
    /* 0x50 */ s32 targetX;
    /* 0x54 */ s32 targetZ;
    /* 0x58 */ u32 trailDelay;
    /* 0x5C */ u8 unk5C[0x24];
} WorldMapPlayer; // Size: 0x80

typedef struct WorldMapSceneLayer {
    /* 0x00 */ s16 hidden; // nonzero: the generic sprite-set render skips this entry
    /* 0x02 */ s16 partIndex; // index into the model's parts
    /* 0x04 */ u16 flags;
    /* 0x06 */ u16 pad6;
    /* 0x08 */ WorldMapVec pos;
    /* 0x18 */ SVECTOR rot;
    /* 0x20 */ MATRIX mat;
    /* 0x40 */ u8* pPrimHeader;
    /* 0x44 */ u8 unk44[0x4];
    /* 0x48 */ u8* pPrims;
    /* 0x4C */ u8* pPrimsCopy;
    /* 0x50 */ struct WorldMapSceneLayer* pParent;
} WorldMapSceneLayer; // Size: 0x54

extern WorldMapSpriteGroup* g_WorldMapSpriteGroups;
extern s32 g_WorldMapMode;
extern s32 g_WorldMapPlayerPosition;
extern s32 g_WorldMapPlayerPositionZ;
extern s32 g_WorldMapPlayerRotation;
extern WorldMapTrailEntry* g_WorldMapTrailEntries;
extern WorldMapPlayer* g_WorldMapSlots;
extern WorldMapSceneLayer* g_WorldMapGroups;
extern WorldMapVec g_WorldMapScrollTargetX;
extern WorldMapTrail g_WorldMapTrail[];
extern u16 g_WorldMapPlayerAngle;
extern u16 g_WorldMapTrailCursor;

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc10", func_80088B40);
/* 73.64%: structure confirmed (loop over g_WorldMapSpriteSetIds calling WorldMapSetSpriteSetParent(0x45,id),
   sh 0 at pDst+0x20, the 0x99 branch, then the >>12 tail shared with
   func_80088D00). Two walls, both seen elsewhere in this unit: (a) the tail
   folds +0x16A4 into the store displacements (0x16AC/0x16B0/0x16B4) where
   retail does addiu a1,a1,0x16A4 once and stores at 8/0xC/0x10; (b) both id
   loops want an `s32 id` instead of a bare `*pIds` (see WorldMapHideSpriteSetGroups). The
   0x40 frame vs retail's 0x20 comes from (b).
s32 func_80088B40(s32 arg0) {
    u8* pDst;
    u8* pBase;
    s32* pSrc;
    s16* pIds;
    s32 ret;

    pIds = g_WorldMapSpriteSetIds;
    while (*pIds != -1) {
        WorldMapSetSpriteSetParent(0x45, *pIds);
        pIds++;
    }
    pDst = g_WorldMapSlots + (arg0 << 7);
    *(s16*)(pDst + 0x20) = 0;
    pBase = g_WorldMapGroups;
    if (g_GameState.fieldScriptMemory[0] == 0x99) {
        *(s32*)(pDst + 0x28) = 0x2000000;
        *(s32*)(pDst + 0x30) = 0x4120000;
        *(s32*)(pDst + 0x2C) = 0;
        ret = 1;
    } else {
        *(s16*)(pBase + 0x16A4) = 1;
        pIds = g_WorldMapSpriteSetIds;
        while (*pIds != -1) {
            *(s16*)(pBase + *pIds * 0x54) = 1;
            pIds++;
        }
        ret = 3;
    }
    pSrc = (s32*)(pBase + 0x16A4);
    pSrc[2] = *(s32*)(pDst + 0x28) >> 12;
    pSrc[3] = *(s32*)(pDst + 0x2C) >> 12;
    pSrc[4] = *(s32*)(pDst + 0x30) >> 12;
    return ret;
}
*/

s32 WorldMapHideSpriteSetGroups(void) {
    s16* pIds;
    s32 id;

    g_WorldMapGroups[0x45].hidden = 1;
    // The guard and the loop test read the array separately, so the body loads
    // the id again; one `while ((id = *pIds) != -1)` merges them (lhu + sra, 16-byte frame).
    if (g_WorldMapSpriteSetIds[0] != -1) {
        pIds = g_WorldMapSpriteSetIds;
        do {
            id = *pIds++;
            g_WorldMapGroups[id].hidden = 1;
        } while (*pIds != -1);
    }
    return 3;
}

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc10", func_80088D00);
/* 84.68% (100 bytes, retail 100): only the g_WorldMapGroups load position and the
   0x16A4 constant fold differ. Retail loads the group pointer before the branch so
   that `addiu a1,a1,0x16A4` lands in the bne delay slot, and stores at 8/0xC/0x10;
   every spelling below sinks the load into the taken block and folds the +0x16A4
   into the displacements (0x16AC/0x16B0/0x16B4). Tried: the typed pointer formed
   before or inside the if, a u8* base with (s32)pGroup + 0x16A4, s32* pSrc[2..4],
   and the three stores written inline through g_WorldMapGroups[0x45] (that one
   reproduces the whole prologue, `li v0,0x99` last and the v1/v0 compare operands
   included, but still folds: 69.88%). The byte count is retail's, so this is a cse
   fold and not a missing instruction.
s32 func_80088D00(s32 arg0) {
    WorldMapActor* pActor;
    WorldMapSceneLayer* pGroup;

    pGroup = g_WorldMapGroups;
    pActor = &g_WorldMapSlots[arg0];
    if (g_GameState.fieldScriptMemory[0] == 0x99) {
        pGroup[0x45].pos.x = pActor->x >> 12;
        pGroup[0x45].pos.y = pActor->y >> 12;
        pGroup[0x45].pos.z = pActor->z >> 12;
    }
    return 1;
}
*/

s32 WorldMapSetActorPosFromSpriteSetWithHeightOffset(s32 slotIndex) {
    WorldMapPlayer* pPlayer;
    WorldMapSceneLayer* pLayer;

    pPlayer = g_WorldMapSlots + slotIndex;
    pLayer = &g_WorldMapGroups[g_WorldMapLevelSpriteSetIdsHeightOffset[g_WorldMapLevelIndex]];
    pPlayer->x = pLayer->pos.x << 12;
    pPlayer->y = (pLayer->pos.y << 12) + 0x40000;
    pPlayer->z = pLayer->pos.z << 12;
    return 1;
}

s32 WorldMapQueueZoneEntryHeight60(s32 slotIndex) {
    WorldMapQueueZoneEntry(slotIndex, &g_WorldMapSlots[slotIndex].x, 0x68, 0x60);
    return 1;
}

s32 WorldMapSetActorPosFromSpriteSet75(s32 slotIndex) {
    WorldMapPlayer* pPlayer;
    WorldMapSceneLayer* pLayer;

    pPlayer = g_WorldMapSlots + slotIndex;
    pLayer = &g_WorldMapGroups[75];
    pPlayer->x = pLayer->pos.x << 12;
    pPlayer->y = pLayer->pos.y << 12;
    pPlayer->z = pLayer->pos.z << 12;
    return 1;
}

s32 WorldMapQueueZoneEntryHeight1A6(s32 slotIndex) {
    WorldMapQueueZoneEntry(slotIndex, &g_WorldMapSlots[slotIndex].x, 0x10C, 0x1A6);
    return 1;
}

s32 WorldMapSetActorPosFromSpriteSet(s32 slotIndex) {
    WorldMapPlayer* pPlayer;
    WorldMapSceneLayer* pLayer;

    pLayer = &g_WorldMapGroups[g_WorldMapLevelSpriteSetIds[g_WorldMapLevelIndex]];
    pPlayer = g_WorldMapSlots + slotIndex;
    pPlayer->x = pLayer->pos.x << 12;
    pPlayer->y = pLayer->pos.y << 12;
    pPlayer->z = pLayer->pos.z << 12;
    return 1;
}

s32 WorldMapQueueZoneEntryHeight392(s32 slotIndex) {
    WorldMapQueueZoneEntry(slotIndex, &g_WorldMapSlots[slotIndex].x, 0xC9, 0x392);
    return 1;
}

s32 WorldMapReturnThreeA(void) {
    return 3;
}

s32 WorldMapReturnThreeB(void) {
    return 3;
}

void WorldMapInitSpriteGroupTables(void) {
    WorldMapSprite* pSprite;
    WorldMapTrailEntry* pTrail;
    s32 i;

    pSprite = g_WorldMapSpriteGroups->sprites;
    for (i = WORLD_MAP_SPRITE_COUNT - 1; i != -1; i--) {
        pSprite->lifeTimer = 0;
        pSprite->count = 0;
        pSprite->timer = 0;
        pSprite->z = 0;
        pSprite->y = 0;
        pSprite->x = 0;
        pSprite->rotZ = 0;
        pSprite->rotY = 0;
        pSprite->rotX = 0;
        pSprite++;
    }
    g_WorldMapTrailEntries = HeapAlloc(WORLD_MAP_TRAIL_ENTRY_COUNT * sizeof(WorldMapTrailEntry), 0);
    pTrail = g_WorldMapTrailEntries;
    for (i = WORLD_MAP_TRAIL_ENTRY_COUNT - 1; i != -1; i--) {
        pTrail->trailFlags.status.active = 0;
        pTrail->trailFlags.status.count = 0;
        pTrail++;
    }
}

void WorldMapFreeSpriteGroupTable(void) {
    HeapFree(g_WorldMapTrailEntries);
}

void WorldMapInitPrimTemplateBuffers(void) {
    POLY_FT4* pPrim;
    Block16* pSrc;
    Block16* pDst;
    Block16* pEnd;
    s32 i;

    g_WorldMapPrimTemplateBuffer[0] = HeapAlloc(WORLD_MAP_PRIM_TEMPLATE_POOL_SIZE, 1);
    g_WorldMapPrimTemplateBufferCopy = HeapAlloc(WORLD_MAP_PRIM_TEMPLATE_POOL_SIZE, 1);
    pPrim = g_WorldMapPrimTemplateBuffer[0];
    for (i = WORLD_MAP_PRIM_TEMPLATE_COUNT - 1; i != -1; i--) {
        // the GPU takes the packet's word count from the tag's high byte; a POLY_FT4 is 9 words past its tag
        ((u8*)&pPrim->tag)[3] = 9;
        pPrim->code = 0x2C; // POLY_FT4
        pPrim->tpage = GetTPage(1, 1, 0x340, 0x100);
        pPrim->clut = GetClut(0x100, 0x1FF);
        pPrim->code |= 2; // semi-transparent
        pPrim++;
    }
    pSrc = g_WorldMapPrimTemplateBuffer[0];
    pDst = g_WorldMapPrimTemplateBufferCopy;
    pEnd = pSrc + WORLD_MAP_PRIM_TEMPLATE_POOL_SIZE / sizeof(Block16);
    do {
        *pDst = *pSrc;
        pSrc++;
        pDst++;
    } while (pSrc != pEnd);
}
void WorldMapFreePrimTemplateBuffers(void) {
    HeapFree(g_WorldMapPrimTemplateBuffer[0]);
    HeapFree(g_WorldMapPrimTemplateBufferCopy);
}

void WorldMapInitSpriteGroupSprites(s32 groupIndex, SVECTOR* pPos, s16* pRot) {
    WorldMapSprite* pSprite;
    s32 i;
    s32 found;

    found = 0;
    pSprite = g_WorldMapSpriteGroups[groupIndex].sprites;
    // The original retests the group's first sprite 8 times: nothing advances pSprite here.
    for (i = WORLD_MAP_SPRITES_PER_GROUP - 1; i != -1; i--) {
        if (pSprite->flags & WORLD_MAP_SPRITE_FLAG_ACTIVE) {
            found++;
            break;
        }
    }
    pSprite = g_WorldMapSpriteGroups[groupIndex].sprites;
    // Both null tests are evaluated and and-ed (retail's sltiu/sltiu/and); `&&` branches instead.
    if ((pPos == 0) & (pRot == 0)) {
        for (i = WORLD_MAP_SPRITES_PER_GROUP - 1; i != -1; i--) {
            if (found == 0) {
                u16 interval;
                u32 lifeTimer;

                pSprite->flags |= WORLD_MAP_SPRITE_FLAG_ACTIVE;
                interval = pSprite->interval;
                lifeTimer = pSprite->lifeTimerInit;
                pSprite->count = 0;
                pSprite->timer = interval;
                pSprite->lifeTimer = lifeTimer;
            }
            // x/y and rotX/rotY are zeroed as one word each; two halfword stores do not reproduce it.
            *(s32*)&pSprite->rotX = 0;
            *(s32*)&pSprite->x = 0;
            pSprite->rotZ = 0;
            pSprite->z = 0;
            pSprite++;
        }
    } else if ((pPos != 0) & (pRot == 0)) {
        for (i = WORLD_MAP_SPRITES_PER_GROUP - 1; i != -1; i--) {
            if (found == 0) {
                u16 interval;
                u32 lifeTimer;

                pSprite->flags |= WORLD_MAP_SPRITE_FLAG_ACTIVE;
                interval = pSprite->interval;
                lifeTimer = pSprite->lifeTimerInit;
                pSprite->count = 0;
                pSprite->timer = interval;
                pSprite->lifeTimer = lifeTimer;
            }
            *(SVECTOR*)&pSprite->x = *pPos;
            *(s32*)&pSprite->rotX = 0;
            pSprite->rotZ = 0;
            pSprite++;
        }
    } else if ((pPos == 0) & (pRot != 0)) {
        for (i = WORLD_MAP_SPRITES_PER_GROUP - 1; i != -1; i--) {
            if (found == 0) {
                u16 interval;
                u32 lifeTimer;

                pSprite->flags |= WORLD_MAP_SPRITE_FLAG_ACTIVE;
                interval = pSprite->interval;
                lifeTimer = pSprite->lifeTimerInit;
                pSprite->count = 0;
                pSprite->timer = interval;
                pSprite->lifeTimer = lifeTimer;
            }
            *(s32*)&pSprite->x = 0;
            pSprite->z = 0;
            pSprite->rotX = -pRot[0];
            pSprite->rotY = -pRot[1];
            pSprite->rotZ = -pRot[2];
            pSprite++;
        }
    } else {
        for (i = WORLD_MAP_SPRITES_PER_GROUP - 1; i != -1; i--) {
            if (found == 0) {
                u16 interval;
                u32 lifeTimer;

                pSprite->flags |= WORLD_MAP_SPRITE_FLAG_ACTIVE;
                interval = pSprite->interval;
                lifeTimer = pSprite->lifeTimerInit;
                pSprite->count = 0;
                pSprite->timer = interval;
                pSprite->lifeTimer = lifeTimer;
            }
            *(SVECTOR*)&pSprite->x = *pPos;
            pSprite->rotX = -pRot[0];
            pSprite->rotY = -pRot[1];
            pSprite->rotZ = -pRot[2];
            pSprite++;
        }
    }
}

void WorldMapInitSpriteGroupSpritesTrail(s32 groupIndex, SVECTOR* pTrailEnd, SVECTOR* pRot) {
    WorldMapSprite* pSprite;
    s32 i;
    s32 found;

    found = 0;
    pSprite = g_WorldMapSpriteGroups[groupIndex].sprites;
    for (i = WORLD_MAP_SPRITES_PER_GROUP - 1; i != -1; i--) {
        if (pSprite->flags & WORLD_MAP_SPRITE_FLAG_ACTIVE) {
            found++;
            break;
        }
    }
    pSprite = g_WorldMapSpriteGroups[groupIndex].sprites;
    for (i = WORLD_MAP_SPRITES_PER_GROUP - 1; i != -1; i--) {
        if (found == 0) {
            u16 interval;
            u32 lifeTimer;

            pSprite->flags |= WORLD_MAP_SPRITE_FLAG_ACTIVE;
            interval = pSprite->interval;
            lifeTimer = pSprite->lifeTimerInit;
            pSprite->count = 0;
            pSprite->timer = interval;
            pSprite->lifeTimer = lifeTimer;
        }
        *(s32*)&pSprite->trailEnd = *(s32*)pTrailEnd;
        *(s32*)&pSprite->x = 0;
        *(s32*)&pSprite->rotX = *(s32*)pRot;
        pSprite->trailEnd.vz = pTrailEnd->vz;
        pSprite->z = 0;
        pSprite->rotZ = pRot->vz;
        pSprite++;
    }
}

void WorldMapDeactivateSpriteGroup(s32 groupIndex) {
    WorldMapSprite* pSprite;
    s32 i;

    pSprite = g_WorldMapSpriteGroups[groupIndex].sprites;
    for (i = WORLD_MAP_SPRITES_PER_GROUP - 1; i != -1; i--) {
        pSprite->flags &= ~WORLD_MAP_SPRITE_FLAG_ACTIVE;
        pSprite++;
    }
}

void WorldMapClearGroupEntryFlag(s32 groupIndex) {
    WorldMapTrailEntry* pEntry;
    s32 i;
    s32 j;

    pEntry = g_WorldMapTrailEntries;
    for (i = WORLD_MAP_TRAIL_ENTRY_COUNT - 1; i != -1; i--) {
        for (j = 0; j < WORLD_MAP_SPRITES_PER_GROUP; j++) {
            if (pEntry->spriteIndex == (groupIndex << 3) + j && pEntry->trailFlags.status.active != 0) {
                pEntry->trailFlags.status.count = 0;
                break;
            }
        }
        pEntry++;
    }
}

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc10", func_80089580);
/* 48.70% (436 bytes, retail 456): struct layout recovered and the head/tests/
   arithmetic are right; two differences remain.
   (1) The count/active word at entry+0x04: retail loads it ONCE as a word and
   splits it with sra 16 (active, jump to loop end when zero) and sll 16/sra 16
   (count, blez to the detach path), so the source reads it as an s32 and the
   WorldMapTrailFlags union below reproduces that head exactly. Retail still
   decrements the count with a plain lhu/sh at +0 and zeroes both halves with a
   single sw in the detach path, which is why the union needs all three members.
   (2) sched2 ties: retail loads all six pos/vel words first, does the six adds,
   and sinks all six stores to the end of the green path; gcc here emits
   load/add/store per statement, and gcc also hoists the g_WorldMapSpriteGroups load out of
   the loop whereas retail reloads it inside the out-of-line detach block.
   Tried: six separate += statements in retail's own add order, nested if/else vs
   continue, u16/s16 for the two halves (the unsigned form is what makes gcc emit
   the separate lhu+beqz instead of the word split).
   The colour step at entry+0x44 is a word in retail: r comes from lb +0, g and b
   from (word<<16)>>24 and (word<<8)>>24, i.e. the source truncates a loaded
   word rather than reading three s8 fields (three lb's was 46.61%).
void func_80089580(void) {
    WorldMapTrailEntry* pEntry;
    s32 i;
    s32 flags;
    s32 r;
    s32 g;
    s32 b;

    pEntry = (WorldMapTrailEntry*)g_WorldMapTrailEntries;
    for (i = 0xFF; i != -1; i--, pEntry++) {
        flags = pEntry->trailFlags.flags;
        if ((flags >> 16) != 0) {
            if ((s16)flags > 0) {
                pEntry->x += pEntry->velX;
                pEntry->y += pEntry->velY;
                pEntry->z += pEntry->velZ;
                pEntry->velX += pEntry->accelX;
                pEntry->velY += pEntry->accelY;
                pEntry->velZ += pEntry->accelZ;
                pEntry->trailFlags.status.count = pEntry->trailFlags.status.count - 1;
                pEntry->scaleX += pEntry->scaleStepX;
                pEntry->scaleY += pEntry->scaleStepY;
                r = (pEntry->color & 0xFF) + pEntry->colorStep.r;
                g = ((pEntry->color >> 8) & 0xFF) + pEntry->colorStep.g;
                b = ((pEntry->color >> 16) & 0xFF) + pEntry->colorStep.b;
                if (r < 0) {
                    r = 0;
                }
                if (r >= 0x100) {
                    r = 0xFF;
                }
                if (g < 0) {
                    g = 0;
                }
                if (g >= 0x100) {
                    g = 0xFF;
                }
                if (b < 0) {
                    b = 0;
                }
                if (b >= 0x100) {
                    b = 0xFF;
                }
                pEntry->color = (pEntry->color & 0xFF000000) | (b << 16) | (g << 8) | r;
            } else {
                ((WorldMapSprite*)g_WorldMapSpriteGroups)[pEntry->spriteIndex].count--;
                pEntry->spriteIndex = 0;
                pEntry->trailFlags.flags = 0;
            }
        }
    }
}
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc10", func_80089748);
/* 1324 bytes against retail's 1328, head byte-identical through the flags/0x10 block;
   objdiff-cli reports 0.00% on this body (target_size comes back null), so score it
   with the instruction diff, not the percentage. Structure recovered: 0x200-sprite
   loop over g_WorldMapSpriteGroups, the packed low/high lifeTimer pair that is only
   reloaded under flags & 0x10 and stored back at the tail of every active sprite
   (`high << 16 | low`), the timer/interval reload, the trailActive&maxCount&count gate,
   the 0xFF-entry scan for a free trail slot, and inside it RotMatrixYXZ of
   pSprite->rotX into 0x1F8000F0, two ApplyMatrix calls (trailStart at 0x24, trailEnd at
   0x2C) against the scratch vector at 0x1F800020, three rand()-seeded direction vectors
   normalised into 0x1F800000 (the 0x14 component is skipped under flags & 0x20 / 0x40),
   the spread pair (0x40/0x42 fields vs rand() % field under flags & 4 / 8), the
   entry->x/y/z = (out << 12) + in * spread + (sprite->xyz << 12) blocks, the difference
   >> 12, the second VectorNormal feeding velX/Y/Z = (vec * speed) >> 12, ratan2 of the
   x/y components for entry->angle, and the word copies of accel/scale/color/colorStep/
   scaleStep plus tpage = (flags & 3) << 5 | 0x9D. Remaining delta: my compile keeps the
   sprite base in s5 where retail uses s6 (and swaps fp/s6 for the lifeTimer pair), so
   the prologue save order and the 0x40 vs 0x38 frame differ, and the flags ^= 0x80 arm
   is a taken jump instead of retail's fall-through into the store. pScratch
   ((u8 *)0x1F800000) is load-bearing: absolute `*(s32 *)0x1F800004`-style accesses make
   gcc hoist 0x1F800004/0x1F800008 into two extra callee-saved registers.

/*
void func_80089748(void) {
    WorldMapSprite* pSprite;
    WorldMapTrailEntry* pEntry;
    u8* pScratch;
    s32 i;
    s32 j;
    s32 flags;
    s32 lifeTimer;
    s32 low;
    s32 high;
    s32 spread;

    pScratch = (u8*)0x1F800000;
    pSprite = g_WorldMapSpriteGroups->sprites;
    for (i = 0; i < 0x200; i++, pSprite++) {
        flags = pSprite->flags;
        if (!(flags & 0x80)) {
            continue;
        }
        // low/high live across iterations: the original only reloads them under flags & 0x10
        // and stores them back at the tail of every active sprite.
        if (flags & 0x10) {
            lifeTimer = pSprite->lifeTimer;
            high = lifeTimer >> 16;
            low = (s16)lifeTimer;
            if (low != 0) {
                low--;
                goto storeLifeTimer;
            }
            if (high == 0) {
                pSprite->flags ^= 0x80;
                goto storeLifeTimer;
            }
            high--;
        }
        if (pSprite->timer != 0) {
            pSprite->timer--;
            goto storeLifeTimer;
        }
        pSprite->timer = pSprite->interval;
        if (!((pSprite->trailActive > 0) & (pSprite->maxCount > 0) & (pSprite->count < pSprite->maxCount))) {
            goto storeLifeTimer;
        }
        pEntry = g_WorldMapTrailEntries;
        for (j = 0xFF; j != -1; j--) {
            if (pEntry->trailFlags.status.active == 0) {
                pEntry->spriteIndex = i;
                pEntry->trailFlags.flags = *(u32*)&pSprite->trailCount;
                RotMatrixYXZ((SVECTOR*)&pSprite->rotX, (MATRIX*)(pScratch + 0xF0));
                ApplyMatrix((MATRIX*)(pScratch + 0xF0), (SVECTOR*)&pSprite->trailStart, (VECTOR*)(pScratch + 0x20));
                if (flags & 0x20) {
                    *(s32*)(pScratch + 0x014) = 0;
                } else {
                    *(s32*)(pScratch + 0x014) = (rand() & 0xFFF) - 0x800;
                }
                *(s32*)(pScratch + 0x010) = (rand() & 0xFFF) - 0x800;
                *(s32*)(pScratch + 0x018) = (rand() & 0xFFF) - 0x800;
                VectorNormal((VECTOR*)(pScratch + 0x10), (VECTOR*)(pScratch + 0x00));
                if (flags & 0x4) {
                    spread = pSprite->trailStartSpread;
                } else {
                    spread = rand() % pSprite->trailStartSpread;
                }
                pEntry->x = (*(s32*)(pScratch + 0x020) << 12) + *(s32*)(pScratch + 0x000) * spread + (pSprite->x << 12);
                pEntry->y = (*(s32*)(pScratch + 0x024) << 12) + *(s32*)(pScratch + 0x004) * spread + (pSprite->y << 12);
                pEntry->z = (*(s32*)(pScratch + 0x028) << 12) + *(s32*)(pScratch + 0x008) * spread + (pSprite->z << 12);
                ApplyMatrix((MATRIX*)(pScratch + 0xF0), (SVECTOR*)&pSprite->trailEnd, (VECTOR*)(pScratch + 0x20));
                if (flags & 0x40) {
                    *(s32*)(pScratch + 0x014) = 0;
                } else {
                    *(s32*)(pScratch + 0x014) = (rand() & 0xFFF) - 0x800;
                }
                *(s32*)(pScratch + 0x010) = (rand() & 0xFFF) - 0x800;
                *(s32*)(pScratch + 0x018) = (rand() & 0xFFF) - 0x800;
                VectorNormal((VECTOR*)(pScratch + 0x10), (VECTOR*)(pScratch + 0x00));
                if (flags & 0x8) {
                    spread = pSprite->trailEndSpread;
                } else {
                    spread = rand() % pSprite->trailEndSpread;
                }
                *(s32*)(pScratch + 0x000) = (*(s32*)(pScratch + 0x020) << 12) + *(s32*)(pScratch + 0x000) * spread + (pSprite->x << 12);
                *(s32*)(pScratch + 0x004) = (*(s32*)(pScratch + 0x024) << 12) + *(s32*)(pScratch + 0x004) * spread + (pSprite->y << 12);
                *(s32*)(pScratch + 0x008) = (*(s32*)(pScratch + 0x028) << 12) + *(s32*)(pScratch + 0x008) * spread + (pSprite->z << 12);
                *(s32*)(pScratch + 0x000) = (*(s32*)(pScratch + 0x000) - pEntry->x) >> 12;
                *(s32*)(pScratch + 0x004) = (*(s32*)(pScratch + 0x004) - pEntry->y) >> 12;
                *(s32*)(pScratch + 0x008) = (*(s32*)(pScratch + 0x008) - pEntry->z) >> 12;
                VectorNormal((VECTOR*)(pScratch + 0x00), (VECTOR*)(pScratch + 0x10));
                pEntry->velX = (*(s32*)(pScratch + 0x010) * pSprite->speed) >> 12;
                pEntry->velY = (*(s32*)(pScratch + 0x014) * pSprite->speed) >> 12;
                pEntry->velZ = (*(s32*)(pScratch + 0x018) * pSprite->speed) >> 12;
                pEntry->angle = ratan2(*(s32*)(pScratch + 0x014), *(s32*)(pScratch + 0x010));
                flags &= 3;
                pEntry->accelX = pSprite->accelX;
                pEntry->accelY = pSprite->accelY;
                pEntry->accelZ = pSprite->accelZ;
                *(u32*)&pEntry->scaleX = pSprite->scale;
                *(u32*)&pEntry->colorStep = pSprite->colorStep;
                pEntry->color = *(u32*)&pSprite->color[0];
                pEntry->tpage = (flags << 5) | 0x9D;
                *(u32*)&pEntry->scaleStepX = pSprite->scaleStep;
                pSprite->count++;
                break;
            }
            pEntry++;
        }
    storeLifeTimer:
        pSprite->lifeTimer = (high << 16) | low;
    }
    func_80089580();
}
*/
// 0x1F800000 scratchpad view this routine projects through. The three matrices and the
// delta are addressed by their own literals in retail, so the byte offsets are the contract.
typedef struct {
    /* 0x00 */ SVECTOR quad[4]; // the sprite's four vertices, loaded by RTPT and RTPS
    /* 0x20 */ SVECTOR delta;   // wrapped world-space delta fed to the view rotation
    /* 0x28 */ MATRIX view;     // copy of g_WorldMapViewMatrix
    /* 0x48 */ MATRIX sprite;   // identity or RotMatrixZ, scaled, then given the translation
    /* 0x68 */ MATRIX identity; // copy of g_WorldMapIdentityMatrix
    /* 0x88 */ s32 wrap[4];     // delta WorldMapWrapDeltaScaled wraps (x, unused, z)
    /* 0x98 */ s32 scale[3];    // the sprite matrix's x, y and depth scale
    /* 0xA4 */ u8 unkA4[8];
    /* 0xAC */ s32 flag;        // GTE FLAG read back after each projection
    /* 0xB0 */ s32 otz;         // SZ3 of the fourth vertex: the ordering table index
} WorldMapTrailScratch; // Size: 0xB4

// A vertex's four texture coordinates and its position come from parallel tables whose
// fourth element retail reaches through its own symbol, so they stay two names each.
extern u16 D_8009AFF0[];
extern u16 D_8009AFF6[];
extern u8 D_8009B040[];
extern u8 D_8009B058[];

typedef struct {
    /* 0x0 */ u8 bytes[8];
} WorldMapSpriteQuadVertex; // Size: 0x8

typedef struct {
    /* 0x00 */ u8 unk00[0x70];
    /* 0x70 */ u_long* pOt[2];
} WorldMapRenderEnv; // partial view: this routine only links through the first table

extern MATRIX g_WorldMapViewMatrix;
extern MATRIX g_WorldMapIdentityMatrix;
extern s32 g_WorldMapCameraTarget;
extern s32 g_WorldMapCameraTargetZ;
extern s32 g_WorldMapPrimPacketIndex;
extern WorldMapTrailEntry* g_WorldMapTrailEntries;
extern WorldMapRenderEnv* g_WorldMapRenderContext;
extern void WorldMapWrapDeltaScaled(s32* pDelta);

#define WORLD_MAP_TRAIL_SCREEN_W 320
#define WORLD_MAP_TRAIL_SCREEN_H 216
#define WORLD_MAP_TRAIL_SZ_MAX 0xC00

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc10", func_80089C78);
/* 85.25% first pass (1648 bytes against retail's 1616, frame 0x48 against retail's 0x50).
   Structure recovered and byte-identical through the whole two-matrix head and the
   per-sprite body: the 0x100-entry WorldMapTrailEntry walk, the identity/RotMatrixZ and
   ScaleMatrix pair into 0x1F800048, the four 8-byte vertex moves out of the 0x8009B040
   table (lwl/lwr, so the copy needs an alignment-1 8-byte aggregate - the
   WorldMapSpriteQuadVertex byte-array below is what produces it), the wrap/delta/MVMVA
   transform, the RTPT + RTPS pair, the screen and SZ3 gates and the addPrim tail.
   The right macros exist and were the whole trick: gte_SetRotMatrixRows_gpr and
   gte_SetRotTransMatrix_gpr in psyq/inline_c.h are EXACTLY retail's interleaved
   $12/$13/$14 ctc2 sequences, gte_mvmva(1,0,0,3,0), gte_rtpt, gte_rtps, gte_stflg,
   gte_stsxy3/gte_stsxy2, gte_stslvnl... i.e. gte_stlvnl, gte_stsz.
   Residual (all allocator/ordering, nothing structural):
   (1) retail keeps a SECOND induction pointer for pEntry: mine emits
       `addiu s2,s2,76` in the loop tail on top of retail's `addiu s0,s0,76`, so the
       body is one instruction long there - the pointer update wants to happen once.
   (2) retail's prologue interleaves the callee-saved stores with the two matrix copies
       (s1 = 0x1F800000 is saved and built FIRST); mine defers all of them past the
       copies, which is most of the remaining frame/save-order delta.
   (3) the scratch base lands in s0 here and s1 in retail.
   Retail sources: 0x8009AFF0/0x8009AFF6 are the interleaved u/v quad (the fourth element
   reached through its own symbol, hence the two names) and 0x8009B040/0x8009B058 the same
   split for the four vertices.
/*
void func_80089C78(void) {
    WorldMapTrailScratch* pScratch;
    WorldMapTrailEntry* pEntry;
    POLY_FT4* pPrim;
    s32 camX;
    s32 camZ;
    s32 i;

    pScratch = (WorldMapTrailScratch*)PSX_SCRATCH;
    *(MATRIX*)0x1F800028 = g_WorldMapViewMatrix;
    *(MATRIX*)0x1F800068 = g_WorldMapIdentityMatrix;
    i = 0;
    camX = g_WorldMapCameraTarget >> 12;
    camZ = g_WorldMapCameraTargetZ >> 12;
    pPrim = (POLY_FT4*)g_WorldMapPrimTemplateBuffer[g_WorldMapPrimPacketIndex];
    for (pEntry = g_WorldMapTrailEntries; i < WORLD_MAP_TRAIL_ENTRY_COUNT; i++, pEntry++) {
        if (pEntry->trailFlags.status.active == 0) {
            continue;
        }
        pScratch->scale[0] = pEntry->scaleX;
        pScratch->scale[1] = pEntry->scaleY;
        pScratch->scale[2] = 0x1000;
        *(MATRIX*)0x1F800048 = *(MATRIX*)0x1F800068;
        if (pEntry->colorStep.a & 1) {
            RotMatrixZ(pEntry->angle, (MATRIX*)0x1F800048);
        }
        ScaleMatrix((MATRIX*)0x1F800048, (VECTOR*)&pScratch->scale[0]);
        *(WorldMapSpriteQuadVertex*)((u8*)pScratch + 0x00) =
            *(WorldMapSpriteQuadVertex*)(D_8009B040 + pEntry->spriteIndex * 0x20);
        *(WorldMapSpriteQuadVertex*)((u8*)pScratch + 0x08) =
            *(WorldMapSpriteQuadVertex*)(D_8009B040 + pEntry->spriteIndex * 0x20 + 0x08);
        *(WorldMapSpriteQuadVertex*)((u8*)pScratch + 0x10) =
            *(WorldMapSpriteQuadVertex*)(D_8009B040 + pEntry->spriteIndex * 0x20 + 0x10);
        *(WorldMapSpriteQuadVertex*)((u8*)pScratch + 0x18) =
            *(WorldMapSpriteQuadVertex*)(D_8009B058 + pEntry->spriteIndex * 0x20);
        pScratch->wrap[0] = (pEntry->x >> 12) - camX;
        pScratch->wrap[2] = (pEntry->z >> 12) - camZ;
        WorldMapWrapDeltaScaled((s32*)0x1F800088);
        pScratch->delta.vx = pScratch->wrap[0];
        pScratch->delta.vz = -pScratch->wrap[2];
        pScratch->delta.vy = pEntry->y >> 12;
        gte_SetRotMatrixRows_gpr(&pScratch->view);
        gte_ldv0(&pScratch->delta);
        gte_mvmva(1, 0, 0, 3, 0);
        gte_stlvnl((s32*)0x1F800088);
        pScratch->sprite.t[0] = pScratch->wrap[0] + pScratch->view.t[0];
        pScratch->sprite.t[1] = pScratch->wrap[1] + pScratch->view.t[1];
        pScratch->sprite.t[2] = pScratch->wrap[2] + pScratch->view.t[2];
        gte_SetRotTransMatrix_gpr(&pScratch->sprite);
        gte_ldv3(&pScratch->quad[0], &pScratch->quad[1], &pScratch->quad[2]);
        gte_rtpt();
        gte_stflg(&pScratch->flag);
        if (pScratch->flag < 0) {
            continue;
        }
        gte_stsxy3(&pPrim->x0, &pPrim->x1, &pPrim->x2);
        gte_ldv0(&pScratch->quad[3]);
        gte_rtps();
        gte_stflg(&pScratch->flag);
        if (pScratch->flag & 0x80000000) {
            continue;
        }
        gte_stsxy2(&pPrim->x3);
        if (pPrim->x0 < WORLD_MAP_TRAIL_SCREEN_W || pPrim->x1 < WORLD_MAP_TRAIL_SCREEN_W ||
            pPrim->x2 < WORLD_MAP_TRAIL_SCREEN_W || pPrim->x3 < WORLD_MAP_TRAIL_SCREEN_W) {
            if (pPrim->y0 < WORLD_MAP_TRAIL_SCREEN_H || pPrim->y1 < WORLD_MAP_TRAIL_SCREEN_H ||
                pPrim->y2 < WORLD_MAP_TRAIL_SCREEN_H || pPrim->y3 < WORLD_MAP_TRAIL_SCREEN_H) {
                gte_stsz(&pScratch->otz);
                if (pScratch->otz < WORLD_MAP_TRAIL_SZ_MAX) {
                    pPrim->r0 = pEntry->color & 0xFF;
                    pPrim->g0 = (pEntry->color >> 8) & 0xFF;
                    pPrim->b0 = (pEntry->color >> 16) & 0xFF;
                    pPrim->tpage = pEntry->tpage;
                    *(u16*)&pPrim->u0 = D_8009AFF0[pEntry->spriteIndex * 4 + 0];
                    *(u16*)&pPrim->u1 = D_8009AFF0[pEntry->spriteIndex * 4 + 1];
                    *(u16*)&pPrim->u2 = D_8009AFF0[pEntry->spriteIndex * 4 + 2];
                    *(u16*)&pPrim->u3 = D_8009AFF6[pEntry->spriteIndex * 4];
                    addPrim(&g_WorldMapRenderContext->pOt[0][pScratch->otz >> 4], (P_TAG*)pPrim);
                    pPrim++;
                }
            }
        }
    }
}
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc10", func_8008A2C8);
/* 95.46% (612 bytes, retail 612): every instruction matches except the
   prologue, which is reordered and one register off. Retail emits
   `sll a2,a0,7` early and `addu s0,v1,a2` right before the jal, so reorg can
   put `li a2,0x1E0` in the jal delay slot; gcc emits the shift into v1, the
   g_WorldMapSlots load into v0 (retail: v1), the 0x1E0 constant early and the
   addu in the delay slot. Everything from `move a0,v0` on is byte-identical
   (the switch and its cross-jumped case 4-6 / case 7 body, the 32-entry trail
   copy, the 0x1820 tail).
   Tried, all byte-identical at 95.46%: `&g_WorldMapSlots[index]`,
   `(u8*)g_WorldMapSlots + index * 0x80`, a named s32 offset local, pActor
   declared first or last, the alloc call as the store's rhs, a void* pSprite
   temp. cdk -O2: 71.79. gcc 2.7.2 -O3: 95.46. A 4-minute permuter run found
   nothing (base 535, best 535).
   Two VOLATILE casts below are load-bearing, not debris:
   (1) the s16 store of `angle` is what makes gcc re-read it with `lh` for
   drawAngle (retail does sh 0x48, lh 0x48, sw 0x5C); without it gcc folds the
   re-read into sll/sra and the function comes out 4 bytes long.
   (2) the s32 store of x in the case 1-3 body stops gcc sinking that store
   below the g_WorldMapPlayerPositionZ load, giving retail's lw/nop/sw order.
   The head reads g_GameState.fieldScriptMemory[0]/[1] (0x1930/0x1932), not
   worldMapPosX/PosZ, while the tail writes 0x1820/0x1822.
s32 func_8008A2C8(s32 index) {
    WorldMapActor* pActor;
    WorldMapTrail* pTrail;
    s32 i;
    s32 mode;
    s32 angle;

    pActor = (WorldMapActor*)((u8*)g_WorldMapSlots + index * 0x80);
    pActor->pSprite = SpriteAllocFromAnimFile(g_WorldMapPartyActorAnimFile0, 0x100, 0x1E0, 0x140, 0x100, 0x40);
    SpritePlayAnimation(pActor->pSprite, 0);
    SpriteSetScale(pActor->pSprite, 0x1800);
    *(u32*)((u8*)pActor->pSprite + 0x3C) &= ~4;
    pActor->hidden = 0;
    pActor->x = g_GameState.fieldScriptMemory[0] << 12;
    pActor->z = g_GameState.fieldScriptMemory[1] << 12;
    pActor->y = WorldMapGetTilePlaneHeight(pActor->x, pActor->z);
    angle = g_WorldMapPlayerRotation;
    mode = g_WorldMapMode;
    pActor->dirZ = 0;
    pActor->dirY = 0;
    pActor->dirX = 0;
    pActor->moveSpeed = 8;
    *(volatile s16*)&pActor->angle = angle;
    pActor->drawAngle = pActor->angle;
    switch (mode) {
    case 1:
    case 2:
    case 3:
        if (g_GameState.gearRide[0] == 0) {
            *(volatile s32*)&pActor->x = g_WorldMapPlayerPosition;
            pActor->z = g_WorldMapPlayerPositionZ;
            pActor->y = WorldMapGetTilePlaneHeight(pActor->x, pActor->z);
            pActor->angle = g_WorldMapPlayerRotation;
            g_WorldMapScrollTargetX = *(WorldMapVec*)&pActor->x;
            g_WorldMapPlayerAngle = pActor->angle;
        } else {
            pActor->state = 1;
            pActor->hidden = 1;
        }
        break;
    case 4:
    case 5:
    case 6:
        pActor->state = 3;
        pActor->hidden = 1;
        if (g_GameState.partyMembers[0] != 0xFF) {
            g_GameState.gearRide[0] = 1;
        }
        break;
    case 7:
        pActor->state = 2;
        pActor->hidden = 1;
        if (g_GameState.partyMembers[0] != 0xFF) {
            g_GameState.gearRide[0] = 1;
        }
        break;
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
        break;
    }
    g_WorldMapTrailCursor = 0;
    for (i = 0, pTrail = g_WorldMapTrail; i < 0x20; i++, pTrail++) {
        pTrail->vec = *(WorldMapVec*)&pActor->x;
        pTrail->angle = pActor->angle;
    }
    g_GameState.worldMapPosX = pActor->x >> 12;
    g_GameState.worldMapPosZ = pActor->z >> 12;
    g_GameState.worldMapPlayerRotation = pActor->angle;
    return 1;
}
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc10", func_8008A52C);
/* 76.69%: only two insns are transposed (everything else is byte-identical).
   Retail emits sll v1,a0,0x7 at the top and defers the 0x40 stack argument's
   store (sw v0,0x14(sp)) to just before the jal; gcc puts the arg store first
   and then sinks the sll into the argument region, which needs v0 and is why
   the shift lands in v0 where retail has v1. Identical bytes for: the scaled
   index inline, as a named s32 local, pActor declared first or last, and the
   call written directly as the store's rhs.
s32 func_8008A52C(s32 arg0) {
    u8* pSprite;
    u8* pActor;

    pActor = g_WorldMapSlots + (arg0 << 7);
    pSprite = SpriteAllocFromAnimFile(g_WorldMapPartyActorAnimFile0, 0x100, 0x1E0, 0x140, 0x100, 0x40);
    *(u8**)(pActor + 0x4C) = pSprite;
    SpritePlayAnimation(pSprite, 0);
    SpriteSetScale(*(u8**)(pActor + 0x4C), 0x1800);
    *(u32*)(*(u8**)(pActor + 0x4C) + 0x3C) &= ~4;
    return 1;
}
*/
/* 90.48% (372 bytes, retail 372): identical instruction multiset, only the
   prologue rotates. Retail shifts arg0 into a2, keeps the (slots + shift) addu
   and the sw of the 0x40 stack argument before the jal and leaves li a2,0x1E0
   for the delay slot; gcc shifts into v1, emits li a2,0x1E0 early and lets
   reorg steal the addu. A 573-iteration permuter run found nothing (base 595,
   best 595). Same wall as func_8008B54C / func_8008BDD0 in misc11.
/*
s32 func_8008A5B8(s32 arg0) {
    WorldMapActor* pActor;
    WorldMapTrail* pTrail;
    void* pSprite;
    s32 i;

    pActor = &g_WorldMapSlots[arg0];
    pSprite = SpriteAllocFromAnimFile(g_WorldMapPartyActorAnimFile0, 0x100, 0x1E0, 0x140, 0x100, 0x40);
    pActor->pSprite = pSprite;
    SpritePlayAnimation(pActor->pSprite, 0);
    SpriteSetScale(pActor->pSprite, 0x1800);
    *(u32*)((u8*)pActor->pSprite + 0x3C) &= ~4;
    pActor->x = g_GameState.worldMapPosX << 12;
    pActor->z = g_GameState.worldMapPosZ << 12;
    pActor->y = WorldMapGetTilePlaneHeight(pActor->x, pActor->z);
    {
        u16 nAngle = g_GameState.worldMapPlayerRotation;

        pActor->hidden = 1;
        pActor->moveSpeed = 8;
        pActor->dirZ = 0;
        pActor->dirY = 0;
        pActor->dirX = 0;
        pActor->state = 2;
        g_WorldMapTrailCursor = 0;
        pActor->angle = nAngle;
    }
    pTrail = g_WorldMapTrail;
    for (i = 0; i < 0x20; i++, pTrail++) {
        pTrail->vec = *(WorldMapVec*)&pActor->x;
        pTrail->angle = pActor->angle;
    }
    g_WorldMapScrollTargetX = *(WorldMapVec*)&pActor->x;
    g_WorldMapPlayerAngle = pActor->angle;
    return 1;
}*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc10", func_8008A5B8);
