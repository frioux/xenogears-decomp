#include "common.h"
#include "psyq/libgpu.h"

/* Bit 25 of a TMD primitive id word is ignored when the primitive type is
   classified: a lit ("L") primitive and its unlit form share a body. */
#define GPU_PRIM_ID_MASK 0xFDFFFFFF

extern u_long *g_GpuCurTIMHandle;

extern volatile long g_GpuTimeoutDeadline;
extern volatile long g_GpuTimeoutCount;

extern volatile u_long *g_GpuGp0Port;
extern volatile u_long *g_GpuGp1Port;
extern u_long *g_GpuDma2Madr;

extern volatile u_long *g_GpuDma2Chcr;
extern volatile u_long *g_GpuDmaDpcr;
extern volatile u_long D_800569C4[];
extern u_long D_800569C8;
extern u_long D_800569CC;

extern volatile u_long g_GpuQueueTail;
extern volatile u_long g_GpuQueueHead;
extern u_long g_GpuSavedIntrMask;

extern int func_8004696C(void);
extern int func_8004709C(u_long mode);
extern void func_80047178(void *p, int value, int n);
extern int func_80046F30(void);

extern u_char g_GpuOpcodeShadow[];

/* The GPU command queue: 64 entries of 0x60 bytes, whose three used words are
   carved up as three separate symbols by splat. */
extern void (*g_GpuQueueTable[][0x18])(u_long *p0, u_long *p1);
extern u_long *D_8006BE38[][0x18];
extern u_long *D_8006BE3C[][0x18];
extern u_long D_8006BE40[][0x18];
extern u_long g_GpuQueueIntrMask;
extern u_char g_GraphQue;
extern u_long g_DrawSyncCallbackPending;
extern u_long g_SavedIntrMask;
extern void (*g_DrawSyncCallbackFn)();

extern u_long *g_TmdVertBase;
extern u_long *g_TmdNormBase;
extern u_long *g_TmdPrimBase;
extern u_long g_TmdPrimCount;

extern char D_80019240[]; // "GPU timeout:que=%d,stat=%08x,chcr=%08x,madr=%08x,"
extern char D_80019274[]; // "func=(%08x)(%08x,%08x)\n"
extern char g_StrTimIdFormat[]; // "id  =%08x\n"
extern char g_StrTimModeFormat[]; // "mode=%08x\n"
extern char g_StrTimAddrFormat[]; // "timaddr=%08x\n"
extern char g_StrTmdAnalyzing[]; // "analizing TMD...\n"
extern char g_StrTmdObjectFormat[]; // "\tid=%08X, flags=%d, nobj=%d, objid=%d\n"
extern char g_StrTmdVertFormat[]; // "\tvert=%08X, nvert=%d\n"
extern char g_StrTmdNormFormat[]; // "\tnorm=%08X, nnorm=%d\n"
extern char g_StrTmdPrimFormat[]; // "\tprim=%08X, nprim=%d\n"
/* The primitive type names the mapped-primitive switch prints when the graph
   debug level is 2 ("L" = per-vertex normal index). */
extern char g_StrPrimF3L[]; // "F3L "
extern char D_80019340[]; // "G3L "
extern char D_80019348[]; // "FT3L "
extern char D_80019350[]; // "GT3L "
extern char D_80019358[]; // "F3 "
extern char D_8001935C[]; // "G3 "
extern char D_80019360[]; // "FT3 "
extern char D_80019368[]; // "GT3 "
extern char D_80019370[]; // "F4L "
extern char D_80019378[]; // "G4L "
extern char D_80019380[]; // "FT4L "
extern char D_80019388[]; // "GT4L "
extern char D_80019390[]; // "F4 "
extern char D_80019394[]; // "G4 "
extern char D_80019398[]; // "FT4 "
extern char D_800193A0[]; // "GT4 "
extern char g_StrPrimUnsupportedFmt[]; // "unsupported type (%08x)\n"

extern int Vsync(int mode);

extern int func_80047518(u_long *p, TIM_IMAGE *tim);
extern u_long func_80047638(u_long *tmd, int obj_no, u_long *pPrim, u_long *pVert, u_long *pNorm);

extern int SetIntrMask(int newMask);
extern int DMACallback(int id, void (*func)());

extern void SetDQA(long dqa);
extern void SetDQB(long dqb);

// best 99.10% (183 insns, 732 bytes against retail's 184 / 736): the ONLY residual is the data-copy loop back edge. Retail keeps 'sw a1,0(a0)' before the 'j' and puts a COPY of the loop-invariant 'move v0,s1' in the delay slot (its preheader holds the first copy), while gcc fills the slot with the store and points 'j' at a single top-of-loop move: same semantics, one instruction fewer, so it cannot link. Every other instruction, the tail order and the frame are retail's. The two 'do { } while (0);' barriers are scheduling boundaries, not shippable C (mark it ROUGH when it matches): without them gcc cse's the volatile g_GpuQueueTail load and the third 'lui/lw' pair is missing (92.01%); reading the increment through 'extern volatile u_long g_GpuQueueTailNext[] __asm__("g_GpuQueueTail")' instead measures 95.82 at 183 insns and pays an 'addiu' for the array base. Also tried: % 0x40 vs & 0x3F, raw D_8006BE38/3C/40 arrays, the increment in a local, -O2, -O1 (74.97), cdk (60.49). Delay-slot fill has no source lever (levers.md c-there-source-ordering-lever).
// round 490: re-measured the note live, 99.10% / 183 insns / 732 bytes against retail's 184 / 736, and located the residual in cc1's OWN output rather than reorg: build/src/slus_006.64/psyq/libgpu_c.c.s already has 'move $2,$17' INSIDE the $L14 loop-head block and 'j $L14 / sw $5,0($4)' from cc1 (maspsx is not involved); retail's preheader holds that copy OUTSIDE its loop label and re-materialises it in the j delay slot, so retail is 184. The copy is gcc's signed-div-by-4 sequence (move / bgez / addiu 3 / sra 2) and the sole difference is where cc1 materialises the dividend copy, which no statement order reaches. Also measured byte-identical to the for form (99.10, 15 differing lines): 'i = 0; while (i < n / 4) { ...; i++; }' and 'n / 4 > i' (swapped slt operands). A sixth permuter run (1243 iterations, base=163 best=163, PERMUTER_NONE) found nothing.
/*
int func_8004668C(void (*pFunc)(u_long *p0, u_long *p1), u_long *p, int n, u_long *arg) {
    int i;

    func_80046EFC();

    while (((g_GpuQueueTail + 1) & 0x3F) == g_GpuQueueHead) {
        if (func_80046F30() != 0)
            return -1;
        func_8004696C();
    }

    g_GpuQueueIntrMask = SetIntrMask(0);
    g_DrawSyncCallbackPending = 1;

    if (g_GraphQue == 0 || (g_GpuQueueTail == g_GpuQueueHead && !(*g_GpuDma2Chcr & 0x1000000)
            && g_DrawSyncCallbackFn == 0)) {
        while (!(*g_GpuGp1Port & 0x4000000))
            ;

        pFunc(p, arg);
        D_800569C4[0] = (u_long)pFunc;
        D_800569C8 = (u_long)p;
        D_800569CC = (u_long)arg;
        SetIntrMask(g_GpuQueueIntrMask);
        return 0;
    }

    DMACallback(2, (void (*)())func_8004696C);

    if (n != 0) {
        for (i = 0; i < n / 4; i++)
            D_8006BE40[g_GpuQueueTail][i] = p[i];
        D_8006BE38[g_GpuQueueTail][0] = &D_8006BE40[g_GpuQueueTail][0];
    } else {
        D_8006BE38[g_GpuQueueTail][0] = p;
    }
    D_8006BE3C[g_GpuQueueTail][0] = arg;
    do {
    } while (0);
    g_GpuQueueTable[g_GpuQueueTail][0] = pFunc;
    do {
    } while (0);
    g_GpuQueueTail = (g_GpuQueueTail + 1) & 0x3F;

    SetIntrMask(g_GpuQueueIntrMask);
    func_8004696C();

    return (g_GpuQueueTail - g_GpuQueueHead) & 0x3F;
}*/
INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgpu_c", func_8004668C);





// best 94.44% (186 insns, 744 bytes against retail's 187 / 724): the whole drain loop, the post-call word stores, the head update, the tail guard, the pending-callback arm and the return all match instruction for instruction; the ONE residual is the queued-call's argument gather. Retail reads g_GpuQueueHead three times serially, interleaving each index chain with the BE38 / BE3C table load (which costs it a load-delay nop before the jalr), while gcc hoists all three index loads to the top of the block and fills the slots: same instruction multiset, one instruction fewer, so it cannot link. The three 'do { } while (0);' blocks around the post-call stores are scheduling boundaries, not shippable C (mark ROUGH when it matches): without them the stores are batched and the body measures 81.36% / 180 insns / 720 bytes. A 'do { } while (0);' around the call itself measures byte-identical (94.44). Also tried: the queue as one 0x60-byte GpuQueueEntry struct aliased onto g_GpuQueueTable and read as .func/.p/.arg - 94.39 (49 differing lines), worse, so the split [][0x18] arrays stay.
/*
int func_8004696C(void) {
    u_long *pQue;
    u_long *pPending;

    if (*g_GpuDma2Chcr & 0x1000000)
        return 1;

    g_SavedIntrMask = SetIntrMask(0);

    if (g_GpuQueueTail != g_GpuQueueHead && !(*g_GpuDma2Chcr & 0x1000000)) {
        pQue = &D_800569C4;
        do {
            if (((g_GpuQueueHead + 1) & 0x3F) == g_GpuQueueTail) {
                if (!g_DrawSyncCallbackFn)
                    DMACallback(2, 0);
            }

            while (!(*g_GpuGp1Port & 0x4000000))
                ;

            g_GpuQueueTable[g_GpuQueueHead][0](D_8006BE38[g_GpuQueueHead][0],
                    D_8006BE3C[g_GpuQueueHead][0]);
            do {
                *pQue = (u_long)g_GpuQueueTable[g_GpuQueueHead][0];
            } while (0);
            do {
                D_800569C8 = (u_long)D_8006BE38[g_GpuQueueHead][0];
            } while (0);
            do {
                D_800569CC = (u_long)D_8006BE3C[g_GpuQueueHead][0];
            } while (0);
            g_GpuQueueHead = (g_GpuQueueHead + 1) & 0x3F;
        } while (g_GpuQueueTail != g_GpuQueueHead && !(*g_GpuDma2Chcr & 0x1000000));
    }

    SetIntrMask(g_SavedIntrMask);

    if (g_GpuQueueTail == g_GpuQueueHead && !(*g_GpuDma2Chcr & 0x1000000)) {
        pPending = &g_DrawSyncCallbackPending;
        if (*pPending && g_DrawSyncCallbackFn) {
            *pPending = 0;
            g_DrawSyncCallbackFn();
        }
    }

    return (g_GpuQueueTail - g_GpuQueueHead) & 0x3F;
}*/
INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgpu_c", func_8004696C);



int func_80046C58(u_long mode) {
    u_char v;

    g_GpuSavedIntrMask = SetIntrMask(0);
    g_GpuQueueHead = 0;
    g_GpuQueueTail = g_GpuQueueHead;

    v = mode & 7;
    switch (v) {
    case 5:
    case 0:
        *g_GpuDma2Chcr = 0x401;
        *g_GpuDmaDpcr |= 0x800;
        *g_GpuGp1Port = 0;
        func_80047178(g_GpuOpcodeShadow, 0, 0x100);
        func_80047178(g_GpuQueueTable, 0, 0x1800);
        break;
    case 1:
    case 3:
        *g_GpuDma2Chcr = 0x401;
        *g_GpuDmaDpcr |= 0x800;
        *g_GpuGp1Port = 0x2000000;
        *g_GpuGp1Port = 0x1000000;
        break;
    }

    SetIntrMask(g_GpuSavedIntrMask);

    if ((mode & 7) == 0)
        return func_8004709C(mode);

    return 0;
}


int func_80046DB4(int mode) {
    u_long n;

    if (mode == 0) {
        func_80046EFC();

        while (g_GpuQueueTail != g_GpuQueueHead) {
            func_8004696C();
            if (func_80046F30() != 0)
                return -1;
        }

        while ((*g_GpuDma2Chcr & 0x1000000) || !(*g_GpuGp1Port & 0x4000000)) {
            if (func_80046F30() != 0)
                return -1;
        }

        return 0;
    }

    n = (g_GpuQueueTail - g_GpuQueueHead) & 0x3F;
    if (n != 0)
        func_8004696C();

    if ((*g_GpuDma2Chcr & 0x1000000) || !(*g_GpuGp1Port & 0x4000000)) {
        if (n == 0)
            return 1;
        return n;
    }

    return n;
}



void func_80046EFC(void) {
    g_GpuTimeoutDeadline = Vsync(-1) + 0xF0;
    g_GpuTimeoutCount = 0;
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgpu_c", func_80046F30);

/* best 99.51% (364 bytes = retail's exact size, 91 insns, instruction order
   identical; only two caller-saved registers differ). The GPU timeout panic
   handler: Vsync(-1) against a deadline, then a queue dump and a queue reset.
   Load-bearing spellings: the discarded status read is a bare
   "(void)*g_GpuGp1Port;" statement before the first printf; the queue word
   D_800569C4 must be declared volatile AND as an array ("extern volatile u_long
   D_800569C4[];", read as D_800569C4[0]) so its address materialises with
   lui+addiu; and the queue delta must be staged through a local in TWO
   statements - "(g_GpuQueueTail - g_GpuQueueHead) & 0x3F" inline measures 94.49
   and a one-statement stage with the head in a local 94.38.
   Residual (local-alloc ordering tie, read off -dl/-dg on the unit's own cc1):
   the g_GpuGp1Port pointer load is qty 81, "used 3 times across 22 insns", and
   the dump's disposition line puts it in $6/$a2 where retail has $3/$v1; the
   g_GpuQueueHead value is qty 83, "used 2 times across 3 insns", and lands in
   $3/$v1 where retail has $8/$t0. Both ref counts are pinned by the instruction
   stream - the pointer's three refs are its address load plus the two volatile
   reads, the head's two are its load plus the subu, and neither can change
   without adding or removing an instruction - so no spelling moves them; every
   other instruction, the frame and the saved set are exact.
   Tried: the dead read stored to an unused local (99.51, identical), the
   inline and one-local delta forms (94.49 / 94.38), gcc-2.7.2-cdk on this body
   (65.03, and the unit's preset is 2.7.2 for all of its matched functions),
   four earlier permuter runs (350 -> 45, 3801 iterations, round 392 1335
   iterations base=45) and a fifth (1790 iterations, base=45 best=45,
   PERMUTER_NONE), the pointer in a local (identical), an explicit volatile cast,
   g_GpuGp1Port[0] vs a deref at each of the two reads, arg3 staged through a
   local (88.41%), and both declaration orders of the two locals. Round 392 also
   proved the array-extern lever dead here: "extern volatile u_long
   *g_GpuGp1Port[];" makes the matched func_80046DB4 and func_8004709C fail to
   compile ("invalid operands to binary &"), so the shared pointer global must
   stay a scalar.
   Round 419 closed the tie out as an allocator artefact rather than a spelling
   question: the -dl dump of this body shows the head at 2 refs over 3 insns and
   the pointer at 3 refs over 22, so if local-alloc sorted by refs/live the head
   would win the first free caller-saved register ($v1) - but moving the head's
   def ABOVE the dead read (the one spelling that changes that order) measures
   94.66% with 30 differing lines and the pointer STILL lands in $a2, so neither
   priority nor qty order is what puts it there. Also measured identical at
   99.51/14 lines: "u_long queued", the head as its own two-statement local, a
   separate "volatile u_long *statusPort" local for the dead read (gcc CSEs it
   back to the one pointer load), -O2 (99.51) and gcc-2.6.0 (99.51) on this body;
   a sixth permuter run scored base=45 best=45 (1420 iterations, PERMUTER_NONE).
   The body below is the parked attempt.
int func_80046F30(void) {
    u_long dma2ChcrWord;
    int queued;

    if (g_GpuTimeoutDeadline < Vsync(-1) || g_GpuTimeoutCount++ > 0xF0000) {
        (void)*g_GpuGp1Port;
        queued = g_GpuQueueTail;
        queued = queued - g_GpuQueueHead;
        printf(D_80019240, queued & 0x3F, *g_GpuGp1Port,
               *g_GpuDma2Chcr, *g_GpuDma2Madr);
        dma2ChcrWord = D_800569C8;
        printf(D_80019274, D_800569C4[0], dma2ChcrWord, D_800569CC);
        g_GpuSavedIntrMask = SetIntrMask(0);
        g_GpuQueueHead = 0;
        g_GpuQueueTail = g_GpuQueueHead;
        *g_GpuDma2Chcr = 0x401;
        *g_GpuDmaDpcr |= 0x800;
        *g_GpuGp1Port = 0x2000000;
        *g_GpuGp1Port = 0x1000000;
        SetIntrMask(g_GpuSavedIntrMask);
        return -1;
    }

    return 0;
}
*/

int func_8004709C(u_long mode) {
    *g_GpuGp1Port = 0x10000007;

    if ((*g_GpuGp0Port & 0xFFFFFF) != 2) {
        *g_GpuGp0Port = (*g_GpuGp1Port & 0x3FFF) | 0xE1001000;
        (void)*g_GpuGp0Port; // the status port is read back to flush the write
        if (!(*g_GpuGp1Port & 0x1000))
            return 0;
        if (!(mode & 8))
            return 1;
        *g_GpuGp1Port = 0x20000504;
        return 2;
    }

    if (!(mode & 8))
        return 3;

    *g_GpuGp1Port = 0x09000001;
    return 4;
}

void func_80047178(void *p, int value, int n) {
    u_char *pByte = p;

    while (n-- != 0)
        *pByte++ = value;
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgpu_c", GPU_cw);

int OpenTIM(u_long *addr) {
    g_GpuCurTIMHandle = addr;
    return 0;
}

TIM_IMAGE *ReadTIM(TIM_IMAGE *timimg) {
    int size;

    if ((size = func_80047518(g_GpuCurTIMHandle, timimg)) == -1)
        return 0;

    g_GpuCurTIMHandle += size;
    return timimg;
}

int OpenTMD(u_long *tmd, int obj_no) {
    return g_TmdPrimCount = func_80047638(tmd, obj_no, &g_TmdPrimBase, &g_TmdVertBase, &g_TmdNormBase);
}

/* One 8-byte vertex/normal entry of the TMD data block: x, y, z and a pad. */
typedef struct {
    /* 0x0 */ u_short x;
    /* 0x2 */ u_short y;
    /* 0x4 */ u_short z;
    /* 0x6 */ u_short pad6;
} TmdVector; // Size: 0x8

/* One RGB triplet of the primitive header, padded to a word. */
typedef struct {
    /* 0x0 */ u8 r;
    /* 0x1 */ u8 g;
    /* 0x2 */ u8 b;
    /* 0x3 */ u8 pad3;
} TmdColor; // Size: 0x4

/* A mapped TMD primitive object (0x78 bytes): func_800477D0 fills the header
   in 0x0-0x1F from the primitive packet, and the vertices/normals named by
   vertIndex/normIndex are copied out of the data block into verts/norms here. */
typedef struct {
    /* 0x00 */ u_long tag;
    /* 0x04 */ TmdColor colors[4];
    /* 0x14 */ u_short unk14;
    /* 0x16 */ u_short unk16;
    /* 0x18 */ u8 unk18[8];
    /* 0x20 */ TmdVector verts[4];
    /* 0x40 */ TmdVector norms[4];
    /* 0x60 */ u_long vertBase;
    /* 0x64 */ u_long normBase;
    /* 0x68 */ u_short vertIndex[4];
    /* 0x70 */ u_short normIndex[4];
} TmdPrimObject; // Size: 0x78

/* A TMD primitive packet. The parameters after the id word depend on the
   primitive type - a colour, a colour per vertex, a texel u/v pair or an
   index pair - so the parameter block is read as bytes or as halfwords. */
typedef union {
    /* 0x00 */ u_long tag;
    /* 0x00 */ u_char bytes[0x2C];
    /* 0x00 */ u_short shorts[0x16];
} TmdPrimPacket;

extern int func_800477D0(TmdPrimPacket *pPrim, TmdPrimObject *pObj);

TmdPrimObject *func_8004726C(TmdPrimObject *pObj) {
    TmdVector *pVert;
    TmdVector *pNorm;
    int size;

    if ((size = func_800477D0((TmdPrimPacket *)g_TmdPrimBase, pObj)) < 0)
        return 0;

    pVert = (TmdVector *)g_TmdVertBase;
    pNorm = (TmdVector *)g_TmdNormBase;
    pObj->vertBase = (u_long)pVert;
    pObj->normBase = (u_long)pNorm;

    pObj->norms[0].x = pNorm[pObj->normIndex[0]].x;
    pObj->norms[0].y = pNorm[pObj->normIndex[0]].y;
    pObj->norms[0].z = pNorm[pObj->normIndex[0]].z;
    pObj->norms[1].x = pNorm[pObj->normIndex[1]].x;
    pObj->norms[1].y = pNorm[pObj->normIndex[1]].y;
    pObj->norms[1].z = pNorm[pObj->normIndex[1]].z;
    pObj->norms[2].x = pNorm[pObj->normIndex[2]].x;
    pObj->norms[2].y = pNorm[pObj->normIndex[2]].y;
    pObj->norms[2].z = pNorm[pObj->normIndex[2]].z;

    // advancing the prim pointer between the two copies is what retail does
    g_TmdPrimBase = (u_long *)((u_char *)g_TmdPrimBase + size);

    pObj->norms[3].x = pNorm[pObj->normIndex[3]].x;
    pObj->norms[3].y = pNorm[pObj->normIndex[3]].y;
    pObj->norms[3].z = pNorm[pObj->normIndex[3]].z;

    pObj->verts[0].x = pVert[pObj->vertIndex[0]].x;
    pObj->verts[0].y = pVert[pObj->vertIndex[0]].y;
    pObj->verts[0].z = pVert[pObj->vertIndex[0]].z;
    pObj->verts[1].x = pVert[pObj->vertIndex[1]].x;
    pObj->verts[1].y = pVert[pObj->vertIndex[1]].y;
    pObj->verts[1].z = pVert[pObj->vertIndex[1]].z;
    pObj->verts[2].x = pVert[pObj->vertIndex[2]].x;
    pObj->verts[2].y = pVert[pObj->vertIndex[2]].y;
    pObj->verts[2].z = pVert[pObj->vertIndex[2]].z;
    pObj->verts[3].x = pVert[pObj->vertIndex[3]].x;
    pObj->verts[3].y = pVert[pObj->vertIndex[3]].y;
    pObj->verts[3].z = pVert[pObj->vertIndex[3]].z;

    return pObj;
}

int func_80047518(u_long *p, TIM_IMAGE *tim) {
    int clutSize;
    int size;

    if (*p++ != 0x10)
        return -1;

    tim->mode = *p++;

    if (GetGraphDebug() == 2)
        printf(g_StrTimIdFormat, 0x10);
    if (GetGraphDebug() == 2)
        printf(g_StrTimModeFormat, tim->mode);
    if (GetGraphDebug() == 2)
        printf(g_StrTimAddrFormat, p);

    if (tim->mode & 8) {
        clutSize = *p >> 2;
        tim->crect = (RECT *)(p + 1);
        tim->caddr = (u_long *)(p + 3);
        p += clutSize;
    } else {
        tim->crect = 0;
        tim->caddr = 0;
        clutSize = 0;
    }

    size = (*p >> 2) + 2;
    tim->prect = (RECT *)(p + 1);
    tim->paddr = (u_long *)(p + 3);
    return clutSize + size;
}

/* Object table entry of a TMD model (struct TMD_STRUCT in libgs.h). */
typedef struct {
    /* 0x0 */ u_long vertop;
    /* 0x4 */ long vern;
    /* 0x8 */ u_long nortop;
    /* 0xC */ long norn;
    /* 0x10 */ u_long primtop;
    /* 0x14 */ long primn;
    /* 0x18 */ u_long scale;
} TmdObj; // Size: 0x1C

u_long func_80047638(u_long *tmd, int obj_no, u_long *pPrim, u_long *pVert, u_long *pNorm) {
    TmdObj *pObj = (TmdObj *)((u_char *)tmd + 0xC);

    if (GetGraphDebug() == 2)
        printf(g_StrTmdAnalyzing);
    if (GetGraphDebug() == 2)
        printf(g_StrTmdObjectFormat, tmd[0], tmd[1], tmd[2], obj_no);
    if (GetGraphDebug() == 2)
        printf(g_StrTmdVertFormat, pObj[obj_no].vertop, pObj[obj_no].vern);
    if (GetGraphDebug() == 2)
        printf(g_StrTmdNormFormat, pObj[obj_no].nortop, pObj[obj_no].norn);
    if (GetGraphDebug() == 2)
        printf(g_StrTmdPrimFormat, pObj[obj_no].primtop, pObj[obj_no].primn);

    *pVert = (u_long)((u_char *)pObj + pObj[obj_no].vertop);
    *pNorm = (u_long)((u_char *)pObj + pObj[obj_no].nortop);
    *pPrim = (u_long)((u_char *)pObj + pObj[obj_no].primtop);

    return pObj[obj_no].primn;
}

/* Map one TMD primitive packet into pObj: the packet id word names the
   primitive type, each arm copies that type's parameters into the object
   header and returns the packet's size in bytes. */
int func_800477D0(TmdPrimPacket *pPrim, TmdPrimObject *pObj) {
    u_long type;

    memset(pObj, 0, sizeof(*pObj));

    type = pPrim->tag;
    pObj->tag = type;

    switch (type & GPU_PRIM_ID_MASK) {
    case 0x20000304:
    case 0x20020304:
        if (GetGraphDebug() == 2)
            printf(g_StrPrimF3L); // "F3L "
        pObj->colors[0].r = pPrim->bytes[0x4];
        pObj->colors[0].g = pPrim->bytes[0x5];
        pObj->colors[0].b = pPrim->bytes[0x6];
        pObj->colors[1].r = pPrim->bytes[0x4];
        pObj->colors[1].g = pPrim->bytes[0x5];
        pObj->colors[1].b = pPrim->bytes[0x6];
        pObj->colors[2].r = pPrim->bytes[0x4];
        pObj->colors[2].g = pPrim->bytes[0x5];
        pObj->colors[2].b = pPrim->bytes[0x6];
        pObj->vertIndex[0] = pPrim->shorts[0x5];
        pObj->vertIndex[1] = pPrim->shorts[0x6];
        pObj->vertIndex[2] = pPrim->shorts[0x7];
        pObj->normIndex[0] = pPrim->shorts[0x4];
        pObj->normIndex[1] = pPrim->shorts[0x4];
        pObj->normIndex[2] = pPrim->shorts[0x4];
        return 16;
    case 0x30000406:
    case 0x30020406:
        if (GetGraphDebug() == 2)
            printf(D_80019340); // "G3L "
        pObj->colors[0].r = pPrim->bytes[0x4];
        pObj->colors[0].g = pPrim->bytes[0x5];
        pObj->colors[0].b = pPrim->bytes[0x6];
        pObj->colors[1].r = pPrim->bytes[0x4];
        pObj->colors[1].g = pPrim->bytes[0x5];
        pObj->colors[1].b = pPrim->bytes[0x6];
        pObj->colors[2].r = pPrim->bytes[0x4];
        pObj->colors[2].g = pPrim->bytes[0x5];
        pObj->colors[2].b = pPrim->bytes[0x6];
        pObj->vertIndex[0] = pPrim->shorts[0x5];
        pObj->vertIndex[1] = pPrim->shorts[0x7];
        pObj->vertIndex[2] = pPrim->shorts[0x9];
        pObj->normIndex[0] = pPrim->shorts[0x4];
        pObj->normIndex[1] = pPrim->shorts[0x6];
        pObj->normIndex[2] = pPrim->shorts[0x8];
        return 20;
    case 0x24000507:
    case 0x24020507:
        if (GetGraphDebug() == 2)
            printf(D_80019348); // "FT3L "
        pObj->unk14 = pPrim->shorts[0x5];
        pObj->unk16 = pPrim->shorts[0x3];
        pObj->unk18[0] = pPrim->bytes[0x4];
        pObj->unk18[1] = pPrim->bytes[0x5];
        pObj->unk18[2] = pPrim->bytes[0x8];
        pObj->unk18[3] = pPrim->bytes[0x9];
        pObj->unk18[4] = pPrim->bytes[0xC];
        pObj->unk18[5] = pPrim->bytes[0xD];
        pObj->vertIndex[0] = pPrim->shorts[0x9];
        pObj->vertIndex[1] = pPrim->shorts[0xA];
        pObj->vertIndex[2] = pPrim->shorts[0xB];
        pObj->normIndex[0] = pPrim->shorts[0x8];
        pObj->normIndex[1] = pPrim->shorts[0x8];
        pObj->normIndex[2] = pPrim->shorts[0x8];
        return 24;
    case 0x34000609:
    case 0x34020609:
        if (GetGraphDebug() == 2)
            printf(D_80019350); // "GT3L "
        pObj->unk14 = pPrim->shorts[0x5];
        pObj->unk16 = pPrim->shorts[0x3];
        pObj->unk18[0] = pPrim->bytes[0x4];
        pObj->unk18[1] = pPrim->bytes[0x5];
        pObj->unk18[2] = pPrim->bytes[0x8];
        pObj->unk18[3] = pPrim->bytes[0x9];
        pObj->unk18[4] = pPrim->bytes[0xC];
        pObj->unk18[5] = pPrim->bytes[0xD];
        pObj->vertIndex[0] = pPrim->shorts[0x9];
        pObj->vertIndex[1] = pPrim->shorts[0xB];
        pObj->vertIndex[2] = pPrim->shorts[0xD];
        pObj->normIndex[0] = pPrim->shorts[0x8];
        pObj->normIndex[1] = pPrim->shorts[0xA];
        pObj->normIndex[2] = pPrim->shorts[0xC];
        return 28;
    case 0x21010304:
    case 0x21030304:
        if (GetGraphDebug() == 2)
            printf(D_80019358); // "F3 "
        pObj->colors[0].r = pPrim->bytes[0x4];
        pObj->colors[0].g = pPrim->bytes[0x5];
        pObj->colors[0].b = pPrim->bytes[0x6];
        pObj->colors[1].r = pPrim->bytes[0x4];
        pObj->colors[1].g = pPrim->bytes[0x5];
        pObj->colors[1].b = pPrim->bytes[0x6];
        pObj->colors[2].r = pPrim->bytes[0x4];
        pObj->colors[2].g = pPrim->bytes[0x5];
        pObj->colors[2].b = pPrim->bytes[0x6];
        pObj->vertIndex[0] = pPrim->shorts[0x4];
        pObj->vertIndex[1] = pPrim->shorts[0x5];
        pObj->vertIndex[2] = pPrim->shorts[0x6];
        return 16;
    case 0x31010506:
    case 0x31030506:
        if (GetGraphDebug() == 2)
            printf(D_8001935C); // "G3 "
        pObj->colors[0].r = pPrim->bytes[0x4];
        pObj->colors[0].g = pPrim->bytes[0x5];
        pObj->colors[0].b = pPrim->bytes[0x6];
        pObj->colors[1].r = pPrim->bytes[0x8];
        pObj->colors[1].g = pPrim->bytes[0x9];
        pObj->colors[1].b = pPrim->bytes[0xA];
        pObj->colors[2].r = pPrim->bytes[0xC];
        pObj->colors[2].g = pPrim->bytes[0xD];
        pObj->colors[2].b = pPrim->bytes[0xE];
        pObj->vertIndex[0] = pPrim->shorts[0x8];
        pObj->vertIndex[1] = pPrim->shorts[0x9];
        pObj->vertIndex[2] = pPrim->shorts[0xA];
        return 24;
    case 0x25010607:
    case 0x25030607:
        if (GetGraphDebug() == 2)
            printf(D_80019360); // "FT3 "
        pObj->unk14 = pPrim->shorts[0x5];
        pObj->unk16 = pPrim->shorts[0x3];
        pObj->unk18[0] = pPrim->bytes[0x4];
        pObj->unk18[1] = pPrim->bytes[0x5];
        pObj->unk18[2] = pPrim->bytes[0x8];
        pObj->unk18[3] = pPrim->bytes[0x9];
        pObj->unk18[4] = pPrim->bytes[0xC];
        pObj->unk18[5] = pPrim->bytes[0xD];
        pObj->colors[0].r = pPrim->bytes[0x10];
        pObj->colors[0].g = pPrim->bytes[0x11];
        pObj->colors[0].b = pPrim->bytes[0x12];
        pObj->colors[1].r = pPrim->bytes[0x10];
        pObj->colors[1].g = pPrim->bytes[0x11];
        pObj->colors[1].b = pPrim->bytes[0x12];
        pObj->colors[2].r = pPrim->bytes[0x10];
        pObj->colors[2].g = pPrim->bytes[0x11];
        pObj->colors[2].b = pPrim->bytes[0x12];
        pObj->vertIndex[0] = pPrim->shorts[0xA];
        pObj->vertIndex[1] = pPrim->shorts[0xB];
        pObj->vertIndex[2] = pPrim->shorts[0xC];
        return 28;
    case 0x35010809:
    case 0x35030809:
        if (GetGraphDebug() == 2)
            printf(D_80019368); // "GT3 "
        pObj->unk14 = pPrim->shorts[0x5];
        pObj->unk16 = pPrim->shorts[0x3];
        pObj->unk18[0] = pPrim->bytes[0x4];
        pObj->unk18[1] = pPrim->bytes[0x5];
        pObj->unk18[2] = pPrim->bytes[0x8];
        pObj->unk18[3] = pPrim->bytes[0x9];
        pObj->unk18[4] = pPrim->bytes[0xC];
        pObj->unk18[5] = pPrim->bytes[0xD];
        pObj->vertIndex[0] = pPrim->shorts[0xE];
        pObj->vertIndex[1] = pPrim->shorts[0xF];
        pObj->vertIndex[2] = pPrim->shorts[0x10];
        pObj->colors[0].r = pPrim->bytes[0x10];
        pObj->colors[0].g = pPrim->bytes[0x11];
        pObj->colors[0].b = pPrim->bytes[0x12];
        pObj->colors[1].r = pPrim->bytes[0x14];
        pObj->colors[1].g = pPrim->bytes[0x15];
        pObj->colors[1].b = pPrim->bytes[0x16];
        pObj->colors[2].r = pPrim->bytes[0x18];
        pObj->colors[2].g = pPrim->bytes[0x19];
        pObj->colors[2].b = pPrim->bytes[0x1A];
        return 36;
    case 0x28000405:
    case 0x28020405:
        if (GetGraphDebug() == 2)
            printf(D_80019370); // "F4L "
        pObj->colors[0].r = pPrim->bytes[0x4];
        pObj->colors[0].g = pPrim->bytes[0x5];
        pObj->colors[0].b = pPrim->bytes[0x6];
        pObj->colors[1].r = pPrim->bytes[0x4];
        pObj->colors[1].g = pPrim->bytes[0x5];
        pObj->colors[1].b = pPrim->bytes[0x6];
        pObj->colors[2].r = pPrim->bytes[0x4];
        pObj->colors[2].g = pPrim->bytes[0x5];
        pObj->colors[2].b = pPrim->bytes[0x6];
        pObj->colors[3].r = pPrim->bytes[0x4];
        pObj->colors[3].g = pPrim->bytes[0x5];
        pObj->colors[3].b = pPrim->bytes[0x6];
        pObj->vertIndex[0] = pPrim->shorts[0x5];
        pObj->vertIndex[1] = pPrim->shorts[0x6];
        pObj->vertIndex[2] = pPrim->shorts[0x7];
        pObj->vertIndex[3] = pPrim->shorts[0x8];
        pObj->normIndex[0] = pPrim->shorts[0x4];
        pObj->normIndex[1] = pPrim->shorts[0x4];
        pObj->normIndex[2] = pPrim->shorts[0x4];
        pObj->normIndex[3] = pPrim->shorts[0x4];
        return 20;
    case 0x38000508:
    case 0x38020508:
        if (GetGraphDebug() == 2)
            printf(D_80019378); // "G4L "
        pObj->colors[0].r = pPrim->bytes[0x4];
        pObj->colors[0].g = pPrim->bytes[0x5];
        pObj->colors[0].b = pPrim->bytes[0x6];
        pObj->colors[1].r = pPrim->bytes[0x4];
        pObj->colors[1].g = pPrim->bytes[0x5];
        pObj->colors[1].b = pPrim->bytes[0x6];
        pObj->colors[2].r = pPrim->bytes[0x4];
        pObj->colors[2].g = pPrim->bytes[0x5];
        pObj->colors[2].b = pPrim->bytes[0x6];
        pObj->colors[3].r = pPrim->bytes[0x4];
        pObj->colors[3].g = pPrim->bytes[0x5];
        pObj->colors[3].b = pPrim->bytes[0x6];
        pObj->vertIndex[0] = pPrim->shorts[0x5];
        pObj->vertIndex[1] = pPrim->shorts[0x7];
        pObj->vertIndex[2] = pPrim->shorts[0x9];
        pObj->vertIndex[3] = pPrim->shorts[0xB];
        pObj->normIndex[0] = pPrim->shorts[0x4];
        pObj->normIndex[1] = pPrim->shorts[0x6];
        pObj->normIndex[2] = pPrim->shorts[0x8];
        pObj->normIndex[3] = pPrim->shorts[0xA];
        return 24;
    case 0x2C000709:
    case 0x2C020709:
        if (GetGraphDebug() == 2)
            printf(D_80019380); // "FT4L "
        pObj->unk14 = pPrim->shorts[0x5];
        pObj->unk16 = pPrim->shorts[0x3];
        pObj->unk18[0] = pPrim->bytes[0x4];
        pObj->unk18[1] = pPrim->bytes[0x5];
        pObj->unk18[2] = pPrim->bytes[0x8];
        pObj->unk18[3] = pPrim->bytes[0x9];
        pObj->unk18[4] = pPrim->bytes[0xC];
        pObj->unk18[5] = pPrim->bytes[0xD];
        pObj->unk18[6] = pPrim->bytes[0x10];
        pObj->unk18[7] = pPrim->bytes[0x11];
        pObj->vertIndex[0] = pPrim->shorts[0xB];
        pObj->vertIndex[1] = pPrim->shorts[0xC];
        pObj->vertIndex[2] = pPrim->shorts[0xD];
        pObj->vertIndex[3] = pPrim->shorts[0xE];
        pObj->normIndex[0] = pPrim->shorts[0xA];
        pObj->normIndex[1] = pPrim->shorts[0xA];
        pObj->normIndex[2] = pPrim->shorts[0xA];
        pObj->normIndex[3] = pPrim->shorts[0xA];
        return 32;
    case 0x3C00080C:
    case 0x3C02080C:
        if (GetGraphDebug() == 2)
            printf(D_80019388); // "GT4L "
        pObj->unk14 = pPrim->shorts[0x5];
        pObj->unk16 = pPrim->shorts[0x3];
        pObj->unk18[0] = pPrim->bytes[0x4];
        pObj->unk18[1] = pPrim->bytes[0x5];
        pObj->unk18[2] = pPrim->bytes[0x8];
        pObj->unk18[3] = pPrim->bytes[0x9];
        pObj->unk18[4] = pPrim->bytes[0xC];
        pObj->unk18[5] = pPrim->bytes[0xD];
        pObj->unk18[6] = pPrim->bytes[0x10];
        pObj->unk18[7] = pPrim->bytes[0x11];
        pObj->vertIndex[0] = pPrim->shorts[0xB];
        pObj->vertIndex[1] = pPrim->shorts[0xD];
        pObj->vertIndex[2] = pPrim->shorts[0xF];
        pObj->vertIndex[3] = pPrim->shorts[0x11];
        pObj->normIndex[0] = pPrim->shorts[0xA];
        pObj->normIndex[1] = pPrim->shorts[0xC];
        pObj->normIndex[2] = pPrim->shorts[0xE];
        pObj->normIndex[3] = pPrim->shorts[0x10];
        return 36;
    case 0x29010305:
    case 0x29030305:
        if (GetGraphDebug() == 2)
            printf(D_80019390); // "F4 "
        pObj->colors[0].r = pPrim->bytes[0x4];
        pObj->colors[0].g = pPrim->bytes[0x5];
        pObj->colors[0].b = pPrim->bytes[0x6];
        pObj->colors[1].r = pPrim->bytes[0x4];
        pObj->colors[1].g = pPrim->bytes[0x5];
        pObj->colors[1].b = pPrim->bytes[0x6];
        pObj->colors[2].r = pPrim->bytes[0x4];
        pObj->colors[2].g = pPrim->bytes[0x5];
        pObj->colors[2].b = pPrim->bytes[0x6];
        pObj->colors[3].r = pPrim->bytes[0x4];
        pObj->colors[3].g = pPrim->bytes[0x5];
        pObj->colors[3].b = pPrim->bytes[0x6];
        pObj->vertIndex[0] = pPrim->shorts[0x4];
        pObj->vertIndex[1] = pPrim->shorts[0x5];
        pObj->vertIndex[2] = pPrim->shorts[0x6];
        pObj->vertIndex[3] = pPrim->shorts[0x7];
        return 16;
    case 0x39010608:
    case 0x39030608:
        if (GetGraphDebug() == 2)
            printf(D_80019394); // "G4 "
        pObj->colors[0].r = pPrim->bytes[0x4];
        pObj->colors[0].g = pPrim->bytes[0x5];
        pObj->colors[0].b = pPrim->bytes[0x6];
        pObj->colors[1].r = pPrim->bytes[0x8];
        pObj->colors[1].g = pPrim->bytes[0x9];
        pObj->colors[1].b = pPrim->bytes[0xA];
        pObj->colors[2].r = pPrim->bytes[0xC];
        pObj->colors[2].g = pPrim->bytes[0xD];
        pObj->colors[2].b = pPrim->bytes[0xE];
        pObj->colors[3].r = pPrim->bytes[0x10];
        pObj->colors[3].g = pPrim->bytes[0x11];
        pObj->colors[3].b = pPrim->bytes[0x12];
        pObj->vertIndex[0] = pPrim->shorts[0xA];
        pObj->vertIndex[1] = pPrim->shorts[0xB];
        pObj->vertIndex[2] = pPrim->shorts[0xC];
        pObj->vertIndex[3] = pPrim->shorts[0xD];
        return 28;
    case 0x2D010709:
    case 0x2D030709:
        if (GetGraphDebug() == 2)
            printf(D_80019398); // "FT4 "
        pObj->unk14 = pPrim->shorts[0x5];
        pObj->unk16 = pPrim->shorts[0x3];
        pObj->unk18[0] = pPrim->bytes[0x4];
        pObj->unk18[1] = pPrim->bytes[0x5];
        pObj->unk18[2] = pPrim->bytes[0x8];
        pObj->unk18[3] = pPrim->bytes[0x9];
        pObj->unk18[4] = pPrim->bytes[0xC];
        pObj->unk18[5] = pPrim->bytes[0xD];
        pObj->unk18[6] = pPrim->bytes[0x10];
        pObj->unk18[7] = pPrim->bytes[0x11];
        pObj->colors[0].r = pPrim->bytes[0x14];
        pObj->colors[0].g = pPrim->bytes[0x15];
        pObj->colors[0].b = pPrim->bytes[0x16];
        pObj->colors[1].r = pPrim->bytes[0x14];
        pObj->colors[1].g = pPrim->bytes[0x15];
        pObj->colors[1].b = pPrim->bytes[0x16];
        pObj->colors[2].r = pPrim->bytes[0x14];
        pObj->colors[2].g = pPrim->bytes[0x15];
        pObj->colors[2].b = pPrim->bytes[0x16];
        pObj->colors[3].r = pPrim->bytes[0x14];
        pObj->colors[3].g = pPrim->bytes[0x15];
        pObj->colors[3].b = pPrim->bytes[0x16];
        pObj->vertIndex[0] = pPrim->shorts[0xC];
        pObj->vertIndex[1] = pPrim->shorts[0xD];
        pObj->vertIndex[2] = pPrim->shorts[0xE];
        pObj->vertIndex[3] = pPrim->shorts[0xF];
        return 32;
    case 0x3D010A0C:
    case 0x3D030A0C:
        if (GetGraphDebug() == 2)
            printf(D_800193A0); // "GT4 "
        pObj->unk14 = pPrim->shorts[0x5];
        pObj->unk16 = pPrim->shorts[0x3];
        pObj->unk18[0] = pPrim->bytes[0x4];
        pObj->unk18[1] = pPrim->bytes[0x5];
        pObj->unk18[2] = pPrim->bytes[0x8];
        pObj->unk18[3] = pPrim->bytes[0x9];
        pObj->unk18[4] = pPrim->bytes[0xC];
        pObj->unk18[5] = pPrim->bytes[0xD];
        pObj->unk18[6] = pPrim->bytes[0x10];
        pObj->unk18[7] = pPrim->bytes[0x11];
        pObj->vertIndex[0] = pPrim->shorts[0x12];
        pObj->vertIndex[1] = pPrim->shorts[0x13];
        pObj->vertIndex[2] = pPrim->shorts[0x14];
        pObj->vertIndex[3] = pPrim->shorts[0x15];
        pObj->colors[0].r = pPrim->bytes[0x14];
        pObj->colors[0].g = pPrim->bytes[0x15];
        pObj->colors[0].b = pPrim->bytes[0x16];
        pObj->colors[1].r = pPrim->bytes[0x18];
        pObj->colors[1].g = pPrim->bytes[0x19];
        pObj->colors[1].b = pPrim->bytes[0x1A];
        pObj->colors[2].r = pPrim->bytes[0x1C];
        pObj->colors[2].g = pPrim->bytes[0x1D];
        pObj->colors[2].b = pPrim->bytes[0x1E];
        pObj->colors[3].r = pPrim->bytes[0x20];
        pObj->colors[3].g = pPrim->bytes[0x21];
        pObj->colors[3].b = pPrim->bytes[0x22];
        return 44;
    default:
        printf(g_StrPrimUnsupportedFmt, pObj->tag & GPU_PRIM_ID_MASK);
        return -1;
    }
}

void SetFogNearFar(long a, long b, long h) {
    long d;
    long n;
    long f;

    d = b - a;
    if (d < 100)
        return;

    n = ((-a * b) / d << 8) / h;
    f = ((b << 12) / d) << 12;

    if (n < -0x8000)
        n = -0x8000;
    if (n > 0x7FFF)
        n = 0x7FFF;

    SetDQA(n);
    SetDQB(f);
}
INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgpu_c", D_80048BBC);

