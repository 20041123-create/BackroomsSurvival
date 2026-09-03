# Backrooms template reconstruction

## Reference-map audit

`/Game/LegoGame/Maps/Backrooms` contains 421 static-mesh actors and 421
different static-mesh assets. Of those actors, 420 share the same actor
transform `(-75, -1185, 2505)`. Their world placement was baked into each
mesh's bounds instead of being represented by reusable local transforms.

The dominant construction vocabulary is:

| Element | Instances | Reusable role |
| --- | ---: | --- |
| ceiling lamp | 66 | light slot |
| closed ceiling panel | 59 | normal 300 cm ceiling module |
| ceiling vent | 40 | ventilation slot |
| straight wall | 35 | 300 cm wall run |
| lamp/vent ceiling panel | 34 | 300 cm module with a central 97 cm opening |
| wall corner | 24 | corner finish |
| large pillar groups | 6 | open-hall rhythm |
| individual pillar family | 3 | reusable structural column |

The reference combines carpet or linoleum floors, wallpaper or plaster wall
faces, light/dark wall trim, acoustic ceiling panels, ceiling frames, vents and
fluorescent fixtures. Variety comes primarily from spatial rhythm and light
density rather than from a large number of unrelated materials.

## Why the map is not cropped directly

Directly cropping the source map would preserve a screenshot but would not
produce robust procedural templates:

- every placed object uses a unique mesh with baked absolute coordinates;
- cropped actors would need to be rebased and normalized one by one;
- source regions do not conform to the Survival 1800 cm cell, connector, anchor
  or navigation contract;
- a Level Instance crop would retain hundreds of one-off mesh dependencies and
  would be difficult to rotate, replicate and reuse in another project.

A hand-authored special room can still be made later by copying one region into
a Survival-owned sublevel, rebasing it to a local origin, and wrapping it in a
room actor selected through `URoomTemplateData::RoomActorClass`. That route is
appropriate for landmarks, not the common procedural room set.

## Reconstructed room grammar

The runtime builder uses the existing `TemplateId`, footprint, connectors and
room type; no Contracts change is required.

| Template family | Construction |
| --- | --- |
| Base hub | open 2x2 hall with a structural pillar rhythm |
| Straight corridor | connector-aligned narrow passage with two inset wall runs |
| Corner corridor | L-shaped passage with the unused corner partitioned off |
| Pillar hall | open hall with one reference pillar per logical cell |
| Partitioned hall | cross-partitioned room with a central circulation opening |
| Resource workshop | linoleum, pale plaster, dark trim and paired columns |
| Dark corridor | sparse warm fixtures and narrow connector-aligned circulation |

Every ceiling module is closed unless it contains a lamp or vent. Lamp and vent
modules use the matching open ceiling panel, so openings can no longer be left
unfilled. A backing slab above the drop ceiling plus shadow-casting room lights
prevents light leaking through wall/ceiling seams.

`L_SurvivalWorld` deliberately has no directional or sky light. Those actors
were useful in the original graybox but illuminated the metallic ceiling-frame
texture across room boundaries and looked like light leaking through every
grid seam. Runtime fixtures now use only downward-facing Rect Lights with a
room-local attenuation radius and no specular contribution; the former
omnidirectional Point Light fill has been removed. A wider source and increased
indirect contribution provide readable ambient bounce without illuminating the
ceiling above each fixture.
