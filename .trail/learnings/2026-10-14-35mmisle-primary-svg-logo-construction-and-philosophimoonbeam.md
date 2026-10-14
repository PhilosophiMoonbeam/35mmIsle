---
title: 35mmIsle primary SVG logo construction and validation
author: PhilosophiMoonbeam
date: 2026-10-14
branch: main
touches:
  - assets/35mmisle-logo.svg
  - README.md
---

## Decided
The primary logo is assets/35mmisle-logo.svg, a 1200 by 400 transparent SVG with an original compact film camera, a single-palm island inside concentric lens rings, and the exact wordmark 35mmIsle. Lettering is original path geometry, with equal weight and color throughout and a barred capital I. No source font is used. The README embedding uses this relative path at width 600; README.md was intentionally left untouched for this assignment.

## Tried and ruled out
Charcoal-filled lettering with a thin cream outline looked too hollow on a dark background. Solid cream lettering with charcoal outlines and cream outer keylines keeps its weight on both backgrounds. Sand extending to the bottom of the lens read as a beach; a curved islet surrounded by water makes the intended scene clearer.

## Watch out
Keep the SVG self-contained, accessible, and vector-only. Preserve title/desc and aria-labelledby, internal-only references, the transparent outer canvas, and original letter paths. Chrome renders were visually inspected at actual 600- and 300-pixel widths on white and GitHub-dark (#0d1117) backgrounds. XML, allowed elements, references, and transparent render bounds were also checked. Future visual edits need fresh renders at both sizes and backgrounds. Avoid official characters, symbols, lettering, traced scenery, and copied camera geometry.
