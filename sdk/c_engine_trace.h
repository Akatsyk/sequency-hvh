#pragma once

#include <cstdint>
#include "c_vector3d.h"
#include "c_base_entity.h"
#include "c_game_rules.h"
#include "c_client_entity_list.h"
#include "../vfunc.h"


#define	contents_empty 0
#define	contents_solid 0x1
#define	contents_window	0x2
#define	contents_aux 0x4
#define	contents_grate 0x8
#define	contents_slime 0x10
#define	contents_water 0x20
#define	contents_blocklos 0x40
#define contents_opaque	0x80
#define	last_visible_contents contents_opaque
#define all_visible_contents (last_visible_contents | (last_visible_contents-1))
#define contents_testfogvolume 0x100
#define contents_unused	0x200
#define contents_blocklight 0x400
#define contents_team1 0x800
#define contents_team2 0x1000
#define contents_ignore_nodraw_opaque 0x2000
#define contents_moveable 0x4000
#define	contents_areaportal 0x8000
#define	contents_playerclip	0x10000
#define	contents_monsterclip 0x20000
#define	contents_current_0 0x40000
#define	contents_current_90	0x80000
#define	contents_current_180 0x100000
#define	contents_current_270 0x200000
#define	contents_current_up	0x400000
#define	contents_current_down 0x800000
#define	contents_origin	0x1000000
#define	contents_monster 0x2000000
#define	contents_debris	0x4000000
#define	contents_detail	0x8000000
#define	contents_translucent 0x10000000
#define	contents_ladder	0x20000000
#define contents_hitbox	0x40000000

#define	mask_all 0xffffffff
#define	mask_solid (contents_solid|contents_moveable|contents_window|contents_monster|contents_grate)
#define	mask_playersolid (contents_solid|contents_moveable|contents_playerclip|contents_window|contents_monster|contents_grate)
#define	mask_npcsolid (contents_solid|contents_moveable|contents_monsterclip|contents_window|contents_monster|contents_grate)
#define	mask_npcfluid (contents_solid|contents_moveable|contents_monsterclip|contents_window|contents_monster)
#define	mask_water (contents_water|contents_moveable|contents_slime)
#define	mask_opaque	(contents_solid|contents_moveable|contents_opaque)
#define mask_opaque_and_npcs (mask_opaque|contents_monster)
#define mask_blocklos (contents_solid|contents_moveable|contents_blocklos)
#define mask_blocklos_and_npcs (mask_blocklos|contents_monster)
#define	mask_visible (mask_opaque|contents_ignore_nodraw_opaque)
#define mask_visible_and_npcs (mask_opaque_and_npcs|contents_ignore_nodraw_opaque)
#define	mask_shot (contents_solid|contents_moveable|contents_monster|contents_window|contents_debris|contents_hitbox)
#define mask_shot_brushonly	(contents_solid|contents_moveable|contents_window|contents_debris)
#define mask_shot_hull (contents_solid|contents_moveable|contents_monster|contents_window|contents_debris|contents_grate)
#define mask_shot_player (mask_shot_hull|contents_hitbox)
#define mask_shot_portal (contents_solid|contents_moveable|contents_window|contents_monster)
#define mask_solid_brushonly (contents_solid|contents_moveable|contents_window|contents_grate)
#define mask_playersolid_brushonly (contents_solid|contents_moveable|contents_window|contents_playerclip|contents_grate)
#define mask_npcsolid_brushonly (contents_solid|contents_moveable|contents_window|contents_monsterclip|contents_grate)
#define mask_npcworldstatic	(contents_solid|contents_window|contents_monsterclip|contents_grate)
#define mask_npcworldstatic_fluid (contents_solid|contents_window|contents_monsterclip)
#define mask_splitareaportal (contents_water|contents_slime)
#define mask_current (contents_current_0|contents_current_90|contents_current_180|contents_current_270|contents_current_up|contents_current_down)
#define	mask_deadsolid (contents_solid|contents_playerclip|contents_window|contents_grate)

#define	hitgroup_generic 0
#define	hitgroup_head 1
#define	hitgroup_chest 2
#define	hitgroup_stomach 3
#define hitgroup_leftarm 4	
#define hitgroup_rightarm 5
#define hitgroup_leftleg 6
#define hitgroup_rightleg 7
#define hitgroup_gear 10

enum trace_type {
	trace_everything = 0,
	trace_world_only,
	trace_entities_only,
	trace_everything_filter_props,
};

// ReSharper disable once CppPolymorphicClassWithNonVirtualPublicDestructor
class trace_filter {
public:
	virtual bool should_hit_entity(c_base_entity* entity, int contents_mask) = 0;
	virtual trace_type get_trace_type() const = 0;
};

// ReSharper disable once CppPolymorphicClassWithNonVirtualPublicDestructor
class c_trace_filter : public trace_filter {
public:
	bool should_hit_entity(c_base_entity* entity, int contents_mask) override
	{
		return !(entity == skip_entity);
	}

	trace_type get_trace_type() const override
	{
		return trace_everything;
	}

	c_base_entity* skip_entity{};

};

class c_trace_filter_world_and_props_only : public trace_filter
{
public:
	virtual bool should_hit_entity(c_base_entity* entity_handle, int contents_maskk) {
		return false;
	}
	virtual trace_type get_trace_type() const {
		return trace_everything;
	}
};

class c_trace_filter_players_only : public trace_filter {
public:
	bool should_hit_entity(c_base_entity* entity, int contents_mask) override
	{
		return !(entity == skip_entity);
	}

	trace_type get_trace_type() const override
	{
		return trace_entities_only;
	}

	c_base_entity* skip_entity{};
};

// ReSharper disable once CppPolymorphicClassWithNonVirtualPublicDestructor
class c_trace_world_filter : public trace_filter {
public:
	bool should_hit_entity(c_base_entity*, int) override
	{
		return false;
	}

	trace_type get_trace_type() const override
	{
		return trace_world_only;
	}

	c_base_entity* skip_entity{};
};

class c_trace_no_player_filter : public trace_filter {
public:
	bool should_hit_entity(c_base_entity* ent, int) override
	{
		return !(ent && ent->is_player());
	}

	trace_type get_trace_type() const override
	{
		return trace_everything;
	}
};

struct plane
{
	c_vector3d normal{};
	float dist{};
	uint8_t	type{};
	uint8_t	signbits{};
	uint8_t	pad[2]{};
};
class __declspec(align(16))vector_aligned : public c_vector3d {
public:
	vector_aligned& operator=(const c_vector3d& other) {
		x = other.x;
		y = other.y;
		z = other.z;
		return *this;
	}

	vector_aligned& operator=(const float& other) {
		*this = { other, other, other };
		return *this;
	}
	float w;
};
struct ray_t {


	vector_aligned m_start;
	vector_aligned m_delta;
	vector_aligned m_start_offset;
	vector_aligned m_extents;
	const matrix3x4* m_world_axis_transform;
	bool    m_is_ray;
	bool    m_is_swept;


	void Init(c_vector3d& start, c_vector3d& end) {
		m_delta = end - start;

		m_is_swept = (m_delta.lengthsqr() != 0);

		m_extents = 0.f;
		m_is_ray = true;

		m_start_offset = 0.f;
		m_start = start;
	}
	void init(c_vector3d vec_start, c_vector3d vec_end)
	{
		m_delta = vec_end - vec_start;
		m_is_swept = (m_delta.lengthsqr() != 0);
		m_extents = 0.f;
		m_world_axis_transform = NULL;
		m_is_ray = true;
		m_start_offset = 0.f;
		m_start = vec_start;
	}
	void Init(c_vector3d& start, c_vector3d& end, c_vector3d& mins, c_vector3d& maxs) {
		m_delta = end - start;

		m_is_swept = (m_delta.lengthsqr() != 0);

		m_extents = maxs - mins;
		m_extents *= 0.5f;
		m_is_ray = (m_extents.lengthsqr() < 1e-6);

		m_start_offset = mins + maxs;
		m_start_offset *= 0.5f;
		m_start = start - m_start_offset;
		m_start_offset *= -1.0f;
	}
	void Init(c_vector3d const& start, c_vector3d const& end, c_vector3d const& mins, c_vector3d const& maxs)
	{
		m_delta = end - start;

		m_world_axis_transform = NULL;
		m_is_swept = (m_delta.lengthsqr() != 0);

		m_extents = maxs - mins;
		m_extents *= 0.5f;
		m_is_ray = (m_extents.lengthsqr() < 1e-6);

		m_start_offset = maxs + mins;
		m_start_offset *= 0.5f;
		m_start = start + m_start_offset;
		m_start_offset *= -1.0f;
	}

	/*void init(const c_vector3d src, const c_vector3d end) {
		*reinterpret_cast<c_vector3d*>(&delta) = end - src;
		is_swept = delta.length() != 0.f;
		extents.x = extents.y = extents.z = 0.0f;
		world_axis_transform = nullptr;
		is_ray = true;
		start_offset.x = start_offset.y = start_offset.z = 0.0f;
		*reinterpret_cast<c_vector3d*>(&start) = src;
	}

	void init(c_vector3d const& start, c_vector3d const& end, c_vector3d const& mins, c_vector3d const& maxs)
	{
		world_axis_transform = nullptr;
		*reinterpret_cast<c_vector3d*>(&delta) = end - start;
		*reinterpret_cast<c_vector3d*>(&extents) = maxs - mins;
		extents *= 0.5f;

		m128 tmp;
		tmp.f[0] = delta.length();
		tmp.f[1] = extents.length();
		const auto calc = sqrt_ps(tmp.v);
		is_swept = reinterpret_cast<const m128*>(&calc)->f[0] != 0.f;
		is_ray = reinterpret_cast<const m128*>(&calc)->f[1] < 1e-6f;

		*reinterpret_cast<c_vector3d*>(&start_offset) = maxs + mins;
		start_offset *= 0.5f;
		*reinterpret_cast<c_vector3d*>(&this->start) = start + start_offset;
		start_offset *= -1.0f;
	}*/
};

class base_trace
{
public:
	c_vector3d startpos{};
	c_vector3d endpos{};
	plane plane{};
	float fraction{};
	int	contents{};
	unsigned short disp_flags{};
	bool allsolid{};
	bool startsolid{};
};

struct surf
{
	const char* name{};
	short surface_props{};
	unsigned short flags{};
};

class game_trace : public base_trace
{
public:
	bool did_hit() const
	{
		return fraction < 1 || allsolid || startsolid;
	}

	bool did_hit_world() const
	{
		//return entity == client_entity_list()->get_client_entity(0);
		return entity && entity->index() == 0;
	}

	bool did_hit_non_world_entity() const
	{
		return entity != nullptr && !did_hit_world();
	}

	float fractionleftsolid{};
	surf surface{};
	int hitgroup{};
	short physicsbone{};
	unsigned short world_surface_index{};
	c_base_entity* entity{};
	int hitbox{};

	game_trace() {}
	game_trace(const game_trace& vOther);
};

class c_engine_trace
{
public:
		int	get_point_contents_n(const c_vector3d& vecAbsPosition, int contentsMask = mask_all, c_base_entity* ppCBaseEntity = NULL)
		{
			typedef int(__thiscall* fnGetPointContents)(void*, const c_vector3d&, int, c_base_entity*);
			return call_vfunc<fnGetPointContents>(this, 0)(this, vecAbsPosition, contentsMask, ppCBaseEntity);
		}

		vfunc(0, get_point_contents(c_vector3d& abs_position, int mask), int(__thiscall*)(c_engine_trace*, c_vector3d&, int, uintptr_t))(abs_position, mask, 0)
		vfunc(3, clip_ray_to_entity(ray_t& r, const uint32_t mask, c_base_entity* entity, game_trace* trace), void(__thiscall*)(c_engine_trace*, ray_t&, uint32_t, c_base_entity*, game_trace*))(r, mask, entity, trace)
		vfunc(5, trace_ray(ray_t& r, const uint32_t mask, trace_filter* filter, game_trace* trace), void(__thiscall*)(c_engine_trace*, ray_t&, uint32_t, trace_filter*, game_trace*))(r, mask, filter, trace)

		__forceinline static bool line_through_smoke(c_vector3d& start, c_vector3d& end)
		{
			static const auto line_through_smoke = reinterpret_cast<bool(__cdecl*)(c_vector3d, c_vector3d)>(
				sig("client.dll", "55 8B EC 83 EC 08 8B 15 ? ? ? ? 0F 57 C0"));
			return line_through_smoke(start, end);
		}

	__forceinline static uint32_t get_filter_simple_vtable()
	{
		static const auto filter_simple = *reinterpret_cast<uint32_t*>(reinterpret_cast<uint32_t>(
			sig("client.dll", "55 8B EC 83 E4 F0 83 EC 7C 56 52")) + 0x3d);
		return filter_simple;
	}
	bool IsVisible(c_base_entity* pLocalClientBaseCBaseEntity, c_vector3d vecOrigin, c_vector3d vecFinal, c_base_entity* pClientBaseCBaseEntity, int& hitgroup)
	{
		ray_t r{};

		r.Init(vecOrigin, vecFinal);

		c_trace_filter* TraceFilter = new c_trace_filter();
		TraceFilter->skip_entity = pLocalClientBaseCBaseEntity;

		trace trace;
		trace_ray(r, mask_shot, TraceFilter, &trace);
		hitgroup = trace.hitgroup;
		delete TraceFilter;
		return (trace.entity == pClientBaseCBaseEntity || trace.fraction >= 1.0f);
	}

	void clip_ray_to_player(c_vector3d& src, c_vector3d& end, const uint32_t mask, c_base_entity* player, trace_filter* filter, game_trace* t)
	{
		if (filter && !filter->should_hit_entity(player, mask))
			return;

		game_trace t_new{};
		ray_t r{};
		r.init(src, end);

		clip_ray_to_entity(r, mask, player, &t_new);
		if (t_new.fraction < t->fraction)
			*t = t_new;
	}





};

interface_var(c_engine_trace, engine_trace, "engine.dll", "EngineTraceClient")
