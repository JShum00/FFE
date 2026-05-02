# FFE Integration Notes

This note captures how the current Frontline Forge Engine gameplay hook is
wired into the Doom 3 BFG game code, what failed during the first passes, and
where to extend it cleanly.

## Current File Layout

### Primary FFE runtime file

- `neo/d3xp/FFE.cpp`

This is the main home for FFE-specific runtime behavior. It currently owns:

- FFE controller/state ownership
- controller-owned startup diagnostics and encounter control
- startup state reset
- startup weapon grant
- startup monster spawn
- cinematic gating
- manual runtime trigger

### Thin Doom-side touch points

- `neo/d3xp/Game_local.cpp`
  - calls `FFE_ResetStartupState()` during `Clear()`
  - calls `FFE_ResetStartupState()` during `MapPopulate()`
  - calls `FFE_RunStartupFrame()` from `RunFrame()`
- `neo/d3xp/Game_local.h`
  - declares FFE runtime entry points
  - no longer stores FFE-owned startup state directly
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

- the dedicated FFE runtime controller/state structure
- controller-local diagnostics, logging, and cinematic-gate checks
- startup state reset behavior
- startup loadout behavior
- startup encounter placement and trigger logic
- cinematic gating for that encounter
- FFE-prefixed startup diagnostics

### `neo/d3xp/Game_local.*`

Owns:

- calling the FFE entry points from existing game lifecycle methods only

Should remain:

- thin glue
- not the long-term home for expanding FFE gameplay logic

### `neo/d3xp/WorldSpawn.cpp`

Owns:

- script-facing event exposure for `ffeTriggerStartupEncounter`

Naming decision:

- keep `ffeTriggerStartupEncounter` as-is during Phase 1
- treat it as an intentionally narrow seam for the current startup encounter
- reconsider a broader name only when FFE gains a real mission-scripting event
  surface beyond startup bootstrap behavior

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
- keep the current startup-specific name through Phase 1
- revisit naming only when the startup encounter grows into broader mission
  scripting behavior and a more general event vocabulary is justified

Reason:

- this is currently the cleanest map/script entry point into FFE-owned runtime
  behavior
- renaming it now would add churn without improving the current seam

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

Disposition:

- completed in early Phase 1 by moving the startup state into an FFE-owned
  controller in `neo/d3xp/FFE.cpp`

Reason:

- FFE-owned runtime state now sits with FFE-owned behavior instead of inflating
  `idGameLocal`

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
2. `FFE_RunStartupFrame()`
3. FFE resolves the local player internally
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

## Borrowed Doom Assumptions Still In Use

The current FFE startup harness still relies on several Doom-era gameplay
assumptions. These are temporary and should not be treated as long-term FFE
architecture.

### Doom weapon and ammo definitions

Current dependency:

- `weapon_machinegun`
- `ammo_clip`
- `SlotForWeapon()` / `WeaponAvailable()` behavior from the existing player and
  inventory stack

Why it is temporary:

- this is Doom inventory and weapon-slot behavior, not an FFE-designed infantry
  weapon system
- it is only being used to validate startup loadout control and script/runtime
  timing

Replacement direction:

- swap this out once FFE has its own weapon/loadout rules and player-facing
  combat pacing

### Doom monster entity usage

Current dependency:

- `monster_zombie_fat`
- `MASK_MONSTERSOLID`
- monster-sized spawn bounds and trace assumptions

Why it is temporary:

- this is a Doom monster class being used as a spawn/encounter test target
- it does not represent human combat behavior, squad logic, or FFE encounter
  pacing

Replacement direction:

- replace with a human combat test actor before Phase 2 becomes the active
  production focus

### Doom cinematic and camera gating semantics

Current dependency:

- `inCinematic`
- `GetPrivateCameraView()`
- `GetCamera()`
- hidden-player checks

Why it is temporary:

- these are existing Doom rules for when the player is considered under
  cinematic control
- FFE currently reuses them to avoid spawning into intro-map transitions

Replacement direction:

- keep the seam for now, but revisit when FFE mission flow and authored
  encounter timing become more explicit

### Doom worldspawn weapon gate behavior

Current dependency:

- `worldspawn.no_Weapons`
- `EV_Player_EnableWeapon`

Why it is temporary:

- this is inherited map/world policy from Doom startup behavior
- FFE currently overrides it only to make the startup harness reliable

Replacement direction:

- replace with explicit FFE mission/start-state rules rather than clearing a
  Doom-era world flag as part of normal gameplay flow

### Doom player-centric encounter bootstrap

Current dependency:

- local-player lookup from `GetLocalPlayer()`
- direct use of `player->viewAngles`
- trace placement in front of the player as the trigger basis

Why it is temporary:

- this is a bootstrap encounter heuristic, not a general mission or squad
  encounter system
- it assumes one immediate player-facing spawn pattern rather than authored
  objective logic

Replacement direction:

- move toward explicit mission scripting and authored encounter definitions once
  the vertical-slice combat space exists

### Doom game-code module surface

Current dependency:

- all FFE runtime behavior still lives inside the inherited `neo/d3xp`
  game-code module
- script-facing entry points still use Doom’s event and thread system

Why it is temporary:

- the module surface is acceptable for the current fork stage, but it still
  inherits Doom-centric gameplay organization
- FFE is restructuring within that module before attempting broader gameplay
  replacement

Replacement direction:

- continue carving FFE-owned gameplay seams inside `neo/d3xp` during Phase 1,
  then replace specific borrowed systems as vertical-slice needs become clear

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

## Phase 1 Verification

Phase 1 seam refactors were verified with:

- `./build.sh`
- `timeout 20s ./run.sh` for normal launcher/bootstrap validation
- `timeout 30s ./build/RBDoom3BFG +devmap game/mars_city1` for direct gameplay
  validation of the startup harness

Observed result:

- the build completed successfully
- the standard launcher still booted cleanly through engine and game
  initialization
- the direct `devmap` launch reproduced the current FFE startup behavior in
  `log.log`

Observed FFE startup sequence:

- `FFE: No cinematic gate seen; waiting 2000 ms after startup before auto encounter`
- `FFE: Cleared no_Weapons gate for startup loadout`
- `FFE: Startup weapon grant ...`
- `FFE: Cinematic gate cleared; waiting 1000 ms before auto encounter`
- `FFE: Startup encounter ready for automatic trigger ...`
- `FFE: Spawned test monster ... source=auto`

Conclusion:

- the Phase 1 refactor preserved the current startup harness behavior while
  moving ownership into the FFE controller seam

### Startup diagnostics

FFE startup diagnostics should stay concentrated in `neo/d3xp/FFE.cpp`.

Current expectations:

- FFE-specific startup logs stay prefixed with `FFE:`
- startup logging explains state transitions, not just final actions
- framework-level FFE logging should stay minimal unless the issue is truly
  outside the gameplay hook

Current audit result:

- startup and trigger diagnostics are localized to `neo/d3xp/FFE.cpp`
- `Game_local.*` no longer emits FFE-prefixed startup diagnostics
- remaining framework-level `FFE:` logs are limited to opt-in minimal-app
  lifecycle messages, which are application-level rather than gameplay-level

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
