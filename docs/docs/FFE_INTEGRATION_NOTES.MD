# FFE Integration Notes

This note captures how the current Frontline Forge Engine gameplay hook is
wired into the Doom 3 BFG game code, what failed during the first passes, and
where to extend it cleanly.

## Current File Layout

### Primary FFE runtime file

- `neo/d3xp/FFE.cpp`

This is the main home for FFE-specific runtime behavior. It currently owns:

- startup state reset
- startup weapon grant
- startup monster spawn
- cinematic gating
- manual runtime trigger

### Thin Doom-side touch points

- `neo/d3xp/Game_local.cpp`
  - calls `FFE_ResetStartupState()` during `Clear()`
  - calls `FFE_ResetStartupState()` during `MapPopulate()`
  - calls `FFE_RunStartupFrame( player )` from `RunFrame()`
- `neo/d3xp/Game_local.h`
  - declares FFE runtime entry points
  - stores minimal FFE runtime state on `idGameLocal`
- `neo/d3xp/WorldSpawn.cpp`
  - exposes a script-callable event: `ffeTriggerStartupEncounter`
- `neo/CMakeLists.txt`
  - adds `d3xp/FFE.cpp` to the game build

The intent is that gameplay-specific FFE logic stays in `FFE.cpp`, while Doom
engine files only know about a small interface.

## Current Ownership Boundary

This is the current practical ownership split for FFE work.

### Framework layer

Primary location:

- `neo/framework/`

Responsibilities:

- application startup and shutdown
- session flow, map-change orchestration, and frame loop ownership
- console, file system, cvar, command, and logging infrastructure
- global launcher/runtime behavior that is not specific to FFE gameplay

FFE-specific rule:

- framework changes should stay minimal and only exist when FFE needs a true
  engine-level hook
- current examples include the opt-in minimal app path and launcher-adjacent
  runtime handling
- do not move gameplay behavior into `neo/framework/`

### Game layer

Primary location:

- `neo/d3xp/`

Responsibilities:

- gameplay runtime and entity behavior
- map population and per-frame gameplay simulation
- player, weapon, AI, trigger, target, and mission logic
- script events and game-facing CVars

FFE-specific rule:

- FFE gameplay ownership currently lives here
- `FFE.cpp` is the preferred home for FFE-specific startup behavior, logging,
  and small integration glue
- `Game_local.*` should expose narrow hooks, not absorb large amounts of
  FFE-specific logic

### Tooling layer

Primary locations:

- repo-root `build.sh`
- repo-root `run.sh`
- `tools/`
- `neo/tools/`

Responsibilities:

- build and launch workflow
- asset-side helper scripts and exporters
- compiler/editor helper utilities

FFE-specific rule:

- workflow improvements belong here when they reduce iteration friction
- tools should support FFE content creation and debugging without becoming the
  place where gameplay policy is implemented

### Documentation layer

Primary locations:

- repo-root `README.md`
- repo-root `ffe-roadmap.md`
- `docs/docs/`

Responsibilities:

- project identity and developer entrypoint docs
- roadmap, technical notes, and architecture decisions
- engine structure references and integration notes

FFE-specific rule:

- branch-level decisions should be written down once they become recurring
  guidance
- low-level implementation notes belong in `docs/docs/`
- roadmap and positioning belong at repo root

## Current FFE Touch Points

The active FFE integration surface is intentionally small.

### `neo/d3xp/FFE.cpp`

Owns:

- startup state reset behavior
- startup loadout behavior
- startup encounter placement and trigger logic
- cinematic gating for that encounter
- FFE-prefixed startup diagnostics

### `neo/d3xp/Game_local.*`

Owns:

- calling the FFE entry points from existing game lifecycle methods
- storing the current minimal FFE runtime state on `idGameLocal`

Should remain:

- thin glue
- not the long-term home for expanding FFE gameplay logic

### `neo/d3xp/WorldSpawn.cpp`

Owns:

- script-facing event exposure for `ffeTriggerStartupEncounter`

### `neo/d3xp/gamesys/SysCvar.h`

Owns:

- external declaration for FFE gameplay-facing CVars that the game/framework
  need to see

### `neo/framework/Common.cpp`

Owns:

- the global ESC handling branch for opt-in minimal app mode

### `neo/framework/Common_load.cpp`

Owns:

- the map-change bypass used by opt-in minimal app mode

### `neo/framework/common_frame.cpp`

Owns:

- the minimal app overlay draw path

### `neo/sys/sdl/sdl_vkimp.cpp`

Owns:

- SDL window title branding for the current runtime window

## Edit Policy For Near-Term FFE Work

When adding or changing FFE behavior, use this order:

1. Put FFE-specific gameplay behavior in `neo/d3xp/FFE.cpp` first.
2. Touch `Game_local.*` only to add or maintain narrow lifecycle hooks.
3. Touch `WorldSpawn.cpp` or script-facing game files only when a map/script
   integration point is required.
4. Touch `neo/framework/` only when the change is truly application-level.
5. Update docs when the boundary or workflow changes in a way others will need
   to follow.

If a change requires broad edits across framework and game at the same time,
that should be treated as a design decision and documented before it grows.

## Phase 0 Audit Findings

This audit captures the current `engine-cleanup` branch shortcuts and how they
should be treated before Phase 1 work expands.

### Retain as bounded debug scaffolding

#### `ffe_minimalApp`

Locations:

- `neo/d3xp/FFE.cpp`
- `neo/framework/Common.cpp`
- `neo/framework/Common_load.cpp`
- `neo/framework/common_frame.cpp`

Disposition:

- keep for now as an opt-in startup/debug path
- do not allow it to become the default runtime path again
- do not add gameplay logic that depends on minimal app mode existing

Reason:

- it is useful for fork bring-up and runtime sanity checks
- it is explicitly a framework-level diagnostic hook, not gameplay architecture

### Retain as intentional integration seam

#### `ffeTriggerStartupEncounter`

Location:

- `neo/d3xp/WorldSpawn.cpp`

Disposition:

- keep as the current script-facing FFE event seam
- revisit naming only when the startup encounter grows into broader mission
  scripting behavior

Reason:

- this is currently the cleanest map/script entry point into FFE-owned runtime
  behavior

### Replace as prototype gameplay harness

#### Doom-asset startup encounter

Location:

- `neo/d3xp/FFE.cpp`

Current placeholders:

- `weapon_machinegun`
- `monster_zombie_fat`
- forward spawn of a single test enemy in front of the player

Disposition:

- treat the entire current startup encounter as temporary prototype behavior
- replace it before Phase 2 vertical-slice work becomes the main branch focus
- do not build long-term combat systems around this harness

Reason:

- it exists to validate FFE lifecycle hooks and timing, not to represent final
  combat architecture

### Replace as temporary state storage

#### FFE runtime state on `idGameLocal`

Location:

- `neo/d3xp/Game_local.h`

Current fields:

- `ffeMonsterSpawned`
- `ffeWeaponGranted`
- `ffeSawCinematic`
- `ffeClearSinceTime`

Disposition:

- keep only as Phase 0 and early Phase 1 scaffolding
- move into a dedicated FFE controller/state object during Phase 1

Reason:

- the fields are acceptable for a small bootstrap seam
- they should not become the long-term storage model for expanding FFE systems

### Keep as branding correction

#### SDL window title rename

Location:

- `neo/sys/sdl/sdl_vkimp.cpp`

Disposition:

- keep

Reason:

- this is a small, low-risk branding fix
- it does not create architectural coupling

### Acknowledge but defer

#### Legacy engine naming still visible at runtime

Locations:

- `neo/d3xp/Game_local.h` via `GAME_VERSION`
- `build/RBDoom3BFG` binary name

Current drift:

- the runtime log still reports `gamename: baseDOOM-1`
- the produced executable is still `RBDoom3BFG`

Disposition:

- document now
- defer the actual rename until packaging/build-script/runtime naming can be
  changed coherently

Reason:

- this is real branding drift, but it is not blocking the current stabilization
  goals
- changing it piecemeal now would risk needless churn across build and launch
  paths

## Runtime Flow

### Automatic flow

The current automatic startup path is:

1. `idGameLocal::RunFrame()`
2. `GetLocalPlayer()`
3. `FFE_RunStartupFrame( player )`
4. FFE checks whether cinematic/camera gating is still active
5. FFE grants the machinegun if needed
6. FFE waits until the player is truly in gameplay
7. FFE spawns the startup monster using the player's live position and facing

### Script-triggered flow

The current manual trigger path is:

1. script/event calls `ffeTriggerStartupEncounter` on worldspawn
2. `idWorldspawn::Event_FFE_TriggerStartupEncounter()`
3. `gameLocal.FFE_TriggerStartupEncounter()`
4. FFE grants the weapon if needed
5. FFE spawns the monster from the player's current position

This gives two ways to drive the same encounter:

- automatically from the frame loop
- explicitly from map script timing

## Important Findings

### 1. `FindEntity( "info_player_start_1" )` was the wrong lookup

`FindEntity()` searches entity `name`, not entity `classname`.

That means:

- `FindEntity( "info_player_start_1" )` only works if a map entity is
  literally named that
- it does not mean "find the player start class"

That was the reason the first startup spawn lookup failed.

### 2. Early spawn-point-based spawning was too detached from real gameplay

Using the first `info_player_start` was better than using a hard-coded entity
name, but it still did not solve the intro-map problem:

- the intro cinematic still controlled the player/camera state
- the real encounter should key off the live player state, not a map start
  entity that may only be relevant during setup

Current FFE spawn placement uses:

- `player->GetPhysics()->GetOrigin()`
- `player->viewAngles.ToForward()`
- a forward offset
- trace-based adjustment to keep the monster out of walls/floor

### 3. The intro cinematic was deleting the spawned monster

One key log line made this clear:

- `removing 'idAI_monster_zombie_fat_XXXX' for cinematic`

That comes from the cinematic cleanup path in `idGameLocal::SetCamera()`.
During cinematic transitions, active AI can be removed.

This is why spawning "successfully" was not enough. The encounter had to wait
until cinematic state fully cleared.

### 4. The machinegun grant was blocked by `no_Weapons`

The map/world state can force `no_Weapons`, which blocks normal weapon pickup
logic even when the player has a valid weapon slot definition.

That is why the log showed:

- ammo grant succeeded
- slot lookup succeeded
- weapon give returned false

Current FFE code fixes that by:

1. clearing `worldspawn.no_Weapons`
2. sending `enableWeapon`
3. giving the machinegun
4. selecting the weapon slot

### 5. Automatic encounter timing must wait for post-cinematic stability

The current automatic gate tracks:

- whether a cinematic/camera state was seen
- when the world first became clear of cinematic gating

Current rule:

- if a cinematic was seen, wait one second after it fully clears
- if no cinematic was seen, wait two seconds after startup

This keeps the automatic encounter from firing during intro setup.

## Script Hook

### Worldspawn event

`WorldSpawn.cpp` now exposes:

- `ffeTriggerStartupEncounter`

This can be called from script on the worldspawn entity to trigger the FFE
startup encounter at an exact script-controlled moment.

The worldspawn entity is named `world`, so the practical script pattern is:

```c
entity worldEnt;
worldEnt = sys.getEntity( "world" );
worldEnt.ffeTriggerStartupEncounter();
```

If the script already has direct access to the world entity object, it can call
the event there instead of looking it up by name.

## Logging

### Launcher log path

`./run.sh` now mirrors launcher/game stdout and stderr into:

- `./log.log`

This is the repo-root log next to `run.sh`.

The script:

- truncates `./log.log` at the start of each run
- launches the game from `build/`
- tees output live to terminal and to `./log.log`
- preserves the game's exit code

This makes the root log the primary runtime log for FFE debugging.

### Startup diagnostics

FFE startup diagnostics should stay concentrated in `neo/d3xp/FFE.cpp`.

Current expectations:

- FFE-specific startup logs stay prefixed with `FFE:`
- startup logging explains state transitions, not just final actions
- framework-level FFE logging should stay minimal unless the issue is truly
  outside the gameplay hook

## Recommended Next Steps

### Good next cleanup

- move FFE runtime state out of `idGameLocal` fields and into a dedicated
  `FFEState` or controller struct

### Good next gameplay step

- let scripts decide whether to use automatic startup encounter timing or
  explicitly call `ffeTriggerStartupEncounter`

### Good next debugging step

- keep all future FFE-specific logs prefixed with `FFE:` and emitted from
  `FFE.cpp` whenever possible

That keeps feature debugging localized to one file and one log prefix.
