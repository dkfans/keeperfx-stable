/******************************************************************************/
// Free implementation of Bullfrog's Dungeon Keeper strategy game.
/******************************************************************************/
/** @file creature_battle.c
 *     Creature battle structure and utility functions.
 * @par Purpose:
 *     Defines CreatureBattle struct and various battle-related routines.
 * @par Comment:
 *     None.
 * @author   KeeperFX Team
 * @date     23 Jul 2011 - 05 Sep 2011
 * @par  Copying and copyrights:
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 2 of the License, or
 *     (at your option) any later version.
 */
/******************************************************************************/
#include "creature_battle.h"
#include "globals.h"

#include "bflib_math.h"
#include "creature_states.h"
#include "thing_list.h"
#include "creature_control.h"
#include "config_creature.h"
#include "config_crtrstates.h"
#include "creature_states_combt.h"
#include "thing_navigate.h"
#include "thing_stats.h"
#include "thing_objects.h"

#include "keeperfx.hpp"

#ifdef __cplusplus
extern "C" {
#endif
/******************************************************************************/
/******************************************************************************/
#ifdef __cplusplus
}
#endif
/******************************************************************************/
long get_flee_position(struct Thing *thing, struct Coord3d *pos)
{
    struct Dungeon *dungeon;
    struct PlayerInfo *player;
    struct CreatureControl *cctrl;
    struct Thing *lairtng;
    struct Thing *heartng;
    struct Thing *gatetng;
    //return _DK_get_flee_position(thing, pos);

    cctrl = creature_control_get_from_thing(thing);
    // Heroes should flee to their gate
    if (thing->owner == game.hero_player_num)
    {
        gatetng = find_hero_door_hero_can_navigate_to(thing);
        if ( !thing_is_invalid(gatetng) )
        {
            pos->x.val = gatetng->mappos.x.val;
            pos->y.val = gatetng->mappos.y.val;
            pos->z.val = gatetng->mappos.z.val;
            return 1;
        }
    } else
    // Neutral creatures don't have flee place
    if (thing->owner == game.neutral_player_num)
    {
        if ( (pos->x.val != 0) || (pos->y.val != 0) )
        {
            return 1;
        }
        return 0;
    }
    // Same with creatures without dungeon - try using last place
    dungeon = get_dungeon(thing->owner);
    if ( dungeon_invalid(dungeon) )
    {
        if ( (pos->x.val != 0) || (pos->y.val != 0) )
        {
            return 1;
        }
        return 0;
    }
    // Other creatures can flee to heart or their lair
    if (cctrl->lairtng_idx > 0)
    {
        lairtng = thing_get(cctrl->lairtng_idx);
        pos->x.val = lairtng->mappos.x.val;
        pos->y.val = lairtng->mappos.y.val;
        pos->z.val = lairtng->mappos.z.val;
    } else
    if (dungeon->dnheart_idx > 0)
    {
        heartng = thing_get(dungeon->dnheart_idx);
        pos->x.val = heartng->mappos.x.val;
        pos->y.val = heartng->mappos.y.val;
        pos->z.val = heartng->mappos.z.val;
    } else
    {
        player = get_player(thing->owner);
        if ( ((player->field_0 & 0x01) != 0) && (player->field_2C == 1) && (player->victory_state != 2) )
        {
            ERRORLOG("The %s has no dungeon heart or lair to flee to",thing_model_name(thing));
            return 0;
        }
        pos->x.val = thing->mappos.x.val;
        pos->y.val = thing->mappos.y.val;
        pos->z.val = thing->mappos.z.val;
    }
    return 1;
}

void setup_combat_flee_position(struct Thing *thing)
{
    struct CreatureControl *cctrl;
    cctrl = creature_control_get_from_thing(thing);
    if (creature_control_invalid(cctrl)) {
        ERRORLOG("Invalid creature control");
        return;
    }
    if ( !get_flee_position(thing, &cctrl->pos_288) )
    {
        ERRORLOG("Couldn't get a flee position for %s index %d",thing_model_name(thing),(int)thing->index);
        cctrl->pos_288.x.val = thing->mappos.x.val;
        cctrl->pos_288.y.val = thing->mappos.y.val;
        cctrl->pos_288.z.val = thing->mappos.z.val;
    }
}

long get_combat_state_for_combat(struct Thing *fighter, struct Thing *enemy, long possible_combat)
{
    struct CreatureControl *enmctrl;
    struct CreatureStats *crstat;
    enmctrl = creature_control_get_from_thing(enemy);
    if (possible_combat == 2)
    {
        if (enmctrl->field_1C < 4) {
            return 2;
        }
        return 1;
    }
    crstat = creature_stats_get_from_thing(fighter);
    if (crstat->attack_preference == 2)
    {
        if ( creature_has_ranged_weapon(fighter) && (enmctrl->field_1C < 4) ) {
            return 2;
        }
    }
    if (enmctrl->field_1B < 4) {
        return 3;
    }
    if ( !creature_has_ranged_weapon(fighter) ) {
      return 1;
    }
    if ( (enmctrl->field_1C < 4) ) {
        return 2;
    }
    return 1;
}

void set_creature_in_combat(struct Thing *fighter, struct Thing *enemy, long possible_combat)
{
    struct CreatureControl *cctrl;
    cctrl = creature_control_get_from_thing(fighter);
    if (creature_control_invalid(cctrl)) {
        ERRORLOG("Invalid creature control");
        return;
    }
    if ( (cctrl->field_3 != 0) && ((cctrl->field_3 & 0x18) == 0) )
    {
        long crstate = get_creature_state_besides_move(fighter);
        ERRORLOG("Creature in combat already - state %s", creature_state_code_name(crstate));
        return;
    }
    if ( external_set_thing_state(fighter, 49) )
    {
        cctrl->field_AA = 0;
        cctrl->field_A9 = 0;
        set_creature_combat_state(fighter, enemy, possible_combat);
        setup_combat_flee_position(fighter);
    }
}

/******************************************************************************/