# Frontline Forge Engine Roadmap

This document turns the current FFE vision into an execution plan.

It is intentionally more concrete than the high-level portfolio write-up. The
goal is to keep engine work tied to playable outcomes, avoid indefinite
refactoring, and define a clear boundary between inherited id Tech 4 systems
and FFE-owned gameplay architecture.

## Purpose

FFE is a long-term fork of the `RBDOOM-3-BFG` codebase aimed at supporting a
grounded, cinematic World War II first-person shooter.

The project should be treated as:

- an engine modernization effort
- a gameplay architecture rewrite
- a vertical-slice-driven FPS production foundation

It should not drift into being a Doom 3 content swap with different weapons.

## Core Rules

The project follows these rules throughout development:

- Every major engine change must support a playable milestone.
- Doom-specific behavior should be isolated before it is replaced.
- FFE-specific gameplay logic should live behind a narrow interface wherever
  possible.
- Asset and licensing boundaries must stay clear from the start.
- Tooling and workflow problems should be treated as first-class engineering
  work, not deferred cleanup.

## Current Baseline

The current codebase already has an initial FFE hook in place:

- `neo/d3xp/FFE.cpp` contains the first FFE runtime behavior
- `neo/d3xp/Game_local.*` includes minimal integration points
- `neo/d3xp/WorldSpawn.cpp` exposes an FFE-specific script hook
- `log.log` is the current runtime debugging target via `run.sh`

Relevant notes already in the repo:

- `docs/docs/FFE_INTEGRATION_NOTES.MD`
- `docs/docs/ENGINE_GAME_REQ.MD`
- `docs/docs/ENGINE_NOTES.md`

This roadmap assumes those documents remain the low-level implementation notes,
while this file acts as the higher-level execution plan.

## Strategic Objectives

The long-term engine and game goals are:

- replace Doom 3 horror pacing with infantry-combat encounter design
- replace monster-centric AI assumptions with human combat behavior
- modernize runtime systems that directly affect feel, iteration speed, and
  maintainability
- establish a clean gameplay layer that can support FFE-specific content
- produce a portfolio-quality codebase with documented technical decisions

## Non-Goals

FFE should avoid spending early years on broad engine ambitions that do not
improve the first playable combat slice.

Non-goals for early development:

- building a general-purpose commercial engine
- rewriting every legacy system before gameplay exists
- pursuing renderer perfection before weapon, movement, and AI fundamentals
  work
- replacing all legacy UI systems before a minimal debug HUD and gameplay loop
  exist

## Technical Direction

### Keep, wrap, or replace

Each subsystem should be classified before work starts:

- Keep: stable engine infrastructure that is good enough for current goals
- Wrap: legacy systems that still work but need an FFE-facing abstraction
- Replace: systems whose Doom-era assumptions directly block infantry gameplay

Initial classification:

- Keep:
  - core file system
  - core map loading path
  - base entity framework
  - collision and physics primitives until proven insufficient
- Wrap:
  - script entry points
  - player startup flow
  - HUD/menu integration
  - renderer backend usage from game-facing code
- Replace:
  - Doom-specific encounter startup logic
  - monster-centric combat AI
  - weapon handling assumptions built around Doom pacing
  - campaign scripting patterns tied to horror set-piece flow

### Architectural boundary

The desired direction is:

- engine layer preserves the underlying runtime and platform systems
- FFE gameplay layer owns combat, mission, player, and encounter logic
- Doom-specific code is either removed or quarantined behind compatibility
  seams until replaced

Near-term boundary target:

- `neo/d3xp/FFE.cpp` evolves from a single runtime hook into a small FFE
  gameplay integration layer
- FFE-owned state should move into a dedicated controller/state object instead
  of growing ad hoc fields on `idGameLocal`
- new FFE gameplay systems should prefer narrow entry points rather than broad
  edits across unrelated Doom files

## Phased Roadmap

### Phase 0: Stabilize The Fork

Goal: establish a reliable working baseline before broad replacement work.

Checklist:

- [x] Make special fork-debug startup behavior opt-in so normal engine startup
  remains the default branch workflow.
- [x] Verify the primary build and launch flow from the current repository docs
  and correct any drift.
- [x] Consolidate current FFE startup diagnostics so logs are useful without
  frame-spam or scattered one-off prints.
- [x] Document the current FFE integration points and code ownership boundary
  between framework, game, tools, and docs.
- [x] Audit the existing engine-cleanup changes for temporary hacks that should
  be either documented, renamed, or removed before deeper refactors.

Deliverables:

- project identity finalized as FFE in docs and developer-facing references
- build and run workflow stays reliable on the primary development platform
- current FFE startup path remains functional and debuggable
- Doom-specific gameplay modifications already made are documented
- initial code ownership map exists for engine, gameplay, tools, and docs

Exit criteria:

- a new contributor can build and launch the fork using repository
  documentation
- the current FFE hook behavior can be reproduced intentionally
- current integration points are documented well enough to refactor safely

Notes:

- this phase is about baseline confidence, not feature ambition
- any recurring startup/debug pain should be fixed here

Current focus:

- The first stabilization task on this branch is making minimal-app startup
  behavior explicitly opt-in rather than the default runtime path.

### Phase 1: Carve Out The Gameplay Layer

Goal: stop scattering FFE logic through Doom-specific pathways.

Checklist:

- [x] Move the current FFE runtime state off `idGameLocal` fields and into a
  dedicated FFE controller/state structure.
- [x] Refactor `neo/d3xp/FFE.cpp` so startup state, diagnostics, and encounter
  control read through that dedicated FFE-owned structure instead of ad hoc
  globals and scattered fields.
- [x] Reduce `Game_local.*` to narrow lifecycle hooks only: reset, frame tick,
  and explicit trigger entry points.
- [x] Keep `WorldSpawn.cpp` as the script-facing seam, but document whether the
  current `ffeTriggerStartupEncounter` naming should stay or be generalized in
  a later mission-scripting phase.
- [x] Re-audit FFE-specific logging so startup and trigger diagnostics remain
  localized to `FFE.cpp` and no new framework-level log drift is introduced.
- [x] Document which current FFE gameplay assumptions are still borrowed from
  Doom systems and therefore remain temporary.
- [x] Verify that the game still builds, launches, and reproduces the current
  FFE startup behavior after the refactor.

Deliverables:

- dedicated FFE state/controller structure replaces ad hoc startup fields
- FFE-specific logging, startup, and mission trigger flow are consolidated
- initial gameplay module boundaries are documented
- Doom-specific startup assumptions are identified and marked for replacement

Key work:

- refactor `FFE.cpp` into a cleaner ownership model
- keep touch points in `Game_local.*` and related files as thin as possible
- standardize FFE log prefixes and runtime diagnostics
- document which systems are still borrowed intact from Doom gameplay code

Exit criteria:

- the game can still boot and run with the same behavior after refactoring
- FFE runtime behavior is easier to trace than the current prototype
- future gameplay work no longer requires arbitrary edits across the module

Execution order:

1. Extract state ownership.
2. Rewire `FFE.cpp` around that ownership.
3. Shrink `Game_local.*` back to thin hooks.
4. Update docs to match the new seam.
5. Rebuild and validate runtime behavior.

### Phase 2: First Playable Infantry Slice

Goal: prove the project is becoming a combat game rather than a fork cleanup
exercise.

Checklist:

- [x] Define the exact Phase 2 slice contract in one short design note:
  starting location, player goal, failure condition, and done condition.
- [ ] Choose one target gameplay map strategy for the slice and document it:
  reuse an existing Doom map as a temporary harness, create a graybox test map,
  or branch an existing map into an FFE-specific combat test space.
- [ ] Define the minimum player feature set for the slice:
  move, aim, fire, reload, take damage, complete objective, restart.
- [ ] Define the minimum enemy feature set for the slice:
  detect player, move, attack, take damage, die, and pressure the objective.
- [ ] Replace the current startup monster harness with a deliberately authored
  combat bootstrap for the slice, even if it is still temporary.
- [ ] Decide what stays placeholder for Phase 2 versus what must be custom
  content before the slice is considered valid.

Asset and pipeline checklist:

- [ ] Inventory the currently usable asset path for custom content:
  model format, material setup, animation path, collision/physics setup, and
  script/entityDef hookup.
- [ ] Audit the current Blender tooling under `tools/blender/` and document
  what it actually supports today versus what is missing for FFE needs.
- [ ] Choose the Phase 2 source-of-truth DCC workflow:
  exact Blender version, export process, naming conventions, and directory
  layout for working files.
- [ ] Create a documented first-pass asset pipeline note covering:
  mesh authoring, export, file placement, material declaration, entityDef
  hookup, and in-engine validation.
- [ ] Create one pipeline test asset that is not gameplay-critical:
  a simple prop or marker model imported from Blender all the way into the
  engine.
- [ ] Create one gameplay-relevant custom asset path proof:
  either a custom weapon view/world model, a custom enemy stand-in, or a
  custom objective prop that appears in-game.
- [ ] Document how collision, origin placement, scale, and orientation are
  validated for imported assets.
- [ ] Record how textures and materials are authored and referenced for the
  slice, including what remains borrowed from Doom materials.
- [ ] Decide whether Phase 2 animations will use inherited Doom animations,
  retargeted assets, or static/limited placeholder motion, and document that
  decision explicitly.

Gameplay implementation checklist:

- [ ] Define the first slice weapon behavior contract:
  damage model, fire cadence, reload behavior, ammo presentation, and feedback
  standard.
- [ ] Decide whether the Phase 2 primary firearm is a temporary reuse of Doom
  weapon internals or the start of an FFE-specific weapon path.
- [ ] Implement the minimum player handling changes needed for grounded
  infantry pacing and document what still remains Doom-like.
- [ ] Define one temporary human-enemy stand-in path:
  reskinned Doom actor, heavily modified existing actor, or first custom human
  actor prototype.
- [ ] Implement objective flow for one complete encounter:
  enter space, receive objective, fight through pressure, satisfy objective,
  and confirm completion.
- [ ] Add only the minimum HUD or debug feedback required to understand ammo,
  damage state, and objective progress.
- [ ] Ensure the slice can be re-entered repeatedly for testing without
  cinematic-only setup or manual editor-only intervention.

Data, defs, and scripting checklist:

- [ ] Define the Phase 2 working directories and naming scheme for new FFE
  defs, materials, scripts, and generated content.
- [ ] Create or identify the entityDef path for the slice player-facing asset
  work: weapon, objective prop, or enemy stand-in.
- [ ] Create or identify the script entry point for the slice objective flow.
- [ ] Keep the script-facing seam narrow and document any new worldspawn or
  trigger event hooks added for the slice.
- [ ] Record which Doom defs/scripts are still being borrowed so they can be
  targeted for later replacement instead of forgotten.

Map and encounter checklist:

- [ ] Build or select one contained combat space suitable for fast iteration.
- [ ] Place spawn, encounter, cover, and objective beats intentionally rather
  than relying on the current forward-spawn startup heuristic.
- [ ] Define where the player starts, where the enemy starts, and what event
  causes combat to begin.
- [ ] Ensure the encounter can succeed and fail in a way that is visible in
  logs and on screen.
- [ ] Add at least one repeatable test path for rapid verification:
  direct launch, devmap, or command-driven encounter start.

Validation checklist:

- [ ] Verify the slice builds from the documented repo workflow.
- [ ] Verify the slice launches directly into its test path without relying on
  the old startup monster harness.
- [ ] Verify at least one custom or pipeline-validated asset survives the full
  DCC-to-engine path.
- [ ] Verify logs clearly show objective start, combat start, and completion or
  failure.
- [ ] Write a short regression checklist for replaying the slice after major
  code or content changes.

Deliverables:

- one test map or graybox combat space
- grounded player movement and camera behavior
- one primary firearm with usable feedback and reload flow
- one human enemy archetype with minimal combat logic
- one objective-driven encounter from start to completion
- minimal debug HUD or mission feedback needed to play the slice

Key work:

- modernize input handling where it directly affects weapon feel
- decouple weapon behavior from Doom-era pacing assumptions
- replace startup monster encounter logic with a simple combat scenario
- define the first mission scripting pattern around objectives, not survival
  beats

Exit criteria:

- a player can launch the build, enter a map, fight human enemies, complete an
  objective, and exit without relying on Doom-style flow
- the slice demonstrates the intended pacing better than any design document

Execution order:

1. Lock the slice contract and choose the map/content strategy.
2. Prove the Blender-to-engine pipeline with one non-critical asset.
3. Stand up the first gameplay-relevant custom asset or stand-in path.
4. Replace the startup harness with an authored encounter bootstrap.
5. Implement objective flow, feedback, and repeatable launch/testing hooks.
6. Validate the slice end to end and capture the regression path.

### Phase 3: Combat Architecture

Goal: build the reusable systems needed for a real campaign mission set.

Deliverables:

- human combat AI foundation
- squad behaviors for movement, pressure, and communication
- animation updates that support readable infantry combat
- cleaner weapon state model for future arsenal growth
- battlefield audio prioritization rules
- mission scripting layer that supports objective progression and controlled
  set pieces

Key work:

- replace monster assumptions in AI perception, movement, and attack behavior
- define a reusable encounter structure for squads, reinforcements, and
  scripted pressure
- separate player weapon handling from monster-oriented combat responses
- improve animation transitions for weapon-ready, movement, and hit response

Exit criteria:

- at least one encounter type can be reused across multiple maps without
  one-off hacks
- AI behavior reads as human tactical pressure rather than reskinned monsters

### Phase 4: Tooling And Content Pipeline

Goal: reduce friction in building content and testing systems.

Deliverables:

- documented asset import workflow
- level scripting workflow that does not depend on obscure tribal knowledge
- exporter/build helper cleanup where needed
- test map conventions for AI, animation, weapons, and mission scripting

Key work:

- inventory the current toolchain and identify the worst iteration bottlenecks
- write docs for map setup, script hooks, and content packaging
- add focused utilities only where they clearly improve iteration speed

Exit criteria:

- adding a new gameplay test scenario is straightforward and documented
- content iteration is no longer blocked by hidden setup steps

### Phase 5: Renderer And Platform Modernization

Goal: modernize deeper engine systems after gameplay direction is proven.

Deliverables:

- clear rendering roadmap with justified priorities
- compatibility decisions documented for OpenGL/Vulkan paths
- profiling baseline for representative gameplay scenes
- obsolete or duplicate rendering paths marked for removal or containment

Key work:

- prioritize rendering work that improves stability, maintainability, or
  battlefield readability
- avoid large renderer rewrites that do not help the active game slice
- measure performance against real combat scenes, not empty benchmarks

Exit criteria:

- rendering changes are justified by measurable benefit
- the engine remains aligned with gameplay production needs

### Phase 6: Pre-Production To Production Transition

Goal: decide when FFE stops being primarily an engine experiment and becomes a
production foundation.

Deliverables:

- stable gameplay architecture for mission production
- content workflow proven by more than one playable scenario
- known technical debt list with priorities
- realistic scope decision for the first public-facing game milestone

Exit criteria:

- the team can create new content on top of FFE without core-system panic
- remaining engine debt is understood well enough to schedule intentionally

## Priority Order

When priorities conflict, use this order:

1. Build and runtime stability
2. Playable infantry-combat slice
3. Gameplay architecture cleanliness
4. Tooling and iteration speed
5. Renderer/platform modernization
6. Broad polish and presentation

This ordering exists to prevent endless infrastructure work with no playable
proof.

## Immediate System Priorities

The next systems to focus on should be:

- player startup and mission flow
- input cleanup that affects aiming and weapon feel
- weapon handling and combat pacing
- human enemy prototype behavior
- encounter scripting and objective flow
- debug visibility through logs, HUD feedback, and test maps

These priorities are intentionally biased toward playability over elegance.

## Risks

### Risk: "Doom with rifles"

Symptoms:

- enemy behavior still reads like monsters with different models
- encounter pacing still depends on corridor ambush logic
- player movement and weapons still feel tuned for Doom rather than infantry
  combat

Mitigation:

- validate every major gameplay change against the vertical slice
- judge combat by pacing and readability, not by whether assets changed

### Risk: endless engine refactor

Symptoms:

- months of cleanup without a better playable build
- architecture documents improve while gameplay stagnates

Mitigation:

- require each major phase to end in a verifiable playable outcome
- defer speculative rewrites until a concrete gameplay need exists

### Risk: tooling becomes the hidden blocker

Symptoms:

- content tests take too long to build or launch
- scripting and map iteration depend on undocumented steps

Mitigation:

- treat tooling docs and test scenarios as milestone work
- remove friction early when it slows iteration on combat features

### Risk: code ownership drift

Symptoms:

- FFE logic spreads through many unrelated legacy files
- future refactors become risky because boundaries are unclear

Mitigation:

- keep FFE entry points explicit
- document ownership and integration points as they change

## Documentation Plan

Docs should be layered:

- `README.md`: project identity, build basics, high-level purpose
- `ffe-roadmap.md`: phased execution plan and priorities
- `docs/docs/FFE_INTEGRATION_NOTES.MD`: current runtime integration details
- `docs/docs/ENGINE_GAME_REQ.MD`: engine-to-game dependency map
- `docs/docs/ENGINE_NOTES.md`: subsystem structure and navigation notes

Any major architectural decision should eventually be captured in `docs/` with:

- decision
- reason
- alternatives considered
- consequences for future work

## Near-Term Action List

The most useful next steps are:

1. Refactor the current `FFE.cpp` startup logic into a dedicated FFE runtime
   state/controller structure.
2. Define the first true vertical slice map and what counts as "playable."
3. Replace the prototype startup monster encounter with a human-combat test
   encounter.
4. Document the intended gameplay boundary between inherited Doom systems and
   FFE-owned systems.
5. Create a short backlog for input, weapon feel, AI prototype behavior, and
   objective scripting.

## Success Criteria

FFE is on the right track when the following become true:

- the repo reads like an intentional engine fork, not an improvised mod
- gameplay behavior is increasingly owned by FFE-specific systems
- each quarter of work results in a better playable combat build
- technical docs explain why key decisions were made
- renderer and engine modernization serve the game instead of distracting from
  it

## Portfolio Positioning

Externally, FFE should be presented as:

- an id Tech 4 modernization and restructuring effort
- a case study in adapting a legacy FPS engine to a different combat model
- a practical demonstration of gameplay architecture, AI redesign, and engine
  integration work

The strongest portfolio evidence will not be the ambition of the plan. It will
be a sequence of playable milestones backed by clean technical documentation.
