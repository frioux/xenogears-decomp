#ifndef _XENO_GAME_H
#define _XENO_GAME_H

#define MAX_GAME_CHARACTERS 11
#define MAX_GAME_GEARS 20
#define MAX_PARTY_MEMBERS 3
#define MAX_BENCHED_PARTY_MEMBERS 6
#define CHARACTER_ID_NONE 0xFF
#define GEAR_ID_NONE 0xFF

#define MAX_ITEM_QUANTITY 99
#define MAX_INVENTORY_WEAPONS 100
#define MAX_INVENTORY_ACCESSORIES 200
#define MAX_INVENTORY_ITEMS 150

#define MAX_GOLD_AMOUNT 9999999


// One of the four 0x8-byte records at the head of a GameCharacter: the cached
// stats of one equipped weapon at +0x0..+0x4, written by
// MenuApplyEquippedItemBonuses, and, in g_GameState.characters[4], the id of the
// character in that battle party slot at +0x6 (see BattleIsCharacterInParty).
typedef struct {
    /* 0x0 */ u16 flags; // OR'ed into unk8E while unk3 reads 0x64 (fully charged)
    /* 0x2 */ u8 unk2;
    /* 0x3 */ u8 unk3;
    /* 0x4 */ u8 attack;
    /* 0x5 */ u8 unk5;
    /* 0x6 */ u8 characterId;
    /* 0x7 */ u8 unk7;
} GameCharacterSlot; // Size: 0x8

typedef struct {
    /* 0x00 */ GameCharacterSlot slots[4];
    /* 0x20 */ u8 unk20[8];
    /* 0x28 */ u8 attackFromEquipment;
    /* 0x29 */ u8 defenseFromEquipment;
    /* 0x2A */ u8 agilityFromEquipment;
    /* 0x2B */ u8 etherFromEquipment;
    /* 0x2C */ u8 etherDefenseFromEquipment;
    /* 0x2D */ u8 defenseFromAccessories; // total of the three accessories' defense field
    /* 0x2E */ u8 hitPercentageFromEquipment;
    /* 0x2F */ u8 evadePercentageFromEquipment;
    /* 0x30 */ u8 unk30;
    /* 0x31 */ u8 unk31;
    /* 0x32 */ u16 unk32;
    /* 0x34 */ u8 unk34[0x10];
    // Two 32-bit numbers the menu window draws wide (up to 8 digits) with
    // MenuParseNumberToDigits.
    /* 0x44 */ s32 unk44;
    /* 0x48 */ s32 unk48;
    /* 0x4C */ u_short hp;
    /* 0x4E */ u_short maxHp;
    /* 0x50 */ u_short mp;
    /* 0x52 */ u_short maxMp;
    /* 0x54 */ u8 unk54[2];
    /* 0x56 */ u8 unk56;
    /* 0x57 */ u8 unk57;
    /* 0x58 */ u_char attack;
    /* 0x59 */ u_char defense;
    /* 0x5A */ u_char agility;
    /* 0x5B */ u_char ether;
    /* 0x5C */ u_char etherDefense;
    /* 0x5D */ undefined8 field_0x5D;
    /* 0x5E */ u_char hitPercentage;
    /* 0x5F */ u_char evadePercentage;
    /* 0x60 */ undefined16 field_0x60;
    /* 0x62 */ u_char level;
    /* 0x63 */ undefined8 unk63;
    /* 0x64 */ undefined8 unk64[0x6];
    /* 0x6A */ undefined8 weaponId[0x5];
    /* 0x6F */ undefined8 extraWeaponIds[0x5]; // Billy's four guns are summed, so all four slots are weapons
    /* 0x74 */ undefined8 accessorySlots[0x3];
    /* 0x77 */ u8 unk77;
    /* 0x78 */ u8 unk78;
    /* 0x79 */ u8 unk79[0x5];
    /* 0x7E */ u16 unk7E; // accessory flag word, OR'ed from the slot[0] flags
    /* 0x80 */ u16 unk80;
    /* 0x82 */ u16 unk82;
    /* 0x84 */ u16 unk84;
    /* 0x86 */ u16 unk86;
    /* 0x88 */ u16 unk88;
    /* 0x8A */ u16 unk8A;
    /* 0x8C */ u16 unk8C;
    /* 0x8E */ u16 unk8E;
    /* 0x90 */ u8 unk90[0x10];
    /* 0xA0 */ u_char gearId;
    /* 0xA1 */ undefined8 field_0xA1[0x3];
} GameCharacter; // size: 0xA4

typedef struct {
    /* 0x00 */ u16 flags; // OR'ed into GameGear.unk86's top nibble once unk4 reads 100, see MenuApplyGearWeapons
    /* 0x02 */ u8 attack;
    /* 0x03 */ u8 unk3;
    /* 0x04 */ u8 unk4;
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
    /* 0x3D */ u8 unk3D;
    /* 0x3E */ u8 unk3E;
    /* 0x3F */ u8 unk3F;
    /* 0x40 */ u16 defenseFromEquipment; // total of defense boosts from slot1 + slot 2 + slot 3
    /* 0x42 */ u16 etherDefenseFromEquipment;
    /* 0x44 */ u16 weightFromEquipment;
    /* 0x46 */ u16 unk46;
    /* 0x48 */ u16 unk48;
    /* 0x4A */ u8 weightPenalty; // subtracted from agility, see GearShopMenuComputeEquipWeightPenalty
    /* 0x4B */ u8 unk4B;
    /* 0x4C */ u8 unk4C;
    /* 0x4D */ u8 responsiveness;
    /* 0x4E */ u8 unk4E;
    /* 0x4F */ u8 unk4F;
    /* 0x50 */ u8 unk50[4];
    /* 0x54 */ u8 etherAmplificationFromEquipment; // added to etherAmplification for the total shown
    /* 0x55 */ u8 unk55[3];
    /* 0x58 */ u16 unk58;
    /* 0x5A */ u16 unk5A;
    /* 0x5C */ u8 unk5C[4];
    /* 0x60 */ u32 hp;
    /* 0x64 */ u32 maxHp;
    /* 0x68 */ u16 weight;
    /* 0x6A */ u16 unk6A;
    /* 0x6C */ u16 unk6C;
    /* 0x6E */ u16 unk6E;
    /* 0x70 */ u16 baseDefense; /* impacted by AR value of equipped armor */
    /* 0x72 */ u16 baseEtherDefense;
    /* 0x74 */ u8 attackPower;
    /* 0x75 */ u8 unk75;
    /* 0x76 */ u8 unk76;
    /* 0x77 */ u8 unk77;
    /* 0x78 */ u8 unk78;
    /* 0x79 */ u8 unk79;
    /* 0x7A */ u8 unk7A;
    /* 0x7B */ u8 unk7B;
    /* 0x7C */ u8 unk7C;
    /* 0x7D */ u8 unk7D;
    /* 0x7E */ u16 unk7E;
    /* 0x80 */ u8 unk80;
    /* 0x81 */ u8 unk81;
    /* 0x82 */ u16 unk82;
    /* 0x84 */ u8 unk84;
    /* 0x85 */ u8 unk85;
    /* 0x86 */ u16 unk86;
    /* 0x88 */ u8 unk88[0x10];
    /* 0x98 */ u8 agility;
    /* 0x99 */ u8 unk99;
    /* 0x9a */ u8 unk9A;
    /* 0x9b */ u8 unk9B;
    /* 0x9c */ u8 unk9C;
    /* 0x9d */ u8 unk9D;
    /* 0x9e */ u8 etherAmplification;
    /* 0x9f */ u8 responsivenessFromFrame;
    /* 0xA0 */ int unkA0;
} GameGear; // size: 0xA4

typedef struct {
    /* 0x00 */ int unk0;
    /* 0x04 */ int unk4;
    /* 0x08 */ int unk8;
    /* 0x0C */ int unkC;
    /* 0x10 */ int unk10;
} StringEntry; /* size: 0x14 */

typedef struct {
    /* 0x00 */ s16 unk0;
    /* 0x02 */ s16 unk2;
    /* 0x04 */ s16 unk4;
    /* 0x06 */ s16 unk6;
    /* 0x08 */ s16 unk8;
    /* 0x0A */ s16 unkA;
    /* 0x0C */ s16 unkC;
    /* 0x0E */ s16 unkE;
    /* 0x10 */ u8 unk10;
    /* 0x11 */ u8 unk11;
    /* 0x12 */ s16 unk12;
    /* 0x14 */ s16 unk14;
    /* 0x16 */ s16 unk16;
    /* 0x18 */ s16 unk18;
    /* 0x1A */ s16 unk1A;
    /* 0x1C */ s16 unk1C;
    /* 0x1E */ s16 statusFlags; // status bit mask, tested through BattleMaskHasStatusBit
} UnkGearState; /* size: 0x20 */

typedef struct {
    /* 0x0000 */ StringEntry strings[31];
    /* 0x026C */ GameCharacter characters[MAX_GAME_CHARACTERS];
    /* 0x0978 */ GameGear gears[MAX_GAME_GEARS];
    /* 0x1648 */ undefined8 unk1648[0x74];
    /* 0x16BC */ UnkGearState unk16BC[10];
    /* 0x17FC */ undefined8 unk17FC[0x24];
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
    /* 0x1836 */ u16 unk1836;
    /* 0x1838 */ undefined8 unk1838[0x16];
    /* 0x184E */ undefined16 unk184E;
    /* 0x1850 */ undefined16 unk1850;
    /* 0x1852 */ undefined16 unk1852;
    /* 0x1854 */ undefined16 unk1854;
    /* 0x1856 */ undefined16 unk1856;
    /* 0x1858 */ undefined8 unk1858[0xCC];
    /* 0x1924 */ u_int gold;
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
    /* 0x22B0 */ undefined8 unk22B0;
    /* 0x22B1 */ undefined8 gearRide[MAX_PARTY_MEMBERS];
    /* 0x22B4 */ undefined8 unk22B4[0x2];
    /* 0x22B6 */ undefined16 unk22B6;
    /* 0x22B8 */ undefined8 unk22B8[0x62];
    /* 0x231A */ u16 mapNum;
    /* 0x231C */ u16 unk231C;
    /* 0x231E */ undefined16 unk231E;
    /* 0x2320 */ u16 unk2320;
    /* 0x2322 */ u16 wdsBankIndex;
    /* 0x2324 */
} GameState; // size: unknown

extern GameState g_GameState;
extern GameState* g_pGameState;
extern int g_GamePartyMembers[MAX_PARTY_MEMBERS];

#endif