#include "common.h"
#include "battle/actor.h"

// The battle work block's per-actor action-row tables: 0x26 rows of 0x28 bytes per
// actor at +0x1058 and 0x2A rows per actor at +0x2228, three actors each.
#define BATTLE_ACTOR_ROW_TABLE 0x1058
#define BATTLE_ACTOR_ROW_STRIDE 0x5F0
#define BATTLE_GEAR_ROW_TABLE 0x2228
#define BATTLE_GEAR_ROW_STRIDE 0x690

extern BattleWorkBlock* g_BattleCurWorkBlock;
extern BattleRow* g_BattleCurrentRow;
extern u8 g_BattleCommandCharIndex;

void BattleSetSlotFlag15A1(u8 arg0) {
    BattleTableEntry* p;
    BattleTableEntry* q;

    g_BattleCurWorkBlock->rowIndex = 0;
    q = &g_BattleCurWorkBlock->slots[arg0];
    q->state.flags |= 1;
    p = &g_BattleCurWorkBlock->slots[arg0];
    if ((p->state.flags & BATTLE_SLOT_GEAR_STATUS_FLAG) && (p->state.accessoryEffectFlags2 & 0x10)) {
        p->state.activeEffectFlags1 &= 0xFE4F;
        p->activeEffectFlags1 &= 0xEFFF;
    }
    if (g_BattleCommandCharIndex == 4) {
        g_BattleCurWorkBlock->commandCode = 0x3D;
    }
}

u8 BattleClearSlotFlag15A1(u8 arg0) {
    BattleTableEntry* p = &g_BattleCurWorkBlock->slots[arg0];
    u8 flags = p->state.flags & 0xFE;

    p->state.flags = flags;
    return flags;
}

// The row walks below cannot use a typed table view of g_BattleCurWorkBlock: the
// address has to stay an integer sum added to the block base, because gcc emits
// `addu d,base,offset` for `block->table[arg0]` where retail has `addu d,offset,base`
// (measured 99.71% / 99.77% with the tables declared on the struct).
// Re-checked with a .c-local view struct (2D BattleRow arrays at +0x1058/+0x2228):
// inline per-store indexing, a block pointer local in the arm and at the top of the
// function, a sizeof-scaled byte offset off the typed member, `[0][0]` addressing and
// a row-array typedef all land on 99.71/99.77 at retail's size, and a 4-minute
// permuter run (2294 iterations, base=20 best=20) found nothing. Also measured, both
// worse: sharing the two `slots[arg0]` pointers p and r (98.68), and a block pointer
// live across the function (90.81 in To2000). So the cast sum stays.
// Round 327 re-derived it with a .c-local view struct above the functions
// (`->gearRows[arg0]`, and the index-first `arg0 * rows + ->gearRows[0]`): 99.71/99.77
// again, the two addu alone, and a single-expression typed sum with no `s32 rowOffset`
// local is 93.75. The row base stays a byte offset in a local added to the block base.
void BattleSetSlotEntryFieldsTo1(u8 arg0) {
    BattleTableEntry* p = &g_BattleCurWorkBlock->slots[arg0];
    BattleTableEntry* r;
    u16 f;

    if (p->state.flags & BATTLE_SLOT_GEAR_STATUS_FLAG) {
        f = p->state.activeEffectFlags2 & BATTLE_GEAR_EFFECT_FLAG;
    } else {
        f = p->activeEffectFlags4 & BATTLE_ACTOR_EFFECT_FLAG;
    }
    if (f == 0) {
        return;
    }
    r = &g_BattleCurWorkBlock->slots[arg0];
    if (r->state.flags & BATTLE_SLOT_GEAR_STATUS_FLAG) {
        s32 rowOffset = arg0 * BATTLE_GEAR_ROW_STRIDE + BATTLE_GEAR_ROW_TABLE;
        BattleRow* pRow = (BattleRow*)(rowOffset + (u32)g_BattleCurWorkBlock);

        pRow[21].flags = 1;
        pRow[23].flags = 1;
        pRow[24].flags = 1;
        pRow[25].flags = 1;
        pRow[26].flags = 1;
        pRow[27].flags = 1;
        pRow[28].flags = 1;
    } else {
        s32 rowOffset = arg0 * BATTLE_ACTOR_ROW_STRIDE + BATTLE_ACTOR_ROW_TABLE;
        BattleRow* pRow = (BattleRow*)(rowOffset + (u32)g_BattleCurWorkBlock);

        pRow[22].flags = 1;
        pRow[24].flags = 1;
        pRow[25].flags = 1;
        pRow[26].flags = 1;
        pRow[27].flags = 1;
        pRow[28].flags = 1;
        pRow[29].flags = 1;
        pRow[30].flags = 1;
        pRow[31].flags = 1;
        pRow[32].flags = 1;
    }
}
void BattleSetSlotEntryFieldsTo2000(u8 arg0, u8 arg1) {
    BattleTableEntry* p;
    BattleTableEntry* r;
    u16 f;

    if (arg1 != 0 && !(g_BattleCurrentRow->actionFlags & 0x100)) {
        return;
    }
    p = &g_BattleCurWorkBlock->slots[arg0];
    if (p->state.flags & BATTLE_SLOT_GEAR_STATUS_FLAG) {
        f = p->state.activeEffectFlags2 & BATTLE_GEAR_EFFECT_FLAG;
    } else {
        f = p->activeEffectFlags4 & BATTLE_ACTOR_EFFECT_FLAG;
    }
    if (f == 0) {
        return;
    }
    r = &g_BattleCurWorkBlock->slots[arg0];
    if (r->state.flags & BATTLE_SLOT_GEAR_STATUS_FLAG) {
        s32 rowOffset = arg0 * BATTLE_GEAR_ROW_STRIDE + BATTLE_GEAR_ROW_TABLE;
        BattleRow* pRow = (BattleRow*)(rowOffset + (u32)g_BattleCurWorkBlock);

        pRow[21].flags = BATTLE_GEAR_EFFECT_FLAG;
        pRow[23].flags = BATTLE_GEAR_EFFECT_FLAG;
        pRow[24].flags = BATTLE_GEAR_EFFECT_FLAG;
        pRow[25].flags = BATTLE_GEAR_EFFECT_FLAG;
        pRow[26].flags = BATTLE_GEAR_EFFECT_FLAG;
        pRow[27].flags = BATTLE_GEAR_EFFECT_FLAG;
        pRow[28].flags = BATTLE_GEAR_EFFECT_FLAG;
        r->state.activeEffectFlags2 &= ~BATTLE_GEAR_EFFECT_FLAG;
    } else {
        s32 rowOffset = arg0 * BATTLE_ACTOR_ROW_STRIDE + BATTLE_ACTOR_ROW_TABLE;
        BattleRow* pRow = (BattleRow*)(rowOffset + (u32)g_BattleCurWorkBlock);

        pRow[22].flags = BATTLE_GEAR_EFFECT_FLAG;
        pRow[24].flags = BATTLE_GEAR_EFFECT_FLAG;
        pRow[25].flags = BATTLE_GEAR_EFFECT_FLAG;
        pRow[26].flags = BATTLE_GEAR_EFFECT_FLAG;
        pRow[27].flags = BATTLE_GEAR_EFFECT_FLAG;
        pRow[28].flags = BATTLE_GEAR_EFFECT_FLAG;
        pRow[29].flags = BATTLE_GEAR_EFFECT_FLAG;
        pRow[30].flags = BATTLE_GEAR_EFFECT_FLAG;
        pRow[31].flags = BATTLE_GEAR_EFFECT_FLAG;
        pRow[32].flags = BATTLE_GEAR_EFFECT_FLAG;
        r->activeEffectFlags4 &= ~BATTLE_ACTOR_EFFECT_FLAG;
    }
}
