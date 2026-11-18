#include "common.h"
#include "main/game.h"
#include "field/main.h"
#include "field/actor.h"
#include "field/script_vm.h"
#include "field/text_box.h"

extern s32 g_FieldPartyActorSlots;
extern s32 D_8005A448;
extern s32 D_8005A44C;
extern s32 g_FieldCurActorIndex;

void FieldScriptVMHandlerWaitForTextBoxClose(void) {
    int nTextBoxIndex;
    u32 lowerFlags;
    u32 upperFlags;

    // Check if there's no visible textbox owned by current actor
    if (FieldTextBoxFindCurrentActorTextBox(&nTextBoxIndex) == -1) {
        g_FieldScriptMaxInstructionCount += 8;
        FieldScriptMemoryWriteU16(0x14, g_FieldScriptVMCurActor->dialogCursorPosition);
        g_FieldScriptVMCurActor->scriptInstructionPointer += 1;
        return;
    }
    
    if (g_FieldActors[g_FieldTextBoxes[nTextBoxIndex].talkingActorID].pActorData->flags & 0x200) {
        upperFlags = g_FieldScriptVMCurActor->dialogFlags >> 0x10;
        if (g_FieldScriptVMCurActor->dialogFlags >> 0x10 == 0) {
            lowerFlags = g_FieldScriptVMCurActor->dialogFlags & 0xFFFF;
        } else {
            lowerFlags = upperFlags & 0xFFFF;
        }
        
        if (!(lowerFlags & 0x1)) {
            if (g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].flags_0x12 != 7) {
                FieldScriptVMHandlerEndScript();
            }
            g_FieldTextBoxes[nTextBoxIndex].status = 0;
        }
    }
    g_FieldScriptVMStopFlag = 1;
}

// The text box keeps a SystemString at _pad[0]. system.c owns its full layout;
// this unit only needs the pending-text state bytes.
typedef struct {
    /* 0x00 */ undefined8 unk0[0x6C];
    /* 0x6C */ u8 unk6C;
    /* 0x6D */ undefined8 unk6D[0x17];
    /* 0x84 */ s16 unk84;
} FieldTextBoxString;

extern int SystemStringGetUnk6B(FieldTextBoxString* pString);
extern void SystemStringSetColor(FieldTextBoxString* pString, u8 red, u8 green, u8 blue);

#define TEXT_BOX_STRING(index) ((FieldTextBoxString*)g_FieldTextBoxes[index]._pad)

// Script command: lay out the cursor columns of the text box from the script byte.
void FieldScriptVMHandlerSetTextBoxCursorColumns(void) {
    int index;
    u8 columns;

    if (FieldTextBoxFindCurrentActorTextBox(&index) == 0) {
        g_FieldScriptMaxInstructionCount += 8;

        if (SystemStringGetUnk6B(TEXT_BOX_STRING(index)) == 1 ||
            (TEXT_BOX_STRING(index)->unk84 != 0 && TEXT_BOX_STRING(index)->unk6C != 0)) {
            g_FieldTextBoxes[index].cursor.visibility = 0;
            g_FieldScriptVMCurActor->dialogCursorPosition = 0xFF;
            columns = SCRIPT_READ_U8_REL(1);
            g_FieldTextBoxes[index].cursor.startPosition = columns >> 4;
            g_FieldTextBoxes[index].cursor.maxPosition = (SCRIPT_READ_U8_REL(1) & 0xF) - (columns >> 4) + 1;
            g_FieldTextBoxes[index].cursor.curPosition = 0;

            SystemStringSetColor(TEXT_BOX_STRING(index), 0xEF, 0x1E, 0xF0);
            g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
        }
    } else {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
    }

    g_FieldScriptVMStopFlag = 1;
}

s32 FieldCheckActorFacingCamera(void) {
    return ((g_FieldScriptVMCurActor->direction - (FieldGetCameraDirection() & 0xffff)) & MASK_8DIR_MOVEMENT_NUM_DIRECTIONS) < 5;
}

// Reset text box?
void FieldScriptVMHandlerCloseTextBoxOrClearDialogParams(void) {
    int index;
    
    if (SCRIPT_READ_U8_REL(1) == 0) {
        if (FieldTextBoxFindCurrentActorTextBox(&index) == 0) {
            g_FieldTextBoxes[index].status = 0;
            g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
        } else {
            g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
        }
    } else {
        g_FieldScriptVMCurActor->dialogPixelWidth = 0;
        g_FieldScriptVMCurActor->dialogPixelHeight = 0;
        g_FieldScriptVMCurActor->dialogWidth = 0;
        g_FieldScriptVMCurActor->dialogHeight = 0;
        g_FieldScriptVMCurActor->dialogFlags = 0;
        g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
    }
    g_FieldScriptVMStopFlag = 1;
}

// modifies the face id.. is this changing out the portrait?
void FieldScriptVMHandlerSetFaceIdFromActor(void) {
    if (FieldScriptVMGetActorIndex(1) != ACTOR_ID_INVALID) {
        g_FieldScriptVMCurActor->faceId = g_FieldActors[FieldScriptVMGetActorIndex(1)].pActorData->faceId;
        FieldScriptVMHandlerOpenTextBoxForActor();
        return;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 6;
}

void FieldScriptVMHandlerOpenTextBoxForActor(void) {
    u32 nActorIndex;

    if (FieldScriptVMGetActorIndex(1) != ACTOR_ID_INVALID) {
        nActorIndex = FieldScriptVMGetActorIndex(1);
        g_FieldScriptVMCurActor->scriptInstructionPointer++;
        if (func_8009C5A8(nActorIndex, 0) == -1) {
            g_FieldScriptVMCurActor->scriptInstructionPointer--;
        }
        return;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 6;
}

void FieldScriptVMHandlerOpenTextBoxMode0(void) {
    func_8009C5A8(g_FieldCurActorIndex, 0);
}

void FieldScriptVMHandlerOpenTextBoxMode1(void) {
    func_8009C5A8(g_FieldCurActorIndex, 1);
}

void FieldScriptVMHandlerOpenTextBoxMode2(void) {
    func_8009C5A8(g_FieldCurActorIndex, 2);
}

void FieldScriptVMHandlerOpenTextBoxMode3(void) {
    func_8009C5A8(g_FieldCurActorIndex, 3);
}

int FieldTextBoxCheckPortraitIdAvailable(int targetId);
extern s32 ArchiveCdDataSync(s32 arg0);
extern u32 ArchiveDecodeAlignedSize(u32 entryIndex);
extern void func_80029AFC(void* pRequest, s32 arg1, s32 arg2);
extern void FieldLoadTIMWithClut(u_long* pTimData, short x, short y, short clutX, short clutY,
                                 short clutWidth, short clutHeight);

// The text box can keep three face textures loaded at a time.
typedef struct {
    /* 0x0 */ s16 portraitId;
    /* 0x2 */ s16 state; // 0 = free, 1 = loading, 2 = loaded
    /* 0x4 */ s16 hasSecondTexture;
} FieldPortraitSlot; // Size: 0x6

// Where the two TIMs of a portrait go in VRAM; the rectangles are handed
// straight to FieldLoadTIMWithClut as x/y/clutX/clutY.
typedef struct {
    /* 0x0 */ s16 x;
    /* 0x2 */ s16 y;
    /* 0x4 */ s16 clutX;
    /* 0x6 */ s16 clutY;
    /* 0x8 */ s16 secondX;
    /* 0xA */ s16 secondY;
    /* 0xC */ s16 secondClutX;
    /* 0xE */ s16 secondClutY;
} FieldPortraitTIMRect; // Size: 0x10

// One archive read; the array of these handed to func_80029AFC ends with a zero entry.
typedef struct {
    /* 0x0 */ u16 size;
    /* 0x2 */ u16 pad2;
    /* 0x4 */ void* pBuffer;
} FieldArchiveRequest; // Size: 0x8

extern FieldPortraitSlot D_800B06A4[3];
extern FieldPortraitTIMRect D_800AEAE4[3];
extern FieldArchiveRequest D_800B00C8[];
extern u8 D_800AE1E0[]; // the two archive entry indices of each portrait face
extern u_long* D_800ADB10;
extern u_long* D_800ADB14;
extern s32 D_800ADB0C; // which portrait slot the next allocation starts from

// https://decomp.me/scratch/tL6mE
INCLUDE_ASM("asm/field/nonmatchings/dialogue/text_box", func_8009C154);
/* 81.50% (940 of 960 bytes; the structure is complete and in address order,
   every call, branch and store is in the right place, and the only missing
   instructions are the ADDRESS ADDS of the two FieldLoadTIMWithClut calls:
   retail builds `base + i * 0x10` from the table pointer THREE times for the
   first call (addu v0,s1,s0 / addu v1,s1,s0 / addu t0,s1,s0, loaded at 2/4/6)
   and four times for the second, plus one walking pointer (s4 = s1, advanced
   by 0x10) for the first argument of each call, where this body gets ONE addu
   and reads all four fields off it. `D_800AEAE4[i].x` (a symbolic base) instead
   gives four `lui $at` macro pairs, i.e. four extra instructions, so the
   pointer-local form is the closer of the two. Tried: struct array indexed,
   pointer local indexed (this one), pointer walker for the first field plus
   indexed for the rest (79.83%, 956 bytes - right size, worse allocation: gcc
   ran out of s0-s7 and used s8), and the embedded-assignment form
   (pRect = &pRects[i])->field three times (CSEd back to one addu). gcc 2.7.2
   CSEs the sum here; retail did not. This is the same shape that holds
   field/main/main.c FieldLoadUITextures at 87.32%, so it is a known wall in
   this repo - the rest of the function is ready to un-park if it is solved.
   Not a preset problem: gcc-2.7.2-cdk scores 65.16 on this function.
   Round 202 re-measured the wall and ruled out: the symbolic array form
   (D_800AEAE4[i].field, no pointer local) 77.27% / 1000 bytes, because gcc then
   reaches the table through a lui $at + addu $at,at,index pair per use instead
   of a base register; an unbounded extern array plus &D_800AEAE4[0] (same
   81.50%); a mem_addr base and the (u8*)pRects + i * 0x10 + K cast spelling
   (both the same 81.50%); the source-level walker for all eight fields 79.08% /
   956 bytes (spills to s8); walker for .x with the rest indexed 79.79% / 956;
   pRects assigned from &D_800AEAE4[0] inside the loop 82.47% / 928 bytes (the
   constant 2 stops being hoisted, but it is still a single walker). Presets on
   this body: -O3 81.5, -O1 62.2, gcc-2.6.0 82.47 (its matched neighbours drop
   to 98.93 and 78.82, so it cannot be a unit switch), cdk 69.0.
   Mechanism: retail keeps the table base in a loop register s1 AND a walker
   s4 = s1 for the offset-0 field, and gives every offset-2/4/6/8/A/C/E field
   its own addu reg,s1,s0; this gcc always combines those givs into the walker,
   which also frees the register it then spends hoisting the constant 2. Same
   wall as field/main/main.c FieldLoadUITextures (87.32%).
s32 func_8009C154(s32 faceId) {
    FieldPortraitTIMRect* pRects;
    u_long* pTimData;
    void* pBuffer;
    u8* pEntries;
    s32 entryIndex;
    s32 requestIndex;
    int found;
    int i;

    pRects = D_800AEAE4;
    for (i = 0; i < 3; i++) {
        if (D_800B06A4[i].state == 1) {
            if (ArchiveCdDataSync(1) != 0) {
                return -1;
            }

            D_800B06A4[i].state = 2;
            FieldLoadTIMWithClut(D_800ADB10, pRects[i].x, pRects[i].y, pRects[i].clutX,
                pRects[i].clutY, 0x100, 1);
            if (D_800B06A4[i].hasSecondTexture == 0) {
                pTimData = D_800ADB10;
            } else {
                pTimData = D_800ADB14;
            }
            FieldLoadTIMWithClut(pTimData, pRects[i].secondX, pRects[i].secondY,
                pRects[i].secondClutX, pRects[i].secondClutY, 0x100, 1);
            return -1;
        } else if (D_800B06A4[i].state == 2) {
            HeapFree(D_800ADB10);
            D_800B06A4[i].state = 0;
            if (D_800B06A4[i].hasSecondTexture == 1) {
                HeapFree(D_800ADB14);
            }
            return -1;
        }
    }

    for (i = 0; i < 3; i++) {
        if (D_800B06A4[i].portraitId == faceId) {
            g_FieldScriptVMCurActor->flags12C_0x2 = i;
            return 0;
        }
    }

    found = 0;
    for (i = 0; i < 3; i++) {
        D_800ADB0C++;
        if (D_800ADB0C >= 3) {
            D_800ADB0C = 0;
        }
        if (FieldTextBoxCheckPortraitIdAvailable(D_800B06A4[D_800ADB0C].portraitId) == 0) {
            found++;
            break;
        }
    }

    if (found == 0) {
        return -1;
    }

    g_FieldScriptVMCurActor->flags12C_0x2 = D_800ADB0C;
    ArchiveSetIndex(4, 0);
    pEntries = &D_800AE1E0[faceId * 2];
    D_800B06A4[D_800ADB0C].portraitId = faceId;
    D_800B06A4[D_800ADB0C].state = 1;
    D_800B06A4[D_800ADB0C].hasSecondTexture = 0;

    entryIndex = pEntries[0] + 0x46;
    D_800B00C8[0].size = entryIndex;
    pBuffer = HeapAlloc(ArchiveDecodeAlignedSize(entryIndex), 0);
    D_800ADB10 = pBuffer;
    D_800B00C8[0].pBuffer = pBuffer;

    requestIndex = 1;
    if (pEntries[1] != pEntries[0]) {
        entryIndex = pEntries[1] + 0x46;
        D_800B06A4[D_800ADB0C].hasSecondTexture = 1;
        D_800B00C8[1].size = entryIndex;
        pBuffer = HeapAlloc(ArchiveDecodeAlignedSize(entryIndex), 0);
        D_800ADB14 = pBuffer;
        D_800B00C8[1].pBuffer = pBuffer;
        requestIndex = 2;
    }

    D_800B00C8[requestIndex].size = 0;
    D_800B00C8[requestIndex].pBuffer = NULL;
    func_80029AFC(D_800B00C8, 0, 0);
    return -1;
}
*/

// Check if there is a portrait w/ targetId that's free to use?
int FieldTextBoxCheckPortraitIdAvailable(int targetId) {
    int i;
    
    for (i = 0; i < 4; i++) {
        if (g_FieldTextBoxes[i].visibility || g_FieldTextBoxes[i].portrait.shouldRenderPortrait != 1) {
            continue;
        }
        
        if (g_FieldTextBoxes[i].portrait.portraitID == targetId) {
            return -1;
        }
    }

    return 0;
}


INCLUDE_ASM("asm/field/nonmatchings/dialogue/text_box", func_8009C5A8);

extern u16 g_FieldTextBoxInUseMask[];

void FieldTextBoxMarkInUse(int arg0) {
    g_FieldTextBoxInUseMask[0] |= 1 << arg0;
}
// find_first_unused()?
int FieldTextBoxFindCurrentActorTextBox(int* pTextBoxIndex) {
    int i;

    for (i = 0; i < 4; i++) {
        if (g_FieldTextBoxes[i].ownerActorID == g_FieldCurActorIndex && g_FieldTextBoxes[i].visibility == 0) {
            *pTextBoxIndex = i;
            return 0;
        }
    }

    return -1;
}

u32 FieldScriptVMGetActorIndexOrDefault(int bytecodeOffset) {
    u32 nActorIndex = FieldScriptVMGetActorIndex(bytecodeOffset);
    if (nActorIndex == ACTOR_ID_INVALID) {
        return g_FieldPartyActorSlots;
    }
    return nActorIndex;
}

u32 FieldScriptVMGetActorIndex(int bytecodeOffset) {
    u32 actorID = SCRIPT_READ_U8_REL(bytecodeOffset);
    if (actorID == ACTOR_ID_INVALID) {
        actorID = g_FieldPartyActorSlots;  
    } else if (actorID == 0xFE) {
        actorID = D_8005A448;
    } else if (actorID == 0xFD) {
        actorID = D_8005A44C;
    } else if (actorID == 0xFB) {
        actorID = g_FieldCurActorIndex;
    }
    return actorID;
}


// Set up dialog window / text box
void FieldScriptVMHandlerSetTextBoxLayoutFromBytes(void) {
    g_FieldScriptVMCurActor->dialogPixelWidth = SCRIPT_READ_U8_REL(1) * 2;
    g_FieldScriptVMCurActor->dialogPixelHeight = SCRIPT_READ_U8_REL(2);
    g_FieldScriptVMCurActor->dialogWidth = SCRIPT_READ_U8_REL(3) * 3;
    g_FieldScriptVMCurActor->dialogHeight = SCRIPT_READ_U8_REL(4);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}


// Set up dialog window / text box
void FieldScriptVMHandlerSetTextBoxLayoutFromArguments(void) {
    g_FieldScriptVMCurActor->dialogPixelWidth = FieldScriptVMGetArgument(1);
    g_FieldScriptVMCurActor->dialogPixelHeight = FieldScriptVMGetArgument(3);
    g_FieldScriptVMCurActor->dialogWidth = FieldScriptVMGetArgument(5) * 3;
    g_FieldScriptVMCurActor->dialogHeight = FieldScriptVMGetArgument(7);
    g_FieldScriptVMCurActor->dialogFlags = FieldScriptVMGetArgument(9);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 0xB;
}
