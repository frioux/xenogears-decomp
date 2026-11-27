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
        if (FieldTextBoxOpen(nActorIndex, 0) == -1) {
            g_FieldScriptVMCurActor->scriptInstructionPointer--;
        }
        return;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 6;
}

void FieldScriptVMHandlerOpenTextBoxMode0(void) {
    FieldTextBoxOpen(g_FieldCurActorIndex, 0);
}

void FieldScriptVMHandlerOpenTextBoxMode1(void) {
    FieldTextBoxOpen(g_FieldCurActorIndex, 1);
}

void FieldScriptVMHandlerOpenTextBoxMode2(void) {
    FieldTextBoxOpen(g_FieldCurActorIndex, 2);
}

void FieldScriptVMHandlerOpenTextBoxMode3(void) {
    FieldTextBoxOpen(g_FieldCurActorIndex, 3);
}

int FieldTextBoxCheckPortraitIdAvailable(int targetId);
extern s32 ArchiveCdDataSync(s32 arg0);
extern u32 ArchiveDecodeAlignedSize(u32 entryIndex);
extern void ArchiveStartStreamQueue(void* pRequest, s32 arg1, s32 arg2);
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

// One archive read; the array of these handed to ArchiveStartStreamQueue ends with a zero entry.
typedef struct {
    /* 0x0 */ u16 size;
    /* 0x2 */ u16 pad2;
    /* 0x4 */ void* pBuffer;
} FieldArchiveRequest; // Size: 0x8

extern FieldPortraitSlot g_FieldPortraitSlots[3];
extern FieldPortraitTIMRect D_800AEAE4[3];
extern FieldArchiveRequest D_800B00C8[];
extern u8 D_800AE1E0[]; // the two archive entry indices of each portrait face
extern u_long* D_800ADB10;
extern u_long* D_800ADB14;
extern s32 g_FieldPortraitSlotCursor; // which portrait slot the next allocation starts from

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
   Round 374 re-measured 81.50 (940 of 960) and added one fact about WHERE the
   three addu come from: retail's `sll $s0,$s2,4` (i * 0x10) is CSEd into ONE
   register and the three `addu reg,s1,s0` in each call are created AFTER cse,
   by loop.c's emit_iv_add_mult - it decided the sum `pRects + i * 0x10` is not
   worth a register (short live range, big loop body) and recomputed it at every
   use, while this gcc keeps it. So the source cannot spell them: the walker
   form (pRect->x for the first field only) was re-measured at 79.71 / 956
   bytes, which is the note's 79.79 - it buys the walker (s4 = s1, +0x10 in the
   loop tail) but the other three fields still read off one register, and it
   costs the s8 spill. Nothing else to try from source short of loop.c's
   register-benefit model, so it stays a park.
   Round 534: re-measured 81.50 (940 of 960) and -G0 81.50 (so the table base is
   a pointer either way, not a gp question). The 20 missing bytes are NOT the two
   calls alone: retail also re-forms &D_800AE1E0 + 1 as `addu a0,s1,s0` off a
   `base + 1` register where this body reads `lbu a1,1(s0)` off the pEntries
   pointer, and spells every D_800B00C8 store through `lui $at`, while this body
   keeps that base in a register and hands it to ArchiveStartStreamQueue as
   `move a0,s2`. So the whole residual is ONE phenomenon: this cc1 keeps bases in
   registers that retail's rebuilds (7 addu + the walker's init for the two
   calls, 1 for the pEntries re-form, the rest around the D_800B00C8 and
   D_800AE1E0 bases). Same class as field/main/main.c FieldLoadUITextures;
   levers.md a-duplicated-source-level-sum-is-folded-before-loop-c-sees-it
   (round 530) proved -fno-rerun-cse-after-loop and -fno-strength-reduce both
   leave that sibling byte-identical, i.e. source-unreachable.
*/
/*
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
        if (g_FieldPortraitSlots[i].state == 1) {
            if (ArchiveCdDataSync(1) != 0) {
                return -1;
            }

            g_FieldPortraitSlots[i].state = 2;
            FieldLoadTIMWithClut(D_800ADB10, pRects[i].x, pRects[i].y, pRects[i].clutX,
                pRects[i].clutY, 0x100, 1);
            if (g_FieldPortraitSlots[i].hasSecondTexture == 0) {
                pTimData = D_800ADB10;
            } else {
                pTimData = D_800ADB14;
            }
            FieldLoadTIMWithClut(pTimData, pRects[i].secondX, pRects[i].secondY,
                pRects[i].secondClutX, pRects[i].secondClutY, 0x100, 1);
            return -1;
        } else if (g_FieldPortraitSlots[i].state == 2) {
            HeapFree(D_800ADB10);
            g_FieldPortraitSlots[i].state = 0;
            if (g_FieldPortraitSlots[i].hasSecondTexture == 1) {
                HeapFree(D_800ADB14);
            }
            return -1;
        }
    }

    for (i = 0; i < 3; i++) {
        if (g_FieldPortraitSlots[i].portraitId == faceId) {
            g_FieldScriptVMCurActor->flags12C_0x2 = i;
            return 0;
        }
    }

    found = 0;
    for (i = 0; i < 3; i++) {
        g_FieldPortraitSlotCursor++;
        if (g_FieldPortraitSlotCursor >= 3) {
            g_FieldPortraitSlotCursor = 0;
        }
        if (FieldTextBoxCheckPortraitIdAvailable(g_FieldPortraitSlots[g_FieldPortraitSlotCursor].portraitId) == 0) {
            found++;
            break;
        }
    }

    if (found == 0) {
        return -1;
    }

    g_FieldScriptVMCurActor->flags12C_0x2 = g_FieldPortraitSlotCursor;
    ArchiveSetIndex(4, 0);
    pEntries = &D_800AE1E0[faceId * 2];
    g_FieldPortraitSlots[g_FieldPortraitSlotCursor].portraitId = faceId;
    g_FieldPortraitSlots[g_FieldPortraitSlotCursor].state = 1;
    g_FieldPortraitSlots[g_FieldPortraitSlotCursor].hasSecondTexture = 0;

    entryIndex = pEntries[0] + 0x46;
    D_800B00C8[0].size = entryIndex;
    pBuffer = HeapAlloc(ArchiveDecodeAlignedSize(entryIndex), 0);
    D_800ADB10 = pBuffer;
    D_800B00C8[0].pBuffer = pBuffer;

    requestIndex = 1;
    if (pEntries[1] != pEntries[0]) {
        entryIndex = pEntries[1] + 0x46;
        g_FieldPortraitSlots[g_FieldPortraitSlotCursor].hasSecondTexture = 1;
        D_800B00C8[1].size = entryIndex;
        pBuffer = HeapAlloc(ArchiveDecodeAlignedSize(entryIndex), 0);
        D_800ADB14 = pBuffer;
        D_800B00C8[1].pBuffer = pBuffer;
        requestIndex = 2;
    }

    D_800B00C8[requestIndex].size = 0;
    D_800B00C8[requestIndex].pBuffer = NULL;
    ArchiveStartStreamQueue(D_800B00C8, 0, 0);
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


extern s32 g_FieldArchiveStreamActive;
extern s32 g_FieldRequestCode;
extern s32 g_FieldStreamSceneActive;
extern s32 g_FieldActive;
extern s32 D_800C4268;

// The dialog data the two text size lookups read; system.c owns its layout.
typedef struct FieldDialogData FieldDialogData;

extern FieldDialogData* g_FieldDialogs;
extern int DialogGetWidth(FieldDialogData* pDialogData, int dialogIndex);
extern int DialogGetHeight(FieldDialogData* pDialogData, int dialogIndex);
extern s32 FieldArchiveTryDataSync(void);
extern s32 func_8009C154(s32 faceId);
extern int FieldTextBoxHasFreeSlot(void);
extern int FieldTextBoxGetBottomIndex(void);
extern int FieldTextBoxAllocateTop(void);
extern void FieldTextBoxMarkInUse(int textBoxIndex);
extern void FieldProjectActorPointToScreen(int actorIndex, int* pScreenX, int* pScreenY, int height);
extern void func_8007F8DC(short x, short y, u16 dialogIndex, s32 textBoxIndex, s32 width,
                          s32 height, s32 curActorIndex, s32 actorIndex, s32 mode,
                          s32 flags, u32 faceFlags);

// Text box flag bits, see g_FieldTextBoxes[].flags in include/field/text_box.h.
#define FIELD_TEXT_BOX_FLAG_TOP 0x1
#define FIELD_TEXT_BOX_FLAG_NO_BORDER 0x40
#define FIELD_TEXT_BOX_FLAG_BOTTOM 0x80

// Where the window ends up relative to the actor, from the two bits the script
// byte at +4 keeps in its nibble.
#define FIELD_TEXT_BOX_POSITION_FROM_ACTOR 0
#define FIELD_TEXT_BOX_POSITION_ABOVE 1
#define FIELD_TEXT_BOX_POSITION_BELOW 2

// Opens the text box a script instruction asked for: picks the slot, sizes the
// window from the dialog string and the instruction's mode, and records the
// layout in the text box table. Returns 0 when a box was opened, -1 otherwise.
s32 FieldTextBoxOpen(s32 actorIndex, s32 mode) {
    s32 foundIndex;
    s32 textBoxIndex;
    s32 x;
    s32 screenY;
    s32 left;
    s32 y;
    s32 projectedY;
    s32 width;
    s32 height;
    s32 visibleCount;
    s32 flagsUnion;
    s32 i;
    u32 dialogFlags;
    u32 faceFlags;
    s32 flags;
    u16 dialogIndex;

    g_FieldScriptMaxInstructionCount += 0x20;

    if (g_FieldArchiveStreamActive != 0 || g_FieldActive != 0 || D_800C4268 != 0 ||
        g_FieldRequestCode != 0xFF ||
        (g_FieldStreamSceneActive == 0 && FieldArchiveTryDataSync() != 0)) {
        g_FieldScriptVMStopFlag = 1;
        return -1;
    }

    if (g_FieldScriptVMCurActor->faceId != 0xFF && func_8009C154(g_FieldScriptVMCurActor->faceId) == -1) {
        g_FieldScriptVMStopFlag = 1;
        return -1;
    }

    D_800C4268++;

    if (FieldTextBoxFindCurrentActorTextBox(&foundIndex) != -1) {
        goto textBoxOpen;
    }

    g_FieldScriptMaxInstructionCount += 8;
    dialogIndex = FieldScriptVMGetInstructionArgument(1);

    if (FieldTextBoxHasFreeSlot() == 0) {
        textBoxIndex = FieldTextBoxAllocateTop();
    } else {
        textBoxIndex = FieldTextBoxGetBottomIndex();
        if (textBoxIndex != TEXT_BOX_UNINITIALIZED) {
            g_FieldTextBoxes[textBoxIndex].status = 0;
            g_FieldScriptVMStopFlag = 1;
            return -1;
        }
    }

    i = 0;
    visibleCount = 0;
    flagsUnion = 0;
    for (; i < 4; i++) {
        if (g_FieldTextBoxes[i].visibility == 0) {
            visibleCount++;
            flagsUnion |= g_FieldTextBoxes[i].flags;
        }
    }

    width = DialogGetWidth(g_FieldDialogs, dialogIndex);
    height = DialogGetHeight(g_FieldDialogs, dialogIndex);

    if (mode == 0 || mode == 3) {
        if (g_FieldScriptVMCurActor->dialogWidth != 0) {
            width = g_FieldScriptVMCurActor->dialogWidth;
        }
        if (g_FieldScriptVMCurActor->dialogHeight != 0) {
            height = g_FieldScriptVMCurActor->dialogHeight;
        }
    }

    dialogFlags = g_FieldScriptVMCurActor->dialogFlags;
    g_FieldScriptVMCurActor->dialogFlags = dialogFlags & 0xFFFF;
    faceFlags = dialogFlags & 0xFFFF;
    if (SCRIPT_READ_U8_REL(3) != 0) {
        faceFlags = (dialogFlags & 0xFF00) | SCRIPT_READ_U8_REL(3);
        g_FieldScriptVMCurActor->dialogFlags = (dialogFlags & 0xFFFF) | (faceFlags << 16);
    }

    y = 0x10;
    switch ((faceFlags >> 4) & 0x3) {
    case FIELD_TEXT_BOX_POSITION_FROM_ACTOR:
        if (!(((g_FieldScriptVMCurActor->direction - (FieldGetCameraDirection() & 0xFFFF)) & MASK_8DIR_MOVEMENT_NUM_DIRECTIONS) < 5)) {
            if (flagsUnion & FIELD_TEXT_BOX_FLAG_BOTTOM) {
                goto layoutBelow;
            }
            if (visibleCount == 0) {
                goto layoutAbove;
            }
            goto layoutBelow;
        } else if (!(flagsUnion & FIELD_TEXT_BOX_FLAG_BOTTOM)) {
            goto layoutBelow;
        }
        // Falls through into the layout above the actor.
    case FIELD_TEXT_BOX_POSITION_ABOVE:
    layoutAbove:
        g_FieldTextBoxes[textBoxIndex].flags = FIELD_TEXT_BOX_FLAG_TOP;
        if (mode == 0 || mode == 3) {
            FieldProjectActorPointToScreen(actorIndex, &x, &screenY, -0x40);
            if (mode == 0) {
                projectedY = screenY;
                projectedY = projectedY - height * 14;
                y = projectedY - 0x24;
            } else {
                y = 0x14;
                x = 0xA0;
            }
            if (g_FieldScriptVMCurActor->faceId != 0xFF && !(faceFlags & 0x2)) {
                height = 4;
                if (width < 0x18) {
                    width = 0x18;
                }
                width += 0x11;
                y = 0x10;
            }
        } else {
            width = 0x48;
            height = 4;
            y = 0x10;
            x = 0xA0;
        }
        break;
    case FIELD_TEXT_BOX_POSITION_BELOW:
    layoutBelow:
        g_FieldTextBoxes[textBoxIndex].flags = FIELD_TEXT_BOX_FLAG_TOP | FIELD_TEXT_BOX_FLAG_BOTTOM;
        if (mode == 0 || mode == 3) {
            FieldProjectActorPointToScreen(actorIndex, &x, &screenY, -0x40);
            if (mode == 0) {
                y = screenY + 0x30;
            } else {
                y = 0x94;
                x = 0xA0;
            }
            if (g_FieldScriptVMCurActor->faceId != 0xFF && !(faceFlags & 0x2)) {
                if (width < 0x18) {
                    width = 0x18;
                }
                width += 0x11;
                height = 4;
                y = 0x94;
            }
        } else {
            y = 0x94;
            width = 0x48;
            height = 4;
            x = 0xA0;
        }
        break;
    }

    // Keep the window inside the visible area.
    i = 8;
    left = x - i - width * 2;
    if (left < 0xC) {
        left = 0xC;
    }
    i = 0x10;
    if (left + i + width * 4 >= 0x135) {
        left = 0x124 - width * 4;
    }
    if (y < 0x10) {
        y = 0x10;
    }
    i = 8;
    if (y + i + height * 14 >= 0xD5) {
        y = 0xCC - height * 14;
    }

    if (mode == 0 || mode == 3) {
        if (g_FieldScriptVMCurActor->dialogPixelWidth != 0) {
            left = g_FieldScriptVMCurActor->dialogPixelWidth;
        }
        if (g_FieldScriptVMCurActor->dialogPixelHeight != 0) {
            y = g_FieldScriptVMCurActor->dialogPixelHeight;
        }
        if (g_FieldScriptVMCurActor->dialogWidth != 0) {
            width = g_FieldScriptVMCurActor->dialogWidth;
        }
        if (g_FieldScriptVMCurActor->dialogHeight != 0) {
            height = g_FieldScriptVMCurActor->dialogHeight;
        }
        if (g_FieldScriptVMCurActor->faceId != 0xFF && !(faceFlags & 0x2)) {
            height = 4;
        }
    }

    if (faceFlags & FIELD_TEXT_BOX_FLAG_NO_BORDER) {
        g_FieldTextBoxes[textBoxIndex].flags |= FIELD_TEXT_BOX_FLAG_NO_BORDER;
    }

    flags = 0;
    if ((faceFlags & 0xC) == 0) {
        if ((((g_FieldActors[actorIndex].pActorData->rotation.vy >> 9) - (FieldGetCameraDirection() & 0xFFFF) + 1) & MASK_8DIR_MOVEMENT_NUM_DIRECTIONS) >= 4) {
            flags = 0x400;
        }
    } else if (faceFlags & 0x4) {
        flags = 0x400;
    }

    func_8007F8DC(left, y, dialogIndex, textBoxIndex, width, height, g_FieldCurActorIndex, actorIndex,
        mode, flags, faceFlags);
    FieldTextBoxMarkInUse(textBoxIndex);

    g_FieldScriptVMCurActor->rotation.vx |= 0x8000;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 4;
    return 0;

textBoxOpen:
    g_FieldScriptVMStopFlag = 1;
    g_FieldTextBoxes[foundIndex].status = 0;
    return -1;
}
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
