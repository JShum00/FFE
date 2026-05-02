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
