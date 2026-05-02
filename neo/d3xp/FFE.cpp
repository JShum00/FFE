/*
===========================================================================

Frontline Forge Engine runtime hooks.

This file keeps FFE-specific gameplay setup out of the core Doom files as
much as possible. Game_local.cpp only calls the small entry points defined
here, and scripts can trigger the same encounter through worldspawn.

===========================================================================
*/

#include "precompiled.h"
#pragma hdrstop

#include "Game_local.h"

idCVar ffe_minimalApp(
	"ffe_minimalApp",
	"0",
	CVAR_BOOL,
	"Run the engine in an opt-in minimal application mode for fork startup debugging"
);

namespace
{

class FFEController
{
public:
	enum StartupDiagState
	{
		STARTUP_DIAG_NONE,
		STARTUP_DIAG_WAITING_FOR_CINEMATIC_CLEAR,
		STARTUP_DIAG_WAITING_FOR_POST_CINEMATIC_DELAY,
		STARTUP_DIAG_WAITING_FOR_STARTUP_DELAY,
		STARTUP_DIAG_READY_FOR_AUTO_TRIGGER
	};

	struct StartupState
	{
		bool monsterSpawned;
		bool weaponGranted;
		bool sawCinematic;
		int clearSinceTime;
		StartupDiagState diagState;

		void Reset()
		{
			monsterSpawned = false;
			weaponGranted = false;
			sawCinematic = false;
			clearSinceTime = -1;
			diagState = STARTUP_DIAG_NONE;
		}
	};

	static FFEController& Get()
	{
		static FFEController controller;
		return controller;
	}

	FFEController()
	{
		startup.Reset();
	}

	void ResetStartupState()
	{
		startup.Reset();
	}

	void RunStartupFrame( idGameLocal& gameLocal )
	{
		idPlayer* player = gameLocal.GetLocalPlayer();
		if( player == NULL )
		{
			return;
		}

		if( IsCinematicGateActive( gameLocal, *player ) )
		{
			startup.sawCinematic = true;
			startup.clearSinceTime = -1;
			SetStartupDiagState( gameLocal, STARTUP_DIAG_WAITING_FOR_CINEMATIC_CLEAR );
			return;
		}

		if( startup.clearSinceTime < 0 )
		{
			startup.clearSinceTime = gameLocal.time;
			SetStartupDiagState( gameLocal,
								 startup.sawCinematic ? STARTUP_DIAG_WAITING_FOR_POST_CINEMATIC_DELAY : STARTUP_DIAG_WAITING_FOR_STARTUP_DELAY,
								 startup.sawCinematic ? SEC2MS( 1.0f ) : SEC2MS( 2.0f ) );
		}

		if( !startup.weaponGranted )
		{
			EnsureStartupWeapon( gameLocal, *player );
		}

		const bool encounterReady = startup.sawCinematic ? ( gameLocal.time - startup.clearSinceTime ) >= SEC2MS( 1.0f ) : gameLocal.time >= SEC2MS( 2.0f );
		if( !startup.monsterSpawned && encounterReady )
		{
			SetStartupDiagState( gameLocal, STARTUP_DIAG_READY_FOR_AUTO_TRIGGER );
			SpawnStartupMonster( gameLocal, *player, "auto" );
		}
	}

	void TriggerStartupEncounter( idGameLocal& gameLocal )
	{
		idPlayer* player = gameLocal.GetLocalPlayer();
		if( player == NULL )
		{
			Warn( gameLocal, "Cannot trigger startup encounter without a local player" );
			return;
		}

		if( IsCinematicGateActive( gameLocal, *player ) )
		{
			Warn( gameLocal, "Startup encounter trigger ignored while cinematic state is active" );
			return;
		}

		Log( gameLocal, "Received script startup encounter trigger at time=%d", gameLocal.time );

		if( !startup.weaponGranted )
		{
			EnsureStartupWeapon( gameLocal, *player );
		}

		if( !startup.monsterSpawned )
		{
			SpawnStartupMonster( gameLocal, *player, "script" );
		}
	}

private:
	static void Log( idGameLocal& gameLocal, const char* fmt, ... )
	{
		char text[1024];
		va_list argptr;
		va_start( argptr, fmt );
		idStr::vsnPrintf( text, sizeof( text ), fmt, argptr );
		va_end( argptr );
		gameLocal.Printf( "FFE: %s\n", text );
	}

	static void Warn( idGameLocal& gameLocal, const char* fmt, ... )
	{
		char text[1024];
		va_list argptr;
		va_start( argptr, fmt );
		idStr::vsnPrintf( text, sizeof( text ), fmt, argptr );
		va_end( argptr );
		gameLocal.Warning( "FFE: %s\n", text );
	}

	static bool IsCinematicGateActive( const idGameLocal& gameLocal, const idPlayer& player )
	{
		return gameLocal.inCinematic || player.IsHidden() || ( player.GetPrivateCameraView() != NULL ) || ( gameLocal.GetCamera() != NULL );
	}

	void SetStartupDiagState( idGameLocal& gameLocal, StartupDiagState state, int delayMs = 0 )
	{
		if( startup.diagState == state )
		{
			return;
		}

		startup.diagState = state;

		switch( state )
		{
			case STARTUP_DIAG_WAITING_FOR_CINEMATIC_CLEAR:
				Log( gameLocal, "Startup encounter waiting for cinematic/camera state to clear at time=%d", gameLocal.time );
				break;

			case STARTUP_DIAG_WAITING_FOR_POST_CINEMATIC_DELAY:
				Log( gameLocal, "Cinematic gate cleared; waiting %d ms before auto encounter", delayMs );
				break;

			case STARTUP_DIAG_WAITING_FOR_STARTUP_DELAY:
				Log( gameLocal, "No cinematic gate seen; waiting %d ms after startup before auto encounter", delayMs );
				break;

			case STARTUP_DIAG_READY_FOR_AUTO_TRIGGER:
				Log( gameLocal, "Startup encounter ready for automatic trigger at time=%d", gameLocal.time );
				break;

			case STARTUP_DIAG_NONE:
			default:
				break;
		}
	}

	bool EnsureStartupWeapon( idGameLocal& gameLocal, idPlayer& player )
	{
		if( gameLocal.world != NULL && gameLocal.world->spawnArgs.GetBool( "no_Weapons" ) )
		{
			gameLocal.world->spawnArgs.SetBool( "no_Weapons", false );
			player.ProcessEvent( &EV_Player_EnableWeapon );
			Log( gameLocal, "Cleared no_Weapons gate for startup loadout" );
		}

		const bool gaveWeapon = player.Give( "weapon", "weapon_machinegun", ITEM_GIVE_FEEDBACK | ITEM_GIVE_UPDATE_STATE );
		const bool gaveAmmo = player.Give( "ammo_clip", "120", ITEM_GIVE_FEEDBACK | ITEM_GIVE_UPDATE_STATE );
		const int machinegunSlot = player.SlotForWeapon( "weapon_machinegun" );
		const bool hasMachinegun = player.WeaponAvailable( "weapon_machinegun" );

		if( hasMachinegun && machinegunSlot >= 0 )
		{
			player.SelectWeapon( machinegunSlot, true );
		}

		if( hasMachinegun )
		{
			Log( gameLocal, "Startup weapon grant weapon=%d ammo=%d slot=%d no_Weapons=%d",
				 gaveWeapon, gaveAmmo, machinegunSlot,
				 gameLocal.world != NULL ? gameLocal.world->spawnArgs.GetBool( "no_Weapons" ) : 0 );
		}
		else
		{
			Warn( gameLocal, "Failed to grant machinegun weapon=%d ammo=%d slot=%d no_Weapons=%d",
				  gaveWeapon, gaveAmmo, machinegunSlot,
				  gameLocal.world != NULL ? gameLocal.world->spawnArgs.GetBool( "no_Weapons" ) : 0 );
		}

		startup.weaponGranted = true;
		return hasMachinegun;
	}

	bool SpawnStartupMonster( idGameLocal& gameLocal, idPlayer& player, const char* sourceTag )
	{
		idDict args;
		trace_t trace;
		trace_t floorTrace;
		const idBounds monsterBounds( idVec3( -24.0f, -24.0f, 0.0f ), idVec3( 24.0f, 24.0f, 68.0f ) );
		idVec3 forward = player.viewAngles.ToForward();
		forward[2] = 0.0f;

		if( forward.Normalize() == 0.0f )
		{
			forward = player.GetPhysics()->GetAxis()[0];
		}

		const idVec3 playerOrigin = player.GetPhysics()->GetOrigin();
		const idVec3 desiredOrigin = playerOrigin + forward * 160.0f;

		gameLocal.clip.TraceBounds( trace, playerOrigin, desiredOrigin, monsterBounds, MASK_MONSTERSOLID, &player );

		idVec3 spawnOrigin = trace.endpos;
		if( trace.fraction < 1.0f )
		{
			spawnOrigin -= forward * 32.0f;
		}

		const idVec3 floorStart = spawnOrigin + idVec3( 0.0f, 0.0f, 32.0f );
		const idVec3 floorEnd = spawnOrigin - idVec3( 0.0f, 0.0f, 256.0f );
		gameLocal.clip.TraceBounds( floorTrace, floorStart, floorEnd, monsterBounds, MASK_MONSTERSOLID, &player );
		if( floorTrace.fraction < 1.0f )
		{
			spawnOrigin = floorTrace.endpos;
		}

		args.Set( "classname", "monster_zombie_fat" );
		args.SetVector( "origin", spawnOrigin );
		args.SetFloat( "angle", idMath::AngleNormalize180( player.viewAngles.yaw + 180.0f ) );

		idEntity* ent = NULL;
		const bool spawned = gameLocal.SpawnEntityDef( args, &ent ) && ent != NULL;
		if( spawned )
		{
			Log( gameLocal, "Spawned test monster at (%.1f %.1f %.1f) in front of player time=%d sawCinematic=%d source=%s",
				 spawnOrigin.x, spawnOrigin.y, spawnOrigin.z, gameLocal.time, startup.sawCinematic, sourceTag );
		}
		else
		{
			Warn( gameLocal, "Failed to spawn test monster in front of player time=%d sawCinematic=%d source=%s",
				  gameLocal.time, startup.sawCinematic, sourceTag );
		}

		startup.monsterSpawned = true;
		return spawned;
	}

	StartupState startup;
};

}

void idGameLocal::FFE_ResetStartupState()
{
	FFEController::Get().ResetStartupState();
}

void idGameLocal::FFE_RunStartupFrame()
{
	if( gamestate != GAMESTATE_ACTIVE )
	{
		return;
	}

	FFEController::Get().RunStartupFrame( *this );
}

void idGameLocal::FFE_TriggerStartupEncounter()
{
	FFEController::Get().TriggerStartupEncounter( *this );
}
