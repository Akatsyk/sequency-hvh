#pragma once

#include <optional>
#include "../utils/c_singleton.h"
#include "../sdk/c_cs_player.h"
#include "../sdk/c_engine_trace.h"
#include "c_animation_system.h"

class c_trace_system : public c_singleton<c_trace_system>
{
public:
	struct wall_pen
	{
		float damage;
		c_cs_player::hitbox hitbox;
		int32_t hitgroup;
	};

	std::optional<wall_pen> wall_penetration(c_vector3d src, c_vector3d end,
		c_animation_system::animation* target, c_base_combat_weapon* c_weapon = nullptr, c_cs_player* override_player = nullptr) const;

	static void run_emulated(c_animation_system::animation* target, std::function<void()> fn);

	static void extrapolate(c_cs_player* player, c_vector3d& origin, c_vector3d& velocity, int& flags, bool on_ground);

	static bool can_wallbang(float& dmg);

	static bool can_wallbang(float& dmg, c_vector3d origin);

private:
	static std::optional<wall_pen> fire_bullet(c_base_combat_weapon::weapon_data* data, c_vector3d src,
		c_vector3d pos, trace_filter* filter, c_cs_player* target = nullptr, bool point = false);

	static bool handle_bullet_penetration(c_base_combat_weapon::weapon_data* weapon_data, game_trace& enter_trace,
		c_vector3d& eye_position, c_vector3d direction, int& penetrate_count,
		float& current_damage, float penetration_power);
	static bool trace_to_exit(game_trace& enter_trace, game_trace& exit_trace, c_vector3d start_position, c_vector3d direction, bool is_local = false);

	static float scale_damage(c_cs_player* target, float damage, float weapon_armor_ratio, int hitgroup, bool is_zeus);
};

struct FireBulletData
{
	FireBulletData(const c_vector3d& eye_pos) : src(eye_pos)
	{
	}

	c_vector3d						src;
	game_trace       enter_trace;
	c_vector3d						direction;
	c_trace_filter  filter;
	float						trace_length;
	float						trace_length_remaining;
	float						current_damage;
	int							penetrate_count;
};


/* New AutoWall      */
struct FireBulletData2
{
	FireBulletData2(const c_vector3d& eyePos, c_base_player* entity) : src(eyePos)
	{
	}

	c_vector3d						src;
	trace       enter_trace;
	c_vector3d						direction;
	c_trace_filter  filter;
	float						trace_length;
	float						trace_length_remaining;
	float						current_damage;
	int							penetrate_count;
	float max_range;
};
class CAutowall
{
public:









	void ClipTraceToPlayers(c_vector3d& absStart, c_vector3d absEnd, unsigned int mask, c_trace_filter* filter, trace* tr);

	void TraceLine(c_vector3d& absStart, c_vector3d& absEnd, unsigned int mask, c_base_entity* ignore, trace* ptr);

	void GetBulletTypeParameters(float& maxRange, float& maxDistance, char* bulletType, bool sv_penetration_type);

	bool IsBreakableEntity(c_client_entity* entity);

	void ScaleDamage(trace& enterTrace, c_base_combat_weapon::weapon_data* weaponData, float& currentDamage);

	bool TraceToExit(trace& enterTrace, trace& exitTrace, c_vector3d startPosition, c_vector3d direction);




	bool HandleBulletPen(c_base_combat_weapon::weapon_data* wpn_data, FireBulletData2& data, bool extracheck, c_vector3d point, c_cs_player* pEntity);

	bool FireBullet(FireBulletData2& data);

	bool HandleBulletPenetration(c_base_combat_weapon::weapon_data* weaponData, trace& enterTrace, c_vector3d& eyePosition, c_vector3d direction, int& possibleHitsRemaining, float& currentDamage, float penetrationPower, bool sv_penetration_type, float ff_damage_reduction_bullets, float ff_damage_bullet_penetration);

	bool FireBullet(c_vector3d& direction, float& currentDamage);



	float CanHit(c_vector3d& point);

	bool PenetrateWall(c_cs_player* pBaseEntity, c_vector3d& vecPoint);




	bool Breakable(c_cs_player* pEntity);

	void ScaleDamage2(c_cs_player* pEntity, int hitgroup, c_base_combat_weapon::weapon_data* weapon_data, float& current_damage);

	bool TraceToExit2(trace& enter_trace, trace& exit_trace, c_vector3d start_position, c_vector3d dir);



	bool VectortoVectorVisible(c_vector3d src, c_vector3d point, c_cs_player* pEntity);

	float GetDamageNew(c_vector3d point);

	bool CanHitFloatingPoint(const c_vector3d& point, const c_vector3d& source, c_cs_player* pEntity);


}; extern CAutowall* g_Autowall;


#define trace_system c_trace_system::instance()
