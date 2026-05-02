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
	"1",
	CVAR_BOOL,
	"Run minimal application mode"
);

namespace
{

bool FFE_EnsureStartupWeapon( idGameLocal& gameLocal, idPlayer& player )
{
	if( gameLocal.world != NULL && gameLocal.world->spawnArgs.GetBool( "no_Weapons" ) )
	{
		gameLocal.world->spawnArgs.SetBool( "no_Weapons", false );
		player.ProcessEvent( &EV_Player_EnableWeapon );
		gameLocal.Printf( "FFE: Cleared no_Weapons gate for startup loadout\n" );
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
		gameLocal.Printf( "FFE: Startup weapon grant weapon=%d ammo=%d slot=%d no_Weapons=%d\n",
						  gaveWeapon, gaveAmmo, machinegunSlot,
						  gameLocal.world != NULL ? gameLocal.world->spawnArgs.GetBool( "no_Weapons" ) : 0 );
	}
	else
	{
		gameLocal.Warning( "FFE: Failed to grant machinegun weapon=%d ammo=%d slot=%d no_Weapons=%d\n",
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
		gameLocal.Printf( "FFE: Spawned test monster at (%.1f %.1f %.1f) in front of player time=%d sawCinematic=%d source=%s\n",
						  spawnOrigin.x, spawnOrigin.y, spawnOrigin.z, gameLocal.time, gameLocal.ffeSawCinematic, sourceTag );
	}
	else
	{
		gameLocal.Warning( "FFE: Failed to spawn test monster in front of player time=%d sawCinematic=%d source=%s\n",
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
		return;
	}

	if( ffeClearSinceTime < 0 )
	{
		ffeClearSinceTime = time;
	}

	if( !ffeWeaponGranted )
	{
		FFE_EnsureStartupWeapon( *this, *player );
	}

	const bool encounterReady = ffeSawCinematic ? ( time - ffeClearSinceTime ) >= SEC2MS( 1.0f ) : time >= SEC2MS( 2.0f );
	if( !ffeMonsterSpawned && encounterReady )
	{
		FFE_SpawnStartupMonster( *this, *player, "auto" );
	}
}

void idGameLocal::FFE_TriggerStartupEncounter()
{
	idPlayer* player = GetLocalPlayer();
	if( player == NULL )
	{
		Warning( "FFE: Cannot trigger startup encounter without a local player\n" );
		return;
	}

	if( FFE_IsCinematicGateActive( *this, *player ) )
	{
		Warning( "FFE: Startup encounter trigger ignored while cinematic state is active\n" );
		return;
	}

	if( !ffeWeaponGranted )
	{
		FFE_EnsureStartupWeapon( *this, *player );
	}

	if( !ffeMonsterSpawned )
	{
		FFE_SpawnStartupMonster( *this, *player, "script" );
	}
}
