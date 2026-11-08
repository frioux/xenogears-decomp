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

extern int func_80085C90(s32 arg0);
extern s32 D_8004F308;
extern s32 D_8004F324;
extern s32 D_800ADC18;

INCLUDE_ASM("asm/field/nonmatchings/main/misc4", func_80078B5C);
/* Best attempt: 96.30%, but 104 bytes against the original's 108: gcc steals the
   decrement into the beqz delay slot, where the original has a real nop, so this
   unit can never link until that nop appears. The nop means the value the branch
   tests is live at the epilogue, but v0 already carries the decremented result
   there, and `return D_800ADC18;` only adds a reload (88.70%, 112 bytes). Tried
   and identical: volatile, a count local decremented then stored, an explicit
   goto, unsigned, and the plain form below.
extern int func_80085C90(s32 arg0);
extern s32 D_8004F308;
extern s32 D_8004F324;
extern s32 D_800ADC18;

void func_80078B5C(void) {
    rand();
    if (D_8004F308 == -1) {
        D_8004F308 = func_80085C90(D_8004F324);
    }
    if (D_800ADC18 != 0) {
        D_800ADC18--;
    }
}
*/

INCLUDE_ASM("asm/field/nonmatchings/main/misc4", func_80078BC8);
/* Best attempt: 90.68%. The original keeps -1 in v0 across the whole guard chain
   (loads go to v1) and copies ArchiveDataSync's result to v1 before testing it.
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
/* Best attempt: 75.64% (568 vs 560 bytes). Indexing the tables by symbol from the
   flat 1D arrays reproduces the original's `sll i,4; addu base,idx; lhu k(v0)`
   form for the six middle entries and its two extra per-table pointers for the
   first/last entry, but gcc spends one register more than the original: it keeps
   pQuad, &pQuad->polys[0] and &pQuad->polys[1] live (spilling the uv[7] pointer)
   and stores the vertices off the polys pointer with negative offsets instead of
   off pQuad with positive ones. Same score with pointer locals instead of the
   symbols; the remaining gap is register allocation / CSE of pQuad vs pPoly.
   Reached ~61% with 2D `D_800ADE30[i][k]` and struct-array `D_800ADE30[i].x0`
   forms (both lose the base+i*16 addressing entirely).
extern u_short D_800ADE30[];     // X0,Z0 -> X3,Z3 positions per direction
extern u_char D_800ADE70[];      // U0,V0 -> U3,V3s per direction
extern Quad D_800B0FEC[];

void func_8007A5C4(void) {
    int i;
    Quad* pQuad;

    pQuad = D_800B0FEC;
    for (i = 0; i < 4; i++) {
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
        pQuad++;
    }
}
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




