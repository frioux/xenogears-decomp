#include "common.h"
#include "main/game.h"
#include "system/math.h"
#include "field/script_vm.h"
#include "field/actor.h"
#include "field/text_box.h"
#include "field/main.h"

extern void func_80076AC0(s32, s32, void*, s32, s32, s32, s32);
extern void func_800A0C94();
extern int ArchiveGetDiscNumber(void);
extern void FieldScriptMemoryWriteU16(int, int);
extern s32 D_800AFD1C; // Current actor index
extern s32 D_800B2264;
extern u8 D_800B225F[];
extern s16 D_800B21DC[];
extern s32 D_800B2268;
extern int g_GameSceneMapNum;
extern void GameWaitForCdData(void);
extern void GamePartySyncSkinData(void);
extern void GamePartySyncStreamedData(void);
extern int func_8008CF3C(int);
extern void* g_PartyDataBuffers[];
extern s32 D_800AFFEC;
extern s16 D_800AFD20;

void FieldScriptVMHandlerDisableDialogActivation(void) {
    g_FieldScriptVMCurActor->scriptFlags.fields.scriptFlags_0xA = 0x1;
    g_FieldScriptVMCurActor->scriptInstructionPointer++;
}

void FieldScriptVMHandlerEnableDialogActivation(void) {
    g_FieldScriptVMCurActor->scriptFlags.fields.scriptFlags_0xA = 0x0;
    g_FieldScriptVMCurActor->scriptInstructionPointer++;
}

void func_8009DA70(void) {
    g_FieldScriptVMCurActor->scriptFlags.fields.scriptFlags_0x16 = 0x1;
    g_FieldScriptVMCurActor->scriptInstructionPointer++;
}

void func_8009DA98(void) {
    g_FieldScriptVMCurActor->scriptFlags.fields.scriptFlags_0x16 = 0;
    g_FieldScriptVMCurActor->scriptInstructionPointer++;
}

void func_8009DAC4(void) {
    int nTextBoxIndex;
    ActorData* pActor;
    FieldActor* pFieldActor;

    if (FieldScriptVMGetActorIndex(1) != ACTOR_ID_INVALID) {
        pActor = g_FieldActors[FieldScriptVMGetActorIndex(1)].pActorData;
        pActor->scriptFlags.fields.scriptFlags_0x0 = 0x1;
        pActor->flags |= 0x100000;
        pFieldActor = &g_FieldActors[FieldScriptVMGetActorIndex(1)];
        pFieldActor->status |= ACTOR_STATUS_INVISIBLE;
        if (func_8009CD18(&nTextBoxIndex) == 0) {
            g_FieldTextBoxes[nTextBoxIndex].status = 0;
        }
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}

void FieldScriptVMHandlerEnableActorVM(void) {
    ActorData* pActor;

    if (FieldScriptVMGetActorIndex(1) != ACTOR_ID_INVALID) {
        pActor = g_FieldActors[FieldScriptVMGetActorIndex(1)].pActorData;
        pActor->scriptFlags.fields.scriptFlags_0x0 = 0;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}

void func_8009DC4C(void) {
    ActorData* pActor;
    int nTextBoxIndex;
    short nNewRotation;

    if (FieldScriptVMGetActorIndex(1) != ACTOR_ID_INVALID) {
        pActor = g_FieldActors[FieldScriptVMGetActorIndex(1)].pActorData;
        pActor->moveModified.vx = 0;
        pActor->moveModified.vy = 0;
        pActor->moveModified.vz = 0;
        pActor->move.vx = 0;
        pActor->move.vy = 0;
        pActor->move.vz = 0;
        pActor->scriptFlags.fields.scriptFlags_0x0 = 1;
        nNewRotation = pActor->rotation.vx | 0x8000;
        pActor->rotation.vy = nNewRotation;
        pActor->rotation.vx = nNewRotation;
        if (func_8009CD18(&nTextBoxIndex) == 0) {
            g_FieldTextBoxes[nTextBoxIndex].status = 0;
        }
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}

void FieldScriptVMHandlerSleep(void) {
    u_char nScriptId;

    nScriptId = g_FieldScriptVMCurActor->curScriptIndex;

    // Initialize the value with the argument as the time to sleep, else count down
    if (g_FieldScriptVMCurActor->scripts[nScriptId].waitTimer == 0) {
        g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].waitTimer = FieldScriptVMGetArgument(1);
    } else {
        g_FieldScriptVMCurActor->scripts[nScriptId].waitTimer--;
    }
    
    // When the timer has reached 0, we move to the next instruction
    if (g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].waitTimer == 0) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
    }

    D_800B00C0 = 1;
}

void FieldScriptVMHandlerShowActorById(void) {
    FieldActor* pFieldActor;

    if (FieldScriptVMGetActorIndex(1) != ACTOR_ID_INVALID) {
        pFieldActor = &g_FieldActors[FieldScriptVMGetActorIndex(1)];
        if (!(pFieldActor->pActorData->flags & 0x100000)) {
            pFieldActor->status &= ~ACTOR_STATUS_INVISIBLE;
            pFieldActor->pActorData->flags &= ~0x2000000;
        }
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}

void FieldScriptVMHandlerHideActorById(void) {
    FieldActor* pFieldActor;

    if (FieldScriptVMGetActorIndex(1) != ACTOR_ID_INVALID) {
        pFieldActor = &g_FieldActors[FieldScriptVMGetActorIndex(1)];
        pFieldActor->status |= ACTOR_STATUS_INVISIBLE;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}

void FieldScriptVMHandlerShowActor(void) {
    FieldActor* pActor = &g_FieldActors[D_800AFD1C];
    pActor->status &= ~ACTOR_STATUS_INVISIBLE;
    g_FieldScriptVMCurActor->curAnimationId = 0xFF;
    g_FieldScriptVMCurActor->flags &= 0xFDFFFFFF;
    g_FieldScriptVMCurActor->scriptInstructionPointer++;
}

void func_8009DF78(void) {
    FieldActor* pFieldActor;

    if (FieldScriptVMGetActorIndex(1) != ACTOR_ID_INVALID) {
        pFieldActor = &g_FieldActors[FieldScriptVMGetActorIndex(1)];
        pFieldActor->pActorData->flags |= 0x02000000;
        pFieldActor->pActorData->flags |= 0x800;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}

void func_8009E014(void) {
    g_FieldScriptVMCurActor->flags |= 0x02000800;
    g_FieldScriptVMCurActor->scriptInstructionPointer++;
}

void FieldScriptVMHandlerHideActor(void) {
    FieldActor* pActor = &g_FieldActors[D_800AFD1C];
    pActor->status |=  ACTOR_STATUS_INVISIBLE;
    g_FieldScriptVMCurActor->scriptInstructionPointer++;
}

// Update sprite movement or animation speed?
void func_8009E094(void) {
    unsigned short nMoveSpeed;

    nMoveSpeed = FieldScriptVMGetArgument(1);
    g_FieldScriptVMCurActor->moveSpeed = nMoveSpeed;
    func_80021BCC(g_FieldActors[D_800AFD1C].pSpriteData, nMoveSpeed);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void func_8009E10C(void) {
    int nValue;
    int nFlags;

    nValue = FieldScriptVMGetArgument(1);
    nFlags = (nValue & 0x1) << 7;
    if (nValue & 0x4) {
        nFlags |= 0x20;
    }
    if (nValue & 0x8) {
        nFlags |= 0x10;
    }
    if (nValue & 0x10) {
        nFlags |= 0x8;
    }
    if (nValue & 0x20) {
        nFlags |= 0x4;
    }
    if (nValue & 0x40) {
        nFlags |= 0x8000000;
    }
    g_FieldScriptVMCurActor->scriptFlags.flags &= 0xF7FFFF43;
    g_FieldScriptVMCurActor->scriptFlags.flags |= nFlags;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void func_8009E1A0(void) {
    u32 nValue;

    nValue = SCRIPT_READ_U8_REL(1) & 7;
    g_FieldScriptVMCurActor->flags &= ~7;
    g_FieldScriptVMCurActor->flags |= nValue;
    
    nValue = (SCRIPT_READ_U8_REL(1) >> 1) & 0x38;
    g_FieldScriptVMCurActor->flags &= ~0x38;
    g_FieldScriptVMCurActor->flags |= nValue;
    
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}

void func_8009E208(void) {
    g_FieldScriptVMCurActor->unkEC = 0;
    g_FieldScriptVMCurActor->scriptFlags.fields.scriptFlags_0x10 = 0x0;
    g_FieldScriptVMCurActor->scriptFlags.fields.scriptFlags_0x15 = 0x1;
    g_FieldScriptVMCurActor->scriptInstructionPointer++;
    g_FieldScriptVMCurActor->curYPos = CONV_TO_GTE(g_FieldScriptVMCurActor->position.vy);
}

void func_8009E248(void) {
    func_8009E574(
        FieldScriptVMGetInstructionArgumentS16(1),
        FieldScriptVMGetInstructionArgumentS16(3)
    );
    func_8009E810(FieldScriptVMGetInstructionArgumentS16(5));
    g_FieldScriptVMCurActor->scriptFlags.fields.scriptFlags_0x10 = 0x1;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 7;
}

void func_8009E2C8(void) {
    func_8009E810(FieldScriptArgument1(1, SCRIPT_READ_U8_REL(3)));
    g_FieldScriptVMCurActor->scriptFlags.fields.scriptFlags_0x10 = 0x1;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 4;
}

// Read short from bytecode, absolute offset
short func_8009E330(int offset) {
    return SCRIPT_READ_U8(offset) + (SCRIPT_READ_U8(offset + 1) << 8);
}

void func_8009E35C(void) {
    g_FieldScriptVMCurActor->walkmeshId = SCRIPT_READ_U8_REL(5);
    func_8009E574(
        FieldScriptArgument1(1, SCRIPT_READ_U8_REL(6)), 
        FieldScriptArgument2(3, SCRIPT_READ_U8_REL(6))
    );
    g_FieldScriptVMCurActor->flags &= ~0x200000;
    g_FieldScriptVMCurActor->scriptFlags.fields.scriptFlags_0xX = 0x0;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 7;
}

void func_8009E428(void) {
    ActorData* pActor;

    g_FieldScriptVMCurActor->walkmeshId = SCRIPT_READ_U8_REL(1);
    pActor = g_FieldActors[D_800AFD1C].pActorData;
    func_8009E574(
        CONV_TO_GTE(pActor->position.vx), 
        CONV_TO_GTE(pActor->position.vz)
    );
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}

void func_8009E4BC(void) {
    func_8009E574(
        FieldScriptArgument1(1, SCRIPT_READ_U8_REL(5)), 
        FieldScriptArgument2(3, SCRIPT_READ_U8_REL(5))
    );
    g_FieldScriptVMCurActor->flags &= ~0x200000;
    g_FieldScriptVMCurActor->scriptFlags.fields.scriptFlags_0xX = 0x0;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 6;
}


typedef struct {
    /* 0x0 */ u16 f0;
    /* 0x2 */ u16 f1;
    /* 0x4 */ u16 f2;
    /* 0x6 */ u16 f3;
    /* 0x8 */ u16 f4;
    /* 0xA */ u16 f5;
    /* 0xC */ u16 f6;
    /* 0xE */ u16 f7;
} FieldScriptArgs; // Size: 0x10

// 73.71% (632 vs 664 bytes), first shape tried on this stub (no prior park).
// Frame (0xA0, seven saves), the loop bounds and both calls match. What is left:
// (a) retail does not keep &g_FieldActors[D_800AFD1C] in a register - it
// re-derives the address at each of the six transform/childMatrix stores, so the
// source probably spells g_FieldActors[D_800AFD1C].transformMatrix.t[n] inline
// and keeps only pSpriteData in a local; my extra live pointer shifts the args
// to s5/s6 and pSpriteData to s4 (retail: s4/s5 and s3).
// (b) loop body: retail computes i * 2 for walkmeshTriIds[i] after the call and
// bumps the points pointer in the bnez delay slot; gcc hoists both.
// Next: inline the g_FieldActors[...] expressions, and give the call result a
// temp before the walkmeshTriIds store.
/*
extern s16 D_800AFB54;
extern int func_8007B1C4(int x, int z, int walkmeshId, SVECTOR* pOut, VECTOR* pNormal);
extern u_int func_80080968(ActorData* pActor);

void func_8009E574(s32 nX, s32 nY) {
    VECTOR normals[4];
    SVECTOR points[4];
    SpriteData* pSprite;
    FieldActor* pFieldActor;
    ActorData* pActor;
    int i;

    pFieldActor = &g_FieldActors[D_800AFD1C];
    pSprite = (SpriteData*)pFieldActor->pSpriteData;
    for (i = 0; i < D_800AFB54 - 1; i++) {
        g_FieldScriptVMCurActor->walkmeshTriIds[i] = func_8007B1C4(nX, nY, i, &points[i], &normals[i]);
    }

    g_FieldScriptVMCurActor->curWalkmeshTriMaterial = func_80080968(g_FieldScriptVMCurActor);
    pActor = g_FieldScriptVMCurActor;
    pActor->curTriNormal.vx = normals[pActor->walkmeshId].vx;
    pActor->curTriNormal.vy = normals[pActor->walkmeshId].vy;
    pActor->curTriNormal.vz = normals[pActor->walkmeshId].vz;
    pFieldActor->transformMatrix.t[0] = nX;
    pFieldActor->childMatrix.t[0] = nX;
    pFieldActor->transformMatrix.t[1] = points[pActor->walkmeshId].vy;
    pFieldActor->childMatrix.t[1] = points[pActor->walkmeshId].vy;
    pFieldActor->transformMatrix.t[2] = nY;
    pFieldActor->childMatrix.t[2] = nY;
    pSprite->field_0x84 = points[pActor->walkmeshId].vy;
    pActor->position.vx = nX << 16;
    pActor->position.vz = nY << 16;
    pActor->position.vy = points[pActor->walkmeshId].vy << 16;
    pActor->curYPos = points[pActor->walkmeshId].vy;
    pSprite->position.x = pActor->position.vx;
    pSprite->position.y = pActor->position.vy;
    pSprite->position.z = pActor->position.vz;
    pActor->move.vx = 0;
    pActor->move.vy = 0;
    pActor->move.vz = 0;
    pActor->moveModified.vx = 0;
    pActor->moveModified.vy = 0;
    pActor->moveModified.vz = 0;
    pActor->unkD0.vx = 0;
    pActor->unkD0.vy = 0;
    pActor->unkD0.vz = 0;
    pActor->unk60.vx = 0;
    pActor->unk60.vy = 0;
    pActor->unk60.vz = 0;
    pSprite->step.x = 0;
    pSprite->step.y = 0;
    pSprite->step.z = 0;
    pActor->unkF0 = 0;
    pActor->unkEC = 0;
    pActor->curYPos = ((u16*)&pActor->position)[3];
    pActor->flags = (pActor->flags & ~0x40000) | 0x400000;
}
*/
INCLUDE_ASM("asm/field/nonmatchings/main/misc6", func_8009E574);

// Writes the actor's Y position (fixed point) and the cached short copies
void func_8009E810(short nY) {
    u8 unused[0x18]; // original has an unused stack slot; reproduces the frame

    g_FieldScriptVMCurActor->position.vy = nY * 0x10000;
    g_FieldScriptVMCurActor->unkEC = nY;
    g_FieldScriptVMCurActor->curYPos = nY;
}

void func_8009E83C(void) {
    unsigned char width;
    unsigned char zWidth;
    unsigned char height;
    unsigned char solidRange;

    width = SCRIPT_READ_U8_REL(1);
    if (width) {
        g_FieldScriptVMCurActor->width = width * 2;
    }
    
    zWidth = SCRIPT_READ_U8_REL(2);
    if (zWidth) {
        g_FieldScriptVMCurActor->zWidth = zWidth * 2;
    }
    
    height = SCRIPT_READ_U8_REL(3);
    if (height) {
        g_FieldScriptVMCurActor->height = height * 2;
    }
    
    solidRange = SCRIPT_READ_U8_REL(4);
    if (solidRange) {
        g_FieldScriptVMCurActor->solidRange = SCRIPT_READ_U8_REL(4) * 2;
    }
    
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}

void func_8009E91C(void) {
    if (!g_FieldScriptVMCurActor->flags12C_0xD) {
        g_FieldScriptVMCurActor->unk114 = HeapAlloc(0x10, 0x0);
    }
    g_FieldScriptVMCurActor->flags12C_0xD = 1;
    ((FieldScriptArgs*)g_FieldScriptVMCurActor->unk114)->f0 = FieldScriptArgument1(1, SCRIPT_READ_U8_REL(0x11));
    ((FieldScriptArgs*)g_FieldScriptVMCurActor->unk114)->f1 = FieldScriptArgument2(3, SCRIPT_READ_U8_REL(0x11));
    ((FieldScriptArgs*)g_FieldScriptVMCurActor->unk114)->f2 = FieldScriptArgument3(5, SCRIPT_READ_U8_REL(0x11));
    ((FieldScriptArgs*)g_FieldScriptVMCurActor->unk114)->f3 = FieldScriptArgument4(7, SCRIPT_READ_U8_REL(0x11));
    ((FieldScriptArgs*)g_FieldScriptVMCurActor->unk114)->f4 = FieldScriptArgument5(9, SCRIPT_READ_U8_REL(0x11));
    ((FieldScriptArgs*)g_FieldScriptVMCurActor->unk114)->f5 = FieldScriptArgument6(0xB, SCRIPT_READ_U8_REL(0x11));
    ((FieldScriptArgs*)g_FieldScriptVMCurActor->unk114)->f6 = FieldScriptArgument7(0xD, SCRIPT_READ_U8_REL(0x11));
    ((FieldScriptArgs*)g_FieldScriptVMCurActor->unk114)->f7 = FieldScriptArgument8(0xF, SCRIPT_READ_U8_REL(0x11));
    g_FieldScriptVMCurActor->scriptInstructionPointer += 0x12;
}

// -1: Actor has a script slot occupied w/ target ID
//  0: Actor has no script slot w/ target ID
int FieldActorGetScriptStatus(ActorData* pActor, int targetId) {
    int i;

    for (i = 0; i < ACTOR_MAX_NUM_SCRIPTS; i++) {
        if (targetId == pActor->scripts[i].scriptId) {
            return -1;
        }
    }
    return 0;
}

void func_8009EB78(void) {
    ActorData* pActor;
    int actorIndex;
    int i;
    int curScriptId;

    if (FieldScriptVMGetActorIndex(1) != 0xFF) {
        actorIndex = FieldScriptVMGetActorIndex(1);
        pActor = g_FieldActors[actorIndex].pActorData;
        if (pActor->flags & 0x100000) {
            curScriptId = g_FieldScriptVMCurActor->curScriptIndex;
            g_FieldScriptVMCurActor->scripts[curScriptId].state = SCRIPT_STATE_IDLE;
            pActor->scripts[g_FieldScriptVMCurActor->unkCF].isInUse = 0;
        } else if (FieldActorGetScriptStatus(pActor, SCRIPT_READ_U8_REL(2) & 0x1F) != ACTOR_SCRIPT_EXISTS) {
            for (i = 0; i < ACTOR_MAX_NUM_SCRIPTS; i++) {
                if (pActor->scripts[i].flags_0x12 != 0xF || pActor->scripts[i].isInUse) {
                    continue;
                }
                
                pActor->scripts[i].currentIP = FieldScriptGetBytecodeOffset(actorIndex, SCRIPT_READ_U8_REL(2) & 0x1F);
                pActor->scripts[i].flags_0x12 = SCRIPT_READ_U8_REL(2) >> 0x5;
                pActor->scripts[i].scriptId = SCRIPT_READ_U8_REL(2) & 0x1F;
                g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
                return;
            }
            return;
        }
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void func_8009ED68(void) {
    ActorData* pActor;
    int actorIndex;
    int i;

    if (FieldScriptVMGetActorIndex(1) == ACTOR_ID_INVALID) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
        return;
    }
    
    actorIndex = FieldScriptVMGetActorIndex(1);
    pActor = g_FieldActors[actorIndex].pActorData;
    if (pActor->flags & 0x100000) {
        g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].state = SCRIPT_STATE_IDLE;
        pActor->scripts[g_FieldScriptVMCurActor->unkCF].isInUse = 0;
    } else {
        switch (g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].state) {
        case 0:
            if (FieldActorGetScriptStatus(pActor, SCRIPT_READ_U8_REL(2) & 0x1F) == ACTOR_SCRIPT_EXISTS) {
                break;
            }
            
            for (i = 0; i < ACTOR_MAX_NUM_SCRIPTS; i++) {
                if (pActor->scripts[i].flags_0x12 != 0xF || pActor->scripts[i].isInUse) {
                    continue;
                }
                pActor->scripts[i].currentIP = FieldScriptGetBytecodeOffset(actorIndex, SCRIPT_READ_U8_REL(2) & 0x1F);
                pActor->scripts[i].flags_0x12 = SCRIPT_READ_U8_REL(2) >> 5;
                pActor->scripts[g_FieldScriptVMCurActor->unkCF].isInUse = 1;
                pActor->scripts[i].scriptId = SCRIPT_READ_U8_REL(2) & 0x1F;
                g_FieldScriptVMCurActor->unkCF = i;
                g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].state = 1;
                break;
            }
            return;
        case 1:
            if ((pActor->curScriptIndex == g_FieldScriptVMCurActor->unkCF) || pActor->scripts[g_FieldScriptVMCurActor->unkCF].flags_0x12 == 0xF) {
                g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
                g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].state = SCRIPT_STATE_IDLE;
                pActor->scripts[g_FieldScriptVMCurActor->unkCF].isInUse = 0;
            } else {
                D_800B00C0 = g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].state;
            }
            return;
        default:
            return;
        }
    }
    
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void func_8009F0A0(void) {
    ActorData* pActor;
    s32 actorIndex;
    int i;

    if (FieldScriptVMGetActorIndex(1) == ACTOR_ID_INVALID) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
        return;
    }
    
    actorIndex = FieldScriptVMGetActorIndex(1);
    pActor = g_FieldActors[actorIndex].pActorData;
    if (pActor->flags & 0x100000) {
        g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].state = SCRIPT_STATE_IDLE;
        pActor->scripts[g_FieldScriptVMCurActor->unkCF].isInUse = 0;
    } else {
        switch (g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].state) {
        case 0:
            if (FieldActorGetScriptStatus(pActor, SCRIPT_READ_U8_REL(2) & 0x1F) == ACTOR_SCRIPT_EXISTS) {
                g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
                break;
            }
            
            for (i = 0; i < ACTOR_MAX_NUM_SCRIPTS; i++) {
                if (pActor->scripts[i].flags_0x12 != 0xF || pActor->scripts[i].isInUse) {
                    continue;
                }
                pActor->scripts[i].currentIP = FieldScriptGetBytecodeOffset(actorIndex, SCRIPT_READ_U8_REL(2) & 0x1F);
                pActor->scripts[i].flags_0x12 = SCRIPT_READ_U8_REL(2) >> 5;
                pActor->scripts[g_FieldScriptVMCurActor->unkCF].isInUse = 1;
                g_FieldScriptVMCurActor->unkCF = i;
                g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].state = 1;
                pActor->scripts[i].scriptId = SCRIPT_READ_U8_REL(2) & 0x1F;
                return;
            }
            break;
        case 1:
            if ((pActor->curScriptIndex == g_FieldScriptVMCurActor->unkCF) || pActor->scripts[g_FieldScriptVMCurActor->unkCF].flags_0x12 == 0xF) {
                g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].state = 2;
                return;
            }
            D_800B00C0 = 1;
            break;
        case 2:
            if (pActor->scripts[g_FieldScriptVMCurActor->unkCF].flags_0x12 == 0xF) {
                g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].state = SCRIPT_STATE_IDLE;
                pActor->scripts[g_FieldScriptVMCurActor->unkCF].isInUse = 0;
                g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
                return;
            }
            D_800B00C0 = 1;
            break;
        }
        return;
    }
    
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

// Randomize X rotation
void func_8009F424(void) {
    int nRotation;
    
    nRotation = g_FieldScriptVMCurActor->rotation.vy;
    g_FieldScriptVMCurActor->unk102++;
    if (!(g_FieldScriptVMCurActor->unk102 & 0xF)) {
        if (!(rand() & 0x1)) {
            nRotation = PSX_ANGLE(g_FieldScriptVMCurActor->rotation.vy + PSX_DEGREES(45));
        } else {
            nRotation = PSX_ANGLE(g_FieldScriptVMCurActor->rotation.vy - PSX_DEGREES(45));
        }
    }
    D_800B00C0 = 1;
    g_FieldScriptVMCurActor->rotation.vx = nRotation;
    g_FieldScriptVMCurActor->scriptInstructionPointer++;
}

void func_8009F4CC(void) {
    short nSpecialRotation;
    int nRotation;
    int nRand;
    int nDeltaRotation;
    int delta;

    nRotation = g_FieldScriptVMCurActor->rotation.vy;
    g_FieldScriptVMCurActor->unk102++;
    if (!(g_FieldScriptVMCurActor->unk102 & 0xF)) {
        nRand = rand();
        if (nRand & 0x30) {
            nSpecialRotation = g_FieldScriptVMCurActor->rotation.vy | ~0x7FFF;
            nRotation = nSpecialRotation;
            g_FieldScriptVMCurActor->rotation.vy = nSpecialRotation;
        } else {
            delta = PSX_DEGREES(45);
            if (!(nRand & 1)) {
                nDeltaRotation = g_FieldScriptVMCurActor->rotation.vy + delta;
            } else {
                nDeltaRotation = g_FieldScriptVMCurActor->rotation.vy - delta;
            }
            nRotation = PSX_ANGLE(nDeltaRotation);
        }
    }
    D_800B00C0 = 1;
    g_FieldScriptVMCurActor->rotation.vx = nRotation;
    g_FieldScriptVMCurActor->scriptInstructionPointer++;
}

void func_8009F5A8(void) {
    unsigned short nStoredIP = g_FieldScriptVMCurActor->scriptInstructionPointer;
    func_8009F5F4(); // Do Encounter
    D_800B00C0 = 1;
    g_FieldScriptVMCurActor->scriptInstructionPointer = nStoredIP;
}


INCLUDE_ASM("asm/field/nonmatchings/main/misc6", func_8009F5F4);

int FieldCharacterIdToPartyId(int characterId) {
    int i;

    if (characterId == CHARACTER_ID_NONE) {
        return -1;
    }
    
    for (i = 0; i < 3; i++) {
        if (g_GamePartyMembers[i] == CHARACTER_ID_NONE)
            return -1;
        
        if (g_GamePartyMembers[i] == characterId) {
            return i;
        }
    }

    return -1;
}

// Screened 36 variants (2.7.2 / cdk / 2.6.0 x -O1/-O2/-O3 x -G4/-G8): best is
// 90.86% here, cdk 87.35%, 2.6.0 90.86% - the 0x7C store's sll/sra pair is left.
// Also tried: the two-return shape `if (SCRIPT_READ_U8(0) == 0xFF) { ...body...;
// return 0; } return 0;` reproduces retail's `bne`-to-shared-epilogue and the exact
// 0x20 frame (90.25%, 320/324 bytes). Only the 0x7C store's sll/sra pair is left;
// gcc folds it because it can prove `((x + 4) & 7) << 9` fits in 13 bits (tried
// s32 nAngle, an `nAngle << 16 >> 16` temp, and reading 0x56 back).
// 90.86% (316 of 324 bytes): frame (0x20 with ra/s2/s1/s0), the `sll s0; sra s0`
// of nX in the 2nd call's delay slot, both call orders and the rotation value are
// right. `s16 nAngle` and `s32 nX` with an explicit (s16) cast on the call are
// what fix those; `s16 nRotation` instead CSEs the 0x8000 constant into $s0, so
// nRotation stays s32. Remaining 8 bytes: (a) the original emits
// `sll a0,a1,16; sra v0,a0,16` for the 0x7C store and shares the sll with the
// 0x60 store, i.e. nAngle is sign-extended because it is a narrow variable;
// gcc folds the conversion away here (and then re-emits a lone `sll` for 0x60),
// it only keeps the pair when the 0x60 store comes first, which then puts the
// three stores in the wrong order (88.83%, 320 bytes); (b) the original keeps
// `addu v0,zero,zero` in the bne delay slot, i.e. it has two `return 0` sites
// that gcc merged into a shared epilogue; a single trailing return puts it after
// the rotation stores and an if/else doubles it into a `j` (90.25%, 320 bytes).
// Also tried: reading 0x7C back through g_Scene.sceneAngle.vy (folded), a named
// s32 fixed-point temp (folded), `unk7C = nAngle` with s16 nAngle (folded).
/*
s32 func_8009FA54(s32 nInstructionOffset) {
    s32 nOffset;
    s32 nX;
    s32 nY;
    u8 nAngleByte;
    s16 nAngle;
    u8 nRotationByte;
    s32 nRotation;
    ActorData* pActor;

    if (SCRIPT_READ_U8(0) == 0xFF) {
        nOffset = nInstructionOffset * 7;
        g_FieldScriptVMCurActor->walkmeshId = SCRIPT_READ_U8(nOffset + 5);
        nX = (s16)func_8009E330(nOffset + 1);
        nY = (s16)func_8009E330(nOffset + 3);
        func_8009E574(nX, nY);

        nAngleByte = SCRIPT_READ_U8(nOffset + 6);
        nAngle = ((nAngleByte + 4) & 7) << 9;
        if (nAngleByte == 0xFF) {
            nAngle = ((FieldScriptVMGetVariableValue(8) + 4) & 7) << 9;
        }
        g_Scene.sceneAngle.vy = nAngle;
        g_Scene.unk7C = nAngle;
        g_Scene.unk60 = nAngle << 16;

        nRotationByte = SCRIPT_READ_U8(nOffset + 7);
        nRotation = ((nRotationByte - 2) & 7) << 9 | 0x8000;
        if (nRotationByte == 0xFF) {
            nRotation = ((FieldScriptVMGetVariableValue(6) - 2) & 7) << 9 | 0x8000;
        }
        pActor = g_FieldScriptVMCurActor;
        pActor->rotation.vx = nRotation;
        pActor->rotation.vy = nRotation;
        pActor->rotation.vz = nRotation;
    }
    return 0;
}
*/
INCLUDE_ASM("asm/field/nonmatchings/main/misc6", func_8009FA54);
// Screened 36 variants: flat 63.17% under all three gccs - only the epilogue store
// order differs, no compiler/flag combination reaches 100.
// 63.17% (120 vs 120 bytes): insn multiset is identical, only the order of the
// two epilogue stores differs. cc1 puts `sh ip+2,0xCC(a0)` before
// `sw D_800B2268` and hoists the second `lhu 0xCC(a0)` above the `lbu`, i.e. it
// schedules the store past the independent one (the unit's scheduler is on at
// -O2) and retail's `sh` ends up in the `jr ra` delay slot while ours does not.
// Tried: a local `pActor` for the two 0xCC accesses (same 63.17%), the macro form
// SCRIPT_READ_U8_REL (same, so the previous 66.50% note was an objdiff-version
// difference), and a `*(volatile s32*)&D_800B2268` store (no change - so cc1 is
// not sinking the store, the scheduling happens before it). Re-tried a `s16 ip`
// temp for the 0xCC read-modify-write: identical 63.17%.
/*
void func_8009FB98(void) {
    ActorData* pActor;

    g_GameSceneMapNum |= 0xC000;
    GameWaitForCdData();
    GamePartySyncSkinData();
    GamePartySyncStreamedData();
    pActor = g_FieldScriptVMCurActor;
    D_800B2268 = *(u8*)&g_FieldScriptVMCurScriptData[pActor->scriptInstructionPointer + 1];
    pActor->scriptInstructionPointer += 2;
}
*/
INCLUDE_ASM("asm/field/nonmatchings/main/misc6", func_8009FB98);

extern s32 D_8006F990[];
int func_8009FC10(int actorIndex) {
    int i;

    for (i = 0; i < 3; i++) {
        if (D_8006F990[i] == actorIndex) {
            return i;
        }
    }
    
    return 0xFF;
}

void FieldScriptPartyMemberRideGear(void) {
    int index = FieldScriptVMGetArgument(1);
    if (index >= 3) {
        index = 2;
    }
    g_pGameState->gearRide[index] = 1;
    func_8009FD10(index);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void FieldScriptPartyMemberDisembarkGear(void) {
    int index = FieldScriptVMGetArgument(1);
    if (index >= 3) {
        index = 2;
    }
    g_pGameState->gearRide[index] = 0;
    func_8009FD10(index);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void func_8009FD10(int partyMemberIndex) {
    switch (partyMemberIndex) { 
        case 0:
            FieldScriptMemoryWriteU16(0x2A, g_GameSceneMapNum & 0xFFF);
            FieldScriptMemoryWriteU16(0x2C, 0);
            FieldScriptMemoryWriteU16(0x2E, 0);
            break;
        case 1:
            FieldScriptMemoryWriteU16(0x30, g_GameSceneMapNum & 0xFFF);
            FieldScriptMemoryWriteU16(0x32, 0);
            FieldScriptMemoryWriteU16(0x34, 0);
            break;
        case 2:
            FieldScriptMemoryWriteU16(0x36, g_GameSceneMapNum & 0xFFF);
            FieldScriptMemoryWriteU16(0x38, 0);
            FieldScriptMemoryWriteU16(0x3A, 0);
            break;
    }
}

extern void func_800AD4D4(int partyMemberIndex);

// 99.50%: gcc puts the index in $v1, the original uses $a0 (`move a0,v0`).
void func_8009FDD4(void) {
    int partyMemberIndex;

    partyMemberIndex = func_8009FC10(D_800AFD1C);
    if (partyMemberIndex != 0xFF && g_pGameState->gearRide[partyMemberIndex] == 0) {
        func_800AD4D4(partyMemberIndex);
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer++;
}

extern void func_800ACFD0(void);

void func_8009FE4C(void) {
    u8 index;

    index = SCRIPT_READ_U8_REL(1);
    if (g_GamePartyMembers[index] != CHARACTER_ID_NONE && g_pGameState->gearRide[index]) {
        func_800ACFD0();
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}

extern s32 D_8005A444[];
extern s32 D_8005A448;
extern s32 D_8005A44C;

// Screened 36 variants: best 99.36% here, cdk 96.03%, 2.6.0 99.36% - the reorg
// delay-slot fill (`sll` into the beq slot) is the whole gap.
// 99.36% (624 bytes; retail is 628). Everything matches except one delay slot: cc1
// (reorg) puts the first insn of the fall-through block, `sll v0,v1,1` (part of
// nActorIndex * sizeof(FieldActor)), into the `beq nActorIndex,0xFF` delay slot, and
// retail leaves a `nop` there (retail's `sll` writes $v0, which the branch reads).
// The `pActorSlots` local is what reproduces retail's `la $s0,D_8005A444` in the
// entry block (without it gcc addresses D_8005A444 with %hi/%lo per use and the
// frame loses $s0: 95.25%); case 1/2 must use the absolute symbols D_8005A448 /
// D_8005A44C (both already in linker/undefined_syms_auto.field.txt) because retail
// does, and case 0 goes through the pointer. Tried: if-wrap instead of the early
// return, and three declaration orders - all identical. Parking for the reorg
// delay-slot fill, which no source shape reached.
/*
void func_8009FEE4(int partyMemberIndex) {
    s32* pActorSlots;
    s32 nActorIndex;
    s32 nWalkmeshFlags;
    ActorData* pActorData;

    pActorSlots = D_8005A444;
    nActorIndex = pActorSlots[partyMemberIndex];
    if (nActorIndex == ACTOR_ID_INVALID) {
        return;
    }
    pActorData = g_FieldActors[nActorIndex].pActorData;
    nWalkmeshFlags = pActorData->walkmeshId << 14;
    switch (partyMemberIndex) {
        case 0:
            FieldScriptMemoryWriteU16(0x2A, (g_GameSceneMapNum & 0xFFF) | nWalkmeshFlags);
            FieldScriptMemoryWriteU16(0x2C, ((s16*)&g_FieldActors[pActorSlots[0]].pActorData->position)[1]);
            FieldScriptMemoryWriteU16(0x2E, ((s16*)&g_FieldActors[pActorSlots[0]].pActorData->position)[5]);
            break;
        case 1:
            FieldScriptMemoryWriteU16(0x30, (g_GameSceneMapNum & 0xFFF) | nWalkmeshFlags);
            FieldScriptMemoryWriteU16(0x32, ((s16*)&g_FieldActors[D_8005A448].pActorData->position)[1]);
            FieldScriptMemoryWriteU16(0x34, ((s16*)&g_FieldActors[D_8005A448].pActorData->position)[5]);
            break;
        case 2:
            FieldScriptMemoryWriteU16(0x36, (g_GameSceneMapNum & 0xFFF) | nWalkmeshFlags);
            FieldScriptMemoryWriteU16(0x38, ((s16*)&g_FieldActors[D_8005A44C].pActorData->position)[1]);
            FieldScriptMemoryWriteU16(0x3A, ((s16*)&g_FieldActors[D_8005A44C].pActorData->position)[5]);
            break;
    }
}
*/
INCLUDE_ASM("asm/field/nonmatchings/main/misc6", func_8009FEE4);

void func_800A0158(s32 arg0, s32* arg1, s32* arg2, s32* arg3) {
    switch (arg0) {
        case 0:
            *arg1 = FieldScriptVMGetVariableValue(0x2A);
            *arg2 = FieldScriptVMGetVariableValue(0x2C);
            *arg3 = FieldScriptVMGetVariableValue(0x2E);
            break;
        case 1:
            *arg1 = FieldScriptVMGetVariableValue(0x30);
            *arg2 = FieldScriptVMGetVariableValue(0x32);
            *arg3 = FieldScriptVMGetVariableValue(0x34);
            break;
        case 2:
            *arg1 = FieldScriptVMGetVariableValue(0x36);
            *arg2 = FieldScriptVMGetVariableValue(0x38);
            *arg3 = FieldScriptVMGetVariableValue(0x3A);
            break;
    }
}

INCLUDE_ASM("asm/field/nonmatchings/main/misc6", func_800A0228);

extern void FieldMatrixCopyTransform(MATRIX* pDest, MATRIX* pSrc);
extern void FieldMatrixCopyTranslation(MATRIX* pDest, MATRIX* pSrc);

// Screened 36 variants: best 91.72% here, cdk 73.20%, 2.6.0 91.36% - 
// the &g_FieldActors materialisation ($s2 hoist) is the gap.
// 91.72% (440 of 452 bytes): every load, store, call and the copy loop match.
// g_FieldActors is declared volatile in include/field/actor.h, but this unit's
// original object reads it through a plain pointer, so the pointer load is
// shared between uses. Reading it through a local `FieldActor**` (below)
// gets that: `(*(FieldActor**)&g_FieldActors)` does NOT, gcc keeps MEM_VOLATILE
// (74.12%). Remaining 12 bytes: the original materialises `&g_FieldActors` into
// $s2 at the first matrix call (after the copy loop, `lui s2; addiu s2; lw
// a1,0(s2)`), gcc hoists the two address insns to the top of the function where
// the local pointer is assigned and leaves `lw a1,0(s2)` where the original has
// it; the saved-$s2 slot then also moves. Tried: macro/inline cast per use
// (volatile, 74.12%), plain `g_FieldActors[...]` (74.12%).
/*
void func_800A0524(int dstActorIndex, int srcActorIndex) {
    FieldActor** pFieldActors;
    ActorData* pDst;
    ActorData* pSrc;
    int i;

    pFieldActors = (FieldActor**)&g_FieldActors;
    pDst = (*pFieldActors)[dstActorIndex].pActorData;
    pSrc = (*pFieldActors)[srcActorIndex].pActorData;
    for (i = 0; i < 4; i++) {
        pDst->walkmeshTriIds[i] = pSrc->walkmeshTriIds[i];
    }
    pDst->walkmeshId = pSrc->walkmeshId;
    pDst->unkEC = pSrc->unkEC;
    pDst->curYPos = pSrc->curYPos;
    pDst->curTriNormal.vx = pSrc->curTriNormal.vx;
    pDst->curTriNormal.vy = pSrc->curTriNormal.vy;
    pDst->curTriNormal.vz = pSrc->curTriNormal.vz;
    pDst->position.vx = pSrc->position.vx;
    pDst->position.vy = pSrc->position.vy;
    pDst->position.vz = pSrc->position.vz;
    FieldMatrixCopyTransform(&(*pFieldActors)[dstActorIndex].transformMatrix, &(*pFieldActors)[srcActorIndex].transformMatrix);
    FieldMatrixCopyTranslation(&(*pFieldActors)[dstActorIndex].transformMatrix, &(*pFieldActors)[srcActorIndex].transformMatrix);
    ((SpriteData*)(*pFieldActors)[dstActorIndex].pSpriteData)->position.x = (*pFieldActors)[srcActorIndex].pActorData->position.vx;
    ((SpriteData*)(*pFieldActors)[dstActorIndex].pSpriteData)->position.y = (*pFieldActors)[srcActorIndex].pActorData->position.vy;
    ((SpriteData*)(*pFieldActors)[dstActorIndex].pSpriteData)->position.z = (*pFieldActors)[srcActorIndex].pActorData->position.vz;
}
*/
INCLUDE_ASM("asm/field/nonmatchings/main/misc6", func_800A0524);

void func_800A06E8(void) {
    FieldActor* pFieldActor;
    u_short nStatus;
    FieldActor* pStatusActor;
    s32 nOffset;
    int partyId;

    pFieldActor = &g_FieldActors[D_800AFD1C];
    partyId = FieldCharacterIdToPartyId(func_8008CF3C(FieldScriptVMGetArgument(1)));
    pFieldActor->status = (pFieldActor->status & 0xF07F) | 0x200;
    if (partyId != -1) {
        func_80076AC0(D_800AFD1C, partyId, g_PartyDataBuffers[partyId], 2, 0, partyId, 1);
        nOffset = D_800AFD1C * sizeof(FieldActor);
        pStatusActor = (FieldActor*)(nOffset + (s32)g_FieldActors);
        nStatus = pStatusActor->status;
        D_800AFD20 = -0xC0;
        pStatusActor->status = nStatus & 0xFFDF;
        func_800A0C94();
        g_FieldScriptVMCurActor->scriptFlags.flags |= 0x100;
        g_FieldScriptVMCurActor->scriptFlags.flags &= ~0x80;
        ((u_short*)&g_FieldActors[D_800AFD1C].status)[0] &= 0xFFDF;
    } else {
        func_80076AC0(D_800AFD1C, 0, g_PartyDataBuffers[0], 1, 0, 0, 1);
        g_FieldScriptVMCurActor->scriptFlags.fields.scriptFlags_0x0 = 1;
        D_800AFFEC = 1;
        D_800B00C0 = 1;
        g_FieldScriptVMCurActor->flags |= 0x100000;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

INCLUDE_ASM("asm/field/nonmatchings/main/misc6", func_800A08B8);

extern s32 g_PlayerActorIndex;

void func_800A0C4C(void) {
    g_FieldActors[g_PlayerActorIndex].pActorData->scriptFlags.flags |= 0x80;
}

// Screened 36 variants: best 99.64% here, cdk 94.07%, 2.6.0 99.64% - the third
// block's `addu` destination register is the whole gap.
// FieldResetActorPosition: copies the current actor's scripted position into the
// field actor's transform/child matrices and into its sprite.
// 99.64%: every instruction matches except the destination register of the third
// block's `addu`: the original coalesces base+offset into the index register
// (`addu v1,v1,v0`, then the two stores off $v1); gcc 2.7.2 always coalesces it
// into the freshly loaded base register (`addu v0,v1,v0`). Same size and operand
// order; -O2/-O3 both do this. index+base operand order is required (base-first
// gives `addu v0,v0,v1`).
// Also tried: `offset += (s32)g_FieldActors` in the third block (mutating the index
// variable moves the whole allocation, 82.38%), `u_int offset` (same 99.64%) and
// computing the pointer after `z = pPosition[5]` (same 99.64%).
// Re-tried with a variant harness (each of these confirmed against the object):
// all 24 declaration orders of x/y/z/offset (identical code, so the choice is not
// pseudo numbering); a second pointer local `pFieldActor2` for the third block and
// an `s32 addr = offset; addr += (s32)g_FieldActors;` copy (both stop gcc from
// CSE-ing the x0x5C, which is then rematerialised per block, 82.38%); storing
// through `((s32*)pFieldActor)[10]/[18]` (same 99.64%); and reading `pPosition[5]`
// twice instead of a `z` temp (the sum then goes to a fresh $a3 in all three
// blocks, 75.00%). The whole function is a single basic block, so the sum, the
// reloaded base and the offset are all local-alloc quantities and gcc 2.7.2 always
// takes the first free $v0; retail's $v1 dest needs the offset quantity to be
// preferred, which no source shape reached.
/*
void func_800A0C94(void) {
    FieldActor* pFieldActor;
    SpriteData* pSpriteData;
    ActorData* pActor;
    s16* pPosition;
    s32 x;
    s32 y;
    s32 z;
    s32 offset;
    u16 curY;

    pActor = g_FieldScriptVMCurActor;
    offset = D_800AFD1C * sizeof(FieldActor);
    pFieldActor = (FieldActor*)(offset + (s32)g_FieldActors);
    pPosition = (s16*)&pActor->position;

    x = pPosition[1];
    pSpriteData = pFieldActor->pSpriteData;
    pFieldActor->transformMatrix.t[0] = x;
    pFieldActor->childMatrix.t[0] = x;

    y = pPosition[3];
    pFieldActor = (FieldActor*)(offset + (s32)g_FieldActors);
    pFieldActor->transformMatrix.t[1] = y;
    pFieldActor->childMatrix.t[1] = y;

    pFieldActor = (FieldActor*)(offset + (s32)g_FieldActors);
    z = pPosition[5];
    pFieldActor->transformMatrix.t[2] = z;
    pFieldActor->childMatrix.t[2] = z;

    pSpriteData->position.x = pActor->position.vx;
    pSpriteData->position.y = pActor->position.vy;
    pSpriteData->position.z = pActor->position.vz;
    pSpriteData->step.y = 0;

    curY = ((u16*)pPosition)[3];
    pSpriteData->field_0x84 = curY;
    pActor->curYPos = curY;
}
*/
INCLUDE_ASM("asm/field/nonmatchings/main/misc6", func_800A0C94);

void func_800A0D3C(void) {
    func_80076AC0(D_800AFD1C, 0, (void*)((*(s32*)(g_FieldSpriteData + 4)) + (s32)g_FieldSpriteData), 0, 0, 0x80, 1);
    func_800A0C94();
    g_FieldScriptVMCurActor->scriptFlags.fields.scriptFlags_0x8 = 0x1;
    g_FieldScriptVMCurActor->flags |= 0x800;
    g_FieldScriptVMCurActor->scriptInstructionPointer++;
}

typedef struct { s16 value; } FieldScriptGlobalS16;
extern FieldScriptGlobalS16 D_800B234A;

void func_800A0DC0(void) {
    D_800B234A.value = FieldScriptVMGetArgument(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void func_800A0DFC(void) {
    int nAddress;

    nAddress = FieldScriptVMGetInstructionArgument(1) & 0xFFFF;
    FieldScriptMemoryWriteU16(nAddress, ArchiveGetDiscNumber());
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

extern int D_800ADB74;

void func_800A0E54(void) {
    if (D_800ADB74 == 0) {
        g_FieldScriptVMCurActor->scriptInstructionPointer++;
    } else {
        g_FieldScriptVMCurActor->scriptInstructionPointer--;
    }
    D_800B00C0 = 1;
}

extern int D_800ADB84;

void func_800A0EB0(void) {
    D_800B00C0 = 1;
    D_800ADB84++;
    g_FieldScriptVMCurActor->scriptInstructionPointer++;
}

extern void* D_801E8670[];
extern void func_801E8030(int modelIndex);

// Screened 36 variants: best 90.55% here, cdk 85.13%, 2.6.0 90.55% - the
// &D_800B2264 lui/addiu vs %hi/%lo shape is the gap.
// 90.55%: dispatch and case 0 match exactly. Only case 1 differs - the original
// materialises &D_800B2264 into a register (lui+addiu, then lw/sw off it), gcc
// emits %hi/%lo per access and reloads the actor first. Pointer (`s32* p =
// &D_800B2264; (*p)--;`) and array (`D_800B2264[0]--`) forms both produce the same
// %hi/%lo code. Result is one insn short (236 vs 240 bytes).
// Re-tried `(*(volatile s32*)&D_800B2264)--` (88.97%: the volatile form adds a
// second address use and hoists the 0xCC load further) and -G0 (90.55%).
/*
void func_800A0EE8(void) {
    int nModelIndex;

    nModelIndex = g_FieldScriptVMCurActor->flags12C_0xE;
    g_FieldScriptVMCurActor->flags &= ~0x2000;
    switch (SCRIPT_READ_U8_REL(1)) {
        case 0:
            ((u8*)D_801E8670[nModelIndex])[0x34] = 0;
            g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
            break;
        case 1:
            func_801E8030(g_FieldScriptVMCurActor->flags12C_0xE);
            D_800B2264--;
            g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
            break;
    }
    D_800B00C0 = 1;
}
*/
INCLUDE_ASM("asm/field/nonmatchings/main/misc6", func_800A0EE8);

INCLUDE_ASM("asm/field/nonmatchings/main/misc6", func_800A0FD8);

// Screened 36 variants: best 84.19% here, cdk 81.67%, 2.6.0 83.64% - the RMW vs
// `++` store order is the gap (already reordered in cc1's .s).
// 84.19% (368 vs 368 bytes): the frame, all four calls and every load/store match.
// The only gap is ORDER: retail emits the flags12C bitfield RMW and only then
// reloads D_800B2264 for the `++`, with `sw D_800B2264` last; gcc shares one load
// between the `& 7` and the `++` and hoists the `++` store above the RMW store.
// Cause is the -O2 scheduler hoisting an independent store, not the allocator: a
// volatile store (`(*(volatile s32*)&D_800B2264)++`) is hoisted just the same (and
// adds a redundant reload, 72.02%), and the reorder is already present in cc1's
// .s. Parking: making the `++` store depend on the RMW store is not expressible in
// source. Everything else in the function is exact.
/*
void func_800A1364(void) {
    int arg;
    FieldActor* pFieldActor;
    u8 unused[8]; // original has an unused stack slot; reproduces the frame

    pFieldActor = &g_FieldActors[D_800AFD1C];
    pFieldActor->status = (pFieldActor->status & 0xF07F) | 0x200;
    arg = FieldScriptVMGetArgument(1);
    func_80076AC0(D_800AFD1C, 0, (void*)((*(s32*)(g_FieldSpriteData + 4)) + (s32)g_FieldSpriteData), 0, 0, 0x80, 1);
    arg *= 2;
    func_800A0C94();
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
    g_FieldScriptVMCurActor->scriptFlags.flags |= 0x100;
    g_FieldActors[D_800AFD1C].status &= ~ACTOR_STATUS_INVISIBLE;
    g_FieldScriptVMCurActor->flags |= 0x2000;
    g_FieldScriptVMCurActor->flags &= ~0x800;
    D_800B21DC[D_800B2264] = arg;
    D_800B225F[D_800B2264] = 0;
    g_FieldScriptVMCurActor->flags12C_0xE = D_800B2264 & 7;
    D_800B2264++;
}
*/
INCLUDE_ASM("asm/field/nonmatchings/main/misc6", func_800A1364);

void func_800A14F0(void) {
    int arg;
    int arg3;
    FieldActor* pFieldActor;
    void* pSprite;

    pFieldActor = &g_FieldActors[D_800AFD1C];
    pFieldActor->status = (pFieldActor->status & 0xF07F) | 0x200;
    arg = FieldScriptVMGetArgument(1);
    pSprite = (void*)((*(s32*)(g_FieldSpriteData + arg * 4 + 4)) + (s32)g_FieldSpriteData);
    arg3 = FieldScriptVMGetArgument(3);
    func_80076AC0(D_800AFD1C, arg, pSprite, 0, arg3, arg | 0x80, 1);
    func_800A0C94();
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
    g_FieldScriptVMCurActor->scriptFlags.flags |= 0x100;
    g_FieldScriptVMCurActor->scriptFlags.flags &= ~0x80;
    g_FieldScriptVMCurActor->flags &= ~0x800;
    g_FieldActors[D_800AFD1C].status &= ~ACTOR_STATUS_INVISIBLE;
}

void func_800A1624(void) {
    int arg;
    FieldActor* pFieldActor;

    pFieldActor = &g_FieldActors[D_800AFD1C];
    pFieldActor->status = (pFieldActor->status & 0xF07F) | 0x200;
    arg = FieldScriptVMGetArgument(1);
    func_80076AC0(D_800AFD1C, arg, (void*)((*(s32*)(g_FieldSpriteData + arg * 4 + 4)) + (s32)g_FieldSpriteData), 0, 0, arg | 0x80, 0);
    func_800A0C94();
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
    g_FieldScriptVMCurActor->scriptFlags.flags |= 0x100;
    g_FieldScriptVMCurActor->scriptFlags.flags &= ~0x80;
    g_FieldScriptVMCurActor->flags &= ~0x800;
    g_FieldActors[D_800AFD1C].status &= ~ACTOR_STATUS_INVISIBLE;
}
