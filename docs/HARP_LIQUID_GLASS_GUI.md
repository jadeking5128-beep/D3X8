# Harp UI and Liquid Glass

D3X8 keeps the Harp compositor and its shared-memory window protocol. The new UI layer is a native C implementation inspired by the composable layout model of Skift Karm, not a port of Karm's C++ runtime.

## Rendering direction

- Opaque ARGB8888 compositor surfaces remain the source of truth.
- Glass surfaces snapshot only their clipped rectangle, then apply bounded background sampling, edge refraction, softening, tint, and specular highlights.
- The dock uses the native glass renderer rather than a pre-blurred wallpaper strip.
- Glass rendering has an allocation-failure fallback that retains a legible translucent surface.
- Refraction and blur are bounded to keep work proportional to the panel area.

## UI model

`harp_ui.h` exposes retained nodes for rows, columns, stacks, containers, text, buttons, glass panels, and spacers. Nodes can be composed, laid out, rendered through backend callbacks, and given click handlers. This keeps layout/state policy separate from Harp's pixel renderer and lets native applications share the same component model.

## Integration contract

- Keep window ownership, focus, Z-order, input routing, shared-memory transport, and dirty-region scheduling in Harp.
- Use Harp UI nodes for application content and system surfaces.
- Provide renderer callbacks that map fill/text/glass operations to the application's drawing backend.
- Avoid full-screen glass passes. Snapshot and process only the panel bounds.
- Keep app text and content crisp above the glass material.
- Reuse nodes and buffers; destroy trees when their owning application closes.

## Visual system

Use dark graphite surfaces with cool blue-white glass highlights, restrained accent blue, rounded squircle geometry, thin borders, and readable high-contrast typography. The dock, titlebars, popovers, settings navigation, dialogs, and control center should all use the same material tokens.

## Current implementation boundary

The first native implementation adds the retained C UI core and a refraction-based glass surface renderer, and applies the glass renderer to Harp's dock groups. The existing application layouts and window titlebars are intentionally preserved until they can be migrated without breaking application behavior.
