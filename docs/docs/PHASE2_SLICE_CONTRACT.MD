# Phase 2 Slice Contract

This note defines the first playable infantry slice for FFE.

The goal is not to prove full game scope. The goal is to prove that FFE can
run one intentional combat scenario with clear player purpose, readable combat
pressure, and a complete success/failure loop.

## Slice Name

Stalingrad Courtyard Push

## Scenario Intent

The player enters a ruined urban combat pocket, takes fire from one enemy
group, clears or suppresses them, and reaches a simple objective point.

This slice should feel like:

- grounded infantry movement
- short-range urban combat pressure
- objective-driven progression
- authored combat start, not random or purely proximity-based chaos

This slice should not yet try to prove:

- full squad tactics
- final animation quality
- final content quality
- complete WWII arsenal breadth
- final mission scripting architecture

## Starting Location

The player starts at the edge of a ruined street or courtyard approach that
immediately frames the combat space ahead.

Required start conditions:

- player begins on foot and in direct gameplay control
- no intro cinematic is required to begin the slice
- player starts with one primary firearm and enough ammunition to finish the
  encounter
- the objective space is visible or strongly implied from the starting angle
- the player has at least one piece of nearby cover available within the first
  few seconds

Environment expectation:

- the space should read as a bombed-out urban infantry lane, checkpoint, or
  courtyard rather than a Doom corridor

## Player Goal

Primary goal:

- push into the combat space and secure the objective point

The first objective should be simple and externally readable, for example:

- reach and secure a doorway
- reach a radio position
- cross a contested courtyard and interact with a supply/objective marker

The goal must require combat pressure to resolve. It should not be satisfiable
by simply walking forward untouched.

## Failure Condition

Phase 2 requires one explicit failure state:

- player death before securing the objective

Optional early slice failure states, if cheap to support:

- leaving the playable combat bounds
- triggering objective failure because a required object or ally was lost

Failure must be visible through:

- a log event
- a visible gameplay state change such as death, reset, or failure message

## Done Condition

The slice is considered complete when all of the following are true:

- the player can launch directly into the slice test path
- combat starts intentionally and reproducibly
- the player can take damage and return fire
- at least one enemy can be defeated
- the player can satisfy the objective condition
- the slice reports success in a visible way
- the player can restart and replay the slice without manual setup

## Minimum Feature Contract

The slice must include:

- one playable combat space
- one primary firearm
- one enemy archetype
- one objective trigger or interaction
- one clear start condition
- one clear success condition
- one clear failure condition

The slice does not require:

- final art
- final audio mix
- final AI sophistication
- multiple objectives
- multiple enemy classes
- final weapon roster

## Placeholder Policy

Allowed placeholders for Phase 2:

- inherited Doom movement code with targeted tuning
- inherited Doom weapon internals if behavior is intentionally constrained
- inherited Doom actor logic if used only as a temporary human stand-in
- placeholder meshes, materials, and animations
- debug HUD or text-based objective feedback

Not allowed if the slice is to count as valid:

- random startup monster spawning as the main combat bootstrap
- a pure cinematic intro standing in for objective flow
- a slice that cannot be replayed directly
- a slice where success can only be inferred from logs

## Testing Path

The slice must have one repeatable launch path, such as:

- `devmap` directly into the slice map
- a specific command-driven encounter bootstrap
- a dedicated test script path

That path should become the standard regression path for Phase 2 work.

## Exit Decision

Phase 2 is successful when the slice answers this question with "yes":

Can FFE now demonstrate a deliberate infantry-combat encounter with a complete
start, pressure, objective, and resolution loop that no longer reads like Doom
with a substituted weapon?
