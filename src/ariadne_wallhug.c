/******************************************************************************/
// Free implementation of Bullfrog's Dungeon Keeper strategy game.
/******************************************************************************/
/** @file ariadne_wallhug.c
 *     Simple wallhug pathfinding functions.
 * @par Purpose:
 *     Functions implementing wallhug pathfinding algorithm.
 * @par Comment:
 *     None.
 * @author   KeeperFX Team
 * @date     11 Mar 2010 - 10 Jan 2014
 * @par  Copying and copyrights:
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 2 of the License, or
 *     (at your option) any later version.
 */
/******************************************************************************/
#include "ariadne_wallhug.h"

#include "globals.h"
#include "bflib_basics.h"
#include "bflib_math.h"
#include "bflib_planar.h"

#include "ariadne.h"
#include "slab_data.h"
#include "map_data.h"
#include "map_utils.h"
#include "thing_data.h"
#include "thing_physics.h"
#include "thing_navigate.h"
#include "engine_camera.h"
#include "config_terrain.h"
#include "creature_control.h"
#include "creature_states.h"
#include "config_creature.h"

#ifdef __cplusplus
extern "C" {
#endif
/******************************************************************************/
DLLIMPORT long _DK_slab_wall_hug_route(struct Thing *thing, struct Coord3d *pos, long a3);
DLLIMPORT short _DK_hug_round(struct Thing *creatng, struct Coord3d *pos1, struct Coord3d *pos2, unsigned short a4, long *a5);
DLLIMPORT long _DK_get_angle_of_wall_hug(struct Thing *creatng, long a2, long a3, unsigned char a4);
DLLIMPORT signed char _DK_get_starting_angle_and_side_of_hug(struct Thing *creatng, struct Coord3d *pos, long *a3, unsigned char *a4, long a5, unsigned char direction);
DLLIMPORT long _DK_creature_cannot_move_directly_to_with_collide(struct Thing *creatng, struct Coord3d *pos, long a3, unsigned char a4);
DLLIMPORT long _DK_check_forward_for_prospective_hugs(struct Thing *creatng, struct Coord3d *pos, long a3, long a4, long a5, long direction, unsigned char a7);
DLLIMPORT long _DK_get_map_index_of_first_block_thing_colliding_with_travelling_to(struct Thing *creatng, struct Coord3d *startpos, struct Coord3d *endpos, long a4, unsigned char a5);
DLLIMPORT long _DK_get_next_position_and_angle_required_to_tunnel_creature_to(struct Thing *creatng, struct Coord3d *pos, unsigned char a3);
DLLIMPORT long _DK_dig_to_position(signed char basestl_x, unsigned short basestl_y, unsigned short plyr_idx, unsigned char a4, unsigned char a5);
DLLIMPORT long _DK_get_map_index_of_first_block_thing_colliding_with_at(struct Thing *creatng, struct Coord3d *pos, long a3, unsigned char a4);
/******************************************************************************/
struct Around const my_around_eight[] = {
  { 0,-1},
  { 1,-1},
  { 1, 0},
  { 1, 1},
  { 0, 1},
  {-1, 1},
  {-1, 0},
  {-1,-1},
};

struct Around const my_around_nine[] = {
  {-1,-1},
  { 0,-1},
  { 1,-1},
  {-1, 0},
  { 0, 0},
  { 1, 0},
  {-1, 1},
  { 0, 1},
  { 1, 1},
};

short const around_map[] = {-257, -256, -255, -1, 0, 1, 255, 256, 257};

/**
 * Should contain values encoded with get_subtile_number(). */
const unsigned short small_around_pos[] = {
  0xFF00, 0x0001, 0x0100, 0xFFFF,
};

struct Around const start_at_around[] = {
    { 0,  0},
    {-1, -1},
    {-1,  0},
    {-1,  1},
    { 0, -1},
    { 0,  1},
    { 1, -1},
    { 1,  0},
    { 1,  1},
};

/******************************************************************************/
/**
 * Computes index in small_around[] array which contains coordinates directing towards given destination.
 * @param srcpos_x Source position X; either map coordinates or subtiles, but have to match type of other coords.
 * @param srcpos_y Source position Y; either map coordinates or subtiles, but have to match type of other coords.
 * @param dstpos_x Destination position X; either map coordinates or subtiles, but have to match type of other coords.
 * @param dstpos_y Destination position Y; either map coordinates or subtiles, but have to match type of other coords.
 * @return Index for small_around[] array.
 */
int small_around_index_in_direction(long srcpos_x, long srcpos_y, long dstpos_x, long dstpos_y)
{
    long i;
    i = ((LbArcTanAngle(dstpos_x - srcpos_x, dstpos_y - srcpos_y) & LbFPMath_AngleMask) + LbFPMath_PI/4);
    return (i >> 9) & 3;
}

TbBool can_step_on_unsafe_terrain_at_position(const struct Thing *creatng, MapSubtlCoord stl_x, MapSubtlCoord stl_y)
{
    struct CreatureStats *crstat;
    crstat = creature_stats_get_from_thing(creatng);
    struct SlabMap *slb;
    slb = get_slabmap_for_subtile(stl_x, stl_y);
    // We can step on lava if it doesn't hurt us or we can fly
    if (slb->kind == SlbT_LAVA) {
        return (crstat->hurt_by_lava <= 0) || ((creatng->movement_flags & TMvF_Flying) != 0);
    }
    return false;
}

TbBool terrain_toxic_for_creature_at_position(const struct Thing *creatng, MapSubtlCoord stl_x, MapSubtlCoord stl_y)
{
    struct CreatureStats *crstat;
    crstat = creature_stats_get_from_thing(creatng);
    // If the position is over lava, and we can't continuously fly, then it's toxic
    if ((crstat->hurt_by_lava > 0) && map_pos_is_lava(stl_x,stl_y)) {
        // Check not only if a creature is now flying, but also whether it's natural ability
        if (((creatng->movement_flags & TMvF_Flying) == 0) || (!crstat->flying))
            return true;
    }
    return false;
}

TbBool hug_can_move_on(struct Thing *creatng, MapSubtlCoord stl_x, MapSubtlCoord stl_y)
{
    struct SlabMap *slb;
    slb = get_slabmap_for_subtile(stl_x, stl_y);
    if (slabmap_block_invalid(slb))
        return false;
    struct SlabAttr *slbattr;
    slbattr = get_slab_attrs(slb);
    if ((slbattr->block_flags & SlbAtFlg_IsDoor) != 0)
    {
        struct Thing *doortng;
        doortng = get_door_for_position(stl_x, stl_y);
        if (!thing_is_invalid(doortng) && (doortng->owner == creatng->owner) && !doortng->door.is_locked)
        {
            return true;
        }
    }
    else
    {
        if (slbattr->is_safe_land || can_step_on_unsafe_terrain_at_position(creatng, stl_x, stl_y))
        {
            return true;
        }
    }
    return false;
}

TbBool wallhug_angle_with_collide_valid(struct Thing *thing, long a2, long move_delta, long angle, unsigned char a4)
{
    struct Coord3d pos;
    pos.x.val = thing->mappos.x.val + distance_with_angle_to_coord_x(move_delta, angle);
    pos.y.val = thing->mappos.y.val + distance_with_angle_to_coord_y(move_delta, angle);
    pos.z.val = get_thing_height_at(thing, &pos);
    return (creature_cannot_move_directly_to_with_collide(thing, &pos, a2, a4) != 4);
}

long get_angle_of_wall_hug(struct Thing *creatng, long a2, long a3, unsigned char a4)
{
    //return _DK_get_angle_of_wall_hug(creatng, a2, a3, a4);
    struct Navigation *navi;
    {
        struct CreatureControl *cctrl;
        cctrl = creature_control_get_from_thing(creatng);
        navi = &cctrl->navi;
    }
    long quadr, whangle;
    switch (navi->field_1[0])
    {
    case 1:
        quadr = angle_to_quadrant(creatng->move_angle_xy);
        whangle = (LbFPMath_PI/2) * ((quadr - 1) & 3);
        if (wallhug_angle_with_collide_valid(creatng, a2, a3, whangle, a4))
          return whangle;
        quadr = angle_to_quadrant(creatng->move_angle_xy);
        whangle = (LbFPMath_PI/2) * (quadr & 3);
        if (wallhug_angle_with_collide_valid(creatng, a2, a3, whangle, a4))
          return whangle;
        quadr = angle_to_quadrant(creatng->move_angle_xy);
        whangle = (LbFPMath_PI/2) * ((quadr + 1) & 3);
        if (wallhug_angle_with_collide_valid(creatng, a2, a3, whangle, a4))
          return whangle;
        quadr = angle_to_quadrant(creatng->move_angle_xy);
        whangle = (LbFPMath_PI/2) * ((quadr + 2) & 3);
        if (wallhug_angle_with_collide_valid(creatng, a2, a3, whangle, a4))
          return whangle;
        break;
    case 2:
        quadr = angle_to_quadrant(creatng->move_angle_xy);
        whangle = (LbFPMath_PI/2) * ((quadr + 1) & 3);
        if (wallhug_angle_with_collide_valid(creatng, a2, a3, whangle, a4))
          return whangle;
        quadr = angle_to_quadrant(creatng->move_angle_xy);
        whangle = (LbFPMath_PI/2) * (quadr & 3);
        if (wallhug_angle_with_collide_valid(creatng, a2, a3, whangle, a4))
          return whangle;
        quadr = angle_to_quadrant(creatng->move_angle_xy);
        whangle = (LbFPMath_PI/2) * ((quadr - 1) & 3);
        if (wallhug_angle_with_collide_valid(creatng, a2, a3, whangle, a4))
          return whangle;
        quadr = angle_to_quadrant(creatng->move_angle_xy);
        whangle = (LbFPMath_PI/2) * ((quadr + 2) & 3);
        if (wallhug_angle_with_collide_valid(creatng, a2, a3, whangle, a4))
          return whangle;
        break;
    }
    return -1;
}

short hug_round(struct Thing *creatng, struct Coord3d *pos1, struct Coord3d *pos2, unsigned short a4, long *a5)
{
    return _DK_hug_round(creatng, pos1, pos2, a4, a5);
}

long slab_wall_hug_route(struct Thing *thing, struct Coord3d *pos, long max_val)
{
    struct Coord3d curr_pos;
    struct Coord3d next_pos;
    struct Coord3d pos3;
    //return _DK_slab_wall_hug_route(thing, pos, max_val);
    curr_pos.x.val = thing->mappos.x.val;
    curr_pos.y.val = thing->mappos.y.val;
    curr_pos.z.val = thing->mappos.z.val;
    curr_pos.x.stl.num = stl_slab_center_subtile(curr_pos.x.stl.num);
    curr_pos.y.stl.num = stl_slab_center_subtile(curr_pos.y.stl.num);
    MapSubtlCoord stl_x, stl_y;
    stl_x = stl_slab_center_subtile(pos->x.stl.num);
    stl_y = stl_slab_center_subtile(pos->y.stl.num);
    pos3.x.val = pos->x.val;
    pos3.y.val = pos->y.val;
    pos3.z.val = pos->z.val;
    pos3.x.stl.num = stl_x;
    pos3.y.stl.num = stl_y;
    next_pos.x.val = curr_pos.x.val;
    next_pos.y.val = curr_pos.y.val;
    next_pos.z.val = curr_pos.z.val;
    int i;
    for (i = 0; i < max_val; i++)
    {
        if ((curr_pos.x.stl.num == stl_x) && (curr_pos.y.stl.num == stl_y)) {
            return i + 1;
        }
        int round_idx;
        round_idx = small_around_index_in_direction(curr_pos.x.stl.num, curr_pos.y.stl.num, stl_x, stl_y);
        if (hug_can_move_on(thing, curr_pos.x.stl.num, curr_pos.y.stl.num))
        {
            next_pos.x.val = curr_pos.x.val;
            next_pos.y.val = curr_pos.y.val;
            next_pos.z.val = curr_pos.z.val;
            curr_pos.x.stl.num += STL_PER_SLB * (int)small_around[round_idx].delta_x;
            curr_pos.y.stl.num += STL_PER_SLB * (int)small_around[round_idx].delta_y;
        } else
        {
            long hug_val;
            hug_val = max_val - i;
            int hug_ret;
            hug_ret = hug_round(thing, &next_pos, &pos3, round_idx, &hug_val);
            if (hug_ret == -1) {
                return -1;
            }
            i += hug_val;
            if (hug_ret == 1) {
                return i + 1;
            }
            curr_pos.x.val = next_pos.x.val;
            curr_pos.y.val = next_pos.y.val;
            curr_pos.z.val = next_pos.z.val;
        }
    }
    return 0;
}

signed char get_starting_angle_and_side_of_hug(struct Thing *creatng, struct Coord3d *pos, long *a3, unsigned char *a4, long a5, unsigned char a6)
{
    return _DK_get_starting_angle_and_side_of_hug(creatng, pos, a3, a4, a5, a6);
}

unsigned short get_hugging_blocked_flags(struct Thing *creatng, struct Coord3d *pos, long mapblk_flags, unsigned char plyr_bits)
{
    unsigned short blkflags;
    struct Coord3d tmpos;
    blkflags = 0;
    {
        tmpos.x.val = pos->x.val;
        tmpos.y.val = creatng->mappos.y.val;
        tmpos.z.val = creatng->mappos.z.val;
        if (creature_cannot_move_directly_to_with_collide(creatng, &tmpos, mapblk_flags, plyr_bits) == 4) {
            blkflags |= SlbBloF_WalledX;
        }
    }
    {
        tmpos.x.val = creatng->mappos.x.val;
        tmpos.y.val = pos->y.val;
        tmpos.z.val = creatng->mappos.z.val;
        if (creature_cannot_move_directly_to_with_collide(creatng, &tmpos, mapblk_flags, plyr_bits) == 4) {
            blkflags |= SlbBloF_WalledY;
        }
    }
    if (blkflags == 0)
    {
        tmpos.x.val = pos->x.val;
        tmpos.y.val = pos->y.val;
        tmpos.z.val = creatng->mappos.z.val;
        if (creature_cannot_move_directly_to_with_collide(creatng, &tmpos, mapblk_flags, plyr_bits) == 4) {
            blkflags |= SlbBloF_WalledZ;
        }
    }
    return blkflags;
}

void set_hugging_pos_using_blocked_flags(struct Coord3d *dstpos, struct Thing *creatng, unsigned short block_flags, int nav_radius)
{
    struct Coord3d tmpos;
    int coord;
    tmpos.x.val = creatng->mappos.x.val;
    tmpos.y.val = creatng->mappos.y.val;
    if (block_flags & SlbBloF_WalledX)
    {
        coord = creatng->mappos.x.val;
        if (dstpos->x.val >= coord)
        {
            tmpos.x.val = coord + nav_radius;
            tmpos.x.stl.pos = COORD_PER_STL-1;
            tmpos.x.val -= nav_radius;
        } else
        {
            tmpos.x.val = coord - nav_radius;
            tmpos.x.stl.pos = 1;
            tmpos.x.val += nav_radius;
        }
    }
    if (block_flags & SlbBloF_WalledY)
    {
        coord = creatng->mappos.y.val;
        if (dstpos->y.val >= coord)
        {
            tmpos.y.val = coord + nav_radius;
            tmpos.y.stl.pos = COORD_PER_STL-1;
            tmpos.y.val -= nav_radius;
        } else
        {
            tmpos.y.val = coord - nav_radius;
            tmpos.y.stl.pos = 1;
            tmpos.y.val += nav_radius;
        }
    }
    if (block_flags & SlbBloF_WalledZ)
    {
        coord = creatng->mappos.x.val;
        if (dstpos->x.val >= coord)
        {
            tmpos.x.val = coord + nav_radius;
            tmpos.x.stl.pos = COORD_PER_STL-1;
            tmpos.x.val -= nav_radius;
        } else
        {
            tmpos.x.val = coord - nav_radius;
            tmpos.x.stl.pos = 1;
            tmpos.x.val += nav_radius;
        }
        coord = creatng->mappos.y.val;
        if (dstpos->y.val >= coord)
        {
            tmpos.y.val = coord + nav_radius;
            tmpos.y.stl.pos = COORD_PER_STL-1;
            tmpos.y.val -= nav_radius;
        } else
        {
            tmpos.y.val = coord - nav_radius;
            tmpos.y.stl.pos = 1;
            tmpos.y.val += nav_radius;
        }
    }
    tmpos.z.val = get_thing_height_at(creatng, &tmpos);
    dstpos->x.val = tmpos.x.val;
    dstpos->y.val = tmpos.y.val;
    dstpos->z.val = tmpos.z.val;
}

long get_map_index_of_first_block_thing_colliding_with_at(struct Thing *creatng, const struct Coord3d *pos, long mapblk_flags, unsigned char plyr_bits)
{
    MapSubtlCoord beg_stl_x, beg_stl_y;
    MapSubtlCoord end_stl_x, end_stl_y;
    //return _DK_get_map_index_of_first_block_thing_colliding_with_at(creatng, pos, mapblk_flags, plyr_bits);
    {
        MapCoordDelta nav_radius;
        MapSubtlDelta i;
        nav_radius = thing_nav_sizexy(creatng) / 2;
        i = (pos->x.val - nav_radius) / COORD_PER_STL;
        if (i <= 0) {
            beg_stl_x = 0;
        } else {
            beg_stl_x = i;
        }
        i = (pos->x.val + nav_radius) / COORD_PER_STL + 1;
        if (i >= COORD_PER_STL-1) {
            end_stl_x = COORD_PER_STL-1;
        } else {
            end_stl_x = i;
        }
        i = (pos->y.val - nav_radius) / COORD_PER_STL;
        if (i <= 0) {
            beg_stl_y = 0;
        } else {
            beg_stl_y = i;
        }
        i = (pos->y.val + nav_radius) / COORD_PER_STL + 1;
        if (i >= COORD_PER_STL-1) {
            end_stl_y = COORD_PER_STL-1;
        } else {
            end_stl_y = i;
        }
    }
    MapSubtlCoord stl_x, stl_y;
    for (stl_y = beg_stl_y; stl_y < end_stl_y; stl_y++)
    {
        for (stl_x = beg_stl_x; stl_x < end_stl_x; stl_x++)
        {
            struct Map *mapblk;
            mapblk = get_map_block_at(stl_x, stl_y);
            struct SlabMap *slb;
            slb = get_slabmap_for_subtile(stl_x, stl_y);
            // If none of given flags are set, reject this place
            if (((mapblk->flags & mapblk_flags) == 0) && (slb->kind != SlbT_ROCK))
                continue;
            // If the place is filled, and owner meets given player flags, reject this place
            if (((mapblk->flags & mapblk_flags & SlbAtFlg_Filled) != 0) && ((1 << slabmap_owner(slb)) & plyr_bits))
                continue;
            if ((mapblk->flags & SlbAtFlg_IsDoor) == 0)
                return get_subtile_number(stl_x, stl_y);
            struct Thing *doortng;
            doortng = get_door_for_position(stl_x, stl_y);
            if (thing_is_invalid(doortng) || !door_will_open_for_thing(doortng, creatng))
                return get_subtile_number(stl_x, stl_y);
        }

    }
    return -1;
}

long creature_cannot_move_directly_to_with_collide_sub(struct Thing *creatng, struct Coord3d pos, long mapblk_flags, unsigned char plyr_bits)
{
    MapCoord height;
    if (thing_in_wall_at(creatng, &pos))
    {
        pos.z.val = subtile_coord(map_subtiles_z,COORD_PER_STL-1);
        height = get_thing_height_at(creatng, &pos);
        if ((height >= subtile_coord(map_subtiles_z,COORD_PER_STL-1)) || (height - creatng->mappos.z.val > COORD_PER_STL))
        {
            if (get_map_index_of_first_block_thing_colliding_with_at(creatng, &pos, mapblk_flags, plyr_bits) >= 0) {
                return 4;
            } else {
                return 1;
            }
        }
    }
    return 0;
}

long creature_cannot_move_directly_to_with_collide(struct Thing *creatng, struct Coord3d *pos, long mapblk_flags, unsigned char plyr_bits)
{
    //return _DK_creature_cannot_move_directly_to_with_collide(creatng, pos, a3, a4);

    MapCoord clpcor;
    int cannot_mv;
    struct Coord3d next_pos;
    struct Coord3d prev_pos;
    struct Coord3d orig_pos;
    MapCoordDelta dt_x, dt_y;

    prev_pos = creatng->mappos;
    dt_x = (prev_pos.x.val - pos->x.val);
    dt_y = (prev_pos.y.val - pos->y.val);
    cannot_mv = 0;
    orig_pos = creatng->mappos;
    if ((pos->x.stl.num == prev_pos.x.stl.num) || (pos->y.stl.num == prev_pos.y.stl.num))
    {
        // Only one coordinate changed enough to switch subtile - easy path
        cannot_mv = creature_cannot_move_directly_to_with_collide_sub(creatng, *pos, mapblk_flags, plyr_bits);
        return cannot_mv;
    }

    if (cross_x_boundary_first(&prev_pos, pos))
    {
        if (pos->x.val <= prev_pos.x.val)
            clpcor = (prev_pos.x.val & (~COORD_PER_STL_MASK)) - 1;
        else
            clpcor = (prev_pos.x.val + COORD_PER_STL) & (~COORD_PER_STL_MASK);
        next_pos.x.val = clpcor;
        next_pos.y.val = dt_y * abs(clpcor - orig_pos.x.val) / dt_x + orig_pos.y.val;
        next_pos.z.val = prev_pos.z.val;
        switch (creature_cannot_move_directly_to_with_collide_sub(creatng, prev_pos, mapblk_flags, plyr_bits))
        {
        case 0:
            creatng->mappos = next_pos;
            creatng->mappos.z.val = get_thing_height_at(creatng, &next_pos);
            break;
        case 1:
            creatng->mappos = orig_pos;
            cannot_mv = 1;
            break;
        case 4:
            // mappos unchanged - no need to restore
            return 4;
        }

        prev_pos = creatng->mappos;
        if (pos->y.val <= prev_pos.y.val)
            clpcor = (prev_pos.y.val & (~COORD_PER_STL_MASK)) - 1;
        else
            clpcor = (prev_pos.y.val + COORD_PER_STL) & (~COORD_PER_STL_MASK);
        next_pos.y.val = clpcor;
        next_pos.x.val = dt_x * abs(clpcor - orig_pos.y.val) / dt_y + orig_pos.x.val;
        next_pos.z.val = prev_pos.z.val;
        switch (creature_cannot_move_directly_to_with_collide_sub(creatng, next_pos, mapblk_flags, plyr_bits))
        {
        case 0:
            creatng->mappos = next_pos;
            creatng->mappos.z.val = get_thing_height_at(creatng, &next_pos);
            break;
        case 1:
            creatng->mappos = next_pos;
            creatng->mappos.z.val = 0;
            cannot_mv = 1;
            break;
        case 4:
            creatng->mappos = orig_pos;
            return 4;
        }

        prev_pos = creatng->mappos;
        next_pos.x.val = pos->x.val;
        next_pos.y.val = pos->y.val;
        next_pos.z.val = creatng->mappos.z.val;
        switch (creature_cannot_move_directly_to_with_collide_sub(creatng, next_pos, mapblk_flags, plyr_bits))
        {
        case 0:
            creatng->mappos = orig_pos; // restore mappos
            break;
        case 1:
            creatng->mappos = orig_pos; // restore mappos
            cannot_mv = 1;
            break;
        case 4:
            creatng->mappos = orig_pos;
            return 4;
        }
        return cannot_mv;
    }

    if (cross_y_boundary_first(&prev_pos, pos))
    {
        if (pos->y.val <= prev_pos.y.val)
            clpcor = (prev_pos.y.val & (~COORD_PER_STL_MASK)) - 1;
        else
            clpcor = (prev_pos.y.val + COORD_PER_STL) & (~COORD_PER_STL_MASK);
        next_pos.y.val = clpcor;
        next_pos.x.val = dt_x * abs(clpcor - orig_pos.y.val) / dt_y + orig_pos.x.val;
        next_pos.z.val = prev_pos.z.val;
        switch (creature_cannot_move_directly_to_with_collide_sub(creatng, next_pos, mapblk_flags, plyr_bits))
        {
        case 0:
            creatng->mappos = next_pos;
            creatng->mappos.z.val = get_thing_height_at(creatng, &next_pos);
            break;
        case 1:
            creatng->mappos = next_pos;
            creatng->mappos.z.val = 0;
            cannot_mv = 1;
            break;
        case 4:
            // mappos unchanged - no need to restore
            return 4;
        }
        prev_pos = creatng->mappos;
        if (pos->x.val <= prev_pos.x.val)
            clpcor = (prev_pos.x.val & (~COORD_PER_STL_MASK)) - 1;
        else
            clpcor = (prev_pos.x.val + COORD_PER_STL) & (~COORD_PER_STL_MASK);
        next_pos.x.val = clpcor;
        next_pos.y.val = dt_y * abs(clpcor - orig_pos.x.val) / dt_x + orig_pos.y.val;
        next_pos.z.val = prev_pos.z.val;
        switch (creature_cannot_move_directly_to_with_collide_sub(creatng, next_pos, mapblk_flags, plyr_bits))
        {
        case 0:
            creatng->mappos = next_pos;
            creatng->mappos.z.val = get_thing_height_at(creatng, &next_pos);
            break;
        case 1:
            creatng->mappos = next_pos;
            creatng->mappos.z.val = 0;
            cannot_mv = 1;
            break;
        case 4:
            creatng->mappos = orig_pos;
            return 4;
        }
        prev_pos = creatng->mappos;
        next_pos.x.val = pos->x.val;
        next_pos.y.val = pos->y.val;
        next_pos.z.val = prev_pos.z.val;
        switch (creature_cannot_move_directly_to_with_collide_sub(creatng, *pos, mapblk_flags, plyr_bits))
        {
        default:
            creatng->mappos = orig_pos; // restore mappos
            break;
        case 1:
            creatng->mappos = orig_pos; // restore mappos
            cannot_mv = 1;
            break;
        case 4:
            creatng->mappos = orig_pos;
            return 4;
        }
        return cannot_mv;
    }

    WARNDBG(3,"While moving %s index %d - crossing two boundaries, but neither is first",thing_model_name(creatng),(int)creatng->index);
    switch (creature_cannot_move_directly_to_with_collide_sub(creatng, *pos, mapblk_flags, plyr_bits))
    {
    default:
        creatng->mappos = orig_pos; // restore mappos
        break;
    case 1:
        creatng->mappos = orig_pos; // restore mappos
        cannot_mv = 1;
        break;
    case 4:
        creatng->mappos = orig_pos;
        return 4;
    }
    return cannot_mv;
}

TbBool thing_can_continue_direct_line_to(struct Thing *creatng, struct Coord3d *pos1, struct Coord3d *pos2, long mapblk_flags, long a5, unsigned char plyr_bits)
{
    long angle;
    struct Coord3d posa;
    struct Coord3d posb;
    struct Coord3d posc;
    int coord;
    angle = get_angle_xy_to(pos1, pos2);
    posa.x.val = pos1->x.val;
    posa.y.val = pos1->y.val;
    posa.z.val = pos1->z.val;
    posa.x.val += distance_with_angle_to_coord_x(a5, angle);
    posa.y.val += distance_with_angle_to_coord_y(a5, angle);
    posa.z.val = get_thing_height_at(creatng, &posa);
    coord = pos1->x.val;
    if (coord < posa.x.val) {
        coord += a5;
    } else
    if (coord > posa.x.val) {
        coord -= a5;
    }
    posb.x.val = coord;
    posb.y.val = pos1->y.val;
    posb.z.val = get_thing_height_at(creatng, &posb);
    coord = pos1->y.val;
    if (coord < posa.y.val) {
        coord += a5;
    } else
    if (coord > posa.y.val) {
        coord -= a5;
    }
    posc.y.val = coord;
    posc.x.val = pos1->x.val;
    posc.z.val = get_thing_height_at(creatng, &posc);
    return creature_cannot_move_directly_to_with_collide(creatng, &posb, mapblk_flags, plyr_bits) != 4
        && creature_cannot_move_directly_to_with_collide(creatng, &posc, mapblk_flags, plyr_bits) != 4
        && creature_cannot_move_directly_to_with_collide(creatng, &posa, mapblk_flags, plyr_bits) != 4;
}

long check_forward_for_prospective_hugs(struct Thing *creatng, struct Coord3d *pos, long a3, long a4, long a5, long a6, unsigned char a7)
{
    return _DK_check_forward_for_prospective_hugs(creatng, pos, a3, a4, a5, a6, a7);
}

TbBool find_approach_position_to_subtile(const struct Coord3d *srcpos, MapSubtlCoord stl_x, MapSubtlCoord stl_y, MoveSpeed spacing, struct Coord3d *aproachpos)
{
    struct Coord3d targetpos;
    targetpos.x.val = subtile_coord_center(stl_x);
    targetpos.y.val = subtile_coord_center(stl_y);
    targetpos.z.val = 0;
    long min_dist;
    long n;
    min_dist = LONG_MAX;
    for (n=0; n < SMALL_AROUND_SLAB_LENGTH; n++)
    {
        long dx,dy;
        dx = spacing * (long)small_around[n].delta_x;
        dy = spacing * (long)small_around[n].delta_y;
        struct Coord3d tmpos;
        tmpos.x.val = targetpos.x.val + dx;
        tmpos.y.val = targetpos.y.val + dy;
        tmpos.z.val = 0;
        struct Map *mapblk;
        mapblk = get_map_block_at(tmpos.x.stl.num, tmpos.y.stl.num);
        if ((!map_block_invalid(mapblk)) && ((mapblk->flags & SlbAtFlg_Blocking) == 0))
        {
            long dist;
            dist = get_2d_box_distance(srcpos, &tmpos);
            if (min_dist > dist)
            {
                min_dist = dist;
                *aproachpos = tmpos;
            }
        }
    }
    return (min_dist < LONG_MAX);
}

long get_map_index_of_first_block_thing_colliding_with_travelling_to(struct Thing *creatng, const struct Coord3d *startpos, const struct Coord3d *endpos, long mapblk_flags, unsigned char plyr_bits)
{
    MapCoord tmp_cor;
    long stl_num; // should be SubtlCodedCoords, but may need to be signed
    struct Coord3d tmp_pos;
    struct Coord3d mod_pos;
    struct Coord3d orig_pos;
    MapCoordDelta delta_x, delta_y;
    //return _DK_get_map_index_of_first_block_thing_colliding_with_travelling_to(creatng, startpos, endpos, mapblk_flags, plyr_bits);

    mod_pos = *startpos;
    delta_x = mod_pos.x.val - (MapCoordDelta)endpos->x.val;
    delta_y = mod_pos.y.val - (MapCoordDelta)endpos->y.val;
    orig_pos = creatng->mappos;
    SYNCDBG(6,"Check %s index %d from (%d,%d) to (%d,%d)",thing_model_name(creatng),(int)creatng->index,
        (int)coord_subtile(startpos->x.val),(int)coord_subtile(startpos->y.val),(int)coord_subtile(endpos->x.val),(int)coord_subtile(endpos->y.val));
    // Speed is limited to one subtile per turn, and usually it is much lower, so in over 99% cases the difference between startpos and endpos
    // will be minimal, and both will be on one slab. Bearing that in mind, we can prepare an optimization for these cases.
    if (cross_one_boundary_at_most_with_radius(&mod_pos, endpos, thing_nav_sizexy(creatng)/2))
    {
        stl_num = get_map_index_of_first_block_thing_colliding_with_at(creatng, endpos, mapblk_flags, plyr_bits);
        if (stl_num >= 0) {
            creatng->mappos = orig_pos;
            return stl_num;
        }

        creatng->mappos = orig_pos;
        return 0;
    }
    if (cross_x_boundary_first(&mod_pos, endpos))
    {
        if (endpos->x.val <= mod_pos.x.val)
          tmp_cor = (mod_pos.x.val & ~COORD_PER_STL_MASK) - 1;
        else
          tmp_cor = (mod_pos.x.val + COORD_PER_STL) & ~COORD_PER_STL_MASK;
        tmp_pos.x.val = tmp_cor;
        tmp_pos.y.val = delta_y * abs(tmp_cor - (MapCoordDelta)orig_pos.x.val) / delta_x + orig_pos.y.val;
        tmp_pos.z.val = mod_pos.z.val;
        stl_num = get_map_index_of_first_block_thing_colliding_with_at(creatng, &tmp_pos, mapblk_flags, plyr_bits);
        if (stl_num >= 0) {
            creatng->mappos = orig_pos;
            return stl_num;
        }

        mod_pos = creatng->mappos;
        if (endpos->y.val <= mod_pos.y.val)
          tmp_cor = (mod_pos.y.val & ~COORD_PER_STL_MASK) - 1;
        else
          tmp_cor = (mod_pos.y.val + COORD_PER_STL) & ~COORD_PER_STL_MASK;
        tmp_pos.x.val = tmp_cor;
        tmp_pos.y.val = delta_x * abs(tmp_cor - (MapCoordDelta)orig_pos.y.val) / delta_y + orig_pos.x.val;
        tmp_pos.z.val = mod_pos.z.val;
        stl_num = get_map_index_of_first_block_thing_colliding_with_at(creatng, &tmp_pos, mapblk_flags, plyr_bits);
        if (stl_num >= 0) {
            creatng->mappos = orig_pos;
            return stl_num;
        }

        mod_pos = creatng->mappos;
        tmp_pos.x.val = endpos->x.val;
        tmp_pos.y.val = endpos->y.val;
        tmp_pos.z.val = creatng->mappos.z.val;
        stl_num = get_map_index_of_first_block_thing_colliding_with_at(creatng, &tmp_pos, mapblk_flags, plyr_bits);
        if (stl_num >= 0) {
            creatng->mappos = orig_pos;
            return stl_num;
        }

        creatng->mappos = orig_pos;
        return 0;
    }
    if (cross_y_boundary_first(&mod_pos, endpos))
    {
      if (endpos->y.val <= mod_pos.y.val)
          tmp_cor = (mod_pos.y.val & ~COORD_PER_STL_MASK) - 1;
      else
          tmp_cor = (mod_pos.y.val + COORD_PER_STL) & ~COORD_PER_STL_MASK;
      tmp_pos.y.val = tmp_cor;
      tmp_pos.x.val = delta_x * abs(tmp_cor - (MapCoordDelta)orig_pos.y.val) / delta_y + orig_pos.x.val;
      tmp_pos.z.val = mod_pos.z.val;
      stl_num = get_map_index_of_first_block_thing_colliding_with_at(creatng, &tmp_pos, mapblk_flags, plyr_bits);
      if (stl_num >= 0) {
          creatng->mappos = orig_pos;
          return stl_num;
      }

      mod_pos = creatng->mappos;
      if (endpos->x.val <= mod_pos.x.val)
        tmp_cor = (mod_pos.x.val & ~COORD_PER_STL_MASK) - 1;
      else
        tmp_cor = (mod_pos.x.val + COORD_PER_STL) & ~COORD_PER_STL_MASK;
      tmp_pos.x.val = tmp_cor;
      tmp_pos.y.val = delta_y * abs(tmp_cor - (MapCoordDelta)orig_pos.x.val) / delta_x + orig_pos.y.val;
      tmp_pos.z.val = mod_pos.z.val;
      stl_num = get_map_index_of_first_block_thing_colliding_with_at(creatng, &tmp_pos, mapblk_flags, plyr_bits);
      if (stl_num >= 0) {
          creatng->mappos = orig_pos;
          return stl_num;
      }

      mod_pos = creatng->mappos;
      tmp_pos.x.val = endpos->x.val;
      tmp_pos.y.val = endpos->y.val;
      tmp_pos.z.val = creatng->mappos.z.val;
      stl_num = get_map_index_of_first_block_thing_colliding_with_at(creatng, &tmp_pos, mapblk_flags, plyr_bits);
      if (stl_num >= 0) {
          creatng->mappos = orig_pos;
          return stl_num;
      }

      creatng->mappos = orig_pos;
      return 0;
    }

    {
        stl_num = get_map_index_of_first_block_thing_colliding_with_at(creatng, endpos, mapblk_flags, plyr_bits);
        if (stl_num >= 0) {
            creatng->mappos = orig_pos;
            return stl_num;
        }

        creatng->mappos = orig_pos;
        return 0;
    }
}

TbBool navigation_push_towards_target(struct Navigation *navi, struct Thing *creatng, const struct Coord3d *pos, MoveSpeed speed, MoveSpeed nav_radius, unsigned char plyr_bits)
{
    navi->navstate = NavS_Unkn2;
    navi->pos_next.x.val = creatng->mappos.x.val + distance_with_angle_to_coord_x(speed, navi->angle_D);
    navi->pos_next.y.val = creatng->mappos.y.val + distance_with_angle_to_coord_y(speed, navi->angle_D);
    navi->pos_next.z.val = get_thing_height_at(creatng, &navi->pos_next);
    struct Coord3d pos1;
    pos1 = navi->pos_next;
    check_forward_for_prospective_hugs(creatng, &pos1, navi->angle_D, navi->field_1[0], SlbAtFlg_Filled|SlbAtFlg_Valuable, speed, plyr_bits);
    if (get_2d_box_distance(&pos1, &creatng->mappos) > MOVE_DESTINATION_SPOT_RADIUS)
    {
        navi->pos_next = pos1;
    }
    navi->field_9 = get_2d_box_distance(&creatng->mappos, &navi->pos_next);
    int cannot_move;
    cannot_move = creature_cannot_move_directly_to_with_collide(creatng, &navi->pos_next, SlbAtFlg_Filled|SlbAtFlg_Valuable, plyr_bits);
    if (cannot_move == 4)
    {
        navi->pos_next = creatng->mappos;
        navi->field_9 = 0;
    }
    navi->field_5 = get_2d_box_distance(&creatng->mappos, pos);
    if (cannot_move == 1)
    {
        SubtlCodedCoords stl_num;
        stl_num = get_map_index_of_first_block_thing_colliding_with_travelling_to(creatng, &creatng->mappos, &navi->pos_next, SlbAtFlg_Filled|SlbAtFlg_Digable, 0);
        navi->stl_15 = stl_num;
        MapSubtlCoord stl_x, stl_y;
        stl_x = slab_subtile_center(subtile_slab_fast(stl_num_decode_x(stl_num)));
        stl_y = slab_subtile_center(subtile_slab_fast(stl_num_decode_y(stl_num)));
        if (!find_approach_position_to_subtile(&creatng->mappos, stl_x, stl_y, nav_radius + 3*COORD_PER_STL/2 + 1, &navi->pos_next)) {
            // Cannot approach selected subtile - reset
            ERRORLOG("The target subtile (%d,%d) has no approachable position",(int)stl_x,(int)stl_y);
            initialise_wallhugging_path_from_to(navi, &creatng->mappos, pos);
            return false;
        }
        navi->angle_D = get_angle_xy_to(&creatng->mappos, &navi->pos_next);
        navi->navstate = NavS_Unkn3;
    }
    return true;
}

long get_next_position_and_angle_required_to_tunnel_creature_to(struct Thing *creatng, struct Coord3d *pos, unsigned char plyr_bits)
{
    struct Navigation *navi;
    int speed;
    {
        struct CreatureControl *cctrl;
        cctrl = creature_control_get_from_thing(creatng);
        navi = &cctrl->navi;
        speed = cctrl->max_speed;
        cctrl->flgfield_2 = 0;
        cctrl->combat_flags = 0;
    }
    plyr_bits |= (1 << creatng->owner);
    SYNCDBG(16,"Move %s index %d from (%d,%d) to (%d,%d) with nav state %d",thing_model_name(creatng),(int)creatng->index,(int)creatng->mappos.x.stl.num,(int)creatng->mappos.y.stl.num,(int)pos->x.stl.num,(int)pos->y.stl.num,(int)navi->navstate);
    //return _DK_get_next_position_and_angle_required_to_tunnel_creature_to(creatng, pos, plyr_bits);
    MapSubtlCoord stl_x, stl_y;
    SubtlCodedCoords stl_num;
    MapCoordDelta dist_to_next;
    struct Coord3d tmpos;
    int nav_radius;
    long angle, i;
    int block_flags, cannot_move;
    struct Map *mapblk;
    switch (navi->navstate)
    {
    case NavS_Unkn1:
        dist_to_next = get_2d_box_distance(&creatng->mappos, &navi->pos_next);
        if (dist_to_next >= navi->field_9) {
            navi->field_4 = 0;
        }
        if (navi->field_4 == 0)
        {
            navi->angle_D = get_angle_xy_to(&creatng->mappos, pos);
            navi->pos_next.x.val = creatng->mappos.x.val + distance_with_angle_to_coord_x(speed, navi->angle_D);
            navi->pos_next.y.val = creatng->mappos.y.val + distance_with_angle_to_coord_y(speed, navi->angle_D);
            navi->pos_next.z.val = get_thing_height_at(creatng, &navi->pos_next);
            if (get_2d_box_distance(&creatng->mappos, pos) < get_2d_box_distance(&creatng->mappos, &navi->pos_next))
            {
                navi->pos_next = *pos;
            }

            cannot_move = creature_cannot_move_directly_to_with_collide(creatng, &navi->pos_next, SlbAtFlg_Filled|SlbAtFlg_Valuable, plyr_bits);
            if (cannot_move == 4)
            {
                struct SlabMap *slb;
                stl_num = get_map_index_of_first_block_thing_colliding_with_travelling_to(creatng, &creatng->mappos, &navi->pos_next, SlbAtFlg_Filled|SlbAtFlg_Digable, 0);
                slb = get_slabmap_for_subtile(stl_num_decode_x(stl_num), stl_num_decode_y(stl_num));
                unsigned short ownflag;
                ownflag = 0;
                if (!slabmap_block_invalid(slb)) {
                    ownflag = 1 << slabmap_owner(slb);
                }
                navi->field_19[0] = ownflag;

                if (get_starting_angle_and_side_of_hug(creatng, &navi->pos_next, &navi->angle_D, navi->field_1, SlbAtFlg_Filled|SlbAtFlg_Valuable, plyr_bits))
                {
                    block_flags = get_hugging_blocked_flags(creatng, &navi->pos_next, SlbAtFlg_Filled|SlbAtFlg_Valuable, plyr_bits);
                    set_hugging_pos_using_blocked_flags(&navi->pos_next, creatng, block_flags, thing_nav_sizexy(creatng)/2);
                    if (block_flags == 4)
                    {
                        if ((navi->angle_D == 0) || (navi->angle_D == LbFPMath_PI))
                        {
                            navi->pos_next.y.val = creatng->mappos.y.val;
                            navi->pos_next.z.val = get_thing_height_at(creatng, &creatng->mappos);
                        } else
                        if ((navi->angle_D == LbFPMath_PI/2) || (navi->angle_D == 3*LbFPMath_PI/2)) {
                            navi->pos_next.x.val = creatng->mappos.x.val;
                            navi->pos_next.z.val = get_thing_height_at(creatng, &creatng->mappos);
                        }
                    }
                    navi->field_4 = 1;
                } else
                {
                    initialise_wallhugging_path_from_to(navi, &creatng->mappos, pos);
                }
            }
            if (cannot_move == 1)
            {
                stl_num = get_map_index_of_first_block_thing_colliding_with_travelling_to(creatng, &creatng->mappos, &navi->pos_next, SlbAtFlg_Filled|SlbAtFlg_Digable, 0);
                navi->stl_15 = stl_num;
                nav_radius = thing_nav_sizexy(creatng) / 2;
                MapSubtlCoord stl_x, stl_y;
                stl_x = slab_subtile_center(subtile_slab_fast(stl_num_decode_x(stl_num)));
                stl_y = slab_subtile_center(subtile_slab_fast(stl_num_decode_y(stl_num)));
                if (!find_approach_position_to_subtile(&creatng->mappos, stl_x, stl_y, nav_radius + 3*COORD_PER_STL/2 + 1, &navi->pos_next)) {
                    // Cannot approach selected subtile - reset
                    ERRORLOG("The target subtile (%d,%d) has no approachable position",(int)stl_x,(int)stl_y);
                    initialise_wallhugging_path_from_to(navi, &creatng->mappos, pos);
                    return 1;
                }
                navi->angle_D = get_angle_xy_to(&creatng->mappos, &navi->pos_next);
                navi->navstate = NavS_Unkn3;
                return 1;
            }
        }
        if (navi->field_4 > 0)
        {
            navi->field_4++;
            if (navi->field_4 > 32) {
                ERRORLOG("I've been pushing for a very long time now...");
            }
            if (get_2d_box_distance(&creatng->mappos, &navi->pos_next) <= MOVE_DESTINATION_SPOT_RADIUS)
            {
                navi->field_4 = 0;
                navigation_push_towards_target(navi, creatng, pos, speed, thing_nav_sizexy(creatng)/2, plyr_bits);
            }
        }
        return 1;
    case NavS_Unkn2:
        dist_to_next = get_2d_box_distance(&creatng->mappos, &navi->pos_next);
        if (dist_to_next > MOVE_DESTINATION_SPOT_RADIUS)
        {
            if ((dist_to_next > navi->field_9) || creature_cannot_move_directly_to_with_collide(creatng, &navi->pos_next, SlbAtFlg_Filled|SlbAtFlg_Valuable, plyr_bits))
            {
                initialise_wallhugging_path_from_to(navi, &creatng->mappos, pos);
                return 1;
            }
            return 1;
        }
        if ((get_2d_box_distance(&creatng->mappos, pos) < navi->field_5)
          && thing_can_continue_direct_line_to(creatng, &creatng->mappos, pos, SlbAtFlg_Filled|SlbAtFlg_Valuable, 1, plyr_bits))
        {
            initialise_wallhugging_path_from_to(navi, &creatng->mappos, pos);
            return 1;
        }
        if (creatng->move_angle_xy != navi->angle_D) {
            return 1;
        }
        angle = get_angle_of_wall_hug(creatng, SlbAtFlg_Filled|SlbAtFlg_Valuable, speed, plyr_bits);
        if (angle != navi->angle_D)
        {
          tmpos.x.val = creatng->mappos.x.val + distance_with_angle_to_coord_x(speed, navi->angle_D);
          tmpos.y.val = creatng->mappos.y.val + distance_with_angle_to_coord_y(speed, navi->angle_D);
          tmpos.z.val = get_thing_height_at(creatng, &tmpos);
          if (creature_cannot_move_directly_to_with_collide(creatng, &tmpos, SlbAtFlg_Filled|SlbAtFlg_Valuable, plyr_bits) == 4)
          {
              block_flags = get_hugging_blocked_flags(creatng, &tmpos, SlbAtFlg_Filled|SlbAtFlg_Valuable, plyr_bits);
              set_hugging_pos_using_blocked_flags(&tmpos, creatng, block_flags, thing_nav_sizexy(creatng)/2);
              if (get_2d_box_distance(&tmpos, &creatng->mappos) > MOVE_DESTINATION_SPOT_RADIUS)
              {
                  navi->pos_next = tmpos;
                  navi->field_9 = get_2d_box_distance(&creatng->mappos, &navi->pos_next);
                  return 1;
              }
          }
        }
        if (((angle + LbFPMath_PI/2) & LbFPMath_AngleMask) == navi->angle_D)
        {
            if (navi->field_1[2] == 1)
            {
                navi->field_1[1]++;
            } else
            {
                navi->field_1[2] = 1;
                navi->field_1[1] = 1;
            }
        } else
        if (((angle - LbFPMath_PI/2) & LbFPMath_AngleMask) == navi->angle_D)
        {
          if (navi->field_1[2] == 2)
          {
              navi->field_1[1]++;
          } else
          {
              navi->field_1[2] = 2;
              navi->field_1[1] = 1;
          }
        } else
        {
          navi->field_1[1] = 0;
          navi->field_1[2] = 0;
        }
        if (navi->field_1[1] >= 4)
        {
            initialise_wallhugging_path_from_to(navi, &creatng->mappos, pos);
            return 1;
        }
        navi->angle_D = angle;
        navi->pos_next.x.val = creatng->mappos.x.val + distance_with_angle_to_coord_x(speed, navi->angle_D);
        navi->pos_next.y.val = creatng->mappos.y.val + distance_with_angle_to_coord_y(speed, navi->angle_D);
        navi->pos_next.z.val = get_thing_height_at(creatng, &navi->pos_next);
        tmpos = navi->pos_next;
        check_forward_for_prospective_hugs(creatng, &tmpos, navi->angle_D, navi->field_1[0], SlbAtFlg_Filled|SlbAtFlg_Valuable, speed, plyr_bits);
        if (get_2d_box_distance(&tmpos, &creatng->mappos) > MOVE_DESTINATION_SPOT_RADIUS)
        {
            navi->pos_next = tmpos;
        }
        navi->field_9 = get_2d_box_distance(&creatng->mappos, &navi->pos_next);
        cannot_move = creature_cannot_move_directly_to_with_collide(creatng, &navi->pos_next, SlbAtFlg_Filled|SlbAtFlg_Valuable, plyr_bits);
        if (cannot_move == 4)
        {
          ERRORLOG("I've been given a shite position");
          tmpos.x.val = creatng->mappos.x.val + distance_with_angle_to_coord_x(speed, navi->angle_D);
          tmpos.y.val = creatng->mappos.y.val + distance_with_angle_to_coord_y(speed, navi->angle_D);
          tmpos.z.val = get_thing_height_at(creatng, &tmpos);
          if (creature_cannot_move_directly_to_with_collide(creatng, &tmpos, SlbAtFlg_Filled|SlbAtFlg_Valuable, plyr_bits) == 4) {
              ERRORLOG("It's even more shit than I first thought");
          }
          initialise_wallhugging_path_from_to(navi, &creatng->mappos, pos);
          navi->pos_next = creatng->mappos;
          return 1;
        }
        if (cannot_move == 1)
        {
            stl_num = get_map_index_of_first_block_thing_colliding_with_travelling_to(creatng, &creatng->mappos, &navi->pos_next, SlbAtFlg_Filled|SlbAtFlg_Digable, 0);
            navi->stl_15 = stl_num;
            nav_radius = thing_nav_sizexy(creatng) / 2;
            stl_x = slab_subtile_center(subtile_slab_fast(stl_num_decode_x(stl_num)));
            stl_y = slab_subtile_center(subtile_slab_fast(stl_num_decode_y(stl_num)));
            find_approach_position_to_subtile(&creatng->mappos, stl_x, stl_y, nav_radius + 385, &navi->pos_next);
            navi->angle_D = get_angle_xy_to(&creatng->mappos, &navi->pos_next);
            navi->field_1[1] = 0;
            navi->field_1[2] = 0;
            navi->navstate = NavS_Unkn4;
        }
        navi->field_9 = get_2d_box_distance(&creatng->mappos, &navi->pos_next);
        return 1;
    case NavS_Unkn4:
        dist_to_next = get_2d_box_distance(&creatng->mappos, &navi->pos_next);
        if (dist_to_next > MOVE_DESTINATION_SPOT_RADIUS)
        {
            if (get_2d_box_distance(&creatng->mappos, &navi->pos_next) > navi->field_9
             || creature_cannot_move_directly_to_with_collide(creatng, &navi->pos_next, SlbAtFlg_Filled|SlbAtFlg_Valuable, plyr_bits))
            {
                initialise_wallhugging_path_from_to(navi, &creatng->mappos, pos);
            }
            navi->navstate = NavS_Unkn4;
            return 1;
        }
        stl_x = slab_subtile_center(subtile_slab_fast(stl_num_decode_x(navi->stl_15)));
        stl_y = slab_subtile_center(subtile_slab_fast(stl_num_decode_y(navi->stl_15)));
        tmpos.x.val = subtile_coord_center(stl_x);
        tmpos.y.val = subtile_coord_center(stl_y);
        navi->angle_D = get_angle_xy_to(&creatng->mappos, &tmpos);
        navi->field_1[1] = 0;
        navi->field_1[2] = 0;
        navi->field_9 = 0;
        if (get_angle_difference(creatng->move_angle_xy, navi->angle_D) != 0) {
            navi->navstate = NavS_Unkn4;
            return 1;
        }
        navi->navstate = NavS_Unkn6;
        stl_num = get_subtile_number(stl_x,stl_y);
        navi->stl_15 = stl_num;
        navi->stl_17 = stl_num;
        return 2;
    case NavS_Unkn3:
        dist_to_next = get_2d_box_distance(&creatng->mappos, &navi->pos_next);
        if (dist_to_next > MOVE_DESTINATION_SPOT_RADIUS)
        {
            navi->angle_D = get_angle_xy_to(&creatng->mappos, &navi->pos_next);
            navi->navstate = NavS_Unkn3;
            return 1;
        }
        stl_x = slab_subtile_center(subtile_slab_fast(stl_num_decode_x(navi->stl_15)));
        stl_y = slab_subtile_center(subtile_slab_fast(stl_num_decode_y(navi->stl_15)));
        tmpos.x.val = subtile_coord_center(stl_x);
        tmpos.y.val = subtile_coord_center(stl_y);
        tmpos.z.val = get_thing_height_at(creatng, &tmpos);
        navi->angle_D = get_angle_xy_to(&creatng->mappos, &tmpos);
        if (get_angle_difference(creatng->move_angle_xy, navi->angle_D) != 0) {
            navi->navstate = NavS_Unkn3;
            return 1;
        }
        navi->navstate = NavS_Unkn5;
        stl_num = get_subtile_number(stl_x,stl_y);
        navi->stl_15 = stl_num;
        navi->stl_17 = stl_num;
        return 2;
    case NavS_Unkn6:
        stl_x = slab_subtile_center(subtile_slab_fast(stl_num_decode_x(navi->stl_15)));
        stl_y = slab_subtile_center(subtile_slab_fast(stl_num_decode_y(navi->stl_15)));
        stl_num = get_subtile_number(stl_x,stl_y);
        navi->stl_15 = stl_num;
        navi->stl_17 = stl_num;
        mapblk = get_map_block_at_pos(navi->stl_15);
        if ((mapblk->flags & SlbAtFlg_Blocking) != 0) {
          return 2;
        }
        nav_radius = thing_nav_sizexy(creatng) / 2;
        if (navi->field_1[0] == 1) {
            i = (creatng->move_angle_xy + LbFPMath_PI/4) / (LbFPMath_PI/2) - 1;
        } else {
            i = (creatng->move_angle_xy + LbFPMath_PI/4) / (LbFPMath_PI/2) + 1;
        }
        navi->pos_next.x.val += (3*COORD_PER_STL/2 - nav_radius) * small_around[i&3].delta_x;
        navi->pos_next.y.val += (3*COORD_PER_STL/2 - nav_radius) * small_around[i&3].delta_y;
        i = (creatng->move_angle_xy + LbFPMath_PI/4) / (LbFPMath_PI/2);
        navi->pos_next.x.val += (COORD_PER_STL/2) * small_around[i&3].delta_x;
        i = (creatng->move_angle_xy) / (LbFPMath_PI/2);
        navi->pos_next.y.val += (COORD_PER_STL/2) * small_around[i&3].delta_y;
        navi->navstate = NavS_Unkn7;
        return 1;
    case NavS_Unkn5:
        stl_x = slab_subtile_center(subtile_slab_fast(stl_num_decode_x(navi->stl_15)));
        stl_y = slab_subtile_center(subtile_slab_fast(stl_num_decode_y(navi->stl_15)));
        stl_num = get_subtile_number(stl_x,stl_y);
        navi->stl_15 = stl_num;
        navi->stl_17 = stl_num;
        mapblk = get_map_block_at_pos(navi->stl_15);
        if ((mapblk->flags & SlbAtFlg_Blocking) != 0) {
            return 2;
        }
        navi->navstate = NavS_Unkn1;
        return 1;
    case NavS_Unkn7:
        if (get_2d_box_distance(&creatng->mappos, &navi->pos_next) > MOVE_DESTINATION_SPOT_RADIUS)
        {
            return 1;
        }
        if (navi->field_1[0] == 1)
            angle = creatng->move_angle_xy + LbFPMath_PI/2;
        else
            angle = creatng->move_angle_xy - LbFPMath_PI/2;
        navi->angle_D = angle & LbFPMath_AngleMask;
        navi->navstate = NavS_Unkn2;
        return 1;
    default:
        break;
    }
    return 1;
}

TbBool slab_good_for_computer_dig_path(const struct SlabMap *slb)
{
    const struct SlabAttr *slbattr;
    slbattr = get_slab_attrs(slb);
    if ( ((slbattr->block_flags & (SlbAtFlg_Filled|SlbAtFlg_Digable|SlbAtFlg_Valuable)) != 0) || (slb->kind == SlbT_LAVA) )
        return true;
    return false;
}

TbBool is_valid_hug_subtile(MapSubtlCoord stl_x, MapSubtlCoord stl_y, PlayerNumber plyr_idx)
{
    struct SlabMap *slb;
    const struct SlabAttr *slbattr;
    slb = get_slabmap_for_subtile(stl_x, stl_y);
    slbattr = get_slab_attrs(slb);
    if ((slbattr->is_diggable) && !slab_kind_is_indestructible(slb->kind))
    {
        struct Map *mapblk;
        mapblk = get_map_block_at(stl_x, stl_y);
        if (((mapblk->flags & SlbAtFlg_Filled) == 0) || (slabmap_owner(slb) == plyr_idx)) {
            SYNCDBG(17,"Subtile (%d,%d) rejected based on attrs",(int)stl_x,(int)stl_y);
            return false;
        }
    }
    if (!slab_good_for_computer_dig_path(slb)) {
        SYNCDBG(17,"Subtile (%d,%d) rejected as not good for dig",(int)stl_x,(int)stl_y);
        return false;
    }
    return true;
}

long dig_to_position(PlayerNumber plyr_idx, MapSubtlCoord basestl_x, MapSubtlCoord basestl_y, int direction_around, TbBool revside)
{
    long i,round_idx,round_change;
    //return _DK_dig_to_position(plyr_idx, basestl_x, basestl_y, direction_around, revside);
    SYNCDBG(14,"Starting for subtile (%d,%d)",(int)basestl_x,(int)basestl_y);
    if (revside) {
      round_change = 1;
    } else {
      round_change = 3;
    }
    round_idx = (direction_around + SMALL_AROUND_LENGTH - round_change) % SMALL_AROUND_LENGTH;
    for (i=0; i < SMALL_AROUND_LENGTH; i++)
    {
        MapSubtlCoord stl_x,stl_y;
        stl_x = basestl_x + STL_PER_SLB * (int)small_around[round_idx].delta_x;
        stl_y = basestl_y + STL_PER_SLB * (int)small_around[round_idx].delta_y;
        if (!is_valid_hug_subtile(stl_x, stl_y, plyr_idx))
        {
            SYNCDBG(7,"Subtile (%d,%d) accepted",(int)stl_x,(int)stl_y);
            SubtlCodedCoords stl_num;
            stl_num = get_subtile_number(stl_x, stl_y);
            return stl_num;
        }
        round_idx = (round_idx + round_change) % SMALL_AROUND_LENGTH;
    }
    return -1;
}

static inline void get_hug_side_next_step(MapSubtlCoord dst_stl_x, MapSubtlCoord dst_stl_y, int dirctn, PlayerNumber plyr_idx,
    char *state, MapSubtlCoord *ostl_x, MapSubtlCoord *ostl_y, short *round, int *maxdist)
{
    MapSubtlCoord curr_stl_x, curr_stl_y;
    curr_stl_x = *ostl_x;
    curr_stl_y = *ostl_y;
    unsigned short round_idx;
    round_idx = small_around_index_in_direction(curr_stl_x, curr_stl_y, dst_stl_x, dst_stl_y);
    int dist;
    dist = max(abs(curr_stl_x - dst_stl_x), abs(curr_stl_y - dst_stl_y));
    int dx,dy;
    dx = small_around[round_idx].delta_x;
    dy = small_around[round_idx].delta_y;
    // If we can follow direction straight to the target, and we will get closer to it, then do it
    if ((dist <= *maxdist) && is_valid_hug_subtile(curr_stl_x + STL_PER_SLB*dx, curr_stl_y + STL_PER_SLB*dy, plyr_idx))
    {
        curr_stl_x += STL_PER_SLB*dx;
        curr_stl_y += STL_PER_SLB*dy;
        *state = WaHSS_Val1;
        *maxdist = max(abs(curr_stl_x - dst_stl_x), abs(curr_stl_y - dst_stl_y));
    } else
    // If met second wall, finish
    if (*state == WaHSS_Val1)
    {
        *state = WaHSS_Val2;
    } else
    { // Here we need to use wallhug to slide until we will be able to move towards destination again
        int n;
        // Try directions starting at the one towards the wall, in case wall has ended
        round_idx = (*round + SMALL_AROUND_LENGTH + dirctn) % SMALL_AROUND_LENGTH;
        for (n = 0; n < SMALL_AROUND_LENGTH; n++)
        {
            dx = small_around[round_idx].delta_x;
            dy = small_around[round_idx].delta_y;
            if (!is_valid_hug_subtile(curr_stl_x + STL_PER_SLB*dx, curr_stl_y + STL_PER_SLB*dy, plyr_idx))
            {
                break;
            }
            // If direction not for wallhug, try next
            round_idx = (round_idx + SMALL_AROUND_LENGTH - dirctn) % SMALL_AROUND_LENGTH;
        }
        if ((n < SMALL_AROUND_LENGTH) || (dirctn > 0)) {
            dx = small_around[round_idx].delta_x;
            dy = small_around[round_idx].delta_y;
            *round = round_idx;
            curr_stl_x += STL_PER_SLB*dx;
            curr_stl_y += STL_PER_SLB*dy;
        }
    }

    *ostl_x = curr_stl_x;
    *ostl_y = curr_stl_y;
}

short get_hug_side_options(MapSubtlCoord src_stl_x, MapSubtlCoord src_stl_y, MapSubtlCoord dst_stl_x, MapSubtlCoord dst_stl_y,
    unsigned short direction, PlayerNumber plyr_idx, MapSubtlCoord *ostla_x, MapSubtlCoord *ostla_y, MapSubtlCoord *ostlb_x, MapSubtlCoord *ostlb_y)
{
    SYNCDBG(4,"Starting");
    MapSubtlCoord stl_b_x, stl_b_y;
    MapSubtlCoord stl_a_x, stl_a_y;
    int maxdist_a,maxdist_b;
    short round_a,round_b;
    char state_a, state_b;
    int dist;
    dist = max(abs(src_stl_x - dst_stl_x), abs(src_stl_y - dst_stl_y));

    state_a = WaHSS_Val0;
    stl_a_x = src_stl_x;
    stl_a_y = src_stl_y;
    round_a = (direction + SMALL_AROUND_LENGTH + 1) % SMALL_AROUND_LENGTH;
    maxdist_a = dist - 1;

    state_b = WaHSS_Val0;
    stl_b_x = src_stl_x;
    stl_b_y = src_stl_y;
    round_b = (direction + SMALL_AROUND_LENGTH - 1) % SMALL_AROUND_LENGTH;
    maxdist_b = dist - 1;

    // Try moving in both directions
    int i;
    for (i = 150; i > 0; i--)
    {
        if ((state_a == WaHSS_Val2) && (state_b == WaHSS_Val2)) {
            break;
        }
        if (state_a != WaHSS_Val2)
        {
            get_hug_side_next_step(dst_stl_x, dst_stl_y, -1, plyr_idx, &state_a, &stl_a_x, &stl_a_y, &round_a, &maxdist_a);
            if ((stl_a_x == dst_stl_x) && (stl_a_y == dst_stl_y)) {
                *ostla_x = stl_a_x;
                *ostla_y = stl_a_y;
                *ostlb_x = stl_b_x;
                *ostlb_y = stl_b_y;
                return 1;
            }
        }
        if (state_b != WaHSS_Val2)
        {
            get_hug_side_next_step(dst_stl_x, dst_stl_y, 1, plyr_idx, &state_b, &stl_b_x, &stl_b_y, &round_b, &maxdist_b);
            if ((stl_b_x == dst_stl_x) && (stl_b_y == dst_stl_y)) {
                *ostla_x = stl_a_x;
                *ostla_y = stl_a_y;
                *ostlb_x = stl_b_x;
                *ostlb_y = stl_b_y;
                return 0;
            }
        }
    }
    *ostla_x = stl_a_x;
    *ostla_y = stl_a_y;
    *ostlb_x = stl_b_x;
    *ostlb_y = stl_b_y;
    return 2;
}
/******************************************************************************/
#ifdef __cplusplus
}
#endif
