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
 *	ROM 2.4 is copyright 1993-1996 Russ Taylor			   				   *
 *	ROM has been brought to you by the ROM consortium		   			   *
 *	    Russ Taylor (rtaylor@efn.org)									   *
 *	    Gabrielle Taylor												   *
 *	    Brian Moore (zump@rom.org)										   *
 *	By using this code, you have agreed to follow the terms of the		   *
 *	ROM license, in the file Rom24/doc/rom.license						   *
 ***************************************************************************/

#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "merc.h"
#include "clans/ClanManager.h"
#include "ConnectedState.h"
#include "NonPlayerCharacter.h"
#include "Object.h"
#include "ObjectHelper.h"
#include "PlayerCharacter.h"
#include "Portal.h"
#include "Room.h"

void raw_kill args((Character * victim));
int xp_compute args((Character * gch, Character *victim, int total_levels));

/* command procedures needed */
DECLARE_DO_FUN(do_look);
DECLARE_DO_FUN(do_stand);

extern ClanManager *clan_manager;

/* random room generation procedure */
ROOM_INDEX_DATA *get_random_room(Character *ch)
{
	ROOM_INDEX_DATA *room;

	for (;;)
	{
		room = get_room_index(number_range(0, 65535));
		if (room != NULL)
			if (can_see_room(ch, room) && !room_is_private(room) && !IS_SET(room->room_flags, ROOM_PRIVATE) && !IS_SET(room->room_flags, ROOM_SOLITARY) && !IS_SET(room->room_flags, ROOM_SAFE) && (ch->isNPC() || IS_SET(ch->act, ACT_AGGRESSIVE) || !IS_SET(room->room_flags, ROOM_LAW)))
				break;
	}

	return room;
}

/* RT Enter portals */
void do_enter(Character *ch, char *argument)
{
	if (ch->fighting != NULL)
		return;

	/* nifty portal stuff */
	if (argument[0] != '\0')
	{
		Character *fch, *fch_next;

		Object *obj = ObjectHelper::findInList(ch, argument, ch->in_room->contents);

		if (obj == NULL)
		{
			send_to_char("You don't see that here.\n\r", ch);
			return;
		}

		if (!obj->isPortal())
		{
			send_to_char("You can't seem to find a way in.\n\r", ch);
			return;
		}

		auto portal = (Portal *)obj;

		try
		{
			portal->enter(ch);
		}
		catch (PortalNotTraversableException)
		{
			send_to_char("You can't seem to find a way in.\n\r", ch);
			return;
		}
		catch (PortalUeePreventedByCurseException)
		{
			send_to_char("Something prevents you from leaving...\n\r", ch);
			return;
		}
	}
	else
	{
		send_to_char("Nope, can't do it.\n\r", ch);
		return;
	}
}

void do_detonate(Character *ch, char *argument)
{
	char buf[MAX_STRING_LENGTH];
	Object *nuke;
	Character *victim, *gch;
	DESCRIPTOR_DATA *d;
	ROOM_INDEX_DATA *room;
	int count = 0;
	int pcount = 0;
	AREA_DATA *pArea;
	int vnum;

	pArea = ch->in_room->area;

	if ((nuke = ch->getEquipment(WEAR_BOMB)) == NULL)
	{
		send_to_char("You do not have a nuclear weapon.\n\r", ch);
		return;
	}

	if (ch->isClanned())
	{
		for (d = descriptor_list; d; d = d->next)
		{
			if (d->connected == ConnectedState::Playing && (victim = d->character) != NULL && victim != ch && !IS_IMMORTAL(victim) && victim->in_room != NULL && !victim->isSameGroup( ch) && victim->in_room->area == ch->in_room->area)
				// Oh boy, lets kick some ass, as a group!
				for (gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room)
				{
					if (!gch->isSameGroup( ch) || IS_NPC(gch))
						continue;

					count++;
					pcount++;
					if (!gch->isNPC())
					{
						((PlayerCharacter *)gch)->incrementPlayerKills(1);
					}

					gch->xp += xp_compute(gch, victim, 8);

					if (!IS_IMMORTAL(victim))
						raw_kill(victim); /* dump the flags */
					if (ch != victim && !ch->isNPC() && !clan_manager->isSameClan(ch, victim))
					{
						if (IS_SET(victim->act, PLR_THIEF))
							REMOVE_BIT(victim->act, PLR_THIEF);
					}
				}
		}
	}

	// now kill the mobs
	//    for (room = get_room_index( ch->in_room->area->min_vnum ); room->vnum <= ch->in_room->area->max_vnum; room = room->next)
	for (vnum = pArea->min_vnum; vnum <= pArea->max_vnum; vnum++)
	{
		room = get_room_index(vnum);
		if (!room || !room->people)
			continue;

		for (victim = room->people; victim != NULL; victim = victim->next_in_room)
		{
			if (IS_NPC(victim) && victim->in_room != NULL && victim->in_room->area == ch->in_room->area)
				// Oh boy, lets kick some ass, as a group!
				for (gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room)
				{
					if (!gch->isSameGroup( ch) || IS_NPC(gch))
						continue;

					count++;
					gch->xp += xp_compute(gch, victim, 8);
					if (!IS_IMMORTAL(victim))
						raw_kill(victim); /* dump the flags */

					for (auto obj : gch->getCarrying())
					{
						if (obj->getWearLocation() == WEAR_NONE)
							continue;

						if (
							(obj->hasStat(ITEM_ANTI_EVIL) && IS_EVIL(gch))
							|| (obj->hasStat(ITEM_ANTI_GOOD) && IS_GOOD(gch))
							|| (obj->hasStat(ITEM_ANTI_NEUTRAL) && IS_NEUTRAL(gch)))
						{
							act("You are zapped by $p.", gch, obj, NULL, TO_CHAR, POS_RESTING);
							act("$n is zapped by $p.", gch, obj, NULL, TO_ROOM, POS_RESTING);
							obj_from_char(obj);
							obj_to_room(obj, gch->in_room);
						}
					}
				}
		}
	}

	for (gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room)
	{
		if (!gch->isSameGroup( ch) || IS_NPC(gch) || gch == ch)
			continue;

		snprintf(buf, sizeof(buf), "%d people have been killed by the bomb blast detonated by %s. (%d PC's)\n\rYou receive %d experience points, before the heat from the bomb incinerates you.\n\r", count, ch->getName().c_str(), pcount, gch->xp);
		send_to_char(buf, gch);
		gch->gain_exp(gch->xp);
		((PlayerCharacter *)gch)->incrementMobKills(count);
		((PlayerCharacter *)gch)->incrementPlayerKills(pcount);
		gch->xp = 0;
		if (!IS_IMMORTAL(gch))
			raw_kill(gch);
	}

	snprintf(buf, sizeof(buf), "%d people have been killed by the blast. (%d PC's)\n\rYou receive %d experience points, before the heat from the bomb incinerates you.\n\r", count, pcount, ch->xp);
	send_to_char(buf, ch);
	ch->gain_exp(ch->xp);
	((PlayerCharacter *)ch)->incrementMobKills(count);
	((PlayerCharacter *)ch)->incrementPlayerKills(pcount);
	ch->xp = 0;
	obj_from_char(nuke);
	if (!IS_IMMORTAL(ch))
		raw_kill(ch);

	for (d = descriptor_list; d != NULL; d = d->next)
	{
		if (d->connected == ConnectedState::Playing)
			send_to_char("You see a mushroom-shaped cloud appear on the horizon.\n\r", d->character);
	}
}
