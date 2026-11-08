#include "common.h"
#include "system/memory.h"
#include "system/model.h"
#include "system/debug.h"

extern u8 D_80059598;
extern u8 D_80059599;
extern u8 D_8005959A;
extern u32 D_800500F8;
extern u32 D_800500FC;
extern u8 D_8006FAF0;
extern s32 g_ModelTextureMode;
extern s32 g_ModelClutMode;
extern u16 g_ModelTexPage;
extern u16 g_ModelClut;
extern s32 g_ModelBaseTexPage;
extern s32 g_ModelBaseClut;

extern void* g_pCurModelLightData;
extern void* g_pCurModelPacket;
extern SVECTOR* g_pCurModelNormals;
extern SVECTOR* g_pCurModelVertices;

// When a model file has been loaded somewhere in memory, the relative pointers
// in the file needs to be resolved to their concrete address.
int ModelResolvePointers(ModelFileHeader* pModelFile) {
    int numParts;
    int i;
    int j;
    ModelPart* pCurPart;
    ModelPacketInfo* pPacketInfo;

    i = pModelFile->flags;
    numParts = pModelFile->numModelParts;
    if ((i & 1) == 0) {
        pModelFile->flags = i | 1;
    
        pCurPart = pModelFile->modelParts;
        for (i = 0; i < numParts; i++) {
            pCurPart->pVertices = (mem_addr)pCurPart->pVertices + (mem_addr)pModelFile;
            pCurPart->unkC = (mem_addr)pCurPart->unkC + (mem_addr)pModelFile;
            pCurPart->unk10 = (mem_addr)pCurPart->unk10 + (mem_addr)pModelFile;
            pCurPart->pColors = (mem_addr)pCurPart->pColors + (mem_addr)pModelFile;
            
            if (pCurPart->unk1C != NULL) {
                pCurPart->unk1C = (mem_addr)pCurPart->unk1C + (mem_addr)pModelFile;
                pPacketInfo = pCurPart->unk1C + 1;
                for (j = *pCurPart->unk1C; j != -1; j--) {
                    pPacketInfo[j].lightDataSize += (mem_addr)pModelFile;
                    pPacketInfo[j].modelPacketSize += (mem_addr)pModelFile;
                }
            }
            
            pCurPart++;
        }
    }

    return numParts;
}

int func_8002C4BC(ModelFileHeader* pModelFile) {
    int numParts;
    int i;
    int j;
    ModelPart* pCurPart;
    ModelPacketInfo* pPacketInfo;

    i = pModelFile->flags;
    numParts = pModelFile->numModelParts;
    if ((i & 1) == 0) {
        return numParts;
    }

    pModelFile->flags = i & ~1;
    pCurPart = pModelFile->modelParts;
    for (i = 0; i < numParts; i++) {
        pCurPart->pVertices = (mem_addr)pCurPart->pVertices - (mem_addr)pModelFile;
        pCurPart->unkC = (mem_addr)pCurPart->unkC - (mem_addr)pModelFile;
        pCurPart->unk10 = (mem_addr)pCurPart->unk10 - (mem_addr)pModelFile;
        pCurPart->pColors = (mem_addr)pCurPart->pColors - (mem_addr)pModelFile;

        if (pCurPart->unk1C != NULL) {
            pPacketInfo = pCurPart->unk1C + 1;
            for (j = *pCurPart->unk1C; j != -1; j--) {
                pPacketInfo[j].lightDataSize -= (mem_addr)pModelFile;
                pPacketInfo[j].modelPacketSize -= (mem_addr)pModelFile;
            }
            pCurPart->unk1C = (mem_addr)pCurPart->unk1C - (mem_addr)pModelFile;
        }

        pCurPart++;
    }

    return numParts;
}

void func_8002C59C(ModelPart* pModelPart) {
    u_short flags;
    ModelPacketInfo* pPacketInfo;
    int j;

    flags = pModelPart->flags;
    if (flags & 0x20) {
        return;
    }
    pModelPart->flags = flags | 0x20;
    pModelPart->pVertices = (mem_addr)pModelPart->pVertices + (mem_addr)pModelPart;
    pModelPart->unkC = (mem_addr)pModelPart->unkC + (mem_addr)pModelPart;
    pModelPart->unk10 = (mem_addr)pModelPart->unk10 + (mem_addr)pModelPart;
    pModelPart->pColors = (mem_addr)pModelPart->pColors + (mem_addr)pModelPart;
    if (pModelPart->unk1C != NULL) {
        pModelPart->unk1C = (mem_addr)pModelPart->unk1C + (mem_addr)pModelPart;
        pPacketInfo = pModelPart->unk1C + 1;
        for (j = *pModelPart->unk1C; j != -1; j--) {
            pPacketInfo[j].lightDataSize += (mem_addr)pModelPart;
            pPacketInfo[j].modelPacketSize += (mem_addr)pModelPart;
        }
    }
}

int func_8002C644(ModelFileHeader* pModelFile) {
    u_int flags;

    flags = pModelFile->flags;
    if (flags & 0x2) {
        return 1;
    }
    pModelFile->flags = flags | 0x2;
    HeapInsertAlloc((HeapBlock*)pModelFile, (mem_addr)pModelFile->modelParts[0].pColors - (mem_addr)pModelFile);
    return 0;
}

int func_8002C68C(ModelPart* pModelPart) {
    u_short flags;

    flags = pModelPart->flags;
    if (flags & 0x40) {
        return 1;
    }
    pModelPart->flags = flags | 0x40;
    HeapInsertAlloc((HeapBlock*)pModelPart, (mem_addr)pModelPart->pColors - (mem_addr)pModelPart);
    pModelPart->pColors = NULL;
    return 0;
}

void SetNearColor(u_char r, u_char g, u_char b) {
    D_80059598 = r;
    D_80059599 = g;
    D_8005959A = b;
}

// Render a model part's packet.
// This usually calls into a handler for the chosen render path, where
// vertices are converted from local to screen space, clipping is performed,
// and the primitives in the packet is added to the OT provided.
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/model", ModelRenderPacket);

// Initialize a model part's packet.
// This involves setting primitive code and len, color (which can include light computation),
// and UVs, texture page and clut for textured primitives.
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/model", ModelInitializePacket);

void ModelAllocatePackets(ModelPart* pModelPart, void** pPacket1, void** pPacket2) {
    void* pPacketBuffer;

    HeapSetCurrentContentType(HEAP_CONTENT_MODEL_PACKET);
    pPacketBuffer = HeapAlloc(pModelPart->unk20[1].modelPacketSize * 2, 0x0);
    *pPacket1 = pPacketBuffer;
    *pPacket2 = pPacketBuffer + pModelPart->unk20[1].modelPacketSize;
}

void ModelPartFreeLightData(ModelPart* pModelPart) {
    if (pModelPart->flags & MODEL_FLAG_HAS_LIGHT_DATA) {
        HeapFree(pModelPart->pLightData);
        pModelPart->flags &= ~MODEL_FLAG_HAS_LIGHT_DATA;
    }
}

void ModelPacketSetBaseTexPage(u_short x, u_short y) {
    g_ModelBaseTexPage = GetTPage(0, 0, x, y) & 0x1F;
    g_ModelTextureMode = 1;
}

void func_8002CC54(u_short arg0) {
    g_ModelBaseTexPage = arg0;
    g_ModelTextureMode = 2;
}

void ModelPacketSetBaseClut(u_short x, u_short y) {
    g_ModelBaseClut = GetClut(x, y) & 0xFFF0;
    g_ModelClutMode = 0;
}

void func_8002CCAC(void) {
    g_ModelTextureMode = 0;
    g_ModelClutMode = 1;
}

void ModelPacketSetTexPage(PrimitiveHeader* pPrim) {
    g_ModelTexPage = pPrim->texpageAndClut;
    if (g_ModelTextureMode == 1) {
        g_ModelTexPage = g_ModelTexPage & 0xFFE0;
        g_ModelTexPage = g_ModelTexPage | g_ModelBaseTexPage;
    } else if (g_ModelTextureMode == 2) {
        g_ModelTexPage = g_ModelBaseTexPage;
    }
}

void ModelPacketSetClut(PrimitiveHeader* pPrim) {
    g_ModelClut = pPrim->texpageAndClut;
    if (g_ModelClutMode == 0) {
        g_ModelClut = g_ModelClut & 0xF;
        g_ModelClut = g_ModelClut | g_ModelBaseClut;
    }
}

u_int ModelPacketSetTextureData(PrimitiveHeader* pPrim) {
    if ((pPrim->code & 0xF0) == 0xC0) {
        switch (pPrim->code) {
            case 0xC4: // Tex page
                ModelPacketSetTexPage(pPrim);
                return PRIM_HAS_TEX_DATA;
            case 0xC8: // CLUT
                ModelPacketSetClut(pPrim);
                return PRIM_HAS_TEX_DATA;
            default:
                return PRIM_NO_TEX_DATA;
        }
    }
    return PRIM_NO_TEX_DATA;
}

// ---------------------------------------------
// This block of functions are initializer handlers which initializes
// primitives in model packets for rendering. The naming of the functions are
// quite vague for now, and should be renamed to something better if it turns out
// any of these handlers are used in specific ways.
//
// Initializing a primitive for the model packet typically involves setting the length,
// primitive code, color (which can entail lighting computation) and UVs, tpage
// and clut if the primitive is a textured one.
// ---------------------------------------------
u_int ModelPacketInitPolyF3(UntexturedPrimitive* pPrim, short* pIndices, u_int flags) {
    SVECTOR vecNormal;
    POLY_F3* pPoly;

    pPoly = g_pCurModelPacket;
    setlen(pPoly, 4);

    // Compute color from triangle normal and color in pPrim
    if (flags & 1) {
        // Store prim code and normal in light data
        if (flags & 2) {
            *(u32*)g_pCurModelLightData = *(u32*)pPrim;

            MathTriangleNormal(
                &g_pCurModelVertices[pIndices[0]], 
                &g_pCurModelVertices[pIndices[1]], 
                &g_pCurModelVertices[pIndices[2]], 
                (SVECTOR*)(g_pCurModelLightData += sizeof(P_CODE))
            );
            NormalLightCol((SVECTOR*)g_pCurModelLightData, 
                           (CVECTOR*)pPrim, 
                           (CVECTOR*)&pPoly->r0);
            g_pCurModelLightData += sizeof(SVECTOR);
            pPoly->code = pPrim->code;
        } else {
            // Don't store stuff in light data
            MathTriangleNormal(
                &g_pCurModelVertices[pIndices[0]],
                &g_pCurModelVertices[pIndices[1]],
                &g_pCurModelVertices[pIndices[2]],
                &vecNormal
            );
            NormalLightCol(&vecNormal, (CVECTOR*)pPrim, (CVECTOR*)&pPoly->r0);
            pPoly->code = pPrim->code;
        }
    } else if ((flags & 4)) {
        // Here, the P_CODE is expected to be set beforehand?
        g_pCurModelLightData += sizeof(P_CODE);
        NormalLightCol((SVECTOR*)g_pCurModelLightData, (CVECTOR*)pPrim, (CVECTOR*)&pPoly->r0);
        g_pCurModelLightData += sizeof(SVECTOR);
        pPoly->code = pPrim->code;
    } else {
        // Use color of pPrim instead of computing it from normal
        *(u32*)&pPoly->r0 = *(u32*)pPrim;
    }
    return 1;
}

u_int ModelPacketInitUnlitPolyF3(UntexturedPrimitive* pPrim, short* pIndices, u_int flags) {
    POLY_F3* pPoly;

    pPoly = (POLY_F3*)g_pCurModelPacket;
    setlen(pPoly, 4);
    *(u32*)&pPoly->r0 = *(u32*)pPrim;
    return 1;
}

// TODO: This one is a bit weird because the primitive stride is 0x18,
// and POLY_F4 is the only poly primitive which fits this size. However,
// in this case setlen(pPoly, 4) is wrong and should have been setlen(pPoly, 5)
u_int ModelPacketInitPolyF4(UntexturedPrimitive* pPrim, short* pIndices, u_int flags) {
    SVECTOR vecNormal;
    POLY_F4* pPoly;

    pPoly = g_pCurModelPacket;
    setlen(pPoly, 4);
    
    if (flags & 1) {
        if (flags & 2) {
            *(u32*)g_pCurModelLightData = *(u32*)pPrim;
            MathTriangleNormal(
                &g_pCurModelVertices[pIndices[0]], 
                &g_pCurModelVertices[pIndices[1]], 
                &g_pCurModelVertices[pIndices[2]], 
                (SVECTOR*)(g_pCurModelLightData += 4)
            );
            NormalLightCol(
                (SVECTOR*)g_pCurModelLightData, 
                (CVECTOR*)pPrim, 
                (CVECTOR*)&pPoly->r0
            );
            g_pCurModelLightData += sizeof(SVECTOR);
            pPoly->code = pPrim->code;
        } else {
            MathTriangleNormal(
                &g_pCurModelVertices[pIndices[0]], 
                &g_pCurModelVertices[pIndices[1]], 
                &g_pCurModelVertices[pIndices[2]], 
                &vecNormal
            );
            NormalLightCol(&vecNormal, (CVECTOR*)pPrim, (CVECTOR*)&pPoly->r0);
            pPoly->code = pPrim->code;
        }
    } else if (flags & 4) {
        g_pCurModelLightData += sizeof(P_CODE);
        NormalLightCol(
            (SVECTOR*)g_pCurModelLightData, 
            (CVECTOR*)pPrim, 
            (CVECTOR*)&pPoly->r0
        );
        g_pCurModelLightData += sizeof(SVECTOR);
        pPoly->code = pPrim->code;
    } else {
        *(u32*)&pPoly->r0 = *(u32*)pPrim;
    }
    return 1;
}

u_int ModelPacketInitUnlitPolyF4(UntexturedPrimitive* pPrim, short* pIndices, u_int flags) {
    POLY_F4* pPoly;

    pPoly = g_pCurModelPacket;
    setlen(pPoly, 5);
    *(u32*)&pPoly->r0 = *(u32*)pPrim;
    return 1;
}

u_int ModelPacketInitUnlitPolyFT4(TexturedQuadPrimitive* pPrim, short* pIndices, u_int flags) {
    POLY_FT4* pPoly;
    
    if (ModelPacketSetTextureData(pPrim) == PRIM_HAS_TEX_DATA) {
        return 0;
    }

    pPoly = (POLY_FT4*)g_pCurModelPacket;
    setlen(pPoly, 9);
    *(u32*)&pPoly->r0 = *(u32*)&pPrim->r;
    *(u32*)&pPoly->u0 = *(u16*)&pPrim->u0 | (g_ModelClut << 0x10);
    *(u32*)&pPoly->u1 = *(u16*)&pPrim->u1 | (g_ModelTexPage << 0x10);
    *(u16*)&pPoly->u2 = *(u16*)&pPrim->u2;
    *(u16*)&pPoly->u3 = *(u16*)&pPrim->u3;
    return 1;
}

u_int ModelPacketInitPolyG4(UntexturedPrimitive* pPrim, short* pIndices, u_int flags) {
    POLY_G4* pPoly;

    pPoly = g_pCurModelPacket;
    setlen(pPoly, 8);
    NormalColorCol3(
        &g_pCurModelNormals[pIndices[0]], 
        &g_pCurModelNormals[pIndices[1]],
        &g_pCurModelNormals[pIndices[2]],
        (CVECTOR*)&pPrim->r,
        &pPoly->r0,
        &pPoly->r1,
        &pPoly->r2
    );
    NormalLightCol(
        &g_pCurModelNormals[pIndices[3]], 
        (CVECTOR*)&pPrim->r,
        &pPoly->r3
    );

    setcode(pPoly, pPrim->code);
    
    POSSIBLE_DEBUG_CODE;
    return 1;
}

u_int ModelPacketInitPolyGT4(TexturedQuadPrimitive* pPrim, short* pIndices, u_int flags) {
    POLY_GT4* pPoly;

    if (ModelPacketSetTextureData(pPrim) == PRIM_HAS_TEX_DATA) {
        return 0;
    }
    
    pPoly = g_pCurModelPacket;
    setlen(pPoly, 12);
    
    NormalColor3(
        &g_pCurModelNormals[pIndices[0]], 
        &g_pCurModelNormals[pIndices[1]],
        &g_pCurModelNormals[pIndices[2]],
        &pPoly->r0,
        &pPoly->r1,
        &pPoly->r2
    );
    NormalColor(&g_pCurModelNormals[pIndices[3]], &pPoly->r3);
    
    *(u32*)&pPoly->u0 = *(u16*)&pPrim->u0 | (g_ModelClut << 0x10);
    *(u32*)&pPoly->u1 = *(u16*)&pPrim->u1 | (g_ModelTexPage << 0x10);
    *(u16*)&pPoly->u2 = *(u16*)&pPrim->u2;
    *(u16*)&pPoly->u3 = *(u16*)&pPrim->u3;

    setcode(pPoly, pPrim->code);
    return 1;
}

u_int func_8002D354(UntexturedPrimitive* pPrim, short* pIndices, u_int flags) {
    POLY_G4* pPoly;

    pPoly = g_pCurModelPacket;
    setlen(pPoly, 8);

    *(u32*)&pPoly->r0 = *(u32*)&pPrim->r;
    
    NormalColorCol3(
        &g_pCurModelNormals[pIndices[0]], 
        &g_pCurModelNormals[pIndices[1]],
        &g_pCurModelNormals[pIndices[2]],
        (CVECTOR*)&pPrim->r,
        &pPoly->r0,
        &pPoly->r1,
        &pPoly->r2
    );
    NormalLightCol(
        &g_pCurModelNormals[pIndices[3]], 
        (CVECTOR*)&pPrim->r,
        &pPoly->r3
    );

    setcode(pPoly, pPrim->code);
    
    POSSIBLE_DEBUG_CODE
    return 1;
}

u_int func_8002D420(TexturedQuadPrimitive* pPrim, short* pIndices, u_int flags) {
    POLY_GT4* pPoly;

    if (ModelPacketSetTextureData(pPrim) == PRIM_HAS_TEX_DATA) {
        return 0;
    }
    
    pPoly = g_pCurModelPacket;
    setlen(pPoly, 12);
    
    NormalColor3(
        &g_pCurModelNormals[pIndices[0]], 
        &g_pCurModelNormals[pIndices[1]],
        &g_pCurModelNormals[pIndices[2]],
        &pPoly->r0,
        &pPoly->r1,
        &pPoly->r2
    );
    NormalColor(&g_pCurModelNormals[pIndices[3]], &pPoly->r3);
    
    *(u32*)&pPoly->u0 = *(u16*)&pPrim->u0 | (g_ModelClut << 0x10);
    *(u32*)&pPoly->u1 = *(u16*)&pPrim->u1 | (g_ModelTexPage << 0x10);
    *(u16*)&pPoly->u2 = *(u16*)&pPrim->u2;
    *(u16*)&pPoly->u3 = *(u16*)&pPrim->u3;

    setcode(pPoly, pPrim->code);
    return 1;
}

u_int ModelPacketInitPolyFT4(TexturedQuadPrimitive* pPrim, short* pVertexIndices, u_int flags) {
    SVECTOR vecNormal;
    POLY_FT4* pPoly;

    if (ModelPacketSetTextureData(pPrim) == PRIM_HAS_TEX_DATA) {
        return 0;
    }
    
    pPoly = g_pCurModelPacket;
    setlen(pPoly, 9);

    *(u32*)&pPoly->u0 = *(u16*)&pPrim->u0 | (g_ModelClut << 0x10);
    *(u32*)&pPoly->u1 = *(u16*)&pPrim->u1 | (g_ModelTexPage << 0x10);
    *(u16*)&pPoly->u2 = *(u16*)&pPrim->u2;
    *(u16*)&pPoly->u3 = *(u16*)&pPrim->u3;
    
    if (flags & 1) {
        if (flags & 2) {
            MathTriangleNormal(
                &g_pCurModelVertices[pVertexIndices[0]], 
                &g_pCurModelVertices[pVertexIndices[1]], 
                &g_pCurModelVertices[pVertexIndices[2]], 
                (SVECTOR*)g_pCurModelLightData
            );
            NormalColor((SVECTOR*)g_pCurModelLightData, (CVECTOR*)&pPoly->r0);
        } else {
            MathTriangleNormal(
                &g_pCurModelVertices[pVertexIndices[0]], 
                &g_pCurModelVertices[pVertexIndices[1]], 
                &g_pCurModelVertices[pVertexIndices[2]], 
                &vecNormal
            );
            NormalColor(&vecNormal, (CVECTOR*)&pPoly->r0);
        }
    } else if (flags & 4) {
        NormalColor((SVECTOR*)g_pCurModelLightData, (CVECTOR*)&pPoly->r0);
    }
    
    g_pCurModelLightData += sizeof(SVECTOR);
    pPoly->code = pPrim->code;
    return 1;
}

u_int ModelPacketInitPolyG3(UntexturedPrimitive* pPrim, short* pIndices, u_int flags) {
    POLY_G3* pPoly;

    pPoly = g_pCurModelPacket;
    setlen(pPoly, 6);
    
    if (flags & 2) {
        *(u32*)g_pCurModelLightData = *(u32*)pPrim; // TODO: Clean up
        g_pCurModelLightData += sizeof(P_CODE);
    }

    NormalColorCol3(
        &g_pCurModelNormals[pIndices[0]], 
        &g_pCurModelNormals[pIndices[1]],
        &g_pCurModelNormals[pIndices[2]],
        (CVECTOR*)&pPrim->r,
        &pPoly->r0,
        &pPoly->r1,
        &pPoly->r2
    );
    
    pPoly->code = pPrim->code;
    return 1;
}

u_int ModelPacketInitPolyG3_2(UntexturedPrimitive* pPrim, short* pIndices, u_int flags) {
    POLY_G3* pPoly;

    pPoly = g_pCurModelPacket;
    setlen(pPoly, 6);
    
    NormalColorCol3(
        &g_pCurModelNormals[pIndices[0]], 
        &g_pCurModelNormals[pIndices[1]],
        &g_pCurModelNormals[pIndices[2]],
        (CVECTOR*)&pPrim->r,
        &pPoly->r0,
        &pPoly->r1,
        &pPoly->r2
    );
    
    pPoly->code = pPrim->code;
    return 1;
}

u_int ModelPacketInitPolyFT3(TexturedTrianglePrimitive* pPrim, short* pIndices, u_int flags) {
    SVECTOR vecNormal;
    POLY_FT3* pPoly;

    if (ModelPacketSetTextureData(pPrim) == PRIM_HAS_TEX_DATA) {
        return 0;
    }
    pPoly = g_pCurModelPacket;
    setlen(pPoly, 7);
    
    // Set UVs, clut and tpage
    *(u32*)&pPoly->u0 = *(u16*)&pPrim->u0 | (g_ModelClut << 0x10);
    *(u32*)&pPoly->u1 = *(u16*)&pPrim->u1 | (g_ModelTexPage << 0x10);
    *(u16*)&pPoly->u2 = *(u16*)&pPrim->u2;
    
    if (flags & 1) {
        if (flags & 2) {
            MathTriangleNormal(
                &g_pCurModelVertices[pIndices[0]], 
                &g_pCurModelVertices[pIndices[1]], 
                &g_pCurModelVertices[pIndices[2]], 
                (SVECTOR*)g_pCurModelLightData
            );
            NormalColor((SVECTOR*)g_pCurModelLightData, (CVECTOR*)&pPoly->r0);
        } else {
            MathTriangleNormal(
                &g_pCurModelVertices[pIndices[0]], 
                &g_pCurModelVertices[pIndices[1]], 
                &g_pCurModelVertices[pIndices[2]], 
                &vecNormal
            );
            NormalColor(&vecNormal, (CVECTOR*)&pPoly->r0);
        }
    } else if (flags & 4) {
        NormalColor((SVECTOR*)g_pCurModelLightData, (CVECTOR*)&pPoly->r0);
    }
    
    g_pCurModelLightData += sizeof(SVECTOR);
    pPoly->code = pPrim->code;
    return 1;
}

u_int ModelPacketInitUnlitPolyFT3(TexturedTrianglePrimitive* pPrim, short* pIndices, u_int flags) {
    POLY_FT3* pPoly;
    
    if (ModelPacketSetTextureData(pPrim) == PRIM_HAS_TEX_DATA) {
        return 0;
    }
    pPoly = (POLY_FT3*)g_pCurModelPacket;
    setlen(pPoly, 7);
    
    // Set UVs, clut and tpage
    *(u32*)&pPoly->u0 = *(u16*)&pPrim->u0 | (g_ModelClut << 0x10);
    *(u32*)&pPoly->u1 = *(u16*)&pPrim->u1 | (g_ModelTexPage << 0x10);
    *(u16*)&pPoly->u2 = *(u16*)&pPrim->u2;

    setcode(pPoly, pPrim->code);
    return 1;
}

u_int ModelPacketInitPolyGT3(TexturedTrianglePrimitive* pPrim, short* pIndices, u_int flags) {
    POLY_GT3* pPoly;
    
    if (ModelPacketSetTextureData(pPrim) == PRIM_HAS_TEX_DATA) {
        return 0;
    }
    
    pPoly = g_pCurModelPacket;
    setlen(pPoly, 9);
    NormalColor3(
        &g_pCurModelNormals[pIndices[0]], 
        &g_pCurModelNormals[pIndices[1]],
        &g_pCurModelNormals[pIndices[2]],
        (CVECTOR*)&pPoly->r0,
        (CVECTOR*)&pPoly->r1,
        (CVECTOR*)&pPoly->r2
    );

    // Set UVs, clut and tpage
    *(u32*)&pPoly->u0 = *(u16*)&pPrim->u0 | (g_ModelClut << 0x10);
    *(u32*)&pPoly->u1 = *(u16*)&pPrim->u1 | (g_ModelTexPage << 0x10);
    *(u16*)&pPoly->u2 = *(u16*)&pPrim->u2;
    
    pPoly->code = pPrim->code;
    return 1;
}

u_int ModelPacketInitPolyFT3_2(TexturedTrianglePrimitive* pPrim, short* pIndices, u_int flags) {
    POLY_FT3* pPoly;

    pPoly = g_pCurModelPacket;
    SetPolyFT3(pPoly);
    SetShadeTex(pPoly, 1);
    pPoly->tpage = (GetTPage(1, 0, 0x280, 0) & 0xFFE0) | g_ModelBaseTexPage;
    pPoly->clut = (GetClut(0, 0x1E0) & 0xF) | g_ModelBaseClut;
    return 1;
}
// -------------------------------------

void MathTriangleNormal(SVECTOR* pVertex1, SVECTOR* pVertex2, SVECTOR* pVertex3, SVECTOR* pNormal) {
    VECTOR edge1;
    VECTOR edge2;
    VECTOR normal;
    int sqrt;
    int largestComponent;

    // Compute vertices from vertex 1 -> vertex 2, and
    // from vertex 1 -> vertex 3
    edge1.vx = pVertex2->vx - pVertex1->vx;
    edge1.vy = pVertex2->vy - pVertex1->vy;
    edge1.vz = pVertex2->vz - pVertex1->vz;
    
    edge2.vx = pVertex3->vx - pVertex1->vx;
    edge2.vy = pVertex3->vy - pVertex1->vy;
    edge2.vz = pVertex3->vz - pVertex1->vz;

    // Taking the cross product of these two vectors gives us a vector
    // perpendicular to the two edges, effectively pointing in the direction of the normal
    OuterProduct0(&edge2, &edge1, &normal);

    // Find the largest component and scale the vector by it
    largestComponent = MathGetLargestVectorComponent(normal.vx, normal.vy, normal.vz);
    if (largestComponent < 0) {
        largestComponent = -largestComponent;
    }
    sqrt = SquareRoot0(largestComponent);
    normal.vx = normal.vx / sqrt;
    normal.vy = normal.vy / sqrt;
    normal.vz = normal.vz / sqrt;
    
    VectorNormalS(&normal, pNormal);
}

// Get largest absolute value
long MathGetLargestVectorComponent(long x, long y, long z) {
    long absX;
    long absY;
    long absZ;
    long result;

    absX = x;
    absY = y;
    absZ = z;
    
    if (x < 0) absX = -x;
    if (y < 0) absY = -y;
    if (z < 0) absZ = -z;
    
    if (absX >= absY && absX >= absZ) {
        return x;
    }
    
    if (absY >= absX && absY >= absZ) {
        return y;
    }

    result = absZ < absX;
    if (result == 0) {
        result = absZ < absY;
        if (result == 0) {
            return z;
        }
    }
    
    return result;
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/model", func_8002DD20);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/model", func_8002DDE4);

void* func_8002DFE0(void) {
    return &D_8006FAF0;
}

void func_8002DFF0(u32 arg0, u32 arg1) {
    D_800500FC = (arg1 - 1) << 0x10;
    D_800500F8 = arg0;
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/model", func_8002E010);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/model", func_8002E448);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/model", func_8002E64C);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/model", func_8002E8B4);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/model", func_8002EAB8);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/model", func_8002ED20);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/model", func_8002EEF8);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/model", func_8002F0E4);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/model", func_8002F2E0);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/model", func_8002F4B4);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/model", func_8002F6B4);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/model", func_8002F8D0);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/model", func_8002FAE8);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/model", func_8002FCFC);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/model", func_8002FF0C);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/model", func_8003014C);

// Vertex data as the packet renderer touches it: three unsigned 16-bit
// components followed by padding.
typedef struct {
    /* 0x0 */ u16 x;
    /* 0x2 */ u16 y;
    /* 0x4 */ u16 z;
    /* 0x6 */ u16 pad6;
} ModelVertex; // Size: 0x8

void func_800301C8(ModelVertex* pDest, ModelVertex* pSrc, int count, short* pIndices) {
    int i;
    int index;

    for (i = count - 1; i != -1; i--) {
        index = pIndices[i];
        pDest[index].x = pSrc[index].x;
        pDest[index].y = pSrc[index].y;
        pDest[index].z = pSrc[index].z;
    }
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/model", func_80030228);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/model", func_800302D4);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/model", func_800303C8);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/model", func_800305D8);

// Saved state of a model part's vertex data, so that a part can have its
// vertices replaced and later restored.
typedef struct {
    /* 0x0 */ ModelPart* pModelPart;
    /* 0x4 */ SVECTOR* pVertices;
    /* 0x8 */ u32 unkC;
} ModelPartVertexBackup;

void func_800306D0(ModelPartVertexBackup* pBackup) {
    ModelPart* pModelPart;

    if (pBackup != NULL) {
        pModelPart = pBackup->pModelPart;
        HeapFree(pModelPart->pVertices);
        if (pModelPart->flags & 0x10) {
            HeapFree(pModelPart->unkC);
        }
        pModelPart->pVertices = pBackup->pVertices;
        pModelPart->unkC = pBackup->unkC;
        HeapFree(pBackup);
    }
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/model", func_80030750);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/model", func_80030988);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/model", func_80030A30);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/model", func_80030B14);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/model", func_80030C40);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/model", func_80030C78);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/model", func_80030C98);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/model", func_80030EE8);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/model", func_8003101C);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/model", func_800315A0);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/model", func_800315C4);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/model", func_800315E8);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/model", func_8003160C);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/model", func_80031630);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/model", func_80031654);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/model", func_80031678);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/model", func_8003169C);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/model", func_800316C0);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/model", func_800316E4);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/model", func_80031708);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/model", func_8003172C);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/model", func_80031750);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/model", func_80031774);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/model", func_80031798);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/model", func_800317BC);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/model", func_800317E0);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/model", func_80031804);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/model", func_80031828);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/model", func_8003184C);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/model", func_80031870);
