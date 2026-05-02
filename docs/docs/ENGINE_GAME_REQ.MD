# Engine Game Requirements Map

This map is based on `neo/d3xp/Game_local.cpp`. It lists the methods and script-facing hooks a game module must provide or preserve for the engine to start, load a map, run frames, render, save/load, and shut down without crashing.

## Required API Export

### `GetGameAPI( gameImport_t* import )`

Engine entry point for the game module.

- Receives engine service pointers such as `common`, `cmdSystem`, `fileSystem`, `renderSystem`, `soundSystem`, `declManager`, `collisionModelManager`, and `AASFileManager`.
- Stores those services in game globals and `idLib`.
- Returns `gameExport_t` with `gameExport.game` and `gameExport.gameEdit`.
- Required because the engine cannot call `Init`, map loading, frame simulation, drawing, or editor helpers until this export succeeds.

Script usage:

- Scripts do not call this directly.
- Treat it as the native bridge that makes script, decl, entity, file, render, sound, and collision systems available to gameplay code.

## Required Game Lifetime Methods

### `idGameLocal::Init()`

One-time game-module startup.

- Registers game decl types and decl folders.
- Registers game console commands and CVars.
- Initializes the event system and runtime class system.
- Creates the shell menu handler.
- Starts the default script program from `SCRIPT_DEFAULT`.
- Creates smoke particle storage.
- Allocates AAS instances from the `aas_types` entityDef.
- Leaves the game in `GAMESTATE_NOMAP`.

Script usage:

- The default script file must compile successfully.
- Any script functions, events, or object types used by spawned entities need to be available before maps are initialized.
- Keep the `aas_types` entityDef valid if AI/navigation is enabled.

### `idGameLocal::Shutdown()`

One-time game-module shutdown.

- Cleans up shell UI, multiplayer state, the active map, AAS, AI obstacle data, events, classes, forces, scripts, smoke particles, and map data.
- Required so the engine can exit or unload the game module without dangling objects.

Script usage:

- Do not rely on script threads surviving shutdown.
- Any persistent script state must be saved before shutdown through normal save-game paths.

### `idGameLocal::Clear()`

Resets global game state to a known empty state.

- Clears server info, persistent player info, entity arrays, spawn IDs, entity lists, counters, and runtime flags.
- Called during startup and relied on before map state is built.

Script usage:

- Scripts should not assume global entity pointers remain valid across map loads or restarts.

## Required Map Load Methods

### `idGameLocal::InitFromNewMap( const char* mapName, idRenderWorld* renderWorld, idSoundWorld* soundWorld, int gameMode, int randseed )`

Primary new-map startup method.

- Shuts down any previous map.
- Stores render and sound worlds.
- Sets game type and skill.
- Calls `LoadMap`.
- Calls `InitScriptForMap`.
- Calls `MapPopulate`.
- Populates environment probes.
- Resets and precaches multiplayer state.
- Syncs players with lobby users.
- Flushes unused animations.
- Leaves the game in `GAMESTATE_ACTIVE`.

Script usage:

- This is the path that runs the global map script entry point.
- Ensure map scripts and entityDefs are available before this runs.
- If your scripts create map-critical entities, they should do it from the default map script function or from entity spawn methods reached during `MapPopulate`.

### `idGameLocal::LoadMap( const char* mapName, int randseed )`

Shared map setup used by both new games and save-game loads.

- Clears sound emitters and resets slow-motion sound speed.
- Initializes async networking.
- Parses the `.map` file.
- Loads collision data.
- Clears entity state and runtime map lists.
- Resets time, random seed, camera, world pointer, slow-time state, portals, clip, PVS, smoke particles, gravity, and cinematic state.
- Initializes AAS files.
- Precaches required built-in defs and materials.

Script usage:

- Your map must exist as a parseable `.map`.
- Required decls such as entityDefs, damage defs, ammo defs, and materials must resolve.
- Scripts should not assume entities exist at `LoadMap` time; entity spawning happens afterward.

### `idGameLocal::InitScriptForMap()`

Creates map script execution state.

- Creates a manual-delete `frameCommandThread`.
- Finds and starts `SCRIPT_DEFAULTFUNC`, currently `doom_main`.

Script usage:

- Provide a global script function named `doom_main` if your game uses this path.
- Use `doom_main` for global map/game script initialization, not for per-entity spawn data.
- Keep the function safe to run after `LoadMap` but before all map entities finish populating.

### `idGameLocal::MapPopulate()`

Builds runtime map content after map and scripts are ready.

- Spawns clients/players as needed.
- Calls `SpawnMapEntities`.
- Performs map setup after entities exist.

Script usage:

- Entity spawn functions and entity script objects must be ready before this method runs.
- Use entityDefs to drive most map-spawned behavior.

### `idGameLocal::SpawnMapEntities()`

Spawns entities from the parsed map entity list.

- Requires map entity `0` to spawn `worldspawn`.
- Calls `SpawnEntityDef` for each non-inhibited map entity.
- Precaches media for each entity before spawning it.
- Adds brush origin offset data through `__brushOrigin`.

Script usage:

- Every map must have a valid world entity.
- Each map entity must have a `classname`.
- EntityDefs should include either `spawnclass` for native C++ entities or `spawnfunc` for script-spawned entities.

### `idGameLocal::SpawnEntityDef( const idDict& args, idEntity** ent, bool setDefaults )`

Creates one runtime entity/object from spawn args.

- Reads `classname`.
- Resolves the matching entityDef.
- Applies entityDef defaults to the instance spawn args.
- Applies slow-motion defaults.
- Uses `spawnclass` to instantiate a registered native `idClass`.
- Uses `spawnfunc` to start a script function when no native class is specified.
- Returns false if the classname, native class, or script function cannot be resolved.

Script usage:

- Define entityDefs with a valid `classname`.
- Use `spawnclass` when the behavior is implemented in C++.
- Use `spawnfunc` when the behavior is script-created.
- Any `spawnfunc` named in an entityDef must exist in the loaded script program.
- Use spawn args as the data contract between `.map`/`.def` files and script/native spawn code.

## Required Frame Methods

### `idGameLocal::RunFrame( idUserCmdMgr& cmdMgr, gameReturn_t& ret )`

Main simulation tick.

- Syncs lobby users and replicated CVars.
- Handles stopped-time debug mode.
- Advances fast and slow time groups.
- Updates render view context.
- Clears debug drawing.
- Frees expired smoke particles.
- Processes network entity events.
- Updates gravity.
- Builds player PVS.
- Sorts and thinks active entities.
- Runs fast time-group entities.
- Runs shared entity thinking.
- Removes inactive entities from the active list.
- Services normal and fast events.
- Runs multiplayer frame logic.
- Fills `gameReturn_t`.
- Handles session commands and cinematic skipping.
- Emits debug overlays and trace capture cleanup.

Script usage:

- Script threads and delayed events are serviced from this frame path.
- Entity `Think`, script events, and time waits depend on this method being called regularly.
- Avoid script logic that assumes all entities run in one time group; slow-motion can split entities between normal and fast time groups.

### `idGameLocal::RunEntityThink( idEntity& ent, idUserCmdMgr& userCmdMgr )`

Runs frame logic for one entity.

- Dispatches player command processing for player entities.
- Runs normal entity `Think` for non-player entities.
- Required by `RunFrame` for active entity simulation.

Script usage:

- Entity script behavior should be safe to execute once per game frame when active.
- Entities that should stop thinking must clear their think flags through normal entity APIs.

### `idGameLocal::RunTimeGroup2( idUserCmdMgr& userCmdMgr )`

Runs entities assigned to the fast time group.

- Keeps player/weapon-critical entities responsive while slow-motion affects other gameplay.

Script usage:

- If your entity must ignore slow motion, ensure its spawn data/native code places it in the fast time group.
- Script timing should not assume all entities share the same effective frame delta.

### `idGameLocal::RunSharedThink()`

Runs shared post-think logic on active entities.

- Called after primary entity think passes.
- Provides a second common update phase for active entities.

Script usage:

- Use this phase only through native entity support; scripts should treat it as post-think native maintenance.

### `idGameLocal::BuildReturnValue( gameReturn_t& ret )`

Builds the result returned to the session layer after a frame.

- Clears and fills session command output.
- Provides controller vibration values.
- Clears consumed session commands.

Script usage:

- Scripts can indirectly request map/session transitions through gameplay targets that set `sessionCommand`.
- Do not write directly to `gameReturn_t` from scripts.

## Required Render and Input Methods

### `idGameLocal::Draw( int clientNum )`

Draws the current frame for a client.

- Rejects invalid clients.
- Lets multiplayer draw its own view when appropriate.
- Sets UI drawing context.
- Gets the player render view.
- Renders the player view and HUD.

Script usage:

- Scripts should ensure the player and camera/render view state are valid before expecting anything visible.
- HUD and view effects should be driven through player/view/HUD systems, not by calling draw directly.

### `idGameLocal::HandlePlayerGuiEvent( const sysEvent_t* ev )`

Routes GUI/input events.

- Gives the local player first chance to handle GUI events.
- Falls back to multiplayer GUI handling if needed.

Script usage:

- In-game GUI scripts should route actions through player GUI handlers or multiplayer menu handlers.
- Do not assume raw system events reach arbitrary entity scripts unless native code forwards them.

## Required Save and Restore Methods

### `idGameLocal::SaveGame( idFile* saveGameFile, idFile* stringTableFile )`

Serializes current game state.

- Saves entities, script threads, program state, server info, persistent player info, entity arrays, world, active/spawned lists, random seed, frame thread, session command, cinematic state, time state, slow-motion state, locations, camera, global material, PVS, gravity, pending events, and other required runtime fields.

Script usage:

- Script objects, variables, and threads must be save-compatible.
- Entity scripts should keep persistent state in fields that the save/restore system knows how to serialize.
- Avoid depending on unsaved transient data after load.

### `idGameLocal::InitFromSaveGame( const char* mapName, idRenderWorld* renderWorld, idSoundWorld* soundWorld, idFile* saveGameFile, idFile* stringTableFile, int saveGameVersion )`

Restores a map from save-game data.

- Shuts down the previous map.
- Loads the map and collision data.
- Recreates saved objects.
- Restores script program state.
- Restores entities, lists, world, events, time, slow-motion, camera, PVS, gravity, multiplayer state, and other saved fields.
- Returns false if script state is incompatible with the save.

Script usage:

- Changing script layouts can invalidate old saves.
- Restore paths must tolerate objects being recreated before all references are fully resolved.

### `idGameLocal::GetSaveGameDetails( idSaveGameDetails& gameDetails )`

Builds save metadata for the UI/session.

- Records expansion, map name, location, save version, difficulty, play time, and date.

Script usage:

- Location and map display data should be kept valid if save UI depends on it.

### `idGameLocal::GetPersistentPlayerInfo( int clientNum )`

Returns player data carried across map transitions.

- Used by the session before changing levels.

Script usage:

- Store cross-level player state in persistent player info when native systems expose it.
- Do not rely on ordinary entity fields surviving a level transition.

### `idGameLocal::SetPersistentPlayerInfo( int clientNum, const idDict& playerInfo )`

Receives player data restored across map transitions.

- Used immediately before loading a new level.

Script usage:

- Read persistent player state after the next map initializes, not before map load.

## Required Map Shutdown Methods

### `idGameLocal::MapShutdown()`

Tears down the active map.

- Marks the game as shutting down.
- Clears debug render data.
- Disables editors.
- Shuts down map entities and map-local runtime state.

Script usage:

- Do not keep script references to entities after map shutdown.
- Use save/persistent data paths for anything that must survive.

### `idGameLocal::MapClear( bool clearClients )`

Clears entity and map-local object state.

- Deletes entities.
- Clears entity hash.
- Deletes frame command thread, editor entities, and location data.
- Can preserve client entities depending on `clearClients`.

Script usage:

- Treat all entity script objects as invalid after this runs.

## Required Server and Time Methods

### `idGameLocal::SetServerInfo( const idDict& serverInfo )`

Receives server/map configuration.

- Stores server info and enforces mode-specific constraints.

Script usage:

- Read server/game mode settings from supported spawn args or native accessors rather than hard-coding mode assumptions.

### `idGameLocal::GetServerInfo()`

Returns current server configuration.

- Used by engine/session save and network code.

Script usage:

- Treat this as native configuration data; scripts should use exposed wrappers if needed.

### `idGameLocal::SetServerGameTimeMs( int time )`

Updates authoritative server game time.

- Called during frame advancement.
- Tracks previous server time.

Script usage:

- Script waits and frame events depend on this time moving forward during `RunFrame`.

### `idGameLocal::GetServerGameTimeMs()`

Returns authoritative server game time.

- Used by prediction, networking, and frame systems.

Script usage:

- Prefer script time APIs instead of native direct access.

### `idGameLocal::GetTimeGroupTime( int timeGroup )`

Returns the current time for the requested time group.

- Required for slow-motion and fast-time entity behavior.

Script usage:

- Do not assume a single global clock if your gameplay uses slow motion.

## Required Entity Lookup and Definition Methods

### `idGameLocal::FindEntityDef( const char* name, bool makeDefault )`

Finds an entityDef declaration.

- Required by map load, precache, and spawning.

Script usage:

- Keep `.def` names stable.
- Missing entityDefs can prevent map startup or individual entity spawns.

### `idGameLocal::FindEntityDefDict( const char* name, bool makeDefault )`

Returns the dictionary for an entityDef.

- Used during startup for required data such as `aas_types`.

Script usage:

- Use entityDef dictionaries as the shared data layer between scripts, maps, and native code.

### `idGameLocal::RegisterEntity( idEntity* ent, int forceSpawnId, const idDict& spawnArgsToCopy )`

Registers a created entity with game state.

- Assigns entity number/spawn ID.
- Adds it to entity lists and lookup structures.

Script usage:

- Script-created entities must go through native spawn paths so registration happens.

### `idGameLocal::UnregisterEntity( idEntity* ent )`

Removes an entity from game state.

- Clears entity slots and lookup structures.
- Required for safe deletion.

Script usage:

- Remove entities through supported entity APIs rather than invalidating references manually.

### `idGameLocal::FindEntity( const char* name )`

Finds a spawned entity by name.

- Used by gameplay, editor helpers, and target resolution.

Script usage:

- Use stable `name` spawn args when scripts need to find specific entities.
- Check for missing entities before using returned references.

## Required Script and Entity Data Contracts

### `SCRIPT_DEFAULT`

Default script file loaded during `Init`.

- Current value: `script/doom_main.script`.
- Must compile before maps can run script logic.

### `SCRIPT_DEFAULTFUNC`

Default global map/game script function called by `InitScriptForMap`.

- Current value: `doom_main`.
- Optional in code because `InitScriptForMap` checks for null, but required for custom script-side game initialization.

### `entityDef` with `classname`

Core spawn declaration contract.

- Every map entity needs a `classname`.
- `classname` must resolve through `FindEntityDef`.

### `entityDef` with `spawnclass`

Native entity spawn contract.

- Names a registered C++ `idClass`.
- Used for normal entities like worldspawn, players, triggers, movers, lights, items, projectiles, and cameras.

### `entityDef` with `spawnfunc`

Script entity spawn contract.

- Names a script function in the loaded script program.
- `SpawnEntityDef` starts it on a new `idThread`.

### `worldspawn`

Required first map entity.

- `SpawnMapEntities` expects map entity `0` to spawn an `idWorldspawn`.
- Failure to spawn a valid world entity is fatal.

### Player EntityDef

Required playable client entity.

- `SpawnPlayer` uses the world spawn arg `def_player`, defaulting to `player_doommarine`.
- Multiplayer can use mode-specific player defs.

## Minimum Startup Order

1. Engine loads the game module and calls `GetGameAPI`.
2. Engine calls `idGameLocal::Init`.
3. Session calls `InitFromNewMap` or `InitFromSaveGame`.
4. `LoadMap` parses map/collision data and resets map state.
5. `InitScriptForMap` starts the default script function.
6. `MapPopulate` spawns players and map entities.
7. Engine repeatedly calls `RunFrame`.
8. Engine calls `Draw` for visible clients and `HandlePlayerGuiEvent` for GUI input.
9. Save/session systems call save, persistent player, networking, and metadata methods as needed.
10. Map transitions call `MapShutdown`; engine shutdown calls `Shutdown`.

