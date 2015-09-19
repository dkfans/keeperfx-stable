/******************************************************************************/
// Free implementation of Bullfrog's Dungeon Keeper strategy game.
/******************************************************************************/
/** @file player_local.h
 *     Header file for player_local.c.
 * @par Purpose:
 *     Local player limited functions.
 * @par Comment:
 *     Just a header file - #defines, typedefs, function prototypes etc.
 * @author   Tomasz Lis
 * @date     10 Nov 2009 - 20 Sep 2014
 * @par  Copying and copyrights:
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 2 of the License, or
 *     (at your option) any later version.
 */
/******************************************************************************/
#ifndef DK_PLYR_LOCAL_H
#define DK_PLYR_LOCAL_H

#include "bflib_basics.h"
#include "globals.h"

#ifdef __cplusplus
extern "C" {
#endif
/******************************************************************************/
/******************************************************************************/
void set_chosen_power(PowerKind pwkind, TextStringId tip_stridx);
void set_chosen_power_none(void);

void set_chosen_room(RoomKind rkind, TextStringId tip_stridx);
void set_chosen_room_none(void);

void set_chosen_manufacture(int manufctr_idx, TextStringId tip_stridx);
void set_chosen_manufacture_none(void);

void set_hand_over_subtile(MapSubtlCoord stl_x, MapSubtlCoord stl_y);

void set_chosen_tendencies_none(void);
void update_chosen_tendencies(void);

void set_autopilot_default(void);

/******************************************************************************/
#ifdef __cplusplus
}
#endif
#endif
