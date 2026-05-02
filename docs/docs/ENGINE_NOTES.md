# Engine Notes

## `neo/d3xp` Structure

`neo/d3xp` is the Doom 3 BFG game-code module. It sits above `idlib`,
`framework`, renderer, sound, file system, decls, AAS, and collision systems,
and exports the game-facing API that the rest of the engine drives.

The important boundary is `Game.h`:

- `idGame` is the public runtime interface used by the engine to initialize,
  load maps, save, run frames, draw, process snapshots, route input, and manage
  shell/menu state.
- `idGameEdit` is the public editing/tools interface for parsing spawn args,
  animation lookup, AF editor support, entity selection, entity mutation, and
  map edits.
- `gameImport_t` is the engine service table passed into the game code.
- `gameExport_t` returns the `idGame` and `idGameEdit` implementations.
- `GetGameAPI` is the exported symbol. `Game.def` also lists this export for
  Windows DLL builds.

`Game_local.h` and `Game_local.cpp` are the private center of the module:

- `idGameLocal` implements `idGame`.
- The global `gameLocal` owns map state, entity arrays, spawn IDs, player
  state, PVS, clip world, multiplayer state, event queues, time groups, and
  per-frame simulation.
- Most `.cpp` files include `Game_local.h`, so subsystem code can reach shared
  game state and engine imports.
- Map startup flows through `InitFromNewMap`, map entity creation through
  `SpawnEntityDef` / `SpawnEntityType`, and gameplay ticking through
  `RunFrame`.
- Networking is split out into `Game_network.cpp`; in-game editor support is
  split out into `GameEdit.cpp`.

### Top-Level Gameplay Files

The root of `neo/d3xp` contains the main gameplay entity hierarchy and the
high-level gameplay features:

- `Entity.*` defines `idEntity`, the base for spawned map/game objects. It
  owns spawn args, script object binding, think flags, targets, visuals,
  physics attachment, networking state, save/restore behavior, and events.
- `Actor.*` builds animated, damageable character behavior on top of AF-backed
  entities.
- `Player.*`, `PlayerView.*`, `PlayerIcon.*`, `AimAssist.*`, `Weapon.*`, and
  `Grabber.*` implement local player behavior, view effects, HUD/player icon
  helpers, aim assist, weapons, and the ROE grabber.
- `AI.*` is not in the root; monsters live under `ai/`, but they derive from
  the root actor/entity hierarchy.
- `AF.*` and `AFEntity.*` bind articulated figures to entities for ragdolls,
  gibbable bodies, vehicles, attachments, and AF-driven world objects.
- `Projectile.*`, `Item.*`, `Moveable.*`, `Mover.*`, `Trigger.*`, `Target.*`,
  `Light.*`, `Sound.*`, `Camera.*`, `SecurityCamera.*`, `WorldSpawn.*`,
  `Misc.*`, `Fx.*`, `SmokeParticles.*`, `BrittleFracture.*`, and
  `EnvironmentProbe.*` are concrete map-spawnable entity families.
- `MultiplayerGame.*`, `Leaderboards.*`, and `Achievements.*` hold game-mode,
  scoreboard/leaderboard, and achievement systems.
- `Pvs.*` wraps potentially visible set tracking used by gameplay.
- `IK.*` contains inverse-kinematics helpers used by actors/animations.
- `PredictedValue.*` supports client-side prediction smoothing.
- `precompiled.cpp` only includes `precompiled.h`; most real behavior is in
  the subsystem files.

### Subdirectories

`neo/d3xp/gamesys`

- Runtime type/event/save infrastructure for game classes.
- `Class.*` implements the `idClass` hierarchy and type metadata.
- `Event.*` defines script/event dispatch.
- `SaveGame.*` handles save/restore serialization helpers.
- `SysCvar.*` and `SysCmds.*` register game CVars and console commands.
- `Callbacks.cpp` contains game callback glue.

`neo/d3xp/script`

- Doom script compiler, program storage, interpreter, and thread execution.
- `Script_Program.*` stores script types, functions, objects, and variables.
- `Script_Compiler.*` compiles `.script` files.
- `Script_Interpreter.*` executes script bytecode.
- `Script_Thread.*` represents running script threads and event waits.

`neo/d3xp/anim`

- MD5 animation and entity animation system.
- `Anim.*` owns animation declarations, model defs, anim blends, and
  `idAnimator`.
- `Anim_Blend.cpp` contains blend/evaluation behavior.
- `Anim_Testmodel.*` implements the in-game test model entity/tooling.

`neo/d3xp/ai`

- Monster AI and AAS pathfinding.
- `AI.*`, `AI_events.cpp`, `AI_pathing.cpp`, and `AI_Vagary.cpp` implement
  `idAI`, script events, movement/pathing helpers, and monster-specific logic.
- `AAS.*`, `AAS_local.h`, `AAS_pathing.cpp`, `AAS_routing.cpp`, and
  `AAS_debug.cpp` wrap Area Awareness System routing, path search, debug draw,
  and navigation data.

`neo/d3xp/physics`

- Game-side physics objects layered on the engine collision system.
- `Physics.*` is the abstract interface.
- `Physics_Base.*` provides common implementation.
- `Physics_Static.*`, `Physics_StaticMulti.*`, `Physics_Parametric.*`,
  `Physics_RigidBody.*`, `Physics_Actor.*`, `Physics_Player.*`,
  `Physics_Monster.*`, and `Physics_AF.*` cover concrete movement models.
- `Clip.*` owns clip models and game collision queries.
- `Push.*` handles entity pushing.
- `Force.*` plus `Force_Constant.*`, `Force_Drag.*`, `Force_Field.*`,
  `Force_Grab.*`, and `Force_Spring.*` implement force generators.

`neo/d3xp/menus`

- SWF/menu frontend code used by shell, PDA, HUD, lobbies, scoreboard, and
  options screens.
- `MenuHandler.*` owns screen stacks and dispatch for Shell, PDA, HUD, and
  Scoreboard handlers.
- `MenuScreen.*` defines the base screen class and the many concrete screens
  such as root, pause, save/load, settings, game lobby, PDA pages, HUD, and
  scoreboards.
- `MenuWidget.*` defines reusable UI widgets: buttons, lists, dynamic lists,
  navigation bars, command bars, carousel, scrollbars, info boxes, save info,
  PDA widgets, lobby list, and item assignment.

### Dependency Shape

The module is intentionally centralized:

- Engine code talks to `idGame` / `idGameEdit` from `Game.h`.
- `GetGameAPI` wires engine singletons into game globals and returns
  `gameLocal`.
- Most gameplay systems depend on `Game_local.h` and therefore on `gameLocal`.
- `idEntity` is the main gameplay object base; almost every map object,
  character, weapon helper, trigger, target, and effect derives from it.
- Script, events, save/restore, animation, physics, AI, and menus are local
  subsystems used by the entity/game layer rather than independent libraries.

For new gameplay work, start by identifying whether the behavior is:

- A new or modified map-spawnable object: begin in `Entity.*` and the relevant
  concrete family such as `Item.*`, `Trigger.*`, `Target.*`, `Mover.*`,
  `Moveable.*`, or `Projectile.*`.
- Player-facing behavior: begin in `Player.*`, `Weapon.*`, `PlayerView.*`,
  `AimAssist.*`, or menu/HUD files under `menus/`.
- Monster behavior: begin in `ai/AI.*` and follow any script events back
  through `gamesys/Event.*` and `script/`.
- Physics behavior: begin in `physics/Physics.*`, `Clip.*`, and the concrete
  physics class attached by the entity.
- Save/network behavior: inspect the entity's `Save` / `Restore` and snapshot
  methods, then check `Game_network.cpp` and `gamesys/SaveGame.*`.
