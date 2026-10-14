---
title: Show the primary logo atop README
author: PhilosophiMoonbeam
date: 2026-10-14
branch: main
touches:
  - README.md
  - assets/35mmisle-logo.svg
---

## Decided
The user requested the finished SVG logo at the top of README.md. Embed assets/35mmisle-logo.svg in a centered paragraph before the project heading, with width 600 and meaningful alternative text.

## Tried and ruled out
The initial logo-only assignment left README.md untouched and returned an embedding snippet. The subsequent request explicitly authorizes placing that logo in README.md.

## Watch out
Use the repository-relative SVG path so the image follows the checked-out branch. Keep the existing project heading and documentation below the logo.
