enum
{
	WPN_PRIMARY = 1,
	WPN_SECONDARY,
	WPN_MELEE
}

enum
{
	EQ_CT = 1,
	EQ_T
}

// Health constant
#define SHEALTH_MAX_CHANCE  5;

// Normal Natives
native og_get_user_ghost(id); 
native og_get_extra_deathly(id)

native og_get_user_HP(id);
native og_get_user_GP(id);
//native og_get_sphealth_chance(id);
//native og_get_user_maxhealth(id);
//native og_set_user_maxhealth(id);
/*
// Forward
forward gf_user_transghost_pre(id, killer, transformation);
forward gf_user_transghost_post(id, killer, transformation);
forward gf_user_spawn(id);
forward gf_user_died(id, killer);
forward gf_user_nvg(id, on, Ghost);
forward gf_round_new();
forward gf_round_start();
forward gf_game_start();
forward gf_round_end(CsTeams:WinTeam);
forward gf_round_time(TimeLeft);
*/
// Weapon
native og_weapon_register(const Name[], Type, BasedOn, Point, Cost);
native og_weapon_get_cswid(id, ItemID);
native og_equip_register(const Name[], Type, Cost);

forward og_weapon_bought(id, ItemID);
forward og_weapon_remove(id, ItemID);
forward og_weapon_addammo(id, ItemID);
forward og_equip_bought(id, ItemID);
forward og_equip_remove(id, ItemID);

/* AMXX-Studio Notes - DO NOT MODIFY BELOW HERE
*{\\ rtf1\\ ansi\\ deff0{\\ fonttbl{\\ f0\\ fnil Tahoma;}}\n\\ viewkind4\\ uc1\\ pard\\ lang1033\\ f0\\ fs16 \n\\ par }
*/
