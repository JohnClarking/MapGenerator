// Original Work Copyright (c) 2010 Amit J Patel

#include "PolygonalMapGen.h"
#include "SquarePointGenerator.h"

TArray<FVector2D> USquarePointGenerator::GeneratePoints_Implementation(int32 numberOfPoints)
{
	TArray<FVector2D> pointArray;
	int32 gridSize = FMath::RoundToInt(FMath::Sqrt((uint32)numberOfPoints));

	for (int32 x = 0; x < gridSize; x++)
	{
		for (int32 y = 0; y < gridSize; y++)
		{
			FVector2D point = FVector2D((0.5f + x) / gridSize * MapSize, (0.5f + y) / gridSize * MapSize);
			if (point.X < Border || point.Y < Border || point.X > MapSize - Border || point.Y > MapSize - Border)
			{
				continue;
			}
			pointArray.Add(point);
		}
	}
	return pointArray;
}
