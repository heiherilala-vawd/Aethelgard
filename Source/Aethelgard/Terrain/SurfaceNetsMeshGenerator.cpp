// Copyright Epic Games, Inc. All Rights Reserved.

#include "Terrain/SurfaceNetsMeshGenerator.h"

static EBlockId GetBlockAtHelper(
    const FChunkData& CenterChunk,
    const TMap<FIntVector, TSharedPtr<FChunkData>>& Neighbors,
    int32 X, int32 Y, int32 Z)
{
    if (X >= 0 && X < CHUNK_SIZE && Y >= 0 && Y < CHUNK_SIZE && Z >= 0 && Z < CHUNK_SIZE)
        return CenterChunk.GetBlock(X, Y, Z);

    int32 WX = CenterChunk.Position.X * CHUNK_SIZE + X;
    int32 WY = CenterChunk.Position.Y * CHUNK_SIZE + Y;
    int32 WZ = CenterChunk.Position.Z * CHUNK_SIZE + Z;

    FIntVector NC(
        FMath::FloorToInt((float)WX / CHUNK_SIZE),
        FMath::FloorToInt((float)WY / CHUNK_SIZE),
        FMath::FloorToInt((float)WZ / CHUNK_SIZE)
    );

    if (NC == CenterChunk.Position)
        return CenterChunk.GetBlock(X, Y, Z);

    const TSharedPtr<FChunkData>* NP = Neighbors.Find(NC);
    if (!NP || !NP->IsValid() || !(*NP)->bIsGenerated)
        return EBlockId::Air;

    const FChunkData& NCData = *NP->Get();
    return NCData.GetBlock(WX - NC.X * CHUNK_SIZE, WY - NC.Y * CHUNK_SIZE, WZ - NC.Z * CHUNK_SIZE);
}

static FVector ComputeVertexPos(
    const FChunkData& CenterChunk,
    const TMap<FIntVector, TSharedPtr<FChunkData>>& Neighbors,
    int32 X, int32 Y, int32 Z)
{
    static const int32 EdgeEndpoints[12][6] = {
        {0,0,0, 1,0,0}, {1,0,0, 1,1,0}, {1,1,0, 0,1,0}, {0,1,0, 0,0,0},
        {0,0,1, 1,0,1}, {1,0,1, 1,1,1}, {1,1,1, 0,1,1}, {0,1,1, 0,0,1},
        {0,0,0, 0,0,1}, {1,0,0, 1,0,1}, {1,1,0, 1,1,1}, {0,1,0, 0,1,1}
    };

    FVector Pos(0, 0, 0);
    int32 Count = 0;

    for (int32 e = 0; e < 12; e++)
    {
        int32 Ax = EdgeEndpoints[e][0], Ay = EdgeEndpoints[e][1], Az = EdgeEndpoints[e][2];
        int32 Bx = EdgeEndpoints[e][3], By = EdgeEndpoints[e][4], Bz = EdgeEndpoints[e][5];

        auto IsSolid = [](EBlockId B) { return B != EBlockId::Air && B < EBlockId::MAX; };
        bool bSolidA = IsSolid(GetBlockAtHelper(CenterChunk, Neighbors, X + Ax, Y + Ay, Z + Az));
        bool bSolidB = IsSolid(GetBlockAtHelper(CenterChunk, Neighbors, X + Bx, Y + By, Z + Bz));

        if (bSolidA != bSolidB)
        {
            Pos.X += (float)(X * 2 + Ax + Bx) * 0.5f;
            Pos.Y += (float)(Y * 2 + Ay + By) * 0.5f;
            Pos.Z += (float)(Z * 2 + Az + Bz) * 0.5f;
            Count++;
        }
    }

    if (Count > 0)
        Pos /= (float)Count;
    else
        Pos = FVector((float)X + 0.5f, (float)Y + 0.5f, (float)Z + 0.5f);

    return Pos;
}

static FVector ComputeVertexNormal(
    const FChunkData& CenterChunk,
    const TMap<FIntVector, TSharedPtr<FChunkData>>& Neighbors,
    const FVector& VertexPos)
{
    auto IsSolid = [](EBlockId B) { return B != EBlockId::Air && B < EBlockId::MAX; };
    float Vx = 0, Vy = 0, Vz = 0;

    for (int32 DZ = -1; DZ <= 1; DZ += 2)
    {
        for (int32 DY = -1; DY <= 1; DY += 2)
        {
            for (int32 DX = -1; DX <= 1; DX += 2)
            {
                if (DX == 0 && DY == 0 && DZ == 0) continue;

                int32 Bx = FMath::FloorToInt(VertexPos.X + DX * 0.5f);
                int32 By = FMath::FloorToInt(VertexPos.Y + DY * 0.5f);
                int32 Bz = FMath::FloorToInt(VertexPos.Z + DZ * 0.5f);

                float Center = IsSolid(GetBlockAtHelper(CenterChunk, Neighbors, Bx, By, Bz)) ? 1.0f : 0.0f;
                float Dx = IsSolid(GetBlockAtHelper(CenterChunk, Neighbors, Bx + 1, By, Bz)) ? 1.0f : 0.0f;
                float Dy = IsSolid(GetBlockAtHelper(CenterChunk, Neighbors, Bx, By + 1, Bz)) ? 1.0f : 0.0f;
                float Dz = IsSolid(GetBlockAtHelper(CenterChunk, Neighbors, Bx, By, Bz + 1)) ? 1.0f : 0.0f;

                Vx += (Dx - Center) * DX;
                Vy += (Dy - Center) * DY;
                Vz += (Dz - Center) * DZ;
            }
        }
    }

    FVector Normal(Vx, Vy, Vz);
    float Len = Normal.Size();
    return (Len > KINDA_SMALL_NUMBER) ? (Normal / Len) : FVector::UpVector;
}

static EBlockId GetBlockColorId(
    const FChunkData& CenterChunk,
    const TMap<FIntVector, TSharedPtr<FChunkData>>& Neighbors,
    int32 X, int32 Y, int32 Z)
{
    for (int32 DZ = 0; DZ <= 1; DZ++)
        for (int32 DY = 0; DY <= 1; DY++)
            for (int32 DX = 0; DX <= 1; DX++)
            {
                EBlockId B = GetBlockAtHelper(CenterChunk, Neighbors, X + DX, Y + DY, Z + DZ);
                if (B != EBlockId::Air && B < EBlockId::MAX)
                    return B;
            }
    return EBlockId::Stone;
}

void USurfaceNetsMeshGenerator::GenerateMesh(
    const FChunkData& ChunkData,
    const TMap<FIntVector, TSharedPtr<FChunkData>>& Neighbors,
    FMeshSectionData& OutMeshData)
{
    OutMeshData.Reset();

    int32 CellCount = CHUNK_SIZE + 1;
    TArray<TArray<TArray<int32>>> VI;
    VI.SetNum(CellCount);
    for (int32 X = 0; X < CellCount; X++)
    {
        VI[X].SetNum(CellCount);
        for (int32 Y = 0; Y < CellCount; Y++)
            VI[X][Y].Init(-1, CellCount);
    }

    for (int32 Z = 0; Z < CellCount; Z++)
        for (int32 Y = 0; Y < CellCount; Y++)
            for (int32 X = 0; X < CellCount; X++)
            {
                bool bSolid = false, bAir = false;
                for (int32 DZ = 0; DZ <= 1 && (!bSolid || !bAir); DZ++)
                    for (int32 DY = 0; DY <= 1 && (!bSolid || !bAir); DY++)
                        for (int32 DX = 0; DX <= 1 && (!bSolid || !bAir); DX++)
                        {
                            EBlockId B = GetBlockAtHelper(ChunkData, Neighbors, X + DX - 1, Y + DY - 1, Z + DZ - 1);
                            if (B != EBlockId::Air && B < EBlockId::MAX) bSolid = true;
                            else bAir = true;
                        }

                if (bSolid && bAir)
                {
                    FVector Pos = ComputeVertexPos(ChunkData, Neighbors, X - 1, Y - 1, Z - 1);
                    VI[X][Y][Z] = OutMeshData.Vertices.Add(Pos);
                    OutMeshData.Normals.Add(ComputeVertexNormal(ChunkData, Neighbors, Pos));
                    EBlockId CB = GetBlockColorId(ChunkData, Neighbors, X - 1, Y - 1, Z - 1);
                    OutMeshData.Colors.Add(GetBlockColor(CB, 2, 0));
                    OutMeshData.UVs.Add(FVector2D((float)X, (float)Y));
                    OutMeshData.Tangents.Add(FProcMeshTangent(1.0f, 0.0f, 0.0f));
                }
            }

    auto AddQuadCopy = [&](int32 I0, int32 I1, int32 I2, int32 I3)
    {
        FVector V[4] = {
            OutMeshData.Vertices[I0], OutMeshData.Vertices[I1],
            OutMeshData.Vertices[I2], OutMeshData.Vertices[I3]
        };
        FVector N[4] = {
            OutMeshData.Normals[I0], OutMeshData.Normals[I1],
            OutMeshData.Normals[I2], OutMeshData.Normals[I3]
        };
        FColor C[4] = {
            OutMeshData.Colors[I0], OutMeshData.Colors[I1],
            OutMeshData.Colors[I2], OutMeshData.Colors[I3]
        };

        int32 B = OutMeshData.Vertices.Num();
        for (int32 i = 0; i < 4; i++)
        {
            OutMeshData.Vertices.Add(V[i]);
            OutMeshData.Normals.Add(N[i]);
            OutMeshData.Colors.Add(C[i]);
            OutMeshData.UVs.Add(FVector2D(0, 0));
            OutMeshData.Tangents.Add(FProcMeshTangent(1.0f, 0.0f, 0.0f));
        }
        OutMeshData.Triangles.Add(B + 0);
        OutMeshData.Triangles.Add(B + 1);
        OutMeshData.Triangles.Add(B + 2);
        OutMeshData.Triangles.Add(B + 0);
        OutMeshData.Triangles.Add(B + 2);
        OutMeshData.Triangles.Add(B + 3);
    };

    for (int32 Z = 0; Z < CHUNK_SIZE; Z++)
        for (int32 Y = 0; Y < CHUNK_SIZE; Y++)
            for (int32 X = 0; X < CHUNK_SIZE; X++)
            {
                int32 V000 = VI[X][Y][Z];
                int32 V100 = VI[X + 1][Y][Z];
                int32 V010 = VI[X][Y + 1][Z];
                int32 V110 = VI[X + 1][Y + 1][Z];
                int32 V001 = VI[X][Y][Z + 1];
                int32 V101 = VI[X + 1][Y][Z + 1];
                int32 V011 = VI[X][Y + 1][Z + 1];
                int32 V111 = VI[X + 1][Y + 1][Z + 1];

                if (V000 >= 0 && V010 >= 0 && V001 >= 0 && V011 >= 0)
                    AddQuadCopy(V000, V010, V011, V001);
                if (V100 >= 0 && V110 >= 0 && V101 >= 0 && V111 >= 0)
                    AddQuadCopy(V100, V101, V111, V110);
                if (V000 >= 0 && V100 >= 0 && V001 >= 0 && V101 >= 0)
                    AddQuadCopy(V000, V001, V101, V100);
                if (V010 >= 0 && V110 >= 0 && V011 >= 0 && V111 >= 0)
                    AddQuadCopy(V010, V110, V111, V011);
                if (V000 >= 0 && V100 >= 0 && V010 >= 0 && V110 >= 0)
                    AddQuadCopy(V000, V100, V110, V010);
                if (V001 >= 0 && V101 >= 0 && V011 >= 0 && V111 >= 0)
                    AddQuadCopy(V001, V011, V111, V101);
            }
}
