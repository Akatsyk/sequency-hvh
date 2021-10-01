#include "c_trace_system.h"
#include "../utils/math.h"
#include "../sdk/c_weapon_system.h"
#include "../sdk/c_engine_trace.h"
#include "../sdk/c_client_entity_list.h"
#include "../sdk/c_surface_props.h"
#include "../utils/c_config.h"
#include "../sdk/c_base_client.h"
#include "c_aimhelper.h"












#include <Windows.h>
#include <Psapi.h>



#define INRANGE(x,a,b)  (x >= a && x <= b) 
#define getBits( x )    (INRANGE((x&(~0x20)),'A','F') ? ((x&(~0x20)) - 'A' + 0xa) : (INRANGE(x,'0','9') ? x - '0' : 0))
#define getByte( x )    (getBits(x[0]) << 4 | getBits(x[1]))

CAutowall* g_Autowall = new CAutowall;

#define    HITGROUP_GENERIC    0
#define    HITGROUP_HEAD        1
#define    HITGROUP_CHEST        2
#define    HITGROUP_STOMACH    3
#define HITGROUP_LEFTARM    4    
#define HITGROUP_RIGHTARM    5
#define HITGROUP_LEFTLEG    6
#define HITGROUP_RIGHTLEG    7
#define HITGROUP_GEAR        10

void ScaleDamage(int hitgroup, c_cs_player* enemy, float weapon_armor_ratio, float& current_damage)
{
	int armor = enemy->get_armor();
	float ratio;

	switch (hitgroup)
	{
	case HITGROUP_HEAD:
		current_damage *= 4.f;
		break;
	case HITGROUP_STOMACH:
		current_damage *= 1.25f;
		break;
	case HITGROUP_LEFTLEG:
	case HITGROUP_RIGHTLEG:
		current_damage *= 0.75f;
		break;
	}

	if (armor > 0)
	{
		switch (hitgroup)
		{
		case HITGROUP_HEAD:
			if (enemy->has_helmet())
			{
				ratio = (weapon_armor_ratio * 0.5) * current_damage;
				if (((current_damage - ratio) * 0.5) > armor)
					ratio = current_damage - (armor * 2.0);
				current_damage = ratio;
			}
			break;
		case HITGROUP_GENERIC:
		case HITGROUP_CHEST:
		case HITGROUP_STOMACH:
		case HITGROUP_LEFTARM:
		case HITGROUP_RIGHTARM:
			ratio = (weapon_armor_ratio * 0.5) * current_damage;
			if (((current_damage - ratio) * 0.5) > armor)
				ratio = current_damage - (armor * 2.0);
			current_damage = ratio;
			break;
		}
	}
}
uint64_t FindSignature2(const char* szModule, const char* szSignature)
{
	MODULEINFO modInfo;
	GetModuleInformation(GetCurrentProcess(), GetModuleHandleA(szModule), &modInfo, sizeof(MODULEINFO));
	DWORD startAddress = (DWORD)modInfo.lpBaseOfDll;
	DWORD endAddress = startAddress + modInfo.SizeOfImage;
	const char* pat = szSignature;
	DWORD firstMatch = 0;
	for (DWORD pCur = startAddress; pCur < endAddress; pCur++) {
		if (!*pat) return firstMatch;
		if (*(PBYTE)pat == '\?' || *(BYTE*)pCur == getByte(pat)) {
			if (!firstMatch) firstMatch = pCur;
			if (!pat[2]) return firstMatch;
			if (*(PWORD)pat == '\?\?' || *(PBYTE)pat != '\?') pat += 3;
			else pat += 2;    //one ?
		}
		else {
			pat = szSignature;
			firstMatch = 0;
		}
	}
	return NULL;
}
void CAutowall::TraceLine(c_vector3d& absStart, c_vector3d& absEnd, unsigned int mask, c_base_entity* ignore, trace* ptr)
{
	ray_t ray;
	ray.Init(absStart, absEnd);
	c_trace_filter filter;
	filter.skip_entity = ignore;

	engine_trace()->trace_ray(ray, mask, &filter, ptr);
}

void CAutowall::ClipTraceToPlayers(c_vector3d& absStart, c_vector3d absEnd, unsigned int mask, c_trace_filter* filter, trace* tr)
{
	//static DWORD ClipTraceToPlayers = FindPatternIDA(client_dll, "53 8B DC 83 EC 08 83 E4 F0 83 C4 04 55 8B 6B 04 89 6C 24 04 8B EC 81 EC ? ? ? ? 8B 43 10");
	//static DWORD ClipTraceToPlayers = reinterpret_cast<void*>(pattern::first_code_match(g_csgo.m_chl.dll(), "53 8B DC 83 EC 08 83 E4 F0 83 C4 04 55 8B 6B 04 89 6C 24 04 8B EC 81 EC ? ? ? ? 8B 43 10"));

	static DWORD ClipTraceToPlayers = FindSignature2("client.dll", "53 8B DC 83 EC 08 83 E4 F0 83 C4 04 55 8B 6B 04 89 6C 24 04 8B EC 81 EC ? ? ? ? 8B 43 10");

	if (!ClipTraceToPlayers)
		return;

	_asm
	{
		MOV		EAX, filter
		LEA		ECX, tr
		PUSH	ECX
		PUSH	EAX
		PUSH	mask
		LEA		EDX, absEnd
		LEA		ECX, absStart
		CALL	ClipTraceToPlayers
		ADD		ESP, 0xC
	}
}

////////////////////////////////////// Legacy Functions //////////////////////////////////////
void CAutowall::GetBulletTypeParameters(float& maxRange, float& maxDistance, char* bulletType, bool sv_penetration_type)
{
	if (sv_penetration_type)
	{
		maxRange = 35.0;
		maxDistance = 3000.0;
	}
	else
	{
		//Play tribune to framerate. Thanks, stringcompare
		//Regardless I doubt anyone will use the old penetration system anyway; so it won't matter much.
		if (!strcmp(bulletType, ("BULLET_PLAYER_338MAG")))
		{
			maxRange = 45.0;
			maxDistance = 8000.0;
		}
		if (!strcmp(bulletType, ("BULLET_PLAYER_762MM")))
		{
			maxRange = 39.0;
			maxDistance = 5000.0;
		}
		if (!strcmp(bulletType, ("BULLET_PLAYER_556MM")) || !strcmp(bulletType, ("BULLET_PLAYER_556MM_SMALL")) || !strcmp(bulletType, ("BULLET_PLAYER_556MM_BOX")))
		{
			maxRange = 35.0;
			maxDistance = 4000.0;
		}
		if (!strcmp(bulletType, ("BULLET_PLAYER_57MM")))
		{
			maxRange = 30.0;
			maxDistance = 2000.0;
		}
		if (!strcmp(bulletType, ("BULLET_PLAYER_50AE")))
		{
			maxRange = 30.0;
			maxDistance = 1000.0;
		}
		if (!strcmp(bulletType, ("BULLET_PLAYER_357SIG")) || !strcmp(bulletType, ("BULLET_PLAYER_357SIG_SMALL")) || !strcmp(bulletType, ("BULLET_PLAYER_357SIG_P250")) || !strcmp(bulletType, ("BULLET_PLAYER_357SIG_MIN")))
		{
			maxRange = 25.0;
			maxDistance = 800.0;
		}
		if (!strcmp(bulletType, ("BULLET_PLAYER_9MM")))
		{
			maxRange = 21.0;
			maxDistance = 800.0;
		}
		if (!strcmp(bulletType, ("BULLET_PLAYER_45ACP")))
		{
			maxRange = 15.0;
			maxDistance = 500.0;
		}
		if (!strcmp(bulletType, ("BULLET_PLAYER_BUCKSHOT")))
		{
			maxRange = 0.0;
			maxDistance = 0.0;
		}
	}
}

////////////////////////////////////// Misc Functions //////////////////////////////////////
bool CAutowall::IsBreakableEntity(c_client_entity* entity)
{

	//client_class* pClass = (client_class*)entity->get_client_class();

	///client_class_t* pclass = entity->get_client_class();
	//c_client_class* pclass = base_client()->get_all_classes();
	client_class* cclass = entity->get_client_class(); // ok
	if (!cclass)
		return false;

	return cclass->id == 30 || cclass->id == 31;

}

void CAutowall::ScaleDamage(trace& enterTrace, c_base_combat_weapon::weapon_data* weaponData, float& currentDamage)
{
	//Cred. to N0xius for reversing this.
	//TODO: _xAE^; look into reversing this yourself sometime

	bool hasHeavyArmor = false;
	int armorValue = ((c_cs_player*)enterTrace.entity)->get_armor();
	int hitGroup = enterTrace.hitgroup;

	//Fuck making a new function, lambda beste. ~ Does the person have armor on for the hitbox checked?
	auto IsArmored = [&enterTrace]()->bool
	{
		c_cs_player* targetEntity = (c_cs_player*)enterTrace.entity;
		switch (enterTrace.hitgroup)
		{
		case HITGROUP_HEAD:
			return !!(c_cs_player*)targetEntity->has_helmet(); //Fuck compiler errors - force-convert it to a bool via (!!)
		case HITGROUP_GENERIC:
		case HITGROUP_CHEST:
		case HITGROUP_STOMACH:
		case HITGROUP_LEFTARM:
		case HITGROUP_RIGHTARM:
			return true;
		default:
			return false;
		}
	};

	switch (hitGroup)
	{
	case HITGROUP_HEAD:
		currentDamage *= 2.f; //Heavy Armor does 1/2 damage
		break;
	case HITGROUP_STOMACH:
		currentDamage *= 1.25f;
		break;
	case HITGROUP_LEFTLEG:
	case HITGROUP_RIGHTLEG:
		currentDamage *= 0.75f;
		break;
	default:
		break;
	}

	if (armorValue > 0 && IsArmored())
	{
		float bonusValue = 1.f, armorBonusRatio = 0.5f, armorRatio = weaponData->flArmorRatio / 2.f;

		//Damage gets modified for heavy armor users
		if (hasHeavyArmor)
		{
			armorBonusRatio = 0.33f;
			armorRatio *= 0.5f;
			bonusValue = 0.33f;
		}

		auto NewDamage = currentDamage * armorRatio;

		if (hasHeavyArmor)
			NewDamage *= 0.85f;

		if (((currentDamage - (currentDamage * armorRatio)) * (bonusValue * armorBonusRatio)) > armorValue)
			NewDamage = currentDamage - (armorValue / armorBonusRatio);

		currentDamage = NewDamage;
	}
}

#define	SURF_LIGHT		0x0001		// value will hold the light strength
#define	SURF_SKY2D		0x0002		// don't draw, indicates we should skylight + draw 2d sky but not draw the 3D skybox
#define	SURF_SKY		0x0004		// don't draw, but add to skybox
#define	SURF_WARP		0x0008		// turbulent water warp
#define	SURF_TRANS		0x0010
#define SURF_NOPORTAL	0x0020	// the surface can not have a portal placed on it
#define	SURF_TRIGGER	0x0040	// FIXME: This is an xbox hack to work around elimination of trigger surfaces, which breaks occluders
#define	SURF_NODRAW		0x0080	// don't bother referencing the texture

#define	SURF_HINT		0x0100	// make a primary bsp splitter

#define	SURF_SKIP		0x0200	// completely ignore, allowing non-closed brushes
#define SURF_NOLIGHT	0x0400	// Don't calculate light
#define SURF_BUMPLIGHT	0x0800	// calculate three lightmaps for the surface for bumpmapping
#define SURF_NOSHADOWS	0x1000	// Don't receive shadows
#define SURF_NODECALS	0x2000	// Don't receive decals
#define SURF_NOCHOP		0x4000	// Don't subdivide patches on this surface 
#define SURF_HITBOX		0x8000	// surface is part of a hitbox
////////////////////////////////////// Main Autowall Functions //////////////////////////////////////
bool CAutowall::TraceToExit(trace& enterTrace, trace& exitTrace, c_vector3d startPosition, c_vector3d direction)
{
	/*
	Masks used:
	MASK_SHOT_HULL					 = 0x600400B
	CONTENTS_HITBOX					 = 0x40000000
	MASK_SHOT_HULL | CONTENTS_HITBOX = 0x4600400B
	*/

	c_vector3d start, end;
	float maxDistance = 90.f, rayExtension = 4.f, currentDistance = 0;
	int firstContents = 0;

	while (currentDistance <= maxDistance)
	{
		//Add extra distance to our ray
		currentDistance += rayExtension;

		//Multiply the direction c_vector3d to the distance so we go outwards, add our position to it.
		start = startPosition + direction * currentDistance;

		if (!firstContents)
			firstContents = engine_trace()->get_point_contents_n(start, mask_shot_hull | contents_hitbox, nullptr); /*0x4600400B*/
		int pointContents = engine_trace()->get_point_contents_n(start, mask_shot_hull | contents_hitbox, nullptr);

		if (!(pointContents & mask_shot_hull) || pointContents & contents_hitbox && pointContents != firstContents) /*0x600400B, *0x40000000*/
		{
			//Let's setup our end position by deducting the direction by the extra added distance
			end = start - (direction * rayExtension);

			//Let's cast a ray from our start pos to the end pos
			TraceLine(start, end, mask_shot_hull | contents_hitbox, nullptr, &exitTrace);

			//Let's check if a hitbox is in-front of our enemy and if they are behind of a solid wall
			if (exitTrace.startsolid && exitTrace.surface.flags & SURF_HITBOX)
			{
				TraceLine(start, startPosition, mask_shot_hull, exitTrace.entity, &exitTrace);

				if (exitTrace.did_hit() && !exitTrace.startsolid)
				{
					start = exitTrace.endpos;
					return true;
				}

				continue;
			}

			//Can we hit? Is the wall solid?
			if (exitTrace.did_hit() && !exitTrace.startsolid)
			{

				//Is the wall a breakable? If so, let's shoot through it.
				if (IsBreakableEntity(enterTrace.entity) && IsBreakableEntity(exitTrace.entity))
					return true;

				if (enterTrace.surface.flags & SURF_NODRAW || !(exitTrace.surface.flags & SURF_NODRAW) && (exitTrace.plane.normal.dot(direction) <= 1.f))
				{
					float multAmount = exitTrace.fraction * 4.f;
					start -= direction * multAmount;
					return true;
				}

				continue;
			}

			if (!exitTrace.did_hit() || exitTrace.startsolid)
			{
				if (enterTrace.did_hit() && IsBreakableEntity(enterTrace.entity))
				{
					exitTrace = enterTrace;
					exitTrace.endpos = start + direction;
					return true;
				}

				continue;
			}
		}
	}
	return false;
}
#define CHAR_TEX_ANTLION		'A'
#define CHAR_TEX_BLOODYFLESH	'B'
#define	CHAR_TEX_CONCRETE		'C'
#define CHAR_TEX_DIRT			'D'
#define CHAR_TEX_EGGSHELL		'E' ///< the egg sacs in the tunnels in ep2.
#define CHAR_TEX_FLESH			'F'
#define CHAR_TEX_GRATE			'G'
#define CHAR_TEX_ALIENFLESH		'H'
#define CHAR_TEX_CLIP			'I'
//#define CHAR_TEX_UNUSED		'J'
//#define CHAR_TEX_UNUSED		'K'
#define CHAR_TEX_PLASTIC		'L'
#define CHAR_TEX_METAL			'M'
#define CHAR_TEX_SAND			'N'
#define CHAR_TEX_FOLIAGE		'O'
#define CHAR_TEX_COMPUTER		'P'
//#define CHAR_TEX_UNUSED		'Q'
//#define CHAR_TEX_UNUSED		'R'
#define CHAR_TEX_SLOSH			'S'
#define CHAR_TEX_TILE			'T'
#define CHAR_TEX_CARDBOARD		'U'
#define CHAR_TEX_VENT			'V'
#define CHAR_TEX_WOOD			'W'
//#define CHAR_TEX_UNUSED		'X'
#define CHAR_TEX_GLASS			'Y'
#define CHAR_TEX_WARPSHIELD		'Z' ///< wierd-looking jello effect for advisor shield.

bool CAutowall::HandleBulletPenetration(c_base_combat_weapon::weapon_data* weaponData, trace& enterTrace, c_vector3d& eyePosition, c_vector3d direction, int& possibleHitsRemaining, float& currentDamage, float penetrationPower, bool sv_penetration_type, float ff_damage_reduction_bullets, float ff_damage_bullet_penetration)
{
	//Because there's been issues regarding this- putting this here.
	if (&currentDamage == nullptr)
		throw std::invalid_argument("currentDamage is null!");
	auto local = c_cs_player::get_local_player();
	if (!local)
		return false;
	auto data = FireBulletData(local->get_shoot_position());
	data.filter = c_trace_filter();
	data.filter.skip_entity = local;
	trace exitTrace;
	c_cs_player* pEnemy = (c_cs_player*)enterTrace.entity;
	surfacedata_t* enterSurfaceData = surface_props()->get_surface_data(enterTrace.surface.surface_props);
	int enterMaterial = enterSurfaceData->game.material;

	float enterSurfPenetrationModifier = enterSurfaceData->game.penetration_mod;
	float enterDamageModifier = enterSurfaceData->game.damage_mod;
	float thickness, modifier, lostDamage, finalDamageModifier, combinedPenetrationModifier;
	bool isSolidSurf = ((enterTrace.contents >> 3) & contents_solid);
	bool isLightSurf = ((enterTrace.surface.flags >> 7) & SURF_LIGHT);

	if (possibleHitsRemaining <= 0
		//Test for "DE_CACHE/DE_CACHE_TELA_03" as the entering surface and "CS_ITALY/CR_MISCWOOD2B" as the exiting surface.
		//Fixes a wall in de_cache which seems to be broken in some way. Although bullet penetration is not recorded to go through this wall
		//Decals can be seen of bullets within the glass behind of the enemy. Hacky method, but works.
		//You might want to have a check for this to only be activated on de_cache.
		|| (enterTrace.surface.name == (const char*)0x2227c261 && exitTrace.surface.name == (const char*)0x2227c868)
		|| (!possibleHitsRemaining && !isLightSurf && !isSolidSurf && enterMaterial != CHAR_TEX_GRATE && enterMaterial != CHAR_TEX_GLASS)
		|| weaponData->flPenetration <= 0.f
		|| !TraceToExit(enterTrace, exitTrace, enterTrace.endpos, direction)
		&& !(engine_trace()->get_point_contents_n(enterTrace.endpos, mask_shot_hull, nullptr) & mask_shot_hull))
		return false;

	surfacedata_t* exitSurfaceData = surface_props()->get_surface_data(exitTrace.surface.surface_props);
	int exitMaterial = exitSurfaceData->game.material;
	float exitSurfPenetrationModifier = exitSurfaceData->game.penetration_mod;
	float exitDamageModifier = exitSurfaceData->game.damage_mod;

	//Are we using the newer penetration system?
	if (sv_penetration_type)
	{
		if (enterMaterial == CHAR_TEX_GRATE || enterMaterial == CHAR_TEX_GLASS)
		{
			combinedPenetrationModifier = 3.f;
			finalDamageModifier = 0.05f;
		}
		else if (isSolidSurf || isLightSurf)
		{
			combinedPenetrationModifier = 1.f;
			finalDamageModifier = 0.16f;
		}
		else if (enterMaterial == CHAR_TEX_FLESH && (local->get_team() == pEnemy->get_team() && ff_damage_reduction_bullets == 0.f)) //TODO: Team check config
		{
			//Look's like you aren't shooting through your teammate today
			if (ff_damage_bullet_penetration == 0.f)
				return false;

			//Let's shoot through teammates and get kicked for teamdmg! Whatever, atleast we did damage to the enemy. I call that a win.
			combinedPenetrationModifier = ff_damage_bullet_penetration;
			finalDamageModifier = 0.16f;
		}
		else
		{
			combinedPenetrationModifier = (enterSurfPenetrationModifier + exitSurfPenetrationModifier) / 2.f;
			finalDamageModifier = 0.16f;
		}

		//Do our materials line up?
		if (enterMaterial == exitMaterial)
		{
			if (exitMaterial == CHAR_TEX_CARDBOARD || exitMaterial == CHAR_TEX_WOOD)
				combinedPenetrationModifier = 3.f;
			else if (exitMaterial == CHAR_TEX_PLASTIC)
				combinedPenetrationModifier = 2.f;
		}

		//Calculate thickness of the wall by getting the length of the range of the trace and squaring
		thickness = (exitTrace.endpos - enterTrace.endpos).lengthsqr();
		modifier = fmaxf(1.f / combinedPenetrationModifier, 0.f);

		//This calculates how much damage we've lost depending on thickness of the wall, our penetration, damage, and the modifiers set earlier
		lostDamage = fmaxf(
			((modifier * thickness) / 24.f)
			+ ((currentDamage * finalDamageModifier)
				+ (fmaxf(3.75f / penetrationPower, 0.f) * 3.f * modifier)), 0.f);

		//Did we loose too much damage?
		if (lostDamage > currentDamage)
			return false;

		//We can't use any of the damage that we've lost
		if (lostDamage > 0.f)
			currentDamage -= lostDamage;

		//Do we still have enough damage to deal?
		if (currentDamage < 1.f)
			return false;

		eyePosition = exitTrace.endpos;
		--possibleHitsRemaining;

		return true;
	}
	else //Legacy penetration system
	{
		combinedPenetrationModifier = 1.f;

		if (isSolidSurf || isLightSurf)
			finalDamageModifier = 0.99f; //Good meme :^)
		else
		{
			finalDamageModifier = fminf(enterDamageModifier, exitDamageModifier);
			combinedPenetrationModifier = fminf(enterSurfPenetrationModifier, exitSurfPenetrationModifier);
		}

		if (enterMaterial == exitMaterial && (exitMaterial == CHAR_TEX_METAL || exitMaterial == CHAR_TEX_WOOD))
			combinedPenetrationModifier += combinedPenetrationModifier;

		thickness = (exitTrace.endpos - enterTrace.endpos).lengthsqr();

		if (sqrt(thickness) <= combinedPenetrationModifier * penetrationPower)
		{
			currentDamage *= finalDamageModifier;
			eyePosition = exitTrace.endpos;
			--possibleHitsRemaining;

			return true;
		}

		return false;
	}
}

bool CAutowall::FireBullet(c_vector3d& direction, float& currentDamage)
{
	auto local = c_cs_player::get_local_player();
	if (!local)
		return false;
	const auto pWeapon = reinterpret_cast<c_base_combat_weapon*>(client_entity_list()->get_client_entity_from_handle(local->get_current_weapon_handle()));


	if (!pWeapon)
		return false;


	const auto wpn_info = weapon_system->get_weapon_data(pWeapon->get_item_definition());
	if (!wpn_info)
		return false;

	auto data = FireBulletData(local->get_shoot_position());
	data.filter = c_trace_filter();
	data.filter.skip_entity = local;
	bool sv_penetration_type;
	//	  Current bullet travel Power to penetrate Distance to penetrate Range               Player bullet reduction convars			  Amount to extend ray by
	float currentDistance = 0.f, penetrationPower, penetrationDistance, maxRange, ff_damage_reduction_bullets, ff_damage_bullet_penetration, rayExtension = 40.f;
	c_vector3d eyePosition = local->get_shoot_position();

	//For being superiour when the server owners think your autowall isn't well reversed. Imagine a meme HvH server with the old penetration system- pff
	static convar* penetrationSystem = cvar()->find_var(("sv_penetration_type"));
	static convar* damageReductionBullets = cvar()->find_var(("ff_damage_reduction_bullets"));
	static convar* damageBulletPenetration = cvar()->find_var(("ff_damage_bullet_penetration"));

	sv_penetration_type = penetrationSystem->get_bool();
	ff_damage_reduction_bullets = damageReductionBullets->get_float();
	ff_damage_bullet_penetration = damageBulletPenetration->get_float();

	c_base_combat_weapon::weapon_data* weaponData = wpn_info;
	trace enterTrace;

	//We should be skipping localplayer when casting a ray to players.
	c_trace_filter filter;
	filter.skip_entity = local;

	if (!weaponData)
		return false;

	maxRange = weaponData->flRange;

	GetBulletTypeParameters(penetrationPower, penetrationDistance, weaponData->szHudName, sv_penetration_type);

	if (sv_penetration_type)
		penetrationPower = weaponData->flPenetration;

	//This gets set in FX_Firebullets to 4 as a pass-through value.
	//CS:GO has a maximum of 4 surfaces a bullet can pass-through before it 100% stops.
	//Excerpt from Valve: https://steamcommunity.com/sharedfiles/filedetails/?id=275573090
	//"The total number of surfaces any bullet can penetrate in a single flight is capped at 4." -CS:GO Official
	int possibleHitsRemaining = 4;

	//Set our current damage to what our gun's initial damage reports it will do
	currentDamage = weaponData->iDamage;

	//If our damage is greater than (or equal to) 1, and we can shoot, let's shoot.
	while (possibleHitsRemaining > 0 && currentDamage >= 1.f)
	{
		//Calculate max bullet range
		maxRange -= currentDistance;

		//Create endpoint of bullet
		c_vector3d end = eyePosition + direction * maxRange;

		TraceLine(eyePosition, end, mask_shot_hull | contents_hitbox, local, &enterTrace);
		ClipTraceToPlayers(eyePosition, end + direction * rayExtension, mask_shot_hull | contents_hitbox, &filter, &enterTrace);

		//We have to do this *after* tracing to the player.
		surfacedata_t* enterSurfaceData = surface_props()->get_surface_data(enterTrace.surface.surface_props);
		float enterSurfPenetrationModifier = enterSurfaceData->game.penetration_mod;
		int enterMaterial = enterSurfaceData->game.material;

		//"Fraction == 1" means that we didn't hit anything. We don't want that- so let's break on it.
		if (enterTrace.fraction == 1.f)
			break;

		//calculate the damage based on the distance the bullet traveled.
		currentDistance += enterTrace.fraction * maxRange;

		//Let's make our damage drops off the further away the bullet is.
		currentDamage *= pow(weaponData->flRangeModifier, (currentDistance / 500.f));

		//Sanity checking / Can we actually shoot through?
		if (currentDistance > penetrationDistance && weaponData->flPenetration > 0.f || enterSurfPenetrationModifier < 0.1f)
			break;

		//This looks gay as fuck if we put it into 1 long line of code.
		bool canDoDamage = (enterTrace.hitgroup != HITGROUP_GEAR && enterTrace.hitgroup != HITGROUP_GENERIC);
		//	bool isPlayer = enterTrace.m_pEnt->GetClientClass()->m_ClassID == CSClasses::CCSPlayer);
		bool isEnemy = (((c_cs_player*)local)->get_team() != ((c_cs_player*)enterTrace.entity)->get_team());
		//	bool onTeam = (((c_cs_player*)enterTrace.m_pEnt)->GetTeamNum() == (int32_t)TEAM_CT || ((c_cs_player*)enterTrace.m_pEnt)->GetTeamNum() == (int32_t)TeamNumbers::TEAM_T);

		//TODO: Team check config
		if ((canDoDamage && isEnemy))
		{
			ScaleDamage(enterTrace, weaponData, currentDamage);
			return true;
		}

		//Calling HandleBulletPenetration here reduces our penetrationCounter, and if it returns true, we can't shoot through it.
		if (!HandleBulletPenetration(weaponData, enterTrace, eyePosition, direction, possibleHitsRemaining, currentDamage, penetrationPower, sv_penetration_type, ff_damage_reduction_bullets, ff_damage_bullet_penetration))
			break;
	}
	return false;
}

////////////////////////////////////// Usage Calls //////////////////////////////////////
float CAutowall::CanHit(c_vector3d& point)
{
	auto local = c_cs_player::get_local_player();
	if (!local)
		return 0.f;

	auto data = FireBulletData(local->get_shoot_position());
	data.filter = c_trace_filter();
	data.filter.skip_entity = local;
	c_vector3d angles, direction;
	c_vector3d tmp = point - local->get_shoot_position();
	float currentDamage = 0;

	math::vector_angles(tmp, angles);
	math::angle_vector(angles, &direction);
	direction.normalize_in_place();

	if (FireBullet(direction, currentDamage))
		return currentDamage;
	return -1; //That wall is just a bit too thick buddy
}


bool CAutowall::PenetrateWall(c_cs_player* pBaseEntity, c_vector3d& vecPoint)
{
	const auto weapon_cfg = c_aimhelper::get_weapon_conf();
	if (!weapon_cfg.has_value())
		return false;

	float min_damage = weapon_cfg.value().min_dmg;
	if (pBaseEntity->get_health() < min_damage)
		min_damage = pBaseEntity->get_health();

	if (CanHit(vecPoint) >= min_damage)
		return true;

	return false;
}


std::uint8_t* PatternScan(void* module, const char* signature)
{
	static auto pattern_to_byte = [](const char* pattern) {
		auto bytes = std::vector<int>{};
		auto start = const_cast<char*>(pattern);
		auto end = const_cast<char*>(pattern) + strlen(pattern);

		for (auto current = start; current < end; ++current) {
			if (*current == '?') {
				++current;
				if (*current == '?')
					++current;
				bytes.push_back(-1);
			}
			else {
				bytes.push_back(strtoul(current, &current, 16));
			}
		}
		return bytes;
	};

	auto dosHeader = (PIMAGE_DOS_HEADER)module;
	auto ntHeaders = (PIMAGE_NT_HEADERS)((std::uint8_t*)module + dosHeader->e_lfanew);

	auto sizeOfImage = ntHeaders->OptionalHeader.SizeOfImage;
	auto patternBytes = pattern_to_byte(signature);
	auto scanBytes = reinterpret_cast<std::uint8_t*>(module);

	auto s = patternBytes.size();
	auto d = patternBytes.data();

	for (auto i = 0ul; i < sizeOfImage - s; ++i) {
		bool found = true;
		for (auto j = 0ul; j < s; ++j) {
			if (scanBytes[i + j] != d[j] && d[j] != -1) {
				found = false;
				break;
			}
		}
		if (found) {
			return &scanBytes[i];
		}
	}
	return nullptr;
}


bool CAutowall::Breakable(c_cs_player* pEntity) {
	if (!pEntity)
		return false;

	const int take_damage = pEntity->get_take_damage();

	//	auto* cclass = g_CHLClient->GetAllClasses();
	//c_client_class* cclass = base_client()->get_all_classes(); // ok
	client_class* cclass = pEntity->get_client_class(); // ok
	if (!cclass)
		return false;

	if ((cclass->network_name[1] == 'B' && cclass->network_name[9] == 'e' && cclass->network_name[10] == 'S' && cclass->network_name[16] == 'e')
		|| (cclass->network_name[1] != 'B' || cclass->network_name[5] != 'D'))
		pEntity->get_take_damage() = 2;

	using IsBreakableEntity_t = bool(__thiscall*)(c_cs_player*);
	static IsBreakableEntity_t  IsBreakableEntityEx = (IsBreakableEntity_t)PatternScan(GetModuleHandleA("client.dll"), "55 8B EC 51 56 8B F1 85 F6 74 68 83 BE");

	pEntity->get_take_damage() = take_damage;

	return IsBreakableEntityEx(pEntity);
}

void CAutowall::ScaleDamage2(c_cs_player* pEntity, int hitgroup, c_base_combat_weapon::weapon_data* weapon_data, float& current_damage) {
	bool   has_heavy_armor = pEntity->has_heavy_armor();
	int    armor = pEntity->has_helmet();
	int    hit_group = hitgroup;

	auto armored = [&]() -> bool {
		c_cs_player* target = pEntity;

		switch (hitgroup) {
		case HITGROUP_HEAD:
			return target->has_helmet();
		case HITGROUP_GENERIC:
		case HITGROUP_CHEST:
		case HITGROUP_STOMACH:
		case HITGROUP_LEFTARM:
		case HITGROUP_RIGHTARM:
			return true;
		default:
			return false;
		}
	};

	switch (hit_group) {
	case HITGROUP_HEAD:
		current_damage *= has_heavy_armor ? 2.f : 4.f;
		break;
	case HITGROUP_STOMACH:
		current_damage *= 1.25f;
		break;
	case HITGROUP_LEFTLEG:
	case HITGROUP_RIGHTLEG:
		current_damage *= 0.75f;
		break;
	default:
		break;
	}

	if (armor > 0 && armored()) {
		float bonus_value = 1.f, armor_bonus_ratio = 0.5f, armor_ratio = weapon_data->flArmorRatio / 2.f;

		if (has_heavy_armor) {
			armor_bonus_ratio = 0.33f;
			armor_ratio *= 0.5f;
			bonus_value = 0.33f;
		}

		auto new_damage = current_damage * armor_ratio;

		if (has_heavy_armor) {
			new_damage *= 0.85f;
		}

		if (((current_damage - (current_damage * armor_ratio)) * (bonus_value * armor_bonus_ratio)) > armor) {
			new_damage = current_damage - (armor / armor_bonus_ratio);
		}

		current_damage = new_damage;
	}
}

bool CAutowall::TraceToExit2(trace& enter_trace, trace& exit_trace, c_vector3d start_position, c_vector3d dir) {
	c_vector3d   start, end;
	float  max_distance = 90.f,
		ray_extension = 4.f,
		current_distance = 0;
	int    first_contents = 0;

	while (current_distance <= max_distance) {
		//  add extra distance to our ray
		current_distance += ray_extension;

		//  multiply the direction vector to the distance so we go outwards, add our position to it.
		start = start_position + dir * current_distance;

		if (!first_contents) {
			first_contents = engine_trace()->get_point_contents_n(start, mask_shot_hull | contents_hitbox, nullptr); /*0x4600400B*/
		}

		int point_contents = engine_trace()->get_point_contents_n(start, mask_shot_hull | contents_hitbox, nullptr);

		if (!(point_contents & mask_shot_hull) || point_contents & contents_hitbox && point_contents != first_contents) /*0x600400B, *0x40000000*/ {

			//  let's setup our end position by deducting the direction by the extra added distance
			end = start - (dir * ray_extension);

			//  let's cast a ray from our start pos to the end pos
			TraceLine(start, end, mask_shot_hull | contents_hitbox, nullptr, &exit_trace);

			//  let's check if a hitbox is in-front of our enemy and if they are behind of a solid wall
			if (exit_trace.startsolid && exit_trace.surface.flags & SURF_HITBOX) {
				TraceLine(start, start_position, mask_shot_hull, (c_cs_player*)exit_trace.entity, &exit_trace);

				if (exit_trace.did_hit() && !exit_trace.startsolid) {
					start = exit_trace.endpos;

					return true;
				}

				continue;
			}

			//  can we hit? is the wall solid?
			if (exit_trace.did_hit() && !exit_trace.startsolid) {

				// is the wall a breakable? if so, let's shoot through it.
				if (Breakable((c_cs_player*)exit_trace.entity) && Breakable((c_cs_player*)exit_trace.entity)) {
					return true;
				}

				if (enter_trace.surface.flags & SURF_NODRAW || !(exit_trace.surface.flags & SURF_NODRAW) && (exit_trace.plane.normal.dot(dir) <= 1.f)) {
					float mult_amount = exit_trace.fraction * 4.f;

					start -= dir * mult_amount;

					return true;
				}

				continue;
			}

			if (!exit_trace.did_hit() || exit_trace.startsolid) {
				if ((enter_trace.entity != nullptr && enter_trace.entity->index() != 0) && Breakable((c_cs_player*)exit_trace.entity)) {
					exit_trace = enter_trace;

					exit_trace.endpos = start + dir;

					return true;
				}

				continue;
			}
		}
	}

	return false;
}

bool CAutowall::HandleBulletPen(c_base_combat_weapon::weapon_data* wpn_data, FireBulletData2& data, bool extracheck = false, c_vector3d point = c_vector3d(0, 0, 0), c_cs_player* pEntity = nullptr) {

	auto local = c_cs_player::get_local_player();
	if (!local)
		return false;

	trace       trace_exit;
	surfacedata_t* enter_surface_data = surface_props()->get_surface_data(data.enter_trace.surface.surface_props);
	int           enter_material = enter_surface_data->game.material;

	c_cs_player* entity = (c_cs_player*)data.enter_trace.entity;

	float         enter_surf_penetration_modifier = enter_surface_data->game.penetration_mod;
	float         final_damage_modifier = 0.f;
	float         combined_penetration_modifier = 0.f;
	bool          solid_surf = ((data.enter_trace.contents >> 3) & contents_solid);
	bool          light_surf = ((data.enter_trace.surface.flags >> 7) & SURF_LIGHT);

	if (data.penetrate_count <= 0
		|| (!data.penetrate_count && !light_surf && !solid_surf && enter_material != CHAR_TEX_GLASS && enter_material != CHAR_TEX_GRATE)
		|| wpn_data->flPenetration <= 0.f
		|| !TraceToExit2(data.enter_trace, trace_exit, data.enter_trace.endpos, data.direction)
		&& !(engine_trace()->get_point_contents_n(data.enter_trace.endpos, mask_shot_hull, nullptr) & (mask_shot_hull)))
		return false;

	surfacedata_t* exit_surface_data = surface_props()->get_surface_data(trace_exit.surface.surface_props);
	int           exit_material = exit_surface_data->game.material;
	float         exit_surf_penetration_modifier = exit_surface_data->game.penetration_mod;

	static convar* damage_reduction_bullets = cvar()->find_var("ff_damage_reduction_bullets");
	static convar* damage_bullet_penetration = cvar()->find_var("ff_damage_bullet_penetration");

	if (enter_material == CHAR_TEX_GLASS || enter_material == CHAR_TEX_GRATE) {
		combined_penetration_modifier = 3.f;
		final_damage_modifier = 0.05f;
	}
	else if (light_surf || solid_surf) {
		combined_penetration_modifier = 1.f;
		final_damage_modifier = 0.16f;
	}
	else if (data.enter_trace.entity && enter_material == CHAR_TEX_FLESH && (local->get_team() == entity->get_team() && damage_reduction_bullets->get_float() == 0.f)) {
		//  look's like you aren't shooting through your teammate today
		if (damage_bullet_penetration->get_float() == 0.f) {
			return false;
		}

		combined_penetration_modifier = damage_bullet_penetration->get_float();
		final_damage_modifier = 0.16f;
	}

	else {
		combined_penetration_modifier = (enter_surf_penetration_modifier + exit_surf_penetration_modifier) * 0.5f;
		final_damage_modifier = 0.16f;
	}

	//  do our materials line up?
	if (enter_material == exit_material) {
		if (exit_material == CHAR_TEX_CARDBOARD || exit_material == CHAR_TEX_WOOD) {
			combined_penetration_modifier = 3.f;
		}

		else if (exit_material == CHAR_TEX_PLASTIC) {
			combined_penetration_modifier = 2.f;
		}
	}

	//  calculate thickness of the wall by getting the length of the range of the trace and squaring
	float thickness = (trace_exit.endpos - data.enter_trace.endpos).lengthsqr();

	if (extracheck)
		if (!VectortoVectorVisible(trace_exit.endpos, point, pEntity))
			return false;

	float modifier = fmaxf(1.f / combined_penetration_modifier, 0.f);

	//  this calculates how much damage we've lost depending on thickness of the wall, our penetration, damage, and the modifiers set earlier
	float lost_damage = fmaxf(
		((modifier * thickness) / 24.f) //* 0.041666668
		+ ((data.current_damage * final_damage_modifier)
			+ (fmaxf(3.75 / wpn_data->flPenetration, 0.f) * 3.f * modifier)), 0.f);

	//  did we loose too much damage?
	if (lost_damage > data.current_damage) {
		return false;
	}

	//  we can't use any of the damage that we've lost
	if (lost_damage > 0.f) {
		data.current_damage -= lost_damage;
	}

	//  do we still have enough damage to deal?
	if (data.current_damage < 1.f) {
		return false;
	}

	data.src = trace_exit.endpos;
	--data.penetrate_count;

	return true;
}


bool CAutowall::FireBullet(FireBulletData2& data) {
	auto local = c_cs_player::get_local_player();
	if (!local)
		return false;
	const auto weapon = reinterpret_cast<c_base_combat_weapon*>(client_entity_list()->get_client_entity_from_handle(local->get_current_weapon_handle()));


	if (!weapon)
		return false;


	const auto wpn_info = weapon_system->get_weapon_data(weapon->get_item_definition());
	if (!wpn_info)
		return false;

	trace     exit_trace;
	c_base_combat_weapon::weapon_data* weapon_info = wpn_info;

	data.penetrate_count = 4;
	data.trace_length = 0.f;

	if (weapon_info == nullptr) {
		return false;
	}

	data.current_damage = weapon_info->iDamage;
	data.max_range = weapon_info->flRange;

	//  if our damage is greater than (or equal to) 1, and we can shoot, let's shoot.
	while (data.penetrate_count > 0 && data.current_damage >= 1.f) {
		data.max_range -= data.trace_length;

		//  create endpoint of bullet
		auto end = (data.direction * data.max_range) + data.src;

		TraceLine(data.src, end, mask_shot_hull | contents_hitbox, local, &data.enter_trace);

		//	UtilClipTraceToPlayers( data.src, end + data.direction * 40.f, mask_shot_hull | contents_hitbox, &data.filter, &data.enter_trace );

			//  "fraction == 1" means that we didn't hit anything. we don't want that- so let's break on it.
		if (data.enter_trace.fraction == 1.f) {
			break;
		}

		//  calculate the damage based on the distance the bullet traveled.
		data.trace_length += data.enter_trace.fraction * data.max_range;

		//  let's make our damage drops off the further away the bullet is.
		data.current_damage *= pow(weapon_info->flRangeModifier, (data.trace_length / 500.f));

		//  get surface data
		auto enter_surface_data = surface_props()->get_surface_data(data.enter_trace.surface.surface_props);
		auto      enter_pen_mod = enter_surface_data->game.penetration_mod;
		int      enter_material = enter_surface_data->game.material;

		if (data.trace_length > 3000.f || enter_pen_mod < 0.1f) {
			break;
		}

		//  this looks gay as fuck if we put it in to 1 long line of code

		c_cs_player* entity = (c_cs_player*)data.enter_trace.entity;
		if (!entity)
			return false;

		bool can_do_damage = (data.enter_trace.hitgroup != HITGROUP_GEAR && data.enter_trace.hitgroup != HITGROUP_GENERIC);
		bool is_enemy = (local->get_team() != entity->get_team());

		//  check to see if we hit an entity, if so scale the damage
		if ((can_do_damage && is_enemy)) {

			//  scale our damage
			ScaleDamage2((c_cs_player*)data.enter_trace.entity, data.enter_trace.hitgroup, weapon_info, data.current_damage);

			return true;
		}

		//  if the bullet can't penetrate wall we don't want to continue looping
		if (!HandleBulletPen(weapon_info, data)) {
			break;
		}
	}

	return false;
}


bool CAutowall::VectortoVectorVisible(c_vector3d src, c_vector3d point, c_cs_player* pEntity = nullptr)
{

	auto local = c_cs_player::get_local_player();
	if (!local)
		return false;


	trace TraceInit;
	TraceInit.entity = nullptr;
	c_trace_filter filter1;
	filter1.skip_entity = local;
	ray_t ray;
	ray.init(src, point);

	engine_trace()->trace_ray(ray, mask_solid, &filter1, &TraceInit);

	if (TraceInit.fraction == 1.0f)
		return true;

	if (pEntity != nullptr && TraceInit.entity == pEntity)
		return true;

	return false;
}
float CAutowall::GetDamageNew(c_vector3d point) {
	//  define local
	auto local = c_cs_player::get_local_player();
	if (!local)
		return 0.f;


	//  make sure local and weapon is not nil
	if (!local) {
		return -1.f;
	}

	// setup fire bullet data
	FireBulletData2 data = FireBulletData2(point, local);

	data.src = local->get_shoot_position();

	//  find the direction from source to point
	c_vector3d angles = math::calc_angle(data.src, point);
	math::angle_vector(angles, &data.direction);
	data.direction.normalize_in_place();

	//  simulate a bullet being fired
	if (FireBullet(data)) {
		return data.current_damage;
	}

	return -1.f;
}

bool CAutowall::CanHitFloatingPoint(const c_vector3d& point, const c_vector3d& source, c_cs_player* pEntity) // ez
{
	auto pLocalEnt = c_cs_player::get_local_player();


	if (!pLocalEnt)
		return false;

	if (!pLocalEnt->is_alive())
		return false;


	static auto VectortoVectorVisible2 = [&](c_vector3d src, c_vector3d point) -> bool {
		trace TraceInit;
		TraceLine(src, point, mask_solid, pLocalEnt, &TraceInit);
		trace Trace;
		TraceLine(src, point, mask_solid, TraceInit.entity, &Trace);

		if (Trace.fraction == 1.0f || TraceInit.fraction == 1.0f)
			return true;

		return false;

	};
	FireBulletData2 data = FireBulletData2(source, pLocalEnt);

	c_vector3d angles = math::calc_angle(data.src, point);
	math::angle_vector(angles, &data.direction);
	data.direction.Normalize();


	const auto weapon = reinterpret_cast<c_base_combat_weapon*>(client_entity_list()->get_client_entity_from_handle(pLocalEnt->get_current_weapon_handle()));


	if (!weapon)
		return false;


	const auto wpn_info = weapon_system->get_weapon_data(weapon->get_item_definition());
	if (!wpn_info)
		return false;
	/*c_base_weapon* pWeapon = pLocalEnt->get_active_weapon();

	if (!pWeapon)
		return false;*/

	data.penetrate_count = 1;
	//data.trace_length = 0.0f;

	

	

	data.current_damage = (float)wpn_info->iDamage;
//	data.trace_length_remaining = wpn_info->flRange - data.trace_length;
	c_vector3d end = data.src + (data.direction * data.trace_length_remaining);
	TraceLine(data.src, end, mask_shot | contents_hitbox, pLocalEnt, &data.enter_trace);

	if (VectortoVectorVisible2(data.src, point) || HandleBulletPen(wpn_info, data, true, point, pEntity))
		return true;

	return false;
}

std::optional<c_trace_system::wall_pen> c_trace_system::wall_penetration(const c_vector3d src, const c_vector3d end,
	c_animation_system::animation* target, c_base_combat_weapon* c_weapon, c_cs_player* override_player) const
{
	static c_base_combat_weapon::weapon_data override_gun{};

	override_gun.iDamage = 15000;
	override_gun.flRangeModifier = 1.0f;
	override_gun.flPenetration = 10.0f;
	override_gun.flArmorRatio = 2.0f;
	override_gun.flRange = 8192.f;
	override_gun.m_pWeaponDef = reinterpret_cast<c_base_combat_weapon::strike_weapon_definition*>(1);

	const auto local = c_cs_player::get_local_player();

	if (!local || !local->is_alive())
		return std::nullopt;

	auto weapon = c_weapon;

	if (!weapon)
		weapon = reinterpret_cast<c_base_combat_weapon*>(client_entity_list()->get_client_entity_from_handle(local->get_current_weapon_handle()));

	if (!weapon && !override_player)
		return std::nullopt;

	const auto data = override_player ? &override_gun :
		weapon_system->get_weapon_data(weapon->get_item_definition());

	if (!data)
		return std::nullopt;

	std::optional<wall_pen> result = std::nullopt;

	if (!override_player)
		run_emulated(target, [&]() -> void
			{
				// setup trace filter.
				uint32_t filter[4] = { c_engine_trace::get_filter_simple_vtable(),
					reinterpret_cast<uint32_t>(local), 0, 0 };

				// run bullet simulation
				result = fire_bullet(data, src, end,
					reinterpret_cast<trace_filter*>(filter), target->player);
			});
	else
	{
		// setup trace filter.
		c_trace_no_player_filter filter;

		// run bullet simulation
		result = fire_bullet(data, src, end, &filter, override_player, true);
	}

	// filter low dmg.
	if (result.has_value() && result.value().damage < 1.f)
		return std::nullopt;

	// report result
	return result;
}

void c_trace_system::run_emulated(c_animation_system::animation* target, const std::function<void()> fn)
{
	// backup player
	const auto backup_origin = target->player->get_origin();
	const auto backup_abs_origin = target->player->get_abs_origin();
	const auto backup_obb_mins = target->player->get_mins();
	const auto backup_obb_maxs = target->player->get_maxs();
	const auto backup_cache = target->player->get_bone_cache();

	// setup trace data
	target->player->get_origin() = target->origin;
	target->player->set_abs_origin(target->origin);
	target->player->get_mins() = target->obb_mins;
	target->player->get_maxs() = target->obb_maxs;
	target->player->get_bone_cache() = reinterpret_cast<matrix3x4**>(target->bones);

	// run emulation
	fn();

	// restore trace data
	target->player->get_origin() = backup_origin;
	target->player->set_abs_origin(backup_abs_origin);
	target->player->get_mins() = backup_obb_mins;
	target->player->get_maxs() = backup_obb_maxs;
	target->player->get_bone_cache() = backup_cache;
}

void c_trace_system::extrapolate(c_cs_player* player, c_vector3d& origin, c_vector3d& velocity, int& flags, bool on_ground)
{
	static const auto sv_gravity = cvar()->find_var(_("sv_gravity"));
	static const auto sv_jump_impulse = cvar()->find_var(_("sv_jump_impulse"));

	if (!(flags & c_base_player::flags::on_ground))
		velocity.z -= ticks_to_time(sv_gravity->get_float());
	else if (player->get_flags() & c_base_player::flags::on_ground && !on_ground)
		velocity.z = sv_jump_impulse->get_float();

	const auto src = origin;
	auto end = src + velocity * global_vars_base->interval_per_tick;

	ray_t r;
	r.Init(src, end, player->get_mins(), player->get_maxs());

	game_trace t{};
	c_trace_filter filter;
	filter.skip_entity = player;

	engine_trace()->trace_ray(r, mask_playersolid, &filter, &t);

	if (t.fraction != 1.f)
	{
		for (auto i = 0; i < 2; i++)
		{
			velocity -= t.plane.normal * velocity.dot(t.plane.normal);

			const auto dot = velocity.dot(t.plane.normal);
			if (dot < 0.f)
				velocity -= c_vector3d(dot * t.plane.normal.x,
					dot * t.plane.normal.y, dot * t.plane.normal.z);

			end = t.endpos + velocity * ticks_to_time(1.f - t.fraction);

			r.Init(t.endpos, end, player->get_mins(), player->get_maxs());
			engine_trace()->trace_ray(r, mask_playersolid, &filter, &t);

			if (t.fraction == 1.f)
				break;
		}
	}

	origin = end = t.endpos;
	end.z -= 2.f;

	r.Init(origin, end, player->get_mins(), player->get_maxs());
	engine_trace()->trace_ray(r, mask_playersolid, &filter, &t);

	flags &= ~c_base_player::flags::on_ground;

	if (t.did_hit() && t.plane.normal.z > .7f)
		flags |= c_base_player::flags::on_ground;
}

std::optional<c_trace_system::wall_pen> c_trace_system::fire_bullet(c_base_combat_weapon::weapon_data* data, c_vector3d src,
	const c_vector3d pos, trace_filter* filter, c_cs_player* target, bool point)
{
	c_vector3d angles;
	math::vector_angles(pos - src, angles);

	if (!angles.is_valid())
		return std::nullopt;

	c_vector3d direction;
	math::angle_vectors(angles, direction);

	if (!direction.is_valid())
		return std::nullopt;

	direction.normalize();

	auto penetrate_count = 5;
	auto length = 0.f, damage = static_cast<float>(data->iDamage);
	game_trace enter_trace{};

	const auto start = src;

	while (penetrate_count > 0 && damage >= 1.0f)
	{
		const auto length_remaining = data->flRange - length;
		auto end = src + direction * length_remaining;

		ray_t r{};
		r.init(src, end);
		engine_trace()->trace_ray(r, mask_shot_player, filter, &enter_trace);

		if (enter_trace.fraction == 1.f && !point)
			break;

		if (point && (enter_trace.fraction == 1.f ||
			(start - enter_trace.endpos).length() > (start - pos).length()))
			return wall_pen{
				scale_damage(target, damage, data->flArmorRatio, hitgroup_head, !data->m_pWeaponDef),
				c_cs_player::hitbox::head, hitgroup_head };

		auto end_extended = end + direction * 40.f;
		engine_trace()->clip_ray_to_player(src, end_extended, mask_shot_player, target, filter, &enter_trace);

		length += enter_trace.fraction * length_remaining;
		damage *= std::powf(data->flRangeModifier, length * .002f);

		if (enter_trace.hitgroup <= 7 && enter_trace.hitgroup > 0)
		{
			if (!enter_trace.entity || enter_trace.entity != target)
				break;

			// we have reached our target!
			return wall_pen{
				scale_damage(target, damage, data->flArmorRatio, enter_trace.hitgroup, !data->m_pWeaponDef),
				static_cast<c_cs_player::hitbox>(enter_trace.hitbox), enter_trace.hitgroup };
		}

		const auto enter_surface = surface_props()->get_surface_data(enter_trace.surface.surface_props);

		if (!enter_surface || enter_surface->game.penetration_mod < .1f)
			break;

		if (!handle_bullet_penetration(data, enter_trace, src, direction, penetrate_count, damage, data->flPenetration))
			break;
	}

	// nothing found
	return std::nullopt;
}

bool c_trace_system::handle_bullet_penetration(c_base_combat_weapon::weapon_data* weapon_data, game_trace& enter_trace,
	c_vector3d& eye_position, const c_vector3d direction, int& penetrate_count, float& current_damage, const float penetration_power)
{
	static const auto ff_damage_reduction_bullets = cvar()->find_var("ff_damage_reduction_bullets");
	static const auto ff_damage_bullet_penetration = cvar()->find_var("ff_damage_bullet_penetration");

	const auto damage_reduction_bullets = ff_damage_reduction_bullets->get_float();
	const auto damage_bullet_penetration = ff_damage_bullet_penetration->get_float();

	game_trace exit_trace{};
	auto enemy = reinterpret_cast<c_cs_player*>(enter_trace.entity);
	if (!enemy) return false;

	const auto enter_surface_data = surface_props()->get_surface_data(enter_trace.surface.surface_props);
	const int enter_material = enter_surface_data->game.material;

	const auto enter_surf_penetration_modifier = enter_surface_data->game.penetration_mod;
	float final_damage_modifier, combined_penetration_modifier;
	const bool is_solid_surf = enter_trace.contents >> 3 & contents_solid;
	const bool is_light_surf = enter_trace.surface.flags >> 7 & surf_light;

	if ((!penetrate_count && !is_light_surf && !is_solid_surf && enter_material != char_tex_grate && enter_material != char_tex_glass)
		|| weapon_data->flPenetration <= 0.f
		|| (!trace_to_exit(enter_trace, exit_trace, enter_trace.endpos, direction, weapon_data->iDamage > 10000.f)
			&& !(engine_trace()->get_point_contents(enter_trace.endpos, mask_shot_hull) & mask_shot_hull)))
		return false;

	const auto exit_surface_data = surface_props()->get_surface_data(exit_trace.surface.surface_props);
	const auto exit_material = exit_surface_data->game.material;
	const auto exit_surf_penetration_modifier = exit_surface_data->game.penetration_mod;

	if (enter_material == char_tex_grate || enter_material == char_tex_glass)
	{
		combined_penetration_modifier = 3.f;
		final_damage_modifier = 0.05f;
	}
	else if (is_solid_surf || is_light_surf)
	{
		combined_penetration_modifier = 1.f;
		final_damage_modifier = 0.16f;
	}
	else if (enter_material == char_tex_flesh && (!enemy->is_enemy() && damage_reduction_bullets == 0.f))
	{
		if (damage_bullet_penetration == 0.f)
			return false;

		combined_penetration_modifier = damage_bullet_penetration;
		final_damage_modifier = 0.16f;
	}
	else
	{
		combined_penetration_modifier = (enter_surf_penetration_modifier + exit_surf_penetration_modifier) / 2.f;
		final_damage_modifier = 0.16f;
	}

	if (enter_material == exit_material)
	{
		if (exit_material == char_tex_cardboard || exit_material == char_tex_wood)
			combined_penetration_modifier = 3.f;
		else if (exit_material == char_tex_plastic)
			combined_penetration_modifier = 2.f;
	}

	auto thickness = (exit_trace.endpos - enter_trace.endpos).length();
	thickness *= thickness;
	thickness *= fmaxf(0.f, 1.0f / combined_penetration_modifier);
	thickness /= 24.0f;

	const auto lost_damage = fmaxf(0.0f, current_damage * final_damage_modifier + fmaxf(0.f, 1.0f / combined_penetration_modifier)
		* 3.0f * fmaxf(0.0f, 3.0f / penetration_power) * 1.25f + thickness);

	if (lost_damage > current_damage)
		return false;

	if (lost_damage > 0.f)
		current_damage -= lost_damage;

	if (current_damage < 1.f)
		return false;

	eye_position = exit_trace.endpos;
	--penetrate_count;
	return true;
}

bool c_trace_system::trace_to_exit(game_trace& enter_trace, game_trace& exit_trace, const c_vector3d start_position,
	const c_vector3d direction, const bool is_local)
{
	const auto max_distance = is_local ? 200.f : 90.f;
	const auto ray_extension = is_local ? 8.f : 4.f;

	float current_distance = 0;
	auto first_contents = 0;

	while (current_distance <= max_distance)
	{
		current_distance += ray_extension;

		auto start = start_position + direction * current_distance;

		if (!first_contents)
			first_contents = engine_trace()->get_point_contents(start, mask_shot_player);

		const auto point_contents = engine_trace()->get_point_contents(start, mask_shot_player);

		if (!(point_contents & mask_shot_hull) || (point_contents & contents_hitbox && point_contents != first_contents))
		{
			const auto end = start - direction * ray_extension;

			ray_t r{};
			r.init(start, end);
			uint32_t filter[4] = { c_engine_trace::get_filter_simple_vtable(),
				uint32_t(c_cs_player::get_local_player()), 0, 0 };
			engine_trace()->trace_ray(r, mask_shot_player, reinterpret_cast<trace_filter*>(filter), &exit_trace);

			if (exit_trace.startsolid && exit_trace.surface.flags & surf_hitbox)
			{
				r.init(start, start_position);
				filter[1] = reinterpret_cast<uint32_t>(exit_trace.entity);
				engine_trace()->trace_ray(r, mask_shot_player, reinterpret_cast<trace_filter*>(filter), &exit_trace);

				if (exit_trace.did_hit() && !exit_trace.startsolid)
				{
					start = exit_trace.endpos;
					return true;
				}

				continue;
			}

			if (exit_trace.did_hit() && !exit_trace.startsolid)
			{
				if (enter_trace.entity->is_breakable() && exit_trace.entity->is_breakable())
					return true;

				if (enter_trace.surface.flags & surf_nodraw
					|| (!(exit_trace.surface.flags & surf_nodraw)
						&& exit_trace.plane.normal.dot(direction) <= 1.f))
				{
					const auto mult_amount = exit_trace.fraction * 4.f;
					start -= direction * mult_amount;
					return true;
				}

				continue;
			}

			if (!exit_trace.did_hit() || exit_trace.startsolid)
			{
				if (enter_trace.did_hit_non_world_entity() && enter_trace.entity->is_breakable())
				{
					exit_trace = enter_trace;
					exit_trace.endpos = start;
					return true;
				}
			}
		}
	}

	return false;
}

// credits to n0xius
float c_trace_system::scale_damage(c_cs_player* target, float damage, const float weapon_armor_ratio, int hitgroup, bool is_zeus)
{
	const auto is_armored = [&]() -> bool
	{
		if (target->get_armor() > 0.f)
		{
			switch (hitgroup)
			{
			case hitgroup_generic:
			case hitgroup_chest:
			case hitgroup_stomach:
			case hitgroup_leftarm:
			case hitgroup_rightarm:
				return true;
			case hitgroup_head:
				return target->has_helmet();
			default:
				break;
			}
		}

		return false;
	};

	if (!is_zeus)
		switch (hitgroup)
		{
		case hitgroup_head:
			if (target->has_heavy_armor())
				damage = (damage * 3.5f) * .5f;
			else
				damage *= 4.f;
			break;
		case hitgroup_stomach:
			damage *= 1.25f;
			break;
		case hitgroup_leftleg:
		case hitgroup_rightleg:
			damage *= .65f;
			break;
		default:
			break;
		}

	if (is_armored())
	{
		auto modifier = 1.f, armor_bonus_ratio = .5f, armor_ratio = weapon_armor_ratio * .5f;

		if (target->has_heavy_armor())
		{
			armor_bonus_ratio = 0.33f;
			armor_ratio = (weapon_armor_ratio * 0.5f) * 0.5f;
			modifier = 0.33f;
		}

		auto new_damage = damage * armor_ratio;

		if (target->has_heavy_armor())
			new_damage *= 0.85f;

		if ((damage - damage * armor_ratio) * (modifier * armor_bonus_ratio) > target->get_armor())
			new_damage = damage - target->get_armor() / armor_bonus_ratio;

		damage = new_damage;
	}

	return damage;
}

void trace_line(c_vector3d& absStart, c_vector3d& absEnd, unsigned int mask, c_base_entity* ignore, game_trace* ptr)
{
	ray_t ray;
	ray.init(absStart, absEnd);
	c_trace_filter filter;
	filter.skip_entity = ignore;

	engine_trace()->trace_ray(ray, mask, &filter, ptr);
}

bool trace_to_exit(game_trace& enterTrace, game_trace& exitTrace, c_vector3d startPosition, c_vector3d direction)
{
	/*
	Masks used:
	MASK_SHOT_HULL					 = 0x600400B
	CONTENTS_HITBOX					 = 0x40000000
	MASK_SHOT_HULL | CONTENTS_HITBOX = 0x4600400B

	*/
	c_vector3d start, end;
	float maxDistance = 90.f, rayExtension = 4.f, currentDistance = 0;
	int firstContents = 0;

	while (currentDistance <= maxDistance)
	{
		//Add extra distance to our ray
		currentDistance += rayExtension;

		//Multiply the direction vector to the distance so we go outwards, add our position to it.
		start = startPosition + direction * currentDistance;

		if (!firstContents)
			firstContents = engine_trace()->get_point_contents(start, mask_shot_hull | contents_hitbox); /*0x4600400B*/
		int pointContents = engine_trace()->get_point_contents(start, mask_shot_hull | contents_hitbox);

		if (!(pointContents & mask_shot_hull) || pointContents & contents_hitbox && pointContents != firstContents) /*0x600400B, *0x40000000*/
		{
			//Let's setup our end position by deducting the direction by the extra added distance
			end = start - (direction * rayExtension);

			//Let's cast a ray from our start pos to the end pos
			trace_line(start, end, mask_shot_hull | contents_hitbox, nullptr, &exitTrace);

			//Let's check if a hitbox is in-front of our enemy and if they are behind of a solid wall
			if (exitTrace.startsolid && exitTrace.surface.flags & surf_hitbox)
			{
				trace_line(start, startPosition, mask_shot_hull, exitTrace.entity, &exitTrace);

				if (exitTrace.did_hit() && !exitTrace.startsolid)
				{
					start = exitTrace.endpos;
					return true;
				}

				continue;
			}

			//Can we hit? Is the wall solid?
			if (exitTrace.did_hit() && !exitTrace.startsolid)
			{

				//Is the wall a breakable? If so, let's shoot through it.
				if (enterTrace.entity->is_breakable() && exitTrace.entity->is_breakable())
					return true;

				if (enterTrace.surface.flags & 0x0080 || !(exitTrace.surface.flags & 0x0080) && (exitTrace.plane.normal.dot(direction) <= 1.f))
				{
					float multAmount = exitTrace.fraction * 4.f;
					start -= direction * multAmount;
					return true;
				}

				continue;
			}

			if (!exitTrace.did_hit() || exitTrace.startsolid)
			{
				if (enterTrace.did_hit() && enterTrace.entity->is_breakable())
				{
					exitTrace = enterTrace;
					exitTrace.endpos = start + direction;
					return true;
				}

				continue;
			}
		}
	}
	return false;
}

bool hbp(c_base_combat_weapon::weapon_data* weaponData, game_trace& enterTrace, c_vector3d& eyePosition, c_vector3d direction, int& possibleHitsRemaining, float& currentDamage, float penetrationPower, bool sv_penetration_type, float ff_damage_reduction_bullets, float ff_damage_bullet_penetration)
{
	auto pLocalEntity = c_cs_player::get_local_player();
	if (!pLocalEntity || pLocalEntity->get_health() <= 0) return false;
	//Because there's been issues regarding this- putting this here.
	if (&currentDamage == nullptr)
		throw std::invalid_argument("currentDamage is null!");

	auto data = FireBulletData(pLocalEntity->get_shoot_position());
	data.filter = c_trace_filter();
	data.filter.skip_entity = pLocalEntity;
	game_trace exitTrace;
	c_base_entity* pEnemy = enterTrace.entity;
	surfacedata_t* enterSurfaceData = surface_props()->get_surface_data(enterTrace.surface.surface_props);
	int enterMaterial = enterSurfaceData->game.material;
	//IPhysicsSurfaceProps
	float enterSurfPenetrationModifier = enterSurfaceData->game.penetration_mod;
	float enterDamageModifier = enterSurfaceData->game.damage_mod;
	float thickness, modifier, lostDamage, finalDamageModifier, combinedPenetrationModifier;
	bool isSolidSurf = ((enterTrace.contents >> 3) & contents_solid);
	bool isLightSurf = ((enterTrace.surface.flags >> 7) & 0x0001);

	if (possibleHitsRemaining <= 0
		//Test for "DE_CACHE/DE_CACHE_TELA_03" as the entering surface and "CS_ITALY/CR_MISCWOOD2B" as the exiting surface.
		//Fixes a wall in de_cache which seems to be broken in some way. Although bullet penetration is not recorded to go through this wall
		//Decals can be seen of bullets within the glass behind of the enemy. Hacky method, but works.
		//You might want to have a check for this to only be activated on de_cache.
		|| (enterTrace.surface.name == (const char*)0x2227c261 && exitTrace.surface.name == (const char*)0x2227c868)
		|| (!possibleHitsRemaining && !isLightSurf && !isSolidSurf && enterMaterial != char_tex_grate && enterMaterial != char_tex_glass)
		|| weaponData->flPenetration <= 0.f
		|| !trace_to_exit(enterTrace, exitTrace, enterTrace.endpos, direction)
		&& !(engine_trace()->get_point_contents(enterTrace.endpos, mask_shot_hull) & mask_shot_hull))
		return false;

	surfacedata_t* exitSurfaceData = surface_props()->get_surface_data(exitTrace.surface.surface_props);
	int exitMaterial = exitSurfaceData->game.material;
	float exitSurfPenetrationModifier = exitSurfaceData->game.penetration_mod;
	float exitDamageModifier = exitSurfaceData->game.damage_mod;

	//Are we using the newer penetration system?
	if (sv_penetration_type)
	{
		if (enterMaterial == char_tex_grate || enterMaterial == char_tex_glass)
		{
			combinedPenetrationModifier = 3.f;
			finalDamageModifier = 0.05f;
		}
		else if (isSolidSurf || isLightSurf)
		{
			combinedPenetrationModifier = 1.f;
			finalDamageModifier = 0.16f;
		}
		else if (enterMaterial == char_tex_flesh && (pLocalEntity->get_team() == pEnemy->get_team() && ff_damage_reduction_bullets == 0.f)) //TODO: Team check config
		{
			//Look's like you aren't shooting through your teammate today
			if (ff_damage_bullet_penetration == 0.f)
				return false;

			//Let's shoot through teammates and get kicked for teamdmg! Whatever, atleast we did damage to the enemy. I call that a win.
			combinedPenetrationModifier = ff_damage_bullet_penetration;
			finalDamageModifier = 0.16f;
		}
		else
		{
			combinedPenetrationModifier = (enterSurfPenetrationModifier + exitSurfPenetrationModifier) / 2.f;
			finalDamageModifier = 0.16f;
		}

		//Do our materials line up?
		if (enterMaterial == exitMaterial)
		{
			if (exitMaterial == char_tex_cardboard || exitMaterial == char_tex_wood)
				combinedPenetrationModifier = 3.f;
			else if (exitMaterial == char_tex_plastic)
				combinedPenetrationModifier = 2.f;
		}

		//Calculate thickness of the wall by getting the length of the range of the trace and squaring
		thickness = (exitTrace.endpos - enterTrace.endpos).lengthsqr();
		modifier = fmaxf(1.f / combinedPenetrationModifier, 0.f);

		//This calculates how much damage we've lost depending on thickness of the wall, our penetration, damage, and the modifiers set earlier
		lostDamage = fmaxf(
			((modifier * thickness) / 24.f)
			+ ((currentDamage * finalDamageModifier)
				+ (fmaxf(3.75f / penetrationPower, 0.f) * 3.f * modifier)), 0.f);

		//Did we loose too much damage?
		if (lostDamage > currentDamage)
			return false;

		//We can't use any of the damage that we've lost
		if (lostDamage > 0.f)
			currentDamage -= lostDamage;

		//Do we still have enough damage to deal?
		if (currentDamage < 1.f)
			return false;

		eyePosition = exitTrace.endpos;
		--possibleHitsRemaining;

		return true;
	}
	else //Legacy penetration system
	{
		combinedPenetrationModifier = 1.f;

		if (isSolidSurf || isLightSurf)
			finalDamageModifier = 0.99f; //Good meme :^)
		else
		{
			finalDamageModifier = fminf(enterDamageModifier, exitDamageModifier);
			combinedPenetrationModifier = fminf(enterSurfPenetrationModifier, exitSurfPenetrationModifier);
		}

		if (enterMaterial == exitMaterial && (exitMaterial == char_tex_metal || exitMaterial == char_tex_wood))
			combinedPenetrationModifier += combinedPenetrationModifier;

		thickness = (exitTrace.endpos - enterTrace.endpos).lengthsqr();

		if (sqrt(thickness) <= combinedPenetrationModifier * penetrationPower)
		{
			currentDamage *= finalDamageModifier;
			eyePosition = exitTrace.endpos;
			--possibleHitsRemaining;

			return true;
		}

		return false;
	}
}

bool c_trace_system::can_wallbang(float& dmg) {

	auto local = c_cs_player::get_local_player();
	if (!local || local->get_health() <= 0) return false;
	FireBulletData data = FireBulletData(local->get_shoot_position());
	data.filter = c_trace_filter();
	data.filter.skip_entity = local;

	c_vector3d eye_angle = engine_client()->get_view_angles();

	c_vector3d dst, forward;

	math::angle_vectors(eye_angle, forward);
	dst = data.src + (forward * 8196.f);

	c_vector3d angles;
	angles = math::calc_angle(data.src, dst);
	math::angle_vectors(angles, data.direction);
	math::normalize(data.direction);

	const auto weapon = reinterpret_cast<c_base_combat_weapon*>(
		client_entity_list()->get_client_entity_from_handle(local->get_current_weapon_handle()));

	if (!weapon || weapon->get_current_clip() == 0) return false;


	data.penetrate_count = 1;
	data.trace_length = 0.0f;

	auto weaponData = weapon_system->get_weapon_data(weapon->get_item_definition());

	if (!weaponData)
		return false;

	data.current_damage = (float)weaponData->iDamage;

	data.trace_length_remaining = weaponData->flRange - data.trace_length;

	c_vector3d end = data.src + data.direction * data.trace_length_remaining;

	trace_line(data.src, end, mask_shot | contents_grate, local, &data.enter_trace);

	static convar* penetrationSystem = cvar()->find_var("sv_penetration_type");
	static convar* damageReductionBullets = cvar()->find_var("ff_damage_reduction_bullets");
	static convar* damageBulletPenetration = cvar()->find_var("ff_damage_bullet_penetration");

	auto ff_damage_reduction_bullets = damageReductionBullets->get_float();
	auto ff_damage_bullet_penetration = damageBulletPenetration->get_float();
	auto sv_penetration_type = penetrationSystem->get_bool();

	if (data.enter_trace.fraction == 1.0f)
		return false;

	if (hbp(weaponData, data.enter_trace, local->get_shoot_position(), data.direction, data.penetrate_count, data.current_damage, weaponData->flPenetration, sv_penetration_type, ff_damage_reduction_bullets, ff_damage_bullet_penetration))
	{
		dmg = data.current_damage;
		return true;
	}

	return false;
}

bool c_trace_system::can_wallbang(float& dmg, c_vector3d origin) {

	auto local = c_cs_player::get_local_player();
	if (!local || local->get_health() <= 0) return false;
	FireBulletData data = FireBulletData(local->get_shoot_position());
	data.filter = c_trace_filter();
	data.filter.skip_entity = local;

	c_vector3d eye_angle = engine_client()->get_view_angles();

	c_vector3d dst, forward;

	math::angle_vectors(eye_angle, origin);
	dst = data.src + origin * 8196.f;//(forward * 8196.f);

	c_vector3d angles;
	angles = math::calc_angle(data.src, dst);
	math::angle_vectors(angles, data.direction);
	math::normalize(data.direction);

	const auto weapon = reinterpret_cast<c_base_combat_weapon*>(
		client_entity_list()->get_client_entity_from_handle(local->get_current_weapon_handle()));

	if (!weapon || weapon->get_current_clip() == 0) return false;


	data.penetrate_count = 1;
	data.trace_length = 0.0f;

	auto weaponData = weapon_system->get_weapon_data(weapon->get_item_definition());

	if (!weaponData)
		return false;

	data.current_damage = (float)weaponData->iDamage;

	data.trace_length_remaining = weaponData->flRange - data.trace_length;

	c_vector3d end = data.src + data.direction * data.trace_length_remaining;

	trace_line(data.src, end, mask_shot | contents_grate, local, &data.enter_trace);

	static convar* penetrationSystem = cvar()->find_var("sv_penetration_type");
	static convar* damageReductionBullets = cvar()->find_var("ff_damage_reduction_bullets");
	static convar* damageBulletPenetration = cvar()->find_var("ff_damage_bullet_penetration");

	auto ff_damage_reduction_bullets = damageReductionBullets->get_float();
	auto ff_damage_bullet_penetration = damageBulletPenetration->get_float();
	auto sv_penetration_type = penetrationSystem->get_bool();

	if (data.enter_trace.fraction == 1.0f)
		return false;

	if (hbp(weaponData, data.enter_trace, local->get_shoot_position(), data.direction, data.penetrate_count, data.current_damage, weaponData->flPenetration, sv_penetration_type, ff_damage_reduction_bullets, ff_damage_bullet_penetration))
	{
		dmg = data.current_damage;
		return true;
	}

	return false;
}