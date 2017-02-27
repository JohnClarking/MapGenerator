// Fill out your copyright notice in the Description page of Project Settings.

#include "PolygonalMapGen.h"
#include "PointGenerator.h"

void UPointGenerator::InitializeSelector(int32 mapSize, int32 seed, int32 border)
{
	MapSize = mapSize;
	RandomGenerator.Initialize(seed);
	Border = border;
}

bool UPointGenerator::NeedsMoreRandomness()
{
	return bNeedsMoreRandomness;
}

bool UPointGenerator::PointIsOnBorder(FVector2D point)
{
	if (point.X <= (Border * 2) || point.Y <= (Border * 2))
	{
		return true;
	}
	if (point.X >= MapSize - (Border * 2) || point.Y >= MapSize - (Border * 2))
	{
		return true;
	}
	return false;
}

TArray<FVector2D> UPointGenerator::GeneratePoints(int32 numberOfPoints)
{
	TArray<FVector2D> pointArray;
	pointArray.AddDefaulted(numberOfPoints);
	return pointArray;
}
