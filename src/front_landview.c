/******************************************************************************/
// Free implementation of Bullfrog's Dungeon Keeper strategy game.
/******************************************************************************/
/** @file front_landview.c
 *     Land view, where the user can select map for campaign or multiplayer.
 * @par Purpose:
 *     Functions for displaying and maintaininfg the land view.
 * @par Comment:
 *     None.
 * @author   Tomasz Lis
 * @date     16 Mar 2009 - 01 Apr 2009
 * @par  Copying and copyrights:
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 2 of the License, or
 *     (at your option) any later version.
 */
/******************************************************************************/
#include "front_landview.h"

#include "globals.h"
#include "bflib_basics.h"
#include "bflib_memory.h"
#include "bflib_datetm.h"
#include "bflib_video.h"
#include "bflib_fileio.h"
#include "bflib_dernc.h"
#include "bflib_filelst.h"
#include "bflib_sprite.h"
#include "bflib_sprfnt.h"
#include "bflib_mouse.h"
#include "bflib_sndlib.h"
#include "bflib_sound.h"
#include "bflib_vidraw.h"
#include "bflib_network.h"

#include "config.h"
#include "config_campaigns.h"
#include "lvl_filesdk1.h"
#include "front_simple.h"
#include "frontend.h"
#include "kjm_input.h"
#include "vidmode.h"

#include "keeperfx.h"

#ifdef __cplusplus
extern "C" {
#endif
/******************************************************************************/
struct TbLoadFiles map_flag_load_files[] = {
  {"ldata/dkflag00.dat", (unsigned char **)&map_flag_data,(unsigned char **)&end_map_flag_data, 0, 0, 0},
  {"ldata/dkflag00.tab", (unsigned char **)&map_flag,     (unsigned char **)&end_map_flag,      0, 0, 0},
  {"",                   NULL,                            NULL,                                 0, 0, 0},
};
/*
struct TbSetupSprite map_flag_setup_sprites[] = {
  {&map_flag, &end_map_flag, &map_flag_data},
  {NULL,      NULL,          NULL,},
};

struct TbSetupSprite netmap_flag_setup_sprites[] = {
  {&map_flag, &end_map_flag, &map_flag_data},
  {&map_font, &end_map_font, &map_font_data},
  {&map_hand, &end_map_hand, &map_hand_data},
  {NULL,      NULL,          NULL,},
};
*/

TbPixel net_player_colours[] = { 251, 58, 182, 11};
const long hand_limp_xoffset[] = { 32,  31,  30,  29,  28,  27,  26,  24,  22,  19,  15,  9, };
const long hand_limp_yoffset[] = {-11, -10,  -9,  -8,  -7,  -6,  -5,  -4,  -3,  -2,  -1,  0, };
struct TbSprite dummy_sprite = {0, 0, 0};

long limp_hand_x = 0;
long limp_hand_y = 0;
LevelNumber mouse_over_lvnum;
LevelNumber playing_speech_lvnum;
/******************************************************************************/
DLLIMPORT long _DK_frontnetmap_update(void);
DLLIMPORT void _DK_frontnetmap_draw(void);
DLLIMPORT void _DK_frontnetmap_input(void);
DLLIMPORT void _DK_frontnetmap_unload(void);
DLLIMPORT void _DK_frontnetmap_load(void);
DLLIMPORT long _DK_frontmap_update(void);
DLLIMPORT void _DK_frontmap_draw(void);
DLLIMPORT void _DK_frontmap_input(void);
DLLIMPORT void _DK_frontmap_unload(void);
DLLIMPORT void _DK_frontnet_init_level_descriptions(void);
DLLIMPORT int _DK_frontmap_load(void);
DLLIMPORT long _DK_load_map_and_window(unsigned long lv_num);
DLLIMPORT void _DK_frontzoom_to_point(long a1, long a2, long a3);
DLLIMPORT void _DK_compressed_window_draw(void);
DLLIMPORT void _DK_check_mouse_scroll(void);
DLLIMPORT void _DK_update_velocity(void);
/******************************************************************************/
void draw_map_screen(void)
{
  unsigned char *dstbuf;
  unsigned char *srcbuf;
  long i;
  long w;
  SYNCDBG(18,"Starting");
  dstbuf = lbDisplay.WScreen;
  w = lbDisplay.PhysicalScreenWidth;
  if (map_info.scrshift_x+w > MAP_SCREEN_WIDTH)
    w = MAP_SCREEN_WIDTH-map_info.scrshift_x;
  srcbuf = &map_screen[MAP_SCREEN_WIDTH*map_info.scrshift_y + map_info.scrshift_x];
  for (i=0; i < lbDisplay.PhysicalScreenHeight; i++)
  {
    if (map_info.scrshift_y+i >= MAP_SCREEN_HEIGHT)
      break;
    memcpy(dstbuf, srcbuf, w);
    srcbuf += MAP_SCREEN_WIDTH;
    dstbuf += lbDisplay.GraphicsScreenWidth;
  }
}

struct TbSprite *get_map_flag(long idx)
{
  struct TbSprite *spr;
  spr = &map_flag[idx];
  if (spr < end_map_flag)
    return spr;
  return &dummy_sprite;
}

short is_over_ensign(struct LevelInformation *lvinfo, long scr_x, long scr_y)
{
  struct TbSprite *spr;
  long map_x,map_y,spr_w,spr_h;
  map_x = map_info.scrshift_x+scr_x;
  map_y = map_info.scrshift_y+scr_y;
  spr = get_map_flag(10);
  spr_w = spr->SWidth;
  spr_h = spr->SHeight;
  if ((map_x >= lvinfo->ensign_x-(spr_w>>1)) && (map_x < lvinfo->ensign_x+(spr_w>>1))
   && (map_y > lvinfo->ensign_y-spr_h) && (map_y < lvinfo->ensign_y-(spr_h/3)))
      return true;
  return false;
}

short is_ensign_in_screen_rect(struct LevelInformation *lvinfo)
{
  if ((lvinfo->ensign_zoom_x >= map_info.scrshift_x) && (lvinfo->ensign_zoom_x < map_info.scrshift_x+lbDisplay.PhysicalScreenWidth))
    if ((lvinfo->ensign_zoom_y >= map_info.scrshift_y) && (lvinfo->ensign_zoom_y < map_info.scrshift_y+lbDisplay.PhysicalScreenHeight))
      return true;
  return false;
}

/*
 * Chanes state of all land map ensigns.
 */
void set_all_ensigns_state(unsigned short nstate)
{
  struct LevelInformation *lvinfo;
  lvinfo = get_first_level_info();
  while (lvinfo != NULL)
  {
    lvinfo->state = nstate;
    lvinfo = get_next_level_info(lvinfo);
  }
}

void update_ensigns_visibility(void)
{
  struct LevelInformation *lvinfo;
  struct PlayerInfo *player;
  long lvnum,bn_lvnum;
  short show_all_sp;
  int i,k;
  SYNCDBG(18,"Starting");
  set_all_ensigns_state(LvSt_Hidden);
  player = &(game.players[my_player_number%PLAYERS_COUNT]);
  show_all_sp = false;
  lvnum = get_continue_level_number();
  if (lvnum > 0)
  {
    lvinfo = get_level_info(lvnum);
    if (lvinfo != NULL)
      lvinfo->state = LvSt_Visible;
  } else
  if (lvnum == SINGLEPLAYER_FINISHED)
  {
    show_all_sp = true;
  }
  lvnum = first_singleplayer_level();
  while (lvnum > 0)
  {
    if (show_all_sp)
    {
      lvinfo = get_level_info(lvnum);
      if (lvinfo != NULL)
        lvinfo->state = LvSt_Visible;
    }
    bn_lvnum = bonus_level_for_singleplayer_level(lvnum);
    if (is_bonus_level_visible(player, bn_lvnum))
    {
      lvinfo = get_level_info(bn_lvnum);
      if (lvinfo != NULL)
        lvinfo->state = LvSt_Visible;
    }
    lvnum = next_singleplayer_level(lvnum);
  }
  // Extra level - full moon
  lvnum = get_extra_level(ExLv_FullMoon);
  lvinfo = get_level_info(lvnum);
  if (lvinfo != NULL)
    lvinfo->state = get_extra_level_kind_visibility(ExLv_FullMoon);
  // Extra level - new moon
  lvnum = get_extra_level(ExLv_NewMoon);
  lvinfo = get_level_info(lvnum);
  if (lvinfo != NULL)
    lvinfo->state = get_extra_level_kind_visibility(ExLv_NewMoon);
}

void update_net_ensigns_visibility(void)
{
  struct LevelInformation *lvinfo;
  struct PlayerInfo *player;
  long lvnum,bn_lvnum;
  short show_all_sp;
  int i,k;
  SYNCDBG(18,"Starting");
  set_all_ensigns_state(LvSt_Hidden);
  player = &(game.players[my_player_number%PLAYERS_COUNT]);
  lvnum = first_multiplayer_level();
  while (lvnum > 0)
  {
    lvinfo = get_level_info(lvnum);
    if (lvinfo != NULL)
      lvinfo->state = LvSt_Visible;
    lvnum = next_multiplayer_level(lvnum);
  }
}

int compute_sound_good_to_bad_factor(void)
{
  struct LevelInformation *lvinfo;
  unsigned int onscr_bad,onscr_good;
  LevelNumber sp_lvnum,continue_lvnum;
  short lv_beaten;
  SYNCDBG(18,"Starting");
  onscr_bad = 0;
  onscr_good = 0;
  continue_lvnum = get_continue_level_number();
  lv_beaten = (continue_lvnum != SINGLEPLAYER_NOTSTARTED);
  sp_lvnum = first_singleplayer_level();
  while (sp_lvnum > 0)
  {
    if (sp_lvnum == continue_lvnum)
      lv_beaten = false;
    lvinfo = get_level_info(sp_lvnum);
    if (is_ensign_in_screen_rect(lvinfo))
    {
      if (lv_beaten)
        onscr_bad++;
      else
        onscr_good++;
    }
    sp_lvnum = next_singleplayer_level(sp_lvnum);
  }
  if ((onscr_bad+onscr_good) == 0)
    onscr_good++;
  return (127*onscr_good)/(onscr_bad+onscr_good);
}

void update_frontmap_ambient_sound(void)
{
  long lvidx;
  long i;
  if (map_sound_fade)
  {
    lvidx = array_index_for_singleplayer_level(get_continue_level_number());
    if ((features_enabled & Ft_AdvAmbSonud) != 0)
    {
      i = compute_sound_good_to_bad_factor();
      SYNCDBG(18,"Volume factor is %ld",i);
      SetSampleVolume(0, campaign.ambient_good, map_sound_fade*(i)/256, 0);
      SetSampleVolume(0, campaign.ambient_bad, map_sound_fade*(127-i)/256, 0);
    } else
    if (lvidx > 13)
    {
      SetSampleVolume(0, campaign.ambient_bad, 127*map_sound_fade/256, 0);
    } else
    {
      SetSampleVolume(0, campaign.ambient_good, 127*map_sound_fade/256, 0);
    }
    SetStreamedSampleVolume(127*map_sound_fade/256);
    if ((game.flags_cd & 0x10) == 0)
      SetRedbookVolume(map_sound_fade*settings.redbook_volume/256);
  } else
  {
    if ((features_enabled & Ft_AdvAmbSonud) != 0)
    {
      SetSampleVolume(0, campaign.ambient_good, 0, 0);
      SetSampleVolume(0, campaign.ambient_bad, 0, 0);
    }
    if ((game.flags_cd & 0x10) == 0)
      SetRedbookVolume(0);
    SetStreamedSampleVolume(0);
  }
}

struct TbSprite *get_ensign_sprite_for_level(struct LevelInformation *lvinfo, int anim_frame)
{
  struct TbSprite *spr;
  int i;
  if (lvinfo == NULL)
    return NULL;
  if (lvinfo->state == LvSt_Hidden)
    return NULL;
  if (lvinfo->options & LvOp_IsSingle)
  {
    switch (lvinfo->state)
    {
    case LvSt_Visible:
        if ((lvinfo->options & LvOp_Tutorial) == 0)
          i = 10; // full red flag
        else
          i = 2; // 'T' flag - tutorial
        if (lvinfo->lvnum == mouse_over_lvnum)
          i += 4;
        spr = get_map_flag(i+(anim_frame & 3));
        break;
    default:
        spr = get_map_flag(10);
        break;
    }
  } else
  if (lvinfo->options & LvOp_IsBonus)
  {
    switch (lvinfo->state)
    {
    case LvSt_Visible:
        i = 18;
        if (lvinfo->lvnum == mouse_over_lvnum)
          i += 4;
        spr = get_map_flag(i+(anim_frame & 3));
        break;
    default:
        spr = get_map_flag(18);
        break;
    }
  } else
  if (lvinfo->options & LvOp_IsExtra)
  {
    switch (lvinfo->state)
    {
    case LvSt_Visible:
        i = 26;
        if (lvinfo->lvnum == mouse_over_lvnum)
          i += 4;
        spr = get_map_flag(i+(anim_frame & 3));
        break;
    default:
        spr = get_map_flag(34);
        break;
    }
  } else
  if (lvinfo->options & LvOp_IsMulti) //Note that multiplayer flags have different file
  {
    switch (lvinfo->players)
    {
    case 2:
        i = 5;
        break;
    case 3:
        i = 7;
        break;
    case 4:
        i = 9;
        break;
    default:
        i = 5;
        break;
    }
    if ((fe_net_level_selected == lvinfo->lvnum) || (net_level_hilighted == lvinfo->lvnum))
      i++;
    spr = get_map_flag(i);
  } else
  {
    spr = get_map_flag(34);
  }
  if (spr == &dummy_sprite)
    ERRORLOG("Can't get Land view Ensign sprite");
  return spr;
}

/*
 * Draws the visible level ensigns on screen.
 * Note that the drawing is in reverse order than the one of reading inputs.
 */
void draw_map_level_ensigns(void)
{
  struct LevelInformation *lvinfo;
  struct TbSprite *spr;
  long x,y;
  int k;
  SYNCDBG(18,"Starting");
  k = LbTimerClock()/200;
  lvinfo = get_last_level_info();
  while (lvinfo != NULL)
  {
    // the flag sprite
    spr = get_ensign_sprite_for_level(lvinfo,k);
    if (spr != NULL)
    {
      x = lvinfo->ensign_x - map_info.scrshift_x - (spr->SWidth>>1);
      y = lvinfo->ensign_y - map_info.scrshift_y -  spr->SHeight;
      LbSpriteDraw(x, y, spr);
    }
    lvinfo = get_prev_level_info(lvinfo);
  }
}

void set_map_info_visible_hotspot(long map_x,long map_y)
{
  map_info.hotspot_x = map_x - lbDisplay.PhysicalScreenWidth/2;
  map_info.hotspot_y = map_y - lbDisplay.PhysicalScreenHeight/2;
  if (map_info.hotspot_x > MAP_SCREEN_WIDTH-lbDisplay.PhysicalScreenWidth)
    map_info.hotspot_x = MAP_SCREEN_WIDTH-lbDisplay.PhysicalScreenWidth;
  if (map_info.hotspot_x < 0)
    map_info.hotspot_x = 0;
  if (map_info.hotspot_y > MAP_SCREEN_HEIGHT-lbDisplay.PhysicalScreenHeight)
    map_info.hotspot_y = MAP_SCREEN_HEIGHT-lbDisplay.PhysicalScreenHeight;
  if (map_info.hotspot_y < 0)
    map_info.hotspot_y = 0;
}

void set_map_info_draw_hotspot(long map_x,long map_y)
{
  map_info.scrshift_x = map_x - lbDisplay.PhysicalScreenWidth/2;
  map_info.scrshift_y = map_y - lbDisplay.PhysicalScreenHeight/2;
  if (map_info.scrshift_x > MAP_SCREEN_WIDTH-lbDisplay.PhysicalScreenWidth)
    map_info.scrshift_x = MAP_SCREEN_WIDTH-lbDisplay.PhysicalScreenWidth;
  if (map_info.scrshift_x < 0)
    map_info.scrshift_x = 0;
  if (map_info.scrshift_y > MAP_SCREEN_HEIGHT-lbDisplay.PhysicalScreenHeight)
    map_info.scrshift_y = MAP_SCREEN_HEIGHT-lbDisplay.PhysicalScreenHeight;
  if (map_info.scrshift_y < 0)
    map_info.scrshift_y = 0;
}

void set_map_info_draw_hotspot_raw(long map_x,long map_y)
{
  map_info.scrshift_x = map_x;
  map_info.scrshift_y = map_y;
  if (map_info.scrshift_x > MAP_SCREEN_WIDTH-lbDisplay.PhysicalScreenWidth)
    map_info.scrshift_x = MAP_SCREEN_WIDTH-lbDisplay.PhysicalScreenWidth;
  if (map_info.scrshift_x < 0)
    map_info.scrshift_x = 0;
  if (map_info.scrshift_y > MAP_SCREEN_HEIGHT-lbDisplay.PhysicalScreenHeight)
    map_info.scrshift_y = MAP_SCREEN_HEIGHT-lbDisplay.PhysicalScreenHeight;
  if (map_info.scrshift_y < 0)
    map_info.scrshift_y = 0;
}

void update_frontmap_info_draw_hotspot(void)
{
  long scr_x,scr_y;
  long step;
  scr_x = map_info.hotspot_x - map_info.scrshift_x;
  scr_y = map_info.hotspot_y - map_info.scrshift_y;
  if ((scr_x != 0) || (scr_y != 0))
  {
      step = abs(scr_y);
      if (step <= abs(scr_x))
        step = abs(scr_x);
      if (step > 59)
        step = 59;
      if (step < 0)
        step = 0;
      if (step <= 1)
      {
        set_map_info_draw_hotspot_raw(map_info.hotspot_x,map_info.hotspot_y);
      } else
      {
        map_info.field_1E += (scr_x << 8) / step;
        map_info.field_22 += (scr_y << 8) / step;
        set_map_info_draw_hotspot_raw(map_info.field_1E >> 8, map_info.field_22 >> 8);
      }
  }
}

TbBool frontmap_input_active_ensign(long curr_mx, long curr_my)
{
  struct LevelInformation *lvinfo;
  lvinfo = get_first_level_info();
  while (lvinfo != NULL)
  {
    if (lvinfo->state == LvSt_Visible)
      if (is_over_ensign(lvinfo, curr_mx, curr_my))
      {
        mouse_over_lvnum = lvinfo->lvnum;
        return true;
      }
    lvinfo = get_next_level_info(lvinfo);
  }
  mouse_over_lvnum = SINGLEPLAYER_NOTSTARTED;
  return false;
}

short clicked_map_level_ensign(void)
{
  struct LevelInformation *lvinfo;
  lvinfo = get_level_info(mouse_over_lvnum);
  if (lvinfo != NULL)
  {
    set_selected_level_number(lvinfo->lvnum);
    map_info.field_0 = 1;
    map_info.field_1 = 1;
    map_info.fade_pos = 1;
    map_info.field_A = lvinfo->ensign_zoom_x;
    map_info.field_E = lvinfo->ensign_zoom_y;
    map_info.fade_step = 4;
    map_info.field_12 = 7;
    set_map_info_visible_hotspot(map_info.field_A, map_info.field_E);
    map_info.field_1E = map_info.scrshift_x << 8;
    map_info.field_22 = map_info.scrshift_y << 8;
    return true;
  }
  return false;
}

short stop_description_speech(void)
{
  if ((playing_good_descriptive_speech) || (playing_bad_descriptive_speech))
  {
    playing_good_descriptive_speech = 0;
    playing_bad_descriptive_speech = 0;
    playing_speech_lvnum = SINGLEPLAYER_NOTSTARTED;
    StopStreamedSample();
    return true;
  }
  return false;
}

short play_current_description_speech(short play_good)
{
  LevelNumber lvnum;
  lvnum = get_continue_level_number();
  if (!play_good)
    lvnum = prev_singleplayer_level(lvnum);
  return play_description_speech(lvnum,play_good);
}

short play_description_speech(LevelNumber lvnum, short play_good)
{
  struct LevelInformation *lvinfo;
  char *fname;
  if (playing_speech_lvnum == lvnum)
    return true;
  lvinfo = get_level_info(lvnum);
  if (lvinfo == NULL)
    return false;
  if (play_good)
  {
    if (lvinfo->speech_before[0] == '\0')
      return false;
    stop_description_speech();
    fname = prepare_file_fmtpath(FGrp_AtlSound,"%s.wav",lvinfo->speech_before);
    playing_good_descriptive_speech = 1;
  } else
  {
    if (lvinfo->speech_after[0] == '\0')
      return false;
    stop_description_speech();
    fname = prepare_file_fmtpath(FGrp_AtlSound,"%s.wav",lvinfo->speech_after);
    playing_bad_descriptive_speech = 1;
  }
  playing_speech_lvnum = lvnum;
  SetStreamedSampleVolume(127);
  PlayStreamedSample(fname, 1622, 0, 1);
  return true;
}

TbBool set_pointer_graphic_spland(long frame)
{
  long i,x,y;
  struct TbSprite *spr;
  spr = get_map_flag(1);
  if (spr == &dummy_sprite)
    ERRORLOG("Can't get Land view Mouse sprite");
  LbMouseChangeSprite(spr);
  return (spr != &dummy_sprite);
}

void frontzoom_to_point(long map_x, long map_y, long zoom)
{
  unsigned char *dst_buf;
  unsigned char *src_buf;
  unsigned char *dst;
  unsigned char *src;
  long scr_x,scr_y;
  long src_delta,bpos_x,bpos_y;
  long dst_width,dst_height,dst_scanln;
  long x,y;
  src_delta = 256 - zoom;
  // Restricting coordinates - to make sure we won't go outside of the buffer
  if (map_x >= (MAP_SCREEN_WIDTH-(lbDisplay.GraphicsScreenWidth>>1)))
    map_x = (MAP_SCREEN_WIDTH-(lbDisplay.GraphicsScreenWidth>>1)-1);
  if (map_x < (lbDisplay.GraphicsScreenWidth>>1))
    map_x = (lbDisplay.GraphicsScreenWidth>>1);
  if (map_y >= (MAP_SCREEN_HEIGHT-(lbDisplay.GraphicsScreenHeight>>1)))
    map_y = (MAP_SCREEN_HEIGHT-(lbDisplay.GraphicsScreenHeight>>1)-1);
  if (map_y < (lbDisplay.GraphicsScreenHeight>>1))
    map_y = (lbDisplay.GraphicsScreenHeight>>1);
  // Initializing variables used for all quadres of screen
  scr_x = map_x - map_info.scrshift_x;
  if (scr_x > lbDisplay.GraphicsScreenWidth) scr_x = lbDisplay.GraphicsScreenWidth;
  if (scr_x < 0) scr_x = 0;
  scr_y = map_y - map_info.scrshift_y;
  if (scr_y > lbDisplay.GraphicsScreenHeight) scr_y = lbDisplay.GraphicsScreenHeight;
  if (scr_y < 0) scr_y = 0;
  src_buf = &map_screen[MAP_SCREEN_WIDTH * map_y + map_x];
  dst_buf = &lbDisplay.WScreen[lbDisplay.GraphicsScreenWidth * scr_y + scr_x];
  dst_scanln = lbDisplay.GraphicsScreenWidth;
  // Drawing first quadre
  bpos_y = 0;
  dst = dst_buf;
  dst_width = scr_x;
  dst_height = scr_y;
  for (y=0; y <= dst_height; y++)
  {
      bpos_x = 0;
      src = &src_buf[-5*(bpos_y & 0xFFFFFF00)];
      for (x=0; x <= dst_width; x++)
      {
        bpos_x += src_delta;
        dst[-x] = src[-(bpos_x >> 8)];
      }
      dst -= dst_scanln;
      bpos_y += src_delta;
  }
  // Drawing 2nd quadre
  bpos_y = 0;
  dst = dst_buf;
  dst_width = -scr_x + lbDisplay.PhysicalScreenWidth;
  dst_height = scr_y;
  for (y=0; y <= dst_height; y++)
  {
      bpos_x = 0;
      src = &src_buf[-5*(bpos_y & 0xFFFFFF00)];
      for (x=0; x < dst_width; x++)
      {
        bpos_x += src_delta;
        dst[x] = src[(bpos_x >> 8)];
      }
      dst -= dst_scanln;
      bpos_y += src_delta;
  }
  // Drawing 3rd quadre
  bpos_y = 0;
  dst = dst_buf + dst_scanln;
  dst_width = scr_x;
  dst_height = -scr_y + lbDisplay.PhysicalScreenHeight - 1;
  for (y=0; y < dst_height; y++)
  {
      bpos_x = 0;
      src = &src_buf[5*(bpos_y & 0xFFFFFF00)];
      for (x=0; x <= dst_width; x++)
      {
        bpos_x += src_delta;
        dst[-x] = src[-(bpos_x >> 8)];
      }
      dst += dst_scanln;
      bpos_y += src_delta;
  }
  // Drawing 4th quadre
  bpos_y = 0;
  dst = dst_buf + dst_scanln;
  dst_width = -scr_x + lbDisplay.PhysicalScreenWidth;
  dst_height = -scr_y + lbDisplay.PhysicalScreenHeight - 1;
  for (y=0; y < dst_height; y++)
  {
      bpos_x = 0;
      src = &src_buf[5*(bpos_y & 0xFFFFFF00)];
      for (x=0; x < dst_width; x++)
      {
        dst[x] = src[(bpos_x >> 8)];
        bpos_x += src_delta;
      }
      dst += dst_scanln;
      bpos_y += src_delta;
  }
}

void compressed_window_draw(void)
{
  unsigned char *src;
  unsigned char *dst;
  long xshift,yshift;
  int wcopy;
  int wdata;
  int wskip;
  long w,h;

  xshift = map_info.scrshift_x / 2;
  yshift = map_info.scrshift_y / 2;
  for (h=0; h < lbDisplay.GraphicsScreenHeight; h++)
  {
    dst = lbDisplay.WScreen + lbDisplay.GraphicsScreenWidth * h;
    src = &map_window[window_y_offset[yshift + h]];
    w = 0;
    while (w < xshift)
    {
      wdata = *(unsigned long *)src;
      src += 4;
      wcopy = wdata + w - xshift;
      if (wcopy >= 0)
      {
        // Draw non-transparent area
        if (wcopy > lbDisplay.GraphicsScreenWidth)
          wcopy = lbDisplay.GraphicsScreenWidth;
        if (wcopy < 0)
          wcopy = 0;
        if (wcopy != 0)
        {
          memcpy(dst, src - w + xshift, wcopy);
          dst += wcopy;
        }
        src += wdata;
        w += wdata;
        // Skip the transparent area
        wskip = *(unsigned long *)src;
        src += 4;
        dst += wskip;
        w += wskip;
      } else
      {
        // Skip non-transparent area
        src += wdata;
        w += wdata;
        // Skip the transparent area
        wskip = *(unsigned long *)src;
        src += 4;
        w += wskip;
        if (w > xshift)
          dst += w-xshift;
      }
    }
    while (w < lbDisplay.GraphicsScreenWidth + xshift)
    {
      wdata = *(unsigned long *)src;
      src += 4;
      if (lbDisplay.GraphicsScreenWidth + xshift >= wdata + w)
      {
        wcopy = wdata;
      } else
      {
        wcopy = lbDisplay.GraphicsScreenWidth + xshift - w;
        if (wcopy > lbDisplay.GraphicsScreenWidth)
          wcopy = lbDisplay.GraphicsScreenWidth;
        if (wcopy < 0)
          wcopy = 0;
      }
      memcpy(dst, src, wcopy);
      src += wdata;
      dst += wdata;
      w += wdata;
      // Transparent bytes count
      wskip = *((unsigned long *)src);
      src += 4;
      dst += wskip;
      w += wskip;
    }
  }
}

void unload_map_and_window(void)
{
  clear_light_system();
  clear_things_and_persons_data();
  clear_mapmap();
  clear_computer();
  clear_slabs();
  clear_rooms();
  clear_dungeons();
  LbMemoryCopy(frontend_palette, frontend_backup_palette, PALETTE_SIZE);
}

TbBool load_map_and_window(LevelNumber lvnum)
{
  struct LevelInformation *lvinfo;
  char *land_view;
  char *land_window;
  char *fname;
  long flen;
//  return _DK_load_map_and_window(cmpgidx);
  land_view = NULL;
  land_window = NULL;
  if (lvnum == SINGLEPLAYER_NOTSTARTED)
  {
    land_view = campaign.land_view_start;
    land_window = campaign.land_window_start;
  } else
  if (lvnum == SINGLEPLAYER_FINISHED)
  {
    land_view = campaign.land_view_end;
    land_window = campaign.land_window_end;
  } else
  {
    lvinfo = get_level_info(lvnum);
    if (lvinfo != NULL)
    {
      land_view = lvinfo->land_view;
      land_window = lvinfo->land_window;
    } else
    {
      land_view = campaign.land_view_start;
      land_window = campaign.land_window_start;
    }
  }
  if ((land_view == NULL) || (land_window == NULL))
  {
    ERRORLOG("No land View file names for level %d",lvnum);
    return false;
  }
  fname = prepare_file_fmtpath(FGrp_LandView,"%s.raw",land_view);
  flen = LbFileLengthRnc(fname);
  if (flen < 1024)
  {
    ERRORLOG("Land Map file doesn't exist or is too small");
    return false;
  }
  if (flen > 1228997)
  {
    ERRORLOG("Not enough memory in game structure for land Map");
    return false;
  }
  if (LbFileLoadAt(fname, &game.land_map_start) != flen)
  {
    ERRORLOG("Unable to load land Map background");
    return false;
  }
  map_screen = &game.land_map_start;
  memcpy(frontend_backup_palette, frontend_palette, PALETTE_SIZE);
  fname = prepare_file_fmtpath(FGrp_LandView,"%s.dat",land_window);
  wait_for_cd_to_be_available();
  if (LbFileLoadAt(fname, block_mem) == -1)
  {
    ERRORLOG("Unable to load Land Map Window");
    unload_map_and_window();
    return false;
  }
  window_y_offset = (long *)block_mem;
  fname = prepare_file_fmtpath(FGrp_LandView,"%s.pal",land_view);
  wait_for_cd_to_be_available();
  if (LbFileLoadAt(fname, frontend_palette) != PALETTE_SIZE)
  {
    ERRORLOG("Unable to load Land Map screen palette");
    unload_map_and_window();
    return 0;
  }
  map_window = (unsigned char *)(window_y_offset + 720);
  return true;
}

void frontnet_init_level_descriptions(void)
{
  find_and_load_lif_files();
  find_and_load_lof_files();
}

void frontnetmap_unload(void)
{
  clear_light_system();
  clear_mapmap();
  clear_things_and_persons_data();
  clear_computer();
  clear_rooms();
  clear_dungeons();
  clear_slabs();
  memcpy(&frontend_palette, frontend_backup_palette, PALETTE_SIZE);
  LbDataFreeAll(netmap_flag_load_files);
  memcpy(&frontend_palette, frontend_backup_palette, PALETTE_SIZE);
  fe_network_active = 0;
  if ((game.flags_cd & 0x10) == 0)
  {
    StopRedbookTrack();
    SetRedbookVolume(settings.redbook_volume);
  }
}

TbBool frontnetmap_load(void)
{
  long i,k;
  if (fe_network_active)
  {
    if (LbNetwork_EnableNewPlayers(0))
      ERRORLOG("Unable to prohibit new players joining exchange");
  }
  wait_for_cd_to_be_available();
  frontend_load_data_from_cd();
  game.selected_level_number = 0;
  if (!load_map_and_window(0))
  {
    frontend_load_data_reset();
    return false;
  }
  if (LbDataLoadAll(netmap_flag_load_files))
  {
    ERRORLOG("Unable to load MAP SCREEN sprites");
    return false;
  }
  LbSpriteSetupAll(netmap_flag_setup_sprites);
  frontend_load_data_reset();
  frontnet_init_level_descriptions();
  map_info.scrshift_x = (MAP_SCREEN_WIDTH - lbDisplay.PhysicalScreenWidth) / 2;
  map_info.scrshift_y = (MAP_SCREEN_HEIGHT - lbDisplay.PhysicalScreenHeight) / 2;
  fe_net_level_selected = SINGLEPLAYER_NOTSTARTED;
  net_level_hilighted = SINGLEPLAYER_NOTSTARTED;
  set_pointer_graphic_none();
  LbMouseSetPosition(lbDisplay.PhysicalScreenWidth/2, lbDisplay.PhysicalScreenHeight/2);
  lbFontPtr = map_font;
  LbTextSetWindow(0, 0, lbDisplay.PhysicalScreenWidth, lbDisplay.PhysicalScreenHeight);
  map_sound_fade = 256;
  lbDisplay.DrawFlags = 0;
  if ((game.flags_cd & 0x10) == 0)
    SetRedbookVolume(settings.redbook_volume);
  if (fe_network_active)
  {
    struct ScreenPacket *nspck;
    net_number_of_players = 0;
    for (i=0; i < 4; i++)
    {
      nspck = &net_screen_packet[i];
      if ((nspck->field_4 & 0x01) != 0)
        net_number_of_players++;
    }
  } else
  {
    net_number_of_players = 1;
  }
  net_map_slap_frame = 0;
  net_map_limp_time = 0;
  update_net_ensigns_visibility();
  return true;
}

TbBool frontmap_load(void)
{
  struct PlayerInfo *player;
  struct LevelInformation *lvinfo;
  struct TbSprite *spr;
  LevelNumber lvnum;
  long lvidx;
  SYNCDBG(18,"Starting");
//  return _DK_frontmap_load();
  memset(scratch, 0, PALETTE_SIZE);
  LbPaletteSet(scratch);
  play_desc_speech_time = 0;
  playing_good_descriptive_speech = 0;
  playing_bad_descriptive_speech = 0;
  played_good_descriptive_speech = 0;
  played_bad_descriptive_speech = 0;
  playing_speech_lvnum = SINGLEPLAYER_NOTSTARTED;
  mouse_over_lvnum = SINGLEPLAYER_NOTSTARTED;
  wait_for_cd_to_be_available();
  frontend_load_data_from_cd();
  lvnum = get_continue_level_number();
  if (!load_map_and_window(lvnum))
  {
    frontend_load_data_reset();
    return false;
  }
  if (LbDataLoadAll(map_flag_load_files))
  {
    ERRORLOG("Unable to load Land View Screen sprites");
    frontend_load_data_reset();
    return false;
  }
  LbSpriteSetupAll(map_flag_setup_sprites);
  frontend_load_data_reset();
  if ((game.flags_cd & 0x10) == 0)
    PlayRedbookTrack(2);
  player = &(game.players[my_player_number%PLAYERS_COUNT]);
  lvnum = get_continue_level_number();
  if ((player->field_6 & 0x02) != 0)
  {
    lvnum = get_loaded_level_number();
    lvinfo = get_level_info(lvnum);
    if (lvinfo != NULL)
    {
      map_info.field_A = lvinfo->ensign_zoom_x;
      map_info.field_E = lvinfo->ensign_zoom_y;
      set_map_info_draw_hotspot(lvinfo->ensign_zoom_x,lvinfo->ensign_zoom_y);
    } else
    {
      map_info.field_A = (MAP_SCREEN_WIDTH>>1);
      map_info.field_E = (MAP_SCREEN_HEIGHT>>1);
      set_map_info_draw_hotspot((MAP_SCREEN_WIDTH>>1),(MAP_SCREEN_HEIGHT>>1));
    }
    map_info.fade_pos = 240;
    map_info.fade_step = -4;
    map_info.field_0 = 1;
    map_info.field_1 = 1;
  } else
  if ((lvnum == first_singleplayer_level()) || (player->victory_state == VicS_LostLevel) || (player->victory_state == VicS_State3))
  {
    lvinfo = get_level_info(lvnum);
    if (lvinfo != NULL)
    {
      set_map_info_draw_hotspot(lvinfo->ensign_zoom_x,lvinfo->ensign_zoom_y);
      set_map_info_visible_hotspot(lvinfo->ensign_zoom_x, lvinfo->ensign_zoom_y);
    } else
    {
      set_map_info_draw_hotspot((MAP_SCREEN_WIDTH>>1),(MAP_SCREEN_HEIGHT>>1));
      set_map_info_visible_hotspot((MAP_SCREEN_WIDTH>>1),(MAP_SCREEN_HEIGHT>>1));
    }
    map_info.field_0 = 0;
    play_desc_speech_time = LbTimerClock() + 1000;
  } else
  {
    lvnum = prev_singleplayer_level(lvnum);
    lvinfo = get_level_info(lvnum);
    if (lvinfo != NULL)
    {
      map_info.field_A = lvinfo->ensign_zoom_x;
      map_info.field_E = lvinfo->ensign_zoom_y;
      set_map_info_draw_hotspot(lvinfo->ensign_zoom_x,lvinfo->ensign_zoom_y);
    } else
    {
      map_info.field_A = (MAP_SCREEN_WIDTH>>1);
      map_info.field_E = (MAP_SCREEN_HEIGHT>>1);
      set_map_info_draw_hotspot((MAP_SCREEN_WIDTH>>1),(MAP_SCREEN_HEIGHT>>1));
    }
    map_info.fade_pos = 240;
    map_info.fade_step = -4;
    map_info.field_0 = 1;
    map_info.field_1 = 1;
  }
  map_sound_fade = 256;
  map_info.field_26 = 0;
  map_info.field_1E = map_info.scrshift_x << 8;
  map_info.field_2A = 0;
  map_info.field_22 = map_info.scrshift_y << 8;
  set_pointer_graphic_spland(0);
  LbMouseSetPosition(lbDisplay.PhysicalScreenWidth/2, lbDisplay.PhysicalScreenHeight/2);
  if ((features_enabled & Ft_AdvAmbSonud) != 0)
  {
    play_sample_using_heap(0, campaign.ambient_good, 0, 0x40, 100, -1, 2, 0);
    play_sample_using_heap(0, campaign.ambient_bad, 0, 0x40, 100, -1, 2, 0);
  }
  if ((game.flags_cd & 0x10) == 0)
    SetRedbookVolume(settings.redbook_volume);
  fe_computer_players = 0;
  update_ensigns_visibility();
  return true;
}

void process_map_zoom_in(void)
{
  update_frontmap_info_draw_hotspot();
  if (map_sound_fade > 0)
  {
    map_sound_fade = 256 + 5 * (1-map_info.fade_pos) / 4;
    if (map_sound_fade < 0)
      map_sound_fade = 0;
  }
}

void process_zoom_palette(void)
{
  if (map_info.fade_step >= 0)
  {
      if (map_info.fade_pos >= 120)
      {
        LbPaletteFade(NULL, 29, Lb_PALETTE_FADE_OPEN);
      }
  } else
  {
      if (map_info.fade_pos > 120)
      {
        if (map_info.fade_step+map_info.fade_pos > 120)
          LbPaletteFade(frontend_palette, 29, Lb_PALETTE_FADE_OPEN);
        else
          LbPaletteSet(frontend_palette);
      }
  }
}

TbBool update_zoom(void)
{
  SYNCDBG(8,"Starting");
  if (map_info.fade_step == 4)
  {
    process_map_zoom_in();
  }
  map_info.fade_pos += map_info.fade_step;
  if ((map_info.fade_pos <= 1) || (map_info.fade_pos >= 240))
  {
    LbPaletteStopOpenFade();
    map_info.field_1 = 0;
    if (map_info.field_12 != 0)
    {
      frontend_set_state(map_info.field_12);
      LbScreenClear(0);
      LbScreenSwap();
      map_info.field_12 = 0;
      return true;
    }
  }
  process_zoom_palette();
  return false;
}

TbBool rectangle_intersects(struct ARect *rcta, struct ARect *rctb)
{
  long left, top, right, bottom;
  left = rcta->left;
  if (rcta->left <= rctb->left)
    left = rctb->left;
  top = rcta->top;
  if (top <= rctb->top)
    top = rctb->top;
  right = rcta->right;
  if (right >= rctb->right)
    right = rctb->right;
  bottom = rcta->bottom;
  if (bottom >= rctb->bottom)
    bottom = rctb->bottom;
  return (left < right) && (top < bottom);
}

TbBool test_hand_slap_collides(long plyr_idx)
{
  struct ARect rcta;
  struct ARect rctb;
  struct ScreenPacket *nspck;
  if (plyr_idx == my_player_number)
    return false;
  nspck = &net_screen_packet[my_player_number];
  if ((nspck->field_4 >> 3) == 0x02)
    return false;
  // Rectangle of given player
  nspck = &net_screen_packet[plyr_idx];
  rcta.left = nspck->field_6 - 7;
  rcta.top = nspck->field_8 - 13;
  rcta.right = rcta.left + 30;
  rcta.bottom = rcta.top + 20;
  // Rectangle of local player
  nspck = &net_screen_packet[my_player_number];
  if ((nspck->field_4 >> 3) == 0x01)
  {
    rctb.left = nspck->field_6 - 31;
    rctb.top = nspck->field_8 - 27;
    rctb.right = map_hand[9].SWidth + rctb.left;
    rctb.bottom = rctb.top + map_hand[9].SHeight;
  } else
  if (nspck->field_A != SINGLEPLAYER_NOTSTARTED)
  {
    rctb.left = nspck->field_6 - 20;
    rctb.top = nspck->field_8 - 14;
    rctb.right = map_hand[17].SWidth + rctb.left;
    rctb.bottom = rctb.top + map_hand[17].SHeight;
  } else
  {
    rctb.left = nspck->field_6 - 19;
    rctb.top = nspck->field_8 - 25;
    rctb.right = map_hand[1].SWidth + rctb.left;
    rctb.bottom = rctb.top + map_hand[1].SHeight;
  }
  // Return if the rectangles are intersecting
  if (rectangle_intersects(&rcta, &rctb))
    return true;
  return false;
}

void frontmap_draw(void)
{
  SYNCDBG(8,"Starting");
  if (map_info.field_1)
  {
    frontzoom_to_point(map_info.field_A, map_info.field_E, map_info.fade_pos);
    compressed_window_draw();
  } else
  {
    draw_map_screen();
    draw_map_level_ensigns();
    set_pointer_graphic_spland(0);
    compressed_window_draw();
  }
}

void check_mouse_scroll(void)
{
  long mx,my;
  mx = GetMouseX();
  if (mx < 8)
  {
    map_info.field_26 -= 8;
    if (map_info.field_26 < -48)
      map_info.field_26 = -48;
    if (map_info.field_26 > 48)
      map_info.field_26 = 48;
  } else
  if (mx >= lbDisplay.PhysicalScreenWidth-8)
  {
    map_info.field_26 += 8;
    if (map_info.field_26 < -48)
      map_info.field_26 = -48;
    if (map_info.field_26 > 48)
      map_info.field_26 = 48;
  }
  my = GetMouseY();
  if (my < 8)
  {
    map_info.field_2A -= 8;
    if (map_info.field_2A < -48)
      map_info.field_2A = -48;
    if (map_info.field_2A > 48)
      map_info.field_2A = 48;
  } else
  if (my >= lbDisplay.PhysicalScreenHeight-8)
  {
    map_info.field_2A += 8;
    if (map_info.field_2A < -48)
      map_info.field_2A = -48;
    if (map_info.field_2A > 48)
      map_info.field_2A = 48;
  }
}

void update_velocity(void)
{
  if (map_info.field_26 != 0)
  {
    map_info.scrshift_x += map_info.field_26 / 4;
    if (MAP_SCREEN_WIDTH-lbDisplay.PhysicalScreenWidth < map_info.scrshift_x)
      map_info.scrshift_x = MAP_SCREEN_WIDTH-lbDisplay.PhysicalScreenWidth;
    if (map_info.scrshift_x < 0)
      map_info.scrshift_x = 0;
    if (map_info.field_26 < 0)
      map_info.field_26 += 2;
    else
      map_info.field_26 -= 2;
  }
  if (map_info.field_2A != 0)
  {
    map_info.scrshift_y += map_info.field_2A / 4;
    if (MAP_SCREEN_HEIGHT-lbDisplay.PhysicalScreenHeight < map_info.scrshift_y)
      map_info.scrshift_y = MAP_SCREEN_HEIGHT - lbDisplay.PhysicalScreenHeight;
    if (map_info.scrshift_y < 0)
      map_info.scrshift_y = 0;
    if (map_info.field_2A < 0)
      map_info.field_2A += 2;
    else
      map_info.field_2A -= 2;
  }
}

/*
 * Draws player's hands.
 */
void draw_netmap_players_hands(void)
{
  struct ScreenPacket *nspck;
  struct TbNetworkPlayerName *plyr_nam;
  struct TbSprite *spr;
  TbPixel colr;
  long x,y,w,h;
  long i,k,n;
  for (i=0; i < NET_PLAYERS_COUNT; i++)
  {
      nspck = &net_screen_packet[i];
      plyr_nam = &net_player[i];
      colr = net_player_colours[i];
      if ((nspck->field_4 & 0x01) != 0)
      {
        x = 0;
        y = 0;
        n = nspck->field_4 & 0xF8;
        if (n == 8)
        {
          spr = &map_hand[nspck->field_A];
        } else
        if (n == 16)
        {
          k = nspck->field_B;
          if (k > 11)
            k = 11;
          if (k < 0)
            k = 0;
          x = hand_limp_xoffset[k];
          y = hand_limp_yoffset[k];
          spr = &map_hand[21];
        } else
        if (nspck->field_A == SINGLEPLAYER_NOTSTARTED)
        {
            k = LbTimerClock() / 150;
            spr = &map_hand[1 + (k%8)];
        } else
        {
            k = timeGetTime() / 150;
            spr = &map_hand[17 + (k%4)];
        }
        x += nspck->field_6 - map_info.scrshift_x - 18;
        y += nspck->field_8 - map_info.scrshift_y - 25;
        LbSpriteDraw(x, y, spr);
        w = LbTextStringWidth(plyr_nam->name);
        if (w > 0)
        {
          lbDisplay.DrawFlags = 0;
          h = LbTextHeight(level_name);
          y += 32;
          x += 32;
          LbDrawBox(x-4, y, w+8, h, colr);
          LbTextDraw(x, y, plyr_nam->name);
        }
      }
  }
}

/*
 * Draws text description of active level.
 */
void draw_map_level_descriptions(void)
{
  struct LevelInformation *lvinfo;
  char *lv_name;
  long lvnum;
  long x,y,w,h;
  if ((fe_net_level_selected > 0) || (net_level_hilighted > 0))
  {
    lbDisplay.DrawFlags = 0;
    lvnum = fe_net_level_selected;
    if (lvnum <= 0)
      lvnum = net_level_hilighted;
    lvinfo = get_level_info(lvnum);
    if (lvinfo == NULL)
      return;
    if ((lvinfo->name_id > 0) && (lvinfo->name_id < STRINGS_MAX))
      lv_name = campaign.strings[lvinfo->name_id];
    else
      lv_name = lvinfo->name;
    if ((lv_name != NULL) && (strlen(lv_name) > 0))
      sprintf(level_name, "%s %d: %s", gui_strings[406], lvinfo->lvnum, lv_name);
    else
      sprintf(level_name, "%s %d", gui_strings[406], lvinfo->lvnum);
    w = LbTextStringWidth(level_name);
    x = lvinfo->ensign_x - map_info.scrshift_x;
    y = lvinfo->ensign_y - map_info.scrshift_y - 8;
    h = LbTextHeight(level_name);
    LbDrawBox(x-4, y, w+8, h, 0);
    LbTextDraw(x, y, level_name);
  }
}

void frontnetmap_draw(void)
{
  SYNCDBG(8,"Starting");
  if (map_info.field_1)
  {
    frontzoom_to_point(map_info.field_A, map_info.field_E, map_info.fade_pos);
    compressed_window_draw();
  } else
  {
    draw_map_screen();
    draw_map_level_ensigns();
    draw_netmap_players_hands();
    draw_map_level_descriptions();
    compressed_window_draw();
  }
}

void frontmap_input(void)
{
  SYNCDBG(8,"Starting");
  short zoom_done;
  long mouse_x,mouse_y;
  if (map_info.field_0)
  {
    if (!map_info.field_1)
    {
      map_info.field_0 = 0;
      play_desc_speech_time = LbTimerClock() + 1000;
    }
    zoom_done = false;
  } else
  {
    zoom_done = true;
  }
  if (is_key_pressed(KC_ESCAPE, KM_DONTCARE))
  {
    clear_key_pressed(KC_ESCAPE);
    frontend_set_state(FeSt_MAIN_MENU);
    LbPaletteStopOpenFade();
    return;
  }
  if (zoom_done)
  {
    check_mouse_scroll();
    if (is_key_pressed(KC_F11, KM_CONTROL))
    {
      if ((game.flags_font & FFlg_AlexCheat) != 0)
      {
        set_all_ensigns_state(LvSt_Visible);
        clear_key_pressed(KC_F11);
        return;
      }
    }
    if (is_key_pressed(KC_F10, KM_CONTROL))
    {
      if ((game.flags_font & FFlg_AlexCheat) != 0)
      {
        move_campaign_to_next_level();
        frontmap_unload();
        frontmap_load();
        //update_ensigns_visibility();
        clear_key_pressed(KC_F10);
        return;
      }
    }
    if (is_key_pressed(KC_F9, KM_CONTROL))
    {
      if ((game.flags_font & FFlg_AlexCheat) != 0)
      {
        move_campaign_to_prev_level();
        frontmap_unload();
        frontmap_load();
        //update_ensigns_visibility();
        clear_key_pressed(KC_F9);
        return;
      }
    }
    if (left_button_clicked)
    {
      left_button_clicked = 0;
      frontmap_input_active_ensign(left_button_clicked_x, left_button_clicked_y);
      if (clicked_map_level_ensign())
        return;
    }
    mouse_x = GetMouseX();
    mouse_y = GetMouseY();
    frontmap_input_active_ensign(mouse_x, mouse_y);
  }
  update_velocity();
}

void frontnetmap_input(void)
{
  struct LevelInformation *lvinfo;
  long i;
  if (lbKeyOn[KC_ESCAPE])
  {
    fe_net_level_selected = -2;
    lbKeyOn[KC_ESCAPE] = 0;
    return;
  }

  if (net_map_limp_time == 0)
  {
    if (right_button_clicked)
    {
        right_button_clicked = 0;
        if (fe_network_active)
        {
          if (fe_net_level_selected == SINGLEPLAYER_NOTSTARTED)
          {
            if ((!net_map_slap_frame) && (!net_map_limp_time))
            {
                net_map_slap_frame = 9;
            }
          } else
          {
            lvinfo = get_level_info(fe_net_level_selected);
            if (lvinfo != NULL)
              LbMouseSetPosition(lvinfo->ensign_x - map_info.scrshift_x, lvinfo->ensign_y - map_info.scrshift_y);
            fe_net_level_selected = SINGLEPLAYER_NOTSTARTED;
          }
        }
    }

    if (fe_net_level_selected == SINGLEPLAYER_NOTSTARTED)
    {
      net_level_hilighted = SINGLEPLAYER_NOTSTARTED;
      frontmap_input_active_ensign(GetMouseX(), GetMouseY());
      if (mouse_over_lvnum > 0)
        net_level_hilighted = mouse_over_lvnum;
      if (net_level_hilighted > 0)
      {
        if ((net_map_slap_frame == 0) && (net_map_limp_time == 0))
        {
          if ( left_button_clicked )
          {
            fe_net_level_selected = net_level_hilighted;
            left_button_clicked = 0;
            lvinfo = get_level_info(fe_net_level_selected);
            if (lvinfo != NULL)
              sprintf(level_name, "%s %d", gui_strings[406], lvinfo->lvnum);
            else
              sprintf(level_name, "%s", gui_strings[406]);
          }
        }
      }
      check_mouse_scroll();
      update_velocity();
    }
  }
}

void frontmap_unload(void)
{
  SYNCDBG(8,"Starting");
  set_pointer_graphic_none();
  unload_map_and_window();
  LbDataFreeAll(map_flag_load_files);
  StopAllSamples();
  stop_description_speech();
  if ((game.flags_cd & 0x10) == 0)
  {
    StopRedbookTrack();
    SetRedbookVolume(settings.redbook_volume);
  }
}

long frontmap_update(void)
{
  SYNCDBG(8,"Starting");
  if ((mouse_over_lvnum > 0) && (playing_speech_lvnum != mouse_over_lvnum))
  {
    play_desc_speech_time = 0;
    play_description_speech(mouse_over_lvnum,1);
  }
  if ((play_desc_speech_time != 0) && (LbTimerClock() > play_desc_speech_time))
  {
    play_desc_speech_time = 0;
    play_current_description_speech(1);
  }
  update_frontmap_ambient_sound();

  if ( map_info.field_1 )
  {
    if (map_info.fade_step == 4)
    {
      process_map_zoom_in();
    }
    map_info.fade_pos += map_info.fade_step;
    if ((map_info.fade_pos <= 1) || (map_info.fade_pos >= 240))
    {
      LbPaletteStopOpenFade();
      map_info.field_1 = 0;
      if (map_info.field_12)
      {
        frontend_set_state(map_info.field_12);
        LbScreenClear(0);
        LbScreenSwap();
        map_info.field_12 = 0;
        return true;
      }
    }
    process_zoom_palette();
  }
  if (playing_good_descriptive_speech)
  {
    if (StreamedSampleFinished())
    {
      playing_good_descriptive_speech = 0;
//      playing_speech_lvnum = SINGLEPLAYER_NOTSTARTED;
    }
  }
  if ((game.flags_cd & 0x10) == 0)
    PlayRedbookTrack(2);
  SYNCDBG(8,"Finished");
  return 0;
}

long frontnetmap_update(void)
{
  struct ScreenPacket *nspck;
  struct LevelInformation *lvinfo;
  struct TbSprite *spr;
  long i;
  long tmp1,tmp2;
  long lvnum;
  TbBool is_selected;
  SYNCDBG(8,"Starting");
//  return _DK_frontnetmap_update();
  if (map_sound_fade > 0)
  {
    i = map_sound_fade * settings.redbook_volume / 256;
  } else
  {
    i = 0;
  }
  if ((game.flags_cd & 0x10) == 0)
  {
    SetRedbookVolume(i);
  }

  if (map_info.field_1 == 0)
  {
    memset(net_screen_packet, 0, sizeof(net_screen_packet));
    nspck = &net_screen_packet[my_player_number];
    nspck->field_4 |= 0x01;
    nspck->field_A = fe_net_level_selected;
    if (net_map_limp_time > 0)
    {
      nspck->field_4 = (nspck->field_4 & 0x07) | 0x10;
      nspck->field_6 = limp_hand_x;
      nspck->field_8 = limp_hand_y;
      net_map_limp_time--;
      nspck->field_B = net_map_limp_time;
      if (net_map_limp_time == 1)
      {
        LbMouseSetPosition(
          limp_hand_x + hand_limp_xoffset[0] - map_info.scrshift_x,
          limp_hand_y + hand_limp_yoffset[0] - map_info.scrshift_y);
      }
    } else
    if (fe_net_level_selected > 0)
    {
      spr = get_map_flag(1);
      lvinfo = get_level_info(fe_net_level_selected);
      if (lvinfo != NULL)
      {
        nspck->field_6 = lvinfo->ensign_x + my_player_number * spr->SWidth;
        nspck->field_8 = lvinfo->ensign_y - 48;
      }
    } else
    if (net_map_slap_frame > 0)
    {
      nspck->field_6 = GetMouseX() + map_info.scrshift_x;
      nspck->field_8 = GetMouseY() + map_info.scrshift_y;
      if (net_map_slap_frame <= 16)
      {
        nspck->field_4 = (nspck->field_4 & 0x07) | 0x08;
        nspck->field_A = net_map_slap_frame;
        net_map_slap_frame++;
      } else
      {
        net_map_slap_frame = 0;
      }
    } else
    {
      nspck->field_6 = GetMouseX() + map_info.scrshift_x;
      nspck->field_8 = GetMouseY() + map_info.scrshift_y;
    }
    if (fe_network_active)
    {
      if ( LbNetwork_Exchange(nspck) )
        ERRORLOG("LbNetwork_Exchange failed");
    }
    memset(scratch, 0, PALETTE_SIZE);
    is_selected = false;
    lvnum = SINGLEPLAYER_NOTSTARTED;
    tmp1 = 0;
    tmp2 = -1;
    for (i=0; i < NET_PLAYERS_COUNT; i++)
    {
      nspck = &net_screen_packet[i];
      if ((nspck->field_4 & 0x01) == 0)
        continue;
      if (nspck->field_A == -2)
      {
          if (fe_network_active)
          {
            if ( LbNetwork_EnableNewPlayers(1) )
              ERRORLOG("Unable to enable new players joining exchange");
            frontend_set_state(FeSt_NET_START);
          } else
          {
            frontend_set_state(FeSt_MAIN_MENU);
          }
          return 0;
      }
      if ((nspck->field_A == SINGLEPLAYER_NOTSTARTED) || ((nspck->field_4 & 0xF8) == 8))
      {
          tmp1++;
      } else
      {
          scratch[nspck->field_A]++;
          if (scratch[nspck->field_A] == tmp2)
          {
            is_selected = false;
          } else
          if (scratch[nspck->field_A] > tmp2)
          {
            lvnum = nspck->field_A;
            tmp2 = scratch[lvnum];
            is_selected = true;
          }
      }
      if (((nspck->field_4 & 0xF8) == 0x08) && (nspck->field_A == 13))
      {
          if ( test_hand_slap_collides(i) )
          {
            net_map_limp_time = 12;
            fe_net_level_selected = SINGLEPLAYER_NOTSTARTED;
            net_map_slap_frame = 0;
            limp_hand_x = nspck->field_6;
            limp_hand_y = nspck->field_8;
            nspck->field_4 = (nspck->field_4 & 7) | 0x10;
          }
      }
    }
  } else
  {
    if (update_zoom())
    {
      SYNCDBG(8,"Zoom end");
      return 1;
    }
  }
  if ((!tmp1) && (lvnum > 0) && (is_selected))
  {
    set_selected_level_number(lvnum);
    sprintf(level_name, "%s %d", gui_strings[406], lvnum);
    map_info.field_1 = 1;
    map_info.fade_pos = 1;
    map_info.fade_step = 4;
    spr = get_map_flag(1);
    lvinfo = get_level_info(lvnum);
    if (lvinfo != NULL)
    {
      map_info.field_A = lvinfo->ensign_x + my_player_number * spr->SWidth;
      map_info.field_E = lvinfo->ensign_y - 48;
    }
    map_info.field_12 = 8 - ((unsigned int)fe_network_active < 1);
    set_map_info_visible_hotspot(map_info.field_A, map_info.field_E);
    map_info.field_1E = map_info.scrshift_x << 8;
    map_info.field_22 = map_info.scrshift_y << 8;
    if (!fe_network_active)
      fe_computer_players = 1;
  }

  if ((game.flags_cd & 0x10) == 0)
    PlayRedbookTrack(2);
  SYNCDBG(8,"Normal end");
  return 0;
}




/******************************************************************************/
#ifdef __cplusplus
}
#endif