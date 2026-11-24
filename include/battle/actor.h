#ifndef _XENO_BATTLE_ACTOR_H
#define _XENO_BATTLE_ACTOR_H

// mp and maxMp are percents times this, as hp and maxHp are percents times BATTLE_ACTOR_HP_SCALE.
#define BATTLE_MP_SCALE 10
// A row's action power is a rate in units of 1/BATTLE_ACTION_POWER_FULL of full power.
#define BATTLE_ACTION_POWER_FULL 20
// Clamp for a slot's agility once equipment bonuses have been added; the stat's cap.
#define BATTLE_AGILITY_MAX 16
// accessoryEffectFlags5 bit that doubles every condition timer the slot starts.
#define BATTLE_ACCESSORY_TIMER_DOUBLE_FLAG 0x40

#include "psyq/libgte.h"
#include "field/actor.h"
#include "main/game.h"
#include "libgear/main.h"

// Shared by the battle units that read the structures below.
// slotStates and slotAmounts hold one entry per battle slot.
#define BATTLE_SLOT_COUNT 12
// The per-slot command codes: the slot's text index maps to the first of its four.
#define BATTLE_SLOT_COMMAND_COUNT 4
// rand() % BATTLE_PERCENT_ROLL_RANGE is the 0..99 percent a chance is rolled against.
#define BATTLE_PERCENT_ROLL_RANGE 100
// The actor slots the u16 slot mask walks one bit apiece (BattleCollectMaskedSlotActors;
// the per-slot byte tables in misc12/misc32 hold this many entries).
#define BATTLE_ACTOR_SLOT_COUNT 11
// BattleTestSlotBit range: the 49 slot bits BattleSetFlag10000IfAllSlotBitsSet walks
#define BATTLE_SLOT_BIT_COUNT 49
// byte stride of one slot record in the D_800C3EB7/8 tables
#define BATTLE_ACTOR_SLOT_STRIDE 0x1C
// activeSlotMask holds one bit per actor slot, low bit first: the low MAX_PARTY_MEMBERS bits are
// the party slots and the bits above them up to bit 10 are the other actors. A row runs for one
// group only, so BattleFilterSlotMaskByGroup narrows the mask within whichever group it touches.
#define BATTLE_PARTY_SLOT_MASK 0x7
#define BATTLE_OTHER_SLOT_MASK 0x7F8
// The condition timer BattleApplyScriptConditionTimer arms instead of the row's power when the
// row's actionFlags has 0x4000, and the one BattleApplyActorConditionTimer always arms.
#define BATTLE_CONDITION_TIMER_FIXED_COUNT 5
// The element bits of an element byte (the row's, the work block's copy of it, and the
// slot's): bit 0x40 above them raises the affinity multiplier from 15 to 18.
#define BATTLE_ELEMENT_MASK 0x3F
// The slotStates byte that means no state at all: what BattleClearAllSlotStates fills.
#define BATTLE_SLOT_STATE_CLEARED 0xFF
// A digit table entry that leaves its position blank.
#define BATTLE_VALUE_DIGIT_NONE 0xFF
// g_BattleActorCharIds holds this in a slot no character occupies.
#define BATTLE_SLOT_CHARACTER_NONE 0x7F
// The hp stat's cap: BattleSetSlotConditionActionValue clamps maxHp to it.
#define BATTLE_HP_MAX 9999
// state.flags bit that selects the gear-side status word over the actor's.
#define BATTLE_SLOT_GEAR_STATUS_FLAG 0x80
// hp and maxHp are percents; state.hp/state.maxHp hold them times this.
#define BATTLE_ACTOR_HP_SCALE 50
// BattleSlotState.hp and .maxHp are clamped to it once the scaled stat overflows.
#define BATTLE_SCALED_HP_MAX 99999
// A computed action amount (BattleComputeActionAmount*) is clamped to it before it is stored.
#define BATTLE_ACTION_AMOUNT_MAX 9999
// Bit of the BattleGetActorActionConditionMask result set while hp is under an eighth of maxHp.
#define BATTLE_ACTION_CONDITION_LOW_HP 0x2
// hp and maxHp are percents, so this is a stat at full; BattleResetActorStats
// starts both there.
#define BATTLE_STAT_PERCENT_MAX 100
// conditionFlags as BattleResetActorStats sets it: the conditions a slot starts
// with (bits 6 and 13-15 clear).
#define BATTLE_CONDITION_FLAGS_RESET 0x1FBF
// The one effect bit BattleSetSlotEntryFieldsTo1/To2000 act on, in whichever word a
// slot carries it: the gear side's activeEffectFlags2 (state.flags bit 0x80 set) or
// the actor's activeEffectFlags4.
#define BATTLE_GEAR_EFFECT_FLAG 0x2000
#define BATTLE_ACTOR_EFFECT_FLAG 0x400

// A queued movement/rotation command, allocated by BattleAllocBlock and linked
// into the owning sprite's p70 slot.
typedef struct {
    /* 0x0 */ u8 inUse; // BattleAllocBlock hands out blocks with this clear; BattleFreeBlock clears it
    /* 0x1 */ u8 flags;
    /* 0x2 */ u8 opcode; // the command code plus 3
    /* 0x3 */ u8 tag; // 0xFF = no tag; BattleFreeSpriteBlocksByTag matches it
    /* 0x4 */ s16 startX; // the component's value when the command was queued
    /* 0x6 */ s16 startY;
    /* 0x8 */ s16 startZ;
    /* 0xA */ s16 targetX; // where the tween ends, or the per-frame step when the absolute bit is clear
    /* 0xC */ s16 targetY;
    /* 0xE */ s16 targetZ;
    /* 0x10 */ s16 curFrame; // counted up to endFrame by the interpolation
    /* 0x12 */ s16 endFrame; // frame the interpolation runs to
} BattleBlock; // Size: 0x14

// One part of an actor's model: its own transform plus the command queue, and
// the element type of a battle actor's sprite array (stride 0x7C). Element 0 is
// the root, which owns the skeleton and the per-part queue heads; the rest link
// back to it through pParent.
typedef struct {
    /* 0x00 */ void* pParent;
    /* 0x04 */ u8 doTransform;
    /* 0x05 */ u8 doRotate;
    /* 0x06 */ u8 rotDirection;
    /* 0x07 */ u8 isVisible; // the draw loop skips a part with this clear; the root and a freed part are 0
    /* 0x08 */ u16 jointIndex;
    /* 0x0A */ u16 count; // number of elements in the array, this one included
    /* 0x0C */ MATRIX localTransform;
    /* 0x2C */ MATRIX globalTransform;
    /* 0x4C */ SVECTOR scale;
    /* 0x54 */ s16 rot[3];
    /* 0x5A */ u16 pad5A;
    /* 0x5C */ s32 pos[3];
    /* 0x68 */ void* pModelPacketBuffer; // what ModelAllocatePackets hands back
    /* 0x6C */ void* pCurModelPacket; // the packet being built, initialised from the buffer
    /* 0x70 */ BattleBlock* p70;
    /* 0x74 */ BattleBlock* p74;
    /* 0x78 */ BattleBlock* p78;
} BattleSprite; // Size: 0x7C

// A 16.16 fixed-point vector as battle code reads it: the integer part of each
// component is the high half of the word, so reading one is a halfword load at
// +2/+6/+A.
typedef struct {
    /* 0x0 */ s16 xFrac;
    /* 0x2 */ s16 xWhole;
    /* 0x4 */ s16 yFrac;
    /* 0x6 */ s16 yWhole;
    /* 0x8 */ s16 zFrac;
    /* 0xA */ s16 zWhole;
} BattleFixed16Vector; // Size: 0xC

// Battle-side view of a battle actor's sprite (SpriteData in
// include/field/actor.h). Battle code reads the integer half of the 16.16
// position and does a whole-word read-modify-write on the 0x3C flag word, which
// the shared header's IVEC3 and bitfield spelling cannot express; that spelling
// is left alone because other units' matches depend on it. Only the words
// src/battle/main's misc28, misc30 and misc31 touch are named.
typedef struct {
    /* 0x00 */ BattleFixed16Vector position;
    /* 0x0C */ IVEC3 step;
    /* 0x18 */ s32 moveSpeed;
    /* 0x1C */ u32 gravity;
    /* 0x20 */ SpriteDataI1* pBase;
    /* 0x24 */ void* pVramData; // the SpriteAnimPackage whose pFrames the animation test reads
    /* 0x28 */ u8 unk28[0x6];
    /* 0x2E */ s16 otDepth; // the sprite's projected depth, 0 when the projection clips
    /* 0x30 */ s16 otDepthOffset; // added to that depth
    /* 0x32 */ s16 direction;
    /* 0x34 */ u8 unk34[0x2];
    /* 0x36 */ u16 height; // BattleGetActorHeight for this sprite
    /* 0x38 */ u16 halfHeight; // height >> 1
    /* 0x3A */ u16 unk3A;
    /* 0x3C */ u32 flags;
    /* 0x40 */ u8 unk40[0x60];
    /* 0xA0 */ SVEC targetPosition;
    /* 0xA6 */ u8 unkA6[0x2];
} BattleSpriteState; // Size: 0xA8

// A battle participant. g_BattleActors[] holds one pointer per slot. Only the
// fields read by src/battle/main/misc24 and misc25 are named; other battle
// units reach past 0x186, so 0x1A0 is a lower bound on the real size. The gear
// side's Model (include/libgear/main.h) is the same layout and names the
// height/attach/position fields below the same way.
typedef struct {
    /* 0x000 */ void* pMesh; // the actor's mesh; BattleFreeModelMesh frees it
    /* 0x004 */ BattleSprite* pSprites;
    /* 0x008 */ s32 pActionTable; // address of the s32 action-value table a queued command id indexes
    /* 0x00C */ s32 pBuffer; // the block BattleFreeActorBuffer HeapFree()s; its +4 holds a pointer into the action-value table
    /* 0x010 */ s32 actionValue; // the entry the queued command selects from that table
    /* 0x014 */ s32 pActionTableLow; // the s32 table BattleGetActorActionEntry reads for action codes below 0x40 (indexed code + 1)
    /* 0x018 */ s32 pActionTableHigh; // and from 0x40 up (indexed code - 0x3F)
    /* 0x01C */ s16 scale;
    /* 0x01E */ s16 groundTriangleIndex; // the ground search's last hit, handed back to it as the next hint
    /* 0x020 */ u8 actorSlot; // this actor's own g_BattleActors slot: target ids 0xFD/0xF9 resolve to it and the slot tables are indexed with it
    /* 0x021 */ u8 linkedActorSlot; // the g_BattleActors slot whose sprites this actor shares: command 0x16 hides the shared parts and forwards its command there
    /* 0x022 */ u8 unk22;
    /* 0x023 */ u8 isModelLoading; // set once the model file command has read the archive, cleared when ArchiveDataSync reports the load finished
    /* 0x024 */ s16 height;
    /* 0x026 */ s16 width;
    /* 0x028 */ s16 depth;
    /* 0x02A */ u8 commands[0x1]; // the command being run: low 7 bits index the action entry tables
    /* 0x02B */ u8 commandCount;
    /* 0x02C */ u8 unk2C[0x2];
    /* 0x02E */ u8 commandArgs[0x6]; // the argument queued alongside commands[n]
    /* 0x034 */ u8 doUpdate; // gates the per-frame animation update; inherited from the parent actor
    /* 0x035 */ u8 unk35;
    /* 0x036 */ u8 skipGroundSnap; // non-zero: BattleSnapActorToGround leaves the sprite's y alone
    /* 0x037 */ u8 useScaledMatrices; // non-zero: the step runs BattleUpdateScaledSpriteTransforms, which folds each part's scale into its parent matrix (libgear's Model.useScaledMatrices)
    /* 0x038 */ u8 isBusy; // keeps BattleWaitAndFreeActorBuffers and BattleWaitForOtherActorAnimations waiting while it is set
    /* 0x039 */ u8 depthShade; // set from the OT depth every frame (BattleDrawActor), and func_800AE2A4 scales it into the actor's sound volume
    /* 0x03A */ s16 savedActorMask; // command 0x6F stores g_BattleCommandActorMask here (-1 when its operand is 0); command 0x70 branches only while it still matches
    /* 0x03C */ u16 blockTag; // func_800A0838 compares it with each queued block's tag and sets that block's result bits
    /* 0x03E */ u16 unk3E;
    /* 0x040 */ u16 frameTimer; // the wait opcode's frame counter: incremented by the frame count and reset when it reaches the operand
    /* 0x042 */ u16 repeatCount; // the repeat opcode's counter: incremented per re-entry and reset when it reaches the operand
    /* 0x044 */ u16 waitTimer; // frames elapsed of the wait that resumes at pWaitScript
    /* 0x046 */ u16 waitFrames; // that wait's length: pWaitScript resumes once waitTimer reaches it
    /* 0x048 */ s16 arrivalDistance; // command 0x2E copies moveDistance here; pArrivalScript resumes once BattleGetActorSpriteDistance is within it
    /* 0x04A */ u16 flags4A;
    /* 0x04C */ s32 pArrivalScript; // script offset resumed when the actor is within arrivalDistance
    /* 0x050 */ s32 pWaitScript; // script offset resumed when waitTimer reaches waitFrames
    /* 0x054 */ s32 pGroundScript; // script offset resumed when the ground projection puts the sprite lower than its gravity
    /* 0x058 */ u16 followTargetId; // target id func_800AEF68 resolves every frame to write posX/posY/posZ from that actor's position; 0 = none
    /* 0x05A */ u16 followJointIndex; // which of that actor's sprites the composed matrix comes from
    /* 0x05C */ u8 parentActorSlot; // g_BattleActors slot of the actor whose transform this one inherits; 0xFF = none
    /* 0x05D */ u8 isAttached;
    /* 0x05E */ s16 parentJointIndex; // which of the parent's sprites this one hangs off
    /* 0x060 */ s16 groundY; // height of the ground point BattleSnapActorToGround projected onto
    /* 0x062 */ u8 hasSedsEntry1; // a SEDS entry is open for pSoundBank1; func_800A9FF0 removes it
    /* 0x063 */ u8 hasSedsEntry2; // and one for pSoundBank2; func_800AAD54 opens it and BattleFreeActorBuffer removes it
    /* 0x064 */ u16 localX; // the parent-relative position the parent matrix rotates into posX/posY/posZ, as in libgear's Model
    /* 0x066 */ u16 localY;
    /* 0x068 */ u16 localZ;
    /* 0x06A */ u16 attachOffsetX;
    /* 0x06C */ u16 attachOffsetY;
    /* 0x06E */ u16 attachOffsetZ;
    /* 0x070 */ u16 rotOffset[3]; // added to the sprite's rot[0..2] every frame, >>3
    /* 0x076 */ u16 rotStep[3]; // added to rotOffset every frame
    /* 0x07C */ u16 posOffset[3]; // scaled by the sprite's scale and rotated into its position every frame
    /* 0x082 */ u16 posStep[3]; // added to posOffset every frame
    /* 0x088 */ s16 posX;
    /* 0x08A */ s16 posY;
    /* 0x08C */ s16 posZ;
    /* 0x08E */ u16 moveDistance; // the distance a move command stops at: the distance commands scale its operand by the actor's size, and command 0x2E copies it into arrivalDistance
    /* 0x090 */ s16 textureOffsetX; // added to the material records' texture x, as in libgear's Model; negative = no offset
    /* 0x092 */ u16 textureOffsetY;
    /* 0x094 */ u16 blendOffsetX; // added to the command's x, as in libgear's Model (func_800AE2A4)
    /* 0x096 */ u16 blendOffsetY; // and to its y
    /* 0x098 */ s16 stepIndex; // the move command's current step; -1 = none installed
    /* 0x09A */ s16 stepCount; // steps the move command runs; -1 = repeat forever
    /* 0x09C */ s16 pointIndex; // points of the current step already run
    /* 0x09E */ s16 pointCount; // points in the current step
    /* 0x0A0 */ mem_addr pPointCursor; // the point being run; func_800AE2A4 advances it
    /* 0x0A4 */ mem_addr pPointList; // where pPointCursor restarts after the last step
    /* 0x0A8 */ mem_addr pModelFile; // the loaded model file; freed just before BattleFreeModelMesh(pMesh)
    /* 0x0AC */ mem_addr pModelSource; // the model's source record; func_800A9FF0 releases it, as LibGearFreeModel does
    /* 0x0B0 */ u8* pSoundBank1; // the libgear texture-upload record whose +8 SEDS entry feeds battle sound bank 1
    /* 0x0B4 */ u8* pSoundBank2; // and bank 2
    /* 0x0B8 */ POLY_FT4 polys[2]; // the screen quads BattleDrawActor fills, one per double-buffered frame: quadIndex picks one
    /* 0x108 */ u8 unk108[0x2];
    /* 0x10A */ u16 modelBitMask;
    /* 0x10C */ u8 numEntries;
    /* 0x10D */ u8 numModelListEntries; // 0x24-byte records at pModelListEntries
    /* 0x10E */ u8 numTextures; // 0x30-byte upload records at pTextures
    /* 0x10F */ u8 unk10F;
    /* 0x110 */ mem_addr pEntries; // numEntries BattleActorEntry records
    /* 0x114 */ mem_addr pModelListEntries; // freed one by one with BattleFreeModelListEntryBuffers
    /* 0x118 */ Temp8* pTextures; // the libgear texture-upload records of the model: freed one by one with BattleFreeImageEntry
    /* 0x11C */ u8 unk11C[0x84];
} BattleActor; // Size: 0x1A0

// The per-slot action state a slot entry carries at +0xA4 (0xCC bytes): the
// status words, the scaled stats and the slot's copy of the equipped weapon
// records. g_BattleCurSlotState and g_BattleTargetSlotState point at the acting and the defending
// slot's copy of it. Only the fields read by src/battle/main's misc18, misc19 and
// misc20 are named.
// The 0xA4 bytes from +0 up hold the gear record's layout: the equipped weapon records, hp/maxHp,
// the int16 stats, the effect flag words and the equipment bonuses sit at the offsets GameGear
// (include/main/game.h) declares them at, and misc18's func_80097D5C adds each bonus into the stat
// it names.
typedef struct {
    /* 0x00 */ u8 unk00[0x4];
    /* 0x04 */ u8 extraWeaponIds[4]; // the gearWeapons record equipped in each weapon slot, as in GearState
    /* 0x08 */ u8 unk08[0x8];
    /* 0x10 */ GearWeaponData weaponData[3]; // sel == 3 lands in unk28, as in retail
    /* 0x28 */ u8 unk28[0x10];
    // The fuel gauge, at the offsets GameGear keeps it at: a battle start writes 9900 into both,
    // and func_8009A2D4 reads fuel < maxFuel >> 3 as the low-hp condition.
    /* 0x38 */ u16 fuel;
    /* 0x3A */ u16 maxFuel;
    /* 0x3C */ u8 engineOutput; // multiplied by attack to give the gear's attack (misc19's func_8009A2D4)
    /* 0x3D */ u8 pad3D;
    /* 0x3E */ u8 unk3E; // attackPower plus the accessory bonus, as attack
    /* 0x3F */ u8 attack; // the multiplier engineOutput turns into the gear's attack (BattleComputeActionAttackValue)
    /* 0x40 */ u16 defenseFromEquipment;
    /* 0x42 */ u16 etherDefenseFromEquipment;
    /* 0x44 */ u16 weightFromEquipment;
    /* 0x46 */ u16 unk46;
    /* 0x48 */ u8 unk48;
    /* 0x49 */ u8 pad49;
    /* 0x4A */ u8 weightPenalty; // subtracted from agility when the slot's stats are built (func_80097D5C), as in GameGear
    /* 0x4B */ u8 pad4B;
    /* 0x4C */ u8 unk4C;
    /* 0x4D */ u8 responsivenessFromEquipment; // added into responsiveness (0x9F) when the slot's stats are built
    /* 0x4E */ u8 agilityFromEquipment; // added into agility, minus weightPenalty
    /* 0x4F */ u8 unk4F;
    /* 0x50 */ u8 statBonuses[0x10]; // per-index bonus added back into a stat: [0..3] into agility (BattleResetSlotConditionCounters), [4] into amountMultiplier, [6] into the attack at 0x3E/0x3F
    /* 0x60 */ u32 hp; // scaled current hp
    /* 0x64 */ u32 maxHp; // scaled max hp
    /* 0x68 */ u16 weight; // weightFromEquipment and unk46 are added to it
    /* 0x6A */ u8 pad6A[0x6];
    /* 0x70 */ u16 baseDefense; // defenseFromEquipment is added to it
    /* 0x72 */ u16 valueBonus; // added to the action's value, scaled by (100 - penaltyPercent) / 100
    /* 0x74 */ u8 attackPower; // copied into unk3E and attack
    /* 0x75 */ u8 pad75[0x7];
    // The active/accessory effect word grid, at the same offsets as in GameCharacter
    // and GameGear (include/main/game.h). Battle code reads these three pairs where
    // the actor record's activeEffectFlags4 and activeEffectFlags7 are tested.
    /* 0x7C */ u16 activeEffectFlags1;
    /* 0x7E */ u16 accessoryEffectFlags1;
    /* 0x80 */ u16 activeEffectFlags2;
    /* 0x82 */ u16 accessoryEffectFlags2;
    /* 0x84 */ u16 activeEffectFlags3;
    /* 0x86 */ u16 accessoryEffectFlags3;
    /* 0x88 */ u8 statusPenaltyPercent[0x10]; // percent taken off an action, one entry per status bit, indexed by the highest set bit of BattleRow.statusMask
// The status penalty percent that takes an action's whole value: an entry at or above it
// zeroes the value, and one below it scales the value by what is left.
#define BATTLE_STATUS_PENALTY_PERCENT_MAX 20
    /* 0x98 */ u8 agility; // copied from the slot entry's agility and clamped the same way: BattleResetSlotConditionCounters keeps it at most 0x10
    /* 0x99 */ u8 penaltyPercent; // the slot's own value and the action's amount are scaled by (100 - it) / 100
    /* 0x9A */ u8 unk9A[0x2];
    /* 0x9C */ u8 damageReduction; // scales an action's amount by (BATTLE_ACTION_POWER_FULL / 2 - it) / BATTLE_ACTION_POWER_FULL, capped one below half
    /* 0x9D */ u8 unk9D;
    /* 0x9E */ u8 amountMultiplier; // quarter units the action's value is multiplied by, 4 = unchanged; BattleScaleValueByMultiplierByte uses 4 in place of a clear byte or an armed activeEffectFlags1 bit 0x100
    /* 0x9F */ s8 responsiveness; // the attacker adds half of it to its hit, the defender all of it to its evade
    /* 0xA0 */ u8 unkA0[0x4];
    /* 0xA4 */ u8 commandCounter; // g_BattleCommandCounter points at it: advanced per row and set from g_BattleCommandCharIndex for the higher rows
    /* 0xA5 */ u8 commandCounterMax; // what commandCounter is clamped to; BattleResetActorStats zeroes it
    /* 0xA6 */ u8 unkA6[0x10];
    /* 0xB6 */ u8 flags; // bit 0x80 selects the gear-side status words (activeEffectFlags2) over the actor's (activeEffectFlags4)
    /* 0xB7 */ u8 conditionTimers[0x15]; // one countdown byte per armed condition flag bit; the flag clears when its timer runs out
} BattleSlotState; // Size: 0xCC

// One slot of the per-battle-slot table family (stride 0x170). Three of them
// open the battle work block (g_BattleCurWorkBlock->slots); g_BattleActingSlot/g_BattleTargetSlotEntry point
// at the acting slot and its target. The first 0xA4 bytes are the character's
// GameCharacter record (its stats, accessory effect flag words and gear id sit
// at these offsets) and the 0xCC after them are the battle copy of it.
// Only the fields read by src/battle/main/misc18 and misc19 are named.
typedef struct {
    /* 0x000 */ u16 flags;
    /* 0x002 */ u8 chance; // percent, rolled against rand() % 100 by BattleTryApplySlotCondition
    /* 0x003 */ u8 conditionKind; // passed as that function's kind word; its bits pick the condition word to clear
    /* 0x004 */ u8 unk04[0x24];
    // The equipment bonuses of the character record these 0xA4 bytes are (include/main/game.h,
    // GameCharacter at the same offsets): misc18's func_80097D5C adds each to the stat it names.
    /* 0x028 */ u8 attackFromEquipment;
    /* 0x029 */ u8 defenseFromEquipment;
    /* 0x02A */ u8 agilityFromEquipment;
    /* 0x02B */ u8 etherFromEquipment;
    /* 0x02C */ u8 etherDefenseFromEquipment;
    /* 0x02D */ u8 defenseFromAccessories;
    /* 0x02E */ u8 equipmentPercents[0x2]; // GameCharacter's hit and evade percentages from equipment, in that order; func_80097D5C adds [0] to evade and [1] to hit
    /* 0x030 */ u8 accessoryEffectFlags8; // as in GameCharacter: the type-8 accessory flag word, summed per accessory
    /* 0x031 */ u8 mpPercentFromAccessories; // the character record's field of this offset and name: percent of maxMp added to both mp and maxMp at battle init
    /* 0x032 */ u16 accessoryEffectFlags5; // the type-5 accessory flag word, as in GameCharacter: bit 0x40 doubles the condition timer, the 0x400/0x200 pair picks a drop rate
    /* 0x034 */ u16 blockFlags; // 0x4000/0x8000: the action is nullified; the row's actionFlags 0x100 picks which bit is tested
    /* 0x036 */ u16 unk36;
    /* 0x038 */ u16 element; // the slot's own element mask in the low 6 bits, as the row's element: bit 0x40 raises the affinity multiplier from 15 to 18
    /* 0x03A */ u16 actionCount; // the per-action counter of the character record at this offset: BattleResolveSlotActionState adds one per action that landed and clamps it at 0xFDE8
    /* 0x03C */ u8 unk3C[0x10];
    /* 0x04C */ u16 hp;
    /* 0x04E */ u16 maxHp; // both scaled by BATTLE_ACTOR_HP_SCALE into state.hp / state.maxHp
    /* 0x050 */ u16 mp; // the same current/max pair as hp/maxHp: scaled by 10 into slotAmounts
    /* 0x052 */ u16 maxMp; // drawn as a gauge target like maxHp; the growth copy caps mp and maxMp at 99
    /* 0x054 */ u16 unk54;
    /* 0x056 */ u8 characterId; // indexes the per-character g_GameState table
    /* 0x057 */ u8 unk57;
    /* 0x058 */ u8 attack;
    /* 0x059 */ u8 defense;
    /* 0x05A */ u8 agility;
    /* 0x05B */ u8 ether; // the ether action's amount is ether * row->power
    /* 0x05C */ u8 etherDefense;
    /* 0x05D */ u8 unk5D;
    /* 0x05E */ u8 evade; // the defender's evasion percent
    /* 0x05F */ u8 hit; // the attacker's hit percent
    /* 0x060 */ u8 conditionChance; // what chanceSource 0 reads: rolled against rand() % 100 in place of the row's own chance
    /* 0x061 */ u8 pad61;
    // The points the slot has to spend on an action: BattleTryUseSlotCommand needs this word to reach the action
    // row's commandPointCost before that row's action runs and then subtracts the cost, and
    // BattleRenderActorValueDigits draws it as the slot's number.
    /* 0x062 */ u16 commandPoints;
    /* 0x064 */ u8 pad64[0xB];
    /* 0x06F */ u8 extraWeaponIds[5]; // the gearWeapons record index of each extra weapon slot, as in GameCharacter: BattleDecrementGearPartUse spends the gear part count of [0] and [3]
    /* 0x074 */ u8 pad74[0x6]; // GameCharacter's accessorySlots[3] and the bytes after them
    // The effect words run in pairs: each activeEffectFlags word holds one bit per effect the slot
    // currently has, and the accessoryEffectFlags word right after it holds those same bits as
    // granted permanently by a worn accessory. Battle code ORs the two (the pair answers "already
    // has this effect"), and BattleDecrementSlotConditionTimers counts a timer in
    // BattleSlotState.conditionTimers down only for a bit the accessory word lacks, so an effect
    // the accessory grants never expires. BattleInitActiveSlotAction clears all five active words.
    /* 0x07A */ u16 conditionFlags; // one bit per condition, initialised to 0x1FBF at reset; a cleared bit makes the slot's status draw (BattleDrawCharStatusIfMaskUnset)
    /* 0x07C */ u16 activeEffectFlags1;
    /* 0x07E */ u16 accessoryEffectFlags1; // the accessory effect flag words of GameCharacter sit at these same offsets: a set bit makes the slot refuse the condition
    /* 0x080 */ u16 activeEffectFlags2;
    /* 0x082 */ u16 accessoryEffectFlags2;
    /* 0x084 */ u16 activeEffectFlags3;
    /* 0x086 */ u16 accessoryEffectFlags3;
    /* 0x088 */ u16 activeEffectFlags4;
    /* 0x08A */ u16 accessoryEffectFlags4;
    /* 0x08C */ u16 activeEffectFlags7;
    /* 0x08E */ u16 accessoryEffectFlags7;
    /* 0x090 */ u16 statWeights[8]; // the dressing-room stat weights of GameCharacter, at its offsets; func_8009BE0C copies the first seven over
    /* 0x0A0 */ u8 gearId; // the gear the slot's character has equipped, as in GameCharacter
    /* 0x0A1 */ u8 accessoryEffectFlags10; // as in GameCharacter: the type-10 accessory flag word, summed per accessory
    /* 0x0A2 */ u8 unkA2[0x2];
    /* 0x0A4 */ BattleSlotState state;
} BattleTableEntry; // Size: 0x170

// The per-slot battle register file (g_BattleSlotRegs[slot]): the two script
// cursors at +0x00/+0x08 and the dword, word and byte registers the script's
// comparison ops test against, indexed by the operand byte that follows the
// opcode.
typedef struct {
    /* 0x00 */ u8* pScriptCursor; // the script BattleScriptResetActorState walks
    /* 0x04 */ u8 unk04[0x4];
    /* 0x08 */ u8* pScanCursor; // the script BattleScriptScanForOpcode62 scans
    /* 0x0C */ u8* pRunCursor; // the script BattleRunActiveSlotScripts walks (same byte stream as pScriptCursor)
    /* 0x10 */ u32 dwordRegs[4];
    /* 0x20 */ u16 wordRegs[8];
    /* 0x30 */ u8 byteRegs[0x10];
} BattleSlotRegs; // Size: 0x40

// One 0x28-byte row of the per-actor action tables the battle work block holds
// at +0x1058 (0x5F0 bytes per actor) and +0x2228 (0x690 bytes per actor). The
// row currently in use is pointed to by g_BattleCurrentRow. Only the fields read by
// src/battle/main/misc18 are named; +0x14 is read as a byte and +0x27 as an s8
// by other units.
typedef struct {
    /* 0x00 */ u16 flags;
    /* 0x02 */ u16 unk02;
    /* 0x04 */ u8 pad04[0x4];
    /* 0x08 */ u16 statusMask; // one bit per status; the highest set bit picks the slot's counter
    /* 0x0A */ u16 actionFlags;
    /* 0x0C */ u8 unk0C[0x4];
    /* 0x10 */ u8 weaponSlotFlags; // which weapon slots the row needs a gear part for (character 4, the one with four guns): bit 0x80 extraWeaponIds[0], 0x10 and 0x20 extraWeaponIds[3]
    /* 0x11 */ u8 power; // multiplies the target's stat to give the action's amount
    /* 0x12 */ u8 unk12[0x2];
    /* 0x14 */ u8 chanceBonus; // added to the acting slot's percent before the rand() % 100 roll
    /* 0x15 */ u8 evadeModifier; // read as s8, added to the defender's evade in the hit-chance roll
    /* 0x16 */ u8 opcode; // selects the handler from the D_800C34DC table
    /* 0x17 */ u8 unk17;
    /* 0x18 */ u8 chanceSource; // which field the condition chance is read from
    /* 0x19 */ u8 pad19;
    /* 0x1A */ u8 amountSource; // which stat the condition amount is scaled from
    /* 0x1B */ u8 pad1B;
    /* 0x1C */ u8 chance; // percent, rolled against rand() % 100
    /* 0x1D */ u8 conditionKind; // passed as the kind to BattleSetSlotConditionTimer
    /* 0x1E */ u16 conditionFlags; // passed as its flag word; its bits pick the condition word to clear
    /* 0x20 */ u8 unk20;
    /* 0x21 */ u8 unk21;
    /* 0x22 */ u8 element; // attack element, low 6 bits; copied from the equipped weapon
    /* 0x23 */ u8 unk23;
    /* 0x24 */ u16 commandPointCost; // the slot's commandPoints must reach it before the row's action runs; BattleTryUseSlotCommand then subtracts it
    /* 0x26 */ u8 pad26;
    /* 0x27 */ s8 agilityOffset; // subtracted from the slot's agility in the action countdown,
                                 // (agility - it) * 9 (BattleSelectActionRow); at or above it the countdown is a flat 9
} BattleRow; // Size: 0x28

// One 0x14-byte record of the battle work block's two per-character gear
// weapon tables: +0x5750 is the record a weapon slot is equipped from,
// +0x5B38 the id list BattleGearHasWeaponId searches. Only the bytes those two
// functions read are named.
typedef struct {
    /* 0x0 */ u8 unk0[0xC];
    /* 0xC */ u8 partId; // written to g_GameState.gearPartsIds for this weapon
    /* 0xD */ u8 unkD;
    /* 0xE */ u8 attack; // BattleApplyGearWeaponRecord copies it over the equipped weapon's attack
    /* 0xF */ u8 weaponId; // matched against the equipped GearWeaponData's weapon id
    /* 0x10 */ u8 chance; // BattleApplyGearWeaponRecord copies it over the weapon record's chance, which BattleTryApplyCondition reads as an s8 percent
    /* 0x11 */ u8 conditionKind; // and over that record's kind word, the condition BattleTryApplyCondition applies
    /* 0x12 */ u8 unk12[0x2];
} BattleGearWeaponRecord; // Size: 0x14

// The battle work block (g_BattleCurWorkBlock): the three slot entries, the per-actor
// action-row tables, and the per-slot action state the battle main loop runs
// on. Only the tail read by src/battle/main/misc18 is named; the row tables at
// +0x1058/+0x2228 and the scratch row at +0x35D8 are still reached by offset.
typedef struct {
    /* 0x0000 */ BattleTableEntry slots[3];
    /* 0x0450 */ u8 unk450[0x5300];
    /* 0x5750 */ BattleGearWeaponRecord gearWeapons[50];
    /* 0x5B38 */ BattleGearWeaponRecord gearWeaponIds[50];
    /* 0x5F20 */ u8 unk5F20[0x34];
    /* 0x5F54 */ s32 characterAmounts[3]; // per-party-slot running total of slotAmounts for the slots whose state.flags lacks BATTLE_SLOT_GEAR_STATUS_FLAG
    /* 0x5F60 */ s32 gearAmounts[3]; // and for the slots that have it
    /* 0x5F6C */ s32 slotAmounts[BATTLE_SLOT_COUNT]; // the amount the current action deals to each slot; its state code is slotStates
    /* 0x5F9C */ u8 unk5F9C[4];
    /* 0x5FA0 */ u8 slotStates[BATTLE_SLOT_COUNT]; // the state each slot's action reached; 0xFF = cleared
    /* 0x5FAC */ u16 activeSlotMask; // the slots the current row runs for; BattleFilterSlotMaskByGroup narrows it
    /* 0x5FAE */ u16 slotMask; // the same mask before that filter
    /* 0x5FB0 */ u16 unk5FB0;
    /* 0x5FB2 */ u8 unk5FB2[0xA];
    /* 0x5FBC */ u8 unk5FBC;
    /* 0x5FBD */ u8 unk5FBD;
    /* 0x5FBE */ u8 element; // the current action's element: the row's byte, with its low 6 bits filled from the affinity nibble when the row left them clear
    /* 0x5FBF */ u8 unk5FBF;
    /* 0x5FC0 */ u8 unk5FC0;
    /* 0x5FC1 */ u8 actorIndex; // selects the actor's row table (stride 0x690)
    /* 0x5FC2 */ u8 rowIndex; // selects the row in it (stride 0x28)
    /* 0x5FC3 */ u8 unk5FC3[4];
    /* 0x5FC7 */ u8 commandCode;
} BattleWorkBlock; // Size: 0x5FC8

// One instruction of the bytecode a battle slot's handlers step through: the
// opcode byte followed by its operand bytes. Instructions vary in length, so
// only the four bytes src/battle/main/misc7 reads are named. Those handlers
// take the slot's instruction pointer by address, so `(*p)` is the instruction
// being executed.
typedef struct {
    /* 0x0 */ u8 opcode;
    /* 0x1 */ u8 operand1;
    /* 0x2 */ u8 operand2;
    /* 0x3 */ u8 operand3;
} BattleCommand; // Size: 0x4

#endif
