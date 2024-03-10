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
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "merc.h"
#include "magic.h"
#include "RaceManager.h"
#include "recycle.h"
#include "Room.h"
#include "tables.h"

/* command procedures needed */
DECLARE_DO_FUN(do_groups	);
DECLARE_DO_FUN(do_help		);
DECLARE_DO_FUN(do_say		);

extern RaceManager * race_manager;


/* used to get new skills */
void do_gain(Character *ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    Character *trainer;
    int gn = 0, sn = 0;

    if (IS_NPC(ch))
	return;

    /* find a trainer */
    for ( trainer = ch->in_room->people; 
	  trainer != NULL; 
	  trainer = trainer->next_in_room)
	if (IS_NPC(trainer) && IS_SET(trainer->act,ACT_GAIN))
	    break;

    if (trainer == NULL || !ch->can_see(trainer))
    {
	send_to_char("You can't do that here.\n\r",ch);
	return;
    }

    one_argument(argument,arg);

    if (arg[0] == '\0')
    {
	do_say(trainer,(char*)"Pardon me?");
	return;
    }

    if (!str_prefix(arg,"list"))
    {
	int col;
	
	col = 0;

	snprintf(buf, sizeof(buf), "%-18s %-5s %-18s %-5s %-18s %-5s\n\r",
	             "group","cost","group","cost","group","cost");
	send_to_char(buf,ch);

	for (gn = 0; gn < MAX_GROUP; gn++)
	{
	    if (group_table[gn].name == NULL)
		break;

	    if (!ch->pcdata->group_known[gn]
	    &&  group_table[gn].rating[ch->class_num] > 0)
	    {
		snprintf(buf, sizeof(buf),"%-18s %-5d ",
		    group_table[gn].name,group_table[gn].rating[ch->class_num]);
		send_to_char(buf,ch);
		if (++col % 3 == 0)
		    send_to_char("\n\r",ch);
	    }
	}
	if (col % 3 != 0)
	    send_to_char("\n\r",ch);
	
	send_to_char("\n\r",ch);		

	col = 0;

        snprintf(buf, sizeof(buf), "%-18s %-5s %-18s %-5s %-18s %-5s\n\r",
                     "skill","cost","skill","cost","skill","cost");
        send_to_char(buf,ch);
 
        for (sn = 0; sn < MAX_SKILL; sn++)
        {
            if (skill_table[sn].name == NULL)
                break;
 
            if (!ch->pcdata->learned[sn]
            &&  skill_rating(ch,sn) > 0
	    &&  skill_table[sn].spell_fun == spell_null)
            {
                snprintf(buf, sizeof(buf),"%-18s %-5d ",
                    skill_table[sn].name,skill_rating(ch,sn));
                send_to_char(buf,ch);
                if (++col % 3 == 0)
                    send_to_char("\n\r",ch);
            }
        }
        if (col % 3 != 0)
            send_to_char("\n\r",ch);
	return;
    }

    if (!str_prefix(arg,"convert"))
    {
	if (ch->practice < 10)
	{
	    act("$N tells you 'You are not yet ready.'",
		ch,NULL,trainer,TO_CHAR, POS_RESTING );
	    return;
	}

	act("$N helps you apply your practice to training",
		ch,NULL,trainer,TO_CHAR, POS_RESTING );
	ch->practice -= 10;
	ch->train +=1 ;
	return;
    }

    if (!str_prefix(arg,"points"))
    {
	if (ch->train < 2)
	{
	    act("$N tells you 'You are not yet ready.'",
		ch,NULL,trainer,TO_CHAR, POS_RESTING );
	    return;
	}

	if (ch->pcdata->points <= 40)
	{
	    act("$N tells you 'There would be no point in that.'",
		ch,NULL,trainer,TO_CHAR, POS_RESTING );
	    return;
	}

	act("$N trains you, and you feel more at ease with your skills.",
	    ch,NULL,trainer,TO_CHAR, POS_RESTING );

	ch->train -= 2;
	ch->pcdata->points -= 1;
	ch->exp = exp_per_level(ch,ch->pcdata->points) * ch->level;
	return;
    }

    /* else add a group/skill */

    gn = group_lookup(argument);
    if (gn > 0)
    {
	if (ch->pcdata->group_known[gn])
	{
	    act("$N tells you 'You already know that group!'",
		ch,NULL,trainer,TO_CHAR, POS_RESTING );
	    return;
	}

	if (group_table[gn].rating[ch->class_num] <= 0)
	{
	    act("$N tells you 'That group is beyond your powers.'",
		ch,NULL,trainer,TO_CHAR, POS_RESTING );
	    return;
	}

	if (ch->train < group_table[gn].rating[ch->class_num])
	{
	    act("$N tells you 'You are not yet ready for that group.'",
		ch,NULL,trainer,TO_CHAR, POS_RESTING );
	    return;
	}

	/* add the group */
	gn_add(ch,gn);
	act("$N trains you in the art of $t",
	    ch,group_table[gn].name,trainer,TO_CHAR, POS_RESTING );
	ch->train -= group_table[gn].rating[ch->class_num];
	return;
    }

    sn = skill_lookup(argument);
    if (sn > -1)
    {
	if (skill_table[sn].spell_fun != spell_null)
	{
	    act("$N tells you 'You must learn the full group.'",
		ch,NULL,trainer,TO_CHAR, POS_RESTING );
	    return;
	}
	    

        if (ch->pcdata->learned[sn])
        {
            act("$N tells you 'You already know that skill!'",
                ch,NULL,trainer,TO_CHAR, POS_RESTING );
            return;
        }
 
        if (skill_rating(ch,sn) <= 0)
        {
            act("$N tells you 'That skill is beyond your powers.'",
                ch,NULL,trainer,TO_CHAR, POS_RESTING );
            return;
        }
 
        if (ch->train < skill_rating(ch,sn))
        {
            act("$N tells you 'You are not yet ready for that skill.'",
                ch,NULL,trainer,TO_CHAR, POS_RESTING );
            return;
        }
 
        /* add the skill */
	ch->pcdata->learned[sn] = 1;
	ch->pcdata->sk_level[sn] = skill_level(ch,sn);
	ch->pcdata->sk_rating[sn] = skill_rating(ch,sn);
        act("$N trains you in the art of $t",
            ch,skill_table[sn].name,trainer,TO_CHAR, POS_RESTING );
        ch->train -= skill_rating(ch,sn);
        return;
    }

    act("$N tells you 'I do not understand...'",ch,NULL,trainer,TO_CHAR, POS_RESTING );
}
    



/* RT spells and skills show the players spells (or skills) */

void do_spells(Character *ch, char *argument)
{
    BUFFER *buffer;
    char arg[MAX_INPUT_LENGTH];
    char spell_list[LEVEL_HERO + 1][MAX_STRING_LENGTH];
    char spell_columns[LEVEL_HERO + 1];
    int sn, level, min_lev = 1, max_lev = LEVEL_HERO, mana;
    bool fAll = FALSE, found = FALSE;
    char buf[MAX_STRING_LENGTH];
 
    if (IS_NPC(ch))
      return;

    if (argument[0] != '\0')
    {
	fAll = TRUE;

	if (str_prefix(argument,"all"))
	{
	    argument = one_argument(argument,arg);
	    if (!is_number(arg))
	    {
		send_to_char("Arguments must be numerical or all.\n\r",ch);
		return;
	    }
	    max_lev = atoi(arg);

	    if (max_lev < 1 || max_lev > LEVEL_HERO)
	    {
		snprintf(buf, sizeof(buf),"Levels must be between 1 and %d.\n\r",LEVEL_HERO);
		send_to_char(buf,ch);
		return;
	    }

	    if (argument[0] != '\0')
	    {
		argument = one_argument(argument,arg);
		if (!is_number(arg))
		{
		    send_to_char("Arguments must be numerical or all.\n\r",ch);
		    return;
		}
		min_lev = max_lev;
		max_lev = atoi(arg);

		if (max_lev < 1 || max_lev > LEVEL_HERO)
		{
		    snprintf(buf, sizeof(buf),
			"Levels must be between 1 and %d.\n\r",LEVEL_HERO);
		    send_to_char(buf,ch);
		    return;
		}

		if (min_lev > max_lev)
		{
		    send_to_char("That would be silly.\n\r",ch);
		    return;
		}
	    }
	}
    }


    /* initialize data */
    for (level = 0; level < LEVEL_HERO + 1; level++)
    {
        spell_columns[level] = 0;
        spell_list[level][0] = '\0';
    }
 
    for (sn = 0; sn < MAX_SKILL; sn++)
    {
        if (skill_table[sn].name == NULL )
	    break;

	if ((level = ch->pcdata->sk_level[sn]) < LEVEL_HERO + 1
	&&  (fAll || level <= ch->level)
	&&  level >= min_lev && level <= max_lev
	&&  skill_table[sn].spell_fun != spell_null
	&&  ch->pcdata->learned[sn] > 0)
        {
	    found = TRUE;
	    level = ch->pcdata->sk_level[sn];
	    if (ch->level < level)
	    	snprintf(buf, sizeof(buf),"%-18s n/a      ", skill_table[sn].name);
	    else
	    {
		mana = UMAX(skill_table[sn].min_mana,
		    100/(2 + ch->level - level));
	        snprintf(buf, sizeof(buf),"%-18s  %3d mana  ",skill_table[sn].name,mana);
	    }
 
	    if (spell_list[level][0] == '\0')
          	snprintf(spell_list[level], MAX_STRING_LENGTH, "\n\rLevel %2d: %s",level,buf);
	    else /* append */
	    {
          	if ( ++spell_columns[level] % 2 == 0)
		    strcat(spell_list[level],"\n\r          ");
          	strcat(spell_list[level],buf);
	    }
	}
    }
 
    /* return results */
 
    if (!found)
    {
      	send_to_char("No spells found.\n\r",ch);
      	return;
    }

    buffer = new_buf();
    for (level = 0; level < LEVEL_HERO + 1; level++)
      	if (spell_list[level][0] != '\0')
	    add_buf(buffer,spell_list[level]);
    add_buf(buffer,"\n\r");
    page_to_char(buf_string(buffer),ch);
    free_buf(buffer);
}

void do_skills(Character *ch, char *argument)
{
    BUFFER *buffer;
    char arg[MAX_INPUT_LENGTH];
    char skill_list[LEVEL_HERO + 1][MAX_STRING_LENGTH];
    char skill_columns[LEVEL_HERO + 1];
    int sn, level, min_lev = 1, max_lev = LEVEL_HERO;
    bool fAll = FALSE, found = FALSE;
    char buf[MAX_STRING_LENGTH];
 
    if (IS_NPC(ch))
      return;

    if (argument[0] != '\0')
    {
	fAll = TRUE;

	if (str_prefix(argument,"all"))
	{
	    argument = one_argument(argument,arg);
	    if (!is_number(arg))
	    {
		send_to_char("Arguments must be numerical or all.\n\r",ch);
		return;
	    }
	    max_lev = atoi(arg);

	    if (max_lev < 1 || max_lev > LEVEL_HERO)
	    {
		snprintf(buf, sizeof(buf),"Levels must be between 1 and %d.\n\r",LEVEL_HERO);
		send_to_char(buf,ch);
		return;
	    }

	    if (argument[0] != '\0')
	    {
		argument = one_argument(argument,arg);
		if (!is_number(arg))
		{
		    send_to_char("Arguments must be numerical or all.\n\r",ch);
		    return;
		}
		min_lev = max_lev;
		max_lev = atoi(arg);

		if (max_lev < 1 || max_lev > LEVEL_HERO)
		{
		    snprintf(buf, sizeof(buf),
			"Levels must be between 1 and %d.\n\r",LEVEL_HERO);
		    send_to_char(buf,ch);
		    return;
		}

		if (min_lev > max_lev)
		{
		    send_to_char("That would be silly.\n\r",ch);
		    return;
		}
	    }
	}
    }


    /* initialize data */
    for (level = 0; level < LEVEL_HERO + 1; level++)
    {
        skill_columns[level] = 0;
        skill_list[level][0] = '\0';
    }
 
    for (sn = 0; sn < MAX_SKILL; sn++)
    {
        if (skill_table[sn].name == NULL )
	    break;

	if ((level = ch->pcdata->sk_level[sn]) < LEVEL_HERO + 1
	&&  (fAll || level <= ch->level)
	&&  level >= min_lev && level <= max_lev
	&&  skill_table[sn].spell_fun == spell_null
	&&  ch->pcdata->learned[sn] > 0)
        {
	    found = TRUE;
	    level = ch->pcdata->sk_level[sn];
	    if (ch->level < level)
	    	snprintf(buf, sizeof(buf),"%-18s n/a      ", skill_table[sn].name);
	    else
	    	snprintf(buf, sizeof(buf),"%-18s %3d%%      ",skill_table[sn].name,
		    ch->pcdata->learned[sn]);
 
	    if (skill_list[level][0] == '\0')
          	snprintf(skill_list[level], MAX_STRING_LENGTH, "\n\rLevel %2d: %s",level,buf);
	    else /* append */
	    {
          	if ( ++skill_columns[level] % 2 == 0)
		    strcat(skill_list[level],"\n\r          ");
          	strcat(skill_list[level],buf);
	    }
	}
    }
 
    /* return results */
 
    if (!found)
    {
      	send_to_char("No skills found.\n\r",ch);
      	return;
    }

    buffer = new_buf();
    for (level = 0; level < LEVEL_HERO + 1; level++)
      	if (skill_list[level][0] != '\0')
	    add_buf(buffer,skill_list[level]);
    add_buf(buffer,"\n\r");
    page_to_char(buf_string(buffer),ch);
    free_buf(buffer);
}

/* shows skills, groups and costs (only if not bought) */
void list_group_costs(Character *ch)
{
    char buf[100];
    int gn,sn,col;

    if (IS_NPC(ch))
	return;

    col = 0;

    snprintf(buf, sizeof(buf),"%-18s %-5s %-18s %-5s %-18s %-5s\n\r","group","cp","group","cp","group","cp");
    send_to_char(buf,ch);

    for (gn = 0; gn < MAX_GROUP; gn++)
    {
	if (group_table[gn].name == NULL)
	    break;

        if (!ch->gen_data->group_chosen[gn] 
	&&  !ch->pcdata->group_known[gn]
	&&  group_table[gn].rating[ch->class_num] > 0)
	{
	    snprintf(buf, sizeof(buf),"%-18s %-5d ",group_table[gn].name,
				    group_table[gn].rating[ch->class_num]);
	    send_to_char(buf,ch);
	    if (++col % 3 == 0)
		send_to_char("\n\r",ch);
	}
    }
    if ( col % 3 != 0 )
        send_to_char( "\n\r", ch );
    send_to_char("\n\r",ch);

    col = 0;
 
    snprintf(buf, sizeof(buf),"%-18s %-5s %-18s %-5s %-18s %-5s\n\r","skill","cp","skill","cp","skill","cp");
    send_to_char(buf,ch);
 
    for (sn = 0; sn < MAX_SKILL; sn++)
    {
        if (skill_table[sn].name == NULL)
            break;
 
        if (!ch->gen_data->skill_chosen[sn] 
	&&  ch->pcdata->learned[sn] == 0
	&&  skill_table[sn].spell_fun == spell_null
	&&  skill_rating(ch,sn) > 0)
        {
            snprintf(buf, sizeof(buf),"%-18s %-5d ",skill_table[sn].name,
                                    skill_rating(ch,sn));
            send_to_char(buf,ch);
            if (++col % 3 == 0)
                send_to_char("\n\r",ch);
        }
    }
    if ( col % 3 != 0 )
        send_to_char( "\n\r", ch );
    send_to_char("\n\r",ch);

    snprintf(buf, sizeof(buf),"Creation points: %d\n\r",ch->pcdata->points);
    send_to_char(buf,ch);
    snprintf(buf, sizeof(buf),"Experience per level: %d\n\r",
	    exp_per_level(ch,ch->gen_data->points_chosen));
    send_to_char(buf,ch);
    return;
}


void list_group_chosen(Character *ch)
{
    char buf[100];
    int gn,sn,col;
 
    if (IS_NPC(ch))
        return;
 
    col = 0;
 
    snprintf(buf, sizeof(buf),"%-18s %-5s %-18s %-5s %-18s %-5s","group","cp","group","cp","group","cp\n\r");
    send_to_char(buf,ch);
 
    for (gn = 0; gn < MAX_GROUP; gn++)
    {
        if (group_table[gn].name == NULL)
            break;
 
        if (ch->gen_data->group_chosen[gn] 
	&&  group_table[gn].rating[ch->class_num] > 0)
        {
            snprintf(buf, sizeof(buf),"%-18s %-5d ",group_table[gn].name,
                                    group_table[gn].rating[ch->class_num]);
            send_to_char(buf,ch);
            if (++col % 3 == 0)
                send_to_char("\n\r",ch);
        }
    }
    if ( col % 3 != 0 )
        send_to_char( "\n\r", ch );
    send_to_char("\n\r",ch);
 
    col = 0;
 
    snprintf(buf, sizeof(buf),"%-18s %-5s %-18s %-5s %-18s %-5s","skill","cp","skill","cp","skill","cp\n\r");
    send_to_char(buf,ch);
 
    for (sn = 0; sn < MAX_SKILL; sn++)
    {
        if (skill_table[sn].name == NULL)
            break;
 
        if (ch->gen_data->skill_chosen[sn] 
	&&  skill_rating(ch,sn) > 0)
        {
            snprintf(buf, sizeof(buf),"%-18s %-5d ",skill_table[sn].name,
                                    skill_rating(ch,sn));
            send_to_char(buf,ch);
            if (++col % 3 == 0)
                send_to_char("\n\r",ch);
        }
    }
    if ( col % 3 != 0 )
        send_to_char( "\n\r", ch );
    send_to_char("\n\r",ch);
 
    snprintf(buf, sizeof(buf),"Creation points: %d\n\r",ch->gen_data->points_chosen);
    send_to_char(buf,ch);
    snprintf(buf, sizeof(buf),"Experience per level: %d\n\r",
	    exp_per_level(ch,ch->gen_data->points_chosen));
    send_to_char(buf,ch);
    return;
}

int exp_per_level(Character *ch, int points)
{
    int count;

    if (points < 1)
	return 100;

    count = 1 + ch->reclass_num;

    return points * (100 * count);
}

/* this procedure handles the input parsing for the skill generator */
bool parse_gen_groups(Character *ch,char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    char buf[100];
    int gn,sn,i;
 
    if (argument[0] == '\0')
	return FALSE;

    argument = one_argument(argument,arg);

    if (!str_prefix(arg,"help"))
    {
	if (argument[0] == '\0')
	{
	    do_help(ch,(char*)"group help");
	    return TRUE;
	}

        do_help(ch,argument);
	return TRUE;
    }

    if (!str_prefix(arg,"add"))
    {
	if (argument[0] == '\0')
	{
	    send_to_char("You must provide a skill name.\n\r",ch);
	    return TRUE;
	}

	gn = group_lookup(argument);
	if (gn != -1)
	{
	    if (ch->gen_data->group_chosen[gn]
	    ||  ch->pcdata->group_known[gn])
	    {
		send_to_char("You already know that group!\n\r",ch);
		return TRUE;
	    }

	    if (group_table[gn].rating[ch->class_num] < 1)
	    {
	  	send_to_char("That group is not available.\n\r",ch);
	 	return TRUE;
	    }

	    snprintf(buf, sizeof(buf),"%s group added\n\r",group_table[gn].name);
	    send_to_char(buf,ch);
	    ch->gen_data->group_chosen[gn] = TRUE;
	    ch->gen_data->points_chosen += group_table[gn].rating[ch->class_num];
	    gn_add(ch,gn);
	    ch->pcdata->points += group_table[gn].rating[ch->class_num];
	    return TRUE;
	}

	sn = skill_lookup(argument);
	if (sn != -1)
	{
	    if (ch->gen_data->skill_chosen[sn]
	    ||  ch->pcdata->learned[sn] > 0)
	    {
		send_to_char("You already know that skill!\n\r",ch);
		return TRUE;
	    }

	    if (skill_rating(ch,sn) < 1
	    ||  skill_table[sn].spell_fun != spell_null)
	    {
		send_to_char("That skill is not available.\n\r",ch);
		return TRUE;
	    }
	    snprintf(buf, sizeof(buf), "%s skill added\n\r",skill_table[sn].name);
	    send_to_char(buf,ch);
	    ch->gen_data->skill_chosen[sn] = TRUE;
	    ch->gen_data->points_chosen += skill_rating(ch,sn);
	    ch->pcdata->learned[sn] = 1;
	    ch->pcdata->sk_level[sn] = skill_level(ch,sn);
	    ch->pcdata->sk_rating[sn] = skill_rating(ch,sn);
	    ch->pcdata->points += skill_rating(ch,sn);
	    return TRUE;
	}

	send_to_char("No skills or groups by that name...\n\r",ch);
	return TRUE;
    }

    if (!strcmp(arg,"drop"))
    {
	if (argument[0] == '\0')
  	{
	    send_to_char("You must provide a skill to drop.\n\r",ch);
	    return TRUE;
	}

	gn = group_lookup(argument);
	if (gn != -1 && ch->gen_data->group_chosen[gn])
	{
	    send_to_char("Group dropped.\n\r",ch);
	    ch->gen_data->group_chosen[gn] = FALSE;
	    ch->gen_data->points_chosen -= group_table[gn].rating[ch->class_num];
	    gn_remove(ch,gn);
	    for (i = 0; i < MAX_GROUP; i++)
	    {
		if (ch->gen_data->group_chosen[gn])
		    gn_add(ch,gn);
	    }
	    ch->pcdata->points -= group_table[gn].rating[ch->class_num];
	    return TRUE;
	}

	sn = skill_lookup(argument);
	if (sn != -1 && ch->gen_data->skill_chosen[sn])
	{
	    send_to_char("Skill dropped.\n\r",ch);
	    ch->gen_data->skill_chosen[sn] = FALSE;
	    ch->gen_data->points_chosen -= skill_rating(ch,sn);
	    ch->pcdata->learned[sn] = 0;
	    ch->pcdata->points -= skill_rating(ch,sn);
	    return TRUE;
	}

	send_to_char("You haven't bought any such skill or group.\n\r",ch);
	return TRUE;
    }

    if (!str_prefix(arg,"premise"))
    {
	do_help(ch,(char*)"premise");
	return TRUE;
    }

    if (!str_prefix(arg,"list"))
    {
	list_group_costs(ch);
	return TRUE;
    }

    if (!str_prefix(arg,"learned"))
    {
	list_group_chosen(ch);
	return TRUE;
    }

    if (!str_prefix(arg,"info"))
    {
	do_groups(ch,argument);
	return TRUE;
    }

    return FALSE;
}
	    
	


        

/* shows all groups, or the sub-members of a group */
void do_groups(Character *ch, char *argument)
{
    char buf[100];
    int gn,sn,col;

    if (IS_NPC(ch))
	return;

    col = 0;

    if (argument[0] == '\0')
    {   /* show all groups */
	
	for (gn = 0; gn < MAX_GROUP; gn++)
        {
	    if (group_table[gn].name == NULL)
		break;
	    if (ch->pcdata->group_known[gn])
	    {
		snprintf(buf, sizeof(buf),"%-20s ",group_table[gn].name);
		send_to_char(buf,ch);
		if (++col % 3 == 0)
		    send_to_char("\n\r",ch);
	    }
        }
        if ( col % 3 != 0 )
            send_to_char( "\n\r", ch );
        snprintf(buf, sizeof(buf),"Creation points: %d\n\r",ch->pcdata->points);
	send_to_char(buf,ch);
	return;
     }

     if (!str_cmp(argument,"all"))    /* show all groups */
     {
        for (gn = 0; gn < MAX_GROUP; gn++)
        {
            if (group_table[gn].name == NULL)
                break;
	    snprintf(buf, sizeof(buf),"%-20s ",group_table[gn].name);
            send_to_char(buf,ch);
	    if (++col % 3 == 0)
            	send_to_char("\n\r",ch);
        }
        if ( col % 3 != 0 )
            send_to_char( "\n\r", ch );
	return;
     }
	
     
     /* show the sub-members of a group */
     gn = group_lookup(argument);
     if (gn == -1)
     {
	send_to_char("No group of that name exist.\n\r",ch);
	send_to_char(
	    "Type 'groups all' or 'info all' for a full listing.\n\r",ch);
	return;
     }

     for (sn = 0; sn < MAX_IN_GROUP; sn++)
     {
	if (group_table[gn].spells[sn] == NULL)
	    break;
	snprintf(buf, sizeof(buf),"%-20s ",group_table[gn].spells[sn]);
	send_to_char(buf,ch);
	if (++col % 3 == 0)
	    send_to_char("\n\r",ch);
     }
    if ( col % 3 != 0 )
        send_to_char( "\n\r", ch );
}

/* checks for skill improvement */
void check_improve( Character *ch, int sn, bool success, int multiplier ) {
    int chance;
    char buf[100];

    if (IS_NPC(ch))
        return;

    if (ch->level < skill_level(ch, sn)
        || ch->pcdata->learned[sn] == 0
        || ch->pcdata->learned[sn] == 100)
        return;  /* skill is not known */

    /* check to see if the character has a chance to learn */
    chance = 10 * int_app[get_curr_stat(ch, STAT_INT)].learn;
    chance /= (multiplier
               * 4);
    chance += ch->level;

    // Improved odds at lower levels
    chance += 100 - ch->pcdata->learned[sn];

    if (number_range(1, 750) > chance)
        return;

    /* now that the character has a CHANCE to learn, see if they really have */

    if (success) {
        chance = URANGE(5, 100 - ch->pcdata->learned[sn], 95);
        if (number_percent() < chance) {
            snprintf(buf, sizeof(buf), "{BYou have become better at %s!{x\n\r",
                    skill_table[sn].name);
            send_to_char(buf, ch);
            ch->pcdata->learned[sn]++;
            ch->gain_exp(2 + skill_rating(ch, sn));
            ch->gain_exp(number_range(25, 40) * skill_rating(ch, sn));
        }
    } else {
        chance = URANGE(5, ch->pcdata->learned[sn] / 2, 30);
        if (number_percent() < chance) {
            snprintf(buf, sizeof(buf),
                    "{BYou learn from your mistakes, and your %s skill improves.{x\n\r",
                    skill_table[sn].name);
            send_to_char(buf, ch);
            ch->pcdata->learned[sn] += number_range(1, 3);
            ch->pcdata->learned[sn] = UMIN(ch->pcdata->learned[sn], 100);
            ch->gain_exp(number_range(15, 30) * skill_rating(ch, sn));
        }
    }

    if ( ch->pcdata->learned[sn] == 100 ) {
        snprintf(buf, sizeof(buf), "{YYou have mastered %s!{x\n\r", skill_table[sn].name);
    }
}

/* returns a group index number given the name */
int group_lookup( const char *name )
{
    int gn;
 
    for ( gn = 0; gn < MAX_GROUP; gn++ )
    {
        if ( group_table[gn].name == NULL )
            break;
        if ( LOWER(name[0]) == LOWER(group_table[gn].name[0])
        &&   !str_prefix( name, group_table[gn].name ) )
            return gn;
    }
 
    return -1;
}

/* recursively adds a group given its number -- uses group_add */
void gn_add( Character *ch, int gn)
{
    int i;
    
    ch->pcdata->group_known[gn] = TRUE;
    for ( i = 0; i < MAX_IN_GROUP; i++)
    {
        if (group_table[gn].spells[i] == NULL)
            break;
        group_add(ch,group_table[gn].spells[i],FALSE);
    }
}

/* recusively removes a group given its number -- uses group_remove */
void gn_remove( Character *ch, int gn)
{
    int i;

    ch->pcdata->group_known[gn] = FALSE;

    for ( i = 0; i < MAX_IN_GROUP; i ++)
    {
	if (group_table[gn].spells[i] == NULL)
	    break;
	group_remove(ch,group_table[gn].spells[i]);
    }
}
	
/* use for processing a skill or group for addition  */
void group_add( Character *ch, const char *name, bool deduct)
{
    int sn,gn;

    if (IS_NPC(ch)) /* NPCs do not have skills */
	return;

    sn = skill_lookup(name);

    if (sn != -1)
    {
	if (ch->pcdata->learned[sn] == 0) /* i.e. not known */
	{
	    ch->pcdata->learned[sn] = 1;
	    ch->pcdata->sk_level[sn] = skill_level(ch,sn);
	    ch->pcdata->sk_rating[sn] = skill_rating(ch,sn);
	    if (deduct)
	   	ch->pcdata->points += skill_rating(ch,sn); 
	}
	return;
    }
	
    /* now check groups */

    gn = group_lookup(name);

    if (gn != -1)
    {
	if (ch->pcdata->group_known[gn] == FALSE)  
	{
	    ch->pcdata->group_known[gn] = TRUE;
	    if (deduct)
		ch->pcdata->points += group_table[gn].rating[ch->class_num];
	}
	gn_add(ch,gn); /* make sure all skills in the group are known */
    }
}

/* used for processing a skill or group for deletion -- no points back! */

void group_remove(Character *ch, const char *name)
{
    int sn, gn;
    
     sn = skill_lookup(name);

    if (sn != -1)
    {
	ch->pcdata->learned[sn] = 0;
	return;
    }
 
    /* now check groups */
 
    gn = group_lookup(name);
 
    if (gn != -1 && ch->pcdata->group_known[gn] == TRUE)
    {
	ch->pcdata->group_known[gn] = FALSE;
	gn_remove(ch,gn);  /* be sure to call gn_add on all remaining groups */
    }
}

/* Make exceptions for skill level check */
int skill_level( Character *ch, int sn )
{
    /* First check is to see if they're a wereperson */
    if (ch->getRace() == race_manager->getRaceByName("werefolk"))
    {
	/* Bunnies first */
	if (ch->morph_form == morph_lookup("bunny")
	|| ch->morph_form == morph_lookup("werebunny"))
	{
	    /* Now lets look for bunny skills.  Start with kick */
	    if (!str_cmp(skill_table[sn].name, "kick"))
	    {
		/* Mages don't get it by default, bunnies do */
		if (IS_MAGE(ch))
		    return 15;
		else
		    return skill_table[sn].skill_level[ch->class_num];
	    }

	    /* Now we see a bite */
	    if (!str_cmp(skill_table[sn].name, "bite"))
	    {
		/* Only mammal werefolk get bite, bunnies don't
		 * hit as hard as other races, and don't fight
		 * much, so they don't get this skill until later.
		 * But have you ever seen a bunny bite?  NASTY!
		 * That's why they're getting the skill here :)
		 */
		return 15;
	    }
	}

	/* Now wolves */
	if (ch->morph_form == morph_lookup("wolf")
	|| ch->morph_form == morph_lookup("werewolf"))
	{
	    /* They have genetic bite! :) */
	    if (!str_cmp(skill_table[sn].name, "bite"))
	    {
		return 5;
	    }
	}

	/* Now bears */
	if (ch->morph_form == morph_lookup("bear")
	|| ch->morph_form == morph_lookup("werebear"))
	{
	    /* They have genetic bite! :) */
	    if (!str_cmp(skill_table[sn].name, "bite"))
	    {
		return 5;
	    }
	}

	/* Now eagles */
	if (ch->morph_form == morph_lookup("eagle"))
	{
	}

	/* Now rats */
	if (ch->morph_form == morph_lookup("rat")
	|| ch->morph_form == morph_lookup("skaven"))
	{
	    /* They have genetic bite! :) */
	    if (!str_cmp(skill_table[sn].name, "bite"))
	    {
		return 5;
	    }
	}

	/* Now check for werefolk-only skills */
	if (!str_cmp(skill_table[sn].name, "morph"))
	{
	    return 1;
	}
    }

    /* Next check humans */
    if (ch->getRace() == race_manager->getRaceByName("human"))
    {
    }

    /* Draconians */
    if (ch->getRace() == race_manager->getRaceByName("draconian"))
    {
		if (!str_cmp(skill_table[sn].name, "breath"))
		{
			return 10;
		}
    }

    /* Elf */
    if (ch->getRace() == race_manager->getRaceByName("elf"))
    {
	if (!str_cmp(skill_table[sn].name, "sneak"))
	{
	    if (IS_MAGE(ch) || IS_CLERIC(ch))
		return 20;
	    else
		return skill_table[sn].skill_level[ch->class_num];
	}

	if (!str_cmp(skill_table[sn].name, "hide"))
	{
	    if (IS_MAGE(ch) || IS_CLERIC(ch))
		return 30;
	    else
		return skill_table[sn].skill_level[ch->class_num];
	}
    }

    /* Dwarf */
    if (ch->getRace() == race_manager->getRaceByName("dwarf"))
    {
		if (!str_cmp(skill_table[sn].name, "berserk"))
		{
			if (IS_MAGE(ch) || IS_CLERIC(ch) || IS_THIEF(ch))
			return 40;
			else
			return skill_table[sn].skill_level[ch->class_num];
		}
    }

    /* Giant */
    if (ch->getRace() == race_manager->getRaceByName("giant"))
    {
		if (!str_cmp(skill_table[sn].name, "bash"))
		{
			if (IS_MAGE(ch) || IS_CLERIC(ch) || IS_THIEF(ch))
			return 4;
			else
			return skill_table[sn].skill_level[ch->class_num];
		}
    }

    /* Everything else is set to default */
    return skill_table[sn].skill_level[ch->class_num];
}


/* Make exceptions for skill purchase cost */
int skill_rating( Character *ch, int sn )
{
    /* First check is to see if they're a wereperson */
    if (ch->getRace() == race_manager->getRaceByName("werefolk"))
    {
		/* Bunnies first */
		if (ch->morph_form == morph_lookup("bunny")
		|| ch->morph_form == morph_lookup("werebunny"))
		{
			/* Now lets look for bunny skills.  Start with kick */
			if (!str_cmp(skill_table[sn].name, "kick"))
			{
			/* Mages don't get it by default, bunnies do */
			if (IS_MAGE(ch))
				return 4;
			else
				return skill_table[sn].rating[ch->class_num];
			}

			/* Now we see a bite */
			if (!str_cmp(skill_table[sn].name, "bite"))
			{
			/* Only mammal werefolk get bite, bunnies don't
			* hit as hard as other races, and don't fight
			* much, so they don't get this skill until later.
			* But have you ever seen a bunny bite?  NASTY!
			* That's why they're getting the skill here :)
			*/
			return 5;
			}
		}

		/* Now wolves */
		if (ch->morph_form == morph_lookup("wolf")
		|| ch->morph_form == morph_lookup("werewolf"))
		{
			/* They have genetic bite! :) */
			if (!str_cmp(skill_table[sn].name, "bite"))
			{
			return 5;
			}
		}

		/* Now bears */
		if (ch->morph_form == morph_lookup("bear")
		|| ch->morph_form == morph_lookup("werebear"))
		{
			/* They have genetic bite! :) */
			if (!str_cmp(skill_table[sn].name, "bite"))
			{
			return 5;
			}
		}

		/* Now eagles */
		if (ch->morph_form == morph_lookup("eagle"))
		{
		}

		/* Now rats */
		if (ch->morph_form == morph_lookup("rat")
		|| ch->morph_form == morph_lookup("skaven"))
		{
			/* They have genetic bite! :) */
			if (!str_cmp(skill_table[sn].name, "bite"))
			{
			return 5;
			}
		}

		/* Now check for werefolk-only skills */
		if (!str_cmp(skill_table[sn].name, "morph"))
		{
			return 3;
		}
    }

    /* Next check humans */
    if (ch->getRace() == race_manager->getRaceByName("human"))
    {
    }

    /* Draconians */
    if (ch->getRace() == race_manager->getRaceByName("draconian"))
    {
		if (!str_cmp(skill_table[sn].name, "breath"))
		{
			return 9;
		}
    }

    /* Elf */
    if (ch->getRace() == race_manager->getRaceByName("elf"))
    {
	if (!str_cmp(skill_table[sn].name, "sneak"))
	{
	    if (IS_MAGE(ch) || IS_CLERIC(ch))
		return 3;
	    else
		return skill_table[sn].rating[ch->class_num];
	}

	if (!str_cmp(skill_table[sn].name, "hide"))
	{
	    if (IS_MAGE(ch) || IS_CLERIC(ch))
		return 4;
	    else
		return skill_table[sn].rating[ch->class_num];
	}
    }

    /* Dwarf */
    if (ch->getRace() == race_manager->getRaceByName("dwarf"))
    {
	if (!str_cmp(skill_table[sn].name, "berserk"))
	{
	    if (IS_MAGE(ch) || IS_CLERIC(ch) || IS_THIEF(ch))
		return 6;
	    else
		return skill_table[sn].rating[ch->class_num];
	}
    }

    /* Giant */
    if (ch->getRace() == race_manager->getRaceByName("giant"))
    {
	if (!str_cmp(skill_table[sn].name, "bash"))
	{
	    if (IS_MAGE(ch) || IS_CLERIC(ch) || IS_THIEF(ch))
		return 2;
	    else
		return skill_table[sn].rating[ch->class_num];
	}
    }

    /* Everything else is set to default */
    return skill_table[sn].rating[ch->class_num];
}
