/******************************************************************************/
// Free implementation of Bullfrog's Dungeon Keeper strategy game.
/******************************************************************************/
/** @file gui_tooltips.c
 *     Tooltips support functions.
 * @par Purpose:
 *     Functions to show, draw and update the in-game tooltips.
 * @par Comment:
 *     None.
 * @author   Tomasz Lis
 * @date     26 Feb 2009 - 14 May 2009
 * @par  Copying and copyrights:
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 2 of the License, or
 *     (at your option) any later version.
 */
/******************************************************************************/
#include "gui_tooltips.h"
#include "globals.h"
#include "bflib_memory.h"
#include "bflib_guibtns.h"

#include "kjm_input.h"
#include "frontend.h"
#include "keeperfx.h"
#include "config_creature.h"

#ifdef __cplusplus
extern "C" {
#endif
/******************************************************************************/
DLLIMPORT void _DK_draw_tooltip(void);
DLLIMPORT void _DK_setup_gui_tooltip(struct GuiButton *gbtn);

/******************************************************************************/
const char jtytext[] = "JONTY HERE   : ...I AM WRITING THIS AT 4AM ON KEEPERS LAST DAY. I LOOK AROUND THE OFFICE AND ALL I SEE ARE THE TIRED PALE FACES OF THE KEEPER TEAM. THIS PROJECT HAS DESTROYED THE HEALTH AND SOCIAL LIVES OF EACH MEMBER, SO I HOPE YOU LIKE THE GAME. "
    "AMAZINGLY AFTER SIXTEEN HOURS A DAY, 7 DAYS A WEEK, FOR NEARLY 5 MONTHS WE STILL DO. THIS GAME HAS BEEN WRITTEN WITH A PASSION I AM PROUD TO BE PART OF.... I DO NOT JUST HOPE YOU LIKE IT, I ALSO HOPE YOU ARE AWARE OF THE HUGE AMOUNT OF WORK WE HAVE ALL DONE. "
    "ENOUGH WAFFLE AND ON TO THE REASON FOR THIS TEXT... THE ENDLESS GREETINGS... GREETINGS GO OUT TO IN RANDOM ORDER... RAB C, JACOON, BUCK, SI, BARRIE, KIK, CHRIS, PROC, RUSS, RIK, ALEX NEEDS, LARRY MOOR, EMMA TEUTON - AN ALE DUE TO EACH ONE OF YOU - THE LIST GOES ON.... "
    "LOUISE GEE, MY GOOD SIS NICOLA, GEMMA, JENNY, HALEY, JO EVANS...  SARAH TELFORD.. PETER AND STEVEN, AMELIA, SARAH, ISY, SALLY, MARK, MATT WOODCOCK, PAUL NETTLETON, PETE BANFORD, TOM, DAVE BANHAM, DAVE KEENS, ALISON BOSSON, MIKE LINCOLN, KIRSTY, DARREN SAWYER... AND ALL UEA NORWICH PAST AND PRESENT. "
    "NICK ARNOTT, BAVERSTOCK, SARAH BANKS, SEANY, MARK STACEY, GILEY MILEY COOKSON [WHERE ARE YOU?], STEVE CLARIDGE, DEUBERT, JAMES GREENGRASS, SIMON ONG, KEVIN RUSSELL, CLARE WRIGHTON, ELTON [GIB IS JUST A DAY AWAY], NICOLA GOULD, STEVE LAST, KEN MALCOLN, RICO, ANDY CAKEBREAD, ROBBO, CARR, "
    "AND THE LITTLE ONE, CROFTY, SCOOPER, JASON STANTON [A CUP OF COFFEE], AARON SENNA, MIKE DORELL, IAN HOWIE, HELEN THAIN, ALEX FOREST-HAY, LEE HAZELWOOD, VICKY ARNOLD, GUY SIMMONS, SHIN, VAL TAYLOR.... IF I FORGOT YOU I AM SORRY... BUT SLEEP IS DUE TO ME... AND I HAVE A DREAM TO LIVE...";

/******************************************************************************/

inline void reset_scrolling_tooltip(void)
{
    tooltip_scroll_offset = 0;
    tooltip_scroll_timer = 25;
}

inline void set_gui_tooltip_box(int bxtype,long stridx)
{
  tool_tip_box.field_0 = 1;
  if ((stridx > 0) && (stridx < STRINGS_MAX))
    strncpy(tool_tip_box.text, gui_strings[stridx], TOOLTIP_MAX_LEN);
  else
    strcpy(tool_tip_box.text, "n/a");
  tool_tip_box.pos_x = GetMouseX();
  tool_tip_box.pos_y = GetMouseY()+86;
  tool_tip_box.field_809 = bxtype;
}

inline void set_gui_tooltip_box_fmt(int bxtype,const char *format, ...)
{
  tool_tip_box.field_0 = 1;
  va_list val;
  va_start(val, format);
  vsprintf(tool_tip_box.text, format, val);
  va_end(val);
  tool_tip_box.pos_x = GetMouseX();
  tool_tip_box.pos_y = GetMouseY()+86;
  tool_tip_box.field_809 = bxtype;
}

inline TbBool update_gui_tooltip_target(void *target)
{
  if (target != tool_tip_box.target)
  {
    help_tip_time = 0;
    tool_tip_box.target = target;
    return true;
  }
  return false;
}

inline void clear_gui_tooltip_target(void)
{
  help_tip_time = 0;
  tool_tip_box.target = NULL;
}

inline void clear_gui_tooltip_button(void)
{
  tool_tip_time = 0;
  tool_tip_box.gbutton = NULL;
}

TbBool setup_trap_tooltips(struct Coord3d *pos)
{
  struct Thing *thing;
  struct PlayerInfo *player;
  SYNCDBG(18,"Starting");
  thing = get_trap_for_slab_position(map_to_slab[pos->x.stl.num],map_to_slab[pos->y.stl.num]);;
  if (thing_is_invalid(thing)) return false;
  player = &(game.players[my_player_number%PLAYERS_COUNT]);
  if ((thing->byte_17.h == 0) && (player->field_2B != thing->owner))
    return false;
  update_gui_tooltip_target(thing);
  if ((help_tip_time > 20) || (player->work_state == 12))
  {
    set_gui_tooltip_box(4,trap_data[thing->model%MANUFCTR_TYPES_COUNT].name_stridx);
  } else
  {
    help_tip_time++;
  }
  return true;
}

TbBool setup_object_tooltips(struct Coord3d *pos)
{
  char *text;
  struct Thing *thing;
  struct CreatureData *crdata;
  struct PlayerInfo *player;
  long i;
  SYNCDBG(18,"Starting");
  player = &(game.players[my_player_number%PLAYERS_COUNT]);
  // Find a special to show tooltip for
  thing = thing_get(player->thing_under_hand);
  if (thing_is_invalid(thing) || !thing_is_special(thing))
    thing = get_special_at_position(pos->x.stl.num, pos->y.stl.num);
  if (thing != NULL)
  {
    update_gui_tooltip_target(thing);
    set_gui_tooltip_box(5,specials_text[thing_to_special(thing)]);
    return true;
  }
  // Find a spellbook to show tooltip for
  thing = get_spellbook_at_position(pos->x.stl.num, pos->y.stl.num);
  if (thing != NULL)
  {
    update_gui_tooltip_target(thing);
    i = object_to_magic[thing->model];
    set_gui_tooltip_box(5,spell_data[i].field_D);
    return true;
  }
  // Find a workshop crate to show tooltip for
  thing = _DK_get_crate_at_position(pos->x.stl.num, pos->y.stl.num);
  if (thing != NULL)
  {
    update_gui_tooltip_target(thing);
    if (workshop_object_class[thing->model%OBJECT_TYPES_COUNT] == 8)
      i = trap_data[object_to_door_or_trap[thing->model%OBJECT_TYPES_COUNT]].name_stridx;
    else
      i = door_names[object_to_door_or_trap[thing->model%OBJECT_TYPES_COUNT]];
    set_gui_tooltip_box(5,i);
    return true;
  }
  if (!settings.tooltips_on)
    return false;
  // Find a hero gate/creature lair to show tooltip for
  thing = _DK_get_nearest_object_at_position(pos->x.stl.num, pos->y.stl.num);
  if (thing != NULL)
  {
    if (thing->model == 49)
    {
      update_gui_tooltip_target(thing);
      if ( (help_tip_time > 20) || (player->work_state == 12))
      {
        set_gui_tooltip_box(5,545); // Hero Gate tooltip
      } else
      {
        help_tip_time++;
      }
      return true;
    }
    if (_DK_objects[thing->model].field_13)
    {
      update_gui_tooltip_target(thing);
      if ( (help_tip_time > 20) || (player->work_state == 12))
      {
        crdata = creature_data_get(_DK_objects[thing->model].field_13);
        set_gui_tooltip_box_fmt(5,"%s %s", gui_strings[crdata->namestr_idx%STRINGS_MAX], gui_strings[609]); // (creature) Lair
      } else
      {
        help_tip_time++;
      }
      return true;
    }
  }
  return false;
}

short setup_land_tooltips(struct Coord3d *pos)
{
  struct PlayerInfo *player;
  struct SlabMap *slb;
  int attridx;
  int stridx;
  SYNCDBG(18,"Starting");
  if (!settings.tooltips_on)
    return false;
  slb = get_slabmap_for_subtile(pos->x.stl.num, pos->y.stl.num);
  attridx = slb->slab;
  stridx = slab_attrs[attridx].field_0;
  if (stridx == 201)
    return false;
  update_gui_tooltip_target((void *)attridx);
  player = &(game.players[my_player_number%PLAYERS_COUNT]);
  if ((help_tip_time > 20) || (player->work_state == 12))
  {
    set_gui_tooltip_box(2,stridx);
  } else
  {
    help_tip_time++;
  }
  return true;
}

short setup_room_tooltips(struct Coord3d *pos)
{
  struct PlayerInfo *player;
  struct SlabMap *slb;
  struct Room *room;
  int stridx;
  if (!settings.tooltips_on)
    return false;
  slb = get_slabmap_block(map_to_slab[pos->x.stl.num], map_to_slab[pos->y.stl.num]);
  room = &game.rooms[slb->room_index];
  if (room == NULL)
    return false;
  stridx = room_data[room->kind].field_13;
  if (stridx == 201)
    return false;
  update_gui_tooltip_target(room);
  player = &(game.players[my_player_number%PLAYERS_COUNT]);
  int widener=0;
  if ( (help_tip_time > 20) || (player->work_state == 12) )
  {
    set_gui_tooltip_box(1,stridx);
  } else
  {
    help_tip_time++;
  }
  return true;
}

short setup_scrolling_tooltips(struct Coord3d *mappos)
{
  short shown;
  SYNCDBG(18,"Starting");
  shown = false;
  if (!shown)
    shown = setup_trap_tooltips(mappos);
  if (!shown)
    shown = setup_object_tooltips(mappos);
  if (!shown)
    shown = setup_land_tooltips(mappos);
  if (!shown)
    shown = setup_room_tooltips(mappos);
  if (!shown)
  {
    clear_gui_tooltip_target();
  }
  return shown;
}

void setup_gui_tooltip(struct GuiButton *gbtn)
{
  struct PlayerInfo *player;
  struct Dungeon *dungeon;
  struct CreatureData *crdata;
  char *text;
  long i,k;
  if (gbtn->tooltip_id == 201)
    return;
  if (!settings.tooltips_on)
    return;
  dungeon = &(game.dungeon[my_player_number%DUNGEONS_COUNT]);
  tool_tip_box.field_0 = 1;
  i = gbtn->tooltip_id;
  if ((i >= 0) && (i < STRINGS_MAX))
    text = gui_strings[i];
  else
    text = lbEmptyString;
  if ((i == 456) || (i == 455))
  {
    k = (long)tool_tip_box.gbutton->field_33;
    player = &(game.players[k%PLAYERS_COUNT]);
    if (player->field_15[0] != '\0')
      sprintf(tool_tip_box.text, "%s: %s", text, player->field_15);
    else
      sprintf(tool_tip_box.text, "%s", text);
  } else
  if ((i == 658) && (dungeon->field_8D4 > 16))
  {
    strncpy(tool_tip_box.text, jtytext, TOOLTIP_MAX_LEN);
  } else
  if (i == 733)
  {
    if (gbtn->field_1B > 0)
      k = breed_activities[(top_of_breed_list+gbtn->field_1B)%CREATURE_TYPES_COUNT];
    else
      k = 23;
    crdata = creature_data_get(k);
    sprintf(tool_tip_box.text, "%-6s: %s", gui_strings[crdata->namestr_idx], text);
  } else
  {
    strncpy(tool_tip_box.text, text, TOOLTIP_MAX_LEN);
  }
  if (gbtn != tool_tip_box.gbutton)
  {
    tool_tip_box.gbutton = gbtn;
    tool_tip_box.pos_x = GetMouseX();
    tool_tip_box.pos_y = GetMouseY()+86;
  }
}

TbBool gui_button_tooltip_update(int gbtn_idx)
{
  static TbBool doing_tooltip = false;
  struct PlayerInfo *player;
  struct GuiButton *gbtn;
  if ((gbtn_idx < 0) || (gbtn_idx >= ACTIVE_BUTTONS_COUNT))
  {
    clear_gui_tooltip_button();
    return false;
  }
  doing_tooltip = false;
  player = &(game.players[my_player_number%PLAYERS_COUNT]);
  gbtn = &active_buttons[gbtn_idx];
  if ((active_menus[gbtn->gmenu_idx].field_1 == 2) && ((gbtn->field_1B & 0x8000u) == 0))
  {
    if (tool_tip_box.gbutton == gbtn)
    {
        if ((tool_tip_time > 10) || (player->work_state == 12))
        {
          busy_doing_gui = 1;
          if (gbtn->field_13 != gui_area_text)
            setup_gui_tooltip(gbtn);
        } else
        {
          tool_tip_time++;
          doing_tooltip = true;
          busy_doing_gui = 1;
        }
    } else
    {
        tool_tip_time = 0;
        tool_tip_box.gbutton = gbtn;
        tool_tip_box.pos_x = GetMouseX();
        tool_tip_box.pos_y = GetMouseY()+86;
        tool_tip_box.field_809 = 0;
    }
    return true;
  }
  clear_gui_tooltip_button();
  return false;
}

short input_gameplay_tooltips(TbBool gameplay_on)
{
  struct Coord3d mappos;
  struct PlayerInfo *player;
  TbBool shown;
  SYNCDBG(7,"Starting");
  shown = false;
  player = &(game.players[my_player_number%PLAYERS_COUNT]);
  if ((gameplay_on) && (tool_tip_time == 0) && (!busy_doing_gui))
  {
    if (player->acamera == NULL)
    {
      ERRORLOG("No active camera");
      return false;
    }
    if (screen_to_map(player->acamera,GetMouseX(),GetMouseY(),&mappos))
    {
      if (subtile_revealed(mappos.x.stl.num,mappos.y.stl.num, player->field_2B))
      {
        if (player->field_37 != 1)
          shown = setup_scrolling_tooltips(&mappos);
      }
    }
  }
  if (tool_tip_box.field_0 == 0)
    reset_scrolling_tooltip();
  SYNCDBG(19,"Finished");
  return shown;
}

void toggle_tooltips(void)
{
  const char *statstr;
  settings.tooltips_on = !settings.tooltips_on;
  if (settings.tooltips_on)
  {
    do_sound_menu_click();
    statstr = "on";
  } else
  {
    statstr = "off";
  }
  show_onscreen_msg(2*game.num_fps, "Tooltips %s", statstr);
  save_settings();
}

void draw_tooltip(void)
{
  SYNCDBG(7,"Starting");
  _DK_draw_tooltip();
}

/******************************************************************************/
#ifdef __cplusplus
}
#endif