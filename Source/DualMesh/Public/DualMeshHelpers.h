/*
* Based on https://github.com/redblobgames/dual-mesh
* Original work copyright 2017 Red Blob Games <redblobgames@gmail.com>
* Unreal Engine 4 implementation copyright 2018 Jay Stevens <jaystevens42@gmail.com>
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
* Unreal Engine 4 Dual Mesh implementation.
*/

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "DelaunayHelper.h"
#include "DualMeshHelpers.generated.h"

USTRUCT(BlueprintType)
struct DUALMESH_API FDualMesh : public FDelaunayMesh
{
	GENERATED_BODY()
public:
	FVector2D MaxSize;

	int32 NumSolidSides;

public:
	FDualMesh()
		: FDelaunayMesh()
	{
		HullStart = -1;
		NumSolidSides = -1;
		MaxSize = FVector2D::ZeroVector;
	}

	FDualMesh(const TArray<FVector2D>& GivenPoints, const FVector2D& MaxMapSize);
private:
	void AddGhostStructure();
};

/**
 * 
 */
UCLASS()
class DUALMESH_API UDualMeshHelpers : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
};