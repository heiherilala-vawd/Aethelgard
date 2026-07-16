// Copyright Epic Games, Inc. All Rights Reserved.

#include "Terrain/GreedyMeshGenerator.h"

void UGreedyMeshGenerator::GenerateMesh(
    const FChunkData& ChunkData,
    const TMap<FIntVector, TSharedPtr<FChunkData>>& Neighbors,
    FMeshSectionData& OutMeshData)
{
    OutMeshData.Reset();

    for (int32 Axis = 0; Axis < 3; Axis++)
    {
        for (int32 Direction = -1; Direction <= 1; Direction += 2)
        {
            for (int32 Layer = 0; Layer < CHUNK_SIZE; Layer++)
            {
                ProcessSlice(ChunkData, Neighbors, Axis, Direction, Layer, OutMeshData);
            }
        }
    }
}

void UGreedyMeshGenerator::ProcessSlice(
    const FChunkData& ChunkData,
    const TMap<FIntVector, TSharedPtr<FChunkData>>& Neighbors,
    int32 Axis,
    int32 Direction,
    int32 Layer,
    FMeshSectionData& OutMeshData)
{
    int32 Axis1 = (Axis + 1) % 3;
    int32 Axis2 = (Axis + 2) % 3;

    uint8 Mask[CHUNK_SIZE][CHUNK_SIZE];
    EBlockId BlockTypes[CHUNK_SIZE][CHUNK_SIZE];
    FMemory::Memzero(Mask, sizeof(Mask));

    for (int32 U = 0; U < CHUNK_SIZE; U++)
    {
        for (int32 V = 0; V < CHUNK_SIZE; V++)
        {
            int32 Pos[3] = {};
            Pos[Axis] = Layer;
            Pos[Axis1] = U;
            Pos[Axis2] = V;

            EBlockId Block = GetBlockAt(ChunkData, Neighbors, Pos[0], Pos[1], Pos[2]);
            if (Block == EBlockId::Air || Block >= EBlockId::MAX)
                continue;

            int32 NeighborPos[3] = { Pos[0], Pos[1], Pos[2] };
            NeighborPos[Axis] += Direction;

            EBlockId Neighbor = GetBlockAt(ChunkData, Neighbors, NeighborPos[0], NeighborPos[1], NeighborPos[2]);

            if (Neighbor == EBlockId::Air)
            {
                Mask[V][U] = 1;
                BlockTypes[V][U] = Block;
            }
        }
    }

    for (int32 V = 0; V < CHUNK_SIZE; V++)
    {
        for (int32 U = 0; U < CHUNK_SIZE; U++)
        {
            if (!Mask[V][U])
                continue;

            EBlockId CurrentBlock = BlockTypes[V][U];
            FColor FaceColor = GetBlockColor(CurrentBlock, Axis, Direction);

            int32 Width = 1;
            while (U + Width < CHUNK_SIZE && Mask[V][U + Width] && BlockTypes[V][U + Width] == CurrentBlock)
                Width++;

            int32 Height = 1;
            bool bCanExpand = true;
            while (V + Height < CHUNK_SIZE && bCanExpand)
            {
                for (int32 DU = 0; DU < Width; DU++)
                {
                    if (!Mask[V + Height][U + DU] || BlockTypes[V + Height][U + DU] != CurrentBlock)
                    {
                        bCanExpand = false;
                        break;
                    }
                }
                if (bCanExpand)
                    Height++;
            }

            for (int32 DV = 0; DV < Height; DV++)
                for (int32 DU = 0; DU < Width; DU++)
                    Mask[V + DV][U + DU] = 0;

            AddQuad(OutMeshData, FaceColor, Axis, Direction, Layer, U, V, Width, Height);
        }
    }
}

EBlockId UGreedyMeshGenerator::GetBlockAt(
    const FChunkData& CenterChunk,
    const TMap<FIntVector, TSharedPtr<FChunkData>>& Neighbors,
    int32 X, int32 Y, int32 Z) const
{
    if (X >= 0 && X < CHUNK_SIZE && Y >= 0 && Y < CHUNK_SIZE && Z >= 0 && Z < CHUNK_SIZE)
        return CenterChunk.GetBlock(X, Y, Z);

    int32 WX = CenterChunk.Position.X * CHUNK_SIZE + X;
    int32 WY = CenterChunk.Position.Y * CHUNK_SIZE + Y;
    int32 WZ = CenterChunk.Position.Z * CHUNK_SIZE + Z;

    FIntVector NeighborCoord(
        FMath::FloorToInt((float)WX / CHUNK_SIZE),
        FMath::FloorToInt((float)WY / CHUNK_SIZE),
        FMath::FloorToInt((float)WZ / CHUNK_SIZE)
    );

    if (NeighborCoord == CenterChunk.Position)
        return CenterChunk.GetBlock(X, Y, Z);

    const TSharedPtr<FChunkData>* NeighborPtr = Neighbors.Find(NeighborCoord);
    if (!NeighborPtr || !NeighborPtr->IsValid() || !(*NeighborPtr)->bIsGenerated)
        return EBlockId::Air;

    const FChunkData& NeighborChunk = *NeighborPtr->Get();
    int32 LX = WX - NeighborCoord.X * CHUNK_SIZE;
    int32 LY = WY - NeighborCoord.Y * CHUNK_SIZE;
    int32 LZ = WZ - NeighborCoord.Z * CHUNK_SIZE;

    return NeighborChunk.GetBlock(LX, LY, LZ);
}

void UGreedyMeshGenerator::AddQuad(
    FMeshSectionData& MeshData,
    FColor Color,
    int32 Axis,
    int32 Direction,
    int32 Layer,
    int32 U, int32 V,
    int32 Width, int32 Height)
{
    int32 BaseIndex = MeshData.Vertices.Num();

    int32 Axis1 = (Axis + 1) % 3;
    int32 Axis2 = (Axis + 2) % 3;

    FVector Normal(0, 0, 0);
    Normal[Axis] = (float)Direction;

    FVector V0, V1, V2, V3;

    V0[Axis] = (float)Layer;
    V0[Axis1] = (float)U;
    V0[Axis2] = (float)V;

    V1[Axis] = (float)Layer;
    V1[Axis1] = (float)(U + Width);
    V1[Axis2] = (float)V;

    V2[Axis] = (float)Layer;
    V2[Axis1] = (float)(U + Width);
    V2[Axis2] = (float)(V + Height);

    V3[Axis] = (float)Layer;
    V3[Axis1] = (float)U;
    V3[Axis2] = (float)(V + Height);

    FVector2D UV0(0, 0);
    FVector2D UV1((float)Width, 0);
    FVector2D UV2((float)Width, (float)Height);
    FVector2D UV3(0, (float)Height);

    FProcMeshTangent Tangent(1.0f, 0.0f, 0.0f);

    if (Direction > 0)
    {
        MeshData.Vertices.Add(V0);
        MeshData.Vertices.Add(V1);
        MeshData.Vertices.Add(V2);
        MeshData.Vertices.Add(V3);

        MeshData.Triangles.Add(BaseIndex + 0);
        MeshData.Triangles.Add(BaseIndex + 1);
        MeshData.Triangles.Add(BaseIndex + 2);
        MeshData.Triangles.Add(BaseIndex + 0);
        MeshData.Triangles.Add(BaseIndex + 2);
        MeshData.Triangles.Add(BaseIndex + 3);

        MeshData.UVs.Add(UV0);
        MeshData.UVs.Add(UV1);
        MeshData.UVs.Add(UV2);
        MeshData.UVs.Add(UV3);
    }
    else
    {
        MeshData.Vertices.Add(V3);
        MeshData.Vertices.Add(V2);
        MeshData.Vertices.Add(V1);
        MeshData.Vertices.Add(V0);

        MeshData.Triangles.Add(BaseIndex + 0);
        MeshData.Triangles.Add(BaseIndex + 1);
        MeshData.Triangles.Add(BaseIndex + 2);
        MeshData.Triangles.Add(BaseIndex + 0);
        MeshData.Triangles.Add(BaseIndex + 2);
        MeshData.Triangles.Add(BaseIndex + 3);

        MeshData.UVs.Add(UV0);
        MeshData.UVs.Add(UV1);
        MeshData.UVs.Add(UV2);
        MeshData.UVs.Add(UV3);
    }

    for (int32 i = 0; i < 4; i++)
    {
        MeshData.Normals.Add(Normal);
        MeshData.Colors.Add(Color);
        MeshData.Tangents.Add(Tangent);
    }
}
