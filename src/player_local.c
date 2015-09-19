/******************************************************************************/
// Free implementation of Bullfrog's Dungeon Keeper strategy game.
/******************************************************************************/
/** @file player_local.c
 *     Local player limited functions.
 * @par Purpose:
 *     Defines functions which should be used only for local
 *     players, before packet files creation.
 * @par Comment:
 *     None.
 * @author   Tomasz Lis
 * @date     10 Nov 2009 - 20 Sep 2014
 * @par  Copying and copyrights:
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 2 of the License, or
 *     (at your option) any later version.
 */
/******************************************************************************/
#include "player_local.h"

#include "globals.h"
#include "bflib_basics.h"

#include "config_terrain.h"
#include "config_trapdoor.h"
#include "dungeon_data.h"
#include "game_legacy.h"
/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
/**
 * Sets keeper power selected by local human player.
 *
 * @param pwkind Power to select.
 * @param sptooltip Tooltip string index.
 * @note Was set_chosen_spell()
 */
void set_chosen_power(PowerKind pwkind, TextStringId tip_stridx)
{
    const struct PowerConfigStats *powerst;
    powerst = get_power_model_stats(pwkind);
    if (power_model_stats_invalid(powerst))
      pwkind = 0;
    SYNCDBG(6,"Setting to %ld",pwkind);
    game.my.chosen_spell_type = pwkind;
    game.my.chosen_spell_spridx = powerst->bigsym_sprite_idx;
    if (tip_stridx != 0) {
        game.my.chosen_spell_tooltip = tip_stridx;
    } else {
        game.my.chosen_spell_tooltip = powerst->tooltip_stridx;
    }
}

void set_chosen_power_none(void)
{
    SYNCDBG(6,"Setting to %d",0);
    game.my.chosen_spell_type = 0;
    game.my.chosen_spell_spridx = 0;
    game.my.chosen_spell_tooltip = 0;
}

void set_chosen_room(RoomKind rkind, TextStringId tip_stridx)
{
    struct RoomConfigStats *roomst;
    roomst = get_room_kind_stats(rkind);
    game.my.chosen_room_kind = rkind;
    game.my.chosen_room_spridx = roomst->bigsym_sprite_idx;
    if (tip_stridx != 0) {
        game.my.chosen_room_tooltip = tip_stridx;
    } else {
        game.my.chosen_room_tooltip = roomst->tooltip_stridx;
    }
}

void set_chosen_room_none(void)
{
    game.my.chosen_room_kind = 0;
    game.my.chosen_room_spridx = 0;
    game.my.chosen_room_tooltip = 0;
}

void set_chosen_manufacture(int manufctr_idx, TextStringId tip_stridx)
{
    struct ManufactureData *manufctr;
    manufctr = get_manufacture_data(manufctr_idx);
    game.my.manufactr_element = manufctr_idx;
    game.my.manufactr_spridx = manufctr->bigsym_sprite_idx;
    if (tip_stridx != 0) {
        game.my.manufactr_tooltip = tip_stridx;
    } else {
        game.my.manufactr_tooltip = manufctr->tooltip_stridx;
    }
}

void set_chosen_manufacture_none(void)
{
    game.my.manufactr_element = 0;
    game.my.manufactr_spridx = 0;
    game.my.manufactr_tooltip = 0;
}

void set_hand_over_subtile(MapSubtlCoord stl_x, MapSubtlCoord stl_y)
{
    game.my.hand_over_subtile_x = stl_x;
    game.my.hand_over_subtile_y = stl_y;
}

void update_chosen_tendencies(void)
{
    struct Dungeon *dungeon;
    dungeon = get_my_dungeon();
    game.my.creatures_tend_imprison = ((dungeon->creature_tendencies & 0x01) != 0);
    game.my.creatures_tend_flee = ((dungeon->creature_tendencies & 0x02) != 0);
}

void set_chosen_tendencies_none(void)
{
    game.my.creatures_tend_imprison = 0;
    game.my.creatures_tend_flee = 0;
}

void set_autopilot_default(void)
{
    game.my.comp_player_aggressive = 0;
    game.my.comp_player_defensive = 1;
    game.my.comp_player_construct = 0;
    game.my.comp_player_creatrsonly = 0;
}
/******************************************************************************/
