// Copyright Epic Games, Inc. All Rights Reserved.

#include "Terrain/GreedyMeshGenerator.h"

FIntPoint UGreedyMeshGenerator::GetChunkZBounds(
    const FChunkData& CD,
    const TMap<FIntPoint, TSharedPtr<FChunkData>>& NB) const
{
    int32 MinZ = WORLD_HEIGHT;
    int32 MaxZ = 0;

    for (int32 X = -1; X <= CHUNK_SIZE; X++)
    {
        for (int32 Y = -1; Y <= CHUNK_SIZE; Y++)
        {
            const FChunkData* Chunk = nullptr;
            int32 LX = X, LY = Y;

            if (X >= 0 && X < CHUNK_SIZE && Y >= 0 && Y < CHUNK_SIZE)
            {
                Chunk = &CD;
            }
            else
            {
                int32 WX = CD.Position.X * CHUNK_SIZE + X;
                int32 WY = CD.Position.Y * CHUNK_SIZE + Y;
                FIntPoint NC(FMath::FloorToInt((float)WX / CHUNK_SIZE),
                              FMath::FloorToInt((float)WY / CHUNK_SIZE));
                if (NC == CD.Position)
                {
                    Chunk = &CD;
                }
                else
                {
                    const auto* NP = NB.Find(NC);
                    if (!NP || !NP->IsValid() || !(*NP)->bIsGenerated) continue;
                    Chunk = NP->Get();
                    LX = WX - NC.X * CHUNK_SIZE;
                    LY = WY - NC.Y * CHUNK_SIZE;
                }
            }

            int32 ColIdx = FChunkData::GetColumnIndex(LX, LY);
            int32 H = Chunk->HeightData[ColIdx];

            if (H > 0) { MinZ = FMath::Min(MinZ, H); MaxZ = FMath::Max(MaxZ, H); }
        }
    }

    if (MinZ > MaxZ) { MinZ = 0; MaxZ = 0; }
    MinZ = FMath::Max(MinZ - 1, 0);
    MaxZ = FMath::Min(MaxZ + 1, WORLD_HEIGHT - 1);

    return FIntPoint(MinZ, MaxZ);
}

void UGreedyMeshGenerator::GenerateMesh(
    const FChunkData& ChunkData,
    const TMap<FIntPoint, TSharedPtr<FChunkData>>& Neighbors,
    TMap<EBlockId, FMeshSectionData>& OutSections,
    float BlockScale)
{
    for (auto& Pair : OutSections)
        Pair.Value.Reset();
    OutSections.Empty();

    FIntPoint Bounds = GetChunkZBounds(ChunkData, Neighbors);
    int32 MinZ = Bounds.X;
    int32 MaxZ = Bounds.Y;

    ProcessAxis(ChunkData, Neighbors, 0, -1, CHUNK_SIZE, OutSections, BlockScale, MinZ, MaxZ);
    ProcessAxis(ChunkData, Neighbors, 0,  1, CHUNK_SIZE, OutSections, BlockScale, MinZ, MaxZ);
    ProcessAxis(ChunkData, Neighbors, 1, -1, CHUNK_SIZE, OutSections, BlockScale, MinZ, MaxZ);
    ProcessAxis(ChunkData, Neighbors, 1,  1, CHUNK_SIZE, OutSections, BlockScale, MinZ, MaxZ);
    ProcessAxis(ChunkData, Neighbors, 2, -1, WORLD_HEIGHT, OutSections, BlockScale, MinZ, MaxZ);
    ProcessAxis(ChunkData, Neighbors, 2,  1, WORLD_HEIGHT, OutSections, BlockScale, MinZ, MaxZ);
}

void UGreedyMeshGenerator::ProcessAxis(
    const FChunkData& CD,
    const TMap<FIntPoint, TSharedPtr<FChunkData>>& NB,
    int32 Axis, int32 Sign, int32 LayerCount,
    TMap<EBlockId, FMeshSectionData>& Out, float Scale,
    int32 MinZ, int32 MaxZ)
{
    int32 A1 = (Axis + 1) % 3;
    int32 A2 = (Axis + 2) % 3;
    bool bA1isZ = (A1 == 2);
    bool bA2isZ = (A2 == 2);
    bool bAxisIsZ = (Axis == 2);

    int32 ZOff = MinZ - 1;

    int32 S1 = bA1isZ ? (MaxZ - MinZ + 3) : CHUNK_SIZE;
    int32 S2 = bA2isZ ? (MaxZ - MinZ + 3) : CHUNK_SIZE;
    int32 RealLayerCount = bAxisIsZ ? (MaxZ - MinZ + 3) : LayerCount;

    TArray<uint8> Mask;
    TArray<EBlockId> Types;
    Mask.SetNumUninitialized(S1 * S2);
    Types.SetNumUninitialized(S1 * S2);

    for (int32 L = 0; L < RealLayerCount; L++)
    {
        FMemory::Memzero(Mask.GetData(), S1 * S2);

        for (int32 U = 0; U < S1; U++)
            for (int32 V = 0; V < S2; V++)
            {
                int32 P[3] = {};
                P[Axis] = bAxisIsZ ? (ZOff + L) : L;
                P[A1]   = bA1isZ   ? (ZOff + U) : U;
                P[A2]   = bA2isZ   ? (ZOff + V) : V;

                EBlockId B = GetBlock(CD, NB, P[0], P[1], P[2]);
                if (B == EBlockId::Air || B >= EBlockId::MAX) continue;

                int32 PN[3] = {P[0], P[1], P[2]}; PN[Axis] += Sign;
                EBlockId Neighbor = GetBlock(CD, NB, PN[0], PN[1], PN[2]);

                if (Neighbor == EBlockId::Air)
                    { Mask[V * S1 + U] = 1; Types[V * S1 + U] = B; }
            }

        for (int32 V = 0; V < S2; V++)
            for (int32 U = 0; U < S1; U++)
            {
                if (!Mask[V * S1 + U]) continue;
                EBlockId Cur = Types[V * S1 + U];

                int32 W = 1;
                while (U + W < S1 && Mask[V * S1 + U + W] && Types[V * S1 + U + W] == Cur) W++;

                int32 H = 1;
                while (V + H < S2)
                {
                    bool ok = true;
                    for (int32 DU = 0; DU < W; DU++)
                        if (!Mask[(V + H) * S1 + U + DU] || Types[(V + H) * S1 + U + DU] != Cur)
                            { ok = false; break; }
                    if (!ok) break;
                    H++;
                }

                for (int32 DV = 0; DV < H; DV++)
                    for (int32 DU = 0; DU < W; DU++)
                        Mask[(V + DV) * S1 + U + DU] = 0;

                FMeshSectionData& Section = Out.FindOrAdd(Cur);

                int32 AdjU = bA1isZ ? (ZOff + U) : U;
                int32 AdjV = bA2isZ ? (ZOff + V) : V;
                int32 AdjLayer = bAxisIsZ ? (ZOff + L) : L;
                AddQuad(Section, Cur, Axis, Sign, AdjLayer, AdjU, AdjV, W, H, Scale);
            }
    }
}

void UGreedyMeshGenerator::AddQuad(
    FMeshSectionData& Out, EBlockId BlockType,
    int32 Axis, int32 Sign, int32 Layer,
    int32 U, int32 V, int32 W, int32 H, float Scale)
{
    int32 A1 = (Axis + 1) % 3;
    int32 A2 = (Axis + 2) % 3;
    float L = (float)(Layer + (Sign > 0 ? 1 : 0)) * Scale;
    float UU = (float)U * Scale;
    float VV = (float)V * Scale;
    float UW = (float)(U + W) * Scale;
    float VH = (float)(V + H) * Scale;

    FVector Verts[4];
    Verts[0][Axis] = L; Verts[0][A1] = UU; Verts[0][A2] = VV;
    Verts[1][Axis] = L; Verts[1][A1] = UW; Verts[1][A2] = VV;
    Verts[2][Axis] = L; Verts[2][A1] = UW; Verts[2][A2] = VH;
    Verts[3][Axis] = L; Verts[3][A1] = UU; Verts[3][A2] = VH;

    FVector Norm(0,0,0); Norm[Axis] = (float)Sign;
    FProcMeshTangent Tan(1,0,0);

    FColor Color = GetBlockColor(BlockType, Axis, Sign);

    int32 B = Out.Vertices.Num();
    if (Sign > 0)
    {
        Out.Vertices.Add(Verts[0]); Out.Vertices.Add(Verts[3]);
        Out.Vertices.Add(Verts[2]); Out.Vertices.Add(Verts[1]);
        Out.Triangles.Add(B+0); Out.Triangles.Add(B+1); Out.Triangles.Add(B+2);
        Out.Triangles.Add(B+0); Out.Triangles.Add(B+2); Out.Triangles.Add(B+3);
    }
    else
    {
        Out.Vertices.Add(Verts[0]); Out.Vertices.Add(Verts[1]);
        Out.Vertices.Add(Verts[2]); Out.Vertices.Add(Verts[3]);
        Out.Triangles.Add(B+0); Out.Triangles.Add(B+1); Out.Triangles.Add(B+2);
        Out.Triangles.Add(B+0); Out.Triangles.Add(B+2); Out.Triangles.Add(B+3);
    }

    for (int32 i = 0; i < 4; i++)
    {
        Out.Normals.Add(Norm);
        Out.Colors.Add(Color);
        Out.UVs.Add(FVector2D(0,0));
        Out.Tangents.Add(Tan);
    }
}

EBlockId UGreedyMeshGenerator::GetBlock(
    const FChunkData& CD,
    const TMap<FIntPoint, TSharedPtr<FChunkData>>& NB,
    int32 X, int32 Y, int32 Z) const
{
    if (Z < 0 || Z >= WORLD_HEIGHT) return EBlockId::Air;
    if (X >= 0 && X < CHUNK_SIZE && Y >= 0 && Y < CHUNK_SIZE)
        return CD.GetBlock(X, Y, Z);

    int32 WX = CD.Position.X * CHUNK_SIZE + X;
    int32 WY = CD.Position.Y * CHUNK_SIZE + Y;
    FIntPoint NC(FMath::FloorToInt((float)WX / CHUNK_SIZE), FMath::FloorToInt((float)WY / CHUNK_SIZE));
    if (NC == CD.Position) return CD.GetBlock(X, Y, Z);

    const auto* NP = NB.Find(NC);
    if (!NP || !NP->IsValid() || !(*NP)->bIsGenerated) return EBlockId::Air;
    return (*NP)->GetBlock(WX - NC.X * CHUNK_SIZE, WY - NC.Y * CHUNK_SIZE, Z);
}
