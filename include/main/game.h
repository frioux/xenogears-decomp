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


typedef struct {
    /* 0x0  */ u8 unk0[0x2C];
    /* 0x2C */ u8 etherDefenseFromEquipment;
    /* 0x2D  */ undefined8 unk2D[0x1F];
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
    /* 0x6A */ undefined8 unk6A[0x5]; // Equipped weapons?
    /* 0x6F */ undefined8 unk6F[0x5]; // Equipped weapons?    
    /* 0x74 */ undefined8 unk74[0x3]; // Equipped accessories?
    /* 0x77 */ undefined8 unk77;    
    /* 0x78 */ undefined8 unk78[0x28];
    /* 0xA0 */ u_char gearId;
    /* 0xA1 */ undefined8 field_0xA1[0x3];
} GameCharacter; // size: 0xA4

typedef struct {
    /* 0x00 */ u16 unk0;
    /* 0x02 */ u8 unk2;
    /* 0x03 */ u8 unk3;
    /* 0x04 */ u8 unk4;
    /* 0x05 */ u8 unk5;
    /* 0x06 */ u16 unk6;
} GearWeaponData;

typedef struct {
    /* 0x00 */ u8 unk0;
    /* 0x01 */ u8 unk1;
    /* 0x02 */ u8 frameId;
    /* 0x03 */ u8 engineId;
    /* 0x04 */ u8 unk4[4];
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
    /* 0x4A */ u8 unk4A;
    /* 0x4B */ u8 unk4B;
    /* 0x4C */ u8 unk4C;
    /* 0x4D */ u8 responsiveness;
    /* 0x4E */ u8 unk4E;
    /* 0x4F */ u8 unk4F;
    /* 0x50 */ u8 unk50[4];
    /* 0x54 */ u8 unk54;
    /* 0x56 */ u8 unk55[3];
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
    /* 0x76 */ u16 unk7E;
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
    /* 0x9f */ u8 unk9F;
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
    /* 0x1E */ s16 unk1E;
} UnkGearState; /* size: 0x20 */

typedef struct {
    /* 0x0000 */ StringEntry strings[31];
    /* 0x026C */ GameCharacter characters[MAX_GAME_CHARACTERS];
    /* 0x0978 */ GameGear gears[MAX_GAME_GEARS];
    /* 0x1648 */ undefined8 unk1648[0x74];
    /* 0x16BC */ UnkGearState unk16BC[10];
    /* 0x17FC */ undefined8 unk17FC[0x52];
    /* 0x184E */ undefined16 unk184E;
    /* 0x1850 */ undefined8 unk1850[0x2];
    /* 0x1852 */ undefined16 unk1852;
    /* 0x1854 */ undefined8 unk1854[0xD0];
    /* 0x1924 */ u_int gold;
    /* 0x1928 */ undefined8 unk1928[0x408];
    /* 0x1D30 */ undefined16 unk1D30;
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
    /* 0x22B8 */
} GameState; // size: unknown

extern GameState g_GameState;
extern GameState* g_pGameState;
extern int g_GamePartyMembers[MAX_PARTY_MEMBERS];

#endif