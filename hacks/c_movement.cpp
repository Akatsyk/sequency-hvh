#include "c_movement.h"
#include "../utils/math.h"
#include "../sdk/c_cvar.h"
#include "../menu/c_menu.h"

void c_movement::run(c_cs_player* local, c_user_cmd* cmd)
{
	bhop(local, cmd);
}

void c_movement::autostrafe(c_user_cmd* cmd)
{
	static const auto cl_sidespeed = cvar()->find_var("cl_sidespeed");
	const auto local = c_cs_player::get_local_player();
	float yaw_add = 0.f;

	if (!local || !local->is_alive())return;
	if (local->is_grounded() || !config->Misc.autostrafe)return;

	if (cmd->buttons & c_user_cmd::flags::back)
		yaw_add = -180.f;
	else if (cmd->buttons & c_user_cmd::flags::move_right)
		yaw_add = -90.f;
	else if (cmd->buttons & c_user_cmd::flags::move_left)
		yaw_add = 90.f;
	else if (cmd->buttons & c_user_cmd::flags::forward)
		yaw_add = 0.f;

	cmd->viewangles.y += yaw_add;
	cmd->forwardmove = 0.f;
	cmd->sidemove = 0.f;

	const auto delta = math::normalize_yaw(cmd->viewangles.y - rad2deg(atan2(local->get_velocity().y, local->get_velocity().x)));

	cmd->sidemove = delta > 0.f ? -cl_sidespeed->get_float() : cl_sidespeed->get_float();

	cmd->viewangles.y = math::normalize_yaw(cmd->viewangles.y - delta);
}

void c_movement::bhop(c_cs_player* local, c_user_cmd* cmd)
{
	if (!config->Misc.bhop)
		return;

	static auto last_jumped = false;
	static auto should_fake = false;

	const auto move_type = local->get_move_type();
	const auto flags = local->get_flags();

	if (move_type != c_cs_player::movetype_ladder && move_type != c_cs_player::movetype_noclip &&
		!(flags & c_cs_player::in_water))
	{
		if (!last_jumped && should_fake)
		{
			should_fake = false;
			cmd->buttons |= c_user_cmd::jump;
		}
		else if (cmd->buttons & c_user_cmd::jump)
		{
			autostrafe(cmd);

			if (flags & c_cs_player::on_ground)
			{
				last_jumped = true;
				should_fake = true;
			}
			else
			{
				cmd->buttons &= ~c_user_cmd::jump;
				last_jumped = false;
			}
		}
		else
		{
			last_jumped = false;
			should_fake = false;
		}
	}
}
