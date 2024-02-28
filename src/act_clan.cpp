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

/***************************************************************************
*	This file contains clan functions by Blizzard for		   *
*	Forgotten Legends MUD						   *
***************************************************************************/

#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "merc.h"
#include "tables.h"
#include "PlayerCharacter.h"

DECLARE_DO_FUN( do_clist	);

void do_join( Character *caller, char *argument )
{
	if ( caller->isNPC() ) {
		send_to_char("NPCs may not join clans\n\r", caller);
		return;
	}

	PlayerCharacter *ch = (PlayerCharacter*)caller;
    char buf[MAX_STRING_LENGTH];

    if (argument[0] == '\0')
    {
	send_to_char("Clans Available for you to Join:\n\r",ch);
	send_to_char("================================\n\r",ch);

	do_clist(ch, (char*)"");

	send_to_char("\n\r",ch);
	return;
    }

/* Outcasts can join clans too :D
    if (ch->pcdata->clan == get_clan("outcast"))
    {
	send_to_char("You are outcasted from any clans and must bow your head in shame.\n\r",ch);
	return;
    }
*/

	CLAN_DATA *join = get_clan(argument);
    if (!join)
    {
	snprintf(buf, sizeof(buf), "%s: that clan does not exist.\n\r",argument);
	send_to_char(buf,ch);
	return;
    }

    ch->setJoin(join);
    snprintf(buf, sizeof(buf), "You are now eligable to join %s.\n\r",join->name);
    send_to_char(buf,ch);
    return;
}

void do_cedit( Character *caller, char *argument )
{
	if ( caller->isNPC() ) {
		send_to_char("NPCs may not join clans\n\r", caller);
		return;
	}

	PlayerCharacter *ch = (PlayerCharacter*)caller;
    if (!IS_CLANNED(ch) || str_cmp(ch->getName(),ch->pcdata->clan->leader))
    {
	send_to_char("Only clan leaders may use this command.",ch);
	return;
    }
    else
    {
		    send_to_char("Clan Customization Menu:\n\r"
		    "1: Admin Options (leader only)\n\r"
		    "2: Room Options\n\r"
		    "3: Mobile Options\n\r"
		    "4: Shop Options\n\r"
		    "5: Exit Editor\n\r", ch);
	ch->setClanCust(3);
	ch->desc->connected = CON_CLAN_CREATE;
	return;
    }
}
