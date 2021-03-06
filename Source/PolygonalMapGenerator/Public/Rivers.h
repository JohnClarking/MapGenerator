/*
* From http://www.redblobgames.com/maps/mapgen2/
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
*/

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PolygonalMapGenerator.h"
#include "TriangleDualMesh.h"
#include "Rivers.generated.h"

/**
 * 
 */
UCLASS()
class POLYGONALMAPGENERATOR_API URivers : public UDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MinSpringElevation;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MaxSpringElevation;

public:
	URivers();

private:
	/**
	* Is this triangle water?
	*/
	bool t_water(FTriangleIndex t, UTriangleDualMesh* Mesh, const TArray<bool>& r_water) const;

public:
	/**
	* Find candidates for river sources
	*
	* Unlike the assign_* functions this does not write into an existing array
	*/
	TArray<FTriangleIndex> find_spring_t(UTriangleDualMesh* Mesh, const TArray<bool>& r_water, const TArray<float>& t_elevation, const TArray<FSideIndex>& t_downslope_s) const;
	void assign_s_flow(TArray<int32>& s_flow, UTriangleDualMesh* Mesh, const TArray<FSideIndex>& t_downslope_s, const TArray<FTriangleIndex>& river_t) const;
};
