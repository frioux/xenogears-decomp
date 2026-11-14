#include "common.h"
#include "main/game.h"
#include "field/actor.h"
#include "field/main.h"
#include "field/text_box.h"
#include "field/effects.h"
#include "system/memory.h"
#include "system/archive.h"
#include "system/sound.h"
#include "psyq/libetc.h"
#include "psyq/libcd.h"
#include "psyq/inline_c.h"

extern int func_80085C90(s32 arg0);
extern s32 D_8004F308;
extern s32 D_8004F324;
extern s32 D_800ADC18;

// Ticks down the countdown timer, returning the value it held after the tick
int func_80078B5C(void) {
    int nCount;

    rand();
    if (D_8004F308 == -1) {
        D_8004F308 = func_80085C90(D_8004F324);
    }
    nCount = D_800ADC18;
    if (nCount != 0) {
        nCount--;
        D_800ADC18 = nCount;
    }
    return nCount;
}

INCLUDE_ASM("asm/field/nonmatchings/main/misc4", func_80078BC8);
/* Best attempt: 90.68%. The original keeps -1 in v0 across the whole guard chain
   (loads go to v1) and copies ArchiveDataSync's result to v1 before testing it.
   Five separate early `return -1`s, one `||` chain, and the nResult form all
   compile to 144 bytes with `li v0,-1` re-materialised in every branch delay slot
   (78.38% for the first two), so the shape below is the closest; also 90.68%
   under every gcc-2.6.0/2.7.2/2.7.2-cdk -O1..-O3 -G4/-G8 combination.
   The lone addiu v0,-1 sits in the ArchiveDataSync branch's delay slot, so it
   also runs on the fall-through path; the three nops are that value simply not
   being clobbered again. Every guard load lands in v0 in our compile and in v1
   in the original, i.e. the -1 has to be live in v0 (a call-crossing constant in
   a call-clobbered register); goto/label, a hoisted nResult and per-block -1 all
   still rematerialise it per block (77-80%).
extern int ArchiveDataSync(void);
extern s32 D_800ADB2C;
extern s32 D_800ADB34;
extern s32 D_800ADB90;
extern s32 D_800ADBC4;

int func_80078BC8(void) {
    int nSyncStatus;
    int nResult;

    if (D_800ADB2C != 0) {
        return -1;
    }
    nSyncStatus = ArchiveDataSync();
    if (nSyncStatus != 0) {
        nResult = -1;
    } else if (D_8004F308 != 0) {
        nResult = -1;
    } else if (D_800ADB90 != 0) {
        nResult = -1;
    } else if (D_800ADB34 != 0) {
        nResult = -1;
    } else {
        nResult = -(D_800ADBC4 != 0xFF);
    }
    return nResult;
}
*/

extern s16 D_800B2344;
void func_80078C5C(void) {
    RECT rect;
    int i;
    u_int* pImage;

    if (D_800B2344 != 0) {
        g_FieldRenderContexts[0].drawEnvs[0].dtd = 0;
        g_FieldRenderContexts[1].drawEnvs[0].dtd = 0;
        pImage = HeapAlloc(0x4000, 0x0);
        rect.y = 0x1E0;
        rect.w = 0x100;
        rect.x = 0;
        rect.h = 0x20;
        StoreImage(&rect, pImage);
        DrawSync(0);

        // Process image before storing it back in VRAM
        for (i = 0; i < 0x1000; i++) {
            if (pImage[i] & 0xFFFF) {
                pImage[i] |= 0xC63;
            }
            if (pImage[i] & 0xFFFF0000) {
                pImage[i] |= 0x0C630000;
            }
        }
        
        LoadImage(&rect, pImage);
        DrawSync(0);
        HeapFree(pImage);
    }
}

INCLUDE_ASM("asm/field/nonmatchings/main/misc4", func_80078D44);

INCLUDE_ASM("asm/field/nonmatchings/main/misc4", func_80079288);
/* Best attempt: 79.46% (the form below scores 78.37%, 728 bytes against the
   original's 684). splat splits this block per word, so the original's
   relocations name D_800B2298 (period), D_800B229C (count) and D_800B22A0
   (values) separately while reading all three through one base register
   (s1 = &D_800B2298; count at -4(a0), values at 0(a0) for a0 = s1 + 8, i.e.
   combine_givs re-basing three symbols onto one pointer). Remaining deltas:
   that rebase, and rand()'s product landing in a2/a3 instead of s1/a2.
extern s32 D_800ADBDC;
extern s32 D_800ADBE4;
extern s32 D_800ADBEC;
extern s32 D_800ADB2C;
extern u_char D_800ADB04;
extern s32 D_800ADBD0;
extern s32 D_8004F370;
extern s32 D_800B2294;   // countdown to the next regeneration of the slot table
extern s32 D_800B2298;   // regeneration period, 0 disables the random skin
extern s32 D_800B229C;   // number of slot table entries in use
extern u16 D_800B22A0[]; // slot table; 0xFFFF marks a free slot
extern u16 D_800B2270[]; // per-skin value handed to the field system
extern s16 D_800B2290;
extern u_char D_80065ADC[]; // per-skin selection weight
extern u_char D_80059508;
extern u_char D_800594F8;
extern void FieldRandomizeSkinSlots(void);
extern void LoadGameStateOverlay(int);
extern void func_80281204(int);
extern int rand(void);

// Ages the slot table and, once a slot frees up, rolls a skin against
// D_80065ADC's weights and switches the party to it.
void func_80079288(void) {
    int aWeightSums[16];
    int nTotal;
    int nTarget;
    int i;
    int nSum;
    s32* pPeriod;
    s32* pCount;
    u16* pSlots;
    u_char* pWeight;

    if (D_800ADBDC == 0) {
        return;
    }
    if (D_800ADBE4 == 0) {
        return;
    }
    if (D_800ADBEC == 0) {
        return;
    }
    if (D_8004F308 == -1) {
        return;
    }
    pPeriod = &D_800B2298;
    if (*pPeriod == 0) {
        return;
    }
    if (g_FieldControl.isRandomEncountersEnabled == -1) {
        return;
    }
    if (D_800ADB2C == 1) {
        return;
    }
    if (D_800ADB04 == 0) {
        return;
    }
    if (--D_800B2294 == 0) {
        FieldRandomizeSkinSlots();
    }
    pSlots = (u16*)&pPeriod[2];
    for (i = 0; i < pPeriod[1]; i++) {
        if (pSlots[i] != 0xFFFF) {
            pSlots[i] = pSlots[i] - 1;
        }
    }
    pCount = &D_800B229C;
    pSlots = (u16*)&pCount[1];
    for (i = 0; i < *pCount; i++) {
        if (pSlots[i] != 0) {
            continue;
        }
        D_800B22A0[i] = 0xFFFF;
        pWeight = D_80065ADC;
        nTotal = 0;
        for (i = 0; i < 16; i++) {
            nTotal += pWeight[i];
        }
        nSum = 0;
        for (i = 0; i < 16; i++) {
            aWeightSums[i] = nSum;
            nSum += pWeight[i];
        }
        nTarget = (rand() * (nTotal + 1)) >> 15;
        nSum = 0;
        for (i = 15; i >= 0; i--) {
            if (pWeight[i] != 0 && aWeightSums[i] < nTarget) {
                nSum++;
                break;
            }
        }
        if (nSum == 0) {
            return;
        }
        D_80059508 = i;
        D_800594F8 = 0;
        D_800B2290 = D_800B2270[i];
        if (D_8004F370 == 0) {
            LoadGameStateOverlay(2);
        }
        D_800ADBDC = 0;
        D_800ADBD0 = 1;
        if (g_FieldSystemMode == 0) {
            func_80281204(i);
        }
        break;
    }
}
*/

extern u_char D_8005942C;
extern s32 D_800AFC78;
extern s32 D_8004F324;
extern s32 D_8004F370;
extern s16 D_8004F384;
extern s32 D_8004F310;
extern s32 g_GamePartySkinsInitialized;
extern s32 D_800B0064;
extern void func_800A30FC(void);
extern void func_8001B66C(void);
extern void func_80085FB8(void);
extern void func_80085F30(void);
extern void func_8001BB50(void);
extern void ArchiveCdDataSync(int);
extern void ChangeGameState(int);

// Field game state machine: one case per transition
void func_8007954C(int state) {
    D_8005942C = 0;
    switch (state) {
    case 0: {
        u16 nSaved;

        func_800A30FC();
        nSaved = g_pGameState->unk1930[1];
        D_8004F324 = D_800AFC78;
        g_pGameState->unk2322 = D_800AFC78;
        g_pGameState->unk2320 = nSaved;
        if (D_8004F370 != 0) {
            return;
        }
        ChangeGameState(2);
        break;
    }
    case 1:
        if (D_8004F384 == state) {
            func_8001B66C();
            func_80085FB8();
            ArchiveCdDataSync(0);
            func_80085F30();
            func_8001B66C();
        }
        if (D_8004F370 != 0) {
            return;
        }
        ChangeGameState(3);
        break;
    case 2: {
        u16 nSaved;

        nSaved = g_pGameState->unk1930[1];
        g_pGameState->unk2322 = D_8004F324;
        g_pGameState->unk2320 = nSaved;
        if (D_8004F370 != 0) {
            return;
        }
        ChangeGameState(4);
        g_GamePartySkinsInitialized++;
        break;
    }
    case 3:
        D_8004F310 = 0;
        g_GamePartySkinsInitialized = 0;
        if (D_8004F370 != 0) {
            return;
        }
        if (D_800B0064 & 0x80) {
            func_8001BB50();
        }
        ChangeGameState(D_800B0064 & 0x7F);
        break;
    }

    MainLoop(0);
}

void func_800796F4(void) {}

void FieldSwapRenderContext(void) {
    g_FieldCurRenderContextIndex = (g_FieldCurRenderContextIndex + 1) % 2;
    g_FieldCurRenderContext = &g_FieldRenderContexts[g_FieldCurRenderContextIndex];
    PutDispEnv(&g_FieldCurRenderContext->dispEnv);
    PutDrawEnv(&g_FieldCurRenderContext->drawEnvs[0]);
}

extern RECT D_800AFE4C;
extern TILE D_800AFE54[];
extern DR_MODE D_800AFE24[];
	
void func_80079784(int color) {
    FieldClearAndSwapOTag();
    setRGB(D_800AFE54[g_FieldCurRenderContextIndex], color * 4);
    addPrim(g_FieldCurRenderContext->ot1, &D_800AFE54[g_FieldCurRenderContextIndex]);
    addPrim(g_FieldCurRenderContext->ot1, &D_800AFE24[g_FieldCurRenderContextIndex]);
    FieldRenderSync();
    MoveImage(&D_800AFE4C, 0, g_FieldCurRenderContextIndex * 0x100);
    PutDispEnv(&g_FieldCurRenderContext->dispEnv);
    PutDrawEnv(&g_FieldCurRenderContext->drawEnvs[0]);
    DrawOTag(g_FieldCurRenderContext->ot1 + 1);
}


extern s32 D_800B2268;
extern u_char D_80059179;
extern s16 D_800B234C;
extern s32 g_PlayerActorIndex;

void func_800798BC(void) {
    if (D_800B2268 != 0 && (g_FieldActors[g_PlayerActorIndex].pActorData->curWalkmeshTriMaterial & 0xC0) == 0) {
        D_80059179 = 0;
    } else {
        D_80059179 = 1;
    }
    if (D_800B234C != 0xFF) {
        D_80059179 = D_800B234C;
    }
}

void func_8007995C(short w, short h, short x, short y, int destX, int destY) {
    RECT rect;

    rect.w = w;
    rect.h = h;
    rect.x = x;
    rect.y = y;
    MoveImage(&rect, destX, destY);
    DrawSync(0);
}

void FieldRenderSyncAndFlush(void) {
    FieldRenderSync();
    EnterCriticalSection();
    FlushCache();
    ExitCriticalSection();
}

INCLUDE_ASM("asm/field/nonmatchings/main/misc4", func_800799D4);


// Quad rendering
// --------------------
void FieldClampPolyFT4UVs(POLY_FT4* poly, short u0, short v0, short u1, short v1, short u2, short v2, short u3, short v3) {
    if (u0 < 0) u0 = 0;
    if (u1 < 0) u1 = 0;
    if (u2 < 0) u2 = 0;
    if (u3 < 0) u3 = 0;
    if (v0 < 0) v0 = 0;
    if (v1 < 0) v1 = 0;
    if (v2 < 0) v2 = 0;
    if (v3 < 0) v3 = 0;
    
    if (u0 >= 0x100) u0 = 0xFF;
    if (u1 >= 0x100) u1 = 0xFF;
    if (u2 >= 0x100) u2 = 0xFF;
    if (u3 >= 0x100) u3 = 0xFF;
    if (v0 >= 0x100) v0 = 0xFF;
    if (v1 >= 0x100) v1 = 0xFF;
    if (v2 >= 0x100) v2 = 0xFF;
    if (v3 >= 0x100) v3 = 0xFF;

    poly->u0 = u0;
    poly->v0 = v0;
    poly->u1 = u1;
    poly->v1 = v1;
    poly->u2 = u2;
    poly->v2 = v2;
    poly->u3 = u3;
    poly->v3 = v3;
}

INCLUDE_ASM("asm/field/nonmatchings/main/misc4", func_8007A5C4);
/* Best attempt: 77.28% (568 vs 560 bytes): vx/vz interleave reproduced, but gcc
   strength-reduces &polys[1] (the copy at the end of the body) into an induction
   register and addresses polys[0] off it with negative offsets, spilling the
   uv[7] pointer to the stack: 10 callee-saved slots vs the original 9.
extern u_short D_800ADE30[]; // X0,Z0 -> X3,Z3 positions per direction
extern u_char D_800ADE70[];  // U0,V0 -> U3,V3s per direction
extern Quad D_800B0FEC[];

void func_8007A5C4(void) {
    int i;
    Quad* pQuad;

    for (i = 0, pQuad = D_800B0FEC; i < 4; i++, pQuad++) {
        SetPolyFT4(&pQuad->polys[0]);

        pQuad->vertices[0].vx = D_800ADE30[i * 8 + 0];
        pQuad->vertices[0].vy = 0;
        pQuad->vertices[0].vz = D_800ADE30[i * 8 + 1];
        pQuad->vertices[1].vx = D_800ADE30[i * 8 + 2];
        pQuad->vertices[1].vy = 0;
        pQuad->vertices[1].vz = D_800ADE30[i * 8 + 3];
        pQuad->vertices[2].vx = D_800ADE30[i * 8 + 4];
        pQuad->vertices[2].vy = 0;
        pQuad->vertices[2].vz = D_800ADE30[i * 8 + 5];
        pQuad->vertices[3].vx = D_800ADE30[i * 8 + 6];
        pQuad->vertices[3].vy = 0;
        pQuad->vertices[3].vz = D_800ADE30[i * 8 + 7];

        setRGB0(&pQuad->polys[0], 0x80, 0x80, 0x80);

        FieldClampPolyFT4UVs(&pQuad->polys[0],
            D_800ADE70[i * 8 + 0], D_800ADE70[i * 8 + 1] + 0xC0,
            D_800ADE70[i * 8 + 2], D_800ADE70[i * 8 + 3] + 0xC0,
            D_800ADE70[i * 8 + 4], D_800ADE70[i * 8 + 5] + 0xC0,
            D_800ADE70[i * 8 + 6], D_800ADE70[i * 8 + 7] + 0xC0);

        SetSemiTrans(&pQuad->polys[0], 1);
        pQuad->polys[0].tpage = GetTPage(0, 2, 0x280, 0x1C0);
        pQuad->polys[0].clut = GetClut(0x100, 0xF2);

        pQuad->polys[1] = pQuad->polys[0];
    }
}
   Previous note: 75.64% (568 vs 560 bytes). Indexing the flat arrays by symbol
   reproduces the six middle entries and the first/last pointers correctly.
*/


extern SVec4 D_800ADCE0[]; // X0 -> X3 positions
extern SVec4 D_800ADD28[]; // Y0 -> Y3 positions
extern SVec4 D_800ADD70[]; // U0 -> U3s
extern SVec4 D_800ADDB8[]; // V0 -> V3s

// Texture / Clut stuff
extern short D_800ADE00[];
extern short D_800ADE02[];
extern short D_800ADE04[];
extern short D_800ADE06[];
extern short D_800ADE08[];
extern short D_800ADE0A[];

// Compass primitive initialization?
void func_8007A7F4(Quad* pPart, int x, int y, int tex) {
    int nIndexTex;
    POLY_FT4* pNextPoly;
    char _pad[0x88];

    pNextPoly = &pPart->polys[1]; 
    SetPolyFT4(&pPart->polys[0]);
    
    nIndexTex = tex * 0x6,

    // Set Position 0, 1, 2, and 3
    pPart->vertices[0].vx = D_800ADCE0[x].x;
    pPart->vertices[0].vy = 0x0;
    pPart->vertices[0].vz = D_800ADD28[y].x;

    pPart->vertices[1].vx = D_800ADCE0[x].y;
    pPart->vertices[1].vy = 0;
    pPart->vertices[1].vz = D_800ADD28[y].y;

    pPart->vertices[2].vx = D_800ADCE0[x].z;
    pPart->vertices[2].vy = 0;
    pPart->vertices[2].vz = D_800ADD28[y].z;

    pPart->vertices[3].vx = D_800ADCE0[x].w;
    pPart->vertices[3].vy = 0;
    pPart->vertices[3].vz = D_800ADD28[y].w;
    
    setRGB0(&pPart->polys[0], 0x80, 0x80, 0x80);
    
    pPart->polys[0].tpage = GetTPage(
        D_800ADE00[nIndexTex], 
        D_800ADE02[nIndexTex], 
        D_800ADE04[nIndexTex], 
        D_800ADE06[nIndexTex]
    );
    
    pPart->polys[0].clut = GetClut(D_800ADE08[nIndexTex], D_800ADE0A[nIndexTex]);
    
    FieldClampPolyFT4UVs(&pPart->polys[0], 
        D_800ADD70[x].x, D_800ADDB8[y].x, 
        D_800ADD70[x].y, D_800ADDB8[y].y, 
        D_800ADD70[x].z, D_800ADDB8[y].z, 
        D_800ADD70[x].w, D_800ADDB8[y].w
    );

    *pNextPoly = pPart->polys[0];
}

// Actor primitive initialization
void func_8007AA44(Quad* pQuad) {
    POLY_FT4* pPoly;
    POLY_FT4* pPoly2;

    pPoly = &pQuad->polys[0];
    pPoly2 = &pQuad->polys[1];
    
    SetPolyFT4(pPoly);
    pQuad->vertices[3].vx = -0x18;
    pQuad->vertices[3].vz = -0x18;
    pQuad->vertices[3].vy = 0;

    pQuad->vertices[2].vx = 0x18;
    pQuad->vertices[2].vy = 0;
    pQuad->vertices[2].vz = -0x18;

    pQuad->vertices[1].vx = -0x18;
    pQuad->vertices[1].vy = 0;
    pQuad->vertices[1].vz = 0x18;

    pQuad->vertices[0].vx = 0x18;
    pQuad->vertices[0].vy = 0;
    pQuad->vertices[0].vz = 0x18;

    setRGB0(&pQuad->polys[0], 0x80, 0x80, 0x80);
    pQuad->polys[0].tpage = GetTPage(0, 2, 0x280, 0x1E0);
    pQuad->polys[0].clut = GetClut(0x100, 0xF3);
    SetSemiTrans(&pQuad->polys[0], 1);

    pPoly->v0 = 0xE0;
    pPoly->v1 = 0xE0;
    pPoly->u0 = 0;
    pPoly->u1 = 0xF;
    pPoly->u2 = 0;
    pPoly->v2 = 0xEF;
    pPoly->u3 = 0xF;
    pPoly->v3 = 0xEF;
    
    *pPoly2 = *pPoly;
}

void FieldRenderQuad(u_long* ot, Quad* pQuad, MATRIX* pMatTransform, int renderContext) {
    long nInterpolation;
    long nFlag;
    POLY_FT4* pPoly;

    pPoly = &pQuad->polys[renderContext];
    PushMatrix();

    // Set our GTE transform matrix (RTM and TRV)
    SetRotMatrix(pMatTransform);
    SetTransMatrix(pMatTransform);

    // Transform our vertices to screen coordinates
    RotAverage4(
        &pQuad->vertices[0], &pQuad->vertices[1], &pQuad->vertices[2], &pQuad->vertices[3], 
        (long*)&pPoly->x0, (long*)&pPoly->x1, (long*)&pPoly->x2, (long*)&pPoly->x3, 
        &nInterpolation, &nFlag
    );

    // Queue our primitive for rendering
    addPrim(ot + 1, pPoly);

    PopMatrix();
}

void func_8007AC58(u_long* ot, Quad* pQuad, MATRIX* pMatTransform, int renderContext) {
    int nInterpolation;
    int nFlag;
    int nPosY;
    int nPosX1;
    POLY_FT4* pPoly;
    int nPosX2;

    pPoly = &pQuad->polys[renderContext];
    PushMatrix();

    // Set our GTE transform matrix (RTM and TRV)
    SetRotMatrix(pMatTransform);
    SetTransMatrix(pMatTransform);

    // Transform our vertices to screen coordinates
    RotAverage4(
        &pQuad->vertices[0], &pQuad->vertices[1], &pQuad->vertices[2], &pQuad->vertices[3], 
        (long*)&pPoly->x0, (long*)&pPoly->x1, (long*)&pPoly->x2, (long*)&pPoly->x3, 
        &nInterpolation, &nFlag
    );
    
    nPosX1 = (pPoly->x3 + pPoly->x2) / 2;
    nPosY = pPoly->y3;

    nPosX2 = nPosX1 + 8;
    nPosX1 = nPosX1 - 8;
    setXY4(pPoly,
        nPosX1, nPosY - 10,
        nPosX2, nPosY - 10,
        nPosX1, nPosY,
        nPosX2, nPosY
    );
    
    // Queue our primitive for rendering
    addPrim(ot + 1, pPoly);

    PopMatrix();
}


// Controller stuff
// --------------------
extern s32 g_pFieldControllerBuffer1;
extern s32 g_pFieldControllerBuffer2;

extern u_short g_FieldMouseSpeedX;
extern u_short g_FieldMouseSpeedY;
extern s32 D_800C3A44;
extern s32 D_800C3A4C;
extern s32 D_800C3A50;
extern s32 D_800C3A54;
extern int g_FieldMousePositionsX[2];
extern int g_FieldMousePositionsY[2];

void FieldSetControllerBuffers(void* controllerBuffer1, void* controllerBuffer2) {
    g_pFieldControllerBuffer1 = controllerBuffer1;
    g_pFieldControllerBuffer2 = controllerBuffer2;
}

void func_8007ADA4(int arg0, int arg1, int arg2, int arg3) {
    D_800C3A44 = arg0 * g_FieldMouseSpeedX;
    D_800C3A50 = arg1 * g_FieldMouseSpeedX;
    D_800C3A4C = arg2 * g_FieldMouseSpeedY;
    D_800C3A54 = arg3 * g_FieldMouseSpeedY;
}

void FieldSetMouseSpeed(u_short xSpeed, u_short ySpeed) {
    g_FieldMouseSpeedX = xSpeed;
    g_FieldMouseSpeedY = ySpeed;
}

void FieldSetMousePosition(int mouseIndex, int xMovement, int yMovement) {
    g_FieldMousePositionsX[mouseIndex] = xMovement * g_FieldMouseSpeedX;
    g_FieldMousePositionsY[mouseIndex] = yMovement * g_FieldMouseSpeedY;
}

void func_8007AF74(int controllerIndex);

INCLUDE_ASM("asm/field/nonmatchings/main/misc4", func_8007AE78);
/* Best attempt: 93.65%. Only difference is scheduling around the byte-1 test:
   the original emits nop; lb v1,1(s0); li v0,0x12 and this puts li,lb before
   the branch (plus a nop for the load delay). Repeating the array expression
   per use is required: it reproduces the three reloads of the buffer pointer.
   A version that loaded both bytes into locals (95.87%) gets li after lb but
   hoists both loads above the bnez.
   Round 42: confirmed the mechanism - gcc emits the constant before the load
   inside the && block, and reorg then steals the li into the bnez delay slot,
   which forces a real load-delay nop after the lb. Nested if, a local for byte
   1 only, and swapping the comparison operands all give the identical 252-byte
   object; the original's 252 bytes exist only with the load emitted first, so
   the block has to start with a non-movable insn.
void func_8007AE78(int controllerIndex, int* pOut) {
    func_8007AF74(controllerIndex);
    pOut[0] = g_FieldMousePositionsX[controllerIndex] / g_FieldMouseSpeedX;
    pOut[1] = g_FieldMousePositionsY[controllerIndex] / g_FieldMouseSpeedY;
    pOut[2] = -0x100;
    pOut[3] = ((s8**)&g_pFieldControllerBuffer1)[controllerIndex][4];
    pOut[4] = ((s8**)&g_pFieldControllerBuffer1)[controllerIndex][5];
    if (!((s8**)&g_pFieldControllerBuffer1)[controllerIndex][0]
        && ((s8**)&g_pFieldControllerBuffer1)[controllerIndex][1] == 0x12) {
        pOut[2] = ~((s8**)&g_pFieldControllerBuffer1)[controllerIndex][3] & 0xC;
    }
}
*/

void func_8007AF74(int controllerIndex) {
    if (((s8**)&g_pFieldControllerBuffer1)[controllerIndex][0] == 0
        && ((s8**)&g_pFieldControllerBuffer1)[controllerIndex][1] == 0x12) {
        g_FieldMousePositionsX[controllerIndex] += ((s8**)&g_pFieldControllerBuffer1)[controllerIndex][4];
        g_FieldMousePositionsY[controllerIndex] += ((s8**)&g_pFieldControllerBuffer1)[controllerIndex][5];
        if (g_FieldMousePositionsX[controllerIndex] > D_800C3A50) {
            g_FieldMousePositionsX[controllerIndex] = D_800C3A50;
        } else if (g_FieldMousePositionsX[controllerIndex] < D_800C3A44) {
            g_FieldMousePositionsX[controllerIndex] = D_800C3A44;
        }
        if (g_FieldMousePositionsY[controllerIndex] > D_800C3A54) {
            g_FieldMousePositionsY[controllerIndex] = D_800C3A54;
        } else if (g_FieldMousePositionsY[controllerIndex] < D_800C3A4C) {
            g_FieldMousePositionsY[controllerIndex] = D_800C3A4C;
        }
    }
}

// Returns the Y of the point on the plane spanned by pA/pB/pC whose X/Z is pOut's
void func_8007B07C(SVECTOR* pA, SVECTOR* pB, SVECTOR* pC, SVECTOR* pOut, VECTOR* pNormal) {
    VECTOR nB;
    VECTOR nC;
    VECTOR v;

    v.vx = pB->vx - pA->vx;
    v.vy = pB->vy - pA->vy;
    v.vz = pB->vz - pA->vz;
    VectorNormal(&v, &nB);

    v.vx = pC->vx - pA->vx;
    v.vy = pC->vy - pA->vy;
    v.vz = pC->vz - pA->vz;
    VectorNormal(&v, &nC);

    OuterProduct12(&nB, &nC, pNormal);
    if (pNormal->vy == 0) {
        pOut->vy = 0;
    } else {
        pOut->vy = pA->vy + (-(pNormal->vx * (pOut->vx - pA->vx)) - pNormal->vz * (pOut->vz - pA->vz)) / pNormal->vy;
    }
}

INCLUDE_ASM("asm/field/nonmatchings/main/misc4", func_8007B1C4);
/* Best attempt: 95.27% (696 bytes; the original is 692). The loop body, the
   three NCLIP rejection tests (sxy3 n0,n1,nRef / n1,n2,nRef / n2,n0,nRef) and
   the nine halfword copies into v[] are instruction-identical; needs
   psyq/inline_c.h for the 4.x gte_ldsxy3/gte_nclip/gte_stopz spellings, whose
   mtc2 $12/$14/$13 order, double nop and swc2 $24 match retail exactly.
   Remaining deltas, all in the prologue and impossible to reach by guessing
   source shape: (1) gcc copies pVerts a3->t0 for the three header uses, one
   extra instruction; (2) the original frame is 0xA0 with locals at 0x48
   (v[3]), 0x60 (ref) and 0x68/0x6C/0x70 (the three stopz temporaries), i.e. 48
   bytes of locals allocated before v[3] that no source line uses, and it saves
   $s2/$s3/$s4 without ever using them - the retail object came from a source
   with three more call-crossing locals, or is a patched object (splat marks
   this stub "Handwritten function", and so does the jtbl-owning func_8007CD80).
   Passing x/z as s16 instead of int adds an sll/sra pair; u16 vertex components
   turn the eight lh into lhu; pTri[i] indexing (instead of pTri++) is required
   or gcc rebases the reads onto pTri+4.
typedef struct {
    s16 v0;
    s16 v1;
    s16 v2;
    undefined8 pad6[0x8];
} FieldWalkmeshTriIdx; // Size: 0xE

typedef struct {
    s16 vx;
    s16 vy;
    s16 vz;
    u16 pad6;
} FieldWalkmeshVert; // Size: 0x8

extern FieldWalkmeshTriIdx* D_800AFB24[];
extern FieldWalkmeshVert* D_800AFB34[];
extern s32 D_800AFB44[];

// Index of the walkmesh triangle that contains pRef's x/z, 0 when none does
int func_8007B1C4(int x, int z, int nWalkmesh, SVECTOR* pOut, VECTOR* pNormal) {
    SVECTOR v[3];
    SVECTOR ref;
    FieldWalkmeshTriIdx* pTri;
    FieldWalkmeshVert* pVerts;
    u_short* pComp;
    int nCount;
    int nRef;
    int n0;
    int n1;
    int n2;
    long nTemp;
    long nTemp2;
    long nTemp3;
    int i;

    ref.vx = x;
    ref.vy = 0;
    ref.vz = z;
    nRef = (x << 16) + z;
    pTri = D_800AFB24[nWalkmesh];
    nCount = D_800AFB44[nWalkmesh];
    pVerts = D_800AFB34[nWalkmesh];
    pComp = (u_short*)pVerts;

    for (i = 0; i < nCount; i++) {
        n0 = (pVerts[pTri[i].v0].vx << 16) + pVerts[pTri[i].v0].vz;
        n1 = (pVerts[pTri[i].v1].vx << 16) + pVerts[pTri[i].v1].vz;
        n2 = (pVerts[pTri[i].v2].vx << 16) + pVerts[pTri[i].v2].vz;

        gte_ldsxy3(n0, n1, nRef);
        gte_nclip();
        gte_stopz(&nTemp);
        if (nTemp < 0) {
            continue;
        }
        gte_ldsxy3(n1, n2, nRef);
        gte_nclip();
        gte_stopz(&nTemp2);
        if (nTemp2 < 0) {
            continue;
        }
        gte_ldsxy3(n2, n0, nRef);
        gte_nclip();
        gte_stopz(&nTemp3);
        if (nTemp3 < 0) {
            continue;
        }

        v[0].vx = pComp[pTri[i].v0 * 4 + 0];
        v[0].vy = pComp[pTri[i].v0 * 4 + 1];
        v[0].vz = pComp[pTri[i].v0 * 4 + 2];
        v[1].vx = pComp[pTri[i].v1 * 4 + 0];
        v[1].vy = pComp[pTri[i].v1 * 4 + 1];
        v[1].vz = pComp[pTri[i].v1 * 4 + 2];
        v[2].vx = pComp[pTri[i].v2 * 4 + 0];
        v[2].vy = pComp[pTri[i].v2 * 4 + 1];
        v[2].vz = pComp[pTri[i].v2 * 4 + 2];

        func_8007B07C(&v[0], &v[1], &v[2], &ref, pNormal);
        pOut->vx = ref.vx;
        pOut->vy = ref.vy;
        pOut->vz = ref.vz;
        return i;
    }

    pOut->vx = 0;
    pOut->vy = 0;
    pOut->vz = 0;
    pNormal->vx = 0;
    pNormal->vy = 0;
    pNormal->vz = 0;
    return 0;
}
*/

// 1 when the triangle pA/pB/pC winds consistently seen from pRef, -1 when it folds back
int func_8007B478(SVECTOR* pA, SVECTOR* pB, SVECTOR* pC, SVECTOR* pRef) {
    VECTOR v;
    VECTOR edge;
    VECTOR cross;

    v.vx = pB->vx - pA->vx;
    v.vy = 0;
    v.vz = pB->vz - pA->vz;
    edge.vx = pRef->vx - pA->vx;
    edge.vy = 0;
    edge.vz = pRef->vz - pA->vz;
    OuterProduct0(&v, &edge, &cross);
    if (cross.vy < 0) {
        return -1;
    }

    v.vx = pC->vx - pB->vx;
    v.vy = 0;
    v.vz = pC->vz - pB->vz;
    edge.vx = pRef->vx - pB->vx;
    edge.vy = 0;
    edge.vz = pRef->vz - pB->vz;
    OuterProduct0(&v, &edge, &cross);
    if (cross.vy >= 0) {
        v.vx = pA->vx - pC->vx;
        v.vy = 0;
        v.vz = pA->vz - pC->vz;
        edge.vx = pRef->vx - pC->vx;
        edge.vy = 0;
        edge.vz = pRef->vz - pC->vz;
        OuterProduct0(&v, &edge, &cross);
        return cross.vy >> 31;
    }
    return -1;
}

INCLUDE_ASM("asm/field/nonmatchings/main/misc4", func_8007B614);
/* Best attempt: 97.88%. Only a rotation of the call-saved registers differs: the
   original has pVec=s2, nAngle=s1, nScale=s0, gcc gives pVec=s1, nAngle=s0,
   nScale=s2 (same instruction order). Declaration and statement order, commuted
   multiplies, split temporaries, split shift, s16/u16 angle and x/z temps all
   reproduce exactly this allocation; the y store must come last in the source so
   it lands after the rcos multiply.
extern s16 D_800B218C;

void func_8007B614(IVEC3* pVec, int scale, int angle) {
    int nAngle = angle & 0xFFF;
    int nScale = (scale << 4) * D_800B218C >> 12;

    pVec->x = rsin(nAngle) * nScale;
    pVec->z = -(rcos(nAngle) * nScale);
    pVec->y = 0;
}
*/

int func_8007B694(IVEC3* pVec) {
    return -ratan2(pVec->z, pVec->x) & 0xFFF;
}

long FieldGetVec2Magnitude(long, long);

INCLUDE_ASM("asm/field/nonmatchings/main/misc4", func_8007B6C4);
/* Best attempt: 84.11% but the exact 332-byte size (earlier attempts were 336
   bytes, which can never link). Seeding nResult with nWallAngle before the range
   test is the only form that puts `move s3,a0` in the bnez delay slot and
   `addiu a0,a0,0x800` in the beqz delay slot like the original; the remaining
   delta is one extra copy of nWallAngle (the original has `move v1,a0` feeding
   both the angle sum and the early `return nWallAngle`).
long FieldGetVec2Magnitude(long, long);

int func_8007B6C4(s16 angle, SVECTOR* pPoints, VECTOR* pOut) {
    VECTOR v;
    VECTOR normal;
    int nAngle;
    int nWallAngle;
    long nResult;
    long nMagnitude;

    nAngle = (0xC00 - angle) & 0xFFF;
    nWallAngle = -ratan2(pPoints[1].vz - pPoints[0].vz, pPoints[1].vx - pPoints[0].vx) & 0xFFF;
    nAngle = (nAngle + nWallAngle) & 0xFFF;
    nResult = nWallAngle;
    if ((u_int)(nAngle - 0x80) >= 0xF01) {
        pOut->vx = 0;
        pOut->vy = 0;
        pOut->vz = 0;
        return nWallAngle;
    }
    if (nAngle < 0x800) {
        v.vx = pPoints[0].vx - pPoints[1].vx;
        v.vy = 0;
        v.vz = pPoints[0].vz - pPoints[1].vz;
        nResult = (nWallAngle + 0x800) & 0xFFF;
    } else {
        v.vx = pPoints[1].vx - pPoints[0].vx;
        v.vy = 0;
        v.vz = pPoints[1].vz - pPoints[0].vz;
    }
    VectorNormal(&v, &normal);

    nMagnitude = FieldGetVec2Magnitude(pOut->vx >> 12, pOut->vz >> 12);
    pOut->vx = normal.vx * nMagnitude;
    pOut->vy = 0;
    pOut->vz = normal.vz * nMagnitude;

    return nResult;
}
*/

int func_8007C694(VECTOR* pVec, VECTOR* pPos, ActorData* pActor, SVECTOR* pPoints, SVECTOR* pOut, int nMode);
int func_8007B6C4(s16 angle, SVECTOR* pPoints, VECTOR* pOut);
extern s32 D_800ADB98;

INCLUDE_ASM("asm/field/nonmatchings/main/misc4", func_8007B814);
/* Best attempt: 96.33% (684 bytes, same size as the original). Two differences:
   1. gcc hoists the tail's first `lw v0,0(pVec)` out of the shared func_8007B6C4
      block into the block that jumps there, so its `j` target is 4 bytes later.
   2. the probe angles: the original computes (angle -/+ 0x100) into s0 itself
      (`addiu s0,s2,-0x100` / `andi s0,s0,0xfff`), gcc computes the subtraction
      into v0 and only the mask into s0. Note the `sra s2,v0,16` at the top of the
      else branch is not an assignment from the probe result: the bne delay slot
      is `sll v0,s2,16`, so it is the sign extension of `angle` for `angle-0x100`,
      and both objects agree on it. Prototypes below are the ones this attempt
      needs; D_800ADB98 = 0x800ADB98 is in symbol_addrs.field.txt.
// Probes the walkmesh around `angle` and drops pVec onto the floor it finds there
int func_8007B814(VECTOR* pVec, ActorData* pActor, SVECTOR* pPoints, s16 angle) {
    VECTOR v;
    SVECTOR out;
    VECTOR pad[2];
    int nAngle;

    nAngle = angle & 0xFFF;
    v.vx = pVec->vx + rsin(nAngle) * 64;
    v.vz = pVec->vz - rcos(nAngle) * 64;
    if (func_8007C694(&v, &pActor->position, pActor, pPoints, &out, -1) == -1) {
        v.vx = pVec->vx;
        v.vy = pVec->vy;
        v.vz = pVec->vz;
        func_8007B6C4(angle, pPoints, &v);
    } else {
        nAngle = (angle - 0x100) & 0xFFF;
        v.vx = pVec->vx + rsin(nAngle) * 64;
        v.vz = pVec->vz - rcos(nAngle) * 64;
        if (func_8007C694(&v, &pActor->position, pActor, pPoints, &out, -1) == -1) {
            v.vx = pVec->vx;
            v.vy = pVec->vy;
            v.vz = pVec->vz;
            func_8007B6C4(angle, pPoints, &v);
        } else {
            nAngle = (angle + 0x100) & 0xFFF;
            v.vx = pVec->vx + rsin(nAngle) * 64;
            v.vz = pVec->vz - rcos(nAngle) * 64;
            if (func_8007C694(&v, &pActor->position, pActor, pPoints, &out, -1) == -1) {
                v.vx = pVec->vx;
                v.vy = pVec->vy;
                v.vz = pVec->vz;
                func_8007B6C4(angle, pPoints, &v);
            } else {
                v.vx = pVec->vx;
                v.vy = pVec->vy;
                v.vz = pVec->vz;
            }
        }
    }

    if (func_8007C694(&v, &pActor->position, pActor, pPoints, &out, 0) == -1) {
        return -1;
    }
    if (pActor->scriptFlags.flags & 0x40000) {
        out.vy = *(u_short*)&pActor->unkEC;
    } else if ((out.vy << 16) < pActor->position.vy && D_800ADB98 == 0) {
        return -1;
    }
    v.vy = (out.vy << 16) - pActor->position.vy;
    pVec->vx = v.vx;
    pVec->vy = v.vy;
    pVec->vz = v.vz;
    pActor->curYPos = (pActor->position.vy + pVec->vy) >> 16;
    return 0;
}
*/

INCLUDE_ASM("asm/field/nonmatchings/main/misc4", func_8007BAC0);

INCLUDE_ASM("asm/field/nonmatchings/main/misc4", func_8007BEF4);

int func_8007C670(int* pCur, int* pNext, int n) {
    int value = *pCur;

    if (n < 0) {
        *pCur = value;
        *pNext = value;
        return value;
    }
    *pCur = value;
    value += n;
    *pNext = value;
    return value;
}

INCLUDE_ASM("asm/field/nonmatchings/main/misc4", func_8007C694);

extern int D_800ADC10;

// Pointer into the scratchpad area for the script VM
int func_8007CD3C(int size) {
    int offset = D_800ADC10;

    D_800ADC10 = offset + size;
    return 0x1F800000 + offset * 4;
}

void func_8007CD60(int size) {
    D_800ADC10 -= size;
}

INCLUDE_ASM("asm/field/nonmatchings/main/misc4", func_8007CD80);

INCLUDE_ASM("asm/field/nonmatchings/main/misc4", func_8007D3D4);

int func_8007D8B4(int x, int y, int z);

void func_8007D818(VECTOR* pVec, VECTOR* pOut) {
    int nDominantAxis;

    nDominantAxis = func_8007D8B4(pVec->vx, pVec->vy, pVec->vz);
    pVec->vx >>= 12;
    pVec->vy >>= 12;
    pVec->vz >>= 12;
    if (nDominantAxis < 0) {
        pVec->vx = -pVec->vx;
        pVec->vy = -pVec->vy;
        pVec->vz = -pVec->vz;
    }
    VectorNormal(pVec, pOut);
}

// Returns whichever component has the largest absolute value
int func_8007D8B4(int x, int y, int z) {
    int nAbsX = x;
    int nAbsY = y;
    int nAbsZ = z;

    if (nAbsX < 0) {
        nAbsX = -nAbsX;
    }
    if (nAbsY < 0) {
        nAbsY = -nAbsY;
    }
    if (nAbsZ < 0) {
        nAbsZ = -nAbsZ;
    }

    if (nAbsX >= nAbsY && nAbsX >= nAbsZ) {
        return x;
    }
    if (nAbsY >= nAbsX && nAbsY >= nAbsZ) {
        return y;
    }
    if (nAbsZ >= nAbsX && nAbsZ >= nAbsY) {
        return z;
    }
    return 0;
}




