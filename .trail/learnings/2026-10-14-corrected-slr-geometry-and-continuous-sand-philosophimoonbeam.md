---
title: Corrected SLR geometry and continuous sand in the README banner
author: PhilosophiMoonbeam
date: 2026-10-14
branch: main
touches:
  - assets/35mmisle-logo.svg
  - README.md
---

## Decided
The user found the first beach-scene camera deformed, its lower-left sand coverage incomplete, and the palette too muted. Retain the 1600 by 800 rectangular SVG scene, volcanic reflection, original corner lettering, standalone wordmark, and existing README embedding. Rebuild the fictional SLR with a rectangular front, matching receding top/right planes, a symmetrical pentaprism aligned with its lens mount, and a short barrel projecting along the body depth axis. Replace separate floating sand strips with one closed foreground dune extending beyond the canvas; it covers the lower-left body corner and overlaps the bottom lens rim. Use brighter blue sky, turquoise/jade water, emerald palms, golden/peach sand, coral strap and flowers, and violet highlights in the reflection.

## Tried and ruled out
The first scene's lens was displaced far to the right of the prism and used a conflicting barrel direction. Narrow sand patches allowed the camera corner to appear beneath them. A first revised dune started partway across the canvas, leaving a vertical seam, and sat below the lens rim; extending the dune past both canvas edges and raising its center fixed those rendered defects. The user correction supersedes the first beach-scene geometry and muted colors; older compact/transparent-logo dimensions are already superseded by the rectangular-scene request.

## Watch out
Inspect the rendered contact edge, not just whether a sand path exists. The final Chrome previews were inspected at 1600, 1000, 600, and 360 pixels, including light/dark contexts. Both SVGs parse with valid accessibility labels and internal references, use vector elements only, and have identical original lettering geometry. XML comparisons must ignore surrounding element tail whitespace. README.md and the standalone wordmark are unchanged. The revised banner is about 106 KB. No fonts, raster images, scripts, or external resources are required. Temporary generation and render scripts are outside the repository.
