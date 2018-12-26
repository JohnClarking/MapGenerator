#Polygonal Map Generator

This is a port of [the ActionScript code](https://github.com/amitp/mapgen2) of [Red Blob Games' *Polygonal Map Generation for Games*](http://www-cs-students.stanford.edu/~amitp/game-programming/polygon-map-generation/). This port is written in C++ and designed to work in Unreal Engine 4 using the Unreal Engine 4 "Plugin" system. This port works fully with games written in C++ as well as games that use Unreal Engine 4's "Blueprint" system.

The original ActionScript code was released under the MIT Open Source license; this C++ port of the code is also released under the MIT Open Source license.

This project also uses [Bl4ckb0ne's C++ Delaunay Triangulation algorithm](https://github.com/Bl4ckb0ne/delaunay-triangulation), modified slightly to work with the Unreal Engine. This code is also used under the MIT License. Conversion from Delaunay to Voronoi is (very loosely) based off of [Joseph Marshall's voronoi-c++ code](https://bitbucket.org/jlm/voronoi-c/src/b06aa9cccba6392d28ad7d7cae9a7361efb22c94?at=default), also modified under an MIT License.

#Installation

First, make a `Plugins` folder at your project root (where the .uproject file is), if you haven't already. Then, clone this project into a subfolder in your Plugins directory. After that, open up your project's .uproject file in Notepad (or a similar text editor), and change the `"AdditionalDependencies"` and `"Plugins"` sections to look like this:

```
	"Modules": [
		{
			"Name": "YourProjectName",
			"Type": "Runtime",
			"LoadingPhase": "Default",
			"AdditionalDependencies": [
				<OTHER DEPENDENCIES GO HERE>
				"PolygonalMapGenerator"
			]
		}
	],
	"Plugins": [
		<OTHER PLUGINS GO HERE>
		{
			"Name": "PolygonalMapGenerator",
			"Enabled": true
		}
	]
```

You can now open up your project in Unreal. You might be told that your project is out of date, and the editor will ask if you want to rebuild them. You do. After that, open up the Plugins menu, scroll down to the bottom, and ensure that the "PolygonalMapGenerator" plugin is enabled.

**Very important!** This project uses Unreal's FGameplayTags system that got brought out of experimental with Unreal Engine 4.15. 

In order for the system to work properly, you ***need*** to also enable the "GameplayTags" plugin in the Plugins menu. Once you restart the Unreal Editor, you need to go to your Project Settings/GameplayTags and find `Gameplay Tag Table List`. Add two elements to the array, pointing to `/PolygonalMapGenerator/GameplayTags/BiomeTags` and `/PolygonalMapGenerator/GameplayTags/MapMetadataTags`. Once this is done, restart the editor one more time (I know, it's a pain) so the GameplayTag array gets populated properly.

Once the editor has reloaded, you can either spawn in the `IslandMapGenerator` Actor raw, or create a Blueprint asset inheriting from it. Place the Actor in your level somewhere and call `CreateMap` on the `IslandMapGenerator`. This will create the actual map and notify you on completion. More details about how to use the data inside `IslandMapGenerator` are provided below.

#Changes from Source Article

There have been a few changes from the ActionScript source. There's nothing too drastic, just a couple changes for better ease-of-use:

* There has been an implementation of a "tag" system to provide more variety. This tag system uses Unreal's `FGameplayTag` class, which is fast, user-extensible, and can quickly and easily add classifications to things on the map. As an example, users can specify part of the map as a volcano by making a new GameplayTag like "MapData.MetaData.Volcano", which is then something that can be taken into account during Biome generation by extending the current classes. This tag system also replaces hardcoded booleans to determine whether part of the map is water, ocean, coast, etc.

* Most of the original ActionScript code was placed in a single class, `Map`, with a couple helper classes for the shape of the island (`IslandShape` in this project) and selecting which points to use (`PointGeneratior` in this project). This code has been further encapsulated, with the `Map` class (`IslandMapGenerator` here) being broken down into various stages. Each stage is its own class, which can be overridden and users can provide their own implementation if the default one isn't to their liking.

#How Maps Are Made

The actual map generation works by generating an array of points using a user-specified PointGeneratior (inheriting from UPointGenerator), which is triangulated using a [Delaunay Triangulation](https://en.wikipedia.org/wiki/Delaunay_triangulation). The Delaunay Triangulation is a dual graph of a [Voronoi Diagram](https://en.wikipedia.org/wiki/Voronoi_diagram), which provides us with 2 sets of points. This approach is called a "polygonal" map generation because we use the points of our Delaunay Triangulation as centers of polygons, with the vertices of that polygon given by the dual graph of the Voronoi Diagram.

Once a map is created, it will generate 3 different arrays:

* `FMapCenter`: This is the "center" of our polygon, and for all intents and purposes is treated as it being a polygon itself.

* `FMapCorner`: This is a "vertex" of our polygon, which runs along the edge of the polygon. Rivers and such flow from FMapCorner to FMapCorner.

* `FMapEdge`: This is a helper class, and contains details on two different graphs. The Delaunay Triangulation can be found by looking at the DelaunayEdge, linking together two neighboring FMapCenters. The Voronoi Diagram can be found by looking at the VoronoiEdge, linking together two neighboring FMapCorners. Note that the order in which Center/Corner comes first cannot be guaranteed; sometimes it may be left-to-right, while other times it may be right-to-left.

Both the FMapCenter and FMapCorner contain an FMapData struct on the inside, which contains data specific to that point in space -- elevation, moisture, whether a polygon is water, etc.

#Using in Unreal

To use in Unreal, simply place an `AIslandMapGenerator` actor in your level and call `CreateMap` on it. `CreateMap` takes a delegate which will be called when the map is complete. After the map is complete, you can access map corners, centers, or edges by calling `GetCorner`, `GetCenter`, or `GetEdge`, all of which ask for an index of the element in question, which is bound between 0 and `GetCornerNum`, `GetCenterNum`, or `GetEdgeNum`. 

Keep in mind that the above functions all return ***copies*** of objects; once modified, you need to call `UpdateCorner`, `UpdateCenter`, or `UpdateEdge`. You can also access an array of ***all** MapData objects across the entire graph; that is to say, it is an array of every single MapData object within every MapCorner and MapCenter. Doing this can give you a representation of the entire map as a whole, which can then be converted into a heightmap for whatever you need.

This system doesn't physically create anything inside the Unreal Engine itself, except for perhaps a debug diagram if the user chooses. It is up to the user to find or implement a system which can take the data from this graph and transform it into something tangible within the engine itself.

There is a class provided, `UPolygonalMapHeightmap`, which can create a heightmap from a `UPolygonMap`. If you are using the default implementation of `AIslandMapGenerator`, this will be done for you and you can access the heightmap after map generation is complete with `AIslandMapGenerator::GetHeightmap()`.

The `UPolygonalMapHeightmap` class provides a couple helper classes:

* `GetMapData()` provides a copy of the raw array of FMapData objects with size `FIslandData::Size` by `FIslandData::Size`, which the user can iterate over.

* `GetMapPoint()` takes in an integer X and Y value and safely outputs the FMapData object corresponding to that location. If that location is outside of the heightmap, it will output a "blank" FMapData object.

This array of FMapData objects can be turned into a 2D grayscale image by the user (using `FMapData::Elevation` to create the color value), or it can be used to create data points in a 3D voxel implementation.
