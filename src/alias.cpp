/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,	   *
 *  Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
 *									   *
 *  Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael	   *
 *  Chastain, Michael Quan, and Mitchell Tse.				   *
 *									   *
 *  In order to use any part of this Merc Diku Mud, you must comply with   *
 *  both the original Diku license in 'license.doc' as well the Merc	   *
 *  license in 'license.txt'.  In particular, you may not remove either of *
 *  these copyright notices.						   *
 *									   *
 *  Much time and thought has gone into this software and you are	   *
 *  benefitting.  We hope that you share your changes too.  What goes	   *
 *  around, comes around.						   *
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
#include "merc.h"
#include "PlayerCharacter.h"

/* does aliasing and other fun stuff */
void substitute_alias(DESCRIPTOR_DATA *d, char *argument)
{
    Character *ch;
    char buf[MAX_STRING_LENGTH],name[MAX_INPUT_LENGTH];
    char *point;
    int alias;

    ch = d->original ? d->original : d->character;

    if (ch->isNPC() || ((PlayerCharacter*)ch)->alias[0] == NULL
    ||	!str_prefix("alias",argument) || !str_prefix("una",argument) 
    ||  !str_prefix("prefix",argument)) 
    {
	interpret(d->character,argument);
	return;
    }

    strcpy(buf,argument);

    for (alias = 0; alias < MAX_ALIAS; alias++)	 /* go through the aliases */
    {
	if (((PlayerCharacter*)ch)->alias[alias] == NULL)
	    break;

	if (!str_prefix(((PlayerCharacter*)ch)->alias[alias],argument))
	{
	    point = one_argument(argument,name);
	    if (!strcmp(((PlayerCharacter*)ch)->alias[alias],name))
	    {
		buf[0] = '\0';
		strcat(buf,((PlayerCharacter*)ch)->alias_sub[alias]);
		strcat(buf," ");
		strcat(buf,point);
		break;
	    }
	    if (strlen(buf) > MAX_INPUT_LENGTH)
	    {
		send_to_char("Alias substitution too long. Truncated.\r\n",ch);
		buf[MAX_INPUT_LENGTH -1] = '\0';
	    }
	}
    }
    interpret(d->character,buf);
}

void do_alia(Character *ch, char *argument)
{
    send_to_char("I'm sorry, alias must be entered in full.\n\r",ch);
    return;
}

void do_alias(Character *ch, char *argument)
{
    Character *rch;
    char arg[MAX_INPUT_LENGTH],buf[MAX_STRING_LENGTH];
    int pos;

    smash_tilde(argument);

    if (ch->desc == NULL)
	rch = ch;
    else
	rch = ch->desc->original ? ch->desc->original : ch;

    if (IS_NPC(rch))
	return;

    argument = one_argument(argument,arg);
    

    if (arg[0] == '\0')
    {

	if (((PlayerCharacter*)rch)->alias[0] == NULL)
	{
	    send_to_char("You have no aliases defined.\n\r",ch);
	    return;
	}
	send_to_char("Your current aliases are:\n\r",ch);

	for (pos = 0; pos < MAX_ALIAS; pos++)
	{
	    if (((PlayerCharacter*)rch)->alias[pos] == NULL
	    ||	((PlayerCharacter*)rch)->alias_sub[pos] == NULL)
		break;

	    snprintf(buf, sizeof(buf),"    %s:  %s\n\r",((PlayerCharacter*)rch)->alias[pos],
		    ((PlayerCharacter*)rch)->alias_sub[pos]);
	    send_to_char(buf,ch);
	}
	return;
    }

    if (!str_prefix("una",arg) || !str_cmp("alias",arg))
    {
	send_to_char("Sorry, that word is reserved.\n\r",ch);
	return;
    }

    if (argument[0] == '\0')
    {
	for (pos = 0; pos < MAX_ALIAS; pos++)
	{
	    if (((PlayerCharacter*)rch)->alias[pos] == NULL
	    ||	((PlayerCharacter*)rch)->alias_sub[pos] == NULL)
		break;

	    if (!str_cmp(arg,((PlayerCharacter*)rch)->alias[pos]))
	    {
		snprintf(buf, sizeof(buf),"%s aliases to '%s'.\n\r",((PlayerCharacter*)rch)->alias[pos],
			((PlayerCharacter*)rch)->alias_sub[pos]);
		send_to_char(buf,ch);
		return;
	    }
	}

	send_to_char("That alias is not defined.\n\r",ch);
	return;
    }

    if (!str_prefix(argument,"delete") || !str_prefix(argument,"prefix"))
    {
	send_to_char("That shall not be done!\n\r",ch);
	return;
    }

    if ( strstr(argument, "'") || strstr(argument, "\""))
    {
	send_to_char("Aliases may not contain ' or \".",ch);
	return;
    }

    for (pos = 0; pos < MAX_ALIAS; pos++)
    {
	if (((PlayerCharacter*)rch)->alias[pos] == NULL)
	    break;

	if (!str_cmp(arg,((PlayerCharacter*)rch)->alias[pos])) /* redefine an alias */
	{
	    free_string(((PlayerCharacter*)rch)->alias_sub[pos]);
	    ((PlayerCharacter*)rch)->alias_sub[pos] = str_dup(argument);
	    snprintf(buf, sizeof(buf),"%s is now realiased to '%s'.\n\r",arg,argument);
	    send_to_char(buf,ch);
	    return;
	}
     }

     if (pos >= MAX_ALIAS)
     {
	send_to_char("Sorry, you have reached the alias limit.\n\r",ch);
	return;
     }
  
     /* make a new alias */
     ((PlayerCharacter*)rch)->alias[pos]		= str_dup(arg);
     ((PlayerCharacter*)rch)->alias_sub[pos]	= str_dup(argument);
     snprintf(buf, sizeof(buf),"%s is now aliased to '%s'.\n\r",arg,argument);
     send_to_char(buf,ch);
}


void do_unalias(Character *ch, char *argument)
{
    Character *rch;
    char arg[MAX_INPUT_LENGTH];
    int pos;
    bool found = FALSE;
 
    if (ch->desc == NULL)
	rch = ch;
    else
	rch = ch->desc->original ? ch->desc->original : ch;
 
    if (IS_NPC(rch))
	return;
 
    one_argument(argument,arg);

    if (arg[0] == '\0')
    {
		send_to_char("Unalias what?\n\r",ch);
		return;
    }

    for (pos = 0; pos < MAX_ALIAS; pos++)
    {
	if (((PlayerCharacter*)rch)->alias[pos] == NULL)
	    break;

	if (found)
	{
	    ((PlayerCharacter*)rch)->alias[pos-1]		= ((PlayerCharacter*)rch)->alias[pos];
	    ((PlayerCharacter*)rch)->alias_sub[pos-1]	= ((PlayerCharacter*)rch)->alias_sub[pos];
	    ((PlayerCharacter*)rch)->alias[pos]		= NULL;
	    ((PlayerCharacter*)rch)->alias_sub[pos]		= NULL;
	    continue;
	}

	if(!strcmp(arg,((PlayerCharacter*)rch)->alias[pos]))
	{
	    send_to_char("Alias removed.\n\r",ch);
	    free_string(((PlayerCharacter*)rch)->alias[pos]);
	    free_string(((PlayerCharacter*)rch)->alias_sub[pos]);
	    ((PlayerCharacter*)rch)->alias[pos] = NULL;
	    ((PlayerCharacter*)rch)->alias_sub[pos] = NULL;
	    found = TRUE;
	}
    }

    if (!found)
	send_to_char("No alias of that name to remove.\n\r",ch);
}

     













