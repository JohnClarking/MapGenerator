// Fill out your copyright notice in the Description page of Project Settings.

#include "PolygonalMapGeneratorPrivatePCH.h"
#include "PolygonalMapHeightmap.h"
#include "Async/Async.h"
#include "Maps/Heightmap/HeightmapPointTask.h"

// The heightmap generator
UPolygonalMapHeightmap* FHeightmapPointGenerator::MapHeightmap = NULL;
UPolygonMap* FHeightmapPointGenerator::MapGraph = NULL;
UBiomeManager* FHeightmapPointGenerator::BiomeManager = NULL;

// Results of the threads
TArray<FMapData> FHeightmapPointGenerator::HeightmapData = TArray<FMapData>();
TArray<FMapData> FHeightmapPointGenerator::StartingMapDataArray = TArray<FMapData>();

// This is the array of thread completions, used to determine if all threads are done
FGraphEventArray FHeightmapPointGenerator::CompletionEvents = FGraphEventArray();
int32 FHeightmapPointGenerator::CompletedThreads = 0;
int32 FHeightmapPointGenerator::TotalNumberOfThreads = 0;

FIslandGeneratorDelegate FHeightmapPointGenerator::OnAllPointsComplete;

bool FHeightmapPointGenerator::TasksAreComplete()
{
	//Check all thread completion events
	return CompletedThreads == TotalNumberOfThreads;
}

void FHeightmapPointGenerator::GenerateHeightmapPoints(const int32 HeightmapSize, int32 NumberOfPointsToAverage, UPolygonalMapHeightmap* HeightmapGenerator, UPolygonMap* Graph, UBiomeManager* BiomeMgr, const FIslandGeneratorDelegate OnComplete)
{
	MapHeightmap = HeightmapGenerator;
	MapGraph = Graph;
	BiomeManager = BiomeMgr;
	OnAllPointsComplete = OnComplete;

	TotalNumberOfThreads = 0;
	CompletedThreads = 0;
	HeightmapData.Empty();

	StartingMapDataArray = FHeightmapPointGenerator::MapGraph->GetAllMapData();

	EPointSelectionMode pointSelectionMode = EPointSelectionMode::InterpolatedWithPolygonBiome;

	if (pointSelectionMode == EPointSelectionMode::Interpolated || pointSelectionMode == EPointSelectionMode::InterpolatedWithPolygonBiome)
	{
		int32 graphSize = FHeightmapPointGenerator::MapGraph->GetGraphSize();
		// First, insert a border around the map
		for (int x = 0; x < graphSize; x++)
		{
			FMapData borderPoint = FMapData();
			borderPoint.Elevation = 0.0f;
			borderPoint.Moisture = 0.0f;
			borderPoint = UMapDataHelper::SetOcean(borderPoint);
			borderPoint = UMapDataHelper::SetBorder(borderPoint);
			borderPoint.Point = FVector2D(x, 0);
			StartingMapDataArray.Add(borderPoint);
			borderPoint.Point = FVector2D(x, graphSize - 1);
			StartingMapDataArray.Add(borderPoint);
		}
		for (int y = 0; y < graphSize; y++)
		{
			FMapData borderPoint = FMapData();
			borderPoint.Elevation = 0.0f;
			borderPoint.Moisture = 0.0f;
			borderPoint = UMapDataHelper::SetOcean(borderPoint);
			borderPoint = UMapDataHelper::SetBorder(borderPoint);
			borderPoint.Point = FVector2D(0, y);
			StartingMapDataArray.Add(borderPoint);
			borderPoint.Point = FVector2D(graphSize - 1, y);
			StartingMapDataArray.Add(borderPoint);
		}
	}

	// Add a task for each prime number
	for (int32 x = 0; x < HeightmapSize; x++)
	{
		for(int32 y = 0; y < HeightmapSize; y++)
		{
			CompletionEvents.Add(TGraphTask<FHeightmapPointTask>::CreateTask(NULL, ENamedThreads::GameThread).ConstructAndDispatchWhenReady(x, y, NumberOfPointsToAverage, pointSelectionMode));
			TotalNumberOfThreads++;
		}
	}
}

void FHeightmapPointGenerator::CheckComplete()
{
	if(TasksAreComplete())
	{
		FHeightmapPointGenerator::TotalNumberOfThreads = 0;
		FHeightmapPointGenerator::CompletedThreads = 0;

		UE_LOG(LogWorldGen, Log, TEXT("Heightmap is complete!"));

		// Call the delegate on the game thread
		AsyncTask(ENamedThreads::GameThread, []() {
			OnAllPointsComplete.Execute();
			OnAllPointsComplete.Unbind();
		});
	}
}

FMapData FHeightmapPointTask::MakeMapPoint(FVector2D PixelPosition, UPolygonMap* MapGraph, UBiomeManager* BiomeManager)
{
	/*if (PointSelectionMode == Interpolated || PointSelectionMode == InterpolatedWithPolygonBiome)
	{
		TArray<FMapData> closestPoints;
		// Iterate over the entire mapData array to find how many points we need to average
		for (int i = 0; i < MapData.Num(); i++)
		{
			if (closestPoints.Num() == 0)
			{
				closestPoints.Add(MapData[i]);
				continue;
			}
			float distance = FVector2D::DistSquared(PixelPosition, MapData[i].Point);
			if (distance <= 0.001f)
			{
				// Close enough
				pixelData = MapData[i];
				return pixelData;
			}

			// This will hold the index of first point we find that's further away than our point
			int addPointIndex = -1;
			for (int j = 0; j < closestPoints.Num(); j++)
			{
				// Get the distance of this point
				float pointDist = FVector2D::DistSquared(PixelPosition, closestPoints[j].Point);
				if (distance < pointDist)
				{
					addPointIndex = j;
					break;
				}
			}

			// If we found a point that's further away than our point, place it in the array and move everything else down
			if (addPointIndex >= 0)
			{
				FMapData last = MapData[i];
				for (int j = addPointIndex; j < closestPoints.Num(); j++)
				{
					FMapData temp = closestPoints[j];
					closestPoints[j] = last;
					last = temp;
				}
				// If we are below the number of points we need to add, then add the furthest point to the end
				if (closestPoints.Num() < NumberOfPointsToAverage)
				{
					closestPoints.Add(last);
				}
			}
			else if (closestPoints.Num() < NumberOfPointsToAverage)
			{
				closestPoints.Add(MapData[i]);
			}
		}

		// Cache the distances
		TArray<float> closestPointDistances;
		float totalDistance = 0.0f;
		for (int i = 0; i < closestPoints.Num(); i++)
		{
			float distance = FVector2D::DistSquared(PixelPosition, closestPoints[i].Point);
			totalDistance += distance;
			closestPointDistances.Add(distance);
		}

		float inversePercentageTotal = 0.0f;

		for (int i = 0; i < closestPoints.Num(); i++)
		{
			// Get the total percentage that this point contributed to the overall distance
			float percentageOfDistance = closestPointDistances[i] / totalDistance;

			// Take the inverse of the distance percentage -- points which are closer get a larger weight
			float inversePercentage = 1.0f - percentageOfDistance;

			// We re-add the inverse percentage to the array so we can make sure it all totals up to 1
			closestPointDistances[i] = inversePercentage;
			inversePercentageTotal += inversePercentage;
		}

		// Now gather the weighted distance for each point
		TArray<TPair<FMapData, float>> pointWeights;
		for (int i = 0; i < closestPoints.Num(); i++)
		{
			TPair<FMapData, float> weight;
			weight.Key = closestPoints[i];
			weight.Value = closestPointDistances[i] / inversePercentageTotal;
			pointWeights.Add(weight);
		}

		float elevation = 0.0f;
		float moisture = 0.0f;

		TMap<FGameplayTag, float> tagWeights;

		for (int i = 0; i < pointWeights.Num(); i++)
		{
			FMapData curPoint = pointWeights[i].Key;
			float weight = pointWeights[i].Value;

			elevation += (curPoint.Elevation * weight);
			moisture += (curPoint.Moisture * weight);

			if(PointSelectionMode == EPointSelectionMode::Interpolated)
			{
				for (int j = 0; j < curPoint.Tags.Num(); j++)
				{
					FGameplayTag tag = curPoint.Tags.GetByIndex(i);
					if (tag.MatchesTag(FGameplayTag::RequestGameplayTag(TEXT("MapData.MetaData.Water.River"))))
					{
						// Rivers are handled later
						continue;
					}
					float currentTagWeight = tagWeights.FindOrAdd(tag);
					currentTagWeight += weight;
					tagWeights[tag] = currentTagWeight;
				}
			}
		}
		pixelData.Elevation = elevation;
		pixelData.Moisture = moisture;
		pixelData.Point = PixelPosition;
		if (PointSelectionMode == EPointSelectionMode::Interpolated)
		{
			pixelData.Tags.Reset();
			for (auto& elem : tagWeights)
			{
				if (elem.Value >= 0.5f)
				{
					pixelData.Tags.AddTagFast(elem.Key);
				}
			}
			// Right now, this sometimes causes a crash
			// TODO: Find out why it crashes (maybe due to multithreading?)
			// In the meantime, use EPointSelectionMode::InterpolatedWithPolygonBiome instead
			pixelData.Biome = BiomeManager->DetermineBiome(pixelData);
		}
		else if(PointSelectionMode == EPointSelectionMode::InterpolatedWithPolygonBiome)
		{
			FMapCenter center = FHeightmapPointGenerator::MapGraph->FindMapCenterForCoordinate(PixelPosition);
			if (center.Index < 0)
			{
				//UE_LOG(LogWorldGen, Warning, TEXT("Could not find polygon! Returning what we have."));
				return pixelData;
			}
			pixelData.Tags = center.CenterData.Tags;
			pixelData.Biome = center.CenterData.Biome;
		}
		return pixelData;
	}
	else if(PointSelectionMode == EPointSelectionMode::UsePolygon)
	{
		FMapCenter center = FHeightmapPointGenerator::MapGraph->FindMapCenterForCoordinate(PixelPosition);
		if (center.Index < 0)
		{
			//UE_LOG(LogWorldGen, Warning, TEXT("Could not find polygon! Returning blank FMapData!"));
			return FMapData();
		}

		pixelData = center.CenterData;
		pixelData.Point = PixelPosition;
		return pixelData;
	}
	else
	{
		// Should never get to this point
		unimplemented();
		return FMapData();
	}*/

	FMapData pixelData = FMapData();
	pixelData.Point = PixelPosition;

	FMapCorner triangleCenter;
	float pointZPostion = MapGraph->CalculateZPosition(PixelPosition, triangleCenter);
	if (triangleCenter.Index >= 0)
	{
		// The point is valid, populate from the triangle
		pixelData.Elevation = pointZPostion;
		if (PointSelectionMode == EPointSelectionMode::UsePolygon)
		{
			pixelData.Moisture = triangleCenter.CornerData.Moisture;
			pixelData.Tags = triangleCenter.CornerData.Tags;
			pixelData.Biome = triangleCenter.CornerData.Biome;
		}
		else
		{
			pixelData.Moisture = MapGraph->InterpolateMapDataMoisture(MapGraph->GetCenter(triangleCenter.Touches[0]).CenterData, MapGraph->GetCenter(triangleCenter.Touches[1]).CenterData, MapGraph->GetCenter(triangleCenter.Touches[2]).CenterData, PixelPosition);
			// TODO: Interpolate tags
			pixelData.Tags = triangleCenter.CornerData.Tags;
			if (PointSelectionMode == EPointSelectionMode::InterpolatedWithPolygonBiome)
			{
				// Grab the biome directly from the CornerData
				pixelData.Biome = triangleCenter.CornerData.Biome;
			}
			else
			{
				// Right now, this sometimes causes a crash
				// TODO: Find out why it crashes (maybe due to multithreading?)
				// In the meantime, use EPointSelectionMode::InterpolatedWithPolygonBiome instead
				pixelData.Biome = BiomeManager->DetermineBiome(pixelData);
			}
		}
	}
	else
	{
		// If the point is invalid, the default constructor for the MapData struct is
		// sufficient for making an ocean pixel. We just need to set the biome.
		pixelData.Biome = FGameplayTag::RequestGameplayTag(TEXT("MapData.Biome.Water.Ocean"));
	}
	return pixelData;
}

void FHeightmapPointTask::DoTask(ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent)
{
	FVector2D point = FVector2D(X, Y);

	// Now make the actual map point
	FMapData mapData = MakeMapPoint(point, FHeightmapPointGenerator::MapGraph, FHeightmapPointGenerator::BiomeManager);
	FHeightmapPointGenerator::HeightmapData.Add(mapData);
	FHeightmapPointGenerator::CompletedThreads++;

	float percentComplete = (float)FHeightmapPointGenerator::CompletedThreads / (float)FHeightmapPointGenerator::TotalNumberOfThreads;
	UE_LOG(LogWorldGen, Log, TEXT("Heightmap completion percent: %f percent."), percentComplete);
	if (FHeightmapPointGenerator::CompletedThreads == FHeightmapPointGenerator::TotalNumberOfThreads)
	{
		// If we're all done, check in with the on completion delegate
		FHeightmapPointGenerator::CheckComplete();
	}
}
