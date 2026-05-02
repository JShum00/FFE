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

enum ffeStartupDiagState_t
{
	FFE_STARTUP_DIAG_NONE,
	FFE_STARTUP_DIAG_WAITING_FOR_CINEMATIC_CLEAR,
	FFE_STARTUP_DIAG_WAITING_FOR_POST_CINEMATIC_DELAY,
	FFE_STARTUP_DIAG_WAITING_FOR_STARTUP_DELAY,
	FFE_STARTUP_DIAG_READY_FOR_AUTO_TRIGGER
};

ffeStartupDiagState_t g_ffeStartupDiagState = FFE_STARTUP_DIAG_NONE;

void FFE_Log( idGameLocal& gameLocal, const char* fmt, ... )
{
	char text[1024];
	va_list argptr;
	va_start( argptr, fmt );
	idStr::vsnPrintf( text, sizeof( text ), fmt, argptr );
	va_end( argptr );
	gameLocal.Printf( "FFE: %s\n", text );
}

void FFE_Warn( idGameLocal& gameLocal, const char* fmt, ... )
{
	char text[1024];
	va_list argptr;
	va_start( argptr, fmt );
	idStr::vsnPrintf( text, sizeof( text ), fmt, argptr );
	va_end( argptr );
	gameLocal.Warning( "FFE: %s\n", text );
}

void FFE_SetStartupDiagState( idGameLocal& gameLocal, ffeStartupDiagState_t state, int delayMs = 0 )
{
	if( g_ffeStartupDiagState == state )
	{
		return;
	}

	g_ffeStartupDiagState = state;

	switch( state )
	{
		case FFE_STARTUP_DIAG_WAITING_FOR_CINEMATIC_CLEAR:
			FFE_Log( gameLocal, "Startup encounter waiting for cinematic/camera state to clear at time=%d", gameLocal.time );
			break;

		case FFE_STARTUP_DIAG_WAITING_FOR_POST_CINEMATIC_DELAY:
			FFE_Log( gameLocal, "Cinematic gate cleared; waiting %d ms before auto encounter", delayMs );
			break;

		case FFE_STARTUP_DIAG_WAITING_FOR_STARTUP_DELAY:
			FFE_Log( gameLocal, "No cinematic gate seen; waiting %d ms after startup before auto encounter", delayMs );
			break;

		case FFE_STARTUP_DIAG_READY_FOR_AUTO_TRIGGER:
			FFE_Log( gameLocal, "Startup encounter ready for automatic trigger at time=%d", gameLocal.time );
			break;

		case FFE_STARTUP_DIAG_NONE:
		default:
			break;
	}
}

bool FFE_EnsureStartupWeapon( idGameLocal& gameLocal, idPlayer& player )
{
	if( gameLocal.world != NULL && gameLocal.world->spawnArgs.GetBool( "no_Weapons" ) )
	{
		gameLocal.world->spawnArgs.SetBool( "no_Weapons", false );
		player.ProcessEvent( &EV_Player_EnableWeapon );
		FFE_Log( gameLocal, "Cleared no_Weapons gate for startup loadout" );
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
		FFE_Log( gameLocal, "Startup weapon grant weapon=%d ammo=%d slot=%d no_Weapons=%d",
				 gaveWeapon, gaveAmmo, machinegunSlot,
				 gameLocal.world != NULL ? gameLocal.world->spawnArgs.GetBool( "no_Weapons" ) : 0 );
	}
	else
	{
		FFE_Warn( gameLocal, "Failed to grant machinegun weapon=%d ammo=%d slot=%d no_Weapons=%d",
				  gaveWeapon, gaveAmmo, machinegunSlot,
				  gameLocal.world != NULL ? gameLocal.world->spawnArgs.GetBool( "no_Weapons" ) : 0 );
	}

	gameLocal.ffeWeaponGranted = true;
	return hasMachinegun;
}

bool FFE_SpawnStartupMonster( idGameLocal& gameLocal, idPlayer& player, const char* sourceTag )
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
		FFE_Log( gameLocal, "Spawned test monster at (%.1f %.1f %.1f) in front of player time=%d sawCinematic=%d source=%s",
				 spawnOrigin.x, spawnOrigin.y, spawnOrigin.z, gameLocal.time, gameLocal.ffeSawCinematic, sourceTag );
	}
	else
	{
		FFE_Warn( gameLocal, "Failed to spawn test monster in front of player time=%d sawCinematic=%d source=%s",
				  gameLocal.time, gameLocal.ffeSawCinematic, sourceTag );
	}

	gameLocal.ffeMonsterSpawned = true;
	return spawned;
}

bool FFE_IsCinematicGateActive( const idGameLocal& gameLocal, const idPlayer& player )
{
	return gameLocal.inCinematic || player.IsHidden() || ( player.GetPrivateCameraView() != NULL ) || ( gameLocal.GetCamera() != NULL );
}

}

void idGameLocal::FFE_ResetStartupState()
{
	ffeMonsterSpawned = false;
	ffeWeaponGranted = false;
	ffeSawCinematic = false;
	ffeClearSinceTime = -1;
	g_ffeStartupDiagState = FFE_STARTUP_DIAG_NONE;
}

void idGameLocal::FFE_RunStartupFrame( idPlayer* player )
{
	if( gamestate != GAMESTATE_ACTIVE || player == NULL )
	{
		return;
	}

	if( FFE_IsCinematicGateActive( *this, *player ) )
	{
		ffeSawCinematic = true;
		ffeClearSinceTime = -1;
		FFE_SetStartupDiagState( *this, FFE_STARTUP_DIAG_WAITING_FOR_CINEMATIC_CLEAR );
		return;
	}

	if( ffeClearSinceTime < 0 )
	{
		ffeClearSinceTime = time;
		FFE_SetStartupDiagState( *this,
								 ffeSawCinematic ? FFE_STARTUP_DIAG_WAITING_FOR_POST_CINEMATIC_DELAY : FFE_STARTUP_DIAG_WAITING_FOR_STARTUP_DELAY,
								 ffeSawCinematic ? SEC2MS( 1.0f ) : SEC2MS( 2.0f ) );
	}

	if( !ffeWeaponGranted )
	{
		FFE_EnsureStartupWeapon( *this, *player );
	}

	const bool encounterReady = ffeSawCinematic ? ( time - ffeClearSinceTime ) >= SEC2MS( 1.0f ) : time >= SEC2MS( 2.0f );
	if( !ffeMonsterSpawned && encounterReady )
	{
		FFE_SetStartupDiagState( *this, FFE_STARTUP_DIAG_READY_FOR_AUTO_TRIGGER );
		FFE_SpawnStartupMonster( *this, *player, "auto" );
	}
}

void idGameLocal::FFE_TriggerStartupEncounter()
{
	idPlayer* player = GetLocalPlayer();
	if( player == NULL )
	{
		FFE_Warn( *this, "Cannot trigger startup encounter without a local player" );
		return;
	}

	if( FFE_IsCinematicGateActive( *this, *player ) )
	{
		FFE_Warn( *this, "Startup encounter trigger ignored while cinematic state is active" );
		return;
	}

	FFE_Log( *this, "Received script startup encounter trigger at time=%d", time );

	if( !ffeWeaponGranted )
	{
		FFE_EnsureStartupWeapon( *this, *player );
	}

	if( !ffeMonsterSpawned )
	{
		FFE_SpawnStartupMonster( *this, *player, "script" );
	}
}
