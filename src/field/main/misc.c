#include "common.h"
#include "main/game.h"
#include "system/math.h"
#include "field/main.h"
#include "field/camera.h"
#include "field/actor.h"
#include "field/script_vm.h"
#include "field/text_box.h"
#include "field/particles.h"

void FieldScriptMemoryWriteU16(int, int);

extern int rcos(int);
extern int rsin(int);

extern s32 D_800AFD1C;
extern s32 D_8005A444[MAX_PARTY_MEMBERS];

void FieldSetScreenDimensions(void) {
    g_FieldRenderContexts[0].dispEnv.screen.x = 0;
    g_FieldRenderContexts[0].dispEnv.screen.y = 10;
    g_FieldRenderContexts[0].dispEnv.screen.w = 0x100;
    g_FieldRenderContexts[0].dispEnv.screen.h = 0xd8;
    g_FieldRenderContexts[1].dispEnv.screen.x = 0;
    g_FieldRenderContexts[1].dispEnv.screen.y = 10;
    g_FieldRenderContexts[1].dispEnv.screen.w = 0x100;
    g_FieldRenderContexts[1].dispEnv.screen.h = 0xd8;
}

extern u8 D_800B2358[]; // Is pause disabled?

void func_80086DE0(void) {
    D_800B2358[0] = SCRIPT_READ_U8_REL(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}

extern s16 D_800ADB54;

void func_80086E1C(void) {
    RECT rect;
    switch (FieldScriptVMGetArgument(1)) {
        case 0:
            rect.w = 0x500;
            rect.x = 0;
            rect.y = 0;
            rect.h = 0x200;
            ClearImage(&rect, 0x0, 0x0, 0x0);
            DrawSync(0);
            Vsync(0);
            SetDefDrawEnv(g_FieldRenderContexts[0].drawEnvs, 0, 0, 0x280, 0xE0);
            SetDefDrawEnv(g_FieldRenderContexts[1].drawEnvs, 0, 0x100, 0x280, 0xE0);
            SetDefDispEnv(&g_FieldRenderContexts[0].dispEnv, 0, 0x100, 0x280, 0xE0);
            SetDefDispEnv(&g_FieldRenderContexts[1].dispEnv, 0, 0, 0x280, 0xE0);
            FieldSetScreenDimensions();
            break;
        case 1:
            D_800ADB54 = 0x1;
            break;
        case 2:
            D_800ADB54 = 0x0;
            break;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

// Set model animation for actor
void func_80086F7C(void) {
    g_FieldScriptVMCurActor->modelAnimation = (FieldScriptVMGetArgument(1) << 0xC) | FieldScriptVMGetArgument(3);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}

void func_80086FD0(void) {
    switch (SCRIPT_READ_U8_REL(1)) {
        case 0:
            // Initialize Sprite List
            func_800AAC08();
            g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
            return;
        case 2:
            // Free Sprite List
            func_800AABD8();
            g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
            return;
        case 1:
            // Set X, Y of Sprite and add it to OT for drawing
            func_800AAE4C(
                FieldScriptVMGetArgument(2), // Sprite Index
                FieldScriptVMGetArgument(4), // X
                FieldScriptVMGetArgument(6), // Y
                FieldScriptVMGetArgument(8)  // Sprite type
            );
            g_FieldScriptVMCurActor->scriptInstructionPointer += 0xA;
            return;
        case 3:
            // Set Sprite Color
            func_800AADC8(
                FieldScriptVMGetArgument(2), // Sprite Index
                FieldScriptVMGetArgument(4), // Red
                FieldScriptVMGetArgument(6), // Green
                FieldScriptVMGetArgument(8)  // Blue
            );
            g_FieldScriptVMCurActor->scriptInstructionPointer += 0xA;
            return;
    }
}

void func_80087148(void) {
    int index = FieldScriptVMGetArgument(1);
    int value = FieldScriptVMGetArgument(3);

    g_pGameState->unk16BC[index].unk1E |= value;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}

INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_800871B0);

extern u8 D_800B225F[];

void func_800873C4(void) {
    int characterId = FieldScriptVMGetArgument(1);
    D_800B225F[characterId] = FieldScriptVMGetArgument(3);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}

void func_80087420(void) {
    int arg1, arg2, arg3, arg4, arg5, arg6;
    int value1, value2;

    arg1 = FieldScriptVMGetArgument(ARG(1));
    arg2 = FieldScriptVMGetArgument(ARG(2));
    arg3 = FieldScriptVMGetArgument(ARG(3));
    arg4 = FieldScriptVMGetArgument(ARG(4));
    arg5 = FieldScriptVMGetArgument(ARG(5));
    arg6 = FieldScriptVMGetArgument(ARG(6));

    value1 = CONV_TO_GTE(
        CONV_FROM_GTE(arg5) / arg3 * arg1
    );
    value2 = CONV_TO_GTE(
        CONV_FROM_GTE(arg6) / arg4 * arg2
    );
    
    FieldScriptMemoryWriteU16(
        SCRIPT_IMM_ARG(7), 
        value1
    );
    FieldScriptMemoryWriteU16(
        SCRIPT_IMM_ARG(8), 
        value2
    );
    
    g_FieldScriptVMCurActor->scriptInstructionPointer += 0x11;
}

void func_8008752C(void) {
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void func_8008754C(void) {
    g_pGameState->unk22B6 |= 0x4000;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 1;
}

void FieldScriptCopyGear(void) {
    int sourceGearIndex = FieldScriptVMGetArgument(1);
    g_pGameState->gears[FieldScriptVMGetArgument(3)] = g_pGameState->gears[sourceGearIndex];
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}

INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_8008764C);

extern u8 D_80050622;

void func_80087800(void) {
    FieldScriptMemoryWriteU16(
        SCRIPT_IMM_ARG(1), 
        D_80050622
    );
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_80087848);

void func_80087960(void) {
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(1), *(u_short*)&g_pGameState->unk17FC[0x48]);
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(2), *(u_short*)&g_pGameState->unk17FC[0x4A]);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}

void func_800879D0(void) {
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(1), g_pGameState->unk184E);
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(2), g_pGameState->unk1852);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}

// Globals written directly by field script handlers. Declaring them as struct
// members (rather than bare scalars) keeps gcc from hoisting the actor's
// instruction pointer load above the store.
typedef struct { s16 value; } FieldScriptGlobalS16;
typedef struct { u16 value; } FieldScriptGlobalU16;
typedef struct { s32 value; } FieldScriptGlobalS32;
typedef struct { u8 value; } FieldScriptGlobalU8;

extern FieldScriptGlobalU8 D_800B2357;

void func_80087A40(void) {
    D_800B2357.value = SCRIPT_READ_U8_REL(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}

extern FieldScriptGlobalU8 D_800B2354;

void func_80087A7C(void) {
    D_800B2354.value = SCRIPT_READ_U8_REL(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}

INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_80087AB8);

void func_80087B5C(void) {
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(1), *(u_short*)&g_pGameState->unk17FC[0x30]);
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(2), *(u_short*)&g_pGameState->unk17FC[0x32]);
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(3), *(u_short*)&g_pGameState->unk17FC[0x34]);
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(4), *(u_short*)&g_pGameState->unk17FC[0x36]);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 9;
}

extern s32 D_8004F300;

void func_80087C0C(void) {
    g_FieldScriptVMCurActor->scriptInstructionPointer += 1;
    D_8004F300 = 1;
}

INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_80087C34);

void func_80087D30(void) {
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(1), *(u_short*)&g_pGameState->unk17FC[0x38]);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void func_80087D80(void) {
    int value = FieldScriptArgument1(1, SCRIPT_READ_U8_REL(3));
    ActorData* pActor = g_FieldScriptVMCurActor;

    *(u_short*)&g_pGameState->unk17FC[0x38] = value;
    pActor->scriptInstructionPointer += 4;
}

extern FieldScriptGlobalU8 D_800B2355;
extern FieldScriptGlobalU8 D_800B2356;

void func_80087DE0(void) {
    int value = FieldScriptVMGetArgument(2);

    if (SCRIPT_READ_U8_REL(1) == 0) {
        D_800B2355.value = value;
    } else {
        D_800B2356.value = value;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 4;
}

extern FieldScriptGlobalS16 D_800B234C;

void func_80087E5C(void) {
    D_800B234C.value = FieldScriptVMGetArgument(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_80087E98);

extern FieldScriptGlobalU16 D_800B2348;

INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_80087FA4);
// 79.83%: gcc folds the symbol into both accesses (`lui/lhu` then `lui at/sh`),
// the original materialises &D_800B2348 once in $v1 and uses 0($v1) twice. A
// local pointer to the global (`p->value++`, also `+= 1`, `volatile` variants)
// is folded back to the same immediate form.
/*
void func_80087FA4(void) {
    D_800B2348.value++;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 1;
}
*/

void func_80087FD4(void) {
    func_800A8BA4();
    g_FieldScriptVMCurActor->scriptInstructionPointer += 1;
}

INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_8008800C);

void func_80088198(void) {
    int i;

    for (i = 0; i < MAX_GAME_GEARS; i++) {
        g_pGameState->gears[i].hp = g_pGameState->gears[i].maxHp;
        g_pGameState->gears[i].fuel = g_pGameState->gears[i].maxFuel;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 1;
}

void func_800881E8(void) {
    if (SCRIPT_READ_U8_REL(1) == 0) {
        func_800A915C();
    } else {
        func_800A91F0();
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}

extern s32 D_8004F308;

void func_8008825C(void) {
    if (D_8004F308 == -1) {
        g_FieldScriptVMCurActor->scriptInstructionPointer -= 1;
    } else {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 1;
    }
    D_800B00C0 = 1;
}

INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_800882B8);

void FieldScriptSetCharacterGear(void) {
    int characterId = FieldScriptVMGetArgument(1);
    g_pGameState->characters[characterId].gearId = FieldScriptVMGetArgument(3);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}

void func_800883D4(void) {
    int partyId = func_8008CF3C(FieldScriptVMGetArgument(2));

    if (partyId != CHARACTER_ID_NONE) {
        if (SCRIPT_READ_U8_REL(1) == 0) {
            ((u_short*)&g_pGameState->unk22B6)[0x31] |= 1 << partyId;
        } else {
            ((u_short*)&g_pGameState->unk22B6)[0x31] &= ~(1 << partyId);
        }
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 4;
}

extern FieldScriptGlobalS16 D_800B236C;

INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_8008848C);
// 78.75%: same instructions in the same order but gcc keeps the instruction
// pointer in a register across both uses and reuses $v0 for the script data
// base where the original reloads the pointer into the killed $v1.
/*
void func_8008848C(void) {
    int value = SCRIPT_READ_U8_REL(1) ^ 1;

    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
    D_800B236C.value = value;
}
*/

extern FieldScriptGlobalS16 D_800B21AC;

void func_800884CC(void) {
    D_800B21AC.value = FieldScriptVMGetArgument(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_80088508);

typedef struct {
    /* 0x0 */ int bankIndex;
    /* 0x4 */ u8 unk4[0xC];
} ParticleBankHandle;

extern ParticleBankHandle D_800B2384;

INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_8008861C);
// 56.59%: z-then-x store order is right; the original stores x through
// %lo(g_FieldDefaultParticleBanks + 0x30)($at) with the bank offset in the
// register, ours keeps a pointer in a temp instead.
/*
void func_8008861C(void) {
    int i;

    for (i = 0; i < NUM_PARTICLE_BANKS; i++) {
        g_FieldDefaultParticleBanks[D_800B2384.bankIndex].directions[i].z = 0;
        g_FieldDefaultParticleBanks[D_800B2384.bankIndex].directions[i].x = 0;
    }
}
*/

INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_80088674);

INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_80088790);

INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_800888A4);

INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_800889BC);

// Start of particle handlers
extern s32 D_800ADB40;
extern s32 D_800ADB8C;
extern s32 D_800B2374;
extern s32 D_800B2378;
extern s32 D_800B237C;
extern s32 D_800B2380;

extern s32 g_FieldScriptMaxInstructionCount;

void func_80088B68(void) {
    int flags = 0;

    switch (FieldScriptVMGetArgument(1)) {
        case 1:
            flags = 0x80;
            break;
        case 2:
            flags = 0x40;
            break;
    }
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].flags |= flags;
    g_FieldScriptMaxInstructionCount += 4;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 7;
}

void func_80088C1C(void) {
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].speedMultiplier = FieldScriptVMGetArgument(1);
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].flags |= FieldScriptVMGetArgument(3) << 8;
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].rotAngle = FieldScriptVMGetArgument(5);
    g_FieldScriptMaxInstructionCount += 4;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 7;
}

void func_80088CF8() {
    FieldScriptSetParticleBankDirections(0);
}

void func_80088D18() {
    FieldScriptSetParticleBankDirections(4);
}

void FieldScriptSetParticleBankDirections(int index) {
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].directions[index].x = FieldScriptArgument1(1, SCRIPT_READ_U8_REL(0x11));
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].directions[index].z = FieldScriptArgument2(3, SCRIPT_READ_U8_REL(0x11));
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].directions[index + 1].x = FieldScriptArgument3(5, SCRIPT_READ_U8_REL(0x11));
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].directions[index + 1].z = FieldScriptArgument4(7, SCRIPT_READ_U8_REL(0x11));
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].directions[index + 2].x = FieldScriptArgument5(9, SCRIPT_READ_U8_REL(0x11));
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].directions[index + 2].z = FieldScriptArgument6(0xB, SCRIPT_READ_U8_REL(0x11));
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].directions[index + 3].x = FieldScriptArgument7(0xD, SCRIPT_READ_U8_REL(0x11));
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].directions[index + 3].z = FieldScriptArgument8(0xF, SCRIPT_READ_U8_REL(0x11));
    g_FieldScriptMaxInstructionCount += 4;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 0x12;
}

void FieldScriptInitializeParticleBank(void) {
    D_800B2384.bankIndex = FieldScriptVMGetArgument(1);
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].speedMultiplier = 1;
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].targetActorID = D_800B2374;
    D_800ADB40 = g_FieldDefaultParticleBanks[D_800B2384.bankIndex].targetActorID;
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].unk0 = 0;
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].rotAngle = 0;
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].max = FieldScriptVMGetArgument(3);
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].swait = FieldScriptVMGetArgument(5);
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].ewait = FieldScriptVMGetArgument(7);
    func_8008861C();
    g_FieldScriptMaxInstructionCount += 4;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 9;
}

void FieldScriptSetParticleBankPosition(void) {
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].pos.vx = FieldScriptArgument1(1, SCRIPT_READ_U8_REL(0xD));
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].pos.vy  = FieldScriptArgument2(3, SCRIPT_READ_U8_REL(0xD));
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].pos.vz = FieldScriptArgument3(5, SCRIPT_READ_U8_REL(0xD));
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].epos.vx = FieldScriptArgument4(7, SCRIPT_READ_U8_REL(0xD));
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].epos.vy = FieldScriptArgument5(9, SCRIPT_READ_U8_REL(0xD));
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].epos.vz = FieldScriptArgument6(0xB, SCRIPT_READ_U8_REL(0xD));
    g_FieldScriptMaxInstructionCount += 4;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 0xE;
}

void FieldScriptSetParticleBankPhysics(void) {
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].speed = FieldScriptArgument1(1, SCRIPT_READ_U8_REL(0xD));
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].gravity.vx = FieldScriptArgument2(3, SCRIPT_READ_U8_REL(0xD));
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].gravity.vy = FieldScriptArgument3(5, SCRIPT_READ_U8_REL(0xD));
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].gravity.vz = FieldScriptArgument4(7, SCRIPT_READ_U8_REL(0xD));
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].srange = FieldScriptArgument5(9, SCRIPT_READ_U8_REL(0xD));
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].erange = FieldScriptArgument6(0xB, SCRIPT_READ_U8_REL(0xD));
    g_FieldScriptMaxInstructionCount += 4;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 0xE;
}

void FieldScriptSetParticleBankParameters(void) {
    int flags;
    int flags_2;

    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].pswait = FieldScriptVMGetArgument(1);
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].pewait = FieldScriptVMGetArgument(3);
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].shape = FieldScriptVMGetArgument(5);
    flags = FieldScriptVMGetArgument(7);
    flags_2 = (FieldScriptVMGetArgument(9) * 2);
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].flags = flags | flags_2 | D_800B2378;
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].unk72 = D_800B237C;
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].unk74 = D_800B2380;
    g_FieldScriptMaxInstructionCount += 4;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 0xB;
}

void FieldScriptSetParticleBankScale(void) {
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].scale.vx = FieldScriptArgument1(1, SCRIPT_READ_U8_REL(0x9));
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].scale.vy = FieldScriptArgument2(3, SCRIPT_READ_U8_REL(0x9));
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].scale.vz = 0;
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].scaleDelta.vx = FieldScriptArgument3(5, SCRIPT_READ_U8_REL(0x9));
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].scaleDelta.vy = FieldScriptArgument4(7, SCRIPT_READ_U8_REL(0x9));
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].scaleDelta.vz = 0;
    g_FieldScriptMaxInstructionCount += 4;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 0xA;
}

void FieldScriptSetParticleBankColor(void) {
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].color.r  = FieldScriptArgument1(1, SCRIPT_READ_U8_REL(0xD));
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].color.g = FieldScriptArgument2(3, SCRIPT_READ_U8_REL(0xD));
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].color.b = FieldScriptArgument3(5, SCRIPT_READ_U8_REL(0xD));
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].colorDelta.r = FieldScriptArgument4(7, SCRIPT_READ_U8_REL(0xD));
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].colorDelta.g = FieldScriptArgument5(9, SCRIPT_READ_U8_REL(0xD));
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].colorDelta.b = FieldScriptArgument6(0xB, SCRIPT_READ_U8_REL(0xD));
    g_FieldScriptMaxInstructionCount += 4;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 0xE;
}

void FieldScriptParticlesInitialize(void) {
    g_FieldScriptMaxInstructionCount += 4;
    if (D_800ADB8C == 0) {
        FieldInitializeParticleBanks(D_800AFD1C);
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 1;
}

void FieldScriptStopParticleActor(void) {
    g_FieldScriptMaxInstructionCount += 4;
    FieldParticleActorStop(D_800AFD1C, SCRIPT_READ_U8_REL(0x1));
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}
// End of particle handlers

INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_80089B54);

INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_80089BF0);

INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_80089DCC);

extern FieldScriptGlobalS16 D_800B22E0;

void func_80089F18(void) {
    D_800B22E0.value = FieldScriptVMGetArgument(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

extern FieldScriptGlobalU8 D_800B21D2;

void func_80089F54(void) {
    D_800B21D2.value = FieldScriptVMGetArgument(1) - 0x80;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

extern s32 D_800AFE84;

void func_80089F94(void) {
    D_800AFE84 = SCRIPT_READ_U8_REL(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}

extern FieldScriptGlobalS16 D_800B0080;
extern FieldScriptGlobalS16 D_800B0082;
extern FieldScriptGlobalS16 D_800B0084;
extern FieldScriptGlobalS16 D_800B0086;
extern FieldScriptGlobalS16 D_800B0088;
extern FieldScriptGlobalS16 D_800B008A;
extern FieldScriptGlobalS16 D_800B008C;
extern FieldScriptGlobalS16 D_800B008E;

void func_80089FD0(void) {
    int value;

    D_800B0080.value = FieldScriptVMGetInstructionArgument(1);
    D_800B0082.value = FieldScriptVMGetInstructionArgument(3);
    value = FieldScriptVMGetInstructionArgument(5);
    D_800B0084.value = value;
    if ((short)value == 0) {
        D_800B0084.value = value + 1;
    }
    D_800B0086.value = FieldScriptVMGetInstructionArgument(7);
    D_800B0088.value = 0;
    D_800B008A.value = FieldScriptVMGetInstructionArgument(9);
    D_800B008C.value = FieldScriptVMGetInstructionArgument(0xB);
    D_800B008E.value = FieldScriptVMGetInstructionArgument(0xD);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 0xF;
}

extern FieldScriptGlobalS32 D_800B0090;
extern FieldScriptGlobalS32 D_800B0094;
extern FieldScriptGlobalS32 D_800B0098;

void func_8008A08C(void) {
    D_800B0090.value = FieldScriptArgument1(1, SCRIPT_READ_U8_REL(7));
    D_800B0098.value = FieldScriptArgument2(3, SCRIPT_READ_U8_REL(7));
    D_800B0094.value = FieldScriptArgument3(5, SCRIPT_READ_U8_REL(7));
    g_FieldScriptVMCurActor->scriptInstructionPointer += 8;
}

INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_8008A148);

extern s32 D_800ADB88;

void func_8008A244(void) {
    if (D_800ADB88 == 0) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 1;
    } else {
        g_FieldScriptVMCurActor->scriptInstructionPointer -= 1;
    }
    D_800B00C0 = 1;
}

extern s32 D_800B06A0;

void func_8008A2A0(void) {
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(1), D_800B06A0);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_8008A2E8);

void func_8008A4E0(void) {}
void func_8008A4E8(void) {}
void func_8008A4F0(void) {}
void func_8008A4F8(void) {}
void func_8008A500(void) {}
void func_8008A508(void) {}
void func_8008A510(void) {}
void func_8008A518(void) {}

void func_8008A520(void) {
    while (func_8008A558()) {
        Vsync(0);
    }
}

extern int ArchiveDataSync(void);
extern void ArchiveCdDataSync(int mode);

extern int D_800ADB2C;

int func_8008A558(void) {
    if (D_800ADB2C != 0) {
        return -1;
    }
    if (ArchiveDataSync() != 0) {
        return -1;
    }
    ArchiveCdDataSync(0);
    return 0;
}

extern void func_8003633C(int);

void func_8008A5A0(void) {
    if (SCRIPT_READ_U8_REL(1) == 0) {
        func_8003633C(0);
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 1;
}

extern FieldScriptGlobalS16 D_800B21D4;

void func_8008A604(void) {
    D_800B21D4.value = FieldScriptVMGetArgument(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_8008A640);

INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_8008A6E0);

int func_8008A790(int characterId, int* pPartyIndex) {
    int i;

    for (i = 0; i < MAX_PARTY_MEMBERS; i++) {
        if (g_GamePartyMembers[i] == characterId) {
            break;
        }
        if (g_GamePartyMembers[i] == CHARACTER_ID_NONE) {
            *pPartyIndex = i;
            return 0;
        }
    }
    return -1;
}

INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_8008A7DC);

void func_8008A93C(void) {
    g_FieldScriptVMCurActor->unkAnimationId = ~SCRIPT_READ_U8_REL(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}

void func_8008A974(void) {
    func_8008A93C();
    g_FieldScriptVMCurActor->flags &= 0xFFFEFFFF;
}

extern s32 D_800ADB90;

void func_8008A9AC(void) {
    if (func_8008A558() == 0) {
        D_800ADB90 = 0;
        SpriteSetSpecialAnimFile(g_FieldActors[D_800AFD1C].pSpriteData, g_FieldScriptVMCurActor->unk120);
        g_FieldScriptVMCurActor->scriptInstructionPointer += 1;
    } else {
        g_FieldScriptVMCurActor->scriptInstructionPointer -= 1;
    }
    D_800B00C0 = 1;
}

void func_8008AA60(void) {
    if (g_FieldScriptVMCurActor->unk124 != -1) {
        HeapFree(g_FieldScriptVMCurActor->unk120);
        g_FieldScriptVMCurActor->unk124 = -1;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 1;
    D_800B00C0 = 1;
}

INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_8008AACC);

INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_8008ACE8);

void func_8008AE5C(void) {
    if (SCRIPT_READ_U8_REL(1) == 0) {
        g_FieldScriptVMCurActor->flags |= 0x20000;
    } else {
        g_FieldScriptVMCurActor->flags &= ~0x20000;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}

INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_8008AEC8);

INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_8008AFD8);

extern FieldScriptGlobalU8 D_800B225C;
extern FieldScriptGlobalU8 D_800B225D;
extern FieldScriptGlobalU8 D_800B225E;

void func_8008B0E8(void) {
    D_800B225C.value = FieldScriptVMGetArgument(1);
    D_800B225D.value = FieldScriptVMGetArgument(3);
    D_800B225E.value = FieldScriptVMGetArgument(5);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 7;
}

extern FieldScriptGlobalS16 D_800B21B4;

void func_8008B144(void) {
    D_800B21B4.value = FieldScriptVMGetArgument(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

extern s32 D_800ADB1C;
extern s16 D_800B21E4[];

void func_8008B180(void) {
    if (D_800ADB1C != 0) {
        func_801E8330(FieldScriptVMGetArgument(1) & 0xFFFF, 0, FieldScriptVMGetArgument(3));
        D_800B21E4[FieldScriptVMGetArgument(1)] = FieldScriptVMGetArgument(3);
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}

void func_8008B210(void) {
    g_FieldScriptVMCurActor->unk11E = FieldScriptVMGetArgument(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

extern void FieldFadeSetParameters(int, int, int, int, int, int);

void func_8008B248(void) {
    FieldFadeSetParameters(
        1,
        FieldScriptVMGetArgument(9),
        FieldScriptVMGetArgument(3),
        FieldScriptVMGetArgument(5),
        FieldScriptVMGetArgument(7),
        FieldScriptVMGetArgument(1)
    );
    g_FieldScriptVMCurActor->scriptInstructionPointer += 0xB;
}

void func_8008B2F0(void) {
    FieldDistortionInitialize(0);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 0xF;
}

INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_8008B328);

extern FieldScriptGlobalS16 D_800B2184;
extern FieldScriptGlobalS16 D_800B2186;
extern FieldScriptGlobalS16 D_800B2188;

void func_8008B45C(void) {
    D_800B2184.value = FieldScriptArgument1(1, SCRIPT_READ_U8_REL(7));
    D_800B2188.value = FieldScriptArgument2(3, SCRIPT_READ_U8_REL(7));
    D_800B2186.value = FieldScriptArgument3(5, SCRIPT_READ_U8_REL(7));
    g_FieldScriptVMCurActor->scriptInstructionPointer += 8;
}

void func_8008B518(void) {
    g_Scene.camRotation.vx = FieldScriptArgument1(1, SCRIPT_READ_U8_REL(7));
    g_Scene.camRotation.vz = FieldScriptArgument2(3, SCRIPT_READ_U8_REL(7));
    g_Scene.camRotation.vy = FieldScriptArgument3(5, SCRIPT_READ_U8_REL(7));
    g_FieldScriptVMCurActor->scriptInstructionPointer += 8;
}

INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_8008B5D4);

INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_8008B894);

INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_8008B978);

INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_8008BC80);

INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_8008BDD8);

INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_8008BF38);

INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_8008C180);

INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_8008C334);

void func_8008C7D8(void) {
    if (g_FieldScriptVMCurActor->flags12C_0xD) {
        HeapFree(g_FieldScriptVMCurActor->unk114);
        g_FieldScriptVMCurActor->flags12C_0xD = 0;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 1;
}

INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_8008C84C);

INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_8008C938);

INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_8008CA60);

INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_8008CB4C);

extern s32 D_8004F36C;
extern s32 D_8004F324;
extern s32 D_80062528;
extern void func_8003AAC4(int, int);

void func_8008CC74(void) {
    if (D_8004F36C != 0) {
        func_8003AAC4(D_80062528, FieldScriptVMGetArgument(1));
        g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
    } else if (D_8004F324 == CHARACTER_ID_NONE) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
    } else if (D_800ADB1C == 0) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
    } else {
        g_FieldScriptVMCurActor->scriptInstructionPointer -= 1;
    }
    D_800B00C0 = 1;
}

INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_8008CD48);

INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_8008CDD4);

void func_8008CE64(void) {
    int partyId = func_8008CF3C(FieldScriptVMGetArgument(1));

    if (partyId != CHARACTER_ID_NONE) {
        g_pGameState->FrMask &= ~(1 << partyId);
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void func_8008CED0(void) {
    int partyId = func_8008CF3C(FieldScriptVMGetArgument(1));

    if (partyId != CHARACTER_ID_NONE) {
        g_pGameState->FrMask |= 1 << partyId;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

extern int func_8008CF3C(int);

// Maps a party slot id (0xFC-0xFF) to the character in that slot.
int func_8008CF3C(int characterId) {
    if (characterId == CHARACTER_ID_NONE) {
        return g_GamePartyMembers[2];
    }
    if (characterId == 0xFE) {
        return g_GamePartyMembers[1];
    }
    if (characterId == 0xFD) {
        return g_GamePartyMembers[0];
    }
    if (characterId == 0xFC) {
        return CHARACTER_ID_NONE;
    }
    return characterId;
}

void func_8008CF9C(void) {
    g_FieldScriptVMCurActor->faceId = func_8008CF3C(FieldScriptVMGetArgument(1));
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

extern FieldScriptGlobalS16 D_800B21A0;
extern FieldScriptGlobalS16 D_800B21A2;
extern FieldScriptGlobalS16 D_800B21A4;
extern FieldScriptGlobalS16 D_800B21A6;
extern FieldScriptGlobalS16 D_800B21A8;
extern FieldScriptGlobalS16 D_800B21AA;

void func_8008CFEC(void) {
    D_800B21A0.value = FieldScriptVMGetInstructionArgument(1);
    D_800B21A4.value = FieldScriptVMGetInstructionArgument(3);
    D_800B21A2.value = FieldScriptVMGetInstructionArgument(5);
    D_800B21A6.value = FieldScriptVMGetInstructionArgument(7);
    D_800B21AA.value = FieldScriptVMGetInstructionArgument(9);
    D_800B21A8.value = FieldScriptVMGetInstructionArgument(0xB);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 0xD;
}

void func_8008D078(void) {
    if (FieldScriptVMGetArgument(1) == 0) {
        g_FieldScriptVMCurActor->flags &= ~0x800;
    } else {
        g_FieldScriptVMCurActor->flags |= 0x800;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

extern void func_80072254(int);

void func_8008D0F4(void) {
    unsigned int scale = FieldScriptVMGetArgument(1);

    ((SpriteData*)g_FieldActors[D_800AFD1C].pSpriteData)->scale = (scale * 3) >> 2;
    g_FieldScriptVMCurActor->scaleX = scale;
    g_FieldScriptVMCurActor->scaleY = scale;
    g_FieldScriptVMCurActor->scaleZ = scale;
    func_80072254(D_800AFD1C);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void func_8008D180(void) {
    int scaleX = FieldScriptVMGetArgument(1);
    int scaleY = FieldScriptVMGetArgument(3);
    int scaleZ = FieldScriptVMGetArgument(5);

    ((SpriteData*)g_FieldActors[D_800AFD1C].pSpriteData)->scale = 0xC00;
    g_FieldScriptVMCurActor->scaleX = scaleX;
    g_FieldScriptVMCurActor->scaleY = scaleY;
    g_FieldScriptVMCurActor->scaleZ = scaleZ;
    func_80072254(D_800AFD1C);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 7;
}

extern FieldScriptGlobalS16 D_800B218C;

void func_8008D230(void) {
    D_800B218C.value = FieldScriptVMGetArgument(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void func_8008D26C(void) {
    ((SpriteData*)g_FieldActors[D_800AFD1C].pSpriteData)->field_0x82 = FieldScriptVMGetArgument(1) * 2;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void func_8008D2D8(void) {
}

void func_8008D2E0(int value, int offset) {
    ((u8*)g_FieldScriptVMCurScriptData)[offset + 1] = value >> 8;
    ((u8*)g_FieldScriptVMCurScriptData)[offset] = value;
}

long FieldGetVec2Magnitude(long, long);

int func_8008D30C(int dstActorId, int srcActorId) {
    FieldActor* actors = g_FieldActors;
    long distance;

    distance = FieldGetVec2Magnitude(
        actors[dstActorId].transformMatrix.t[0] - actors[srcActorId].transformMatrix.t[0],
        actors[dstActorId].transformMatrix.t[2] - actors[srcActorId].transformMatrix.t[2]
    );
    return -(distance >= 0x10);
}

static inline void CopySpriteFieldS32(FieldActor* actors, int dstId, int srcId, int offset) {
    void* dst = actors[dstId].pSpriteData;
    void* src = actors[srcId].pSpriteData;
    *(s32*)((u8*)dst + offset) = *(s32*)((u8*)src + offset);
}

static inline void CopySpriteFieldU16(FieldActor* actors, int dstId, int srcId, int offset) {
    void* dst = actors[dstId].pSpriteData;
    void* src = actors[srcId].pSpriteData;
    *(u16*)((u8*)dst + offset) = *(u16*)((u8*)src + offset);
}

static inline void CopyActorFieldS32(FieldActor* base, int dstId, int srcId, int offset) {
    *(s32*)((u8*)&base[dstId] + offset) = *(s32*)((u8*)&base[srcId] + offset);
}

void FieldActorCopyPlacement(int dstActorId, int srcActorId) {
    FieldActor* actors = g_FieldActors;
    ActorData* dstActor;
    ActorData* srcActor;
    int i;

    srcActor = actors[srcActorId].pActorData;
    dstActor = actors[dstActorId].pActorData;

    for (i = 0; i < 4; i++) {
        dstActor->walkmeshTriIds[i] = srcActor->walkmeshTriIds[i];
    }
    {
      s16 walkmeshId = srcActor->walkmeshId;
      dstActor->walkmeshId = walkmeshId;
    }
    dstActor->curTriNormal.vx = srcActor->curTriNormal.vx;
    dstActor->curTriNormal.vy = srcActor->curTriNormal.vy;
    dstActor->curTriNormal.vz = srcActor->curTriNormal.vz;
    dstActor->position.vx = srcActor->position.vx;
    dstActor->position.vy = srcActor->position.vy;
    dstActor->position.vz = srcActor->position.vz;
    dstActor->unkEC = srcActor->unkEC;
    dstActor->curYPos = srcActor->curYPos;
    dstActor->curWalkmeshTriMaterial = srcActor->curWalkmeshTriMaterial;

    CopySpriteFieldU16(g_FieldActors, dstActorId, srcActorId, offsetof(SpriteData, field_0x84));
    CopySpriteFieldS32(g_FieldActors, dstActorId, srcActorId, offsetof(SpriteData, position.x));
    CopySpriteFieldS32(g_FieldActors, dstActorId, srcActorId, offsetof(SpriteData, position.y));
    CopySpriteFieldS32(g_FieldActors, dstActorId, srcActorId, offsetof(SpriteData, position.z));

    CopyActorFieldS32(g_FieldActors, dstActorId, srcActorId, offsetof(FieldActor, transformMatrix.t[0]));
    CopyActorFieldS32(g_FieldActors, dstActorId, srcActorId, offsetof(FieldActor, transformMatrix.t[1]));
    CopyActorFieldS32(g_FieldActors, dstActorId, srcActorId, offsetof(FieldActor, transformMatrix.t[2]));
}

extern u8 D_800B219F;

void func_8008D570(void) {
    int i = 0;
    s32 actorIndex = D_800AFD1C;
    u8* pMask = &D_800B219F;

    for (; i < MAX_PARTY_MEMBERS; i++) {
        if (D_8005A444[i] == actorIndex) {
            *pMask &= ~(1 << i);
        }
    }
}

extern FieldScriptGlobalU8 D_800B21CD;

void func_8008D5C8(void) {
    D_800B21CD.value = SCRIPT_READ_U8_REL(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}

void func_8008D604(void) {
    switch (SCRIPT_READ_U8_REL(1)) {
        case 0:
            g_FieldScriptVMCurActor->flags &= ~0x400;
            break;
        case 1:
            g_FieldScriptVMCurActor->flags |= 0x400;
            break;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}

void func_8008D684(void) {
    int mask;
    unsigned int slotIndex;

    slotIndex = (unsigned int) (SCRIPT_IMM_ARG(1)) >> 4;
    mask = 1 << (FieldScriptVMGetInstructionArgument(1) & 0xF);
    FieldScriptMemoryWriteU16(slotIndex, FieldScriptVMGetVariableValue(slotIndex) | mask);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void func_8008D700(void) {
    int mask;
    unsigned int slotIndex;

    slotIndex = (unsigned int) (SCRIPT_IMM_ARG(1)) >> 4;
    mask = 1 << (FieldScriptVMGetInstructionArgument(1) & 0xF);
    FieldScriptMemoryWriteU16(slotIndex, FieldScriptVMGetVariableValue(slotIndex) & ~mask);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void func_8008D780(void) {
    short destination;
    int mask;
    unsigned int slotIndex;

    slotIndex = (unsigned int) (SCRIPT_IMM_ARG(1)) >> 4;
    mask = 1 << (FieldScriptVMGetInstructionArgument(1) & 0xF);
    if (FieldScriptVMGetVariableValue(slotIndex) & mask) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
    } else {
        destination = FieldScriptVMGetInstructionArgument(3);
        g_FieldScriptVMCurActor->scriptInstructionPointer = destination;
    }
}

INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_8008D808);

INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_8008DA04);

void func_8008DAFC(void) {
    g_FieldScriptVMCurActor->parentActorId = ACTOR_ID_INVALID;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 1;
}

extern FieldScriptGlobalS16 D_800B233E;

void func_8008DB2C(void) {
    D_800B233E.value = FieldScriptVMGetActorIndex(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}


int FieldPartyMemberIncreaseGearHp(int partyMemberIndex, unsigned int amount) {
    int gearId;
    unsigned int maxHp;
    unsigned int newHp;

    gearId = GameCharacterGetGearID(g_GamePartyMembers[partyMemberIndex]);
    if (gearId != CHARACTER_ID_NONE) {
        maxHp = g_pGameState->gears[gearId].maxHp;
        newHp = g_pGameState->gears[gearId].hp + amount;
        g_pGameState->gears[gearId].hp = newHp;
        if (maxHp < newHp) {
            g_pGameState->gears[gearId].hp = maxHp;
        }
    }
}

int FieldPartyMemberDecreaseGearHp(int partyMemberIndex, unsigned int amount) {
    int gearId;
    int newHp;

    gearId = GameCharacterGetGearID(g_GamePartyMembers[partyMemberIndex]);
    if (gearId != CHARACTER_ID_NONE) {
        newHp = g_pGameState->gears[gearId].hp - amount;
        if (newHp <= 0) {
            newHp = 1;
        }
        g_pGameState->gears[gearId].hp = newHp;
    }
}

extern s16 g_FieldNumPartyMembersMasks[4];

void FieldScriptVMHandlerIncreasePartyGearHp(void) {
    int mask;
    int amount;
    int i;

    amount = FieldScriptArgument1(1, SCRIPT_READ_U8_REL(3));
    mask = g_FieldNumPartyMembersMasks[SCRIPT_READ_U8_REL(3) & 0x3];

    for (i = 0; i < MAX_PARTY_MEMBERS; i++) {
        if ((g_GamePartyMembers[i] != CHARACTER_ID_NONE) && (mask & 1)) {
            FieldPartyMemberIncreaseGearHp(i, amount);
        }
        mask >>= 1;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 4;
}

void FieldScriptVMHandlerDecreasePartyGearHp(void) {
    int mask;
    int amount;
    int i;

    amount = FieldScriptArgument1(1, SCRIPT_READ_U8_REL(3));
    mask = g_FieldNumPartyMembersMasks[SCRIPT_READ_U8_REL(3) & 0x3];

    for (i = 0; i < MAX_PARTY_MEMBERS; i++) {
        if ((g_GamePartyMembers[i] != CHARACTER_ID_NONE) && (mask & 1)) {
            FieldPartyMemberDecreaseGearHp(i, amount);
        }
        mask >>= 1;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 4;
}

void FieldScriptSetParentActor(void) {
    int actorIndex = FieldScriptVMGetActorIndex(1);
    if (actorIndex != ACTOR_ID_INVALID) {
        g_FieldScriptVMCurActor->parentActorId = actorIndex;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}

INCLUDE_ASM("asm/field/nonmatchings/main/misc", FieldScriptWriteActorFlags1);
// 95.29%: needs g_FieldActors non-volatile for the last call to keep `li a0,1`
// in the delay slot, but removing the qualifier breaks another field function.
/*
void FieldScriptWriteActorFlags1(void) {
    ActorData* pActorData;
    int actorIndex;
    u_int scriptFlags;
    u_int address;

    actorIndex = FieldScriptVMGetActorIndex(1);
    if (actorIndex != ACTOR_ID_INVALID) {
        pActorData = g_FieldActors[actorIndex].pActorData;
        scriptFlags = pActorData->scriptFlags.flags;
        address = SCRIPT_IMM_ARG(1);
        FieldScriptMemoryWriteU16(address, scriptFlags);
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}
*/

INCLUDE_ASM("asm/field/nonmatchings/main/misc", FieldScriptWriteActorFlags2);
// 95.29% (same miss as Flags1): gcc hoists the `li a0,1` for the
// FieldScriptVMGetInstructionArgument call above the actor address computation;
// the original leaves it in the jal delay slot. Everything else is identical, so
// all four FlagsN handlers share this one blocker.
/*
void FieldScriptWriteActorFlags2(void) {
    volatile int actorIndex;
    ActorData* pActorData;

    actorIndex = FieldScriptVMGetActorIndex(1);
    if (actorIndex != ACTOR_ID_INVALID) {
        pActorData = g_FieldActors[actorIndex].pActorData;
        FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(1), ((u_short*)&pActorData->scriptFlags)[1]);
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}
*/

INCLUDE_ASM("asm/field/nonmatchings/main/misc", FieldScriptWriteActorFlags3);
/*
void FieldScriptWriteActorFlags3(void) {
    ActorData* pActorData;
    int actorIndex;

    actorIndex = FieldScriptVMGetActorIndex(1);
    if (actorIndex != ACTOR_ID_INVALID) {
        pActorData = g_FieldActors[actorIndex].pActorData;
        FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(1), pActorData->flags);
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}
*/

INCLUDE_ASM("asm/field/nonmatchings/main/misc", FieldScriptWriteActorFlags4);
/*
void FieldScriptWriteActorFlags4(void) {
    ActorData* pActorData;
    int actorIndex;

    actorIndex = FieldScriptVMGetActorIndex(1);
    if (actorIndex != ACTOR_ID_INVALID) {
        pActorData = g_FieldActors[actorIndex].pActorData;
        FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(1), ((u_short*)&pActorData->flags)[1]);
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}
*/

// TODO: These two handlers could use more semantic names since they're likely used
// in certain situations.
void FieldScriptVMConditionalJump6(unsigned short flag) {
    unsigned short argument = SCRIPT_IMM_ARG(1);
    if (argument & flag) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 6;
    } else {
        g_FieldScriptVMCurActor->scriptInstructionPointer = FieldScriptVMGetInstructionArgument(4);
    }
}

void FieldScriptVMConditionalJump5(unsigned short flag) {
    unsigned short argument = SCRIPT_IMM_ARG(1);
    if (argument & flag) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
    } else {
        g_FieldScriptVMCurActor->scriptInstructionPointer = SCRIPT_IMM_ARG(2);
    }
}

INCLUDE_ASM("asm/field/nonmatchings/main/misc", FieldScriptWriteActorDistance);
/*
Matches as long as g_FieldActors is NOT volatile.

void FieldScriptWriteActorDistance(void) {
    ActorData* pActorA;
    ActorData* pActorB;
    int actorIndexA;
    int actorIndexB;
    int distance;

    distance = 0;
    actorIndexA = FieldScriptVMGetActorIndex(3);
    actorIndexB = FieldScriptVMGetActorIndex(4);
    if ((actorIndexA != ACTOR_ID_INVALID) && (actorIndexB != ACTOR_ID_INVALID)) {
        pActorA = g_FieldActors[actorIndexA].pActorData;
        pActorB = g_FieldActors[actorIndexB].pActorData;
        distance = FieldGetVec2Magnitude(
            CONV_TO_GTE(pActorA->position.vx) - CONV_TO_GTE(pActorB->position.vx), 
            CONV_TO_GTE(pActorA->position.vz) - CONV_TO_GTE(pActorB->position.vz)
        );
    }
    FieldScriptMemoryWriteU16(
        SCRIPT_IMM_ARG(1), 
        distance
    );
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}
*/

// The four halves of an actor's scriptFlags and flags words are tested by the script VM.
void func_8008E298(void) {
    FieldScriptVMConditionalJump6(((u_short*)g_FieldActors[FieldScriptVMGetActorIndex(3)].pActorData)[0]);
}

void func_8008E2EC(void) {
    FieldScriptVMConditionalJump6(((u_short*)g_FieldActors[FieldScriptVMGetActorIndex(3)].pActorData)[1]);
}

void func_8008E340(void) {
    FieldScriptVMConditionalJump6(((u_short*)g_FieldActors[FieldScriptVMGetActorIndex(3)].pActorData)[2]);
}

void func_8008E394(void) {
    FieldScriptVMConditionalJump6(((u_short*)g_FieldActors[FieldScriptVMGetActorIndex(3)].pActorData)[3]);
}

void func_8008E3E8(void) {
    FieldScriptVMConditionalJump5(((u_short*)g_FieldScriptVMCurActor)[0]);
}

void func_8008E414(void) {
    FieldScriptVMConditionalJump5(((u_short*)g_FieldScriptVMCurActor)[1]);
}

void func_8008E440(void) {
    FieldScriptVMConditionalJump5(((u_short*)g_FieldScriptVMCurActor)[2]);
}

void func_8008E46C(void) {
    FieldScriptVMConditionalJump5(((u_short*)g_FieldScriptVMCurActor)[3]);
}

void func_8008E498(unsigned int flag) {
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(1), (unsigned short)flag);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void func_8008E4EC(void) {
    func_8008E498(((u_short*)g_FieldScriptVMCurActor)[0]);
}

void func_8008E518(void) {
    func_8008E498(((u_short*)g_FieldScriptVMCurActor)[1]);
}

void func_8008E544(void) {
    func_8008E498(((u_short*)g_FieldScriptVMCurActor)[2]);
}

void func_8008E570(void) {
    func_8008E498(((u_short*)g_FieldScriptVMCurActor)[3]);
}

INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_8008E59C);

INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_8008E718);

extern FieldScriptGlobalS32 D_800B2298;
extern FieldScriptGlobalS32 D_800B229C;

void func_8008E85C(void) {
    int count;

    D_800B2298.value = FieldScriptVMGetArgument(1);
    count = FieldScriptVMGetArgument(3);
    D_800B229C.value = count;
    if (count >= 0x21) {
        D_800B229C.value = 0x20;
    }
    func_8008E718();
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}

INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_8008E8C8);

extern s32 D_800ADB7C;

void func_8008E9F8(void) {
    if (D_800ADB7C == 0) {
        g_FieldScriptVMCurActor->scriptInstructionPointer -= 1;
    } else {
        D_800ADB7C = 0;
        g_FieldScriptVMCurActor->scriptInstructionPointer += 1;
    }
    D_800B00C0 = 1;
}

INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_8008EA58);

INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_8008EC30);

INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_8008EE14);

extern s32 D_800ADB3C;
extern s32 D_800ADB38;

void func_8008EF5C(void) {
    D_800ADB3C = FieldScriptVMGetArgument(1);
    D_800ADB38 = 1;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void func_8008EFA0(void) {
    D_800ADB3C = FieldScriptVMGetArgument(1);
    D_800ADB38 = 2;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void func_8008EFE4(void) {
    FieldSetClipDimensions(
        FieldScriptVMGetArgument(1),
        FieldScriptVMGetArgument(3),
        FieldScriptVMGetArgument(5),
        FieldScriptVMGetArgument(7)
    );
    g_FieldScriptVMCurActor->scriptInstructionPointer += 9;
}

void func_8008F070(void) {
    D_800ADB3C = FieldScriptVMGetArgument(1);
    D_800ADB38 = 3;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_8008F0B4);

INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_8008F1C8);

extern void func_80023290(void*, int);

void func_8008F2D8(void) {
    func_80023290(g_FieldActors[D_800AFD1C].pSpriteData, FieldScriptVMGetArgument(1));
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

extern s32 D_800C3A5C;
extern s32 D_800C3A60;

void func_8008F348(void) {
    D_800C3A5C = FieldScriptVMGetArgument(1);
    D_800C3A60 = FieldScriptVMGetArgument(3);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}

extern FieldScriptGlobalS16 D_800B233C;

void func_8008F394(void) {
    D_800B233C.value = FieldScriptVMGetArgument(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

extern void func_8003A450(int, int, int);

void func_8008F3D0(void) {
    func_8003A450(FieldScriptVMGetArgument(3) * 2, FieldScriptVMGetArgument(1), FieldScriptVMGetArgument(5));
    g_FieldScriptVMCurActor->scriptInstructionPointer += 7;
}

extern void func_8003A344(int, int);

void func_8008F444(void) {
    func_8003A344(FieldScriptVMGetArgument(3) * 2, FieldScriptVMGetArgument(1));
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}

extern void func_8003A55C(int, int);

void func_8008F4A0(void) {
    func_8003A55C(FieldScriptVMGetArgument(3) * 2, FieldScriptVMGetArgument(1));
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}

void func_8008F4FC(void) {
    func_80085634(FieldScriptVMGetArgument(1), FieldScriptVMGetArgument(3));
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}

void func_8008F558(void) {
    func_800855C8(
        FieldScriptVMGetArgument(1),
        FieldScriptVMGetArgument(5),
        FieldScriptVMGetArgument(3),
        FieldScriptVMGetArgument(7)
    );
    g_FieldScriptVMCurActor->scriptInstructionPointer += 9;
}

extern int func_8003A5D0(int);

void func_8008F5E4(void) {
    int inputMask = func_8003A5D0(-1);

    if ((inputMask & (FieldScriptVMGetArgument(1) << 8)) == 0) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
    } else {
        g_FieldScriptVMCurActor->scriptInstructionPointer -= 1;
    }
    D_800B00C0 = 1;
}

extern void func_80085634(int, int);

void func_8008F668(void) {
    func_80085634(FieldScriptVMGetArgument(1), 3);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

extern void func_800855C8(int, int, int, int);

void func_8008F6AC(void) {
    func_800855C8(FieldScriptVMGetArgument(1), FieldScriptVMGetArgument(5), FieldScriptVMGetArgument(3), 3);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 7;
}

extern FieldScriptGlobalS32 D_800ADBDC;
extern FieldScriptGlobalS32 D_8004F340;

void func_8008F724(void) {
    if (D_800ADBDC.value == 0) {
        D_800B00C0 = 1;
    } else {
        D_8004F340.value = 0;
        func_8008F7B8();
    }
}

void func_8008F76C(void) {
    if (D_800ADBDC.value == 0) {
        D_800B00C0 = 1;
    } else {
        D_8004F340.value = -1;
        func_8008F7B8();
    }
}

INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_8008F7B8);

INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_8008F90C);