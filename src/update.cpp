/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
 *  Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
 *                                                                         *
 *  Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael          *
 *  Chastain, Michael Quan, and Mitchell Tse.                              *
 *                                                                         *
 *  In order to use any part of this Merc Diku Mud, you must comply with   *
 *  both the original Diku license in 'license.doc' as well the Merc       *
 *  license in 'license.txt'.  In particular, you may not remove either of *
 *  these copyright notices.                                               *
 *                                                                         *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 ***************************************************************************/

/***************************************************************************
 *	ROM 2.4 is copyright 1993-1996 Russ Taylor			   *
 *	ROM has been brought to you by the ROM consortium		   *
 *	    Russ Taylor (rtaylor@efn.org)				   *
 *	    Gabrielle Taylor						   *
 *	    Brian Moore (zump@rom.org)					   *
 *	By using this code, you have agreed to follow the terms of the	   *
 *	ROM license, in the file Rom24/doc/rom.license			   *
 ***************************************************************************/

#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <vector>
#include <fstream>
#include "merc.h"
#include "ExitFlag.h"
#include "music.h"
#include "tables.h"
#include "recycle.h"
#include "ConnectedState.h"
#include "NonPlayerCharacter.h"
#include "Object.h"
#include "Room.h"
#include "Wiznet.h"

/* command procedures needed */
DECLARE_DO_FUN(do_quit);
DECLARE_DO_FUN(do_mobhunt);

/*
 * Local functions.
 */
void mobile_update args((void));
void weather_update args((void));
void char_update args((void));
void obj_update args((void));
void aggr_update args((void));
void info_update args((void));

/* used for saving */

int save_number = 0;

/*
 * Mob autonomous action.
 * This function takes 25% to 35% of ALL Merc cpu time.
 * -- Furey
 */
void mobile_update(void)
{
	Character *ch;
	EXIT_DATA *pexit;
	int door;

	/* Examine all mobs. */
	for (std::list<Character *>::iterator it = char_list.begin(); it != char_list.end(); it++)
	{
		ch = *it;

		if (!IS_NPC(ch) || ch->in_room == NULL || IS_AFFECTED(ch, AFF_CHARM))
			continue;

		if (ch->in_room->area->empty && !IS_SET(ch->act, ACT_UPDATE_ALWAYS))
			continue;

		/* Examine call for special procedure */
		if (ch->spec_fun != 0)
		{
			if ((*ch->spec_fun)(ch))
				continue;
		}

		if (ch->pIndexData->pShop != NULL) /* give him some gold */
			if ((ch->gold * 100 + ch->silver) < ch->pIndexData->wealth)
			{
				ch->gold += ch->pIndexData->wealth * number_range(1, 20) / 5000000;
				ch->silver += ch->pIndexData->wealth * number_range(1, 20) / 50000;
			}

		/*
		 * Check triggers only if mobile still in default position
		 */
		if (ch->position == ch->pIndexData->default_pos)
		{
			/* Delay */
			if (HAS_TRIGGER(ch, TRIG_DELAY) && ch->mprog_delay > 0)
			{
				if (--ch->mprog_delay <= 0)
				{
					mp_percent_trigger(ch, NULL, NULL, NULL, TRIG_DELAY);
					continue;
				}
			}
			if (HAS_TRIGGER(ch, TRIG_RANDOM))
			{
				if (mp_percent_trigger(ch, NULL, NULL, NULL, TRIG_RANDOM))
					continue;
			}
		}

		/* That's all for sleeping / busy monster, and empty zones */
		if (ch->position != POS_STANDING)
			continue;

		/* Scavenge */
		if (IS_SET(ch->act, ACT_SCAVENGER) && !ch->in_room->contents.empty() && number_bits(6) == 0)
		{
			Object *obj;
			Object *obj_best = nullptr;
			int max;

			max = 1;
			for (auto obj : ch->in_room->contents)
			{
				if (obj->canWear(ITEM_TAKE) && can_loot(ch, obj) && obj->getCost() > max && obj->getCost() > 0)
				{
					obj_best = obj;
					max = obj->getCost();
				}
			}

			if (obj_best)
			{
				obj_from_room(obj_best);
				ch->addObject(obj_best);
				act("$n gets $p.", ch, obj_best, NULL, TO_ROOM, POS_RESTING);
			}
		}

		/* Wander */
		if (!IS_SET(ch->act, ACT_SENTINEL) && number_bits(3) == 0 && (door = number_bits(5)) <= 5 && (pexit = ch->in_room->exit[door]) != NULL && pexit->u1.to_room != NULL && !IS_SET(pexit->exit_info, ExitFlag::ExitClosed) && !IS_SET(pexit->u1.to_room->room_flags, ROOM_NO_MOB) && (!IS_SET(ch->act, ACT_STAY_AREA) || pexit->u1.to_room->area == ch->in_room->area) && (!IS_SET(ch->act, ACT_OUTDOORS) || !IS_SET(pexit->u1.to_room->room_flags, ROOM_INDOORS)) && (!IS_SET(ch->act, ACT_INDOORS) || IS_SET(pexit->u1.to_room->room_flags, ROOM_INDOORS)))
		{
			move_char(ch, door, FALSE);
		}
	}

	return;
}

/*
 * Update the weather.
 */
void weather_update(void)
{
	char buf[MAX_STRING_LENGTH];
	DESCRIPTOR_DATA *d;
	int diff;

	buf[0] = '\0';

	switch (++time_info.hour)
	{
	case 5:
		weather_info.sunlight = SUN_LIGHT;
		strcat(buf, "The day has begun.\n\r");
		break;

	case 6:
		weather_info.sunlight = SUN_RISE;
		strcat(buf, "The sun rises in the east.\n\r");
		break;

	case 19:
		weather_info.sunlight = SUN_SET;
		strcat(buf, "The sun slowly disappears in the west.\n\r");
		break;

	case 20:
		weather_info.sunlight = SUN_DARK;
		strcat(buf, "The night has begun.\n\r");
		break;

	case 24:
		time_info.hour = 0;
		time_info.day++;
		break;
	}

	if (time_info.day >= 35)
	{
		time_info.day = 0;
		time_info.month++;
	}

	if (time_info.month >= 17)
	{
		time_info.month = 0;
		time_info.year++;
	}

	/*
	 * Weather change.
	 */
	if (time_info.month >= 9 && time_info.month <= 16)
		diff = weather_info.mmhg > 985 ? -2 : 2;
	else
		diff = weather_info.mmhg > 1015 ? -2 : 2;

	weather_info.change += diff * dice(1, 4) + dice(2, 6) - dice(2, 6);
	weather_info.change = UMAX(weather_info.change, -12);
	weather_info.change = UMIN(weather_info.change, 12);

	weather_info.mmhg += weather_info.change;
	weather_info.mmhg = UMAX(weather_info.mmhg, 960);
	weather_info.mmhg = UMIN(weather_info.mmhg, 1040);

	switch (weather_info.sky)
	{
	default:
		bug("Weather_update: bad sky %d.", weather_info.sky);
		weather_info.sky = SKY_CLOUDLESS;
		break;

	case SKY_CLOUDLESS:
		if (weather_info.mmhg < 990 || (weather_info.mmhg < 1010 && number_bits(2) == 0))
		{
			strcat(buf, "The sky is getting cloudy.\n\r");
			weather_info.sky = SKY_CLOUDY;
		}
		break;

	case SKY_CLOUDY:
		if (weather_info.mmhg < 970 || (weather_info.mmhg < 990 && number_bits(2) == 0))
		{
			strcat(buf, "It starts to rain.\n\r");
			weather_info.sky = SKY_RAINING;
		}

		if (weather_info.mmhg > 1030 && number_bits(2) == 0)
		{
			strcat(buf, "The clouds disappear.\n\r");
			weather_info.sky = SKY_CLOUDLESS;
		}
		break;

	case SKY_RAINING:
		if (weather_info.mmhg < 970 && number_bits(2) == 0)
		{
			strcat(buf, "Lightning flashes in the sky.\n\r");
			weather_info.sky = SKY_LIGHTNING;
		}

		if (weather_info.mmhg > 1030 || (weather_info.mmhg > 1010 && number_bits(2) == 0))
		{
			strcat(buf, "The rain stopped.\n\r");
			weather_info.sky = SKY_CLOUDY;
		}
		break;

	case SKY_LIGHTNING:
		if (weather_info.mmhg > 1010 || (weather_info.mmhg > 990 && number_bits(2) == 0))
		{
			strcat(buf, "The lightning has stopped.\n\r");
			weather_info.sky = SKY_RAINING;
			break;
		}
		break;
	}

	if (buf[0] != '\0')
	{
		for (d = descriptor_list; d != NULL; d = d->next)
		{
			if (d->connected == ConnectedState::Playing && IS_OUTSIDE(d->character) && IS_AWAKE(d->character))
				send_to_char(buf, d->character);
		}
	}

	return;
}

/*
 * Gives helpfull tips about the MUD.
 */
void info_update(void)
{
	static bool info_initialized = false;
	static std::vector<std::string> info_text;
	if (!info_initialized)
	{
		std::string line;
		info_initialized = true;
		srand(time(NULL));

		std::ifstream infile(INFO_FILE);
		if (!infile.is_open())
		{
			return;
		}

		while (getline(infile, line))
		{
			info_text.push_back(line);
		}
	}

	if (info_text.size() == 0)
	{
		return;
	}

	int num = rand() % info_text.size();

	for (DESCRIPTOR_DATA *d = descriptor_list; d != NULL; d = d->next)
	{
		Character *victim;

		victim = d->character;

		if (d->connected == ConnectedState::Playing &&
			!IS_SET(victim->comm, COMM_NOINFO) &&
			!IS_SET(victim->comm, COMM_QUIET))
		{
			char buf[MAX_STRING_LENGTH];
			snprintf(buf, sizeof(buf), "{YINFO: {C'%s{C'{x\n\r", info_text[num].c_str());
			send_to_char(buf, victim);
		}
	}
}

/*
 * Update all chars, including mobs.
 */
void char_update(void)
{
	std::vector<Character *> quitters;

	for (std::list<Character *>::iterator it = char_list.begin(); it != char_list.end(); it++)
	{
		Character *ch = *it;
		if (ch->timer > 30)
		{
			quitters.push_back(ch);
		}
		if (ch->desc != NULL)
		{
			save_char_obj(ch);
		}
		ch->update();
	}

	for (auto it = quitters.begin(); it != quitters.end(); ++it)
	{
		do_quit(*it, (char *)"");
	}
}

/*
 * Update all objs.
 * This function is performance sensitive.
 */
void obj_update(void)
{
	for (auto obj : object_list)
	{
		AFFECT_DATA *paf, *paf_next;
		Character *rch;
		char *message;

		/* go through affects and decrement */
		auto itl = obj->getAffectedBy();
		for (auto it = itl.begin(); it != itl.end(); paf = paf_next)
		{
			paf = *it;
			paf_next = *(++it);
			if (paf->duration > 0)
			{
				paf->duration--;
				if (number_range(0, 4) == 0 && paf->level > 0)
					paf->level--; /* spell strength fades with time */
			}
			else if (paf->duration < 0)
				;
			else
			{
				if (paf_next == NULL || paf_next->type != paf->type || paf_next->duration > 0)
				{
					if (paf->type > 0 && skill_table[paf->type].msg_obj)
					{
						if (obj->getCarriedBy() != NULL)
						{
							rch = obj->getCarriedBy();
							act(skill_table[paf->type].msg_obj,
								rch, obj, NULL, TO_CHAR, POS_RESTING);
						}
						if (obj->getInRoom() != NULL && obj->getInRoom()->people != NULL)
						{
							rch = obj->getInRoom()->people;
							act(skill_table[paf->type].msg_obj,
								rch, obj, NULL, TO_ALL, POS_RESTING);
						}
					}
				}

				obj->removeAffect(paf);
			}
		}

		if (obj->getTimer() <= 0 || obj->decrementTimer() > 0)
			continue;

		switch (obj->getItemType())
		{
		default:
			message = (char *)"$p crumbles into dust.";
			break;
		case ITEM_FOUNTAIN:
			message = (char *)"$p dries up.";
			break;
		case ITEM_CORPSE_NPC:
			message = (char *)"$p decays into dust.";
			break;
		case ITEM_CORPSE_PC:
			message = (char *)"$p decays into dust.";
			break;
		case ITEM_FOOD:
			message = (char *)"$p decomposes.";
			break;
		case ITEM_POTION:
			message = (char *)"$p has evaporated from disuse.";
			break;
		case ITEM_PORTAL:
			message = (char *)"$p fades out of existence.";
			break;
		case ITEM_CONTAINER:
			if (obj->canWear(ITEM_WEAR_FLOAT))
			{
				if (!obj->getContents().empty())
					message =
						(char *)"$p flickers and vanishes, spilling its contents on the floor.";
				else
					message = (char *)"$p flickers and vanishes.";
			}
			else
				message = (char *)"$p crumbles into dust.";
			break;
		}

		if (obj->getCarriedBy() != NULL)
		{
			if (IS_NPC(obj->getCarriedBy()) && obj->getCarriedBy()->pIndexData->pShop != NULL)
				obj->getCarriedBy()->silver += obj->getCost() / 5;
			else
			{
				act(message, obj->getCarriedBy(), obj, NULL, TO_CHAR, POS_RESTING);
				if (obj->getWearLocation() == WEAR_FLOAT)
					act(message, obj->getCarriedBy(), obj, NULL, TO_ROOM, POS_RESTING);
			}
		}
		else if (obj->getInRoom() != NULL && (rch = obj->getInRoom()->people) != NULL)
		{
			if (!(obj->getInObject() && obj->getInObject()->getObjectIndexData()->vnum == OBJ_VNUM_PIT && !(obj->getInObject(), ITEM_TAKE)))
			{
				act(message, rch, obj, NULL, TO_ROOM, POS_RESTING);
				act(message, rch, obj, NULL, TO_CHAR, POS_RESTING);
			}
		}

		if ((obj->getItemType() == ITEM_CORPSE_PC || obj->getWearLocation() == WEAR_FLOAT))
		{ /* save the contents */
			for (auto t_obj : obj->getContents())
			{
				obj_from_obj(t_obj);

				if (obj->getInObject()) /* in another object */
					obj->getInObject()->addObject(t_obj);

				else if (obj->getCarriedBy()) /* carried */
				{
					if (obj->getWearLocation() == WEAR_FLOAT)
					{
						if (obj->getCarriedBy()->in_room == NULL)
							extract_obj(t_obj);
						else
							obj_to_room(t_obj, obj->getCarriedBy()->in_room);
					}
					else
						obj->getCarriedBy()->addObject(t_obj);
				}
				else if (obj->getInRoom() == NULL) /* destroy it */
					extract_obj(t_obj);

				else /* to a room */
					obj_to_room(t_obj, obj->getInRoom());
			}
		}

		extract_obj(obj);
	}

	return;
}

/*
 * Aggress.
 *
 * for each mortal PC
 *     for each mob in room
 *         aggress on some random PC
 *
 * This function takes 25% to 35% of ALL Merc cpu time.
 * Unfortunately, checking on each PC move is too tricky,
 *   because we don't the mob to just attack the first PC
 *   who leads the party into the room.
 *
 * -- Furey
 */
void aggr_update(void)
{
	Character *wch;
	Character *ch;
	Character *ch_next;
	Character *vch;
	Character *vch_next;
	Character *victim;

	for (std::list<Character *>::iterator it = char_list.begin(); it != char_list.end(); it++)
	{
		wch = *it;
		if (IS_NPC(wch) || wch->level >= LEVEL_IMMORTAL || wch->in_room == NULL || wch->in_room->area->empty)
			continue;

		for (ch = wch->in_room->people; ch != NULL; ch = ch_next)
		{
			int count;

			ch_next = ch->next_in_room;

			if (!IS_NPC(ch) || !IS_SET(ch->act, ACT_AGGRESSIVE) || IS_SET(ch->in_room->room_flags, ROOM_SAFE) || IS_AFFECTED(ch, AFF_CALM) || ch->fighting != NULL || IS_AFFECTED(ch, AFF_CHARM) || !IS_AWAKE(ch) || (IS_SET(ch->act, ACT_WIMPY) && IS_AWAKE(wch)) || !ch->can_see( wch) || number_bits(1) == 0)
				continue;

			/*
			 * Ok we have a 'wch' player character and a 'ch' npc aggressor.
			 * Now make the aggressor fight a RANDOM pc victim in the room,
			 *   giving each 'vch' an equal chance of selection.
			 */
			count = 0;
			victim = NULL;
			for (vch = wch->in_room->people; vch != NULL; vch = vch_next)
			{
				vch_next = vch->next_in_room;

				if (!IS_NPC(vch) && vch->level < LEVEL_IMMORTAL && ch->level >= vch->level - 5 && (!IS_SET(ch->act, ACT_WIMPY) || !IS_AWAKE(vch)) && ch->can_see( vch))
				{
					if (number_range(0, count) == 0)
						victim = vch;
					count++;
				}
			}

			if (victim == NULL)
				continue;

			multi_hit(ch, victim, TYPE_UNDEFINED);
		}
	}

	return;
}

/*
 * Handle all kinds of updates.
 * Called once per pulse from game loop.
 * Random times to defeat tick-timing clients and players.
 */

void update_handler(void)
{
	static int pulse_area;
	static int pulse_mobile;
	static int pulse_violence;
	static int pulse_point;
	static int pulse_music;
	static int pulse_info;

	if (--pulse_area <= 0)
	{
		pulse_area = PULSE_AREA;
		/* number_range( PULSE_AREA / 2, 3 * PULSE_AREA / 2 ); */
		area_update();
	}

	if (--pulse_music <= 0)
	{
		pulse_music = PULSE_MUSIC;
		song_update();
	}

	if (--pulse_info <= 0)
	{
		pulse_info = PULSE_INFO;
		info_update();
	}

	if (--pulse_mobile <= 0)
	{
		pulse_mobile = PULSE_MOBILE;
		mobile_update();
	}

	if (--pulse_violence <= 0)
	{
		pulse_violence = PULSE_VIOLENCE;
		violence_update();
	}

	if (--pulse_point <= 0)
	{
		Wiznet::instance()->report("TICK!", NULL, NULL, WIZ_TICKS, 0, 0);
		pulse_point = PULSE_TICK;
		/* number_range( PULSE_TICK / 2, 3 * PULSE_TICK / 2 ); */
		weather_update();
		char_update();
		obj_update();
		who_html_update();
	}

	aggr_update();
	tail_chain();
	return;
}
