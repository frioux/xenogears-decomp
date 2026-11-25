#ifndef _XENO_GAME_H
#define _XENO_GAME_H

// Number of ids in each inventory table (the tables themselves are declared below;
// the MAX_INVENTORY_* macros are the array dimensions, which are larger).
#define MENU_WEAPON_ID_COUNT 72
#define MENU_ACCESSORY_ID_COUNT 150
#define MENU_ITEM_ID_COUNT 76
#define MENU_GEAR_WEAPON_ID_COUNT 72
#define MENU_GEAR_PART_ID_COUNT 105


#define MAX_GAME_CHARACTERS 11
// one bit per character id in g_GameState.characterFlags, cleared by MenuResetConditionMasks
#define MENU_CHARACTER_FLAG_MASK 0x7FF
#define MAX_GAME_GEARS 20
#define MAX_PARTY_MEMBERS 3
#define MAX_BENCHED_PARTY_MEMBERS 6
#define CHARACTER_ID_NONE 0xFF
// g_WorldMapAreaId / g_WorldMapMode4AreaId when no area is loaded: both are cleared
// to it before a map's tables are read, and the mode-4 exit guards test it.
#define WORLD_MAP_AREA_NONE (-1)
// an angle is a 4096-step turn counter; every sum of angles is wrapped back into it
#define WORLD_MAP_ANGLE_MASK 0xFFF
#define WORLD_MAP_TILE_SIZE 2048 // world-position units per tile (a tile is 2048 << 12 in the 12.4 form)
#define GEAR_ID_NONE 0xFF

#define MAX_ITEM_QUANTITY 99
// equipment weight is counted in points; baseWeightClass is weightFromEquipment / this
#define MENU_WEIGHT_POINTS_PER_CLASS 120
#define MAX_INVENTORY_WEAPONS 100
#define MAX_INVENTORY_ACCESSORIES 200
#define MAX_INVENTORY_ITEMS 150

#define MAX_GOLD_AMOUNT 9999999


// One of the four 0x8-byte records at the head of a GameCharacter: the cached
// stats of one equipped weapon at +0x0..+0x4, written by
// MenuApplyEquippedItemBonuses, and, in g_GameState.characters[4], the id of the
// character in that battle party slot at +0x6 (see BattleIsCharacterInParty).
typedef struct {
    /* 0x0 */ u16 flags; // OR'ed into GameCharacter.accessoryEffectFlags7 while conditionKind reads 0x64
    // The equipped weapon record's bytes 0xA and 0xB, written by MenuApplyEquippedItemBonuses. The
    // battle copy of this record (BattleTableEntry, include/battle/actor.h, whose first 0xA4 bytes
    // are the GameCharacter record) names the same two offsets: BattleApplyActorConditionTimer
    // passes them as the condition's chance and kind.
    /* 0x2 */ u8 chance;
    /* 0x3 */ u8 conditionKind;
    /* 0x4 */ u8 attack;
    /* 0x5 */ u8 unk5;
    /* 0x6 */ u8 characterId;
    /* 0x7 */ u8 unk7;
} GameCharacterSlot; // Size: 0x8

typedef struct {
    /* 0x00 */ GameCharacterSlot slots[4];
    // No code in the program reads or writes these eight bytes.
    /* 0x20 */ u8 unk20[8];
    /* 0x28 */ u8 attackFromEquipment;
    /* 0x29 */ u8 defenseFromEquipment;
    /* 0x2A */ u8 agilityFromEquipment;
    /* 0x2B */ u8 etherFromEquipment;
    /* 0x2C */ u8 etherDefenseFromEquipment;
    /* 0x2D */ u8 defenseFromAccessories; // total of the three accessories' defense field
    /* 0x2E */ u8 hitPercentageFromEquipment;
    /* 0x2F */ u8 evadePercentageFromEquipment;
    /* 0x30 */ u8 accessoryEffectFlags8; // the type-8 accessory flag word, summed not OR'ed: cases 8 and 9 both accumulate into it (MenuApplyEquippedItemBonuses)
    /* 0x31 */ u8 mpPercentFromAccessories; // percent of maxMp added to mp and maxMp at battle init, cleared with the equipment bonuses (MenuApplyEquippedItemBonuses)
    /* 0x32 */ u16 accessoryEffectFlags5; // the type-5 accessory flag word, see the 0x7E block
    // No code in the program reads or writes these six bytes. The battle copy of this
    // record (BattleTableEntry, include/battle/actor.h) names them blockFlags, unk36 and
    // element.
    /* 0x34 */ u8 unk34[0x6];
    // The per-action counter the battle slot keeps at this same offset (it is incremented
    // and clamped at 0xFDE8 by BattleResolveSlotActionState) and copies into the character
    // record after the battle, see func_8009BE0C.
    /* 0x3A */ u16 actionCount;
    // The gear window's two wide numbers, drawn by MenuSetGearWindowWideNumberPrims
    // (src/menu/main/misc7_r2.c); no function here assigns them, so they only change
    // with the save record.
    /* 0x3C */ s32 unk3C;
    /* 0x40 */ s32 unk40;
    // Two 32-bit numbers the menu window draws wide (up to 8 digits) with
    // MenuParseNumberToDigits, in both the character and the gear window. BattleInitActiveSlotActions
    // (BattleApplyActionRowToSlot) is the only function in the program that writes them: one case of its jump table
    // stores 1 into both.
    /* 0x44 */ s32 unk44;
    /* 0x48 */ s32 unk48;
    /* 0x4C */ u_short hp;
    /* 0x4E */ u_short maxHp;
    /* 0x50 */ u_short mp;
    /* 0x52 */ u_short maxMp;
    // No code in the program reads or writes these two bytes. The battle copy of this
    // record keeps a u16 here whose low byte BattleAdjustSlotAmountByCondition bumps by
    // the action's row.
    /* 0x54 */ u8 unk54[2];
    // The same character id the battle copy of this record keeps at this offset
    // (BattleTableEntry.characterId); 4 is the character with two weapon slots,
    // see MenuApplyEquippedItemBonuses.
    /* 0x56 */ u8 characterId;
    /* 0x57 */ u8 unk57;
    /* 0x58 */ u_char attack;
    /* 0x59 */ u_char defense;
    /* 0x5A */ u_char agility;
    /* 0x5B */ u_char ether;
    /* 0x5C */ u_char etherDefense;
    /* 0x5D */ undefined8 unk5D;
    /* 0x5E */ u_char hitPercentage;
    /* 0x5F */ u_char evadePercentage;
    // The battle copy of this record names this byte conditionChance and the one after it
    // padding (BattleTableEntry); nothing here reads or writes either.
    /* 0x60 */ undefined16 unk60;
    /* 0x62 */ u_char level;
    /* 0x63 */ undefined8 unk63; // drawn as a 3-digit number beside the level digits into the same g_Menu->digits[6..8] slots (green in the member change menu), see MenuSetCharacterLevelDigits; nothing in the program writes it
    /* 0x64 */ undefined8 unk64[0x6];
    /* 0x6A */ undefined8 weaponId[0x5];
    /* 0x6F */ undefined8 extraWeaponIds[0x5]; // Billy's four guns are summed, so all four slots are weapons
    /* 0x74 */ undefined8 accessorySlots[0x3];
    // FieldScriptSetCharacterField78 stores argument - statBase in statBonus and
    // FieldScriptWriteCharacterFieldSum reads back their sum, so statBase is the value
    // the script cannot set below.
    /* 0x77 */ u8 statBase;
    /* 0x78 */ u8 statBonus;
    // The tenths digit of the statBase + statBonus total: the gear window's stat row
    // scales that sum by ten and adds this, see func_801D7884 in src/menu/main/misc7_r2.c.
    /* 0x79 */ u8 statFraction;
    /* 0x7A */ u16 conditionFlags; // one bit per condition, at the offset and under the name the battle copy of this record gives it (BattleTableEntry.conditionFlags)
    /* 0x7C */ u16 activeEffectFlags1; // starts the active/accessory word grid below, as in GameGear and BattleTableEntry
    // One flag word per accessory effectType: the OR of the effectFlags of the
    // worn accessories whose effectType selects it, see
    // MenuApplyEquippedItemBonuses in src/menu/main/misc10.c. The words in
    // between are touched by no matched code here; the battle copy of this
    // record names them, see BattleTableEntry in include/battle/actor.h.
    /* 0x7E */ u16 accessoryEffectFlags1;
    /* 0x80 */ u16 activeEffectFlags2;
    /* 0x82 */ u16 accessoryEffectFlags2;
    /* 0x84 */ u16 activeEffectFlags3;
    /* 0x86 */ u16 accessoryEffectFlags3;
    /* 0x88 */ u16 activeEffectFlags4;
    /* 0x8A */ u16 accessoryEffectFlags4;
    /* 0x8C */ u16 activeEffectFlags7;
    /* 0x8E */ u16 accessoryEffectFlags7; // also gains GameCharacterSlot.flags once that slot's conditionKind reads 0x64
    /* 0x90 */ u16 statWeights[8]; // one weight per dressing-room stat row (7 read), weighed against a per-character table to make a percentage, see the parked func_801E1418 in menu/main/misc10; BattleInitActiveSlotActions adds 10 to each of the first seven at battle init
    /* 0xA0 */ u_char gearId;
    /* 0xA1 */ u8 accessoryEffectFlags10; // the type-10 accessory flag word, summed not OR'ed, see MenuApplyEquippedItemBonuses
    /* 0xA2 */ undefined8 unkA2[0x2];
} GameCharacter; // size: 0xA4

typedef struct {
    /* 0x00 */ u16 flags; // OR'ed into GameGear.accessoryEffectFlags3's top nibble once conditionKind reads 100, see MenuApplyGearWeapons
    /* 0x02 */ u8 attack;
    // The weapon record's bytes 0x10 and 0x11, as BattleGearWeaponRecord (include/battle/actor.h)
    // names them: BattleApplyGearWeaponRecord copies its chance and conditionKind over these two,
    // and BattleTryApplyCondition rolls the chance as a percent and switches on the kind.
    /* 0x03 */ u8 chance;
    /* 0x04 */ u8 conditionKind;
    /* 0x05 */ u8 weaponId; // the weapon this cached record holds, see BattleGearHasWeaponId
    /* 0x06 */ u16 unk6;
} GearWeaponData;

typedef struct {
    /* 0x00 */ u8 unk0;
    /* 0x01 */ u8 unk1;
    /* 0x02 */ u8 frameId;
    /* 0x03 */ u8 engineId;
    /* 0x04 */ u8 extraWeaponIds[4];
    /* 0x08 */ u8 armorId;
    /* 0x09 */ u8 accessorySlots[3];
    /* 0x0C */ u8 weaponId[4];
    /* 0x10 */ GearWeaponData weaponData[3];
    /* 0x28 */ u8 unk28[0x10];
    /* 0x38 */ u16 fuel;
    /* 0x3A */ u16 maxFuel;
    /* 0x3C */ u8 engineOutput;
    // Both ApplyGearEngine functions copy these three bytes out of the engine record
    // (GearEngine.unkD into unk3D, GearEngine.unkE into both unk3E and unk3F) and nothing
    // reads them back. The battle copy of this record keeps attackPower plus its
    // accessory bonus at 0x3E/0x3F.
    /* 0x3D */ u8 unk3D;
    /* 0x3E */ u8 unk3E;
    /* 0x3F */ u8 unk3F;
    /* 0x40 */ u16 defenseFromEquipment; // total of defense boosts from slot1 + slot 2 + slot 3
    /* 0x42 */ u16 etherDefenseFromEquipment;
    /* 0x44 */ u16 weightFromEquipment;
    /* 0x46 */ u16 unk46;
    /* 0x48 */ u16 accessoryEffectFlags9; // the type-9 accessory flag word, see GearShopMenuRecalculateGearStats
    /* 0x4A */ u8 weightPenalty; // subtracted from agility, see GearShopMenuComputeEquipWeightPenalty
    /* 0x4B */ u8 unk4B;
    // Both ApplyGearAccessories functions clear unk4C and unk50[] and then sum them over
    // the three equipped accessories (GearAccessory.unk18 and GearAccessory.unk10[]), and
    // nothing reads them back; unk4E is cleared beside them and never added to or read.
    /* 0x4C */ u8 unk4C;
    /* 0x4D */ u8 responsiveness;
    /* 0x4E */ u8 unk4E;
    /* 0x4F */ u8 accessoryEffectFlags5; // summed, not OR'ed; nonzero sets the pilot's statusFlags bit 0x8000
    /* 0x50 */ u8 unk50[4];
    /* 0x54 */ u8 etherAmplificationFromEquipment; // added to etherAmplification for the total shown
    // [1] sums the effectFlags of the effectType-9 and 10 accessories and is added
    // to attackPower; [2] sums those of the type-11 ones. MenuApplyGearAccessories
    // zeroes all three in one loop, so they stay a single array.
    /* 0x55 */ u8 unk55[3];
    /* 0x58 */ u16 unk58;
    /* 0x5A */ u16 unk5A;
    /* 0x5C */ u8 unk5C[4];
    /* 0x60 */ u32 hp;
    /* 0x64 */ u32 maxHp;
    /* 0x68 */ u16 weight;
    // Copied into the dressing room's unkAA stat slot by MenuFillDressingRoomGearStats and
    // GearShopMenuFillDressingRoomStats; no other code reads or writes it.
    /* 0x6A */ u16 unk6A;
    /* 0x6C */ u16 unk6C;
    /* 0x6E */ u16 accessoryEffectFlags4; // one bit per accessory attribute, accessoryEffectAmounts[] accumulates its magnitude per bit
    /* 0x70 */ u16 baseDefense; /* impacted by AR value of equipped armor */
    /* 0x72 */ u16 baseEtherDefense;
    /* 0x74 */ u8 attackPower;
    /* 0x75 */ u8 baseWeightClass; // the equipment weight class (weightFromEquipment / MENU_WEIGHT_POINTS_PER_CLASS) above it halves into weightPenalty
    /* 0x76 */ u8 unk76;
    /* 0x77 */ u8 unk77;
    /* 0x78 */ u8 unk78;
    /* 0x79 */ u8 unk79;
    /* 0x7A */ u16 unk7A;
    // The gear keeps the same 2-byte active/accessory effect word grid as the
    // character record it is copied from (GameCharacter and BattleTableEntry
    // name the words at these offsets); no matched code here reads these.
    /* 0x7C */ u16 activeEffectFlags1;
    /* 0x7E */ u16 accessoryEffectFlags1; // the type-1 accessory flag word, see GearShopMenuRecalculateGearStats
    /* 0x80 */ u16 activeEffectFlags2;
    /* 0x82 */ u16 accessoryEffectFlags2;
    /* 0x84 */ u16 activeEffectFlags3;
    /* 0x86 */ u16 accessoryEffectFlags3; // low 12 bits; the top nibble holds the equipped weapon's flags
    /* 0x88 */ u8 accessoryEffectAmounts[0x10]; // summed effectAmount of the accessories whose effectFlags set the matching accessoryEffectFlags4 bit
    /* 0x98 */ u8 agility;
    // Round-tripped through the save slot as MenuSaveGear.unk38 (src/menu/main/misc11_r2.c)
    // and read or written nowhere else.
    /* 0x99 */ u8 unk99;
    /* 0x9a */ u8 unk9A;
    /* 0x9b */ u8 unk9B;
    // Both are copied into the dressing room's unkB5/unkB6 stat bytes. unk9D is loaded
    // from the gear frame (GearFrame.unk16) by both ApplyGearFrame functions; nothing in
    // the program writes unk9C.
    /* 0x9c */ u8 unk9C;
    /* 0x9d */ u8 unk9D;
    /* 0x9e */ u8 etherAmplification;
    /* 0x9f */ u8 responsivenessFromFrame;
    /* 0xA0 */ int unkA0;
} GameGear; // size: 0xA4

// g_GameState.strings: the 0x14 bytes SystemRenderStringEntry renders for one
// name. No code here reads a field of its own: the whole entry is passed as
// text, and SystemEncodeGlyphCodes rewrites it in place into u16 glyph codes
// (see the parked func_801CBA4C in src/menu/main/misc3.c).
typedef struct {
    /* 0x00 */ u32 text[5];
} StringEntry; /* size: 0x14 */

typedef struct {
    /* 0x00 */ s16 unk0;
    /* 0x02 */ s16 unk2;
    // The character-side and the gear-side command bit masks: each is tested bit by bit
    // against the 13 learnable command rows (src/battle/main/misc13, whose battle reset
    // sets one bit per command the character has) and against the command-availability
    // masks (BattleCheckActorCommandAvailable). gearCommandMask is the word the gear's
    // accessories grant: MenuApplyGearAccessories clears its derived bits and recomputes
    // them from the pilot's accessory effect flags.
    /* 0x04 */ u16 commandMask;
    // One bit per condition the slot can be given. The character-side and the
    // gear-side slot answer the same test from these two words, see
    // BattleDrawCharStatusIfFlagged and BattleRollActorActionCondition in src/battle/main/misc19.
    /* 0x06 */ u16 characterConditionMask;
    /* 0x08 */ u16 gearCommandMask;
    /* 0x0A */ u16 gearConditionMask;
    /* 0x0C */ s16 unkC;
    /* 0x0E */ s16 unkE;
    /* 0x10 */ u8 unk10;
    /* 0x11 */ u8 unk11;
    /* 0x12 */ s16 unk12;
    /* 0x14 */ s16 unk14;
    /* 0x16 */ s16 unk16;
    /* 0x18 */ s16 unk18;
    /* 0x1A */ u8 unk1A;
    /* 0x1B */ u8 unk1B; // both condition-mask resets write 7 here and nothing reads it
    /* 0x1C */ s16 unk1C;
    /* 0x1E */ u16 statusFlags; // status bit mask, tested through BattleMaskHasStatusBit
} UnkGearState; /* size: 0x20 */

typedef struct {
    /* 0x0000 */ StringEntry strings[31];
    /* 0x026C */ GameCharacter characters[MAX_GAME_CHARACTERS];
    /* 0x0978 */ GameGear gears[MAX_GAME_GEARS];
    /* 0x1648 */ undefined8 unk1648[0x74];
    // Eleven 0x20-byte per-character condition/command rows; the eleventh also runs over the
    // four bytes after it, which is what the eleven 0x20-byte rows at +0x16C0 of MenuResetConditionMasks cover.
    /* 0x16BC */ UnkGearState conditionRows[11];
    /* 0x17FC */ undefined8 unk17FC[0x4];
    // World map ground position in map units; << 12 gives the actor coordinate
    // WorldMapSetPositionFromState copies into g_WorldMapPlayerPosition.
    /* 0x1820 */ u16 worldMapPosX;
    /* 0x1822 */ u16 worldMapPosZ;
    // World map facing angle of the player actor, copied into g_WorldMapPlayerRotation
    // by WorldMapSetPositionFromState. The three party actors' angles follow it in
    // unk1826 (a u16[3] there); splitting that span changes the load schedule of
    // WorldMapInitPartyActorSprite0/1/2, so it stays opaque.
    /* 0x1824 */ u16 worldMapPlayerRotation;
    /* 0x1826 */ undefined8 unk1826[0x6]; // +0x1824..+0x182B read as a u16[4] of actor angles
    // World map position a field script latches with the 0x17FC30 opcode, see
    // WorldMapGetGameStatePosition and FieldScriptSetGameState17FC30.
    /* 0x182C */ s16 scriptPosX;
    /* 0x182E */ s16 scriptPosY;
    /* 0x1830 */ s16 scriptPosZ;
    /* 0x1832 */ u16 scriptAngle;
    /* 0x1834 */ u16 worldMapState;
    // Set while the world-map menu holds the map's state (WorldMapEnterMenuMode
    // saves gearRide here and sets 1, WorldMapExitMenuMode and
    // WorldMapSetPositionFromState clear it).
    /* 0x1836 */ u16 worldMapMenuActive;
    // The world-map state WorldMapMainLoop fills in before the map's script runs
    // and WorldMapInitializePlayerSlot reads back when the map is entered again.
    /* 0x1838 */ u16 pad1838[2];
    /* 0x183C */ u16 savedGearRide[MAX_PARTY_MEMBERS]; // g_GameState.gearRide[] saved while a mode-4 map runs, compared back by WorldMapSyncGearRideActors
    /* 0x1842 */ u16 hideTileLayer; // toggled by a SELECT press; while 0, WorldMapMainLoop calls func_800740B8, which draws the map's tile layer
    /* 0x1844 */ u16 playerMapPosX; // the player actor's WorldMapPlayer.x in map units, saved across a map change
    /* 0x1846 */ u16 playerMapPosZ;
    /* 0x1848 */ u16 playerTileIndex; // WorldMapPlayer.unk50, the index into the tile coordinate tables, saved with them
    /* 0x184A */ u16 playerSlotInitCount; // 0 until the player slot has been placed once; after that the saved fields above decide the position
    // World-map position the field scripts set, split as map units << 12 |
    // fraction: x = mapPosX << 12 | mapPosXFraction, z = mapPosZ << 12 |
    // mapPosZFraction (FieldScriptSetGameState184E writes the map-unit halves).
    /* 0x184C */ u16 mapPosXFraction;
    /* 0x184E */ undefined16 mapPosX;
    /* 0x1850 */ undefined16 mapPosZFraction;
    /* 0x1852 */ undefined16 mapPosZ;
    // Zero makes WorldMapInitializePlayerSlot place the player at the map's own default
    // start tile; that first placement counts it to 1, and from then on the player is
    // placed from mapPosX/mapPosZ. FieldScriptSetGameState184E clears it when a script
    // sets a new position, and raises the flag below.
    /* 0x1854 */ undefined16 playerPlaced;
    // Set to 1 by FieldScriptSetGameState184E beside playerPlaced = 0; nothing reads it.
    /* 0x1856 */ undefined16 unk1856;
    /* 0x1858 */ undefined8 unk1858[0xCC];
    /* 0x1924 */ u_int gold;
    // Nothing reads these eight bytes. The one access to them in the C, `*(s16*)&g_pGameState->unk1928[0xA]`
    // (field/main/misc11.c), indexes past the end of it and is really fieldScriptMemory[1].
    /* 0x1928 */ undefined8 unk1928[0x8];
    /* 0x1930 */ u16 fieldScriptMemory[0x200]; // saved copy of g_FieldScriptMemory, see FieldScriptSyncMemoryWithGameState
    /* 0x1D30 */ undefined16 characterFlags;
    /* 0x1D32 */ u_short FrMask;
    /* 0x1D34 */ undefined8 partyMembers[MAX_PARTY_MEMBERS];
    /* 0x1D37 */ undefined8 unk1D37;

    // Inventory
    /* 0x1D38 */ u_char weaponQuantities[MAX_INVENTORY_WEAPONS];
    /* 0x1D9C */ u_char weaponIDs[MAX_INVENTORY_WEAPONS];
    /* 0x1E00 */ u_char accessoryQuantities[MAX_INVENTORY_ACCESSORIES];
    /* 0x1EC8 */ u_char accessoryIDs[MAX_INVENTORY_ACCESSORIES];
    /* 0x1F90 */ u_char itemQuantities[MAX_INVENTORY_ITEMS];
    /* 0x2026 */ u_char itemIDs[MAX_INVENTORY_ITEMS];

    /* 0x20BC */ undefined8 gearWeaponsQty[0x64];
    /* 0x2120 */ undefined8 gearWeaponsIds[0x64];
    /* 0x2184 */ undefined8 gearPartsQty[0x96];
    /* 0x221A */ undefined8 gearPartsIds[0x96];
    // WorldMapUpdateActorSlot indexes `((u8*)&g_GameState.unk22B0)[index]` for index 0..2, a 3-byte
    // per-actor array over 0x22B0..0x22B2, so it overlaps gearRide[0..2] one byte off; nothing else
    // in the program touches this byte.
    /* 0x22B0 */ undefined8 unk22B0;
    /* 0x22B1 */ undefined8 gearRide[MAX_PARTY_MEMBERS];
    /* 0x22B4 */ undefined8 unk22B4[0x2];
    // The word at 0x22B6 is also the base of the 0x64-byte per-gear-weapon array the battle's gear-part
    // countdown indexes (`((u8*)&g_GameState.unk22B6)[weaponId]`, src/battle/main/misc20.c), and of
    // the two-digit per-weapon number func_80093B08 prints (src/battle/main/misc17.c). Read as a u16
    // flags word too: FieldScriptCopyCharacter ORs 0x2000 into it for character 9 and 0x1000 for
    // character 10, and MenuFillDressingRoomGearStats tests 0x1000 with an lhu.
    /* 0x22B6 */ undefined16 unk22B6;
    /* 0x22B8 */ undefined8 unk22B8[0x60];
    // One bit per character id that is in the party: FieldScriptSetCharacterBit2318 sets or
    // clears the bit of the character a script names.
    /* 0x2318 */ u16 partyMemberFlags;
    /* 0x231A */ u16 mapNum;
    // The 12-bit angle a pending field entry uses, set with mapNum by
    // FieldScriptVMHandlerSetFieldEntryState: an 8-step direction << 9, or the
    // current sceneAngle turned 180 degrees when the script passes -1.
    /* 0x231C */ u16 entryAngle;
    // The other two arguments of that opcode: FieldScriptVMHandlerSetFieldEntryState stores its
    // second argument here and its fourth in nextMapFlags, and nothing reads either back.
    // FieldScriptSyncMemoryWithGameState and FieldAdvanceGameState also copy
    // fieldScriptMemory[1] into nextMapFlags.
    /* 0x231E */ undefined16 unk231E;
    // The flags word of the pending map entry, set with mapNum and entryAngle: this address is the
    // extern g_NextMapFlags the world map's map-change requests store their flags in
    // (src/world_map/main/misc3.c, misc4.c, misc7.c).
    /* 0x2320 */ u16 nextMapFlags;
    /* 0x2322 */ u16 wdsBankIndex;
    /* 0x2324 */
} GameState; // size: unknown

extern GameState g_GameState;
extern GameState* g_pGameState;
extern int g_GamePartyMembers[MAX_PARTY_MEMBERS];

#endif