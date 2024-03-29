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
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "merc.h"
#include "clans/ClanManager.h"
#include "magic.h"
#include "Object.h"
#include "ObjectHelper.h"
#include "Room.h"


DECLARE_DO_FUN(do_scan);

extern char *target_name;
extern ClanManager * clan_manager;

void spell_farsight( int sn, int level, bool succesful_cast, Character *ch, void *vo,int target)
{
    if (IS_AFFECTED(ch,AFF_BLIND))
    {
        send_to_char("Maybe it would help if you could see?\n\r",ch);
        return;
    }
 
    do_scan(ch,target_name);
}


void spell_portal( int sn, int level, bool succesful_cast, Character *ch, void *vo,int target)
{
    Character *victim;
    Object *portal, *stone;

        if ( ( victim = get_char_world( ch, target_name ) ) == NULL
    ||   victim == ch
    ||   victim->in_room == NULL
    ||   !can_see_room(ch,victim->in_room)
    ||   IS_SET(victim->in_room->room_flags, ROOM_SAFE)
    ||   IS_SET(victim->in_room->room_flags, ROOM_PRIVATE)
    ||   IS_SET(victim->in_room->room_flags, ROOM_SOLITARY)
    ||   IS_SET(victim->in_room->room_flags, ROOM_NO_RECALL)
    ||   IS_SET(ch->in_room->room_flags, ROOM_NO_RECALL)
    ||   victim->level >= level + 3
    ||   (!IS_NPC(victim) && victim->level >= LEVEL_HERO)  /* NOT trust */
    ||   (IS_NPC(victim) && IS_SET(victim->imm_flags,IMM_SUMMON))
    ||   (IS_NPC(victim) && saves_spell( level, victim,DAM_NONE) ) 
    ||	(victim->isClanned() && !clan_manager->isSameClan(ch,victim)))
    {
        send_to_char( "You failed.\n\r", ch );
        return;
    }   

    stone = ch->getEquipment(WEAR_HOLD);
    if (!IS_IMMORTAL(ch) 
    &&  (stone == NULL || stone->getItemType() != ITEM_WARP_STONE))
    {
	send_to_char("You lack the proper component for this spell.\n\r",ch);
	return;
    }

    if (stone != NULL && stone->getItemType() == ITEM_WARP_STONE)
    {
     	act("You draw upon the power of $p.",ch,stone,NULL,TO_CHAR, POS_RESTING );
     	act("It flares brightly and vanishes!",ch,stone,NULL,TO_CHAR, POS_RESTING );
     	extract_obj(stone);
    }

    portal = ObjectHelper::createFromIndex(get_obj_index(OBJ_VNUM_PORTAL),0);
    portal->setTimer( 2 + level / 25 );
    portal->getValues().at(3) = victim->in_room->vnum;

    obj_to_room(portal,ch->in_room);

    act("$p rises up from the ground.",ch,portal,NULL,TO_ROOM, POS_RESTING );
    act("$p rises up before you.",ch,portal,NULL,TO_CHAR, POS_RESTING );
}

void spell_nexus( int sn, int level, bool succesful_cast, Character *ch, void *vo, int target)
{
    Character *victim;
    Object *portal, *stone;
    ROOM_INDEX_DATA *to_room, *from_room;

    from_room = ch->in_room;
 
        if ( ( victim = get_char_world( ch, target_name ) ) == NULL
    ||   victim == ch
    ||   (to_room = victim->in_room) == NULL
    ||   !can_see_room(ch,to_room) || !can_see_room(ch,from_room)
    ||   IS_SET(to_room->room_flags, ROOM_SAFE)
    ||	 IS_SET(from_room->room_flags,ROOM_SAFE)
    ||   IS_SET(to_room->room_flags, ROOM_PRIVATE)
    ||   IS_SET(to_room->room_flags, ROOM_SOLITARY)
    ||   IS_SET(to_room->room_flags, ROOM_NO_RECALL)
    ||   IS_SET(from_room->room_flags,ROOM_NO_RECALL)
    ||   victim->level >= level + 3
    ||   (!IS_NPC(victim) && victim->level >= LEVEL_HERO)  /* NOT trust */
    ||   (IS_NPC(victim) && IS_SET(victim->imm_flags,IMM_SUMMON))
    ||   (IS_NPC(victim) && saves_spell( level, victim,DAM_NONE) ) 
    ||	 (victim->isClanned() && !clan_manager->isSameClan(ch,victim)))
    {
        send_to_char( "You failed.\n\r", ch );
        return;
    }   
 
    stone = ch->getEquipment(WEAR_HOLD);
    if (!IS_IMMORTAL(ch)
    &&  (stone == NULL || stone->getItemType() != ITEM_WARP_STONE))
    {
        send_to_char("You lack the proper component for this spell.\n\r",ch);
        return;
    }
 
    if (stone != NULL && stone->getItemType() == ITEM_WARP_STONE)
    {
        act("You draw upon the power of $p.",ch,stone,NULL,TO_CHAR, POS_RESTING );
        act("It flares brightly and vanishes!",ch,stone,NULL,TO_CHAR, POS_RESTING );
        extract_obj(stone);
    }

    /* portal one */ 
    portal = ObjectHelper::createFromIndex(get_obj_index(OBJ_VNUM_PORTAL),0);
    portal->setTimer( 1 + level / 10 );
    portal->getValues().at(3) = to_room->vnum;
 
    obj_to_room(portal,from_room);
 
    act("$p rises up from the ground.",ch,portal,NULL,TO_ROOM, POS_RESTING );
    act("$p rises up before you.",ch,portal,NULL,TO_CHAR, POS_RESTING );

    /* no second portal if rooms are the same */
    if (to_room == from_room)
	return;

    /* portal two */
    portal = ObjectHelper::createFromIndex(get_obj_index(OBJ_VNUM_PORTAL),0);
    portal->setTimer( 1 + level/10 );
    portal->getValues().at(3) = from_room->vnum;

    obj_to_room(portal,to_room);

    if (to_room->people != NULL)
    {
	act("$p rises up from the ground.",to_room->people,portal,NULL,TO_ROOM, POS_RESTING );
	act("$p rises up from the ground.",to_room->people,portal,NULL,TO_CHAR, POS_RESTING );
    }
}
