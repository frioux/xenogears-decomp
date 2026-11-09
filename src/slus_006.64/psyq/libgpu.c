#include "common.h"
#include "psyq/libetc.h"
#include "psyq/libgpu.h"

extern void memcpy(u8*, u8*, int);

extern char g_GraphDebugLevel;
extern void (*g_DrawSyncCallbackFn)();
extern int (*g_GpuPrintf)(char*, ...);

extern DRAWENV g_GpuDrawEnv;
extern DISPENV g_GpuDispEnv;

// GPU driver vtable, selected by the video mode
// (0x80056888, one entry per function; entries 3/6/7/8 hold addresses)
typedef struct {
    /* 0x00 */ char *version;
    /* 0x04 */ void (*unk04)();
    /* 0x08 */ int (*unk08)(u_long madr, void *p, int n, u_long param);
    /* 0x0C */ u_long unk0C;
    /* 0x10 */ void (*unk10)(u_long value);
    /* 0x14 */ void (*unk14)(void *p, int n);
    /* 0x18 */ u_long unk18;
    /* 0x1C */ u_long unk1C;
    /* 0x20 */ u_long unk20;
    /* 0x24 */ void (*unk24)();
    /* 0x28 */ u_long (*unk28)(int mode);
    /* 0x2C */ void (*unk2C)(u_long *ot, int n);
    /* 0x30 */ void (*unk30)();
    /* 0x34 */ void (*unk34)(int mode);
    /* 0x38 */ void (*unk38)();
    /* 0x3C */ int (*unk3C)(int mode);
} GpuSysFuncs; // Size: 0x40

extern char D_8001908C[]; // "ResetGraph:jtb=%08x,env=%08x\n"
extern char D_800190AC[]; // "ResetGraph(%d)...\n"
extern char D_800190D8[]; // "SetGraphDebug:level:%d,type:%d reverse:%d\n"
extern char D_800190C0[]; // "SetGraphReverse(%d)...\n"
extern char D_80019104[]; // "SetGrapQue(%d)...\n"
extern char D_80019134[]; // "SetDispMask(%d)...\n"
extern char D_80019148[]; // "DrawSync(%d)...\n"
extern char D_80019180[]; // "ClearImage"
extern char D_8001915C[]; // "%s:bad RECT"
extern char D_80019168[]; // "(%d,%d)-(%d,%d)\n"
extern char D_8001917C[]; // "%s:"
extern char D_8001918C[]; // "LoadImage"
extern char D_80019198[]; // "StoreImage"
extern char D_800191A4[]; // "MoveImage"
extern char D_800191B0[]; // "ClearOTag(%08x,%d)...\n"
extern char D_800191C8[]; // "ClearOTagR(%08x,%d)...\n"
extern char D_800191E0[]; // "DrawOTag(%08x)...\n"
extern char D_800191F4[]; // "PutDrawEnv(%08x)...\n"
extern char D_8001920C[]; // "DrawOTagEnv(%08x,%08x)...\n"

extern GpuSysFuncs *D_800568C8;
extern u_char D_800568D0;
extern u_char D_800568D1;
extern u_char D_800568D3;
extern char D_8005698C;

extern short D_800568D4;
extern short D_800568D6;

// MoveImage's DR_MOVE packet: tag/0x80000000 at D_80056964+0x14, then the fields
// filled in below, ending with the 0x04FFFFFF end marker at D_8005698C
extern u_long D_80056980;
extern u_long D_80056984;
extern u_long D_80056988;

extern GpuSysFuncs D_80056888; // the driver table D_800568C8 points at
extern u_int D_80056950[];     // draw-mode table: clip width/height per mode
extern u_int D_80056964[];     // draw-mode table: VRAM height per mode

extern void func_8004463C(char *name, RECT *rect);
extern void GPU_cw(u_long value);
extern u_char func_80046C58(int mode);
extern void func_8004574C(DR_ENV *pDrEnv, DRAWENV *pEnv);
extern void func_80047178(void *p, int value, int n);
extern int DMACallback(int id, void (*pCallbackFn)());

DRAWENV* SetDefDrawEnv(DRAWENV* env, int x, int y, int w, int h) {
    int nVideoMode;

    nVideoMode = GetVideoMode();
    env->clip.x = x;
    env->clip.y = y;
    env->clip.w = w;
    env->clip.h = h;
    env->tw.x = 0;
    env->tw.y = 0;
    env->tw.w = 0;
    env->tw.h = 0;
    env->r0 = 0;
    env->g0 = 0;
    env->b0 = 0;
    env->dtd = 1;
    
    if (nVideoMode != MODE_NTSC)
        env->dfe  = h < 0x121;
    else
        env->dfe  = h < 0x101;

    env->ofs[0] = x;
    env->ofs[1] = y;
    env->tpage = 0xA;
    env->isbg = 0;
    return env;
}

DISPENV* SetDefDispEnv(DISPENV* env, int x, int y, int w, int h) {
    env->disp.x = x;
    env->disp.y = y;
    env->disp.w = w;
    env->disp.h = h;
    env->screen.x = 0;
    env->screen.y = 0;
    env->screen.w = 0;
    env->screen.h = 0;
    env->isrgb24 = 0;
    env->isinter = 0;
    env->pad1 = 0;
    env->pad0 = 0;
    return env;
}

u_short GetTPage(int tp, int abr, int x, int y) {
    return getTPage(tp, abr, x, y);
}

u_short GetClut(int x, int y) {
    return getClut(x, y);
}

void DumpTPage(u_short tpage) {
    dumpTPage(tpage);
}

void DumpClut(u_short clut) {
    dumpClut(clut);
}

void* NextPrim(void *p) {
    return nextPrim(p);
}

int IsEndPrim(void *p) {
    return isendprim(p);
}

void AddPrim(void *ot, void *p) {
    addPrim(ot, p);
}

void AddPrims(void *ot, void *p0, void *p1) {
    addPrims(ot, p0, p1);
}

void CatPrim(void *p0, void *p1) {
    setaddr(p0, p1);
}

void TermPrim(void *p) {
    termPrim(p);
}

void SetSemiTrans(void *p, int abe) {
    setSemiTrans(p, abe);
}

void SetShadeTex(void *p, int tge) {
    setShadeTex(p, tge);
}

void SetPolyF3(POLY_F3 *p) {
    setlen(p, 4);
    setcode(p, 0x20);
}

void SetPolyFT3(POLY_FT3 *p) {
    setlen(p, 7);
    setcode(p, 0x24);
}

void SetPolyG3(POLY_G3 *p) {
    setlen(p, 6);
    setcode(p, 0x30);
}

void SetPolyGT3(POLY_GT3 *p) {
    setlen(p, 9);
    setcode(p, 0x34);
}

void SetPolyF4(POLY_F4 *p) {
    setlen(p, 5);
    setcode(p, 0x28);
}

void SetPolyFT4(POLY_FT4 *p) {
    setlen(p, 9);
    setcode(p, 0x2C);
}

void SetPolyG4(POLY_G4 *p) {
    setlen(p, 8);
    setcode(p, 0x38);
}

void SetPolyGT4(POLY_GT4 *p) {
    setlen(p, 12);
    setcode(p, 0x3C);
}

void SetSprt8(SPRT_8 *p) {
    setlen(p, 3);
    setcode(p, 0x74);
}

void SetSprt16(SPRT_16 *p) {
    setlen(p, 3);
    setcode(p, 0x7C);
}

void SetSprt(SPRT *p) {
    setlen(p, 4);
    setcode(p, 0x64);
}

void SetTile1(TILE_1 *p) {
    setlen(p, 2);
    setcode(p, 0x68);
}

void SetTile8(TILE_8 *p) {
    setlen(p, 2);
    setcode(p, 0x70);
}

void SetTile16(TILE_16 *p) {
    setlen(p, 2);
    setcode(p, 0x78);
}

void SetTile(TILE *p) {
    setlen(p, 3);
    setcode(p, 0x60);
}

void SetLineF2(LINE_F2 *p) {
    setlen(p, 3);
    setcode(p, 0x40);
}

void SetLineG2(LINE_G2 *p) {
    setlen(p, 4);
    setcode(p, 0x50);
}

void SetLineF3(LINE_F3 *p) {
    setlen(p, 5);
    setcode(p, 0x48);
    p->pad = 0x55555555;
}

void SetLineG3(LINE_G3 *p) {
    setlen(p, 7);
    setcode(p, 0x58);
    p->pad = 0x55555555;
}

void SetLineF4(LINE_F4 *p) {
    setlen(p, 6);
    setcode(p, 0x4c);
    p->pad = 0x55555555;
}

void SetLineG4(LINE_G4 *p) {
    setlen(p, 9);
    setcode(p, 0x5c);
    p->pad = 0x55555555;
}

void SetDrawTPage(DR_TPAGE *p, int dfe, int dtd, int tpage) {
    setlen(p, 1);
    ((u_long *)(p))[1] = _get_mode(dfe, dtd, tpage);
}

void SetDrawMove(DR_MOVE *p, RECT *rect, int x, int y) {
    int len = 5;

    if (rect->w == 0 || rect->h == 0)
        len = 0;

    setlen(p, len);
    p->code[0] = 0x01000000;
    p->code[1] = 0x80000000;
    *(u_long*)&p->code[2] = *(u_long*)&rect->x;
    *(u_long*)&p->code[3] = (y << 16) | (x & 0xFFFF);
    *(u_long*)&p->code[4] = *(u_long*)&rect->w;
}

void func_80043EAC(DR_LOAD *p, RECT *rect) {
    u_int len = (rect->w * rect->h + 1) / 2;
    u_int nLen = len + 4;

    if (len >= 13)
        nLen = 0;

    setlen(p, nLen);
    p->code[0] = 0xA0000000;
    *(u_long*)&p->code[1] = *(u_long*)&rect->x;
    *(u_long*)&p->code[2] = *(u_long*)&rect->w;
    p->p[nLen - 4] = 0x01000000;
}

int func_80043F18(u_char *p0, u_char *p1) {
    int len = p0[3] + p1[3] + 1;

    if (len >= 0x11)
        return -1;

    p0[3] = len;
    *(u_long*)p1 = 0;
    return 0;
}

void DumpDrawEnv(DRAWENV *env) {
    g_GpuPrintf("clip (%3d,%3d)-(%d,%d)\n", env->clip.x, env->clip.y, env->clip.w, env->clip.h);
    g_GpuPrintf("ofs  (%3d,%3d)\n", env->ofs[0], env->ofs[1]);
    g_GpuPrintf("tw   (%d,%d)-(%d,%d)\n", env->tw.x, env->tw.y, env->tw.w, env->tw.h);
    g_GpuPrintf("dtd   %d\n", env->dtd);
    g_GpuPrintf("dfe   %d\n", env->dfe);
    dumpTPage(env->tpage);
}

void DumpDispEnv(DISPENV *env) {
    g_GpuPrintf("disp   (%3d,%3d)-(%d,%d)\n", env->disp.x, env->disp.y, env->disp.w, env->disp.h);
    g_GpuPrintf("screen (%3d,%3d)-(%d,%d)\n", env->screen.x, env->screen.y, env->screen.w, env->screen.h);
    g_GpuPrintf("isinter %d\n", env->isinter);
    g_GpuPrintf("isrgb24 %d\n", env->isrgb24);
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgpu", ResetGraph);

/* best: 92.76% -- everything matches up to the case-5 write block: gcc hoists
   both D_80056950/D_80056964 loads above the D_800568D1 store and materialises
   li a1,-1 early, retail keeps them in source order (asm-level scheduling again,
   as with MoveImage). Register allocation (s0 = pLevel, s1 = mode) is right and
   needs pLevel assigned inside case 5 with printf taking &D_800568D0 directly.
int ResetGraph(int mode) {
    u_char *pLevel;
    u_char nMode;

    switch (mode & 7) {
    case 0:
    case 3:
        printf(D_8001908C, &D_80056888, &D_800568D0);
    case 5:
        pLevel = &D_800568D0;
        func_80047178(pLevel, 0, 0x80);
        ResetCallback();
        GPU_cw((u_long)D_800568C8 & 0xFFFFFF);
        nMode = func_80046C58(mode);
        *pLevel = nMode;
        D_800568D1 = 1;
        D_800568D4 = D_80056950[nMode];
        D_800568D6 = D_80056964[nMode];
        func_80047178(pLevel + 0x10, -1, sizeof(DRAWENV));
        func_80047178(pLevel + 0x6C, -1, sizeof(DISPENV));
        return *pLevel;
    default:
        if (g_GraphDebugLevel >= 2)
            g_GpuPrintf(D_800190AC, mode);
        D_800568C8->unk34(1);
        break;
    }
}
*/

int SetGraphReverse(int mode) {
    u_long nStatus;
    u_char *pReverse = &D_800568D3;
    int nPrev = *pReverse;

    if (g_GraphDebugLevel >= 2)
        g_GpuPrintf(D_800190C0, mode);

    *pReverse = mode;
    nStatus = D_800568C8->unk28(8);
    if (*pReverse)
        nStatus |= 0x8000080;
    else
        nStatus |= 0x8000000;
    D_800568C8->unk10(nStatus);

    if (D_800568D0 == 2) {
        GpuSysFuncs *pSys = D_800568C8;
        u_long nMode = 0x20000504;

        if (D_800568D3)
            nMode = 0x20000501;
        pSys->unk10(nMode);
    }
    return nPrev;
}

int SetGraphDebug(int level) {
    u_char *pLevel = &g_GraphDebugLevel;
    int nPrev = *pLevel;
    u_char nLevel = level;

    *pLevel = level;
    if (nLevel)
        g_GpuPrintf(&D_800190D8, nLevel, D_800568D0, D_800568D3);
    return nPrev;
}

int func_8004440C(int que) {
    u_char *pQue = &D_800568D1;
    int nPrev = *pQue;

    if (g_GraphDebugLevel >= 2)
        g_GpuPrintf(&D_80019104, que);

    if (que != *pQue) {
        D_800568C8->unk34(1);
        *pQue = que;
        DMACallback(2, 0);
    }
    return nPrev;
}

int func_800444B8(void) {
    return D_800568D0;
}

int GetGraphDebug(void) {
    return g_GraphDebugLevel;
}

extern char D_80019118; // "DrawSyncCallback(%08x)...\n"
u_long DrawSyncCallback(void (*pCallbackFn)()) {
    void (*pPrevCallbackFn)();

    if (g_GraphDebugLevel >= 2)
        g_GpuPrintf(&D_80019118, pCallbackFn);

    pPrevCallbackFn = g_DrawSyncCallbackFn;
    g_DrawSyncCallbackFn = pCallbackFn;
    return (u_long) pPrevCallbackFn;
}

void SetDispMask(int mask) {
    u_char *pLevel = &g_GraphDebugLevel;

    if (*pLevel >= 2)
        g_GpuPrintf(&D_80019134, mask);

    // 0x6A past the debug level is g_GpuDispEnv; the original reaches it from here
    if (mask == 0)
        func_80047178(pLevel + 0x6A, -1, sizeof(DISPENV));

    D_800568C8->unk10(mask ? 0x03000000 : 0x03000001);
}

int DrawSync(int mode) {
    if (g_GraphDebugLevel >= 2)
        g_GpuPrintf(&D_80019148, mode);

    return D_800568C8->unk3C(mode);
}

void func_8004463C(char *name, RECT *rect) {
    switch (g_GraphDebugLevel) {
    case 1:
        if (rect->w > D_800568D4 || rect->w + rect->x > D_800568D4 ||
            rect->y > D_800568D6 || rect->y + rect->h > D_800568D6 ||
            rect->w <= 0 || rect->x < 0 || rect->y < 0 || rect->h <= 0) {
            g_GpuPrintf(D_8001915C, name);
            g_GpuPrintf(D_80019168, rect->x, rect->y, rect->w, rect->h);
        }
        break;
    case 2:
        g_GpuPrintf(D_8001917C, name);
        g_GpuPrintf(D_80019168, rect->x, rect->y, rect->w, rect->h);
        break;
    }
}

int ClearImage(RECT *rect, u_char r, u_char g, u_char b) {
    func_8004463C(D_80019180, rect);
    return D_800568C8->unk08(D_800568C8->unk0C, rect, 8, (b << 16) | (g << 8) | r);
}


int ClearImage2(RECT *rect, u_char r, u_char g, u_char b) {
    func_8004463C(D_80019180, rect);
    return D_800568C8->unk08(D_800568C8->unk0C, rect, 8, 0x80000000 | (b << 16) | (g << 8) | r);
}

int LoadImage(RECT *rect, u_long *p) {
    func_8004463C(D_8001918C, rect);
    return D_800568C8->unk08(D_800568C8->unk20, rect, 8, (u_long)p);
}

int StoreImage(RECT *rect, u_long *p) {
    func_8004463C(D_80019198, rect);
    return D_800568C8->unk08(D_800568C8->unk1C, rect, 8, (u_long)p);
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgpu", MoveImage);

/* best: 82.73% -- identical instruction multiset; aspsx 2.56 scheduled the block
   (all three loads hoisted above the two stores, li a2,0x14 between them) and gcc
   emits source order, so no C shape reproduces it.
int MoveImage(RECT *rect, int x, int y) {
    u_long *pPacket;

    func_8004463C(D_800191A4, rect);

    if (rect->w == 0 || rect->h == 0)
        return -1;

    pPacket = &D_80056980;
    D_80056984 = (y << 16) | (x & 0xFFFF);
    *pPacket = *(u_long*)&rect->x;
    D_80056988 = *(u_long*)&rect->w;
    pPacket -= 2;
    return D_800568C8->unk08(D_800568C8->unk18, pPacket, 0x14, 0);
}
*/


u_long* ClearOTag(u_long *ot, int n) {
    if (g_GraphDebugLevel >= 2)
        g_GpuPrintf(D_800191B0, ot, n);

    if (--n != 0) {
        do {
            n--;
            *(u_char*)((u_char*)ot + 3) = 0;
            setaddr(ot, ot + 1);
            ot++;
        } while (n != 0);
    }
    *(u_long*)ot = (u_long)&D_8005698C & 0xFFFFFF;
    return ot;
}

u_long* ClearOTagR(u_long *ot, int n) {
    if (g_GraphDebugLevel >= 2)
        g_GpuPrintf(D_800191C8, ot, n);

    D_800568C8->unk2C(ot, n);
    *(u_long*)ot = (u_long)&D_8005698C & 0xFFFFFF;
    return ot;
}

void DrawPrim(void *p) {
    u_char len = *(u_char*)((u_char*)p + 3);

    D_800568C8->unk3C(0);
    D_800568C8->unk14((u_char*)p + 4, len);
}

void DrawOTag(u_long *p) {
    if (g_GraphDebugLevel >= 2)
        g_GpuPrintf(D_800191E0, p);

    D_800568C8->unk08(D_800568C8->unk18, p, 0, 0);
}

DRAWENV* PutDrawEnv(DRAWENV *env) {
    char *pLevel = &g_GraphDebugLevel;

    if (*pLevel >= 2)
        g_GpuPrintf(D_800191F4, env);

    func_8004574C(&env->dr_env, env);
    env->dr_env.tag |= 0xFFFFFF;
    D_800568C8->unk08(D_800568C8->unk18, &env->dr_env, sizeof(DR_ENV), 0);
    // 0xE past the debug level is g_GpuDrawEnv; the original reaches it from here
    *(DRAWENV*)(pLevel + 0xE) = *env;
    return env;
}

void DrawOTagEnv(u_long *ot, DRAWENV *env) {
    char *pLevel = &g_GraphDebugLevel;

    if (*pLevel >= 2)
        g_GpuPrintf(D_8001920C, ot, env);

    func_8004574C(&env->dr_env, env);
    env->dr_env.tag = (env->dr_env.tag & 0xFF000000) | ((u_long)ot & 0xFFFFFF);
    D_800568C8->unk08(D_800568C8->unk18, &env->dr_env, sizeof(DR_ENV), 0);
    // 0xE past the debug level is g_GpuDrawEnv; the original reaches it from here
    *(DRAWENV*)(pLevel + 0xE) = *env;
}

DRAWENV* GetDrawEnv(DRAWENV* env) {
    memcpy(env, &g_GpuDrawEnv, sizeof(DRAWENV));
    return env;
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgpu", PutDispEnv);

DISPENV* GetDispEnv(DISPENV* env) {
    memcpy(env, &g_GpuDispEnv, sizeof(DISPENV));
    return env;
}
