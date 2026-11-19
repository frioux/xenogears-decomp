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


extern s32 g_FieldArchiveStreamActive;
extern s32 g_FieldRequestCode;
extern s32 g_FieldStreamSceneActive;
extern s32 D_800AFD04;
extern s32 D_800C4268;

// The dialog data the two text size lookups read; system.c owns its layout.
typedef struct FieldDialogData FieldDialogData;

extern FieldDialogData* D_800ADBF0;
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
INCLUDE_ASM("asm/field/nonmatchings/dialogue/text_box", func_8009C5A8);
/* 99.87% (1872 bytes = retail's exact size, 469 instructions; the whole body
   matches instruction for instruction except ONE dead instruction).
   The difference is in the below-actor arm at 0x2CEEC: retail fills
   `bne $s7,$v0` with `ori $s0,zero,0x94` - a y = 0x94 whose value both later
   paths overwrite - where this build leaves a nop. Retail already has the live
   copy of that constant in the following `bnez $s7` delay slot at 0x2CF0C and a
   third in the faceId block's `j` slot at 0x2CF60, and this body has those two,
   so retail's source carries one extra 0x94 assignment in this arm and gcc
   2.7.2 declines to duplicate it into the bne's slot here.
   Tried and measured on this body: `y = 0x94;` as the first statement of the
   mode==0||mode==3 block (seed before the call) puts the constant in the bnez
   slot AFTER the branch instead and cse then folds the else arm's copy away
   (99.56, the bnez slot takes `li $v0,0xA0`); the same seed before the two mode
   tests is stolen by reorg into the `beqz $s7` slot with the else copy folded
   again (99.42); `y = screenY; y = y + 0x30;` regresses the else arm (99.82);
   an empty `do { } while (0);` before the mode test changes nothing; a
   4-minute permuter run gave PERMUTER_NONE, base 200 -> best 200.
   Mechanism: cse walks from the flags store straight into the block after the
   mode tests (that label has a single use), so two 0x94 assignments to `y` in
   this arm can never both survive; retail's extra copy is a reorg fill this
   build does not make. Do not re-derive the rest - it took 99.41 -> 99.87:
   - `i = 0;` as its own statement before the visibleCount/flagsUnion loop (with
     `for (; i < 4; i++)`) is what puts `move $a0,zero` FIRST, as retail has it;
     the loop's own init clause emits it after the two accumulators (0.03).
   - the three-statement `projectedY` split of the above arm's y is required:
     with `y = screenY - height * 14 - 0x24;` this gcc computes height*14 before
     loading screenY, retail loads screenY first (0.46).
   - `foundIndex` (the address-taken out-param of
     FieldTextBoxFindCurrentActorTextBox) is NOT textBoxIndex: retail keeps
     textBoxIndex in a register ($s1) and spills only the found index, which is
     read once in the "already open" path. With one variable for both the whole
     allocation collapses (textBoxIndex spills, 8 saved regs, 0x68 frame).
   - the "already open" tail must be a `goto` to a label at the END of the
     function: coded inline it lands at 0x160 instead of retail's 0x6E4 and
     every branch target in the function is off by 0x38.
   - `x` is the projection out-param (memory, 0x34) and `screenY` is a second
     out-param (0x38) that only exists to be copied into `y`, a register
     variable; the window's left edge is a third register variable (`left`)
     computed from `x`. Writing the clamps back into `x` stores to memory and
     costs 9 instructions.
   - the `i = 8` / `i = 0x10` locals before the clamp comparisons must stay:
     without them gcc reassociates `(x - 8) - width * 2` into
     `x - (width * 2 + 8)` and `(y + 8) + height * 14` into
     `y + (height * 14 + 8)`, five instructions off each way (97.10).
s32 func_8009C5A8(s32 actorIndex, s32 mode) {
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
    s32 position;
    s32 i;
    u32 dialogFlags;
    u32 faceFlags;
    s32 flags;
    u16 dialogIndex;

    g_FieldScriptMaxInstructionCount += 0x20;

    if (g_FieldArchiveStreamActive != 0 || D_800AFD04 != 0 || D_800C4268 != 0 ||
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

    width = DialogGetWidth(D_800ADBF0, dialogIndex);
    height = DialogGetHeight(D_800ADBF0, dialogIndex);

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
*/

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
