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
#else
#include <sys/types.h>
#include <sys/time.h>
#endif
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include <unistd.h>
#include <vector>
#include <map>
#include "merc.h"
#include "magic.h"
#include "recycle.h"
#include "tables.h"
#include "lookup.h"
#include "PlayerCharacter.h"

/* command procedures needed */
DECLARE_DO_FUN(	do_exits	);
DECLARE_DO_FUN( do_look		);
DECLARE_DO_FUN( do_help		);
DECLARE_DO_FUN( do_affects	);
DECLARE_DO_FUN( do_play		);

void show_eq_char( Character *ch, Character *ch1 );

/* for do_count */
int max_on = 0;



/*
 * Local functions.
 */
char *	format_obj_to_char	args( ( OBJ_DATA *obj, Character *ch,
				    bool fShort ) );
void	show_list_to_char	args( ( OBJ_DATA *list, Character *ch,
				    bool fShort, bool fShowNothing ) );
void	show_char_to_char_0	args( ( Character *victim, Character *ch ) );
void	show_char_to_char_1	args( ( Character *victim, Character *ch ) );
void	show_char_to_char	args( ( Character *list, Character *ch ) );
bool	check_blind		args( ( Character *ch ) );



char *format_obj_to_char( OBJ_DATA *obj, Character *ch, bool fShort )
{
    static char buf[MAX_STRING_LENGTH];

    buf[0] = '\0';

    if ((fShort && (obj->short_descr == NULL || obj->short_descr[0] == '\0'))
    ||  (obj->description == NULL || obj->description[0] == '\0'))
	return buf;

    if ( IS_OBJ_STAT(obj, ITEM_INVIS)     )   strcat( buf, "(Invis) "     );
    if ( IS_AFFECTED(ch, AFF_DETECT_EVIL)
         && IS_OBJ_STAT(obj, ITEM_EVIL)   )   strcat( buf, "(Red Aura) "  );
    if (IS_AFFECTED(ch, AFF_DETECT_GOOD)
    &&  IS_OBJ_STAT(obj,ITEM_BLESS))	      strcat(buf,"(Blue Aura) "	);
    if ( IS_AFFECTED(ch, AFF_DETECT_MAGIC)
         && IS_OBJ_STAT(obj, ITEM_MAGIC)  )   strcat( buf, "(Magical) "   );
    if ( IS_OBJ_STAT(obj, ITEM_GLOW)      )   strcat( buf, "(Glowing) "   );
    if ( IS_OBJ_STAT(obj, ITEM_HUM)       )   strcat( buf, "(Humming) "   );

    if ( ch->level <= 20 && ch->level >= obj->level )
	strcat( buf, "{Y*{x ");

    if ( fShort )
    {
	if ( obj->short_descr != NULL )
	    strcat( buf, obj->short_descr );
    }
    else
    {
	if ( obj->description != NULL)
	    strcat( buf, obj->description );
    }

    return buf;
}



/*
 * Show a list to a character.
 * Can coalesce duplicated items.
 */
void show_list_to_char( OBJ_DATA *list, Character *ch, bool fShort, bool fShowNothing )
{
    char buf[MAX_STRING_LENGTH];
    std::string output;
    std::vector<std::tuple<std::string, int>> prgpstrShow;
    char *pstrShow;
    OBJ_DATA *obj;
    int count;
    bool fCombine;

    if ( ch->desc == NULL ) {
        return;
    }

    count = 0;
    for ( obj = list; obj != NULL; obj = obj->next_content ) {
        count++;
    }

    if ( count > 1000 ) {
        send_to_char("That is WAY too much junk, drop some of it!\n\r",ch);
        return;
    }

    /*
     * Format the list of objects.
     */
    for ( obj = list; obj != NULL; obj = obj->next_content ) {
        if ( obj->wear_loc == WEAR_NONE && can_see_obj( ch, obj )) {
            pstrShow = format_obj_to_char( obj, ch, fShort );

            fCombine = FALSE;

            if ( IS_NPC(ch) || IS_SET(ch->comm, COMM_COMBINE) ) {
                /*
                 * Look for duplicates, case sensitive.
                 * Matches tend to be near end so run loop backwords.
                 */
                for ( auto it = prgpstrShow.begin(); it != prgpstrShow.end(); ++it ) {
                    auto objName = std::get<0>(*it);
                    if ( !objName.compare(pstrShow) ) {
                        std::get<1>(*it)++;
                        fCombine = TRUE;
                        break;
                    }
                }
            }

            /*
             * Couldn't combine, or didn't want to.
             */
            if ( !fCombine ) {
                prgpstrShow.push_back(std::make_tuple(pstrShow, 1));
            }
        }
    }

    /*
     * Output the formatted list.
     */
    for ( auto it = prgpstrShow.begin(); it != prgpstrShow.end(); ++it ) {
        auto objName = std::get<0>(*it);
        auto objCount = std::get<1>(*it);
        if ( IS_NPC(ch) || IS_SET(ch->comm, COMM_COMBINE) ) {
            if ( objCount != 1 ) {
                snprintf(buf, sizeof(buf), "(%2d) ", objCount );
                output.append(buf);
            } else {
                output.append("     ");
            }
        }

        output.append(objName);
        output.append("\n\r");
    }

    if ( fShowNothing && prgpstrShow.size() == 0 ) {
        if ( IS_NPC(ch) || IS_SET(ch->comm, COMM_COMBINE) ) {
            send_to_char("     ", ch);
        }
        send_to_char( "Nothing.\n\r", ch );
    }
    page_to_char(output.c_str(),ch);
    return;
}



void show_char_to_char_0( Character *victim, Character *ch )
{
    char buf[MAX_STRING_LENGTH],message[MAX_STRING_LENGTH];

    buf[0] = '\0';

    if ( IS_SET(victim->comm,COMM_AFK	  )   ) strcat( buf, "[AFK] "	     );
    if ( IS_AFFECTED(victim, AFF_INVISIBLE)   ) strcat( buf, "(Invis) "      );
    if ( victim->invis_level >= LEVEL_HERO    ) strcat( buf, "(Wizi) "	     );
    if ( IS_AFFECTED(victim, AFF_HIDE)        ) strcat( buf, "(Hide) "       );
    if ( IS_AFFECTED(victim, AFF_CHARM)       ) strcat( buf, "(Charmed) "    );
    if ( IS_AFFECTED(victim, AFF_PASS_DOOR)   ) strcat( buf, "(Translucent) ");
    if ( IS_AFFECTED(victim, AFF_FAERIE_FIRE) ) strcat( buf, "(Pink Aura) "  );
    if ( IS_EVIL(victim)
    &&   IS_AFFECTED(ch, AFF_DETECT_EVIL)     ) strcat( buf, "(Red Aura) "   );
    if ( IS_GOOD(victim)
    &&   IS_AFFECTED(ch, AFF_DETECT_GOOD)     ) strcat( buf, "(Golden Aura) ");
    if ( IS_AFFECTED(victim, AFF_SANCTUARY)   ) strcat( buf, "(White Aura) " );
    if ( !IS_NPC(victim) && IS_SET(victim->act, PLR_THIEF  ) )
						strcat( buf, "{B({YTHIEF{B){x " );

    if ( IS_CLANNED(victim)	)		strcat( buf, victim->pcdata->clan->whoname );
    if ( victim->position == victim->start_pos && victim->long_descr[0] != '\0' )
    {
	strcat( buf, victim->long_descr );
	send_to_char( buf, ch );
	return;
    }

    strcat( buf, PERS( victim, ch ) );
    if ( !IS_NPC(victim) && !IS_SET(ch->comm, COMM_BRIEF) 
    &&   victim->position == POS_STANDING && ch->on == NULL )
	strcat( buf, victim->pcdata->title );

    switch ( victim->position )
    {
    case POS_DEAD:     strcat( buf, " is DEAD!!" );              break;
    case POS_MORTAL:   strcat( buf, " is mortally wounded." );   break;
    case POS_INCAP:    strcat( buf, " is incapacitated." );      break;
    case POS_STUNNED:  strcat( buf, " is lying here stunned." ); break;
    case POS_SLEEPING: 
	if (victim->on != NULL)
	{
	    if (IS_SET(victim->on->value[2],SLEEP_AT))
  	    {
		snprintf(message, sizeof(message)," is sleeping at %s.",
		    victim->on->short_descr);
		strcat(buf,message);
	    }
	    else if (IS_SET(victim->on->value[2],SLEEP_ON))
	    {
		snprintf(message, sizeof(message)," is sleeping on %s.",
		    victim->on->short_descr); 
		strcat(buf,message);
	    }
	    else
	    {
		snprintf(message, sizeof(message), " is sleeping in %s.",
		    victim->on->short_descr);
		strcat(buf,message);
	    }
	}
	else 
	    strcat(buf," is sleeping here.");
	break;
    case POS_RESTING:  
        if (victim->on != NULL)
	{
            if (IS_SET(victim->on->value[2],REST_AT))
            {
                snprintf(message, sizeof(message)," is resting at %s.",
                    victim->on->short_descr);
                strcat(buf,message);
            }
            else if (IS_SET(victim->on->value[2],REST_ON))
            {
                snprintf(message, sizeof(message)," is resting on %s.",
                    victim->on->short_descr);
                strcat(buf,message);
            }
            else 
            {
                snprintf(message, sizeof(message), " is resting in %s.",
                    victim->on->short_descr);
                strcat(buf,message);
            }
	}
        else
	    strcat( buf, " is resting here." );       
	break;
    case POS_SITTING:  
        if (victim->on != NULL)
        {
            if (IS_SET(victim->on->value[2],SIT_AT))
            {
                snprintf(message, sizeof(message)," is sitting at %s.",
                    victim->on->short_descr);
                strcat(buf,message);
            }
            else if (IS_SET(victim->on->value[2],SIT_ON))
            {
                snprintf(message, sizeof(message)," is sitting on %s.",
                    victim->on->short_descr);
                strcat(buf,message);
            }
            else
            {
                snprintf(message, sizeof(message), " is sitting in %s.",
                    victim->on->short_descr);
                strcat(buf,message);
            }
        }
        else
	    strcat(buf, " is sitting here.");
	break;
    case POS_STANDING: 
	if (victim->on != NULL)
	{
	    if (IS_SET(victim->on->value[2],STAND_AT))
	    {
		snprintf(message, sizeof(message)," is standing at %s.",
		    victim->on->short_descr);
		strcat(buf,message);
	    }
	    else if (IS_SET(victim->on->value[2],STAND_ON))
	    {
		snprintf(message, sizeof(message)," is standing on %s.",
		   victim->on->short_descr);
		strcat(buf,message);
	    }
	    else
	    {
		snprintf(message, sizeof(message)," is standing in %s.",
		    victim->on->short_descr);
		strcat(buf,message);
	    }
	}
	else
	    strcat( buf, " is here." );               
	break;
    case POS_FIGHTING:
	strcat( buf, " is here, fighting " );
	if ( victim->fighting == NULL )
	    strcat( buf, "thin air??" );
	else if ( victim->fighting == ch )
	    strcat( buf, "YOU!" );
	else if ( victim->in_room == victim->fighting->in_room )
	{
	    strcat( buf, PERS( victim->fighting, ch ) );
	    strcat( buf, "." );
	}
	else
	    strcat( buf, "someone who left??" );
	break;
    }

    strcat( buf, "\n\r" );
    buf[0] = UPPER(buf[0]);
    send_to_char( buf, ch );
    return;
}



void show_char_to_char_1( Character *victim, Character *ch )
{
    char buf[MAX_STRING_LENGTH];
    int percent;

    if ( can_see( victim, ch ) )
    {
	if (ch == victim)
	    act( "$n looks at $mself.",ch,NULL,NULL,TO_ROOM);
	else
	{
	    act( "$n looks at you.", ch, NULL, victim, TO_VICT    );
	    act( "$n looks at $N.",  ch, NULL, victim, TO_NOTVICT );
	}
    }

    if ( victim->getDescription() )
    {
	send_to_char( victim->getDescription(), ch );
    }
    else
    {
	act( "You see nothing special about $M.", ch, NULL, victim, TO_CHAR );
    }

    if ( victim->max_hit > 0 )
	percent = ( 100 * victim->hit ) / victim->max_hit;
    else
	percent = -1;

    strcpy( buf, PERS(victim, ch) );

    if (percent >= 100) 
	strcat( buf, " is in excellent condition.\n\r");
    else if (percent >= 90) 
	strcat( buf, " has a few scratches.\n\r");
    else if (percent >= 75) 
	strcat( buf," has some small wounds and bruises.\n\r");
    else if (percent >=  50) 
	strcat( buf, " has quite a few wounds.\n\r");
    else if (percent >= 30)
	strcat( buf, " has some big nasty wounds and scratches.\n\r");
    else if (percent >= 15)
	strcat ( buf, " looks pretty hurt.\n\r");
    else if (percent >= 0 )
	strcat (buf, " is in awful condition.\n\r");
    else
	strcat(buf, " is bleeding to death.\n\r");

    buf[0] = UPPER(buf[0]);
    send_to_char( buf, ch );

    show_eq_char( victim, ch );

    if ( victim != ch
    &&   !IS_NPC(ch)
    &&   number_percent( ) < get_skill(ch,gsn_peek))
    {
	send_to_char( "\n\rYou peek at the inventory:\n\r", ch );
	check_improve(ch,gsn_peek,TRUE,4);
	show_list_to_char( victim->carrying, ch, TRUE, TRUE );
    }

    return;
}



void show_char_to_char( Character *list, Character *ch )
{
    Character *rch;

    for ( rch = list; rch != NULL; rch = rch->next_in_room )
    {
	if ( rch == ch )
	    continue;

	if ( get_trust(ch) < rch->invis_level)
	    continue;

	if ( can_see( ch, rch ) )
	{
	    show_char_to_char_0( rch, ch );
	}
	else if ( room_is_dark( ch->in_room )
	&&        IS_AFFECTED(rch, AFF_INFRARED ) )
	{
	    send_to_char( "You see glowing red eyes watching YOU!\n\r", ch );
	}
    }

    return;
} 



bool check_blind( Character *ch )
{

    if (!IS_NPC(ch) && IS_SET(ch->act,PLR_HOLYLIGHT))
	return TRUE;

    if ( IS_AFFECTED(ch, AFF_BLIND) )
    { 
	send_to_char( "You can't see a thing!\n\r", ch ); 
	return FALSE; 
    }

    return TRUE;
}

/* changes your scroll */
void do_scroll(Character *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    char buf[100];
    int lines;

    one_argument(argument,arg);
    
    if (arg[0] == '\0')
    {
	if (ch->lines == 0)
	    send_to_char("You do not page long messages.\n\r",ch);
	else
	{
	    snprintf(buf, sizeof(buf),"You currently display %d lines per page.\n\r",
		    ch->lines + 2);
	    send_to_char(buf,ch);
	}
	return;
    }

    if (!is_number(arg))
    {
	send_to_char("You must provide a number.\n\r",ch);
	return;
    }

    lines = atoi(arg);

    if (lines == 0)
    {
        send_to_char("Paging disabled.\n\r",ch);
        ch->lines = 0;
        return;
    }

    if (lines < 10 || lines > 100)
    {
	send_to_char("You must provide a reasonable number.\n\r",ch);
	return;
    }

    snprintf(buf, sizeof(buf),"Scroll set to %d lines.\n\r",lines);
    send_to_char(buf,ch);
    ch->lines = lines - 2;
}

/* RT does socials */
void do_socials(Character *ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    int iSocial;
    int col;
     
    col = 0;
   
    for (iSocial = 0; social_table[iSocial].name[0] != '\0'; iSocial++)
    {
	snprintf(buf, sizeof(buf),"%-12s",social_table[iSocial].name);
	send_to_char(buf,ch);
	if (++col % 6 == 0)
	    send_to_char("\n\r",ch);
    }

    if ( col % 6 != 0)
	send_to_char("\n\r",ch);
    return;
}


 
/* RT Commands to replace news, motd, imotd, etc from ROM */

void do_motd(Character *ch, char *argument)
{
    do_help(ch,(char*)"motd");
}

void do_imotd(Character *ch, char *argument)
{  
    do_help(ch,(char*)"imotd");
}

void do_rules(Character *ch, char *argument)
{
    do_help(ch,(char*)"rules");
}

void do_story(Character *ch, char *argument)
{
    do_help(ch,(char*)"story");
}

void do_wizlist(Character *ch, char *argument)
{
    do_help(ch,(char*)"wizlist");
}

/* RT this following section holds all the auto commands from ROM, as well as
   replacements for config */

void do_autolist(Character *ch, char *argument)
{
    /* lists most player flags */
    if (IS_NPC(ch))
      return;

    send_to_char("   action     status\n\r",ch);
    send_to_char("---------------------\n\r",ch);
 
    send_to_char("autoassist     ",ch);
    if (IS_SET(ch->act,PLR_AUTOASSIST))
        send_to_char("ON\n\r",ch);
    else
        send_to_char("OFF\n\r",ch); 

    send_to_char("autoexit       ",ch);
    if (IS_SET(ch->act,PLR_AUTOEXIT))
        send_to_char("ON\n\r",ch);
    else
        send_to_char("OFF\n\r",ch);

    send_to_char("autogold       ",ch);
    if (IS_SET(ch->act,PLR_AUTOGOLD))
        send_to_char("ON\n\r",ch);
    else
        send_to_char("OFF\n\r",ch);

    send_to_char("autoloot       ",ch);
    if (IS_SET(ch->act,PLR_AUTOLOOT))
        send_to_char("ON\n\r",ch);
    else
        send_to_char("OFF\n\r",ch);

    send_to_char("autosac        ",ch);
    if (IS_SET(ch->act,PLR_AUTOSAC))
        send_to_char("ON\n\r",ch);
    else
        send_to_char("OFF\n\r",ch);

    send_to_char("autosplit      ",ch);
    if (IS_SET(ch->act,PLR_AUTOSPLIT))
        send_to_char("ON\n\r",ch);
    else
        send_to_char("OFF\n\r",ch);

    send_to_char("compact mode   ",ch);
    if (IS_SET(ch->comm,COMM_COMPACT))
        send_to_char("ON\n\r",ch);
    else
        send_to_char("OFF\n\r",ch);

    send_to_char("prompt         ",ch);
    if (IS_SET(ch->comm,COMM_PROMPT))
	send_to_char("ON\n\r",ch);
    else
	send_to_char("OFF\n\r",ch);

    send_to_char("combine items  ",ch);
    if (IS_SET(ch->comm,COMM_COMBINE))
	send_to_char("ON\n\r",ch);
    else
	send_to_char("OFF\n\r",ch);

    if (!IS_SET(ch->act,PLR_CANLOOT))
	send_to_char("Your corpse is safe from thieves.\n\r",ch);
    else 
        send_to_char("Your corpse may be looted.\n\r",ch);

    if (IS_SET(ch->act,PLR_NOSUMMON))
	send_to_char("You cannot be summoned.\n\r",ch);
    else
	send_to_char("You can be summoned.\n\r",ch);
   
    if (IS_SET(ch->act,PLR_NOFOLLOW))
	send_to_char("You do not welcome followers.\n\r",ch);
    else
	send_to_char("You accept followers.\n\r",ch);
}

void do_autoassist(Character *ch, char *argument) {
    if (IS_NPC(ch))
        return;

    if (IS_SET(ch->act, PLR_AUTOASSIST)) {
        send_to_char("Autoassist removed.\n\r", ch);
        REMOVE_BIT(ch->act, PLR_AUTOASSIST);
    } else {
        send_to_char("You will now assist when needed.\n\r", ch);
        SET_BIT(ch->act, PLR_AUTOASSIST);
    }
}

void do_autoexit(Character *ch, char *argument)
{
    if (IS_NPC(ch))
      return;
 
    if (IS_SET(ch->act,PLR_AUTOEXIT))
    {
      send_to_char("Exits will no longer be displayed.\n\r",ch);
      REMOVE_BIT(ch->act,PLR_AUTOEXIT);
    }
    else
    {
      send_to_char("Exits will now be displayed.\n\r",ch);
      SET_BIT(ch->act,PLR_AUTOEXIT);
    }
}

void do_autogold(Character *ch, char *argument)
{
    if (IS_NPC(ch))
      return;
 
    if (IS_SET(ch->act,PLR_AUTOGOLD))
    {
      send_to_char("Autogold removed.\n\r",ch);
      REMOVE_BIT(ch->act,PLR_AUTOGOLD);
    }
    else
    {
      send_to_char("Automatic gold looting set.\n\r",ch);
      SET_BIT(ch->act,PLR_AUTOGOLD);
    }
}

void do_autoloot(Character *ch, char *argument)
{
    if (IS_NPC(ch))
      return;
 
    if (IS_SET(ch->act,PLR_AUTOLOOT))
    {
      send_to_char("Autolooting removed.\n\r",ch);
      REMOVE_BIT(ch->act,PLR_AUTOLOOT);
    }
    else
    {
      send_to_char("Automatic corpse looting set.\n\r",ch);
      SET_BIT(ch->act,PLR_AUTOLOOT);
    }
}

void do_autosac(Character *ch, char *argument)
{
    if (IS_NPC(ch))
      return;
 
    if (IS_SET(ch->act,PLR_AUTOSAC))
    {
      send_to_char("Autosacrificing removed.\n\r",ch);
      REMOVE_BIT(ch->act,PLR_AUTOSAC);
    }
    else
    {
      send_to_char("Automatic corpse sacrificing set.\n\r",ch);
      SET_BIT(ch->act,PLR_AUTOSAC);
    }
}

void do_autosplit(Character *ch, char *argument)
{
    if (IS_NPC(ch))
      return;
 
    if (IS_SET(ch->act,PLR_AUTOSPLIT))
    {
      send_to_char("Autosplitting removed.\n\r",ch);
      REMOVE_BIT(ch->act,PLR_AUTOSPLIT);
    }
    else
    {
      send_to_char("Automatic gold splitting set.\n\r",ch);
      SET_BIT(ch->act,PLR_AUTOSPLIT);
    }
}

void do_brief(Character *ch, char *argument)
{
    if (IS_SET(ch->comm,COMM_BRIEF))
    {
      send_to_char("Full descriptions activated.\n\r",ch);
      REMOVE_BIT(ch->comm,COMM_BRIEF);
    }
    else
    {
      send_to_char("Short descriptions activated.\n\r",ch);
      SET_BIT(ch->comm,COMM_BRIEF);
    }
}

void do_compact(Character *ch, char *argument)
{
    if (IS_SET(ch->comm,COMM_COMPACT))
    {
      send_to_char("Compact mode removed.\n\r",ch);
      REMOVE_BIT(ch->comm,COMM_COMPACT);
    }
    else
    {
      send_to_char("Compact mode set.\n\r",ch);
      SET_BIT(ch->comm,COMM_COMPACT);
    }
}

void do_show(Character *ch, char *argument)
{
    if (IS_SET(ch->comm,COMM_SHOW_AFFECTS))
    {
      send_to_char("Affects will no longer be shown in score.\n\r",ch);
      REMOVE_BIT(ch->comm,COMM_SHOW_AFFECTS);
    }
    else
    {
      send_to_char("Affects will now be shown in score.\n\r",ch);
      SET_BIT(ch->comm,COMM_SHOW_AFFECTS);
    }
}

void do_prompt(Character *ch, char *argument)
{
   char buf[MAX_STRING_LENGTH];
 
   if ( argument[0] == '\0' )
   {
	if (IS_SET(ch->comm,COMM_PROMPT))
   	{
      	    send_to_char("You will no longer see prompts.\n\r",ch);
      	    REMOVE_BIT(ch->comm,COMM_PROMPT);
    	}
    	else
    	{
      	    send_to_char("You will now see prompts.\n\r",ch);
      	    SET_BIT(ch->comm,COMM_PROMPT);
    	}
       return;
   }
 
   if( !strcmp( argument, "all" ) )
      strcpy( buf, "<%X to level>%c<%h/%Hhp %m/%Mm %v/%Vmv> ");
   else
   {
      if ( strlen(argument) > 50 )
         argument[50] = '\0';
      strcpy( buf, argument );
      smash_tilde( buf );
      if (str_suffix("%c",buf))
	strcat(buf," ");
	
   }
 
   free_string( ch->prompt );
   ch->prompt = str_dup( buf );
   snprintf(buf, sizeof(buf),"Prompt set to %s\n\r",ch->prompt );
   send_to_char(buf,ch);
   return;
}

void do_combine(Character *ch, char *argument)
{
    if (IS_SET(ch->comm,COMM_COMBINE))
    {
      send_to_char("Long inventory selected.\n\r",ch);
      REMOVE_BIT(ch->comm,COMM_COMBINE);
    }
    else
    {
      send_to_char("Combined inventory selected.\n\r",ch);
      SET_BIT(ch->comm,COMM_COMBINE);
    }
}

void do_noloot(Character *ch, char *argument)
{
    if (IS_NPC(ch))
      return;
 
    if (IS_SET(ch->act,PLR_CANLOOT))
    {
      send_to_char("Your corpse is now safe from thieves.\n\r",ch);
      REMOVE_BIT(ch->act,PLR_CANLOOT);
    }
    else
    {
      send_to_char("Your corpse may now be looted.\n\r",ch);
      SET_BIT(ch->act,PLR_CANLOOT);
    }
}

void do_nofollow(Character *ch, char *argument)
{
    if (IS_NPC(ch))
      return;
 
    if (IS_SET(ch->act,PLR_NOFOLLOW))
    {
      send_to_char("You now accept followers.\n\r",ch);
      REMOVE_BIT(ch->act,PLR_NOFOLLOW);
    }
    else
    {
      send_to_char("You no longer accept followers.\n\r",ch);
      SET_BIT(ch->act,PLR_NOFOLLOW);
      die_follower( ch );
    }
}

void do_nosummon(Character *ch, char *argument)
{
    if (IS_NPC(ch))
    {
      if (IS_SET(ch->imm_flags,IMM_SUMMON))
      {
	send_to_char("You are no longer immune to summon.\n\r",ch);
	REMOVE_BIT(ch->imm_flags,IMM_SUMMON);
      }
      else
      {
	send_to_char("You are now immune to summoning.\n\r",ch);
	SET_BIT(ch->imm_flags,IMM_SUMMON);
      }
    }
    else
    {
      if (IS_SET(ch->act,PLR_NOSUMMON))
      {
        send_to_char("You are no longer immune to summon.\n\r",ch);
        REMOVE_BIT(ch->act,PLR_NOSUMMON);
      }
      else
      {
        send_to_char("You are now immune to summoning.\n\r",ch);
        SET_BIT(ch->act,PLR_NOSUMMON);
      }
    }
}

void do_look( Character *ch, char *argument )
{
    char buf  [MAX_STRING_LENGTH];
    char arg1 [MAX_INPUT_LENGTH];
    char arg2 [MAX_INPUT_LENGTH];
    char arg3 [MAX_INPUT_LENGTH];
    EXIT_DATA *pexit;
    Character *victim;
    OBJ_DATA *obj;
    char *pdesc;
    int door;
    int number,count;

    if ( ch->desc == NULL )
	return;

    if ( ch->position < POS_SLEEPING )
    {
	send_to_char( "You can't see anything but stars!\n\r", ch );
	return;
    }

    if ( ch->position == POS_SLEEPING )
    {
	send_to_char( "You can't see anything, you're sleeping!\n\r", ch );
	return;
    }

    if ( !check_blind( ch ) )
	return;

    if ( !IS_NPC(ch)
    &&   !IS_SET(ch->act, PLR_HOLYLIGHT)
    &&   room_is_dark( ch->in_room ) )
    {
	send_to_char( "It is pitch black ... \n\r", ch );
	show_char_to_char( ch->in_room->people, ch );
	return;
    }

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    number = number_argument(arg1,arg3);
    count = 0;

    if ( arg1[0] == '\0' || !str_cmp( arg1, "auto" ) )
    {
	/* 'look' or 'look auto' */
	send_to_char( "{y", ch);
	send_to_char( ch->in_room->name, ch );

	if ( (IS_IMMORTAL(ch) && (IS_NPC(ch) || IS_SET(ch->act,PLR_HOLYLIGHT)))
	||   IS_BUILDER(ch, ch->in_room->area) )
	{
	    snprintf(buf, sizeof(buf)," [Room %d]",ch->in_room->vnum);
	    send_to_char(buf,ch);
	}

	send_to_char( "{x\n\r", ch );

	if ( arg1[0] == '\0'
	|| ( !IS_NPC(ch) && !IS_SET(ch->comm, COMM_BRIEF) ) )
	{
	    send_to_char( "{g  ",ch);
	    send_to_char( ch->in_room->description, ch );
	    send_to_char( "{x",ch);
	}

        if ( !IS_NPC(ch) && IS_SET(ch->act, PLR_AUTOEXIT) )
	{
	    send_to_char("\n\r",ch);
            do_exits( ch, (char*)"auto" );
	}

	show_list_to_char( ch->in_room->contents, ch, FALSE, FALSE );
	show_char_to_char( ch->in_room->people,   ch );
	return;
    }

    if ( !str_cmp( arg1, "i" ) || !str_cmp(arg1, "in")  || !str_cmp(arg1,"on"))
    {
	/* 'look in' */
	if ( arg2[0] == '\0' )
	{
	    send_to_char( "Look in what?\n\r", ch );
	    return;
	}

	if ( ( obj = get_obj_here( ch, arg2 ) ) == NULL )
	{
	    send_to_char( "You do not see that here.\n\r", ch );
	    return;
	}

	switch ( obj->item_type )
	{
	default:
	    send_to_char( "That is not a container.\n\r", ch );
	    break;

	case ITEM_DRINK_CON:
	    if ( obj->value[1] <= 0 )
	    {
		send_to_char( "It is empty.\n\r", ch );
		break;
	    }

	    snprintf(buf, sizeof(buf), "It's %sfilled with  a %s liquid.\n\r",
		obj->value[1] <     obj->value[0] / 4
		    ? "less than half-" :
		obj->value[1] < 3 * obj->value[0] / 4
		    ? "about half-"     : "more than half-",
		liq_table[obj->value[2]].liq_color
		);

	    send_to_char( buf, ch );
	    break;

	case ITEM_CONTAINER:
	case ITEM_CORPSE_NPC:
	case ITEM_CORPSE_PC:
	    if ( IS_SET(obj->value[1], CONT_CLOSED) )
	    {
		send_to_char( "It is closed.\n\r", ch );
		break;
	    }

	    act( "$p holds:", ch, obj, NULL, TO_CHAR );
	    show_list_to_char( obj->contains, ch, TRUE, TRUE );
	    break;
	}
	return;
    }

    if ( ( victim = get_char_room( ch, arg1 ) ) != NULL )
    {
	show_char_to_char_1( victim, ch );
	return;
    }

    for ( obj = ch->carrying; obj != NULL; obj = obj->next_content )
    {
	if ( can_see_obj( ch, obj ) )
	{  /* player can see object */
	    if (obj->item_type == ITEM_WINDOW)
	    /* Voltecs Window code */
	    {
		look_window(ch, obj);
           	return;
	    }

	    pdesc = get_extra_descr( arg3, obj->extra_descr );
	    if ( pdesc != NULL )
	    {
	    	if (++count == number)
	    	{
		    send_to_char( pdesc, ch );
		    return;
	    	}
	    	else continue;
	    }

 	    pdesc = get_extra_descr( arg3, obj->pIndexData->extra_descr );
 	    if ( pdesc != NULL )
	    {
 	    	if (++count == number)
 	    	{	
		    send_to_char( pdesc, ch );
		    return;
	     	}
		else continue;
	    }

	    if ( is_name( arg3, obj->name ) )
	    	if (++count == number)
	    	{
	    	    send_to_char( obj->description, ch );
	    	    send_to_char( "\n\r",ch);
		    return;
		  }
	  }
    }

    for ( obj = ch->in_room->contents; obj != NULL; obj = obj->next_content )
    {
	if ( can_see_obj( ch, obj ) )
	{
	    pdesc = get_extra_descr( arg3, obj->extra_descr );
	    if ( pdesc != NULL )
	    	if (++count == number)
	    	{
		    send_to_char( pdesc, ch );
		    return;
	    	}

	    pdesc = get_extra_descr( arg3, obj->pIndexData->extra_descr );
	    if ( pdesc != NULL )
	    	if (++count == number)
	    	{
		    send_to_char( pdesc, ch );
		    return;
	    	}

	    if ( is_name( arg3, obj->name ) )
		if (++count == number)
		{
		    send_to_char( obj->description, ch );
		    send_to_char("\n\r",ch);
		    return;
		}
	}
    }

    pdesc = get_extra_descr(arg3,ch->in_room->extra_descr);
    if (pdesc != NULL)
    {
	if (++count == number)
	{
	    send_to_char(pdesc,ch);
	    return;
	}
    }
    
    if (count > 0 && count != number)
    {
    	if (count == 1)
    	    snprintf(buf, sizeof(buf),"You only see one %s here.\n\r",arg3);
    	else
    	    snprintf(buf, sizeof(buf),"You only see %d of those here.\n\r",count);
    	
    	send_to_char(buf,ch);
    	return;
    }

         if ( !str_cmp( arg1, "n" ) || !str_cmp( arg1, "north" ) ) door = 0;
    else if ( !str_cmp( arg1, "e" ) || !str_cmp( arg1, "east"  ) ) door = 1;
    else if ( !str_cmp( arg1, "s" ) || !str_cmp( arg1, "south" ) ) door = 2;
    else if ( !str_cmp( arg1, "w" ) || !str_cmp( arg1, "west"  ) ) door = 3;
    else if ( !str_cmp( arg1, "u" ) || !str_cmp( arg1, "up"    ) ) door = 4;
    else if ( !str_cmp( arg1, "d" ) || !str_cmp( arg1, "down"  ) ) door = 5;
    else
    {
	send_to_char( "You do not see that here.\n\r", ch );
	return;
    }

    /* 'look direction' */
    if ( ( pexit = ch->in_room->exit[door] ) == NULL )
    {
	send_to_char( "Nothing special there.\n\r", ch );
	return;
    }

    if ( pexit->description != NULL && pexit->description[0] != '\0' )
	send_to_char( pexit->description, ch );
    else
	send_to_char( "Nothing special there.\n\r", ch );

    if ( pexit->keyword    != NULL
    &&   pexit->keyword[0] != '\0'
    &&   pexit->keyword[0] != ' ' )
    {
	if ( IS_SET(pexit->exit_info, EX_CLOSED) )
	{
	    act( "The $d is closed.", ch, NULL, pexit->keyword, TO_CHAR );
	}
	else if ( IS_SET(pexit->exit_info, EX_ISDOOR) )
	{
	    act( "The $d is open.",   ch, NULL, pexit->keyword, TO_CHAR );
	}
    }

    return;
}

/* RT added back for the hell of it */
void do_read (Character *ch, char *argument )
{
    do_look(ch,argument);
}

void do_examine( Character *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	send_to_char( "Examine what?\n\r", ch );
	return;
    }

    do_look( ch, arg );

    if ( ( obj = get_obj_here( ch, arg ) ) != NULL )
    {
	switch ( obj->item_type )
	{
	default:
	    break;
	
	case ITEM_JUKEBOX:
	    do_play(ch,(char*)"list");
	    break;

	case ITEM_MONEY:
	    if (obj->value[0] == 0)
	    {
	        if (obj->value[1] == 0)
		    snprintf(buf, sizeof(buf),"Odd...there's no coins in the pile.\n\r");
		else if (obj->value[1] == 1)
		    snprintf(buf, sizeof(buf),"Wow. One gold coin.\n\r");
		else
		    snprintf(buf, sizeof(buf),"There are %d gold coins in the pile.\n\r",
			obj->value[1]);
	    }
	    else if (obj->value[1] == 0)
	    {
		if (obj->value[0] == 1)
		    snprintf(buf, sizeof(buf),"Wow. One silver coin.\n\r");
		else
		    snprintf(buf, sizeof(buf),"There are %d silver coins in the pile.\n\r",
			obj->value[0]);
	    }
	    else
		snprintf(buf, sizeof(buf),
		    "There are %d gold and %d silver coins in the pile.\n\r",
		    obj->value[1],obj->value[0]);
	    send_to_char(buf,ch);
	    break;

	case ITEM_DRINK_CON:
	case ITEM_CONTAINER:
	case ITEM_CORPSE_NPC:
	case ITEM_CORPSE_PC:
	    snprintf(buf, sizeof(buf),"in %s",argument);
	    do_look( ch, buf );
	}
    }

    return;
}



/*
 * Thanks to Zrin for auto-exit part.
 */
void do_exits( Character *ch, char *argument )
{
    extern const char * const dir_name[];
    char buf[MAX_STRING_LENGTH];
    EXIT_DATA *pexit;
    bool found;
    bool fAuto;
    int door;

    fAuto  = !str_cmp( argument, "auto" );

    if ( !check_blind( ch ) )
	return;

    if (fAuto)
	snprintf(buf, sizeof(buf),"[Exits:");
    else if (IS_IMMORTAL(ch))
	snprintf(buf, sizeof(buf),"Obvious exits from room %d:\n\r",ch->in_room->vnum);
    else
	snprintf(buf, sizeof(buf),"Obvious exits:\n\r");

    found = FALSE;
    for ( door = 0; door <= 5; door++ )
    {
	if ( ( pexit = ch->in_room->exit[door] ) != NULL
	&&   pexit->u1.to_room != NULL
	&&   can_see_room(ch,pexit->u1.to_room) 
	&&   !IS_SET(pexit->exit_info, EX_CLOSED) )
	{
	    found = TRUE;
	    if ( fAuto )
	    {
		strcat( buf, " " );
		strcat( buf, dir_name[door] );
	    }
	    else
	    {
		snprintf(buf + strlen(buf), sizeof(buf) + strlen(buf), "%-5s - %s",
		    capitalize( dir_name[door] ),
		    room_is_dark( pexit->u1.to_room )
			?  "Too dark to tell"
			: pexit->u1.to_room->name
		    );
		if (IS_IMMORTAL(ch))
		    snprintf(buf + strlen(buf),sizeof(buf) + strlen(buf), 
			" (room %d)\n\r",pexit->u1.to_room->vnum);
		else
		    snprintf(buf + strlen(buf),sizeof(buf) + strlen(buf), "\n\r");
	    }
	}
    }

    if ( !found )
	strcat( buf, fAuto ? " none" : "None.\n\r" );

    if ( fAuto )
	strcat( buf, "]\n\r" );

    send_to_char( buf, ch );
    return;
}

void do_worth( Character *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];

    if (IS_NPC(ch))
    {
	snprintf(buf, sizeof(buf),"You have %ld gold and %ld silver.\n\r",
	    ch->gold,ch->silver);
	send_to_char(buf,ch);
	return;
    }

    snprintf(buf, sizeof(buf), 
    "You have %ld gold, %ld silver, and %d experience (%d exp to level).\n\r",
	ch->gold, ch->silver,ch->exp,
	(ch->level + 1) * exp_per_level(ch,ch->pcdata->points) - ch->exp);

    send_to_char(buf,ch);

    return;
}


void do_score( Character *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    int i;

    snprintf(buf, sizeof(buf),
	"You are %s%s, level %d, %d years old (%d hours).\n\r",
	ch->getName(),
	IS_NPC(ch) ? "" : ch->pcdata->title,
	ch->level, get_age(ch),
        ( ch->played + (int) (current_time - ch->logon) ) / 3600);
    send_to_char( buf, ch );

    if ( get_trust( ch ) != ch->level )
    {
	snprintf(buf, sizeof(buf), "You are trusted at level %d.\n\r",
	    get_trust( ch ) );
	send_to_char( buf, ch );
    }

    snprintf(buf, sizeof(buf), "Race: %s  Sex: %s  Class: %s",
	race_table[ch->race].name,
	ch->sex == 0 ? "sexless" : ch->sex == 1 ? "male" : "female",
 	IS_NPC(ch) ? "mobile" : class_table[ch->class_num].name);
    send_to_char(buf,ch);
	
    if (ch->race == race_lookup("draconian"))
    {
	snprintf(buf, sizeof(buf), "  Colour: %s", draconian_table[ch->drac].colour );
	buf[10] = UPPER(buf[10]);
	send_to_char(buf,ch);
    }

    send_to_char("\n\r",ch);

    if (ch->race == race_lookup("werefolk"))
    {
	snprintf(buf, sizeof(buf), "You are currently in your %s form.\n\r",
	    IS_SET(ch->act, PLR_IS_MORPHED) ?
		morph_table[ch->morph_form].who_name :
		pc_race_table[ch->orig_form].who_name);
	send_to_char(buf,ch);
    }

    snprintf(buf, sizeof(buf),
	"You have %d/%d hit, %d/%d mana, %d/%d movement.\n\r",
	ch->hit,  ch->max_hit,
	ch->mana, ch->max_mana,
	ch->move, ch->max_move);
    send_to_char( buf, ch );

    snprintf(buf, sizeof(buf),
	"You have %d practices and %d training sessions.\n\r",
	ch->practice, ch->train);
    send_to_char( buf, ch );

    snprintf(buf, sizeof(buf),
	"You are carrying %d/%d items with weight %ld/%d pounds.\n\r",
	ch->carry_number, can_carry_n(ch),
	get_carry_weight(ch) / 10, can_carry_w(ch) /10 );
    send_to_char( buf, ch );

    snprintf(buf, sizeof(buf),
	"Str: %d(%d)  Int: %d(%d)  Wis: %d(%d)  Dex: %d(%d)  Con: %d(%d)\n\r",
	ch->perm_stat[STAT_STR],
	get_curr_stat(ch,STAT_STR),
	ch->perm_stat[STAT_INT],
	get_curr_stat(ch,STAT_INT),
	ch->perm_stat[STAT_WIS],
	get_curr_stat(ch,STAT_WIS),
	ch->perm_stat[STAT_DEX],
	get_curr_stat(ch,STAT_DEX),
	ch->perm_stat[STAT_CON],
	get_curr_stat(ch,STAT_CON) );
    send_to_char( buf, ch );

    snprintf(buf, sizeof(buf),
	"You have scored %d exp, and have %ld gold and %ld silver coins.\n\r",
	ch->exp,  ch->gold, ch->silver );
    send_to_char( buf, ch );

    /* RT shows exp to level */
    if (!IS_NPC(ch) && ch->level < LEVEL_HERO)
    {
      snprintf(buf, sizeof(buf), 
	"You need %d exp to level.\n\r",
	((ch->level + 1) * exp_per_level(ch,ch->pcdata->points) - ch->exp));
      send_to_char( buf, ch );
     }

    snprintf(buf, sizeof(buf), "Wimpy set to %d hit points.\n\r", ch->wimpy );
    send_to_char( buf, ch );

    if ( !IS_NPC(ch) && ch->pcdata->condition[COND_DRUNK]   > 10 )
	send_to_char( "You are drunk.\n\r",   ch );
    if ( !IS_NPC(ch) && ch->pcdata->condition[COND_THIRST] ==  0 )
	send_to_char( "You are thirsty.\n\r", ch );
    if ( !IS_NPC(ch) && ch->pcdata->condition[COND_HUNGER]   ==  0 )
	send_to_char( "You are hungry.\n\r",  ch );

    switch ( ch->position )
    {
    case POS_DEAD:     
	send_to_char( "You are DEAD!!\n\r",		ch );
	break;
    case POS_MORTAL:
	send_to_char( "You are mortally wounded.\n\r",	ch );
	break;
    case POS_INCAP:
	send_to_char( "You are incapacitated.\n\r",	ch );
	break;
    case POS_STUNNED:
	send_to_char( "You are stunned.\n\r",		ch );
	break;
    case POS_SLEEPING:
	send_to_char( "You are sleeping.\n\r",		ch );
	break;
    case POS_RESTING:
	send_to_char( "You are resting.\n\r",		ch );
	break;
    case POS_SITTING:
	send_to_char( "You are sitting.\n\r",		ch );
	break;
    case POS_STANDING:
	send_to_char( "You are standing.\n\r",		ch );
	break;
    case POS_FIGHTING:
	send_to_char( "You are fighting.\n\r",		ch );
	break;
    }


    /* print AC values */
    if (ch->level >= 25)
    {	
	snprintf(buf, sizeof(buf),"Armor: pierce: %d  bash: %d  slash: %d  magic: %d\n\r",
		 GET_AC(ch,AC_PIERCE),
		 GET_AC(ch,AC_BASH),
		 GET_AC(ch,AC_SLASH),
		 GET_AC(ch,AC_EXOTIC));
    	send_to_char(buf,ch);
    }

    for (i = 0; i < 4; i++)
    {
	char * temp;

	switch(i)
	{
	    case(AC_PIERCE):	temp = (char*)"piercing";	break;
	    case(AC_BASH):	temp = (char*)"bashing";	break;
	    case(AC_SLASH):	temp = (char*)"slashing";	break;
	    case(AC_EXOTIC):	temp = (char*)"magic";		break;
	    default:		temp = (char*)"error";		break;
	}
	
	send_to_char("You are ", ch);

	if      (GET_AC(ch,i) >=  101 ) 
	    snprintf(buf, sizeof(buf),"hopelessly vulnerable to %s.\n\r",temp);
	else if (GET_AC(ch,i) >= 80) 
	    snprintf(buf, sizeof(buf),"defenseless against %s.\n\r",temp);
	else if (GET_AC(ch,i) >= 60)
	    snprintf(buf, sizeof(buf),"barely protected from %s.\n\r",temp);
	else if (GET_AC(ch,i) >= 40)
	    snprintf(buf, sizeof(buf),"slightly armored against %s.\n\r",temp);
	else if (GET_AC(ch,i) >= 20)
	    snprintf(buf, sizeof(buf),"somewhat armored against %s.\n\r",temp);
	else if (GET_AC(ch,i) >= 0)
	    snprintf(buf, sizeof(buf),"armored against %s.\n\r",temp);
	else if (GET_AC(ch,i) >= -20)
	    snprintf(buf, sizeof(buf),"well-armored against %s.\n\r",temp);
	else if (GET_AC(ch,i) >= -40)
	    snprintf(buf, sizeof(buf),"very well-armored against %s.\n\r",temp);
	else if (GET_AC(ch,i) >= -60)
	    snprintf(buf, sizeof(buf),"heavily armored against %s.\n\r",temp);
	else if (GET_AC(ch,i) >= -80)
	    snprintf(buf, sizeof(buf),"superbly armored against %s.\n\r",temp);
	else if (GET_AC(ch,i) >= -100)
	    snprintf(buf, sizeof(buf),"almost invulnerable to %s.\n\r",temp);
	else
	    snprintf(buf, sizeof(buf),"divinely armored against %s.\n\r",temp);

	send_to_char(buf,ch);
    }


    /* RT wizinvis and holy light */
    if ( IS_IMMORTAL(ch))
    {
      send_to_char("Holy Light: ",ch);
      if (IS_SET(ch->act,PLR_HOLYLIGHT))
        send_to_char("on",ch);
      else
        send_to_char("off",ch);
 
      if (ch->invis_level)
      {
        snprintf(buf, sizeof(buf), "  Invisible: level %d",ch->invis_level);
        send_to_char(buf,ch);
      }

      if (ch->incog_level)
      {
	snprintf(buf, sizeof(buf),"  Incognito: level %d",ch->incog_level);
	send_to_char(buf,ch);
      }
      send_to_char("\n\r",ch);
    }

    if ( ch->level >= 15 )
    {
	snprintf(buf, sizeof(buf), "Hitroll: %d  Damroll: %d.\n\r",
	    GET_HITROLL(ch), GET_DAMROLL(ch) );
	send_to_char( buf, ch );
    }
    
    if (IS_SET(ch->comm,COMM_SHOW_AFFECTS))
	do_affects(ch,(char*)"");
}

void do_affects(Character *ch, char *argument )
{
    AFFECT_DATA *paf, *paf_last = NULL;
    char buf[MAX_STRING_LENGTH];
    
    if ( ch->affected != NULL )
    {
	send_to_char( "You are affected by the following spells:\n\r", ch );
	for ( paf = ch->affected; paf != NULL; paf = paf->next )
	{
	    if (paf_last != NULL && paf->type == paf_last->type)
	    {
		if (ch->level >= 20)
		    snprintf(buf, sizeof(buf), "                      ");
		else
		    continue;
	    }
	    else
	    	snprintf(buf, sizeof(buf), "Spell: %-15s", skill_table[paf->type].name );

	    send_to_char( buf, ch );

	    if ( ch->level >= 20 )
	    {
		snprintf(buf, sizeof(buf),
		    ": modifies %s by %d ",
		    affect_loc_name( paf->location ),
		    paf->modifier);
		send_to_char( buf, ch );
		if ( paf->duration == -1 )
		    snprintf(buf, sizeof(buf), "permanently" );
		else
		    snprintf(buf, sizeof(buf), "for %d hours", paf->duration );
		send_to_char( buf, ch );
	    }

	    send_to_char( "\n\r", ch );
	    paf_last = paf;
	}
    }
    else 
	send_to_char("You are not affected by any spells.\n\r",ch);

    return;
}



const char *	const	day_name	[] =
{
    "the Moon", "the Bull", "Deception", "Thunder", "Freedom",
    "the Great Gods", "the Sun"
};

const char *	const	month_name	[] =
{
    "Winter", "the Winter Wolf", "the Frost Giant", "the Old Forces",
    "the Grand Struggle", "the Spring", "Nature", "Futility", "the Dragon",
    "the Sun", "the Heat", "the Battle", "the Dark Shades", "the Shadows",
    "the Long Shadows", "the Ancient Darkness", "the Great Evil"
};

void do_time( Character *ch, char *argument )
{
    extern char str_boot_time[];
    char buf[MAX_STRING_LENGTH];
    char *suf = new char[3];
    int day;

    day     = time_info.day + 1;

         if ( day > 4 && day <  20 ) strcpy( suf, "th" );
    else if ( day % 10 ==  1       ) strcpy( suf, "st" );
    else if ( day % 10 ==  2       ) strcpy( suf, "nd" );
    else if ( day % 10 ==  3       ) strcpy( suf, "rd" );
    else                             strcpy( suf, "th" );

    snprintf(buf, sizeof(buf),
	"It is %d o'clock %s, Day of %s, %d%s the Month of %s.\n\r",
	(time_info.hour % 12 == 0) ? 12 : time_info.hour %12,
	time_info.hour >= 12 ? "pm" : "am",
	day_name[day % 7],
	day, suf,
	month_name[time_info.month]);
    send_to_char(buf,ch);
    snprintf(buf, sizeof(buf),"Forgotten Legends started up at %s\n\rThe system time is %s.\n\r",
	str_boot_time,
	(char *) ctime( &current_time )
	);

    send_to_char( buf, ch );
    delete [] suf;
    return;
}



void do_weather( Character *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];

    static const char * const sky_look[4] =
    {
	"cloudless",
	"cloudy",
	"rainy",
	"lit by flashes of lightning"
    };

    if ( !IS_OUTSIDE(ch) )
    {
	send_to_char( "You can't see the weather indoors.\n\r", ch );
	return;
    }

    snprintf(buf, sizeof(buf), "The sky is %s and %s.\n\r",
	sky_look[weather_info.sky],
	weather_info.change >= 0
	? "a warm southerly breeze blows"
	: "a cold northern gust blows"
	);
    send_to_char( buf, ch );
    return;
}

void do_help( Character *ch, char *argument )
{
    HELP_DATA *pHelp;
    BUFFER *output;
    bool found = FALSE;
    char argall[MAX_INPUT_LENGTH],argone[MAX_INPUT_LENGTH];
    int level;

    output = new_buf();

    if ( argument[0] == '\0' ) {
        argument = (char*)"summary";
    }

    /* this parts handles help a b so that it returns help 'a b' */
    argall[0] = '\0';
    while (argument[0] != '\0' )
    {
	argument = one_argument(argument,argone);
	if (argall[0] != '\0')
	    strcat(argall," ");
	strcat(argall,argone);
    }

    for ( pHelp = help_first; pHelp != NULL; pHelp = pHelp->next )
    {
    	level = (pHelp->level < 0) ? -1 * pHelp->level - 1 : pHelp->level;

	if (level > get_trust( ch ) )
	    continue;

	if ( is_name( argall, pHelp->keyword ) )
	{
	    /* add seperator if found */
	    if (found)
		add_buf(output,
    "\n\r============================================================\n\r\n\r");
	    if ( pHelp->level >= 0 && str_cmp( argall, "imotd" ) )
	    {
		add_buf(output,pHelp->keyword);
		add_buf(output,"\n\r");
	    }

	    /*
	     * Strip leading '.' to allow initial blanks.
	     */
	    if ( pHelp->text[0] == '.' )
		add_buf(output,pHelp->text+1);
	    else
		add_buf(output,pHelp->text);
	    found = TRUE;
	    /* small hack :) */
	    if (ch->desc != NULL && ch->desc->connected != CON_PLAYING 
	    &&  		    ch->desc->connected != CON_GEN_GROUPS)
		break;
	}
    }

    if (!found)
    {
    	send_to_char( "No help on that word.\n\r", ch );
	append_file(ch, HELP_ERR, argall);
    }
    else
	page_to_char(buf_string(output),ch);
    free_buf(output);
}


/* whois command */
void do_whois (Character *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    BUFFER *output;
    char buf[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;
    bool found = FALSE;

    one_argument(argument,arg);
  
    if (arg[0] == '\0')
    {
	send_to_char("You must provide a name.\n\r",ch);
	return;
    }

    output = new_buf();

    for (d = descriptor_list; d != NULL; d = d->next)
    {
	Character *wch;
        char *race;
	char const *class_name;

 	if (d->connected != CON_PLAYING || !can_see(ch,d->character))
	    continue;
	
	wch = ( d->original != NULL ) ? d->original : d->character;

 	if (!can_see(ch,wch))
	    continue;

	if (!str_prefix(arg,wch->getName()))
	{
	    found = TRUE;
	    
	    /* work out the printing */
	    class_name = class_table[wch->class_num].who_name;
	    switch(wch->level)
	    {
		case MAX_LEVEL - 0 : class_name = "IMP"; 	break;
		case MAX_LEVEL - 1 : class_name = "CRE";	break;
		case MAX_LEVEL - 2 : class_name = "SUP";	break;
		case MAX_LEVEL - 3 : class_name = "DEI";	break;
		case MAX_LEVEL - 4 : class_name = "GOD";	break;
		case MAX_LEVEL - 5 : class_name = "IMM";	break;
		case MAX_LEVEL - 6 : class_name = "DEM";	break;
		case MAX_LEVEL - 7 : class_name = "ANG";	break;
		case MAX_LEVEL - 8 : class_name = "AVA";	break;
	    }
    
	    race = str_dup(pc_race_table[wch->race].who_name);
	    if (wch->race == race_lookup("werefolk"))
	    {
		if (IS_SET(wch->act, PLR_IS_MORPHED))
		    race = str_dup(morph_table[wch->morph_form].who_name);
		else
		    race = str_dup(pc_race_table[wch->orig_form].who_name);
	    }

	    /* a little formatting */
	    snprintf(buf, sizeof(buf), "{w[{R%2d {y%6s {W%s{w]{x %s%s%s%s%s%s%s%s\n\r",
		wch->level, race, class_name,
	     wch->incog_level >= LEVEL_HERO ? "(Incog) ": "",
 	     wch->invis_level >= LEVEL_HERO ? "(Wizi) " : "",
	     IS_CLANNED(wch) && (IS_IMMORTAL(ch) || wch->pcdata->clan == ch->pcdata->clan || wch->in_room == ch->in_room) ? wch->pcdata->clan->whoname : "",
	     IS_SET(wch->comm, COMM_AFK) ? "[AFK] " : "",
             (IS_CLANNED(ch) && IS_CLANNED(wch) && IS_SET(wch->pcdata->clan->flags, CLAN_PK) && IS_SET(ch->pcdata->clan->flags, CLAN_PK) && wch->level + wch->getRange() >= ch->level ) ? "{R({rPK{R){x " : "",
             IS_SET(wch->act,PLR_THIEF) ? "{B({YTHIEF{B){x " : "",
		wch->getName(), IS_NPC(wch) ? "" : wch->pcdata->title);
	    add_buf(output,buf);
	}
    }

    if (!found)
    {
	send_to_char("No one of that name is playing.\n\r",ch);
	return;
    }

    page_to_char(buf_string(output),ch);
    free_buf(output);
}


/*
 * New 'who' command originally by Alander of Rivers of Mud.
 */
void do_who( Character *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];
    BUFFER *output;
    DESCRIPTOR_DATA *d;
    int iClass;
    int iRace;
    CLAN_DATA *iClan;
    int iLevelLower;
    int iLevelUpper;
    int nNumber;
    int nMatch;
    bool rgfClass[MAX_CLASS];
    bool rgfRace[MAX_PC_RACE];
    bool fClassRestrict = FALSE;
    bool fClanRestrict = FALSE;
    bool fClan = FALSE;
    bool fRaceRestrict = FALSE;
    bool fImmortalOnly = FALSE;
 
    /*
     * Set default arguments.
     */
    iLevelLower    = 0;
    iLevelUpper    = MAX_LEVEL;
    for ( iClass = 0; iClass < MAX_CLASS; iClass++ )
        rgfClass[iClass] = FALSE;
    for ( iRace = 0; iRace < MAX_PC_RACE; iRace++ )
        rgfRace[iRace] = FALSE;
 
    /*
     * Parse arguments.
     */
    nNumber = 0;
    for ( ;; )
    {
        char arg[MAX_STRING_LENGTH];
 
        argument = one_argument( argument, arg );
        if ( arg[0] == '\0' )
            break;
 
        if ( is_number( arg ) )
        {
            switch ( ++nNumber )
            {
            case 1: iLevelLower = atoi( arg ); break;
            case 2: iLevelUpper = atoi( arg ); break;
            default:
                send_to_char( "Only two level numbers allowed.\n\r", ch );
                return;
            }
        }
        else
        {
 
            /*
             * Look for classes to turn on.
             */
            if (!str_prefix(arg,"immortals"))
            {
                fImmortalOnly = TRUE;
            }
            else
            {
                iClass = class_lookup(arg);
                if (iClass == -1)
                {
                    iRace = race_lookup(arg);
 
                    if (iRace == 0 || iRace >= MAX_PC_RACE)
		    {
			if (!str_prefix(arg,"clan"))
			    fClan = TRUE;
			else
		        {
			    iClan = get_clan(arg);
			    if (iClan)
			    {
				fClanRestrict = TRUE;
			    }
			    else
			    {
                        	send_to_char(
                            	"That's not a valid race, class, or clan.\n\r",
				   ch);
                            	return;
			    }
                        }
		    }
                    else
                    {
                        fRaceRestrict = TRUE;
                        rgfRace[iRace] = TRUE;
                    }
                }
                else
                {
                    fClassRestrict = TRUE;
                    rgfClass[iClass] = TRUE;
                }
            }
        }
    }
 
    /*
     * Now show matching chars.
     */
    nMatch = 0;
    buf[0] = '\0';
    output = new_buf();
    for ( d = descriptor_list; d != NULL; d = d->next )
    {
        Character *wch;
        char const *class_name;
	char *race;
 
        /*
         * Check for match against restrictions.
         * Don't use trust as that exposes trusted mortals.
         */
        if ( d->connected != CON_PLAYING || !can_see( ch, d->character ) )
            continue;
 
        wch   = ( d->original != NULL ) ? d->original : d->character;

	if (!can_see(ch,wch))
	    continue;

        if ( wch->level < iLevelLower
        ||   wch->level > iLevelUpper
        || ( fImmortalOnly  && wch->level < LEVEL_IMMORTAL )
        || ( fClassRestrict && !rgfClass[wch->class_num] )
        || ( fRaceRestrict && !rgfRace[wch->race])
 	|| ( fClan && !IS_IMMORTAL(ch) &&
		(!IS_CLANNED(wch) ||
		(wch->pcdata->clan != ch->pcdata->clan &&
		ch->in_room != wch->in_room) ) )
	|| ( (fClanRestrict && !IS_IMMORTAL(ch)) &&
		(wch->pcdata->clan != ch->pcdata->clan &&
		wch->in_room != ch->in_room)))
            continue;
 
        nMatch++;
 
        /*
         * Figure out what to print for class.
	 */
	class_name = class_table[wch->class_num].who_name;
	switch ( wch->level )
	{
	default: break;
            {
                case MAX_LEVEL - 0 : class_name = "IMP";     break;
                case MAX_LEVEL - 1 : class_name = "CRE";     break;
                case MAX_LEVEL - 2 : class_name = "SUP";     break;
                case MAX_LEVEL - 3 : class_name = "DEI";     break;
                case MAX_LEVEL - 4 : class_name = "GOD";     break;
                case MAX_LEVEL - 5 : class_name = "IMM";     break;
                case MAX_LEVEL - 6 : class_name = "DEM";     break;
                case MAX_LEVEL - 7 : class_name = "ANG";     break;
                case MAX_LEVEL - 8 : class_name = "AVA";     break;
            }
	}

	    race = str_dup(pc_race_table[wch->race].who_name);
	    if (wch->race == race_lookup("werefolk"))
	    {
		if (IS_SET(wch->act, PLR_IS_MORPHED))
		    race = str_dup(morph_table[wch->morph_form].who_name);
		else
		    race = str_dup(pc_race_table[wch->orig_form].who_name);
	    }

	/*
	 * Format it up.
	 */
	snprintf(buf, sizeof(buf), "{w[{R%2d {y%6s {W%s{w]{x %s%s%s%s%s%s%s%s\n\r",
	    wch->level, race, class_name,
	    wch->incog_level >= LEVEL_HERO ? "(Incog) " : "",
	    wch->invis_level >= LEVEL_HERO ? "(Wizi) " : "",
	    IS_CLANNED(wch) && (IS_IMMORTAL(ch) || wch->pcdata->clan == ch->pcdata->clan || wch->in_room == ch->in_room) ? wch->pcdata->clan->whoname : "",
	    IS_SET(wch->comm, COMM_AFK) ? "[AFK] " : "",
             (IS_CLANNED(ch) && IS_CLANNED(wch) && IS_SET(wch->pcdata->clan->flags, CLAN_PK) && IS_SET(ch->pcdata->clan->flags, CLAN_PK) && wch->level + wch->getRange() >= ch->level ) ? "{R({rPK{R){x " : "",
            IS_SET(wch->act, PLR_THIEF)  ? "{B({YTHIEF{B){x "  : "",
	    wch->getName(),
	    IS_NPC(wch) ? "" : wch->pcdata->title );
	add_buf(output,buf);
    }

    snprintf(buf2, sizeof(buf2), "\n\rPlayers found: %d\n\r", nMatch );
    add_buf(output,buf2);
    page_to_char( buf_string(output), ch );
    free_buf(output);
    return;
}

void do_count ( Character *ch, char *argument )
{
    int count;
    DESCRIPTOR_DATA *d;
    char buf[MAX_STRING_LENGTH];

    count = 0;

    for ( d = descriptor_list; d != NULL; d = d->next )
        if ( d->connected == CON_PLAYING && can_see( ch, d->character ) )
	    count++;

    max_on = UMAX(count,max_on);

    if (max_on == count)
        snprintf(buf, sizeof(buf),"There are %d characters on, the most so far today.\n\r",
	    count);
    else
	snprintf(buf, sizeof(buf),"There are %d characters on, the most on today was %d.\n\r",
	    count,max_on);

    send_to_char(buf,ch);
}

void do_inventory( Character *ch, char *argument )
{
    send_to_char( "You are carrying:\n\r", ch );
    show_list_to_char( ch->carrying, ch, TRUE, TRUE );
    return;
}

void do_compare( Character *ch, char *argument )
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    OBJ_DATA *obj1;
    OBJ_DATA *obj2;
    int value1;
    int value2;
    char *msg = new char[MAX_STRING_LENGTH];

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    if ( arg1[0] == '\0' )
    {
	send_to_char( "Compare what to what?\n\r", ch );
	return;
    }

    if ( ( obj1 = get_obj_carry( ch, arg1, ch ) ) == NULL )
    {
	send_to_char( "You do not have that item.\n\r", ch );
	return;
    }

    if (arg2[0] == '\0')
    {
	for (obj2 = ch->carrying; obj2 != NULL; obj2 = obj2->next_content)
	{
	    if (obj2->wear_loc != WEAR_NONE
	    &&  can_see_obj(ch,obj2)
	    &&  obj1->item_type == obj2->item_type
	    &&  (obj1->wear_flags & obj2->wear_flags & ~ITEM_TAKE) != 0 )
		break;
	}

	if (obj2 == NULL)
	{
	    send_to_char("You aren't wearing anything comparable.\n\r",ch);
	    return;
	}
    } 

    else if ( (obj2 = get_obj_carry(ch,arg2,ch) ) == NULL )
    {
	send_to_char("You do not have that item.\n\r",ch);
	return;
    }

    msg		= NULL;
    value1	= 0;
    value2	= 0;

    if ( obj1 == obj2 )
    {
	   strncpy( msg, "You compare $p to itself.  It looks about the same.", MAX_STRING_LENGTH );
    }
    else if ( obj1->item_type != obj2->item_type )
    {
	   strncpy( msg, "You can't compare $p and $P.", MAX_STRING_LENGTH );
    }
    else
    {
	switch ( obj1->item_type )
	{
	default:
	    strncpy( msg, "You can't compare $p and $P.", MAX_STRING_LENGTH );
	    break;

	case ITEM_ARMOR:
	    value1 = obj1->value[0] + obj1->value[1] + obj1->value[2];
	    value2 = obj2->value[0] + obj2->value[1] + obj2->value[2];
	    break;

	case ITEM_WEAPON:
	    if (obj1->pIndexData->new_format)
		value1 = (1 + obj1->value[2]) * obj1->value[1];
	    else
	    	value1 = obj1->value[1] + obj1->value[2];

	    if (obj2->pIndexData->new_format)
		value2 = (1 + obj2->value[2]) * obj2->value[1];
	    else
	    	value2 = obj2->value[1] + obj2->value[2];
	    break;
	}
    }

    if ( msg == NULL )
    {
	     if ( value1 == value2 ) strncpy( msg, "$p and $P look about the same.", MAX_STRING_LENGTH );
	else if ( value1  > value2 ) strncpy( msg, "$p looks better than $P.", MAX_STRING_LENGTH );
	else                         strncpy( msg, "$p looks worse than $P.", MAX_STRING_LENGTH );
    }

    act( msg, ch, obj1, obj2, TO_CHAR );
    delete [] msg;
    return;
}



void do_credits( Character *ch, char *argument )
{
    do_help( ch, (char*)"diku" );
    return;
}



void do_where( Character *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    Character *victim;
    DESCRIPTOR_DATA *d;
    bool found;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	send_to_char( "Players near you:\n\r", ch );
	found = FALSE;
	for ( d = descriptor_list; d; d = d->next )
	{
	    if ( d->connected == CON_PLAYING
	    && ( victim = d->character ) != NULL
	    &&   !IS_NPC(victim)
	    &&   victim->in_room != NULL
	    &&   !IS_SET(victim->in_room->room_flags,ROOM_NOWHERE)
 	    &&   (is_room_owner(ch,victim->in_room) 
	    ||    !room_is_private(victim->in_room))
	    &&   victim->in_room->area == ch->in_room->area
	    &&   can_see( ch, victim ) )
	    {
		found = TRUE;
		snprintf(buf, sizeof(buf), "%-28s %s\n\r",
		    victim->getName(), victim->in_room->name );
		send_to_char( buf, ch );
	    }
	}
	if ( !found )
	    send_to_char( "None\n\r", ch );
    }
    else
    {
	found = FALSE;
    for (std::list<Character*>::iterator it = char_list.begin(); it != char_list.end(); it++)
	{
       victim = *it;
	    if ( victim->in_room != NULL
	    &&   victim->in_room->area == ch->in_room->area
	    &&   !IS_AFFECTED(victim, AFF_HIDE)
	    &&   !IS_AFFECTED(victim, AFF_SNEAK)
	    &&   can_see( ch, victim )
	    &&   is_name( arg, victim->getName() ) )
	    {
		found = TRUE;
		snprintf(buf, sizeof(buf), "%-28s %s\n\r",
		    PERS(victim, ch), victim->in_room->name );
		send_to_char( buf, ch );
		break;
	    }
	}
	if ( !found )
	    act( "You didn't find any $T.", ch, NULL, arg, TO_CHAR );
    }

    return;
}




void do_consider( Character *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    Character *victim;
    char *msg = new char[MAX_STRING_LENGTH];
    int diff;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	send_to_char( "Consider killing whom?\n\r", ch );
	return;
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL )
    {
	send_to_char( "They're not here.\n\r", ch );
	return;
    }

    if (is_safe(ch,victim))
    {
	send_to_char("Don't even think about it.\n\r",ch);
	return;
    }

    diff = victim->level - ch->level;

         if ( diff <= -10 ) strncpy( msg, "You can kill $N naked and weaponless.", MAX_STRING_LENGTH );
    else if ( diff <=  -5 ) strncpy( msg, "$N is no match for you.", MAX_STRING_LENGTH );
    else if ( diff <=  -2 ) strncpy( msg, "$N looks like an easy kill.", MAX_STRING_LENGTH );
    else if ( diff <=   1 ) strncpy( msg, "The perfect match!", MAX_STRING_LENGTH );
    else if ( diff <=   4 ) strncpy( msg, "$N says 'Do you feel lucky, punk?'.", MAX_STRING_LENGTH );
    else if ( diff <=   9 ) strncpy( msg, "$N laughs at you mercilessly.", MAX_STRING_LENGTH );
    else                    strncpy( msg, "Death will thank you for your gift.", MAX_STRING_LENGTH );

    act( msg, ch, NULL, victim, TO_CHAR );
    delete [] msg;
    return;
}



void set_title( Character *ch, char *title )
{
    char buf[MAX_STRING_LENGTH];

    if ( IS_NPC(ch) )
    {
	bug( "Set_title: NPC.", 0 );
	return;
    }

    if ( title[0] != '.' && title[0] != ',' && title[0] != '!' && title[0] != '?' )
    {
	buf[0] = ' ';
	strcpy( buf+1, title );
    }
    else
    {
	strcpy( buf, title );
    }

    free_string( ch->pcdata->title );
    ch->pcdata->title = str_dup( buf );
    return;
}



void do_title( Character *ch, char *argument )
{
    if ( IS_NPC(ch) )
	return;

    if ( argument[0] == '\0' )
    {
	send_to_char( "Change your title to what?\n\r", ch );
	return;
    }

    if ( strlen_color(argument) > 45 )
	argument[45] = '\0';

    smash_tilde( argument );
    set_title( ch, argument );
    send_to_char( "Ok.\n\r", ch );
}



void do_description( Character *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];

    if ( argument[0] != '\0' )
    {
	buf[0] = '\0';
	smash_tilde( argument );

    	if (argument[0] == '-')
    	{
            int len;
            bool found = FALSE;
 
            if (ch->getDescription() == NULL || ch->getDescription()[0] == '\0')
            {
                send_to_char("No lines left to remove.\n\r",ch);
                return;
            }
	
  	    strcpy(buf,ch->getDescription());
 
            for (len = strlen(buf); len > 0; len--)
            {
                if (buf[len] == '\r')
                {
                    if (!found)  /* back it up */
                    {
                        if (len > 0)
                            len--;
                        found = TRUE;
                    }
                    else /* found the second one */
                    {
                        buf[len + 1] = '\0';
			ch->setDescription(buf);
			send_to_char( "Your description is:\n\r", ch );
			send_to_char( ch->getDescription() ? ch->getDescription() : 
			    "(None).\n\r", ch );
                        return;
                    }
                }
            }
            buf[0] = '\0';
	    ch->setDescription(buf);
	    send_to_char("Description cleared.\n\r",ch);
	    return;
        }
	if ( argument[0] == '+' )
	{
	    if ( ch->getDescription() != NULL )
		strcat( buf, ch->getDescription() );
	    argument++;
	    while ( isspace(*argument) )
		argument++;
	}

        if ( strlen(buf) >= 1024)
	{
	    send_to_char( "Description too long.\n\r", ch );
	    return;
	}

	strcat( buf, argument );
	strcat( buf, "\n\r" );
    ch->setDescription( buf );
    }

    send_to_char( "Your description is:\n\r", ch );
    send_to_char( ch->getDescription() ? ch->getDescription() : "(None).\n\r", ch );
    return;
}



void do_report( Character *ch, char *argument )
{
    char buf[MAX_INPUT_LENGTH];

    snprintf(buf, sizeof(buf),
	"You say 'I have %d/%d hp %d/%d mana %d/%d mv %d xp.'\n\r",
	ch->hit,  ch->max_hit,
	ch->mana, ch->max_mana,
	ch->move, ch->max_move,
	ch->exp   );

    send_to_char( buf, ch );

    snprintf(buf, sizeof(buf), "$n says 'I have %d/%d hp %d/%d mana %d/%d mv %d xp.'",
	ch->hit,  ch->max_hit,
	ch->mana, ch->max_mana,
	ch->move, ch->max_move,
	ch->exp   );

    act( buf, ch, NULL, NULL, TO_ROOM );

    return;
}



void do_practice( Character *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    int sn;

    if ( IS_NPC(ch) )
	return;

    if ( argument[0] == '\0' )
    {
	int col;

	col    = 0;
	for ( sn = 0; sn < MAX_SKILL; sn++ )
	{
	    if ( skill_table[sn].name == NULL )
		break;
	    if ( ch->level < ch->pcdata->sk_level[sn]
	      || ch->pcdata->learned[sn] < 1 /* skill is not known */)
		continue;

	    snprintf(buf, sizeof(buf), "%-18s %3d%%  ",
		skill_table[sn].name, ch->pcdata->learned[sn] );
	    send_to_char( buf, ch );
	    if ( ++col % 3 == 0 )
		send_to_char( "\n\r", ch );
	}

	if ( col % 3 != 0 )
	    send_to_char( "\n\r", ch );

	snprintf(buf, sizeof(buf), "You have %d practice sessions left.\n\r",
	    ch->practice );
	send_to_char( buf, ch );
    }
    else
    {
	Character *mob;
	int adept;

	if ( !IS_AWAKE(ch) )
	{
	    send_to_char( "In your dreams, or what?\n\r", ch );
	    return;
	}

	for ( mob = ch->in_room->people; mob != NULL; mob = mob->next_in_room )
	{
	    if ( IS_NPC(mob) && IS_SET(mob->act, ACT_PRACTICE) )
		break;
	}

	if ( mob == NULL )
	{
	    send_to_char( "You can't do that here.\n\r", ch );
	    return;
	}

	if ( ch->practice <= 0 )
	{
	    send_to_char( "You have no practice sessions left.\n\r", ch );
	    return;
	}

	if ( ( sn = find_spell( ch,argument ) ) < 0
	|| ( !IS_NPC(ch)
	&&   (ch->level < ch->pcdata->sk_level[sn]
 	||    ch->pcdata->learned[sn] < 1 /* skill is not known */
	||    ch->pcdata->sk_rating[sn] == 0)))
	{
	    send_to_char( "You can't practice that.\n\r", ch );
	    return;
	}

	adept = IS_NPC(ch) ? 100 : class_table[ch->class_num].skill_adept;

	if ( ch->pcdata->learned[sn] >= adept )
	{
	    snprintf(buf, sizeof(buf), "You are already learned at %s.\n\r",
		skill_table[sn].name );
	    send_to_char( buf, ch );
	}
	else
	{
	    ch->practice--;
	    ch->pcdata->learned[sn] += 
		int_app[get_curr_stat(ch,STAT_INT)].learn / 
	        ch->pcdata->sk_rating[sn];
	    if ( ch->pcdata->learned[sn] < adept )
	    {
		act( "You practice $T.",
		    ch, NULL, skill_table[sn].name, TO_CHAR );
		act( "$n practices $T.",
		    ch, NULL, skill_table[sn].name, TO_ROOM );
	    }
	    else
	    {
		ch->pcdata->learned[sn] = adept;
		act( "You are now learned at $T.",
		    ch, NULL, skill_table[sn].name, TO_CHAR );
		act( "$n is now learned at $T.",
		    ch, NULL, skill_table[sn].name, TO_ROOM );
	    }
	}
    }
    return;
}



/*
 * 'Wimpy' originally by Dionysos.
 */
void do_wimpy( Character *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    int wimpy;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
	wimpy = ch->max_hit / 5;
    else
	wimpy = atoi( arg );

    if ( wimpy < 0 )
    {
	send_to_char( "Your courage exceeds your wisdom.\n\r", ch );
	return;
    }

    if ( wimpy > ch->max_hit/2 )
    {
	send_to_char( "Such cowardice ill becomes you.\n\r", ch );
	return;
    }

    ch->wimpy	= wimpy;
    snprintf(buf, sizeof(buf), "Wimpy set to %d hit points.\n\r", wimpy );
    send_to_char( buf, ch );
    return;
}



void do_password( Character *ch, char *argument )
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char *pArg;
    char *pwdnew;
    char *p;
    char cEnd;

    if ( IS_NPC(ch) )
	return;

    /*
     * Can't use one_argument here because it smashes case.
     * So we just steal all its code.  Bleagh.
     */
    pArg = arg1;
    while ( isspace(*argument) )
	argument++;

    cEnd = ' ';
    if ( *argument == '\'' || *argument == '"' )
	cEnd = *argument++;

    while ( *argument != '\0' )
    {
	if ( *argument == cEnd )
	{
	    argument++;
	    break;
	}
	*pArg++ = *argument++;
    }
    *pArg = '\0';

    pArg = arg2;
    while ( isspace(*argument) )
	argument++;

    cEnd = ' ';
    if ( *argument == '\'' || *argument == '"' )
	cEnd = *argument++;

    while ( *argument != '\0' )
    {
	if ( *argument == cEnd )
	{
	    argument++;
	    break;
	}
	*pArg++ = *argument++;
    }
    *pArg = '\0';

    if ( arg1[0] == '\0' || arg2[0] == '\0' )
    {
	send_to_char( "Syntax: password <old> <new>.\n\r", ch );
	return;
    }

    if ( strcmp( crypt( arg1, ch->pcdata->pwd ), ch->pcdata->pwd ) )
    {
	WAIT_STATE( ch, 40 );
	send_to_char( "Wrong password.  Wait 10 seconds.\n\r", ch );
	return;
    }

    if ( strlen(arg2) < 5 )
    {
	send_to_char(
	    "New password must be at least five characters long.\n\r", ch );
	return;
    }

    /*
     * No tilde allowed because of player file format.
     */
    pwdnew = crypt( arg2, ch->getName() );
    for ( p = pwdnew; *p != '\0'; p++ )
    {
	if ( *p == '~' )
	{
	    send_to_char(
		"New password not acceptable, try again.\n\r", ch );
	    return;
	}
    }

    free_string( ch->pcdata->pwd );
    ch->pcdata->pwd = str_dup( pwdnew );
    save_char_obj( ch );
    send_to_char( "Ok.\n\r", ch );
    return;
}

void look_window( Character *ch, OBJ_DATA *obj )    /* Voltecs Window code 1998 */
{
    char 			 buf[MAX_STRING_LENGTH];
    ROOM_INDEX_DATA *window_room;   

	if ( obj->value[0] == 0 )
       {
	   snprintf(buf, sizeof(buf), "%s\n\r", obj->description );
	   send_to_char(buf, ch);
	   return;
	   }

	window_room = get_room_index( obj->value[0] );

	if ( window_room == NULL )
	   {
	   send_to_char( "!!BUG!! Window looks into a NULL room! Please report!\n\r", ch );
	   bug( "Window %d looks into a null room!!!", obj->pIndexData->vnum );
	   return;
	   }
		 
	if ( !IS_NPC(ch) )
	  {
	  send_to_char( "Looking through the window you can see ",ch);
	  send_to_char( window_room->name, ch );
	  send_to_char( "\n\r", ch);
	  show_list_to_char( window_room->contents, ch, FALSE, FALSE );
	  show_char_to_char( window_room->people,   ch );
	  return;
	  }
}

void do_nw( Character *ch, char *argument )
{
    DESCRIPTOR_DATA *d;
    CLAN_DATA *clan;
    char buf[MAX_STRING_LENGTH];

    if (IS_NPC(ch))
	return;

    snprintf(buf, sizeof(buf), "%-15s %15s\n\r", "Clan Name", "Net Worth (gold)");
    send_to_char(buf,ch);

    for (clan = first_clan; clan != NULL; clan = clan->next)
    {
	if (!IS_SET(clan->flags, CLAN_NOSHOW))
	{
	    snprintf(buf, sizeof(buf), "%-15s %-15ld\n\r", capitalize(clan->name), clan_nw_lookup(clan));
	    send_to_char(buf,ch);
	}
    }

    send_to_char("\n\r",ch);
    snprintf(buf, sizeof(buf), "%-15s %15s\n\r", "Name", "Net Worth (gold)" );
    send_to_char(buf,ch);
    for (d = descriptor_list; d != NULL; d = d->next)
    {
        PlayerCharacter* wch = (PlayerCharacter*)d->character;

        if (d->connected != CON_PLAYING || !can_see(ch,wch))
            continue;

        snprintf(buf, sizeof(buf), "%-15s %-15d\n\r", wch->getName(), nw_lookup(wch));
        send_to_char(buf,ch);
    }
}

const char *eq_worn(Character *ch, int iWear)
{
  OBJ_DATA *obj;

  if ((obj = get_eq_char(ch,iWear)) != NULL){
  
    if ( can_see_obj( ch, obj ) )
      {
	return ( format_obj_to_char(obj,ch,TRUE));
      }
    else
      {
	return( "Something");
      }
  }
  return("Nothing");
}

void do_equipment( Character *ch, char *argument )
{
  send_to_char("\n{CYou are Currently Wearing the following Eq:{x\n\r",ch);
  show_eq_char(ch,ch);
  return;
}

/* ROM 2.4 Lore Skill v 1.0. */

/*  The lore skill is to replace the one that DOES not seem to come
    with the basic STOCK rom.  This is a smaller version of identify
    as lore should not reflect a lot of specific spellcaster
    information since it is mainly used by warriors and the like.

    This file can be used.. and have fun with it.  Give me credit or
    not.. This is free.  I, however, do not take resposibility if you
    do not insert this code correctly.  Heck, it is free..  use it and
    enjoy.

    The Mage
*/


/* Lore skill by The Mage */
void do_lore( Character *ch, char *argument )
{
  char object_name[MAX_INPUT_LENGTH + 100];

  OBJ_DATA *obj;
  argument = one_argument(argument, object_name);
  if ( ( obj = get_obj_carry( ch, object_name, ch ) ) == NULL )
    {
      send_to_char( "You are not carrying that.\n\r", ch );
      return;
    }

  send_to_char("You ponder the item.\n\r",ch);
  if (number_percent() < get_skill(ch,gsn_lore) &&
      ch->level >= obj->level){

    printf_to_char(ch,
	     "Object '%s' is type %s, extra flags %s.\n\rWeight is %d, value is %d, level is %d.\n\r",

	     obj->name,
	     item_name(obj->item_type),
	     extra_bit_name( obj->extra_flags ),
	     obj->weight / 10,
	     obj->cost,
	     obj->level
	     );

    switch ( obj->item_type )
      {
      case ITEM_SCROLL: 
      case ITEM_POTION:
      case ITEM_PILL:
	printf_to_char(ch, "Some level spells of:");

	if ( obj->value[1] >= 0 && obj->value[1] < MAX_SKILL )
	  {
	    send_to_char( " '", ch );
	    send_to_char( skill_table[obj->value[1]].name, ch );
	    send_to_char( "'", ch );
	  }

	if ( obj->value[2] >= 0 && obj->value[2] < MAX_SKILL )
	  {
	    send_to_char( " '", ch );
	    send_to_char( skill_table[obj->value[2]].name, ch );
	    send_to_char( "'", ch );
	  }

	if ( obj->value[3] >= 0 && obj->value[3] < MAX_SKILL )
	  {
	    send_to_char( " '", ch );
	    send_to_char( skill_table[obj->value[3]].name, ch );
	    send_to_char( "'", ch );
	  }

	if (obj->value[4] >= 0 && obj->value[4] < MAX_SKILL)
	  {
	    send_to_char(" '",ch);
	    send_to_char(skill_table[obj->value[4]].name,ch);
	    send_to_char("'",ch);
	  }

	send_to_char( ".\n\r", ch );
	break;

      case ITEM_WAND: 
      case ITEM_STAFF: 
	printf_to_char(ch, "Has some charges of some level" );
      
	if ( obj->value[3] >= 0 && obj->value[3] < MAX_SKILL )
	  {
	    send_to_char( " '", ch );
	    send_to_char( skill_table[obj->value[3]].name, ch );
	    send_to_char( "'", ch );
	  }

	send_to_char( ".\n\r", ch );
	break;

      case ITEM_DRINK_CON:
        printf_to_char(ch,"It holds %s-colored %s.\n\r",
		liq_table[obj->value[2]].liq_color,
		liq_table[obj->value[2]].liq_name);
        break;

      case ITEM_CONTAINER:
	printf_to_char(ch,"Capacity: %d#  Maximum weight: %d#  flags: %s\n\r",
		obj->value[0], obj->value[3], cont_bit_name(obj->value[1]));
	if (obj->value[4] != 100)
	  {
	    printf_to_char(ch,"Weight multiplier: %d%%\n\r",
		    obj->value[4]);
	  }
	break;
		
      case ITEM_WEAPON:
 	send_to_char("Weapon type is ",ch);
	switch (obj->value[0])
	  {
	  case(WEAPON_EXOTIC) : send_to_char("exotic.\n\r",ch);	break;
	  case(WEAPON_SWORD)  : send_to_char("sword.\n\r",ch);	break;	
	  case(WEAPON_DAGGER) : send_to_char("dagger.\n\r",ch);	break;
	  case(WEAPON_SPEAR)	: send_to_char("spear/staff.\n\r",ch); break;
	  case(WEAPON_MACE) 	: send_to_char("mace/club.\n\r",ch); break;
	  case(WEAPON_AXE)	: send_to_char("axe.\n\r",ch); break;
	  case(WEAPON_FLAIL)	: send_to_char("flail.\n\r",ch); break;
	  case(WEAPON_WHIP)	: send_to_char("whip.\n\r",ch); break;
	  case(WEAPON_POLEARM): send_to_char("polearm.\n\r",ch); break;
	  default		: send_to_char("unknown.\n\r",ch); break;
	  }
	if (obj->pIndexData->new_format)
	  printf_to_char(ch,"Damage is %dd%d (average %d).\n\r",
		  obj->value[1],obj->value[2],
		  (1 + obj->value[2]) * obj->value[1] / 2);
	else
	  printf_to_char(ch, "Damage is %d to %d (average %d).\n\r",
		   obj->value[1], obj->value[2],
		   ( obj->value[1] + obj->value[2] ) / 2 );
        if (obj->value[4])  /* weapon flags */
	  {
            printf_to_char(ch,"Weapons flags: %s\n\r",weapon_bit_name(obj->value[4]));
	  }
	break;

      case ITEM_ARMOR:
	printf_to_char(ch, 
		 "Armor class is %d pierce, %d bash, %d slash, and %d vs. magic.\n\r", 
		 obj->value[0], obj->value[1], obj->value[2], obj->value[3] );
	break;
      }
  }
  return;
}

/* gives an eq list.  Ch is the character being examined,
   ch1 is the person seeing the list */
void show_eq_char( Character *ch, Character *ch1 )
{ 
  if ( get_eq_char( ch, WEAR_FLOAT ) != NULL )
  printf_to_char(ch1,"{CF{Wloatin{Cg {g: {x%-40s\n\r",eq_worn(ch,WEAR_FLOAT));
  if ( get_eq_char( ch, WEAR_LIGHT ) != NULL )
  printf_to_char(ch1,"{CL{Wigh{Ct    {g: {x%-40s\n\r",eq_worn(ch,WEAR_LIGHT));
  if ( get_eq_char( ch, WEAR_HEAD ) != NULL )
  printf_to_char(ch1,"{CH{Wea{Cd     {g: {x%-40s\n\r",eq_worn(ch,WEAR_HEAD));
  if ( get_eq_char( ch, WEAR_NECK_1 ) != NULL )
  printf_to_char(ch1,"{CN{Wec{Ck     {g: {x%-40s\n\r",eq_worn(ch,WEAR_NECK_1));
  if ( get_eq_char( ch, WEAR_NECK_2 ) != NULL )
  printf_to_char(ch1,"{CN{Wec{Ck     {g: {x%-40s\n\r",eq_worn(ch,WEAR_NECK_2));
  if ( get_eq_char( ch, WEAR_BODY ) != NULL )
  printf_to_char(ch1,"{CB{Wod{Cy     {g: {x%-40s\n\r",eq_worn(ch,WEAR_BODY));
  if ( get_eq_char( ch, WEAR_ABOUT ) != NULL )
  printf_to_char(ch1,"{CT{Wors{Co    {g: {x%-40s\n\r",eq_worn(ch,WEAR_ABOUT));
  if ( get_eq_char( ch, WEAR_ARMS ) != NULL )
  printf_to_char(ch1,"{CA{Wrm{Cs     {g: {x%-40s\n\r",eq_worn(ch,WEAR_ARMS));
  if ( get_eq_char( ch, WEAR_WRIST_L ) != NULL )
  printf_to_char(ch1,"{CW{Wris{Ct    {g: {x%-40s\n\r",eq_worn(ch,WEAR_WRIST_L));
  if ( get_eq_char( ch, WEAR_WRIST_R ) != NULL )
  printf_to_char(ch1,"{CW{Wris{Ct    {g: {x%-40s\n\r",eq_worn(ch,WEAR_WRIST_R));
  if ( get_eq_char( ch, WEAR_HANDS ) != NULL )
  printf_to_char(ch1,"{CH{Wand{Cs    {g: {x%-40s\n\r",eq_worn(ch,WEAR_HANDS));
  if ( get_eq_char( ch, WEAR_BOMB ) != NULL )
  printf_to_char(ch1,"{CB{Wom{Cb     {g: {x%-40s\n\r",eq_worn(ch,WEAR_BOMB));
  if ( get_eq_char( ch, WEAR_FINGER_L ) != NULL )
  printf_to_char(ch1,"{CF{Winge{Cr   {g: {x%-40s\n\r",eq_worn(ch,WEAR_FINGER_L));
  if ( get_eq_char( ch, WEAR_FINGER_R ) != NULL )
  printf_to_char(ch1,"{CF{Winge{Cr   {g: {x%-40s\n\r",eq_worn(ch,WEAR_FINGER_R));
  if ( get_eq_char( ch, WEAR_WAIST ) != NULL )
  printf_to_char(ch1,"{CW{Wais{Ct    {g: {x%-40s\n\r",eq_worn(ch,WEAR_WAIST));
  if ( get_eq_char( ch, WEAR_WIELD ) != NULL )
  printf_to_char(ch1,"{CP{Wrimar{Cy  {g: {x%-40s\n\r",eq_worn(ch,WEAR_WIELD));
  if ( get_eq_char( ch, WEAR_SECONDARY ) != NULL )
  printf_to_char(ch1,"{CO{WffHan{Cd{x  {g: {x%-40s\n\r",eq_worn(ch,WEAR_SECONDARY));
  if ( get_eq_char( ch, WEAR_SHIELD ) != NULL )
  printf_to_char(ch1,"{CS{Whiel{Cd   {g: {x%-40s\n\r",eq_worn(ch,WEAR_SHIELD));
  if ( get_eq_char( ch, WEAR_HOLD ) != NULL )
  printf_to_char(ch1,"{CH{Wel{Cd     {g: {x%-40s\n\r",eq_worn(ch,WEAR_HOLD));
  if ( get_eq_char( ch, WEAR_LEGS ) != NULL )
  printf_to_char(ch1,"{CL{Weg{Cs     {g: {x%-40s\n\r",eq_worn(ch,WEAR_LEGS));
  if ( get_eq_char( ch, WEAR_FEET ) != NULL )
  printf_to_char(ch1,"{CF{Wee{Ct     {g: {x%-40s\n\r",eq_worn(ch,WEAR_FEET));
}
