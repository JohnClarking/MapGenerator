// Original Work Copyright (c) 2010 Amit J Patel, Modified Work Copyright (c) 2016 Jay M Stevens

#include "PolygonalMapGen.h"
#include "IslandShape.h"

void UIslandShape::SetSeed(int32 seed, int32 size)
{
	StreamRNG.Initialize(seed);
	Size = size;
}

// Default does nothing
bool UIslandShape::IsPointLand(FVector2D point)
{
	return false;
}
