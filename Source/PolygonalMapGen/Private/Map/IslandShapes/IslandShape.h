// Original Work Copyright (c) 2010 Amit J Patel, Modified Work Copyright (c) 2016 Jay M Stevens

#pragma once

#include "Object.h"
#include "IslandShape.generated.h"

/**
 * The IslandShape class is a class which takes a 2D point and
 * returns a value indicating whether the point is land or water.
 */
UCLASS(Blueprintable)
class UIslandShape : public UObject
{
	GENERATED_BODY()
public:
	UIslandShape() {};
	~UIslandShape() {};

	// Sets the seed and the size of the island.
	// Will be called before any IsPointLand calls.
	UFUNCTION(BlueprintNativeEvent, Category = "Island Generation|Graph|Points")
	void SetSeed(int32 seed, int32 size);

	// Whether the given point is land or not.
	// By default, all points will be considered water.
	UFUNCTION(BlueprintNativeEvent, Category = "Island Generation|Graph|Points")
	bool IsPointLand(FVector2D point);

	// 1.0 means no small islands; 2.0 leads to a lot
	UPROPERTY(Category = "Island Size", BlueprintReadWrite, EditAnywhere)
	float IslandFactor = 1.07f;

protected:
	// C++ version of SetSeed
	virtual void SetSeed_Implementation(int32 seed, int32 size);
	// C++ verion of IsPointLand
	virtual bool IsPointLand_Implementation(FVector2D point);

	// The RandomStream that has been initialized with our seed.
	// Will be used if any RNG is needed.
	UPROPERTY(Category = "Island Randomness", BlueprintReadWrite, EditAnywhere)
	FRandomStream StreamRNG;
	// The size of our island.
	UPROPERTY(Category = "Island Size", BlueprintReadWrite, EditAnywhere)
	int32 Size;
};
