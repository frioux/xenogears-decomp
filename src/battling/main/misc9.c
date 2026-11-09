#include "common.h"

extern s32 D_80092790;
extern s32 D_800927AC;
extern s32* D_800928DC;

typedef struct {
    /* 0x0 */ u8 unk0[0x8];
} Unk8; // Size: 0x8

extern Unk8 D_80092768;

extern s32 func_8008AC0C();

INCLUDE_ASM("asm/battling/nonmatchings/main/misc9", func_80082178);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc9", func_80082300);

void func_80082458(Unk8* p) {
    *p = D_80092768;
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc9", func_80082488);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc9", func_80082880);

u32 func_800828C4(s32* p) {
    s32 a;
    s32 b;

    a = p[0] >> 8;
    b = p[2] >> 8;
    return (u32)D_800928DC[(b << 7) + a];
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc9", func_800828F8);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc9", func_80082A70);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc9", func_80082C4C);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc9", func_80082E60);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc9", func_800831C8);

void func_800832C0(s32 flags) {
    if (flags & 0x8000) {
        D_800927AC += 0x20;
    }
    if (flags & 0x2000) {
        D_800927AC -= 0x20;
    }
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc9", func_80083310);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc9", func_8008369C);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc9", func_80083738);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc9", func_80083B54);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc9", func_80083BB4);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc9", func_80083C0C);

s32 func_80083CD8(void) {
    return D_80092790;
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc9", func_80083CE8);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc9", func_80083DCC);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc9", func_800840CC);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc9", func_800846A0);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc9", func_800849E0);

s32 func_80084A40(u8* p) {
    return func_8008AC0C(*(s32*)(p + 0x284));
}
