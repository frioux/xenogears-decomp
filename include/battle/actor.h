#ifndef _XENO_BATTLE_ACTOR_H
#define _XENO_BATTLE_ACTOR_H

#include "psyq/libgte.h"
#include "field/actor.h"
#include "main/game.h"

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
// src/battle/main/misc28 touches are named.
typedef struct {
    /* 0x00 */ BattleFixed16Vector position;
    /* 0x0C */ IVEC3 step;
    /* 0x18 */ s32 moveSpeed;
    /* 0x1C */ u32 gravity;
    /* 0x20 */ SpriteDataI1* pBase;
    /* 0x24 */ u8 unk24[0xE];
    /* 0x32 */ s16 direction;
    /* 0x34 */ u8 unk34[0x8];
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
    /* 0x008 */ s32 unk8;
    /* 0x00C */ s32 pBuffer; // the block BattleFreeActorBuffer HeapFree()s; its +4 holds a pointer into the action-value table
    /* 0x010 */ s32 unk10;
    /* 0x014 */ s32 unk14;
    /* 0x018 */ s32 unk18;
    /* 0x01C */ s16 scale;
    /* 0x01E */ s16 groundTriangleIndex; // the ground search's last hit, handed back to it as the next hint
    /* 0x020 */ u8 unk20[0x3];
    /* 0x023 */ u8 unk23;
    /* 0x024 */ s16 height;
    /* 0x026 */ s16 width;
    /* 0x028 */ s16 depth;
    /* 0x02A */ u8 commands[0x1]; // the command being run: low 7 bits index the action entry tables
    /* 0x02B */ u8 commandCount;
    /* 0x02C */ u8 unk2C[0x2];
    /* 0x02E */ u8 commandArgs[0x6]; // the argument queued alongside commands[n]
    /* 0x034 */ u8 doUpdate; // gates the per-frame animation update; inherited from the parent actor
    /* 0x035 */ u8 unk35;
    /* 0x036 */ u8 unk36;
    /* 0x037 */ u8 unk37;
    /* 0x038 */ u8 isBusy; // keeps BattleWaitAndFreeActorBuffers and BattleWaitForOtherActorAnimations waiting while it is set
    /* 0x039 */ u8 unk39;
    /* 0x03A */ s16 unk3A;
    /* 0x03C */ u16 unk3C;
    /* 0x03E */ u16 unk3E;
    /* 0x040 */ u16 unk40;
    /* 0x042 */ u16 unk42;
    /* 0x044 */ u8 unk44[0x6];
    /* 0x04A */ u16 flags4A;
    /* 0x04C */ s32 unk4C;
    /* 0x050 */ s32 unk50;
    /* 0x054 */ s32 unk54;
    /* 0x058 */ u16 unk58;
    /* 0x05A */ u16 unk5A;
    /* 0x05C */ u8 parentActorSlot; // g_BattleActors slot of the actor whose transform this one inherits; 0xFF = none
    /* 0x05D */ u8 isAttached;
    /* 0x05E */ s16 parentJointIndex; // which of the parent's sprites this one hangs off
    /* 0x060 */ s16 groundY; // height of the ground point BattleSnapActorToGround projected onto
    /* 0x062 */ u8 unk62;
    /* 0x063 */ u8 hasSedsEntry; // a SEDS entry is open for this actor; BattleFreeActorBuffer removes it
    /* 0x064 */ u8 unk64[0x6];
    /* 0x06A */ u16 attachOffsetX;
    /* 0x06C */ u16 attachOffsetY;
    /* 0x06E */ u16 attachOffsetZ;
    /* 0x070 */ u16 unk70[0xC];
    /* 0x088 */ s16 posX;
    /* 0x08A */ s16 posY;
    /* 0x08C */ s16 posZ;
    /* 0x08E */ u16 unk8E;
    /* 0x090 */ u8 unk90[0x8];
    /* 0x098 */ s16 stepIndex; // the move command's current step; -1 = none installed
    /* 0x09A */ s16 stepCount; // steps the move command runs; -1 = repeat forever
    /* 0x09C */ s16 pointIndex; // points of the current step already run
    /* 0x09E */ s16 pointCount; // points in the current step
    /* 0x0A0 */ mem_addr pPointCursor; // the point being run; func_800AE2A4 advances it
    /* 0x0A4 */ mem_addr pPointList; // where pPointCursor restarts after the last step
    /* 0x0A8 */ mem_addr pModelFile; // the loaded model file; freed just before BattleFreeModelMesh(pMesh)
    /* 0x0AC */ mem_addr unkAC;
    /* 0x0B0 */ u8* unkB0;
    /* 0x0B4 */ u8* unkB4;
    /* 0x0B8 */ u8 unkB8[0x52];
    /* 0x10A */ u16 modelBitMask;
    /* 0x10C */ u8 numEntries;
    /* 0x10D */ u8 numModelListEntries; // 0x24-byte records at pModelListEntries
    /* 0x10E */ u8 numTextures; // 0x30-byte upload records at pTextures
    /* 0x10F */ u8 unk10F;
    /* 0x110 */ mem_addr pEntries; // numEntries BattleActorEntry records
    /* 0x114 */ mem_addr pModelListEntries; // freed one by one with BattleFreeModelListEntryBuffers
    /* 0x118 */ mem_addr pTextures; // freed one by one with BattleFreeImageEntry
    /* 0x11C */ u8 unk11C[0x84];
} BattleActor; // Size: 0x1A0

// The per-slot action state a slot entry carries at +0xA4 (0xCC bytes): the
// status words, the scaled stats and the slot's copy of the equipped weapon
// records. D_800D2DC8 and D_800D2D6C point at the acting and the defending
// slot's copy of it. Only the fields read by src/battle/main/misc19 and
// misc20 are named.
typedef struct {
    /* 0x00 */ u8 unk00[0x4];
    /* 0x04 */ u8 extraWeaponIds[4]; // the gearWeapons record equipped in each weapon slot, as in GearState
    /* 0x08 */ u8 unk08[0x8];
    /* 0x10 */ GearWeaponData weaponData[3]; // sel == 3 lands in unk28, as in retail
    /* 0x28 */ u8 unk28[0x12];
    /* 0x3A */ u16 unk3A;
    /* 0x3C */ u8 unk3C[0x13];
    /* 0x4F */ u8 unk4F;
    /* 0x50 */ u8 unk50[0x10];
    /* 0x60 */ u32 hp; // scaled current hp
    /* 0x64 */ u32 maxHp; // scaled max hp
    /* 0x68 */ u8 unk68[0x14];
    /* 0x7C */ u16 unk7C;
    /* 0x7E */ u8 unk7E[0x2];
    /* 0x80 */ u16 unk80;
    /* 0x82 */ u16 unk82;
    /* 0x84 */ u16 unk84;
    /* 0x86 */ u8 unk86[0x13];
    /* 0x99 */ u8 penaltyPercent; // the slot's own value and the action's amount are scaled by (100 - it) / 100
    /* 0x9A */ u8 unk9A[0x4];
    /* 0x9E */ u8 unk9E;
    /* 0x9F */ u8 unk9F[0x5];
    /* 0xA4 */ u8 unkA4;
    /* 0xA5 */ u8 unkA5;
    /* 0xA6 */ u8 unkA6[0x10];
    /* 0xB6 */ u8 flags; // bit 0x80 selects the gear-side status words (unk80) over the actor's (unk88)
    /* 0xB7 */ u8 conditionTimers[0x15]; // one countdown byte per armed condition flag bit; the flag clears when its timer runs out
} BattleSlotState; // Size: 0xCC

// One slot of the per-battle-slot table family (stride 0x170). Three of them
// open the battle work block (g_BattleCurWorkBlock->slots); g_BattleActingSlot/D_800C3E34 point
// at the acting slot and its target. Only the fields read by
// src/battle/main/misc18 and misc19 are named.
typedef struct {
    /* 0x000 */ u16 flags;
    /* 0x002 */ u8 chance; // percent, rolled against rand() % 100 by BattleTryApplySlotCondition
    /* 0x003 */ u8 conditionKind; // passed as that function's kind word; its bits pick the condition word to clear
    /* 0x004 */ u8 unk04[0x2E];
    /* 0x032 */ u16 unk32; // flag word: bit 0x40 doubles the condition timer, the 0x400/0x200 pair picks a drop rate
    /* 0x034 */ u8 unk34[0x4];
    /* 0x038 */ u16 unk38;
    /* 0x03A */ u8 unk3A[0x12];
    /* 0x04C */ u16 hp;
    /* 0x04E */ u16 maxHp; // both scaled by 50 into state.hp / state.maxHp
    /* 0x050 */ u16 mp; // the same current/max pair as hp/maxHp: scaled by 10 into slotAmounts
    /* 0x052 */ u16 maxMp; // drawn as a gauge target like maxHp; the growth copy caps mp and maxMp at 99
    /* 0x054 */ u16 unk54;
    /* 0x056 */ u8 characterId; // indexes the per-character g_GameState table
    /* 0x057 */ u8 unk57[0x4]; // +1/+2/+3 mirror GameCharacter's attack/defense/agility; nothing reads them yet
    /* 0x05B */ u8 ether; // the ether action's amount is ether * row->power
    /* 0x05C */ u8 unk5C[0x2];
    /* 0x05E */ u8 evade; // the defender's evasion percent
    /* 0x05F */ u8 hit; // the attacker's hit percent
    /* 0x060 */ u8 unk60[0x1A];
    /* 0x07A */ u16 unk7A;
    /* 0x07C */ u16 unk7C;
    /* 0x07E */ u16 unk7E;
    /* 0x080 */ u16 unk80;
    /* 0x082 */ u16 unk82;
    /* 0x084 */ u16 unk84;
    /* 0x086 */ u16 unk86;
    /* 0x088 */ u16 unk88;
    /* 0x08A */ u16 unk8A;
    /* 0x08C */ u16 unk8C;
    /* 0x08E */ u16 unk8E;
    /* 0x090 */ u8 unk90[0x14];
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
    /* 0x0C */ u8 unk0C[0x4];
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
    /* 0x0C */ u8 unk0C[0x5];
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
    /* 0x24 */ u8 unk24[0x3];
    /* 0x27 */ s8 unk27;
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
    /* 0x10 */ u8 unk10;
    /* 0x11 */ u8 unk11;
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
    /* 0x5F20 */ u8 unk5F20[0x4C];
    /* 0x5F6C */ s32 slotAmounts[12]; // the amount the current action deals to each slot; its state code is unk5FA0
    /* 0x5F9C */ u8 unk5F9C[4];
    /* 0x5FA0 */ u8 unk5FA0[12];
    /* 0x5FAC */ u16 activeSlotMask; // the slots the current row runs for; BattleFilterSlotMaskByGroup narrows it
    /* 0x5FAE */ u16 slotMask; // the same mask before that filter
    /* 0x5FB0 */ u16 unk5FB0;
    /* 0x5FB2 */ u8 unk5FB2[0xA];
    /* 0x5FBC */ u8 unk5FBC;
    /* 0x5FBD */ u8 unk5FBD;
    /* 0x5FBE */ u8 unk5FBE;
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
    /* 0x0 */ u8 unk0;
    /* 0x1 */ u8 unk1;
    /* 0x2 */ u8 unk2;
    /* 0x3 */ u8 unk3;
} BattleCommand; // Size: 0x4

#endif
