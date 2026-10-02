# Map export

`mapexport` reconstructs a Radiant `.map` from the loaded collision map. It is
not a lossless decompiler: render-only surfaces are not included, triangle and
quad meshes use caulk, and model placements reference assets by name. Model
assets and textures must be available separately in Radiant.

## Entire collision map (default)

After loading a map, enable collision drawing and allow it to initialize:

```text
r_drawCollision 3
```

Then export:

```text
mapexport_useFilters 0
mapexport_writeTriangles 1
mapexport_writeQuads 1
mapexport_writeEntities 1
mapexport_writeModels 1
mapexport_writeDynModels 1
mapexport
```

This ignores debug brush amount, sorting, material, index, selection-box and
minimum-size filters. The write options above still control which kinds of
objects are included. Camera position, view direction and collision draw
distance do not restrict the export. Output is written to
`<CoD4 folder>/iw3xo/map_export/<map name>.map`.

## Filtered export

Set `mapexport_useFilters 1` ("Use Debug Collision Filters" in the developer GUI)
to use the previous behavior. Brush amount, sorting, material and index filters
apply to brushes; selection-box mode also filters collision triangles, map
entities, probes and static-model origins. Dynamic models retain the existing
behavior and are not restricted by the selection box. A selection box needs
two points added with `mapexport_selectionAdd`.

The exporter reports available, attempted and written brush counts and rejection
counts for insufficient points/sides, minimum size and invalid bounds. Geometry
that cannot be reconstructed safely is still rejected. Check these counts if
objects remain missing; changing draw distance will not help.

## Validation

Build the Win32 Release DLL with the existing Visual Studio/Premake workflow.
Run the independent regression test from a Visual Studio developer prompt:

```bat
cl /nologo /EHsc /std:c++17 tests\map_export_tests.cpp /Fe:build\map_export_tests.exe /Fo:build\map_export_tests.obj
build\map_export_tests.exe
```

In-game/Radiant checks (require CoD4 and the relevant map/assets):

1. Set a brush limit of 1, a material filter, an index filter and a small
   selection box. With `mapexport_useFilters 0`, export from two different camera
   positions. Both exports should attempt every collision brush, include all
   collision triangles with the write options enabled, and have identical bounds.
2. Enable `mapexport_useFilters 1` and verify the previous filtered export still
   works, including the minimum-size filter and selection-box behavior.
3. Export a map with a multi-brush submodel, including a sloped brush at a nonzero
   origin. Verify all brushes occur in the entity, with translated planes matching
   translated bounds. Test entity order differing from the `*N` model order and
   a brushmodel without an explicit origin (uses zero).
4. Export the same map repeatedly, first whole, then selected, then whole again.
   Confirm brushmodel geometry is neither duplicated nor retained from an earlier
   selection. Exercise a five-sided reconstructed brushmodel as well.
5. Disable entity/model write options and confirm those objects are omitted.
   Attempt an export to an unwritable output location and check the error message.

The standalone test covers multi-brush leaves, split nodes, overlapping-brush
subtrees, deduplication, invalid brush indices, null leaves and invalid roots or
offsets. It does not replace in-game geometry validation.
