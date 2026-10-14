---
title: The README now uses an illustrated beach scene and separate wordmark
author: PhilosophiMoonbeam
date: 2026-10-14
branch: main
touches:
  - README.md
  - assets/35mmisle-logo.svg
  - assets/35mmisle-wordmark.svg
---

## Decided
The user explicitly rejected the compact transparent camera logo and requested a full rectangular anime-inspired beach scene. This supersedes the earlier logo-construction and 600-pixel placement learnings. assets/35mmisle-logo.svg is now a self-contained 1600 by 800 vector illustration: a weathered fictional film SLR resting partly in sand, a loose woven strap, turquoise surf, palms and a volcanic island reflected in curved lens glass. The upper-left 35mmIsle wordmark uses new original slanted letter paths. assets/35mmisle-wordmark.svg provides exactly the same lettering on a transparent canvas for independent use. README.md retains the centered repository-relative image before its heading and now requests width 1000 so the scene fills the available content width.

## Tried and ruled out
The previous compact camera icon, transparent overall canvas, 3:1 proportions, rounded wordmark and width 600 are superseded by the user's new art direction. The new banner uses a volcano reflection. First-pass palm leaflets overlapped into heavy sawtooth shapes; slimmer separated leaflets read more naturally as palms. Surf uses irregular foam ribbons instead of only continuous curves. Existing learnings are historical, not restrictions against this explicitly requested replacement.

## Watch out
Both SVGs remain accessible, self-contained and entirely vector, with no runtime fonts, embedded raster images, scripts or external resources. All lettering is original path geometry, and the standalone mark must match the banner. Chrome renders were inspected at 1600, 1000, 600 and 360 pixels, with light/dark contact-sheet checks and standalone lettering at 600 and 300 pixels. XML, references, accessibility labels and README path checks pass. Future edits need fresh visual inspection; scripts and preview PNGs used for this review live outside the repository. Keep the existing project documentation below the image.
