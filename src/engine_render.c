/******************************************************************************/
// Free implementation of Bullfrog's Dungeon Keeper strategy game.
/******************************************************************************/
/** @file engine_render.c
 *     Rendering the 3D view functions.
 * @par Purpose:
 *     Functions for displaying drawlist elements on screen.
 * @par Comment:
 *     None.
 * @author   Tomasz Lis
 * @date     20 Mar 2009 - 30 Mar 2009
 * @par  Copying and copyrights:
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 2 of the License, or
 *     (at your option) any later version.
 */
/******************************************************************************/
#include "engine_render.h"

#include "globals.h"
#include "bflib_basics.h"
#include "bflib_memory.h"
#include "bflib_video.h"
#include "bflib_sprite.h"
#include "bflib_vidraw.h"

#include "engine_lenses.h"
#include "engine_camera.h"
#include "kjm_input.h"
#include "keeperfx.h"

#ifdef __cplusplus
extern "C" {
#endif
/******************************************************************************/
DLLIMPORT void _DK_draw_element(struct Map *map, long a2, long a3, long a4, long a5, long a6, long a7, unsigned char a8, long *a9);
DLLIMPORT long _DK_convert_world_coord_to_front_view_screen_coord(struct Coord3d *pos, struct Camera *cam, long *x, long *y, long *z);
DLLIMPORT void _DK_display_fast_drawlist(struct Camera *cam);
DLLIMPORT void _DK_draw_frontview_engine(struct Camera *cam);
DLLIMPORT void _DK_display_drawlist(void);
DLLIMPORT void _DK_draw_view(struct Camera *cam, unsigned char a2);
DLLIMPORT void _DK_do_a_plane_of_engine_columns_perspective(long a1, long a2, long a3, long a4);
DLLIMPORT void _DK_do_a_plane_of_engine_columns_cluedo(long a1, long a2, long a3, long a4);
DLLIMPORT void _DK_do_a_plane_of_engine_columns_isometric(long a1, long a2, long a3, long a4);
DLLIMPORT void _DK_rotpers_parallel_3(struct EngineCoord *epos, struct M33 *matx);
DLLIMPORT void _DK_rotate_base_axis(struct M33 *matx, short a2, unsigned char a3);
DLLIMPORT void _DK_fill_in_points_perspective(long a1, long a2, struct MinMax *mm);
DLLIMPORT void _DK_fill_in_points_cluedo(long a1, long a2, struct MinMax *mm);
DLLIMPORT void _DK_fill_in_points_isometric(long a1, long a2, struct MinMax *mm);
DLLIMPORT void _DK_find_gamut(void);
DLLIMPORT void _DK_fiddle_gamut(long a1, long a2);
DLLIMPORT void _DK_create_map_volume_box(long a1, long a2, long a3);
DLLIMPORT void _DK_frame_wibble_generate(void);
DLLIMPORT void _DK_setup_rotate_stuff(long a1, long a2, long a3, long a4, long a5, long a6, long a7, long a8);
/******************************************************************************/
unsigned short shield_offset[] = {
 0x0,  0x100, 0x100, 0x100, 0x100, 0x100, 0x100, 0x100, 0x100, 0x100, 0x100, 0x100, 0x100, 0x100, 0x118, 0x80,
 0x80, 0x100,  0x80,  0x80, 0x100, 0x100, 0x138,  0x80,  0x80, 0x138,  0x80,  0x80, 0x100,  0x80, 0x80, 0x100,
};
long const x_offs[] =  { 0, 1, 1, 0};
long const y_offs[] =  { 0, 0, 1, 1};
long const x_step1[] = { 0,-1, 0, 1};
long const y_step1[] = { 1, 0,-1, 0};
long const x_step2[] = { 1, 0,-1, 0};
long const y_step2[] = { 0, 1, 0,-1};
long const orient_table_xflip[] =  {0, 0, 1, 1};
long const orient_table_yflip[] =  {0, 1, 1, 0};
long const orient_table_rotate[] = {0, 1, 0, 1};
unsigned char i_can_see_levels[] = {15, 20, 25, 30,};
unsigned char temp_cluedo_mode;
/******************************************************************************/
void gpoly_enable_pentium_pro(TbBool state)
{
#if (BFDEBUG_LEVEL > 4)
  LbSyncLog("Pentium Pro polygon rendering %s\n",state?"on":"off");
#endif
  if (state)
    gpoly_pro_enable_mode_ofs = (1<<6);
  else
    gpoly_pro_enable_mode_ofs = 0;
}

long compute_cells_away(void)
{
  static const char *func_name="compute_cells_away";
  long xmin,ymin,xmax,ymax;
  long xcell,ycell;
  struct PlayerInfo *player;
  long ncells_a;
  player = &(game.players[my_player_number%PLAYERS_COUNT]);
    if ((vert_offset[1]) || (hori_offset[1]))
    {
      xcell = 660/pixel_size - player->engine_window_x/pixel_size - x_init_off;
      ycell = (8 * high_offset[1] >> 8) - 20/pixel_size - player->engine_window_y/pixel_size - y_init_off;
      ymax = (((vert_offset[1] * xcell) >> 1) - ((vert_offset[0] * ycell) >> 1))
         / ((hori_offset[0] * vert_offset[1] - vert_offset[0] * hori_offset[1]) >> 11) >> 2;
      xmax = (((hori_offset[1] * xcell) >> 1) - ((hori_offset[0] * ycell) >> 1))
         / ((vert_offset[0] * hori_offset[1] - hori_offset[0] * vert_offset[1]) >> 11) >> 2;
    } else
    {
      ymax = 0;
      xmax = 0;
    }
    if ((vert_offset[1]) || (hori_offset[1]))
    {
      xcell = 320 / pixel_size - player->engine_window_x/pixel_size - x_init_off;
      ycell = 200 / pixel_size - ymin - y_init_off;
      ymin = (((vert_offset[1] * xcell) >> 1) - ((vert_offset[0] * ycell) >> 1))
          / ((hori_offset[0] * vert_offset[1] - vert_offset[0] * hori_offset[1]) >> 11) >> 2;
      xmin = (((hori_offset[1] * xcell) >> 1) - ((hori_offset[0] * ycell) >> 1))
         / ((vert_offset[0] * hori_offset[1] - hori_offset[0] * vert_offset[1]) >> 11) >> 2;
    } else
    {
      ymin = 0;
      xmin = 0;
    }
    xcell = abs(ymax - ymin);
    ycell = abs(xmax - xmin);
    if (ycell >= xcell)
      ncells_a = ycell + (xcell >> 1);
    else
      ncells_a = xcell + (ycell >> 1);
    ncells_a += 2;
    if (ncells_a > max_i_can_see)
      ncells_a = max_i_can_see;
    return ncells_a;
}

void init_coords_and_rotation(struct EngineCoord *origin,struct M33 *matx)
{
  origin->x = 0;
  origin->y = 0;
  origin->z = 0;
  matx->r0[0] = 0x4000u;
  matx->r0[1] = 0;
  matx->r0[2] = 0;
  matx->r1[0] = 0;
  matx->r1[1] = 0x4000u;
  matx->r1[2] = 0;
  matx->r2[0] = 0;
  matx->r2[1] = 0;
  matx->r2[2] = 0x4000u;
}

void update_fade_limits(long ncells_a)
{
  fade_max = (ncells_a << 8);
  fade_scaler = (ncells_a << 8);
  fade_way_out = (ncells_a + 1) << 8;
  fade_min = 768 * ncells_a / 4;
  split_1 = (split1at << 8);
  split_2 = (split2at << 8);
}

void update_normal_shade(struct M33 *matx)
{
  normal_shade_left = matx->r2[0];
  normal_shade_right = -matx->r2[0];
  normal_shade_back = -matx->r2[2];
  normal_shade_front = matx->r2[2];
  if (normal_shade_front < 0)
    normal_shade_front = 0;
  if (normal_shade_back < 0)
    normal_shade_back = 0;
  if (normal_shade_left < 0)
    normal_shade_left = 0;
  if (normal_shade_right < 0)
    normal_shade_right = 0;
}

void update_engine_settings(struct PlayerInfo *player)
{
  engine_player_number = player->field_2B;
  player_bit = (1 << engine_player_number);
  switch (settings.field_0)
  {
  case 0:
      split1at = 4;
      split2at = 3;
      break;
  case 1:
      split1at = 3;
      split2at = 2;
      break;
  case 2:
      split1at = 2;
      split2at = 1;
      break;
  case 3:
      split1at = 0;
      split2at = 0;
      break;
  }
  me_pointed_at = NULL;
  me_distance = 100000000;
  max_i_can_see = i_can_see_levels[settings.view_distance % 4];
  if (lens_mode != 0)
    temp_cluedo_mode = 0;
  else
    temp_cluedo_mode = settings.video_cluedo_mode;
  thing_pointed_at = NULL;
}

TbBool is_free_space_in_poly_pool(int nitems)
{
  return (getpoly+(nitems*sizeof(struct BasicQ)) <= poly_pool_end);
}

void rotpers_parallel_3(struct EngineCoord *epos, struct M33 *matx)
{
  _DK_rotpers_parallel_3(epos, matx);
}

void rotate_base_axis(struct M33 *matx, short a2, unsigned char a3)
{
  _DK_rotate_base_axis(matx, a2, a3);
}

void fill_in_points_perspective(long a1, long a2, struct MinMax *mm)
{
  _DK_fill_in_points_perspective(a1, a2, mm);
}

void fill_in_points_cluedo(long a1, long a2, struct MinMax *mm)
{
  _DK_fill_in_points_cluedo(a1, a2, mm);
}

void fill_in_points_isometric(long a1, long a2, struct MinMax *mm)
{
  _DK_fill_in_points_isometric(a1, a2, mm);
}

void frame_wibble_generate(void)
{
  _DK_frame_wibble_generate();
}

void setup_rotate_stuff(long a1, long a2, long a3, long a4, long a5, long a6, long a7, long a8)
{
  _DK_setup_rotate_stuff(a1, a2, a3, a4, a5, a6, a7, a8);
}

void do_perspective_rotation(long x, long y, long z)
{
  struct PlayerInfo *player;
  struct EngineCoord epos;

  player = &(game.players[my_player_number%PLAYERS_COUNT]);
    epos.x = -x;
    epos.y = 0;
    epos.z = y;
    rotpers_parallel_3(&epos, &camera_matrix);
    x_init_off = epos.field_0;
    y_init_off = epos.field_4;
    depth_init_off = epos.z;
    epos.x = 65536;
    epos.y = 0;
    epos.z = 0;
    rotpers_parallel_3(&epos, &camera_matrix);
    hori_offset[0] = epos.field_0 - ((player->engine_window_width/pixel_size) >> 1);
    hori_offset[1] = epos.field_4 - ((player->engine_window_height/pixel_size) >> 1);
    hori_offset[2] = epos.z;
    epos.x = 0;
    epos.y = 0;
    epos.z = -65536;
    rotpers_parallel_3(&epos, &camera_matrix);
    vert_offset[0] = epos.field_0 - ((player->engine_window_width/pixel_size) >> 1);
    vert_offset[1] = epos.field_4 - ((player->engine_window_height/pixel_size) >> 1);
    vert_offset[2] = epos.z;
    epos.x = 0;
    epos.y = 65536;
    epos.z = 0;
    rotpers_parallel_3(&epos, &camera_matrix);
    high_offset[0] = epos.field_0 - ((player->engine_window_width/pixel_size) >> 1);
    high_offset[1] = epos.field_4 - ((player->engine_window_height/pixel_size) >> 1);
    high_offset[2] = epos.z;
}

void find_gamut(void)
{
  static const char *func_name="find_gamut";
#if (BFDEBUG_LEVEL > 19)
    SYNCDBG(0,"Starting");
#endif
  _DK_find_gamut();
}

void fiddle_gamut(long a1, long a2)
{
  _DK_fiddle_gamut(a1, a2);
}

void create_map_volume_box(long x, long y, long z)
{
  _DK_create_map_volume_box(x, y, z);
}

void do_a_plane_of_engine_columns_perspective(long a1, long a2, long a3, long a4)
{
  _DK_do_a_plane_of_engine_columns_perspective(a1, a2, a3, a4);
}

void do_a_plane_of_engine_columns_cluedo(long a1, long a2, long a3, long a4)
{
  _DK_do_a_plane_of_engine_columns_cluedo(a1, a2, a3, a4);
}

void do_a_plane_of_engine_columns_isometric(long a1, long a2, long a3, long a4)
{
  _DK_do_a_plane_of_engine_columns_isometric(a1, a2, a3, a4);
}

void create_box_coords(struct EngineCoord *coord, long x, long z, long y)
{
  coord->x = x;
  coord->z = z;
  coord->field_8 = 0;
  coord->y = y;
  rotpers(coord, &camera_matrix);
}

void draw_map_volume_box(long a1, long a2, long a3, long a4, long a5, unsigned char color)
{
  map_volume_box.field_0 = 1;
  map_volume_box.field_3 = a1 & 0xFFFF0000;
  map_volume_box.field_7 = a2 & 0xFF00;
  map_volume_box.field_B = a3 & 0xFFFF0000;
  map_volume_box.field_13 = a5;
  map_volume_box.field_F = a4 & 0xFFFF0000;
  map_volume_box.color = color;
}

void display_drawlist(void)
{
  static const char *func_name="display_drawlist";
#if (BFDEBUG_LEVEL > 9)
    SYNCDBG(0,"Starting");
#endif
  _DK_display_drawlist();
}

void draw_view(struct Camera *cam, unsigned char a2)
{
  static const char *func_name="draw_view";
  long nlens;
  long x,y,z;
  long xcell,ycell;
  long i;
  long aposc,bposc;
  struct EngineCol *ec;
  struct MinMax *mm;
#if (BFDEBUG_LEVEL > 9)
    SYNCDBG(0,"Starting");
#endif
  nlens = cam->field_17 / pixel_size;
  getpoly = poly_pool;
  LbMemorySet(buckets, 0, sizeof(buckets));
  perspective = perspective_routines[lens_mode%PERS_ROUTINES_COUNT];
  rotpers = rotpers_routines[lens_mode%PERS_ROUTINES_COUNT];
  update_fade_limits(cells_away);
  init_coords_and_rotation(&object_origin,&camera_matrix);
  rotate_base_axis(&camera_matrix, cam->orient_a, 2);
  update_normal_shade(&camera_matrix);
  rotate_base_axis(&camera_matrix, -cam->orient_b, 1);
  rotate_base_axis(&camera_matrix, -cam->orient_c, 3);
  map_angle = cam->orient_a;
  map_roll = cam->orient_c;
  map_tilt = -cam->orient_b;
  x = cam->mappos.x.val;
  y = cam->mappos.y.val;
  z = cam->mappos.z.val;
  frame_wibble_generate();
  view_alt = z;
  if (lens_mode != 0)
  {
    cells_away = max_i_can_see;
    update_fade_limits(cells_away);
    fade_range = (fade_max - fade_min) >> 8;
    setup_rotate_stuff(x, y, z, fade_max, fade_min, lens, map_angle, map_roll);
  } else
  {
    fade_min = 1000000;
    setup_rotate_stuff(x, y, z, fade_max, fade_min, nlens, map_angle, map_roll);
    do_perspective_rotation(x, y, z);
    cells_away = compute_cells_away();
  }
  xcell = (x >> 8);
  aposc = -(unsigned char)x;
  bposc = (cells_away << 8) + (y & 0xFF);
  ycell = (y >> 8) - (cells_away+1);
  find_gamut();
  fiddle_gamut(xcell, ycell + (cells_away+1));
  apos = aposc;
  bpos = bposc;
  back_ec = &ecs1[0];
  front_ec = &ecs2[0];
  mm = &minmaxs[31-cells_away];
  if (lens_mode != 0)
  {
    fill_in_points_perspective(xcell, ycell, mm);
  } else
  if (settings.video_cluedo_mode)
  {
    fill_in_points_cluedo(xcell, ycell, mm);
  } else
  {
    fill_in_points_isometric(xcell, ycell, mm);
  }
  for (i = 2*cells_away-1; i > 0; i--)
  {
      ycell++;
      bposc -= 256;
      mm++;
      ec = front_ec;
      front_ec = back_ec;
      back_ec = ec;
      apos = aposc;
      bpos = bposc;
      if (lens_mode != 0)
      {
        fill_in_points_perspective(xcell, ycell, mm);
        if (mm->min < mm->max)
        {
          apos = aposc;
          bpos = bposc;
          do_a_plane_of_engine_columns_perspective(xcell, ycell, mm->min, mm->max);
        }
      } else
      if ( settings.video_cluedo_mode )
      {
        fill_in_points_cluedo(xcell, ycell, mm);
        if (mm->min < mm->max)
        {
          apos = aposc;
          bpos = bposc;
          do_a_plane_of_engine_columns_cluedo(xcell, ycell, mm->min, mm->max);
        }
      } else
      {
        fill_in_points_isometric(xcell, ycell, mm);
        if (mm->min < mm->max)
        {
          apos = aposc;
          bpos = bposc;
          do_a_plane_of_engine_columns_isometric(xcell, ycell, mm->min, mm->max);
        }
      }
  }
  if (map_volume_box.field_0)
    create_map_volume_box(x, y, z);
  display_drawlist();
  map_volume_box.field_0 = 0;
#if (BFDEBUG_LEVEL > 9)
    LbSyncLog("%s: Finished\n",func_name);
#endif
}

void clear_fast_bucket_list(void)
{
  getpoly = poly_pool;
  LbMemorySet(buckets, 0, sizeof(buckets));
}

void display_fast_drawlist(struct Camera *cam)
{
  _DK_display_fast_drawlist(cam);
}

void gtblock_set_clipping_window(unsigned char *addr, long cwidth, long cheight, long scrwidth)
{
  gtblock_screen_addr = addr;
  gtblock_clip_width = cwidth;
  gtblock_clip_height = cheight;
  gtblock_screen_width = scrwidth;
}

long convert_world_coord_to_front_view_screen_coord(struct Coord3d *pos, struct Camera *cam, long *x, long *y, long *z)
{
  return _DK_convert_world_coord_to_front_view_screen_coord(pos, cam, x, y, z);
}

void create_line_element(long a1, long a2, long a3, long a4, long bckt_idx, TbPixel color)
{
  struct BasicQ *poly;
  if (bckt_idx >= 703)
    bckt_idx = 702;
  else
  if (bckt_idx < 0)
    bckt_idx = 0;
  poly = (struct BasicQ *)getpoly;
  getpoly += sizeof(struct BasicQ);
  poly->next = buckets[bckt_idx];
  poly->field_4 = 13;
  buckets[bckt_idx] = poly;
  if (pixel_size > 0)
  {
    poly->field_8 = a1 / pixel_size;
    poly->field_C = a2 / pixel_size;
    poly->field_10 = a3 / pixel_size;
    poly->field_14 = a4 / pixel_size;
  }
  poly->field_18 = color;
}

void create_line_segment(struct EngineCoord *start, struct EngineCoord *end, TbPixel color)
{
  struct BasicQ *poly;
  long bckt_idx;
  if (!is_free_space_in_poly_pool(1))
    return;
  // Get bucket index
  bckt_idx = (start->z+end->z)/2 / 16 - 2;
  if (bckt_idx >= 703)
    bckt_idx = 702;
  else
  if (bckt_idx < 0)
    bckt_idx = 0;
  // Add to bucket
  poly = (struct BasicQ *)getpoly;
  getpoly += sizeof(struct BasicQ);
  poly->next = buckets[bckt_idx];
  poly->field_4 = 13;
  buckets[bckt_idx] = poly;
  // Fill parameters
  if (pixel_size > 0)
  {
    poly->field_8 = start->field_0;
    poly->field_C = start->field_4;
    poly->field_10 = end->field_0;
    poly->field_14 = end->field_4;
  }
  poly->field_18 = color;
}

void add_unkn11_to_polypool(struct Thing *thing, long a2, long a3, long a4, long bckt_idx)
{
  struct BasicQ *poly;
  if (bckt_idx >= 703)
    bckt_idx = 702;
  else
  if (bckt_idx < 0)
    bckt_idx = 0;
  poly = (struct BasicQ *)getpoly;
  getpoly += sizeof(struct BasicQ);
  poly->next = buckets[bckt_idx];
  poly->field_4 = 11;
  buckets[bckt_idx] = poly;
  poly->field_8 = (unsigned long)thing;
  if (pixel_size > 0)
  {
    poly->field_C = a2 / pixel_size;
    poly->field_10 = a3 / pixel_size;
  }
  poly->field_14 = a4;
}

void add_unkn18_to_polypool(struct Thing *thing, long a2, long a3, long a4, long bckt_idx)
{
  struct BasicQ *poly;
  if (bckt_idx >= 703)
    bckt_idx = 702;
  else
  if (bckt_idx < 0)
    bckt_idx = 0;
  poly = (struct BasicQ *)getpoly;
  getpoly += sizeof(struct BasicQ);
  poly->next = buckets[bckt_idx];
  poly->field_4 = 18;
  buckets[bckt_idx] = poly;
  poly->field_8 = (unsigned long)thing;
  if (pixel_size > 0)
  {
    poly->field_C = a2 / pixel_size;
    poly->field_10 = a3 / pixel_size;
  }
  poly->field_14 = a4;
}

void create_status_box_element(struct Thing *thing, long a2, long a3, long a4, long bckt_idx)
{
  struct BasicQ *poly;
  if (bckt_idx >= 703)
    bckt_idx = 702;
  else
  if (bckt_idx < 0)
    bckt_idx = 0;
  poly = (struct BasicQ *)getpoly;
  getpoly += sizeof(struct BasicQ);
  poly->next = buckets[bckt_idx];
  poly->field_4 = 14;
  buckets[bckt_idx] = poly;
  poly->field_8 = (unsigned long)thing;
  if (pixel_size > 0)
  {
    poly->field_C = a2 / pixel_size;
    poly->field_10 = a3 / pixel_size;
  }
  poly->field_14 = a4;
}

void create_fast_view_status_box(struct Thing *thing, long x, long y)
{
  create_status_box_element(thing, x, y - (shield_offset[thing->model]+thing->field_58) / 12, y, 1);
}

void add_unkn16_to_polypool(long x, long y, long lvl, long bckt_idx)
{
  struct BasicUnk1 *poly;
  if (bckt_idx >= 703)
    bckt_idx = 702;
  else
  if (bckt_idx < 0)
    bckt_idx = 0;
  poly = (struct BasicUnk1 *)getpoly;
  getpoly += sizeof(struct BasicUnk1);
  poly->next = buckets[bckt_idx];
  poly->field_4 = 16;
  buckets[bckt_idx] = (struct BasicQ *)poly;
  if (pixel_size > 0)
  {
    poly->x = x / pixel_size;
    poly->y = y / pixel_size;
  }
  poly->lvl = lvl;
}

void add_unkn17_to_polypool(long x, long y, long lvl, long bckt_idx)
{
  struct BasicUnk2 *poly;
  if (bckt_idx >= 703)
    bckt_idx = 702;
  else
  if (bckt_idx < 0)
    bckt_idx = 0;
  poly = (struct BasicUnk2 *)getpoly;
  getpoly += sizeof(struct BasicUnk2);
  poly->next = buckets[bckt_idx];
  poly->field_4 = 17;
  buckets[bckt_idx] = (struct BasicQ *)poly;
  if (pixel_size > 0)
  {
    poly->x = x / pixel_size;
    poly->y = y / pixel_size;
  }
  poly->lvl = lvl;
}

void add_unkn19_to_polypool(long x, long y, long lvl, long bckt_idx)
{
  struct BasicUnk2 *poly;
  if (bckt_idx >= 703)
    bckt_idx = 702;
  else
  if (bckt_idx < 0)
    bckt_idx = 0;
  poly = (struct BasicUnk2 *)getpoly;
  getpoly += sizeof(struct BasicUnk2);
  poly->next = buckets[bckt_idx];
  poly->field_4 = 19;
  buckets[bckt_idx] = (struct BasicQ *)poly;
  if (pixel_size > 0)
  {
    poly->x = x / pixel_size;
    poly->y = y / pixel_size;
  }
  poly->lvl = lvl;
}

void draw_element(struct Map *map, long a2, long a3, long a4, long a5, long a6, long a7, unsigned char a8, long *a9)
{
  _DK_draw_element(map, a2, a3, a4, a5, a6, a7, a8, a9);
}

void update_frontview_pointed_block(unsigned long laaa, unsigned char qdrant, long w, long h, long qx, long qy)
{
  struct TbGraphicsWindow ewnd;
  unsigned long mask;
  struct Map *map;
  long pos_x,pos_y;
  long slb_x,slb_y;
  long point_x,point_y,delta;
  long i;
  store_engine_window(&ewnd,1);
  point_x = (((GetMouseX() - ewnd.x) << 8) - qx) << 8;
  point_y = (((GetMouseY() - ewnd.y) << 8) - qy) << 8;
  delta = (laaa << 7) / 256 << 8;
  for (i=0; i < 8; i++)
  {
    pos_x = (point_x / laaa) * x_step2[qdrant] + (point_y / laaa) * x_step1[qdrant] + (w << 8);
    pos_y = (point_x / laaa) * y_step2[qdrant] + (point_y / laaa) * y_step1[qdrant] + (h << 8);
    slb_x = (pos_x >> 8) + x_offs[qdrant];
    slb_y = (pos_y >> 8) + y_offs[qdrant];
    map = get_map_block(slb_x, slb_y);
    if (!map_block_invalid(map))
    {
      map = get_map_block(slb_x, slb_y);
      if (i == 0)
      {
        floor_pointed_at_x = slb_x;
        floor_pointed_at_y = slb_y;
        block_pointed_at_x = slb_x;
        block_pointed_at_y = slb_y;
        pointed_at_frac_x = pos_x & 0xFF;
        pointed_at_frac_y = pos_y & 0xFF;
        me_pointed_at = map;
      } else
      {
        mask = 0;
        mask = game.columns[map->data & 0x7FF].solidmask;
        if ( (1 << (i-1)) & mask )
        {
          pointed_at_frac_x = pos_x & 0xFF;
          pointed_at_frac_y = pos_y & 0xFF;
          block_pointed_at_x = slb_x;
          block_pointed_at_y = slb_y;
          me_pointed_at = map;
        }
        if ((temp_cluedo_mode) && (i == 2) || (!temp_cluedo_mode) && (i == 5))
        {
          top_pointed_at_frac_x = pos_x & 0xFF;
          top_pointed_at_frac_y = pos_y & 0xFF;
          top_pointed_at_x = slb_x;
          top_pointed_at_y = slb_y;
        }
      }
    }
    point_y += delta;
  }
}

void draw_selected_slab_box(struct Camera *cam, unsigned char stl_width)
{
  struct Coord3d pos;
  long coord_x,coord_y,coord_z;
  unsigned char orient;
  long i;
  long slb_width,depth;
  long vstart,vend;
  long delta[4];
  
  if (!map_volume_box.field_0)
    return;
  pos.y.val = map_volume_box.field_7;
  pos.x.val = map_volume_box.field_3;
  pos.z.val = 1280;
  orient = ((unsigned int)(cam->orient_a + 256) >> 9) & 0x03;
  convert_world_coord_to_front_view_screen_coord(&pos, cam, &coord_x, &coord_y, &coord_z);
  depth = (5 - map_volume_box.field_13) * (stl_width << 7) / 256;
  slb_width = 3 * stl_width;
  switch ( orient )
  {
  case 1:
      coord_y -= slb_width;
      coord_z += slb_width;
      break;
  case 2:
      coord_x -= slb_width;
      coord_y -= slb_width;
      coord_z += slb_width;
      break;
  case 3:
      coord_x -= slb_width;
      break;
  }
  vstart = 0;
  coord_z -= (stl_width >> 1);
  vend = stl_width;
  delta[0] = 0;
  delta[1] = slb_width;
  delta[2] = depth;
  delta[3] = slb_width + depth;
  // Draw a horizonal line element for every subtile
  for (i=3; i > 0; i--)
  {
    if (!is_free_space_in_poly_pool(4))
      break;
    create_line_element(coord_x + vstart,    coord_y + delta[0],  coord_x + vend,      coord_y + delta[0], coord_z,             map_volume_box.color);
    create_line_element(coord_x + vstart,    coord_y + delta[1],  coord_x + vend,      coord_y + delta[1], coord_z - slb_width, map_volume_box.color);
    create_line_element(coord_x + vstart,    coord_y + delta[2],  coord_x + vend,      coord_y + delta[2], coord_z,             map_volume_box.color);
    create_line_element(coord_x + vstart,    coord_y + delta[3],  coord_x + vend,      coord_y + delta[3], coord_z - slb_width, map_volume_box.color);
    vend += stl_width;
    vstart += stl_width;
  }
  // Now the rectangles at left and right
  for (i=3; i > 0; i--)
  {
    if (!is_free_space_in_poly_pool(4))
      break;
    create_line_element(coord_x,             coord_y + delta[0],  coord_x,             coord_y + delta[1], coord_z - delta[0], map_volume_box.color);
    create_line_element(coord_x + slb_width, coord_y + delta[0],  coord_x + slb_width, coord_y + delta[1], coord_z - delta[0], map_volume_box.color);
    create_line_element(coord_x,             coord_y + delta[2],  coord_x,             coord_y + delta[3], coord_z - delta[0], map_volume_box.color);
    create_line_element(coord_x + slb_width, coord_y + delta[2],  coord_x + slb_width, coord_y + delta[3], coord_z - delta[0], map_volume_box.color);
    delta[0] += stl_width;
    delta[2] += stl_width;
    delta[3] += stl_width;
    delta[1] += stl_width;
  }
}

void draw_frontview_thing_on_element(struct Thing *thing, struct Map *map, struct Camera *cam)
{
  long cx,cy,cz;
  if ((thing->field_4F & 0x01) != 0)
    return;
  switch ( (thing->field_50 >> 2) )
  {
  case 2:
          convert_world_coord_to_front_view_screen_coord(&thing->mappos,cam,&cx,&cy,&cz);
          if (is_free_space_in_poly_pool(1))
          {
            add_unkn11_to_polypool(thing, cx, cy, cy, cz-3);
            if ((thing->class_id == TCls_Creature) && is_free_space_in_poly_pool(1))
            {
              create_fast_view_status_box(thing, cx, cy);
            }
          }
          break;
  case 4:
          convert_world_coord_to_front_view_screen_coord(&thing->mappos,cam,&cx,&cy,&cz);
          if (is_free_space_in_poly_pool(1))
          {
            add_unkn16_to_polypool(cx, cy, thing->long_13, 1);
          }
          break;
  case 5:
          convert_world_coord_to_front_view_screen_coord(&thing->mappos,cam,&cx,&cy,&cz);
          if (is_free_space_in_poly_pool(1))
          {
            if (game.play_gameturn - *(unsigned long *)(&thing->word_13.w1) != 1)
            {
              thing->field_19 = 0;
            } else
            if (thing->field_19 < 40)
            {
              thing->field_19++;
            }
            *(unsigned long *)(&thing->word_13.w1) = game.play_gameturn;
            if (thing->field_19 == 40)
            {
              add_unkn17_to_polypool(cx, cy, thing->long_13, cz-3);
              if (is_free_space_in_poly_pool(1))
              {
                add_unkn19_to_polypool(cx, cy, thing->long_13, 1);
              }
            }
          }
          break;
  case 6:
          convert_world_coord_to_front_view_screen_coord(&thing->mappos,cam,&cx,&cy,&cz);
          if (is_free_space_in_poly_pool(1))
          {
            add_unkn18_to_polypool(thing, cx, cy, cy, cz-3);
          }
          break;
  default:
          break;
  }
}

void draw_frontview_things_on_element(struct Map *map, struct Camera *cam)
{
  struct Thing *thing;
  long i;
  unsigned long k;
  k = 0;
  i = thing_index_on_map_block(map);
  while (i != 0)
  {
    thing = thing_get(i);
    if (thing_is_invalid(thing))
    {
      ERRORLOG("Jump to invalid thing detected");
      break;
    }
    i = thing->field_2;
    draw_frontview_thing_on_element(thing, map, cam);
    k++;
    if (k > THINGS_COUNT)
    {
      ERRORLOG("Infinite loop detected when sweeping things list");
      break;
    }
  }
}

void draw_frontview_engine(struct Camera *cam)
{
  static const char *func_name="draw_frontview_engine";
  struct PlayerInfo *player;
  struct TbGraphicsWindow grwnd;
  struct TbGraphicsWindow ewnd;
  unsigned char qdrant;
  struct Map *map;
  long px,py,qx,qy;
  long w,h;
  long pos_x,pos_y;
  long stl_x,stl_y;
  long lim_x,lim_y;
  unsigned long laaa;
  long i;
  SYNCDBG(10,"Starting");
  player = &(game.players[my_player_number%PLAYERS_COUNT]);
  UseFastBlockDraw = (cam->field_17 == 65536);
  if (cam->field_17 > 65536)
    cam->field_17 = 65536;
  LbScreenStoreGraphicsWindow(&grwnd);
  store_engine_window(&ewnd,pixel_size);
  LbScreenSetGraphicsWindow(ewnd.x, ewnd.y, ewnd.width, ewnd.height);
  gtblock_set_clipping_window(lbDisplay.GraphicsWindowPtr, ewnd.width, ewnd.height, lbDisplay.GraphicsScreenWidth);
  setup_vecs(lbDisplay.GraphicsWindowPtr, NULL, lbDisplay.GraphicsScreenWidth, ewnd.width, ewnd.height);
  engine_player_number = player->field_2B;
  player_bit = (1 << player->field_2B);
  clear_fast_bucket_list();
  store_engine_window(&ewnd,1);
  setup_engine_window(ewnd.x, ewnd.y, ewnd.width, ewnd.height);
  qdrant = ((unsigned int)(cam->orient_a + 256) >> 9) & 0x03;
  laaa = 32 * cam->field_17 / 256;
  w = (ewnd.width << 16) / laaa >> 1;
  h = (ewnd.height << 16) / laaa >> 1;
  switch (qdrant)
  {
  case 0:
      px = (cam->mappos.x.val - w) >> 8;
      py = (cam->mappos.y.val - h) >> 8;
      qx = (ewnd.width << 7) - (laaa * (cam->mappos.x.val - (px << 8)) >> 8);
      qy = (ewnd.height << 7) - (laaa * (cam->mappos.y.val - (py << 8)) >> 8);
      break;
  case 1:
      px = (cam->mappos.x.val + h) >> 8;
      py = (cam->mappos.y.val - w) >> 8;
      qx = (ewnd.width << 7) - (laaa * (cam->mappos.y.val - (py << 8)) >> 8);
      qy = (ewnd.height << 7) - (laaa * ((px << 8) - cam->mappos.x.val) >> 8);
      px--;
      break;
  case 2:
      px = ((cam->mappos.x.val + w) >> 8) + 1;
      py = (cam->mappos.y.val + h) >> 8;
      qx = (ewnd.width << 7) - (laaa * ((px << 8) - cam->mappos.x.val) >> 8);
      qy = (ewnd.height << 7) - (laaa * ((py << 8) - cam->mappos.y.val) >> 8);
      px--;
      py--;
      break;
  case 3:
      px = (cam->mappos.x.val - h) >> 8;
      py = ((cam->mappos.y.val + w) >> 8) + 1;
      qx = (ewnd.width << 7) - (laaa * ((py << 8) - cam->mappos.y.val) >> 8);
      qy = (ewnd.height << 7) - (laaa * (cam->mappos.x.val - (px << 8)) >> 8);
      py--;
      break;
  default:
      ERRORLOG("Illegal quadrant, %d.",qdrant);
      LbScreenLoadGraphicsWindow(&grwnd);
      return;
  }

  update_frontview_pointed_block(laaa, qdrant, px, py, qx, qy);
  draw_selected_slab_box(cam, (laaa >> 8) & 0xFF);
  map_volume_box.field_0 = 0;

  h = (8 * (laaa + 32 * ewnd.height) - qy) / laaa;
  w = (8 * (laaa + 32 * ewnd.height) - qy) / laaa;
  qy += laaa * h;
  px += x_step1[qdrant] * w;
  stl_x = x_step1[qdrant] * w + px;
  stl_y = y_step1[qdrant] * h + py;
  py += y_step1[qdrant] * h;
  lim_x = ewnd.width << 8;
  lim_y = -laaa;
  pos_x = qx;
  while (pos_x < lim_x)
  {
    i = (ewnd.height << 8);
    if (x_step1[qdrant] != 0)
      stl_x = px;
    else
      stl_y = py;
    pos_y = qy;
    while (pos_y > lim_y)
    {
          map = get_map_block(stl_x, stl_y);
          if (!map_block_invalid(map))
          {
            if (map->data & 0x7FF)
            {
              draw_element(map, game.field_46157[get_subtile_number(stl_x,stl_y)], stl_x, stl_y, pos_x, pos_y, laaa, qdrant, &i);
              if ( subtile_revealed(stl_x, stl_y, player->field_2B) )
              {
                draw_frontview_things_on_element(map, cam);
              }
            }
          }
          stl_x -= x_step1[qdrant];
          stl_y -= y_step1[qdrant];
          pos_y -= laaa;
    }
    stl_x += x_step2[qdrant];
    stl_y += y_step2[qdrant];
    pos_x += laaa;
  }

  display_fast_drawlist(cam);
  LbScreenLoadGraphicsWindow(&grwnd);
#if (BFDEBUG_LEVEL > 9)
    LbSyncLog("%s: Finished\n",func_name);
#endif
}
/******************************************************************************/
#ifdef __cplusplus
}
#endif