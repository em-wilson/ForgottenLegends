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

#if defined(macintosh)
#include <types.h>
#include <time.h>
#else
#include <sys/types.h>
#include <sys/time.h>
#endif
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "merc.h"

void    raw_kill        args( ( Character *victim ) );
int     xp_compute      args( ( Character *gch, Character *victim, int
total_levels ) );

/* command procedures needed */
DECLARE_DO_FUN(do_look		);
DECLARE_DO_FUN(do_stand		);

/* random room generation procedure */
ROOM_INDEX_DATA  *get_random_room(Character *ch)
{
    ROOM_INDEX_DATA *room;

    for ( ; ; )
    {
        room = get_room_index( number_range( 0, 65535 ) );
        if ( room != NULL )
        if ( can_see_room(ch,room)
	&&   !room_is_private(room)
        &&   !IS_SET(room->room_flags, ROOM_PRIVATE)
        &&   !IS_SET(room->room_flags, ROOM_SOLITARY) 
	&&   !IS_SET(room->room_flags, ROOM_SAFE) 
	&&   (IS_NPC(ch) || IS_SET(ch->act,ACT_AGGRESSIVE) 
	||   !IS_SET(room->room_flags,ROOM_LAW)))
            break;
    }

    return room;
}

/* RT Enter portals */
void do_enter( Character *ch, char *argument)
{    
    ROOM_INDEX_DATA *location; 

    if ( ch->fighting != NULL ) 
	return;

    /* nifty portal stuff */
    if (argument[0] != '\0')
    {
        ROOM_INDEX_DATA *old_room;
	OBJ_DATA *portal;
	Character *fch, *fch_next;

        old_room = ch->in_room;

	portal = get_obj_list( ch, argument,  ch->in_room->contents );
	
	if (portal == NULL)
	{
	    send_to_char("You don't see that here.\n\r",ch);
	    return;
	}

	if (portal->item_type != ITEM_PORTAL 
        ||  (IS_SET(portal->value[1],EX_CLOSED) && !IS_TRUSTED(ch,ANGEL)))
	{
	    send_to_char("You can't seem to find a way in.\n\r",ch);
	    return;
	}

	if (!IS_TRUSTED(ch,ANGEL) && !IS_SET(portal->value[2],GATE_NOCURSE)
	&&  (IS_AFFECTED(ch,AFF_CURSE) 
	||   IS_SET(old_room->room_flags,ROOM_NO_RECALL)))
	{
	    send_to_char("Something prevents you from leaving...\n\r",ch);
	    return;
	}

	if (IS_SET(portal->value[2],GATE_RANDOM) || portal->value[3] == -1)
	{
	    location = get_random_room(ch);
	    portal->value[3] = location->vnum; /* for record keeping :) */
	}
	else if (IS_SET(portal->value[2],GATE_BUGGY) && (number_percent() < 5))
	    location = get_random_room(ch);
	else
	    location = get_room_index(portal->value[3]);

	if (location == NULL
	||  location == old_room
	||  !can_see_room(ch,location) 
	||  (room_is_private(location) && !IS_TRUSTED(ch,IMPLEMENTOR)))
	{
	   act("$p doesn't seem to go anywhere.",ch,portal,NULL,TO_CHAR);
	   return;
	}

        if (IS_NPC(ch) && IS_SET(ch->act,ACT_AGGRESSIVE)
        &&  IS_SET(location->room_flags,ROOM_LAW))
        {
            send_to_char("Something prevents you from leaving...\n\r",ch);
            return;
        }

	act("$n steps into $p.",ch,portal,NULL,TO_ROOM);
	
	if (IS_SET(portal->value[2],GATE_NORMAL_EXIT))
	    act("You enter $p.",ch,portal,NULL,TO_CHAR);
	else
	    act("You walk through $p and find yourself somewhere else...",
	        ch,portal,NULL,TO_CHAR); 

	char_from_room(ch);
	char_to_room(ch, location);

	if (IS_SET(portal->value[2],GATE_GOWITH)) /* take the gate along */
	{
	    obj_from_room(portal);
	    obj_to_room(portal,location);
	}

	if (IS_SET(portal->value[2],GATE_NORMAL_EXIT))
	    act("$n has arrived.",ch,portal,NULL,TO_ROOM);
	else
	    act("$n has arrived through $p.",ch,portal,NULL,TO_ROOM);

	do_look(ch,(char*)"auto");

	/* charges */
	if (portal->value[0] > 0)
	{
	    portal->value[0]--;
	    if (portal->value[0] == 0)
		portal->value[0] = -1;
	}

	/* protect against circular follows */
	if (old_room == location)
	    return;

    	for ( fch = old_room->people; fch != NULL; fch = fch_next )
    	{
            fch_next = fch->next_in_room;

            if (portal == NULL || portal->value[0] == -1) 
	    /* no following through dead portals */
                continue;
 
            if ( fch->master == ch && IS_AFFECTED(fch,AFF_CHARM)
            &&   fch->position < POS_STANDING)
            	do_stand(fch,(char*)"");

            if ( fch->master == ch && fch->position == POS_STANDING)
            {
 
                if (IS_SET(ch->in_room->room_flags,ROOM_LAW)
                &&  (IS_NPC(fch) && IS_SET(fch->act,ACT_AGGRESSIVE)))
                {
                    act("You can't bring $N into the city.",
                    	ch,NULL,fch,TO_CHAR);
                    act("You aren't allowed in the city.",
                    	fch,NULL,NULL,TO_CHAR);
                    continue;
            	}
 
            	act( "You follow $N.", fch, NULL, ch, TO_CHAR );
		do_enter(fch,argument);
            }
    	}

 	if (portal != NULL && portal->value[0] == -1)
	{
	    act("$p fades out of existence.",ch,portal,NULL,TO_CHAR);
	    if (ch->in_room == old_room)
		act("$p fades out of existence.",ch,portal,NULL,TO_ROOM);
	    else if (old_room->people != NULL)
	    {
		act("$p fades out of existence.", 
		    old_room->people,portal,NULL,TO_CHAR);
		act("$p fades out of existence.",
		    old_room->people,portal,NULL,TO_ROOM);
	    }
	    extract_obj(portal);
	}

	/* 
	 * If someone is following the char, these triggers get activated
	 * for the followers before the char, but it's safer this way...
	 */
	if ( IS_NPC( ch ) && HAS_TRIGGER( ch, TRIG_ENTRY ) )
	    mp_percent_trigger( ch, NULL, NULL, NULL, TRIG_ENTRY );
	if ( !IS_NPC( ch ) )
	    mp_greet_trigger( ch );

	return;
    }

    send_to_char("Nope, can't do it.\n\r",ch);
    return;
}

void do_detonate( Character *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    OBJ_DATA *nuke;
    Character *victim, *gch;
    DESCRIPTOR_DATA *d;
    ROOM_INDEX_DATA *room;
    int count = 0;
    int pcount = 0;
    AREA_DATA *pArea;
    int vnum;

    pArea = ch->in_room->area;

    if ( ( nuke = get_eq_char(ch,WEAR_BOMB)) == NULL)
    {
	send_to_char("You do not have a nuclear weapon.\n\r",ch);
        return;
    }

    if (IS_CLANNED(ch))
    {
	for ( d = descriptor_list; d; d = d->next )
	{
	    if ( d->connected == CON_PLAYING
	    && ( victim = d->character ) != NULL
            && victim != ch
	    &&   !IS_IMMORTAL(victim)
	    &&   victim->in_room != NULL
	    && !is_same_group(victim, ch)
	    &&   victim->in_room->area == ch->in_room->area )
	    // Oh boy, lets kick some ass, as a group!
	    for ( gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room )
	    {
	        if ( !is_same_group( gch, ch ) || IS_NPC(gch))
	            continue;
	
		count++;
		pcount++;
		gch->pkills++;

	        gch->xp += xp_compute( gch, victim, 8 );
	
		if (!IS_IMMORTAL(victim))
		raw_kill( victim );        /* dump the flags */
	        if (ch != victim && !IS_NPC(ch) && !is_same_clan(ch,victim))
	        {
	            if (IS_SET(victim->act,PLR_THIEF))
	                REMOVE_BIT(victim->act,PLR_THIEF);
	        }
	    }
       }
    }

    // now kill the mobs
//    for (room = get_room_index( ch->in_room->area->min_vnum ); room->vnum <= ch->in_room->area->max_vnum; room = room->next)
    for ( vnum = pArea->min_vnum; vnum <= pArea->max_vnum; vnum++ )
    {
	room = get_room_index( vnum );
	if (!room || !room->people)
	    continue;

       for (victim = room->people; victim != NULL; victim = victim->next_in_room)
       {
	    if ( IS_NPC(victim)
	    &&   victim->in_room != NULL
	    &&   victim->in_room->area == ch->in_room->area )
	    // Oh boy, lets kick some ass, as a group!
	    for ( gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room )
	    {
	        OBJ_DATA *obj;
	        OBJ_DATA *obj_next;
	
	        if ( !is_same_group( gch, ch ) || IS_NPC(gch))
	            continue;
	
		count++;
	        gch->xp += xp_compute( gch, victim, 8 );
		if (!IS_IMMORTAL(victim))
		raw_kill( victim );        /* dump the flags */

	
	        for ( obj = gch->carrying; obj != NULL; obj = obj_next )
	        {
	            obj_next = obj->next_content;
	            if ( obj->wear_loc == WEAR_NONE )
	                continue;
	
	            if ( ( IS_OBJ_STAT(obj, ITEM_ANTI_EVIL)    && IS_EVIL(gch) )
	            ||   ( IS_OBJ_STAT(obj, ITEM_ANTI_GOOD)    && IS_GOOD(gch) )
	            ||   ( IS_OBJ_STAT(obj, ITEM_ANTI_NEUTRAL) && IS_NEUTRAL(gch) ) )
	            {
	                act( "You are zapped by $p.", gch, obj, NULL, TO_CHAR );
	                act( "$n is zapped by $p.",   gch, obj, NULL, TO_ROOM );
	                obj_from_char( obj );
	                obj_to_room( obj, gch->in_room );
	            }
	        }
	    }
       }
    }

    for ( gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room )
    {
        if ( !is_same_group( gch, ch ) || IS_NPC(gch) || gch == ch)
            continue;

        sprintf( buf, "%d people have been killed by the bomb blast detonated by %s. (%d PC's)\n\rYou receive %d experience points, before the heat from the bomb incinerates you.\n\r", count, ch->getName(), pcount, gch->xp);
        send_to_char( buf, gch );
        gch->gain_exp( gch->xp );
	gch->mkills += count;
	gch->pkills += pcount;
        gch->xp = 0;
	if (!IS_IMMORTAL(gch))
        raw_kill( gch );
    }

    sprintf( buf, "%d people have been killed by the blast. (%d PC's)\n\rYou receive %d experience points, before the heat from the bomb incinerates you.\n\r", count, pcount, ch->xp );
    send_to_char( buf, ch );
    ch->gain_exp( ch->xp );
    ch->mkills += count;
    ch->pkills += pcount;
    ch->xp = 0;
    obj_from_char( nuke );
    if (!IS_IMMORTAL(ch))
    raw_kill( ch );

      for ( d = descriptor_list; d != NULL; d = d->next )
      {
	if (d->connected == CON_PLAYING)
    send_to_char("You see a mushroom-shaped cloud appear on the horizon.\n\r",d->character);
      }
}
