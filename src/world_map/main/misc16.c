#include "common.h"
#include "psyq/libgte.h"

extern VECTOR D_8009BB4C;
extern VECTOR D_8009BB5C;
extern VECTOR D_8009BB6C;
extern VECTOR D_8009BB7C;
extern VECTOR D_8009BB8C;
extern VECTOR D_8009BB9C;
extern VECTOR D_8009C7F0;
extern VECTOR D_8009C828;
extern VECTOR D_8009C844;
extern VECTOR D_8009C874;

typedef struct {
    /* 0x0 */ s32 unk0;
    /* 0x4 */ s32 unk4;
    /* 0x8 */ s32 unk8;
} WorldMapMisc16Struct; // Size: 0xC

extern u16 D_8009D558;
extern s16 D_8009C838;
extern s16 D_8009C83C;

void func_80098044(void) {
    OuterProduct0(&D_8009BB6C, &D_8009BB4C, &D_8009C828);
    OuterProduct0(&D_8009BB4C, &D_8009BB7C, &D_8009C844);
    OuterProduct0(&D_8009BB8C, &D_8009BB5C, &D_8009C874);
    OuterProduct0(&D_8009BB5C, &D_8009BB9C, &D_8009C7F0);
}

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc16", func_800980D4);
/*
void func_800980D4(WorldMapMisc16Struct* p) {
    s32 y;
    D_8009D558 = 0;
    y = p->unk8;
    if (p->unk0 < -0x800000) {
        p->unk0 += 0x800000;
        D_8009D558 = 4;
    } else if (p->unk0 > 0x800000) {
        p->unk0 -= 0x800000;
        D_8009D558 = 8;
    }
    if (y < -0x800000) {
        p->unk8 += 0x800000;
        D_8009D558 |= 1;
    } else if (y > 0x800000) {
        p->unk8 -= 0x800000;
        D_8009D558 |= 2;
    }
    y = p->unk0 >> 23;
    D_8009C838 = y + 2;
    y = p->unk8 >> 23;
    D_8009C83C = y + 2;
}
*/ // 93.74%: the second field load is hoisted above the first store by our cc1.
// Writing the tail as D_8009C838 = (p->unk0 >> 23) + 2; puts the two top loads in
// the original order but then cc1 hoists the second tail load above the first
// D_8009C838 store (88.77%). Both halves cannot be won at once so far.
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc16", func_800981C8);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc16", func_800983A0);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc16", func_800987AC);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc16", func_80098CC0);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc16", func_8009932C);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc16", func_80099708);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc16", func_8009980C);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc16", func_80099BFC);
