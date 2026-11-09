#include "common.h"
#include "system/memory.h"

// .data/.bss of the overlay (supplied by splat's asm segments)
extern u8 D_80099D98;
extern u8 D_80099D9F;
extern u32 D_800912F4[];
extern u32 D_800912F0;
extern s32 D_80092734;
extern s32 D_80092950;

void func_80083C0C(s32 arg);
void func_8007F834(void);
void func_80080C48(s32 arg);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc7", func_8007F854);

void func_8007F8B4(void) {
    long pad[2];

    func_80083C0C(1);
    D_80092734 = 0;
    func_8007F834();
}

void func_8007F8E4(void) {
    s32 arg;

    func_80080C48(0);
    if (D_80099D9F != 0 || D_80092950 == 1) {
        arg = 3;
    } else {
        D_80092950--;
        arg = 6;
    }
    func_80083C0C(arg);
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc7", func_8007F948);

u32 func_8007F97C(void) {
    return D_800912F4[D_80099D98];
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc7", func_8007F9A0);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc7", func_8007FB0C);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc7", func_8007FBEC);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc7", func_8007FE48);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc7", func_8007FF70);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc7", func_80080054);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc7", func_80080090);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc7", func_800800CC);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc7", func_80080108);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc7", func_80080144);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc7", func_80080180);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc7", func_800801BC);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc7", func_800801F8);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc7", func_80080234);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc7", func_80080268);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc7", func_800802A4);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc7", func_8008040C);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc7", func_80080570);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc7", func_80080644);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc7", func_80080780);
