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
 *	ROM 2.4 is copyright 1993-1996 Russ Taylor							   *
 *	ROM has been brought to you by the ROM consortium					   *
 *	    Russ Taylor (rtaylor@efn.org)									   *
 *	    Gabrielle Taylor												   *
 *	    Brian Moore (zump@rom.org)										   *
 *	By using this code, you have agreed to follow the terms of the		   *
 *	ROM license, in the file Rom24/doc/rom.license						   *
 ***************************************************************************/

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "merc.h"
#include "Object.h"
#include "recycle.h"
#include "Room.h"

/* local functions */
void acid_effect(Object *obj, int level, int dam);
void cold_effect(Object *obj, int level, int dam);
void fire_effect(Object *obj, int level, int dam);
void poison_effect(Object *obj, int level, int dam);
void shock_effect(Object *obj, int level, int dam);

void acid_effect(ROOM_INDEX_DATA * room, int level, int dam)
{
	for (auto obj : room->contents)
	{
		acid_effect(obj, level, dam);
	}
	return;
}

void acid_effect(Character *victim, int level, int dam)
{
	/* let's toast some gear */
	for (auto obj : victim->getCarrying())
	{
		acid_effect(obj, level, dam);
	}
	return;
}


void acid_effect(Object *obj, int level, int dam)
{
	int chance;
	char *msg;

	if (obj->hasStat(ITEM_BURN_PROOF) || obj->hasStat(ITEM_NOPURGE) || number_range(0, 4) == 0)
		return;

	chance = level / 4 + dam / 10;

	if (chance > 25)
		chance = (chance - 25) / 2 + 25;
	if (chance > 50)
		chance = (chance - 50) / 2 + 50;

	if (obj->hasStat(ITEM_BLESS))
		chance -= 5;

	chance -= obj->getLevel() * 2;

	switch (obj->getItemType())
	{
	default:
		return;
	case ITEM_CONTAINER:
	case ITEM_CORPSE_PC:
	case ITEM_CORPSE_NPC:
		msg = (char *)"$p fumes and dissolves.";
		break;
	case ITEM_ARMOR:
		msg = (char *)"$p is pitted and etched.";
		break;
	case ITEM_CLOTHING:
		msg = (char *)"$p is corroded into scrap.";
		break;
	case ITEM_STAFF:
	case ITEM_WAND:
		chance -= 10;
		msg = (char *)"$p corrodes and breaks.";
		break;
	case ITEM_SCROLL:
		chance += 10;
		msg = (char *)"$p is burned into waste.";
		break;
	}

	chance = URANGE(5, chance, 95);

	if (number_percent() > chance)
		return;

	if (obj->getCarriedBy() != NULL)
		act(msg, obj->getCarriedBy(), obj, NULL, TO_ALL, POS_RESTING);
	else if (obj->getInRoom() != NULL && obj->getInRoom()->people != NULL)
		act(msg, obj->getInRoom()->people, obj, NULL, TO_ALL, POS_RESTING);

	if (obj->getItemType() == ITEM_ARMOR) /* etch it */
	{
		bool af_found = FALSE;
		int i;

		affect_enchant(obj);

		for (auto paf : obj->getAffectedBy())
		{
			if (paf->location == APPLY_AC)
			{
				af_found = TRUE;
				paf->type = -1;
				paf->modifier += 1;
				paf->level = UMAX(paf->level, level);
				break;
			}
		}

		if (!af_found)
		/* needs a new affect */
		{
			AFFECT_DATA *paf = new_affect();

			paf->type = -1;
			paf->level = level;
			paf->duration = -1;
			paf->location = APPLY_AC;
			paf->modifier = 1;
			paf->bitvector = 0;
			obj->addAffect(paf);
		}

		if (obj->getCarriedBy() != NULL && obj->getWearLocation() != WEAR_NONE)
			for (i = 0; i < 4; i++)
				obj->getCarriedBy()->armor[i] += 1;
		return;
	}

	/* get rid of the object */
	for (auto t_obj : obj->getContents())
	{
		obj_from_obj(t_obj);
		if (obj->getInRoom() != NULL)
			obj_to_room(t_obj, obj->getInRoom());
		else if (obj->getCarriedBy() != NULL)
			obj_to_room(t_obj, obj->getCarriedBy()->in_room);
		else
		{
			extract_obj(t_obj);
			continue;
		}

		acid_effect(t_obj, level / 2, dam / 2);
	}

	extract_obj(obj);
	return;
}

void cold_effect(ROOM_INDEX_DATA *room, int level, int dam)
{
	for (auto obj : room->contents)
	{
		cold_effect(obj, level, dam);
	}
	return;
}

void cold_effect(Character *victim, int level, int dam)
{
	/* chill touch effect */
	if (!saves_spell(level / 4 + dam / 20, victim, DAM_COLD))
	{
		AFFECT_DATA af;

		act("$n turns blue and shivers.", victim, NULL, NULL, TO_ROOM, POS_RESTING);
		act("A chill sinks deep into your bones.", victim, NULL, NULL, TO_CHAR, POS_RESTING);
		af.where = TO_AFFECTS;
		af.type = skill_lookup("chill touch");
		af.level = level;
		af.duration = 6;
		af.location = APPLY_STR;
		af.modifier = -1;
		af.bitvector = 0;
		affect_join(victim, &af);
	}

	/* hunger! (warmth sucked out */
	if (!IS_NPC(victim))
		victim->gain_condition(COND_HUNGER, dam / 20);

	/* let's toast some gear */
	for (auto obj : victim->getCarrying())
	{
		cold_effect(obj, level, dam);
	}
	return;
}

void cold_effect(Object *obj, int level, int dam)
{
	int chance;
	char *msg;

	if (obj->hasStat(ITEM_BURN_PROOF) || obj->hasStat(ITEM_NOPURGE) || number_range(0, 4) == 0)
		return;

	chance = level / 4 + dam / 10;

	if (chance > 25)
		chance = (chance - 25) / 2 + 25;
	if (chance > 50)
		chance = (chance - 50) / 2 + 50;

	if (obj->hasStat(ITEM_BLESS))
		chance -= 5;

	chance -= obj->getLevel() * 2;

	switch (obj->getItemType())
	{
	default:
		return;
	case ITEM_POTION:
		msg = (char *)"$p freezes and shatters!";
		chance += 25;
		break;
	case ITEM_DRINK_CON:
		msg = (char *)"$p freezes and shatters!";
		chance += 5;
		break;
	}

	chance = URANGE(5, chance, 95);

	if (number_percent() > chance)
		return;

	if (obj->getCarriedBy() != NULL)
		act(msg, obj->getCarriedBy(), obj, NULL, TO_ALL, POS_RESTING);
	else if (obj->getInRoom() != NULL && obj->getInRoom()->people != NULL)
		act(msg, obj->getInRoom()->people, obj, NULL, TO_ALL, POS_RESTING);

	extract_obj(obj);
	return;
}

void fire_effect(ROOM_INDEX_DATA *room, int level, int dam)
{
	for (auto obj : room->contents)
	{
		fire_effect(obj, level, dam);
	}
	return;
}

void fire_effect(Character *victim, int level, int dam)
{
	/* chance of blindness */
	if (!IS_AFFECTED(victim, AFF_BLIND) && !saves_spell(level / 4 + dam / 20, victim, DAM_FIRE))
	{
		AFFECT_DATA af;
		act("$n is blinded by smoke!", victim, NULL, NULL, TO_ROOM, POS_RESTING);
		act("Your eyes tear up from smoke...you can't see a thing!",
			victim, NULL, NULL, TO_CHAR, POS_RESTING);

		af.where = TO_AFFECTS;
		af.type = skill_lookup("fire breath");
		af.level = level;
		af.duration = number_range(0, level / 10);
		af.location = APPLY_HITROLL;
		af.modifier = -4;
		af.bitvector = AFF_BLIND;

		victim->giveAffect( &af);
	}

	/* getting thirsty */
	if (!IS_NPC(victim))
		victim->gain_condition(COND_THIRST, dam / 20);

	/* let's toast some gear! */
	for (auto obj : victim->getCarrying())
	{
		fire_effect(obj, level, dam);
	}
	return;
}

void fire_effect(Object *obj, int level, int dam)
{
	int chance;
	char *msg;

	if (obj->hasStat(ITEM_BURN_PROOF) || obj->hasStat(ITEM_NOPURGE) || number_range(0, 4) == 0)
		return;

	chance = level / 4 + dam / 10;

	if (chance > 25)
		chance = (chance - 25) / 2 + 25;
	if (chance > 50)
		chance = (chance - 50) / 2 + 50;

	if (obj->hasStat(ITEM_BLESS))
		chance -= 5;
	chance -= obj->getLevel() * 2;

	switch (obj->getItemType())
	{
	default:
		return;
	case ITEM_CONTAINER:
		msg = (char *)"$p ignites and burns!";
		break;
	case ITEM_POTION:
		chance += 25;
		msg = (char *)"$p bubbles and boils!";
		break;
	case ITEM_SCROLL:
		chance += 50;
		msg = (char *)"$p crackles and burns!";
		break;
	case ITEM_STAFF:
		chance += 10;
		msg = (char *)"$p smokes and chars!";
		break;
	case ITEM_WAND:
		msg = (char *)"$p sparks and sputters!";
		break;
	case ITEM_FOOD:
		msg = (char *)"$p blackens and crisps!";
		break;
	case ITEM_PILL:
		msg = (char *)"$p melts and drips!";
		break;
	}

	chance = URANGE(5, chance, 95);

	if (number_percent() > chance)
		return;

	if (obj->getCarriedBy() != NULL)
		act(msg, obj->getCarriedBy(), obj, NULL, TO_ALL, POS_RESTING);
	else if (obj->getInRoom() != NULL && obj->getInRoom()->people != NULL)
		act(msg, obj->getInRoom()->people, obj, NULL, TO_ALL, POS_RESTING);

	/* dump the contents */
	for (auto t_obj : obj->getContents())
	{
		obj_from_obj(t_obj);
		if (obj->getInRoom() != NULL)
			obj_to_room(t_obj, obj->getInRoom());
		else if (obj->getCarriedBy() != NULL)
			obj_to_room(t_obj, obj->getCarriedBy()->in_room);
		else
		{
			extract_obj(t_obj);
			continue;
		}
		fire_effect(t_obj, level / 2, dam / 2);
	}

	extract_obj(obj);
	return;
}

void poison_effect(ROOM_INDEX_DATA *room, int level, int dam)
{
	for (auto obj : room->contents)
	{
		poison_effect(obj, level, dam);
	}
	return;
}

void poison_effect(Character *victim, int level, int dam)
{
	/* chance of poisoning */
	if (!saves_spell(level / 4 + dam / 20, victim, DAM_POISON))
	{
		AFFECT_DATA af;

		send_to_char("You feel poison coursing through your veins.\n\r",
						victim);
		act("$n looks very ill.", victim, NULL, NULL, TO_ROOM, POS_RESTING);

		af.where = TO_AFFECTS;
		af.type = gsn_poison;
		af.level = level;
		af.duration = level / 2;
		af.location = APPLY_STR;
		af.modifier = -1;
		af.bitvector = AFF_POISON;
		affect_join(victim, &af);
	}

	/* equipment */
	for (auto obj : victim->getCarrying())
	{
		poison_effect(obj, level, dam);
	}
}

void poison_effect(Object *obj, int level, int dam)
{
	int chance;

	if (obj->hasStat(ITEM_BURN_PROOF) || obj->hasStat(ITEM_BLESS) || number_range(0, 4) == 0)
		return;

	chance = level / 4 + dam / 10;
	if (chance > 25)
		chance = (chance - 25) / 2 + 25;
	if (chance > 50)
		chance = (chance - 50) / 2 + 50;

	chance -= obj->getLevel() * 2;

	switch (obj->getItemType())
	{
	default:
		return;
	case ITEM_FOOD:
		break;
	case ITEM_DRINK_CON:
		if (obj->getValues().at(0) == obj->getValues().at(1))
			return;
		break;
	}

	chance = URANGE(5, chance, 95);

	if (number_percent() > chance)
		return;

	obj->getValues().at(3) = 1;
}

void shock_effect(ROOM_INDEX_DATA *room, int level, int dam)
{
	for (auto obj : room->contents)
	{
		shock_effect(obj, level, dam);
	}
}

void shock_effect(Character *victim, int level, int dam)
{
	/* daze and confused? */
	if (!saves_spell(level / 4 + dam / 20, victim, DAM_LIGHTNING))
	{
		send_to_char("Your muscles stop responding.\n\r", victim);
		DAZE_STATE(victim, UMAX(12, level / 4 + dam / 20));
	}

	/* toast some gear */
	for (auto obj : victim->getCarrying())
	{
		shock_effect(obj, level, dam);
	}
}

void shock_effect(Object *obj, int level, int dam)
{
	int chance;
	char *msg;

	if (obj->hasStat(ITEM_BURN_PROOF) || obj->hasStat(ITEM_NOPURGE) || number_range(0, 4) == 0)
		return;

	chance = level / 4 + dam / 10;

	if (chance > 25)
		chance = (chance - 25) / 2 + 25;
	if (chance > 50)
		chance = (chance - 50) / 2 + 50;

	if (obj->hasStat(ITEM_BLESS))
		chance -= 5;

	chance -= obj->getLevel() * 2;

	switch (obj->getItemType())
	{
	default:
		return;
	case ITEM_WAND:
	case ITEM_STAFF:
		chance += 10;
		msg = (char *)"$p overloads and explodes!";
		break;
	case ITEM_JEWELRY:
		chance -= 10;
		msg = (char *)"$p is fused into a worthless lump.";
	}

	chance = URANGE(5, chance, 95);

	if (number_percent() > chance)
		return;

	if (obj->getCarriedBy() != NULL)
		act(msg, obj->getCarriedBy(), obj, NULL, TO_ALL, POS_RESTING);
	else if (obj->getInRoom() != NULL && obj->getInRoom()->people != NULL)
		act(msg, obj->getInRoom()->people, obj, NULL, TO_ALL, POS_RESTING);

	extract_obj(obj);
}