/******************************************************************************/
// Free implementation of Bullfrog's Dungeon Keeper strategy game.
/******************************************************************************/
/** @file map_data.c
 *     Map array data management functions.
 * @par Purpose:
 *     Functions to support the map data array, which stores map blocks information.
 * @par Comment:
 *     None.
 * @author   Tomasz Lis
 * @date     15 May 2009 - 12 Apr 2009
 * @par  Copying and copyrights:
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 2 of the License, or
 *     (at your option) any later version.
 */
/******************************************************************************/
#include "map_data.h"
#include "globals.h"

#include "slab_data.h"
#include "keeperfx.h"

#ifdef __cplusplus
extern "C" {
#endif
/******************************************************************************/
struct Map bad_map_block;
const long map_to_slab[] = {
   0,  0,  0,  1,  1,  1,  2,  2,  2,  3,  3,  3,  4,  4,  4,  5,  5,  5,
   6,  6,  6,  7,  7,  7,  8,  8,  8,  9,  9,  9, 10, 10, 10, 11, 11, 11,
  12, 12, 12, 13, 13, 13, 14, 14, 14, 15, 15, 15, 16, 16, 16, 17, 17, 17,
  18, 18, 18, 19, 19, 19, 20, 20, 20, 21, 21, 21, 22, 22, 22, 23, 23, 23,
  24, 24, 24, 25, 25, 25, 26, 26, 26, 27, 27, 27, 28, 28, 28, 29, 29, 29,
  30, 30, 30, 31, 31, 31, 32, 32, 32, 33, 33, 33, 34, 34, 34, 35, 35, 35,
  36, 36, 36, 37, 37, 37, 38, 38, 38, 39, 39, 39, 40, 40, 40, 41, 41, 41,
  42, 42, 42, 43, 43, 43, 44, 44, 44, 45, 45, 45, 46, 46, 46, 47, 47, 47,
  48, 48, 48, 49, 49, 49, 50, 50, 50, 51, 51, 51, 52, 52, 52, 53, 53, 53,
  54, 54, 54, 55, 55, 55, 56, 56, 56, 57, 57, 57, 58, 58, 58, 59, 59, 59,
  60, 60, 60, 61, 61, 61, 62, 62, 62, 63, 63, 63, 64, 64, 64, 65, 65, 65,
  66, 66, 66, 67, 67, 67, 68, 68, 68, 69, 69, 69, 70, 70, 70, 71, 71, 71,
  72, 72, 72, 73, 73, 73, 74, 74, 74, 75, 75, 75, 76, 76, 76, 77, 77, 77,
  78, 78, 78, 79, 79, 79, 80, 80, 80, 81, 81, 81, 82, 82, 82, 83, 83, 83,
  84, 84, 84, 85, 85, 85, 86, 86, 86, 87, 87, 87, 88, 88, 88, 89, 89, 89,
};
/******************************************************************************/
/*
 * Returns if the subtile coords are in range of subtiles which have slab entry.
 */
TbBool subtile_has_slab(long stl_x, long stl_y)
{
  if ((stl_x >= 0) && (stl_x < 3*map_tiles_x))
    if ((stl_y >= 0) && (stl_y < 3*map_tiles_y))
      return true;
  return false;
}

/*
 * Returns if the subtile coords are in range map subtiles.
 */
TbBool subtile_coords_invalid(long stl_x, long stl_y)
{
  if ((stl_x < 0) || (stl_x > map_subtiles_x))
      return true;
  if ((stl_y < 0) || (stl_y > map_subtiles_y))
      return true;
  return false;
}

struct Map *get_map_block(long stl_x, long stl_y)
{
  if ((stl_x < 0) || (stl_x > map_subtiles_x))
      return &bad_map_block;
  if ((stl_y < 0) || (stl_y > map_subtiles_y))
      return &bad_map_block;
  return &game.map[get_subtile_number(stl_x,stl_y)];
}

TbBool map_block_invalid(struct Map *map)
{
  if (map == NULL)
    return true;
  if (map == &bad_map_block)
    return true;
  return (map < &game.map[0]);
}

unsigned long get_map_flags(long stl_x, long stl_y)
{
  if ((stl_x < 0) || (stl_x > map_subtiles_x))
      return 0;
  if ((stl_y < 0) || (stl_y > map_subtiles_y))
      return 0;
  return game.mapflags[get_subtile_number(stl_x,stl_y)];
}

long get_ceiling_height(struct Coord3d *pos)
{
  long i;
  i = get_subtile_number(pos->x.stl.num,pos->y.stl.num);
  return ((game.map[i].data & 0xF000000u) >> 24) << 8;
}

long thing_index_on_map_block(struct Map *map)
{
  return ((map->data >> 11) & 0x7FF);
}

void reveal_map_subtile(long stl_x, long stl_y, long plyr_idx)
{
  unsigned short nflag;
  struct Map *map;
  unsigned long i;
  nflag = (1 << plyr_idx);
  map = get_map_block(stl_x, stl_y);
  i = (map->data >> 28) | nflag;
  map->data |= (i & 0x0F) << 28;
}

TbBool subtile_revealed(long stl_x, long stl_y, long plyr_idx)
{
  unsigned short nflag;
  struct Map *map;
  nflag = (1 << plyr_idx);
  map = get_map_block(stl_x, stl_y);
  if (map_block_invalid(map))
    return false;
  if ((map->data >> 28) & nflag)
    return true;
  return false;
}

TbBool map_block_revealed(struct Map *map, long plyr_idx)
{
  unsigned short nflag;
  nflag = (1 << plyr_idx);
  if (map_block_invalid(map))
    return false;
  if ((map->data >> 28) & nflag)
    return true;
  return false;
}

/******************************************************************************/

TbBool set_coords_to_subtile_center(struct Coord3d *pos, long stl_x, long stl_y, long stl_z)
{
  if (stl_x > map_subtiles_x+1) stl_x = map_subtiles_x+1;
  if (stl_y > map_subtiles_y+1) stl_y = map_subtiles_y+1;
  if (stl_z > 16) stl_z = 16;
  if (stl_x < 0)  stl_x = 0;
  if (stl_y < 0) stl_y = 0;
  if (stl_z < 0) stl_z = 0;
  pos->x.val = (stl_x<<8) + 128;
  pos->y.val = (stl_y<<8) + 128;
  pos->z.val = (stl_z<<8) + 128;
  return true;
}

TbBool set_coords_to_slab_center(struct Coord3d *pos, long slb_x, long slb_y)
{
  return set_coords_to_subtile_center(pos, slb_x*3+1,slb_y*3+1, 1);
}

/*
 * Subtile number - stores both X and Y coords in one number.
 */
unsigned long get_subtile_number(long stl_x, long stl_y)
{
  if (stl_x > map_subtiles_x+1) stl_x = map_subtiles_x+1;
  if (stl_y > map_subtiles_y+1) stl_y = map_subtiles_y+1;
  if (stl_x < 0)  stl_x = 0;
  if (stl_y < 0) stl_y = 0;
  return stl_y*(map_subtiles_x+1) + stl_x;
}

/*
 * Decodes X coordinate from subtile number.
 */
long stl_num_decode_x(unsigned long stl_num)
{
  return stl_num % (map_subtiles_x+1);
}

/*
 * Decodes Y coordinate from subtile number.
 */
long stl_num_decode_y(unsigned long stl_num)
{
  return (stl_num/(map_subtiles_x+1))%map_subtiles_y;
}

/*
 * Returns subtile number for center subtile on given slab.
 */
unsigned long get_subtile_number_at_slab_center(long slb_x, long slb_y)
{
  return get_subtile_number(slb_x*3+1,slb_y*3+1);
}

/*
 * Returns subtile coordinate for central subtile on given slab.
 */
long slab_center_subtile(long stl_v)
{
  return map_to_slab[stl_v]*3+1;
}

/*
 * Returns subtile coordinate for starting subtile on given slab.
 */
long slab_starting_subtile(long stl_v)
{
  return map_to_slab[stl_v]*3;
}

/*
 * Returns subtile coordinate for ending subtile on given slab.
 */
long slab_ending_subtile(long stl_v)
{
  return map_to_slab[stl_v]*3+2;
}
/******************************************************************************/

void clear_mapwho(void)
{
  //_DK_clear_mapwho();
  struct Map *map;
  unsigned long x,y;
  for (y=0; y < (map_subtiles_y+1); y++)
    for (x=0; x < (map_subtiles_x+1); x++)
    {
      map = &game.map[get_subtile_number(x,y)];
      map->data &= 0xFFC007FFu;
    }
}

void clear_mapmap_soft(void)
{
  struct Map *map;
  unsigned long x,y;
  unsigned short *wptr;
  for (y=0; y < (map_subtiles_y+1); y++)
    for (x=0; x < (map_subtiles_x+1); x++)
    {
      map = &game.map[get_subtile_number(x,y)];
      wptr = &game.field_46157[get_subtile_number(x,y)];
      map->data &= 0xFF3FFFFFu;
      map->data &= 0xFFFFF800u;
      map->data &= 0xFFC007FFu;
      map->data &= 0x0FFFFFFFu;
      map->flags = 0;
      *wptr = 8192;
    }
}

void clear_mapmap(void)
{
  struct Map *map;
  unsigned long x,y;
  unsigned short *wptr;
  unsigned char *flg;
  for (y=0; y < (map_subtiles_y+1); y++)
    for (x=0; x < (map_subtiles_x+1); x++)
    {
      map = get_map_block(x,y);
      wptr = &game.field_46157[get_subtile_number(x,y)];
      flg = &game.mapflags[get_subtile_number(x,y)];
      memset(map, 0, sizeof(struct Map));
      *wptr = 8192;
      *flg = 0;
    }
}

/*
 * Clears digging operations for given player on given map slabs rectangle.
 */
void clear_dig_for_map_rect(long plyr_idx,long start_x,long end_x,long start_y,long end_y)
{
  long x,y;
  for (y = start_y; y < end_y; y++)
    for (x = start_x; x < end_x; x++)
    {
      clear_slab_dig(x, y, plyr_idx);
    }
}

/*
 * Reveals map subtiles rectangle for given player.
 * Low level function - use reveal_map_area() instead.
 */
void reveal_map_rect(long plyr_idx,long start_x,long end_x,long start_y,long end_y)
{
  long x,y;
  for (y = start_y; y < end_y; y++)
    for (x = start_x; x < end_x; x++)
    {
      reveal_map_subtile(x, y, plyr_idx);
    }
}

/*
 * Reveals map subtiles rectangle for given player.
 */
void reveal_map_area(long plyr_idx,long start_x,long end_x,long start_y,long end_y)
{
  start_x = slab_starting_subtile(start_x);
  start_y = slab_starting_subtile(start_y);
  end_x = slab_ending_subtile(end_x)+1;
  end_y = slab_ending_subtile(end_y)+1;
  clear_dig_for_map_rect(plyr_idx,map_to_slab[start_x],map_to_slab[end_x],
      map_to_slab[start_y],map_to_slab[end_y]);
  reveal_map_rect(plyr_idx,start_x,end_x,start_y,end_y);
  pannel_map_update(start_x,start_y,end_x,end_y);
}
/******************************************************************************/

/******************************************************************************/
#ifdef __cplusplus
}
#endif