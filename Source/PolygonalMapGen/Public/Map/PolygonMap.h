// Original Work Copyright (c) 2010 Amit J Patel, Modified Work Copyright (c) 2016 Jay M Stevens

#pragma once

#include "Object.h"
#include "PolygonalMapGen/Private/Map/PointGenerator/PointGenerator.h"
#include "PolygonMap.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogWorldGen, Log, All);

USTRUCT(BlueprintType)
struct FMapData
{
	GENERATED_BODY()

	// Location in the map itself
	UPROPERTY(Category = "Map Graph", BlueprintReadWrite, EditAnywhere)
		FVector2D Point;
	// Is this MapData on the map's border?
	UPROPERTY(Category = "Map Graph", BlueprintReadWrite, EditAnywhere)
		bool bIsBorder;

	// Is this MapData a lake or ocean?
	UPROPERTY(Category = "Map Biome", BlueprintReadWrite, EditAnywhere)
		bool bIsWater;
	// Is the MapData in the ocean?
	UPROPERTY(Category = "Map Biome", BlueprintReadWrite, EditAnywhere)
		bool bIsOcean;
	// Is this MapData a land polygon touching the ocean?
	UPROPERTY(Category = "Map Biome", BlueprintReadWrite, EditAnywhere)
		bool bIsCoast;
	// Is this MapData a river?
	UPROPERTY(Category = "Map Biome", BlueprintReadWrite, EditAnywhere)
		bool bIsRiver;
	// The height of this MapData (0.0 - 1.0)
	UPROPERTY(Category = "Map Biome", BlueprintReadWrite, EditAnywhere)
		float Elevation;
	// How much moisture this MapData recieves (0.0 - 1.0)
	UPROPERTY(Category = "Map Biome", BlueprintReadWrite, EditAnywhere)
		float Moisture;

	UPROPERTY(Category = "Map Biome", BlueprintReadWrite, EditAnywhere)
		TArray<FName> Tags;

	FMapData()
	{
		Point = FVector2D::ZeroVector;

		bIsBorder = true;
		bIsOcean = true;
		bIsWater = true;

		bIsCoast = false;
		bIsRiver = false;
		Elevation = 0.0f;
		Moisture = 0.0f;
	}
};

struct FMapCorner;
struct FMapEdge;

USTRUCT(BlueprintType)
struct FMapCenter
{
	GENERATED_BODY()
	// The index of this Center
	UPROPERTY(Category = "Map Graph", BlueprintReadWrite, VisibleAnywhere)
	int32 Index;

	UPROPERTY(Category = "Map Graph", BlueprintReadWrite, EditAnywhere)
	FMapData CenterData;

	// The index of all neighboring Centers
	UPROPERTY(Category = "Map Graph", BlueprintReadOnly, VisibleAnywhere)
	TArray<int32> Neighbors;
	// The index of all Edges that make up our borders
	UPROPERTY(Category = "Map Graph", BlueprintReadOnly, VisibleAnywhere)
	TArray<int32> Borders;
	// The index of all Corners of our polygon
	UPROPERTY(Category = "Map Graph", BlueprintReadOnly, VisibleAnywhere)
	TArray<int32> Corners;

	FMapCenter()
	{
		Index = -1;
	}
};

USTRUCT(BlueprintType)
struct FMapCorner
{
	GENERATED_BODY()
	// The index of this Corner
	UPROPERTY(Category = "Map Graph", BlueprintReadWrite, VisibleAnywhere)
	int32 Index;

	UPROPERTY(Category = "Map Graph", BlueprintReadWrite, EditAnywhere)
	FMapData CornerData;

	// The index of all Centers that touch this Corner
	UPROPERTY(Category = "Map Graph", BlueprintReadOnly, VisibleAnywhere)
	TArray<int32> Touches;
	// The index of all Edges that have one end terminating at this Corner
	UPROPERTY(Category = "Map Graph", BlueprintReadOnly, VisibleAnywhere)
	TArray<int32> Protrudes;
	// The index of all Corners adjacent to this Corner
	UPROPERTY(Category = "Map Graph", BlueprintReadOnly, VisibleAnywhere)
	TArray<int32> Adjacent;

	// The size of the river flowing through this corner.
	// If there is no river, this is 0.
	UPROPERTY(Category = "Map Graph", BlueprintReadWrite, EditAnywhere)
	int32 RiverSize;
	// Pointer to adjacent Corner most downhill from us, or -1
	UPROPERTY(Category = "Map Graph", BlueprintReadWrite, EditAnywhere)
	int32 Downslope;
	// Pointer to coastal Corner, or -1 for null
	//UPROPERTY(Category = "Map Graph", BlueprintReadWrite, EditAnywhere)
	int32 Watershed;
	// The size of the watershed
	UPROPERTY(Category = "Map Graph", BlueprintReadWrite, EditAnywhere)
	int32 WatershedSize;

	FMapCorner()
	{
		Index = -1;
		RiverSize = 0;
		Downslope = -1;
		Watershed = -1;
		WatershedSize = 0;
	}

	FORCEINLINE bool operator <(const FMapCorner& c) const
	{
		return CornerData.Elevation < c.CornerData.Elevation;
	}
};

USTRUCT(BlueprintType)
struct FMapEdge
{
	GENERATED_BODY()
	// The index of this Edge
	UPROPERTY(Category = "Map Graph", BlueprintReadWrite, VisibleAnywhere)
	int32 Index;

	// The Delaunay Edge points to a center.
	UPROPERTY(Category = "Map Graph", BlueprintReadOnly, VisibleAnywhere)
	int32 DelaunayEdge0;
	// The Delaunay Edge points to a center.
	UPROPERTY(Category = "Map Graph", BlueprintReadOnly, VisibleAnywhere)
	int32 DelaunayEdge1;

	// The Voronoi Edge points to a corner.
	UPROPERTY(Category = "Map Graph", BlueprintReadOnly, VisibleAnywhere)
	int32 VoronoiEdge0;
	// The Voronoi Edge points to a corner.
	UPROPERTY(Category = "Map Graph", BlueprintReadOnly, VisibleAnywhere)
	int32 VoronoiEdge1;

	// The midway pont between our two voronoi edges
	UPROPERTY(Category = "Map Graph", BlueprintReadOnly, VisibleAnywhere)
	FVector2D Midpoint;

	// The volume of the river flowing through this edge, or 0
	UPROPERTY(Category = "Map Biome", BlueprintReadWrite, EditAnywhere)
	int32 RiverVolume;

	//Constructor
	FMapEdge()
	{
		Index = -1;
		DelaunayEdge0 = -1;
		DelaunayEdge1 = -1;
		VoronoiEdge0 = -1;
		VoronoiEdge1 = -1;

		RiverVolume = 0;
	}
};

USTRUCT(BlueprintType)
struct FPolygonMapData
{
	GENERATED_BODY()

	// The size each debug point is rendered
	UPROPERTY(Category = "Map", BlueprintReadWrite, EditAnywhere)
	int32 PointSize;
	// The Z offset of the bottom of the ocean -- what's the lowest
	// point on the entire map?
	UPROPERTY(Category = "Map", BlueprintReadWrite, EditAnywhere)
	float ElevationOffset;
	// How much to scale the map on the XY scale.
	UPROPERTY(Category = "Map", BlueprintReadWrite, EditAnywhere)
	float XYScaleFactor;
	// How tall the map should be -- how much higher is the highest
	// point in the map from the lowest point?
	UPROPERTY(Category = "Map", BlueprintReadWrite, EditAnywhere)
	float ElevationScale;

	FPolygonMapData()
	{
		PointSize = 256;
		ElevationOffset = -6350.0f;
		XYScaleFactor = 102.4f;
		ElevationScale = 3200.0f;
	}
};

/**
 * The PolygonMap is a class which uses a Voronoi diagram to collect data about a graph of 
 * points on the XY plane.
 * These points are separated into map 'centers' (the points of the Delaunay 'dual graph'),
 * map 'corners' (the points of the Voronoi graph), and map 'edges' (which make up the
 * connections between Voronoi graphs and Delaunay graphs).
 * Map centers and map corners both have metadata associated with them in the form of a 
 * 'MapData' class, which is used in the actual MapGenerator.
 */
UCLASS(Blueprintable)
class UPolygonMap : public UObject
{
	GENERATED_BODY()
	
public:
	// Creates the graph's initial points
	UFUNCTION(BlueprintCallable, Category = "Island Generation|Graph")
	void CreatePoints(UPointGenerator* PointSelector, const int32& numberOfPoints);

	// Build graph data structure in 'edges', 'centers', 'corners',
	// based on information in the Voronoi results: point.neighbors
	// will be a list of neighboring points of the same type (corner
	// or center); point.edges will be a list of edges that include
	// that point. Each edge connects to four points: the Voronoi edge
	// edge.{voronoiEdge0,voronoiEdge1} and its dual Delaunay triangle 
	// edge edge.{delaunayEdge0,delaunayEdge1}.0 For boundary
	// polygons, the Delaunay edge will have one null point, and the 
	// Voronoi edge may be null.
	UFUNCTION(BlueprintCallable, Category = "Island Generation|Graph")
	void BuildGraph(const int32& mapSize, const FPolygonMapData& data);

	UFUNCTION(BlueprintCallable, Category = "Island Generation|Graph")
	FMapCenter& MakeCenter(const FVector2D& point);
	UFUNCTION(BlueprintCallable, Category = "Island Generation|Graph")
	FMapCorner& MakeCorner(const FVector2D& point);

	// Although Lloyd relaxation improves the uniformity of polygon
	// sizes, it doesn't help with the edge lengths. Short edges can
	// be bad for some games, and lead to weird artifacts on
	// rivers. We can easily lengthen short edges by moving the
	// corners, but **we lose the Voronoi property**.  The corners are
	// moved to the average of the polygon centers around them. Short
	// edges become longer. Long edges tend to become shorter. The
	// polygons tend to be more uniform after this step.
	UFUNCTION(BlueprintCallable, Category = "Island Generation|Graph")
	void ImproveCorners();

	// Returns a MapCenter at the given index.
	// If the index is invalid, an empty MapCenter will be returned.
	// This empty MapCenter will have an index of -1.
	// Make sure to use the function UpdateCenter() to update the graph
	// after the MapCenter has been modified. 
	UFUNCTION(BlueprintPure, Category = "Island Generation|Graph")
	FMapCenter& GetCenter(const int32& index);
	// Returns a MapCorner at the given index.
	// If the index is invalid, an empty MapCorner will be returned.
	// This empty MapCorner will have an index of -1.
	// Make sure to use the function UpdateCorner() to update the graph
	// after the MapCorner has been modified. 
	UFUNCTION(BlueprintPure, Category = "Island Generation|Graph")
	FMapCorner& GetCorner(const int32& index);
	// Returns a MapEdge at the given index.
	// If the index is invalid, an empty MapEdge will be returned.
	// This empty MapEdge will have an index of -1.
	// Make sure to use the function UpdateEdge() to update the graph
	// after the MapEdge has been modified. 
	UFUNCTION(BlueprintPure, Category = "Island Generation|Graph")
	FMapEdge& GetEdge(const int32& index);

	// Update a MapCenter in the graph after it has been modified.
	UFUNCTION(BlueprintCallable, Category = "Island Generation|Graph")
	void UpdateCenter(const FMapCenter& center);
	// Update a MapCorner in the graph after it has been modified.
	UFUNCTION(BlueprintCallable, Category = "Island Generation|Graph")
	void UpdateCorner(const FMapCorner& corner);
	// Update a MapEdge in the graph after it has been modified.
	UFUNCTION(BlueprintCallable, Category = "Island Generation|Graph")
	void UpdateEdge(const FMapEdge& edge);

	// Returns the number of MapCenters in our graph.
	UFUNCTION(BlueprintPure, Category = "Island Generation|Graph")
	int32 GetCenterNum();
	// Returns the number of MapCorners in our graph.
	UFUNCTION(BlueprintPure, Category = "Island Generation|Graph")
	int32 GetCornerNum();
	// Returns the number of MapEdges in our graph.
	UFUNCTION(BlueprintPure, Category = "Island Generation|Graph")
	int32 GetEdgeNum();

	// Returns the size of our graph.
	UFUNCTION(BlueprintPure, Category = "Island Generation|Graph")
	int32 GetGraphSize();


	// Returns ALL MapData points in our graph.
	// This is like returning a collection of all MapCenters and MapCorners.
	// Note that you cannot determine from a MapData object alone whether it
	// came from a MapCenter or a MapCorner, and that because of this, the
	// MapData object's "Index" property now means nothing. 2 different
	// MapData objects may share the same indicies.
	UFUNCTION(BlueprintPure, Category = "Island Generation|MapData")
	TArray<FMapData>& GetAllMapData();

	// Gets a list of all MapCorners which are not marked as ocean.
	UFUNCTION(BlueprintPure, Category = "Island Generation|Graph")
	TArray<int32> FindLandCorners();

	// Returns the MapEdge defined by two adjacent MapCenters.
	// If the edge doesn't exist, an empty MapEdge will be returned.
	// This empty MapEdge will have an index of -1.
	// Make sure to use the function UpdateEdge() to update the graph
	// after the MapEdge has been modified.
	UFUNCTION(BlueprintPure, Category = "Island Generation|Graph")
	FMapEdge& FindEdgeFromCenters(const FMapCenter& v0, const FMapCenter& v1);
	// Returns the MapEdge defined by two adjacent MapCorners.
	// If the edge doesn't exist, an empty MapEdge will be returned.
	// This empty MapEdge will have an index of -1.
	// Make sure to use the function UpdateEdge() to update the graph
	// after the MapEdge has been modified.
	UFUNCTION(BlueprintPure, Category = "Island Generation|Graph")
	FMapEdge& FindEdgeFromCorners(const FMapCorner& v0, const FMapCorner& v1);

	// Compiles all map data into the CachedMapData array
	// This is used by the PixelMap to create pixels
	UFUNCTION(BlueprintCallable, Category = "Island Generation|Graph")
	void CompileMapData();

	// Converts a MapData object into the point it represents in 3D world space.
	// This can be used to mark where rivers are, to place props, etc.
	UFUNCTION(BlueprintPure, Category = "Island Generation")
	FVector ConvertGraphPointToWorldSpace(const FMapData& mapData);

	// Draws a 3D version of the underlying Delaunay graph in world space.
	// This graph is a collection of all the map's centers and the
	// Delaunay edges which connect them.
	UFUNCTION(BlueprintCallable, Category = "Island Generation|Graph")
	void DrawDebugDelaunayGrid(const UWorld* world);
	// Draws a 3D version of the underlying Voronoi graph in world space.
	// This graph is a collection of all the map's corners and the
	// Voronoi edges which connect them.
	UFUNCTION(BlueprintCallable, Category = "Island Generation|Graph")
	void DrawDebugVoronoiGrid(const UWorld* world);

	// Tags
	static const FName TAG_Volcano;
	static const FName TAG_VolcanoCaldera;

private:
	UPROPERTY()
	UPointGenerator* PointSelector;
	UPROPERTY()
	int32 MapSize;

	UPROPERTY()
	FPolygonMapData MapData;

	/// Graph Data
	// The points in our graph
	UPROPERTY()
	TArray<FVector2D> Points;
	// The Centers in our graph
	UPROPERTY()
	TArray<FMapCenter> Centers;
	// The Corners in our graph
	UPROPERTY()
	TArray<FMapCorner> Corners;
	// The Edges of our graph
	UPROPERTY()
	TArray<FMapEdge> Edges;

	UPROPERTY()
	TArray<FMapData> CachedMapData;

	TMap<FVector2D, int32> CenterLookup;
	TMap<FVector2D, int32> CornerLookup;

	UPROPERTY()
	FMapCenter emptyCenter;
	UPROPERTY()
	FMapCorner emptyCorner;
	UPROPERTY()
	FMapEdge emptyEdge;
};
