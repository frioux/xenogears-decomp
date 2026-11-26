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

#define WORLD_MAP_PRIM_TAG_LEN 9 // a POLY_FT4's packet length in words past its tag
// A POLY_FT4's primitive code byte.
#define WORLD_MAP_PRIM_CODE_POLY_FT4 0x2C
// Bit 1 of a primitive code byte: the primitive is drawn semi-transparent (same define
// as misc7.c and misc9.c).
#define WORLD_MAP_PRIM_CODE_SEMI_TRANS 0x2
// The tpage word of the sheet the trail prims are textured from, as GetTPage(1, 0, 0x340,
// 0x100) packs it; the sprite's two ABR bits are added as flags << 5.
#define WORLD_MAP_TRAIL_TPAGE 0x9D
// The VRAM page and clut the prim templates sample: an 8-bit page at (0x340, 0x100),
// the same sheet WORLD_MAP_TRAIL_TPAGE packs and WorldMapSetupPrimTemplates in misc4.c
// points its prims at.
#define WORLD_MAP_PRIM_TPAGE_X 0x340
#define WORLD_MAP_PRIM_TPAGE_Y 0x100
#define WORLD_MAP_PRIM_CLUT_X 0x100
#define WORLD_MAP_PRIM_CLUT_Y 0x1FF

#define WORLD_MAP_SPRITE_COUNT 512
#define WORLD_MAP_SPRITES_PER_GROUP 8
#define WORLD_MAP_TRAIL_ENTRY_COUNT 256
// g_WorldMapSpriteSetIds is a list of sprite set ids closed by this value, so the first
// id is tested against it before the walk.
#define WORLD_MAP_SPRITE_SET_ID_END (-1)
// g_WorldMapGroups is indexed by sprite set id; this is the scene layer the party
// actor's own slot drives - its position is copied there and it is hidden with the actor.
#define WORLD_MAP_PLAYER_SCENE_LAYER 0x45
// How far above the scene layer's position the "with height offset" placement puts the
// slot: 64 world units, in 12.12 fixed point.
#define WORLD_MAP_ACTOR_POS_HEIGHT_OFFSET 0x40000

// Bits of WorldMapSprite.flags, tested by WorldMapEmitSpriteTrailEntries.
#define WORLD_MAP_SPRITE_FLAG_ACTIVE 0x80 // the slot is in use; cleared once the life timer runs out
#define WORLD_MAP_SPRITE_FLAG_LIFE_TIMER 0x10 // the lifeTimer pair counts down and clears ACTIVE at zero
#define WORLD_MAP_SPRITE_FLAG_TRAIL_START_FLAT 0x20 // trailStartSpread gets no vertical component
#define WORLD_MAP_SPRITE_FLAG_TRAIL_END_FLAT 0x40 // trailEndSpread gets no vertical component
#define WORLD_MAP_SPRITE_FLAG_TRAIL_START_SPREAD_UNRANDOMIZED 0x4 // trailStartSpread is used as-is, not rand() % it
#define WORLD_MAP_SPRITE_FLAG_TRAIL_END_SPREAD_UNRANDOMIZED 0x8 // the same for trailEndSpread
// The two flag bits the trail prim's ABR field is filled from.
#define WORLD_MAP_SPRITE_ABR_MASK 0x3

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
    /* 0x40 */ s32 color;
    /* 0x44 */ s32 colorStep; // one signed 8-bit step per colour plane
    /* 0x48 */ u16 tpage; // ABR field taken from the sprite's flags, copied into the prim's tpage
    /* 0x4A */ undefined16 pad4A;
} WorldMapTrailEntry; // Size: 0x4C

typedef struct {
    /* 0x00 */ u32 lifeTimerInit; // what lifeTimer is armed with when the slot is claimed
    /* 0x04 */ u32 lifeTimer;
    /* 0x08 */ s16 maxCount;
    /* 0x0A */ s16 count;
    /* 0x0C */ s16 trailCount;
    /* 0x0E */ s16 trailActive;
    /* 0x10 */ u16 interval;
    /* 0x12 */ s16 timer;
    /* 0x14 */ s16 x;
    /* 0x16 */ s16 y;
    /* 0x18 */ s16 z;
    /* 0x1A */ undefined16 pad1A;
    /* 0x1C */ u16 rotX;
    /* 0x1E */ u16 rotY;
    /* 0x20 */ u16 rotZ;
    /* 0x22 */ undefined16 pad22;
    /* 0x24 */ SVECTOR trailStart; // local point a trail is emitted from; rotated by rot, added to x/y/z
    /* 0x2C */ SVECTOR trailEnd; // local point a trail heads for; (trailEnd - trailStart) is its direction
    /* 0x34 */ s32 speed;
    /* 0x38 */ s16 accelX;
    /* 0x3A */ s16 accelY;
    /* 0x3C */ s16 accelZ;
    /* 0x3E */ undefined16 unk3E;
    /* 0x40 */ u16 trailStartSpread; // random spread of trailStart; WORLD_MAP_SPRITE_FLAG_TRAIL_START_SPREAD_UNRANDOMIZED uses it unrandomized
    /* 0x42 */ u16 trailEndSpread; // random spread of trailEnd; WORLD_MAP_SPRITE_FLAG_TRAIL_END_SPREAD_UNRANDOMIZED uses it unrandomized
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

// 97.62% (336 bytes = retail, 84 insns = retail): the whole body is retail's; the ONE
//    remaining difference is the order of two preheader instructions - retail emits
//    'li s1,-1' then 'move s0,a0', ours the reverse - and the body is 84 insns either way.
//    THE EDIT THAT CLOSED THREE RESIDUALS (95.56 -> 97.62): the tail's layer pointer must
//    be a STEP OF THE GROUP POINTER ITSELF - `pGroup += 0x45;` then `pGroup->pos.x = ...`
//    - and not a second pointer local. With a second local (pLayer = &pGroup[0x45];
//    pLayer->pos.x) cc1 folds 0x16A4 into the store displacements (0x16AC/0x16B0/0x16B4),
//    leaves a nop where retail has 'addiu a1,a1,0x16A4', and the group base loses the
//    allocator contest to the actor (a1/a2 mirrored) - 95.56. The self-reassignment makes
//    cse's equivalence for that register circular, so the add is NOT folded, the layer
//    value reuses the group's own register, and retail's 'addiu a1,a1,0x16A4' / 'sw
//    v0,8(a1)' / 0xC / 0x10 appear verbatim. The same edit matched WorldMapCopyActorPosToSpriteSetLayer.
//    THE GUARD/WALK SPLIT IS STILL load-bearing (89.25 -> 95.56): the first id loop must
//    read its GUARD through the array and the WALK through a second, separately
//    initialised pointer, exactly as the matched neighbour WorldMapHideSpriteSetGroups
//    spells it. With the compact form (pIds = g_WorldMapSpriteSetIds; if (*pIds != END)
//    ... *pIds++ ...) cse coalesces the base and the walk into ONE pseudo, so the base
//    takes s0 and retail's duplicated 'sll v0,s2,0x7' on the loop backedge and the guard's
//    'move s0,a0' both disappear (89.25).
//    Measured dead for residual 2 this round, all at 84 insns or fewer: an `id` temp in
//    the first while (82 insns - it also re-registers the SECOND id loop), the first loop
//    as `while (*pWalk != END) { call(*pWalk); pWalk++; }` (byte-identical to the
//    do-while), a `s32 endId = -1` local for the sentinel (83 insns, 93.45 - retail has
//    the guard's own 'li v0,-1' separately, so the constant is spelled inline twice), the
//    walk pointer initialised above the guard (83 insns, 92.98), and swapping the pGroup
//    / pActor assignments (byte-identical at 95.56).
/*
s32 func_80088B40(s32 arg0) {
    WorldMapSceneLayer* pGroup;
    WorldMapPlayer* pActor;
    s16* pIds;
    s16* pWalk;
    s32 id;
    s32 result;

    if (g_WorldMapSpriteSetIds[0] != WORLD_MAP_SPRITE_SET_ID_END) {
        pWalk = g_WorldMapSpriteSetIds;
        do {
            WorldMapSetSpriteSetParent(0x45, *pWalk++);
        } while (*pWalk != WORLD_MAP_SPRITE_SET_ID_END);
    }
    pGroup = g_WorldMapGroups;
    pActor = &g_WorldMapSlots[arg0];
    pActor->state = 0;
    if (g_GameState.fieldScriptMemory[0] == 0x99) {
        pActor->x = 0x2000000;
        pActor->z = 0x4120000;
        pActor->y = 0;
        result = 1;
    } else {
        pGroup[0x45].hidden = 1;
        if (g_WorldMapSpriteSetIds[0] != WORLD_MAP_SPRITE_SET_ID_END) {
            pIds = g_WorldMapSpriteSetIds;
            do {
                id = *pIds++;
                pGroup[id].hidden = 1;
            } while (*pIds != WORLD_MAP_SPRITE_SET_ID_END);
        }
        result = 3;
    }
    pGroup += 0x45;
    pGroup->pos.x = pActor->x >> 12;
    pGroup->pos.y = pActor->y >> 12;
    pGroup->pos.z = pActor->z >> 12;
    return result;
}
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc10", func_80088B40);



s32 WorldMapHideSpriteSetGroups(void) {
    s16* pIds;
    s32 id;

    g_WorldMapGroups[WORLD_MAP_PLAYER_SCENE_LAYER].hidden = 1;
    // The guard and the loop test read the array separately, so the body loads
    // the id again; one `while ((id = *pIds) != -1)` merges them (lhu + sra, 16-byte frame).
    if (g_WorldMapSpriteSetIds[0] != WORLD_MAP_SPRITE_SET_ID_END) {
        pIds = g_WorldMapSpriteSetIds;
        do {
            id = *pIds++;
            g_WorldMapGroups[id].hidden = 1;
        } while (*pIds != WORLD_MAP_SPRITE_SET_ID_END);
    }
    return 3;
}

// 100.00%. The layer pointer MUST be a step of the group pointer itself (pGroup += 0x45),
// not a second local: with `pLayer = &pGroup[0x45];` cc1 folds 0x16A4 into the store
// displacements (0x16AC/0x16B0/0x16B4) and the schedule loses retail's
// 'addiu a1,a1,0x16A4'. The self-reassignment makes cse's equivalence for that register
// circular, so the add survives. The same constraint is parked under func_80088B40.
s32 WorldMapCopyActorPosToSpriteSetLayer(s32 arg0) {
    WorldMapPlayer* pActor;
    WorldMapSceneLayer* pGroup;

    pGroup = g_WorldMapGroups;
    pActor = &g_WorldMapSlots[arg0];
    if (g_GameState.fieldScriptMemory[0] == 0x99) {
        pGroup += WORLD_MAP_PLAYER_SCENE_LAYER;
        pGroup->pos.x = pActor->x >> 12;
        pGroup->pos.y = pActor->y >> 12;
        pGroup->pos.z = pActor->z >> 12;
    }
    return 1;
}


s32 WorldMapSetActorPosFromSpriteSetWithHeightOffset(s32 slotIndex) {
    WorldMapPlayer* pPlayer;
    WorldMapSceneLayer* pLayer;

    pPlayer = g_WorldMapSlots + slotIndex;
    pLayer = &g_WorldMapGroups[g_WorldMapLevelSpriteSetIdsHeightOffset[g_WorldMapLevelIndex]];
    pPlayer->x = pLayer->pos.x << 12;
    pPlayer->y = (pLayer->pos.y << 12) + WORLD_MAP_ACTOR_POS_HEIGHT_OFFSET;
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
        ((u8*)&pPrim->tag)[3] = WORLD_MAP_PRIM_TAG_LEN;
        pPrim->code = WORLD_MAP_PRIM_CODE_POLY_FT4;
        pPrim->tpage = GetTPage(1, 1, WORLD_MAP_PRIM_TPAGE_X, WORLD_MAP_PRIM_TPAGE_Y);
        pPrim->clut = GetClut(WORLD_MAP_PRIM_CLUT_X, WORLD_MAP_PRIM_CLUT_Y);
        pPrim->code |= WORLD_MAP_PRIM_CODE_SEMI_TRANS;
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
            if (pEntry->spriteIndex == groupIndex * WORLD_MAP_SPRITES_PER_GROUP + j && pEntry->trailFlags.status.active != 0) {
                pEntry->trailFlags.status.count = 0;
                break;
            }
        }
        pEntry++;
    }
}

/* 59.86% (444 bytes / 111 insns against retail's 456 / 114). Round 425 replaced the
   old 62.19% park with this body, rebuilt from m2c_draft plus the assembly. It is the
   only one of five measured whose STORE-GROUP ANCHOR is retail's: two induction
   pointers (t4 = pEntry at +0 for spriteIndex, a2 = pEntry+4 for everything else,
   exactly retail's a2 = &pEntry->trailFlags). That anchor is produced by writing the
   count store through the SAME pEntry base LAST - do not re-add a separate flags
   pointer, which anchors the giv at 0x20 (lever g-giv-anchor-follows-source-order).
   The 3 missing instructions are exactly (i) retail's separate s16 conversion
   (sll v0,a1,0x10 + sra a1,v0,0x10, tested with blez; this body compares the sll
   result directly) and (ii) the CALLEE-SAVED LOOP COUNTER: retail keeps i in s0
   (sp,-0x10 / sw s0,8(sp) / li s0,0xFF / bne s0,v0), ours sinks to t8 with an 8-byte
   frame. The counter only goes callee-saved when enough values are live across the
   arm at once, and with plain `pEntry->x = x + velX;` gcc SINKS each of the six
   pos/vel loads to its store instead of emitting retail's six-load group at the top
   of the arm; assigning x/y/z/velX/velY/velZ as temps at the arm head does not stop
   it (measured identical 59.86 / 111). Next lead: a scheduling barrier that keeps
   those six loads at the arm head, which is what should push the counter into s0 and
   restore the 2 frame instructions.
   Load-bearing spellings found earlier and kept here: WorldMapTrailEntry.color must
   be s32 (retail shifts it with sra) and colorStep is a packed s32 STEP WORD, not a
   struct of 4 s8s - retail loads it once and shift-extracts g and b while r is a
   plain lb, so it is spelled
     r = (color & 0xFF) + (s8)pEntry->colorStep;
     g = ((color >> 8) & 0xFF) + (s8)(pEntry->colorStep >> 8);
     b = ((color >> 16) & 0xFF) + (s8)(pEntry->colorStep >> 16);
   (three lb fields scored 49 and was 436 bytes). The parked neighbours that touch this
   entry, WorldMapEmitSpriteTrailEntries and func_80089C78, still spell colorStep as a
   struct (*(u32*)&pEntry->colorStep, pEntry->colorStep.a); they need the s32 form when
   they are unparked. Rejected this round: temps plus a second `pFlags` induction
   pointer with the count store in the middle reads 64.77 at 472 bytes / 118 insns
   (anchor 0x20, saves s0 AND s1, 4 instructions long - the higher fuzzy score is the
   extra instructions, not progress), and the same with the count store last reads
   58.55 at retail's exact 456 / 114 with the anchor still at 0x20. Presets: this body
   has not been screened against 2.6.0 / cdk0.
/*
void func_80089580(void) {
    WorldMapTrailEntry* pEntry;
    WorldMapSprite* pSprite;
    s32 i;
    s32 x;
    s32 y;
    s32 z;
    s32 velX;
    s32 velY;
    s32 velZ;
    s32 color;
    s32 step;
    s32 r;
    s32 g;
    s32 b;

    pEntry = g_WorldMapTrailEntries;
    for (i = WORLD_MAP_TRAIL_ENTRY_COUNT - 1; i != -1; i--) {
        if ((pEntry->trailFlags.flags >> 16) != 0) {
            if ((s16)pEntry->trailFlags.flags > 0) {
                x = pEntry->x;
                y = pEntry->y;
                z = pEntry->z;
                velX = pEntry->velX;
                velY = pEntry->velY;
                velZ = pEntry->velZ;
                color = pEntry->color;
                step = pEntry->colorStep;
                r = (color & 0xFF) + (s8)step;
                g = ((color >> 8) & 0xFF) + (s8)(step >> 8);
                b = ((color >> 16) & 0xFF) + (s8)(step >> 16);
                pEntry->scaleX += pEntry->scaleStepX;
                pEntry->scaleY += pEntry->scaleStepY;
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
                pEntry->color = (color & 0xFF000000) | (b << 16) | (g << 8) | r;
                pEntry->x = x + velX;
                pEntry->y = y + velY;
                pEntry->z = z + velZ;
                pEntry->velX = velX + pEntry->accelX;
                pEntry->velY = velY + pEntry->accelY;
                pEntry->velZ = velZ + pEntry->accelZ;
                pEntry->trailFlags.status.count = pEntry->trailFlags.status.count - 1;
            } else {
                pSprite = &((WorldMapSprite*)g_WorldMapSpriteGroups)[pEntry->spriteIndex];
                pSprite->count--;
                pEntry->spriteIndex = 0;
                pEntry->trailFlags.flags = 0;
            }
        }
        pEntry++;
    }
}*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc10", func_80089580);


/* 99.89% (1328 bytes / 332 insns = retail, all instructions identical). The only
   residual is the scratchpad address form: retail's expected object spells the four
   `addiu reg,s3,%lo(D_1F8000xx)` (0xF0, 0x20, 0x10) with a relocation because splat
   named them, this body folds the same offsets into the immediate (+240/+32/+16) over
   s3 = 0x1F800000, which links to the same bytes. Declaring the symbols instead is
   wrong: misc14/misc7 measured that a named D_1F8000xx makes gcc form a fresh
   lui+addiu. Verified with check_bin.sh world_map.
   Three constructs are load-bearing and were found by measurement:
   (1) `low = pSprite->lifeTimer; high = low >> 16; low = (s16)low;` - ONE variable for
   the packed pair, which is why retail loads it straight into s5 and keeps the sprite
   base in s6.
   (2) the flags clear is reached by `goto clearSpriteFlags` to a label AFTER the scan
   loop (its block is placed just before the store, so retail's scan exit has to `j`
   over it and reorg duplicates the store's `sll` into that jump's delay slot). Writing
   `pSprite->flags ^= 0x80; goto storeLifeTimer;` in the arm instead is 4 bytes short
   (331 insns) at 97.08%.
   (3) store pEntry->scaleStep BEFORE pEntry->tpage (permuter find from an earlier
   round): it puts the strength-reduction base at retail's 0x48.
   The uninitialised low/high read on the not-flags-0x10 path is RETAIL OWN: the
   lifeTimer load sits inside the flags-0x10 block and the pack store at the loop tail
   is shared, so those registers really are stale; zeroing them costs two instructions.
*/
void WorldMapEmitSpriteTrailEntries(void) {
    WorldMapSprite* pSprite;
    WorldMapTrailEntry* pEntry;
    u8* pScratch;
    s32 i;
    s32 flags;
    s32 low;
    s32 high;
    s32 spread;

    pScratch = (u8*)PSX_SCRATCH;
    pSprite = g_WorldMapSpriteGroups->sprites;
    for (i = 0; i < WORLD_MAP_SPRITE_COUNT; i++, pSprite++) {
        flags = pSprite->flags;
        if (!(flags & WORLD_MAP_SPRITE_FLAG_ACTIVE)) {
            continue;
        }
        // low/high live across iterations: the original only reloads them under WORLD_MAP_SPRITE_FLAG_LIFE_TIMER
        // and stores them back at the tail of every active sprite.
        if (flags & WORLD_MAP_SPRITE_FLAG_LIFE_TIMER) {
            low = pSprite->lifeTimer;
            high = low >> 16;
            low = (s16)low;
            if (low != 0) {
                low--;
                goto storeLifeTimer;
            }
            if (high != 0) {
                high--;
            } else {
                goto clearSpriteFlags;
            }
        }
        if (pSprite->timer != 0) {
            pSprite->timer = pSprite->timer - 1;
            goto storeLifeTimer;
        }
        pSprite->timer = pSprite->interval;
        if ((pSprite->trailActive != 0) & (pSprite->maxCount > 0) & (pSprite->count < pSprite->maxCount)) {
        pEntry = g_WorldMapTrailEntries;
        for (spread = WORLD_MAP_TRAIL_ENTRY_COUNT - 1; spread != -1; spread--) {
            if (pEntry->trailFlags.status.active == 0) {
                pEntry->spriteIndex = i;
                pEntry->trailFlags.flags = *(u32*)&pSprite->trailCount;
                RotMatrixYXZ((SVECTOR*)&pSprite->rotX, (MATRIX*)(pScratch + 0xF0));
                ApplyMatrix((MATRIX*)(pScratch + 0xF0), (SVECTOR*)&pSprite->trailStart, (VECTOR*)(pScratch + 0x20));
                if (!(flags & WORLD_MAP_SPRITE_FLAG_TRAIL_START_FLAT)) {
                    *(s32*)(pScratch + 0x014) = (rand() & 0xFFF) - 0x800;
                } else {
                    *(s32*)(pScratch + 0x014) = 0;
                }
                *(s32*)(pScratch + 0x010) = (rand() & 0xFFF) - 0x800;
                *(s32*)(pScratch + 0x018) = (rand() & 0xFFF) - 0x800;
                VectorNormal((VECTOR*)(pScratch + 0x10), (VECTOR*)(pScratch + 0x00));
                if (flags & WORLD_MAP_SPRITE_FLAG_TRAIL_START_SPREAD_UNRANDOMIZED) {
                    spread = pSprite->trailStartSpread;
                } else {
                    spread = rand() % pSprite->trailStartSpread;
                }
                pEntry->x = (*(s32*)(pScratch + 0x020) << 12) + *(s32*)(pScratch + 0x000) * spread + (pSprite->x << 12);
                pEntry->y = (*(s32*)(pScratch + 0x024) << 12) + *(s32*)(pScratch + 0x004) * spread + (pSprite->y << 12);
                pEntry->z = (*(s32*)(pScratch + 0x028) << 12) + *(s32*)(pScratch + 0x008) * spread + (pSprite->z << 12);
                ApplyMatrix((MATRIX*)(pScratch + 0xF0), (SVECTOR*)&pSprite->trailEnd, (VECTOR*)(pScratch + 0x20));
                if (!(flags & WORLD_MAP_SPRITE_FLAG_TRAIL_END_FLAT)) {
                    *(s32*)(pScratch + 0x014) = (rand() & 0xFFF) - 0x800;
                } else {
                    *(s32*)(pScratch + 0x014) = 0;
                }
                *(s32*)(pScratch + 0x010) = (rand() & 0xFFF) - 0x800;
                *(s32*)(pScratch + 0x018) = (rand() & 0xFFF) - 0x800;
                VectorNormal((VECTOR*)(pScratch + 0x10), (VECTOR*)(pScratch + 0x00));
                if (flags & WORLD_MAP_SPRITE_FLAG_TRAIL_END_SPREAD_UNRANDOMIZED) {
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
                flags &= WORLD_MAP_SPRITE_ABR_MASK;
                pEntry->accelX = pSprite->accelX;
                pEntry->accelY = pSprite->accelY;
                pEntry->accelZ = pSprite->accelZ;
                pEntry->color = *(u32*)&pSprite->color[0];
                pEntry->colorStep = pSprite->colorStep;
                *(u32*)&pEntry->scaleX = pSprite->scale;
                *(u32*)&pEntry->scaleStepX = pSprite->scaleStep;
                pEntry->tpage = (flags << 5) | WORLD_MAP_TRAIL_TPAGE;
                pSprite->count++;
                break;
            }
            pEntry++;
        }
        }
        goto storeLifeTimer;
    clearSpriteFlags:
        pSprite->flags ^= WORLD_MAP_SPRITE_FLAG_ACTIVE;
    storeLifeTimer:
        pSprite->lifeTimer = (high << 16) | low;
    }
    func_80089580();
}


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
        if ((pEntry->colorStep >> 24) & 1) {
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
}*/
/* 95.46% (612 bytes, retail 612): every instruction and every branch matches - the
//    switch, its cross-jumped case 4-6 / case 7 body, the 32-entry trail copy and the
//    0x1820 tail - and the ONLY differing lines outside the prologue are the label
//    spellings (retail names jtbl_800704E0, this unit owns its table, so diff_func
//    prints .rodata; the .L branch targets are named in retail and function+offset
//    here). So a prologue fix converts this function outright.
//    RESIDUAL - the shared head of this unit's three slot init functions
//    (func_8008A5B8, func_8008A52C, this one): retail emits
//    `addiu a1,0x100 / sll a2,a0,0x7 / li v0,0x100 / sw v0,0x10(sp) / li v0,0x40 /
//    lui a0(anim) / lui v1(g_WorldMapSlots) / li a3,0x140 / sw ra / sw s0 /
//    sw v0,0x14(sp) / addu s0,v1,a2 / jal / addiu a2,zero,0x1E0`, i.e. the index
//    shift sits in the THIRD-ARGUMENT register at insn 3 and the base in a temp v1,
//    which pushes `li a2,0x1E0` into the jal delay slot. Our cc1 shifts into v1
//    (insn 6), loads the base into v0 and leaves the addu in the slot. Causality runs
//    from the register choice: sll and li a2 are both a2 in retail, so the constant
//    cannot be scheduled before the addu. Everything from `move a0,v0` on is
//    byte-identical.
//    The shift position is source-independent: misc11_b WorldMapInitPartyActorSprite2
//    (matched, same 6-line head, XenoWorldMap) compiles to `sll v1` at insn 3 with
//    the base loaded straight into s0 and the addu in the slot, and this unit's
//    func_8008A52C retail is the same shape - so retail's a2 here is a per-function
//    allocator outcome, not a different spelling.
//    Measured identical at 95.46 this round (round 452 line): the park's
//    (u8*)g_WorldMapSlots + index * 0x80, "(WorldMapActor*)g_WorldMapSlots + index",
//    a separate s32 offset local, and &g_WorldMapSlots[index]. Older measurements:
//    cdk -O2 71.79, gcc 2.7.2 -O3 95.46, a 4-minute permuter run base 535 best 535.
//    Two VOLATILE casts below are load-bearing, not debris:
//    (1) the s16 store of `angle` is what makes gcc re-read it with `lh` for
//    drawAngle (retail: sh 0x48, lh 0x48, sw 0x5C); without it gcc folds the re-read
//    into sll/sra and the function comes out 4 bytes short.
//    (2) the s32 store of x in the case 1-3 body stops gcc sinking that store below
//    the g_WorldMapPlayerPositionZ load, giving retail's lw/nop/sw order.
//    The head reads g_GameState.fieldScriptMemory[0]/[1] (0x1930/0x1932), not
//    worldMapPosX/PosZ, while the tail writes 0x1820/0x1822.
//    Round 468 re-measured 95.46% / 612 bytes / 153 insns: the jtbl/.L label
//    spellings plus the same 12-line head, nothing else. Compiling this unit's own
//    .i with the unit's cc1 plus -fschedule-insns (a diagnostic, not a preset flag)
//    gives the identical 95.46 and the same `sll $3` / base `$2` in cc1's own
//    output, so sched1 is not what decides the head's registers; register
//    assignment runs before sched2, so the store placement is a consequence.
//    Round 481 re-measured 95.46% / 612 bytes and closed the remaining question
//    about the head: it is BODY-DEPENDENT, not a fixed per-function tie. Cutting
//    this body down to the alloc call alone moves the base to `s0` (coalesced into
//    the sum) and the shift to `v0`, while the full body gives base `v0` / shift
//    `v1`; retail wants base `v1` / shift `a2`. The mechanism is visible in the
//    same probe: retail's base is `v1` only because `v0` still holds the `li v0,64`
//    of the sixth stack argument when the base is loaded (retail stores it at insn
//    14, we store it at insn 9), and that store's position is a sched2 consequence
//    of the register set, so cause and effect cannot be separated from source.
//    The body compiles instruction-for-instruction identical, so there is nothing
//    in it to move. See func_8008A5B8 below for the spellings measured this round.
// */
/*
s32 func_8008A2C8(s32 index) {
    WorldMapActor* pActor;
    WorldMapTrail* pTrail;
    s32 i;
    s32 mode;
    s32 angle;

    pActor = (WorldMapActor*)g_WorldMapSlots + index;
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
}*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc10", func_8008A2C8);



/* 76.69% (140 bytes, retail 140): instruction multiset identical, 35 vs 35 insns,
   and the tail from the first jal delay slot on is byte-identical. The residual is
   lever h-stack-arg-store-sink-is-a-war-tie (round 402): retail gives the index
   shift `sll v1,a0,0x7` at insn 3, before the 0x10 push, so the 6th argument store
   `sw v0,0x14(sp)` sinks to just before the jal; our cc1 gives the shift $v0, which
   is a WAR on the `li v0,0x40` feeding that store, so sched2 emits the store at
   insn 6 and the shift right after it, and the jal slot adds through v0 instead of
   v1. The same 6 lines as misc11 func_8008B54C / func_8008BDD0, where a 4-minute
   permuter run found only a call swap (wrong code) and try_flags --sweep topped
   out, so this is parked instead of re-spelled. Measured identical 76.69:
   "g_WorldMapSlots + arg0", "&g_WorldMapSlots[arg0]", and the typed twin spelling
   of the flags clear through a 0x3C flags field instead of the u32 cast.
   Presets: gcc 2.6.0 76.09, 2.7.2-cdk 76.69 - not a preset question.
   Round 425 re-measured this exact body: still 76.69% at retail's 140 bytes / 35
   insns, 6 differing lines, so the diagnosis above stands unchanged.
/*
s32 func_8008A52C(s32 arg0) {
    WorldMapPlayer* pActor = &g_WorldMapSlots[arg0];

    pActor->pSprite = SpriteAllocFromAnimFile(g_WorldMapPartyActorAnimFile0, 0x100, 0x1E0, 0x140, 0x100, 0x40);
    SpritePlayAnimation(pActor->pSprite, 0);
    SpriteSetScale(pActor->pSprite, 0x1800);
    *(u32*)((u8*)pActor->pSprite + 0x3C) &= ~4;
    return 1;
}*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc10", func_8008A52C);


/* 92.63% (372 bytes, retail 372): 93 vs 93 insns and, after the loop-preheader
//    fix, the WHOLE body matches - the only differing lines are the prologue.
//    FIX FOUND THIS ROUND: the trail loop init has to be ONE comma statement
//    (`for (i = 0, pTrail = g_WorldMapTrail; ...)`), not `pTrail = g_WorldMapTrail;`
//    followed by the for: retail emits `move a3,zero` (i = 0) BEFORE the lui/addiu of
//    g_WorldMapTrail, and the two-statement form put them the other way round
//    (90.48 -> 92.63). Also un-parked a `void* pSprite` temp (retail stores the call
//    result straight into 0x4C) and the initialiser/pointer-cast/offset-local forms of
//    the slot address, all measured identical at 92.63.
//    RESIDUAL: retail puts the index shift in the THIRD-ARGUMENT register
//    (`sll a2,a0,0x7` at insn 3), loads the base into a temp `v1` and does
//    `addu s0,v1,a2` before the jal, so `addiu a2,zero,0x1E0` can fill the delay
//    slot; our cc1 shifts into `v1` at insn 6, loads the base into `v0` and leaves the
//    addu in the slot. Causality runs from the register choice: the sll and the li a2
//    are both a2 in retail, so li a2 cannot be scheduled before the addu. The same head
//    compiles to retail's own shape in misc11_b WorldMapInitPartyActorSprite2
//    (matched: `sll v1` at insn 3, base into s0, addu in the slot) and in
//    func_8008A52C next door (retail `sll v1`), so this is a per-function allocator
//    contest, not a spelling: index shift address-formation, a separate `s32`
//    offset local, an initialiser, and the loop merge were all measured.
// */
/* 92.63% (372 bytes, retail 372): 93 vs 93 insns. The body is exact - the ONLY
//    differing lines are the prologue.
//    THIS ROUND (measured together, 90.48 -> 92.63): (a) the trail loop init is ONE
//    comma statement, `for (i = 0, pTrail = g_WorldMapTrail; i < 0x20; i++, pTrail++)`,
//    where the park had `pTrail = g_WorldMapTrail;` followed by the `for`: retail emits
//    `move a3,zero` (i = 0) BEFORE the lui/addiu of g_WorldMapTrail and the
//    two-statement form put them the other way round (that hunk of the diff is gone);
//    (b) the park's `void* pSprite` temp is dropped - retail stores the call result
//    straight into 0x4C.
//    RESIDUAL (see func_8008A2C8 below for the full note - same head): retail puts the
//    index shift in the THIRD-ARGUMENT register (`sll a2,a0,0x7` at insn 3), loads the
//    base into a temp `v1` and does `addu s0,v1,a2` before the jal so that
//    `addiu a2,zero,0x1E0` fills the delay slot; our cc1 shifts into `v1` at insn 6,
//    loads the base into `v0` and leaves the addu in the slot. The sll and the li a2 are
//    both a2 in retail, so the constant cannot be scheduled before the addu - the
//    register choice is the cause, not a consequence. Measured identical at 92.63: an
//    initialiser form of the slot pointer, "(WorldMapPlayer*)((u8*)g_WorldMapSlots +
//    slotOffset)" with a named offset local, and &g_WorldMapSlots[arg0]. The same head
//    compiles to retail's own shape in misc11_b WorldMapInitPartyActorSprite2 (matched,
//    `sll v1` at insn 3, base straight into s0, addu in the slot) and in this unit's
//    func_8008A52C retail, so a2 is a per-function allocator outcome.
//    Round 468 re-measured 92.63% / 372 bytes / 93 insns / 12 differing lines and
//    closed two more spellings, both identical at 92.63: plain
//    `pActor = &g_WorldMapSlots[arg0];` (no offset local) and the anim file in a
//    named `void* pAnimFile` local before the pointer (pseudo numbering does not
//    move the contest). The park keeps the &g_WorldMapSlots[arg0] form.
//    Round 481: four more scored runs, all 92.63% / 372 bytes / 93 insns.
//    The head is BODY-DEPENDENT (probe: truncating this body to the alloc call and
//    `return 1` gives shift `v0` and base `s0` coalesced into the sum, the full
//    body gives shift `v1` / base `v0`, retail wants shift `a2` / base `v1`), so it
//    is not a standalone top-block tie - but this body compiles
//    instruction-for-instruction identical to retail from `move a0,v0` on, so it
//    holds no handle. Retail's base is `v1` only because `v0` still holds the sixth
//    stack argument's `li v0,64` when the base is loaded (retail stores it at insn
//    14, we at insn 9), and which of the two is cause cannot be told apart from
//    the source: the store's position is a sched2 consequence of the register set.
//    Closed this round, all identical at 92.63: `pActor = g_WorldMapSlots + arg0;`
//    (pointer arithmetic, no subscript) with the anim file inline in the call, the
//    `void* pAnimFile` local placed AFTER the pointer instead of before, and a named
//    `s32 size` local holding the sixth argument's 0x40 (copy-propagated back to the
//    literal, so `li v0,64` still dies at insn 9 and the base still takes `v0`) -
//    which also kills the theory that retail's base is `v1` because `v0` is busy.
// */
/*
s32 func_8008A5B8(s32 arg0) {
    WorldMapPlayer* pActor;
    WorldMapTrail* pTrail;
    s32 i;

    pActor = &g_WorldMapSlots[arg0];
    pActor->pSprite = SpriteAllocFromAnimFile(g_WorldMapPartyActorAnimFile0, 0x100, 0x1E0, 0x140, 0x100, 0x40);
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
    for (i = 0, pTrail = g_WorldMapTrail; i < 0x20; i++, pTrail++) {
        pTrail->vec = *(WorldMapVec*)&pActor->x;
        pTrail->angle = pActor->angle;
    }
    g_WorldMapScrollTargetX = *(WorldMapVec*)&pActor->x;
    g_WorldMapPlayerAngle = pActor->angle;
    return 1;
}*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc10", func_8008A5B8);



