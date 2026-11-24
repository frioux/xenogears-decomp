#include "common.h"

#include "main/game.h"
#include "system/archive.h"
#include "system/memory.h"
#include "system/menu.h"

// The menu resource table this function loads from the menus directory's entry 2:
// one pointer per file, a file offset until ResolveArchiveEntryPointers rewrites
// it. Every case of the switch below decompresses the entries of one menu panel
// into the panel buffer that owns them, and the matching "free" case (0x10 later)
// hands the same fields back to HeapFree.
typedef struct {
    /* 0x00 */ void* unk0;
    /* 0x04 */ void* pItemData; // → pDressingRoom->pItemsData
    /* 0x08 */ void* pWeaponsData; // → pDressingRoom->pWeaponsData
    /* 0x0C */ void* pAccessoriesData; // → pDressingRoom->pAccessoriesData
    /* 0x10 */ void* pCharacterSlots[MAX_GAME_CHARACTERS]; // [character id] → pDressingRoom->pCharacterSlots[]
    /* 0x3C */ void* pItemRowsVramBuffer; // → pItemRows->pVramBuffer
    /* 0x40 */ void* pColumnPanelStrings; // → the 0x430 panel's pStringData
    /* 0x44 */ void* pGearFrames; // → pDressingRoom->pGearFrames
    /* 0x48 */ void* pGearArmor; // → pDressingRoom->pGearArmor
    /* 0x4C */ void* pGearEngines; // → pDressingRoom->pGearEngines
    /* 0x50 */ void* pGearAccessories; // → pDressingRoom->pGearAccessories
    // The two gear-side column string lists: resource set 5 and 6 do the same work
    // and differ only in which of these two becomes the 0x430 panel's pStringData.
    /* 0x54 */ void* pGearColumnStrings;
    /* 0x58 */ void* pGearColumnStrings2;
    /* 0x5C */ void* pGearBlocks[MAX_GAME_GEARS]; // [gear id] → pDressingRoom->pGearBlocks[]
    /* 0xAC */ void* pGearWeapons; // → pDressingRoom->pGearWeapons
    /* 0xB0 */ void* pStatTable; // → the 0x438 row block's pStatTable
    /* 0xB4 */ void* unkB4[8];
    /* 0xD4 */ void* pSpriteStringLists[4]; // → pCharacterSpriteRecords->pStringLists[]
} MenuResourceTable; // Size: 0xE4

// The two panel buffers this function fills, as far as it reaches them: misc9
// declares the whole 0x1094-byte column panel and misc10 the whole 0x25C0-byte
// row block, so only the field each one stores into is repeated here.
typedef struct {
    /* 0x0000 */ u8 unk0[0x1080];
    /* 0x1080 */ void* pStringData; // String list the column headings are fetched from
} MenuColumnPanelStrings; // Size: 0x1084

typedef struct {
    /* 0x0000 */ u8 unk0[0x2578];
    /* 0x2578 */ void* pStatTable; // Stat values per character and row, see func_801E1418
} MenuRowBlockStatTable; // Size: 0x257C

// The menus archive directory's resource table entry: the file of pointers the
// switch below decompresses one panel at a time.
#define MENU_RESOURCE_TABLE_ENTRY 2
// Resource set ids split in two: an id below this loads a menu panel's files into
// its buffers, the id MENU_RESOURCE_SET_FREE_BASE above it hands the same fields
// back to HeapFree.
#define MENU_RESOURCE_SET_FREE_BASE 0x10

extern int ArchiveDecodeAlignedSize(int entryIndex);
extern void ArchiveReadFileToBuffer(s32 index, void* pBuffer, u32 arg2, u32 flags);
extern void ArchiveCdDataSync(int arg0);
extern void ResolveArchiveEntryPointers(u32*);
extern void* LZSSHeapDecompress(void* pCompressedData, int flags);
extern void MenuSetGearHpScaled(MenuDressingRoom* pDressingRoom, u8 gearId);

// The gear id the character in party slot `slot` rides. Retail re-derives it at
// every use in the gear-block loops below (it is spent across a
// LZSSHeapDecompress call), so it is written as an expression and not a local.
#define MenuGetSlotGearId(slot) g_GameState.characters[g_Menu->pManager->currentCharacterIDs[slot]].gearId

void func_801C72BC(u8 index) {
    MenuResourceTable* pTable;
    s32 i;

    if (index < MENU_RESOURCE_SET_FREE_BASE) {
        ArchiveSetIndex(ARCHIVE_DIR_MENUS, 0);
        pTable = HeapAlloc(ArchiveDecodeAlignedSize(MENU_RESOURCE_TABLE_ENTRY), 1);
        ArchiveReadFileToBuffer(MENU_RESOURCE_TABLE_ENTRY, pTable, 0, 0x80);
        ArchiveCdDataSync(0);
        ResolveArchiveEntryPointers((u32*)pTable);
    }
    switch (index) {
        case 0:
            g_Menu->pDressingRoom->pItemsData = LZSSHeapDecompress(pTable->pItemData, 0);
            g_Menu->pItemRows->pVramBuffer = LZSSHeapDecompress(pTable->pItemRowsVramBuffer, 0);
            break;
        case 1:
            g_Menu->pDressingRoom->pGearFrames = LZSSHeapDecompress(pTable->pGearFrames, 0);
            g_Menu->pDressingRoom->pGearArmor = LZSSHeapDecompress(pTable->pGearArmor, 0);
            g_Menu->pDressingRoom->pGearEngines = LZSSHeapDecompress(pTable->pGearEngines, 0);
            g_Menu->pDressingRoom->pGearAccessories = LZSSHeapDecompress(pTable->pGearAccessories, 0);
            break;
        case 2:
            for (i = 0; i < MAX_PARTY_MEMBERS; i++) {
                if (g_Menu->pManager->currentCharacterIDs[i] != CHARACTER_ID_NONE) {
                    g_Menu->pDressingRoom->pCharacterSlots[g_Menu->pManager->currentCharacterIDs[i]] =
                        LZSSHeapDecompress(pTable->pCharacterSlots[g_Menu->pManager->currentCharacterIDs[i]], 0);
                }
            }
            ((MenuColumnPanelStrings*)g_Menu->unk430)->pStringData =
                LZSSHeapDecompress(pTable->pColumnPanelStrings, 0);
            break;
        case 3:
            g_Menu->pDressingRoom->pWeaponsData = LZSSHeapDecompress(pTable->pWeaponsData, 0);
            g_Menu->pDressingRoom->pAccessoriesData = LZSSHeapDecompress(pTable->pAccessoriesData, 0);
            g_Menu->pDressingRoom->pGearWeapons = LZSSHeapDecompress(pTable->pGearWeapons, 0);
            g_Menu->pDressingRoom->pGearAccessories = LZSSHeapDecompress(pTable->pGearAccessories, 0);
            break;
        case 4:
            for (i = 0; i < MAX_PARTY_MEMBERS; i++) {
                if (g_Menu->pManager->currentCharacterIDs[i] != CHARACTER_ID_NONE) {
                    g_Menu->pDressingRoom->pCharacterSlots[g_Menu->pManager->currentCharacterIDs[i]] =
                        LZSSHeapDecompress(pTable->pCharacterSlots[g_Menu->pManager->currentCharacterIDs[i]], 0);
                }
            }
            ((MenuRowBlockStatTable*)g_Menu->unk438)->pStatTable = LZSSHeapDecompress(pTable->pStatTable, 0);
            break;
        case 5:
        case 6:
            for (i = 0; i < MAX_PARTY_MEMBERS; i++) {
                if (g_Menu->pManager->currentCharacterIDs[i] != CHARACTER_ID_NONE) {
                    if (MenuGetSlotGearId(i) != GEAR_ID_NONE) {
                        g_Menu->pDressingRoom->pGearBlocks[MenuGetSlotGearId(i)] =
                            LZSSHeapDecompress(pTable->pGearBlocks[MenuGetSlotGearId(i)], 0);
                        MenuSetGearHpScaled(g_Menu->pDressingRoom, MenuGetSlotGearId(i));
                    }
                }
            }
            if (index == 5) {
                ((MenuColumnPanelStrings*)g_Menu->unk430)->pStringData =
                    LZSSHeapDecompress(pTable->pGearColumnStrings, 0);
            } else {
                ((MenuColumnPanelStrings*)g_Menu->unk430)->pStringData =
                    LZSSHeapDecompress(pTable->pGearColumnStrings2, 0);
            }
            break;
        case 7:
            g_Menu->pCharacterSpriteRecords->pStringLists[0] =
                LZSSHeapDecompress(pTable->pSpriteStringLists[0], 0);
            g_Menu->pCharacterSpriteRecords->pStringLists[1] =
                LZSSHeapDecompress(pTable->pSpriteStringLists[1], 0);
            g_Menu->pCharacterSpriteRecords->pStringLists[2] =
                LZSSHeapDecompress(pTable->pSpriteStringLists[2], 0);
            g_Menu->pCharacterSpriteRecords->pStringLists[3] =
                LZSSHeapDecompress(pTable->pSpriteStringLists[3], 0);
            break;
        case 16:
            HeapFree(g_Menu->pDressingRoom->pItemsData);
            HeapFree(g_Menu->pItemRows->pVramBuffer);
            break;
        case 17:
            HeapFree(g_Menu->pDressingRoom->pGearFrames);
            HeapFree(g_Menu->pDressingRoom->pGearArmor);
            HeapFree(g_Menu->pDressingRoom->pGearEngines);
            HeapFree(g_Menu->pDressingRoom->pGearAccessories);
            break;
        case 18:
            for (i = 0; i < MAX_PARTY_MEMBERS; i++) {
                if (g_Menu->pManager->currentCharacterIDs[i] != CHARACTER_ID_NONE) {
                    HeapFree(g_Menu->pDressingRoom->pCharacterSlots[g_Menu->pManager->currentCharacterIDs[i]]);
                }
            }
            HeapFree(((MenuColumnPanelStrings*)g_Menu->unk430)->pStringData);
            break;
        case 19:
            HeapFree(g_Menu->pDressingRoom->pWeaponsData);
            HeapFree(g_Menu->pDressingRoom->pAccessoriesData);
            HeapFree(g_Menu->pDressingRoom->pGearWeapons);
            HeapFree(g_Menu->pDressingRoom->pGearAccessories);
            break;
        case 20:
            for (i = 0; i < MAX_PARTY_MEMBERS; i++) {
                if (g_Menu->pManager->currentCharacterIDs[i] != CHARACTER_ID_NONE) {
                    HeapFree(g_Menu->pDressingRoom->pCharacterSlots[g_Menu->pManager->currentCharacterIDs[i]]);
                }
            }
            HeapFree(((MenuRowBlockStatTable*)g_Menu->unk438)->pStatTable);
            break;
        case 21:
        case 22:
            for (i = 0; i < MAX_PARTY_MEMBERS; i++) {
                if (g_Menu->pManager->currentCharacterIDs[i] != CHARACTER_ID_NONE) {
                    if (MenuGetSlotGearId(i) != GEAR_ID_NONE) {
                        HeapFree(g_Menu->pDressingRoom->pGearBlocks[MenuGetSlotGearId(i)]);
                    }
                }
            }
            HeapFree(((MenuColumnPanelStrings*)g_Menu->unk430)->pStringData);
            break;
        case 23:
            HeapFree(g_Menu->pCharacterSpriteRecords->pStringLists[0]);
            HeapFree(g_Menu->pCharacterSpriteRecords->pStringLists[1]);
            HeapFree(g_Menu->pCharacterSpriteRecords->pStringLists[2]);
            HeapFree(g_Menu->pCharacterSpriteRecords->pStringLists[3]);
            break;
        default:
            break;
    }
    if (index < MENU_RESOURCE_SET_FREE_BASE) {
        HeapFree(pTable);
    }
}
