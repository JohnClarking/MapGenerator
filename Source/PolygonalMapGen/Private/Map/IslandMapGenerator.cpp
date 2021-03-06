// Original Work Copyright (c) 2010 Amit J Patel, Modified Work Copyright (c) 2016 Jay M Stevens

#include "PolygonalMapGen.h"
#include "IslandMapGenerator.h"

AIslandMapGenerator::AIslandMapGenerator()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.bTickEvenWhenPaused = true;
}

void AIslandMapGenerator::Tick(float deltaSeconds)
{
	Super::Tick(deltaSeconds);
	GenerateMap();
}

void AIslandMapGenerator::SetData(FIslandData islandData)
{
	IslandData = islandData;
}

void AIslandMapGenerator::ResetMap()
{
	UObject* outer = (UObject*)GetTransientPackage();

	if (IslandData.IslandType == NULL)
	{
		UE_LOG(LogWorldGen, Error, TEXT("IslandShape was null!"));
		IslandShape = NewObject<UIslandShape>(outer, UIslandShape::StaticClass());
	}
	else
	{
		IslandShape = NewObject<UIslandShape>(outer, IslandData.IslandType);
	}
	if (IslandData.IslandPointSelector == NULL)
	{
		UE_LOG(LogWorldGen, Error, TEXT("PointSelector was null!"));
		PointSelector = NewObject<UPointGenerator>(outer, UPointGenerator::StaticClass());
	}
	else
	{
		PointSelector = NewObject<UPointGenerator>(outer, IslandData.IslandPointSelector);
	}

	ElevationDistributor = NewObject<UElevationDistributor>(outer, UElevationDistributor::StaticClass());

	RandomGenerator.Initialize(IslandData.Seed);
	IslandData.GameWorld = GetWorld();
}

void AIslandMapGenerator::CreateMap(const FIslandGeneratorDelegate onComplete)
{
	OnGenerationComplete = onComplete;
	AddMapSteps();
	GenerateMap();
}

void AIslandMapGenerator::GenerateMap()
{
	if (IslandGeneratorSteps.IsEmpty() && OnGenerationComplete.IsBound())
	{
		OnGenerationComplete.Execute();
		OnGenerationComplete.Unbind();
		return;
	}

	FIslandGeneratorDelegate mapDelegate;
	IslandGeneratorSteps.Dequeue(mapDelegate);
	if (mapDelegate.IsBound())
	{
		mapDelegate.Execute();
		mapDelegate.Unbind();
	}
}

void AIslandMapGenerator::AddMapSteps_Implementation()
{
	UE_LOG(LogWorldGen, Log, TEXT("Generating a new map at %f."), GetWorld()->GetRealTimeSeconds());

	FIslandGeneratorDelegate resetMap;
	resetMap.BindDynamic(this, &AIslandMapGenerator::ResetMap);
	AddGenerationStep(resetMap);

	/*if (IslandShape == NULL || PointSelector == NULL)
	{
		UE_LOG(LogWorldGen, Error, TEXT("IslandShape or PointSelector were null!"));
		return;
	}*/

	// Initialization
	FIslandGeneratorDelegate initialization;
	initialization.BindDynamic(this, &AIslandMapGenerator::InitializeMap);
	AddGenerationStep(initialization);
	//InitializeMap();

	FIslandGeneratorDelegate buildGraph;
	buildGraph.BindDynamic(this, &AIslandMapGenerator::BuildGraph);
	AddGenerationStep(buildGraph);
	//BuildGraph();

	FIslandGeneratorDelegate assignElevation;
	assignElevation.BindDynamic(this, &AIslandMapGenerator::AssignElevation);
	AddGenerationStep(assignElevation);
	//AssignElevation();

	FIslandGeneratorDelegate assignMoisture;
	assignMoisture.BindDynamic(this, &AIslandMapGenerator::AssignMoisture);
	AddGenerationStep(assignMoisture);
	//AssignMoisture();

	// Package the map up to send to the voxel engine
	FIslandGeneratorDelegate finalizePoints;
	finalizePoints.BindDynamic(this, &AIslandMapGenerator::FinalizeAllPoints);
	AddGenerationStep(finalizePoints);
	//FinalizeAllPoints();

	FIslandGeneratorDelegate drawGraph;
	drawGraph.BindDynamic(this, &AIslandMapGenerator::DrawVoronoiGraph);
	AddGenerationStep(drawGraph);
	//MapGraph->DrawDebugVoronoiGrid(IslandData.GameWorld);
	//MapGraph->DrawDebugDelaunayGrid(IslandData.GameWorld);
	//PixelMap->DrawDebugPixelGrid(IslandData.GameWorld);
}

void AIslandMapGenerator::AddGenerationStep(const FIslandGeneratorDelegate step)
{
	IslandGeneratorSteps.Enqueue(step);
}

void AIslandMapGenerator::ClearAllGenerationSteps()
{
	IslandGeneratorSteps.Empty();
}

void AIslandMapGenerator::InitializeMap()
{
	MapGraph = NewObject<UPolygonMap>();
	IslandShape->SetSeed(IslandData.Seed, IslandData.Size);
	PointSelector->InitializeSelector(IslandData.Size, IslandData.Seed);
}

void AIslandMapGenerator::BuildGraph()
{
	// Place points
	MapGraph->CreatePoints(PointSelector, IslandData.NumberOfPoints);

	// Build graph
	MapGraph->BuildGraph(IslandData.Size, IslandData.PolygonMapSettings);
	MapGraph->ImproveCorners();
}

UPolygonMap* AIslandMapGenerator::GetGraph()
{
	return MapGraph;
}

int32 AIslandMapGenerator::GetCenterNum()
{
	return MapGraph->GetCenterNum();
}
int32 AIslandMapGenerator::GetCornerNum()
{
	return MapGraph->GetCornerNum();
}
int32 AIslandMapGenerator::GetEdgeNum()
{
	return MapGraph->GetEdgeNum();
}

FMapCenter& AIslandMapGenerator::GetCenter(const int32 index)
{
	return MapGraph->GetCenter(index);
}
FMapCorner& AIslandMapGenerator::GetCorner(const int32 index)
{
	return MapGraph->GetCorner(index);
}
FMapEdge& AIslandMapGenerator::GetEdge(const int32 index)
{
	return MapGraph->GetEdge(index);
}
FMapEdge& AIslandMapGenerator::FindEdgeFromCenters(const FMapCenter& v0, const FMapCenter& v1)
{
	return MapGraph->FindEdgeFromCenters(v0, v1);
}

FMapEdge& AIslandMapGenerator::FindEdgeFromCorners(const FMapCorner& v0, const FMapCorner& v1)
{
	return MapGraph->FindEdgeFromCorners(v0, v1);
}

void AIslandMapGenerator::UpdateCenter(const FMapCenter& center)
{
	MapGraph->UpdateCenter(center);
}
void AIslandMapGenerator::UpdateCorner(const FMapCorner& corner)
{
	MapGraph->UpdateCorner(corner);
}
void AIslandMapGenerator::UpdateEdge(const FMapEdge& edge)
{
	MapGraph->UpdateEdge(edge);
}

void AIslandMapGenerator::AssignElevation()
{
	ElevationDistributor->SetGraph(GetGraph());
	// Assign elevations
	ElevationDistributor->AssignCornerElevations(IslandShape, PointSelector->NeedsMoreRandomness(), RandomGenerator);
	// Determine polygon and corner type: ocean, coast, land.
	ElevationDistributor->AssignOceanCoastAndLand(IslandData.LakeThreshold);

	// Change the overall distribution of elevations so that lower
	// elevations are more common than higher
	// elevations. Specifically, we want elevation X to have frequency
	// (1-X).  To do this we will sort the corners, then set each
	// corner to its desired elevation.
	ElevationDistributor->RedistributeElevations(MapGraph->FindLandCorners());
	ElevationDistributor->FlattenWaterElevations();
	ElevationDistributor->AssignPolygonElevations();
}


void AIslandMapGenerator::AssignMoisture()
{
	// Moisture distribution
	// Determine downslope paths.
	CalculateDownslopes();

	// Determine watersheds: for every corner, where does it flow
	// out into the ocean? 
	CalculateWatersheds();

	// Create rivers.
	CreateRivers();

	// Determine moisture at corners, starting at rivers
	// and lakes, but not oceans. Then redistribute
	// moisture to cover the entire range evenly from 0.0
	// to 1.0. Then assign polygon moisture as the average
	// of the corner moisture.
	AssignCornerMoisture();
	RedistributeMoisture(MapGraph->FindLandCorners());
	AssignPolygonMoisture();
}

void AIslandMapGenerator::CalculateDownslopes()
{
	for (int i = 0; i < GetCornerNum(); i++)
	{
		FMapCorner corner = GetCorner(i);
		int32 downslopeIndex = corner.Index;
		for (int j = 0; j < corner.Adjacent.Num(); j++)
		{
			FMapCorner adjacent = GetCorner(corner.Adjacent[j]);
			if (adjacent.CornerData.Elevation <= GetCorner(downslopeIndex).CornerData.Elevation)
			{
				downslopeIndex = adjacent.Index;
			}
		}
		corner.Downslope = downslopeIndex;
		UpdateCorner(corner);
	}
}

void AIslandMapGenerator::CalculateWatersheds()
{
	// Initially the watershed pointer points downslope one step.  
	for (int i = 0; i < GetCornerNum(); i++)
	{
		FMapCorner corner = GetCorner(i);
		corner.Watershed = corner.Index;
		if (!corner.CornerData.bIsOcean && !corner.CornerData.bIsCoast)
		{
			corner.Watershed = corner.Downslope;
		}
		UpdateCorner(corner);
	}

	// Follow the downslope pointers to the coast. Limit to 100
	// iterations although most of the time with numPoints==2000 it
	// only takes 20 iterations because most points are not far from
	// a coast.
	for (int i = 0; i < 100; i++)
	{
		bool bChanged = false;
		for (int j = 0; j < GetCornerNum(); j++)
		{
			FMapCorner corner = GetCorner(j);
			FMapCorner watershed = GetCorner(corner.Watershed);
			if (!corner.CornerData.bIsOcean && !corner.CornerData.bIsCoast && !watershed.CornerData.bIsCoast)
			{
				FMapCorner downstreamWatershed = GetCorner(watershed.Watershed);
				if (!downstreamWatershed.CornerData.bIsOcean)
				{
					corner.Watershed = downstreamWatershed.Index;
					UpdateCorner(corner);
					bChanged = true;
				}
			}
		}
		if (!bChanged)
		{
			break;
		}
	}

	// How big is each watershed?
	for (int i = 0; i < GetCornerNum(); i++)
	{
		FMapCorner corner = GetCorner(i);
		FMapCorner watershed = GetCorner(corner.Watershed);
		watershed.WatershedSize += 1;
		UpdateCorner(watershed);
	}
}

void AIslandMapGenerator::CreateRivers()
{
	bool hasCoastTile = false;
	for (int i = 0; i < GetCornerNum(); i++)
	{
		if (GetCorner(i).CornerData.bIsCoast)
		{
			hasCoastTile = true;
			break;
		}
	}
	if (!hasCoastTile)
	{
		UE_LOG(LogWorldGen, Error, TEXT("No tiles were marked as being coastline!"));
		return;
	}

	for (int i = 0; i < IslandData.Size / 2; i++)
	{
		int cornerIndex = RandomGenerator.RandRange(0, GetCornerNum() - 1);
		FMapCorner riverSource = GetCorner(cornerIndex);
		if (riverSource.CornerData.bIsOcean || riverSource.CornerData.Tags.Contains(UPolygonMap::TAG_Volcano))
		{
			continue;
		}
		int j = 0;
		while (!riverSource.CornerData.bIsCoast)
		{
			FMapCorner downslopeCorner = GetCorner(riverSource.Downslope);
			if (downslopeCorner.Index == riverSource.Index || downslopeCorner.Index < 0)
			{
				break;
			}
			FMapEdge edge = FindEdgeFromCorners(riverSource, downslopeCorner);
			edge.RiverVolume++;
			riverSource.RiverSize++;
			riverSource.CornerData.bIsRiver = true;
			riverSource.CornerData.Elevation -= 0.05f;
			downslopeCorner.RiverSize++;
			downslopeCorner.CornerData.bIsRiver = true;
			downslopeCorner.CornerData.Elevation -= 0.05f;

			UpdateEdge(edge);
			UpdateCorner(riverSource);
			UpdateCorner(downslopeCorner);

			riverSource = downslopeCorner;
			j++;
			if (j > IslandData.NumberOfPoints) // Should never happen
			{
				break;
			}
		}
	}
}

void AIslandMapGenerator::AssignCornerMoisture()
{
	TQueue<FMapCorner> moistureQueue;
	for (int i = 0; i < GetCornerNum(); i++)
	{
		FMapCorner corner = GetCorner(i);
		if ((corner.CornerData.bIsWater || corner.CornerData.bIsRiver) && !corner.CornerData.bIsOcean)
		{
			corner.CornerData.Moisture = corner.CornerData.bIsRiver ? FMath::Min(3.0f, (0.2f * corner.RiverSize)) : 1.0f;
			moistureQueue.Enqueue(corner);
		}
		else
		{
			corner.CornerData.Moisture = 0.0f;
			UpdateCorner(corner);
		}
	}

	while (!moistureQueue.IsEmpty())
	{
		FMapCorner corner;
		moistureQueue.Dequeue(corner);
		for (int i = 0; i < corner.Adjacent.Num(); i++)
		{
			FMapCorner neighbor = GetCorner(corner.Adjacent[i]);
			float newMoisture = corner.CornerData.Moisture * 0.9f;
			if (newMoisture > neighbor.CornerData.Moisture)
			{
				neighbor.CornerData.Moisture = newMoisture;
				moistureQueue.Enqueue(neighbor);
			}
		}
		UpdateCorner(corner);
	}

	// Saltwater
	for (int i = 0; i < GetCornerNum(); i++)
	{
		FMapCorner corner = GetCorner(i);
		if (corner.CornerData.bIsCoast || corner.CornerData.bIsOcean)
		{
			corner.CornerData.Moisture = 1.0f;
			UpdateCorner(corner);
		}
	}
}

void AIslandMapGenerator::RedistributeMoisture(TArray<int32> landCorners)
{
	float maxMoisture = -1.0f;
	for (int i = 0; i < landCorners.Num(); i++)
	{
		FMapCorner corner = GetCorner(landCorners[i]);
		if (corner.CornerData.Moisture > maxMoisture)
		{
			maxMoisture = corner.CornerData.Moisture;
		}
	}

	for (int i = 0; i < landCorners.Num(); i++)
	{
		FMapCorner corner = GetCorner(landCorners[i]);
		corner.CornerData.Moisture /= maxMoisture;
		UpdateCorner(corner);
	}
}

void AIslandMapGenerator::AssignPolygonMoisture()
{
	for (int i = 0; i < GetCenterNum(); i++)
	{
		FMapCenter center = GetCenter(i);
		float sumMoisture = 0.0f;
		for (int j = 0; j < center.Corners.Num(); j++)
		{
			FMapCorner corner = GetCorner(center.Corners[j]);
			sumMoisture = corner.CornerData.Moisture;
		}
		center.CenterData.Moisture = sumMoisture / center.Corners.Num();
		UpdateCenter(center);
	}
}

void AIslandMapGenerator::FinalizeAllPoints()
{
	for (int i = 0; i < GetCornerNum(); i++)
	{
		FMapCorner corner = GetCorner(i);
		corner.CornerData.Elevation = FMath::Clamp(corner.CornerData.Elevation, 0.0f, 1.0f);
		corner.CornerData.Moisture = FMath::Clamp(corner.CornerData.Moisture, 0.0f, 1.0f);

		if (!corner.CornerData.bIsOcean)
		{
			corner.CornerData.Elevation = 0.1f + (corner.CornerData.Elevation * 0.9f);
		}
		UpdateCorner(corner);
	}

	for (int i = 0; i < GetCenterNum(); i++)
	{
		FMapCenter center = GetCenter(i);
		center.CenterData.Elevation = FMath::Clamp(center.CenterData.Elevation, 0.0f, 1.0f);
		center.CenterData.Moisture = FMath::Clamp(center.CenterData.Moisture, 0.0f, 1.0f);

		if (!center.CenterData.bIsOcean)
		{
			center.CenterData.Elevation = 0.1f + (center.CenterData.Elevation * 0.9f);
		}
		UpdateCenter(center);
	}

	// Compile to get ready to make pixels
	MapGraph->CompileMapData();
}

void AIslandMapGenerator::DrawVoronoiGraph()
{
	MapGraph->DrawDebugVoronoiGrid(IslandData.GameWorld);
}

void AIslandMapGenerator::DrawDelaunayGraph()
{
	MapGraph->DrawDebugDelaunayGrid(IslandData.GameWorld);
}
