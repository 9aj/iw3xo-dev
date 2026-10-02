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
counts for invalid/empty convex hulls and minimum size. Rejected hulls also leave
a comment containing the source collision-brush index and bounds in the `.map`.
Check these counts if objects remain missing; changing draw distance will not help.

## Slopes and small convex brushes

Brush export clips each collision bounding box against its additional planes,
independently of debug drawing. Valid four- and five-sided brushes are included
automatically. Fractional plane points are preserved, and each exported face uses
its original collision-side material even when other faces disappear during clipping.

`mapexport_5SideBrush`, `mapexport_eps1` and `mapexport_eps2` remain registered for
old configurations but no longer affect brush export. No special toggle or rounding
setting is needed to include wedges and preserve slopes.

## Validation

Build the Win32 Release DLL with the existing Visual Studio/Premake workflow.
Run the independent regression test from a Visual Studio developer prompt:

```bat
cl /nologo /EHsc /std:c++17 tests\map_export_tests.cpp /Fe:build\map_export_tests.exe /Fo:build\map_export_tests.obj
build\map_export_tests.exe
cl /nologo /EHsc /std:c++17 tests\convex_brush_tests.cpp /Fe:build\convex_brush_tests.exe /Fo:build\convex_brush_tests.obj
build\convex_brush_tests.exe
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
   selection. Exercise four- and five-sided brushmodels as well.
5. Disable entity/model write options and confirm those objects are omitted.
   Attempt an export to an unwritable output location and check the error message.

The standalone test covers multi-brush leaves, split nodes, overlapping-brush
subtrees, deduplication, invalid brush indices, null leaves and invalid roots or
offsets. The geometry test checks boxes, tetrahedra, wedges, fractional and
translated slopes, redundant/nonunit planes, a 160-sided prism and invalid hulls.
It checks emitted plane orientation, source-side identity, half-space containment
and serialization precision.

A local offline audit of `mp_qube.ff` (SHA-256
`fdc7e10c6b8f206bada4ad6c458c6107ce85a0f596197446e2092e21e2cc4912`)
passed all 3,304 collision brushes through the production reconstruction helper:
3,304 accepted, zero rejected, 33 five-sided hulls and 1,200 hulls with fractional
output points. This verifies reconstruction against that compiled map; it does not
replace a fresh in-game export and inspection of the reported bounces in Radiant.
