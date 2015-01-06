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
#endif
#include <stdio.h>
#include <time.h>
#include "merc.h"
#include "tables.h"
#include "PlayerCharacter.h"

int flag_lookup (const char *name, const struct flag_type *flag_table)
{
    int flag;

    for (flag = 0; flag_table[flag].name != NULL; flag++)
    {
	if (LOWER(name[0]) == LOWER(flag_table[flag].name[0])
	&&  !str_prefix(name,flag_table[flag].name))
	    return flag_table[flag].bit;
    }

    return 0;
}

int position_lookup (const char *name)
{
   int pos;

   for (pos = 0; position_table[pos].name != NULL; pos++)
   {
	if (LOWER(name[0]) == LOWER(position_table[pos].name[0])
	&&  !str_prefix(name,position_table[pos].name))
	    return pos;
   }
   
   return -1;
}

int sex_lookup (const char *name)
{
   int sex;
   
   for (sex = 0; sex_table[sex].name != NULL; sex++)
   {
	if (LOWER(name[0]) == LOWER(sex_table[sex].name[0])
	&&  !str_prefix(name,sex_table[sex].name))
	    return sex;
   }

   return -1;
}

int size_lookup (const char *name)
{
   int size;
 
   for ( size = 0; size_table[size].name != NULL; size++)
   {
        if (LOWER(name[0]) == LOWER(size_table[size].name[0])
        &&  !str_prefix( name,size_table[size].name))
            return size;
   }
 
   return -1;
}

/* returns race number */
int race_lookup (const char *name)
{
   int race;

   for ( race = 0; race_table[race].name != NULL; race++)
   {
	if (LOWER(name[0]) == LOWER(race_table[race].name[0])
	&&  !str_prefix( name,race_table[race].name))
	    return race;
   }

   return 0;
} 

/* returns morph race number */
int morph_lookup (const char *name)
{
   int race;

   for ( race = 0; morph_table[race].name[0] != '\0'; race++)
   {
	if (LOWER(name[0]) == LOWER(morph_table[race].name[0])
	&&  !str_prefix( name,morph_table[race].name))
	    return race;
   }

   return -1;
} 

int item_lookup(const char *name)
{
    int type;

    for (type = 0; item_table[type].name != NULL; type++)
    {
        if (LOWER(name[0]) == LOWER(item_table[type].name[0])
        &&  !str_prefix(name,item_table[type].name))
            return item_table[type].type;
    }
 
    return -1;
}

int liq_lookup (const char *name)
{
    int liq;

    for ( liq = 0; liq_table[liq].liq_name != NULL; liq++)
    {
	if (LOWER(name[0]) == LOWER(liq_table[liq].liq_name[0])
	&& !str_prefix(name,liq_table[liq].liq_name))
	    return liq;
    }

    return -1;
}

int nw_lookup( PlayerCharacter *ch )
{
    OBJ_DATA *obj;
    long nw;

    /* First, monetary value */
    nw = ch->gold * 100;
    nw +=  ch->silver;

    // Pkills are worth +/- 10 gold, mkills are worth +/- 5 gold
    nw += 100 * (ch->getPlayerKillCount() * 10);
    nw -= 100 * (ch->getKilledByPlayersCount() * 10);
    nw += 100 * (ch->getMobKillCount() * 5);
    nw -= 100 * (ch->getKilledByMobCount() * 5);

    /* Next, lets get their item costs */
    for (obj = ch->carrying; obj != NULL; obj = obj->next_content)
    {
	if (obj->item_type == ITEM_BANKNOTE)
	{
	    nw += obj->value[0] * 100;
	    nw += obj->value[1];
	}
	else
	nw += obj->cost;
    }

    return nw /= 100;
}

long clan_nw_lookup( CLAN_DATA * clan)
{
    long nw;

    nw = 0;

    /* First, monetary value */
    nw += clan->money * 10;

    // Pkills are worth +/- 1000 gold, mkills are worth +/- 50 gold
    nw += 10000 * (clan->pkills * 10);
    nw -= 100 * (clan->pdeaths * 10);
    nw += 5000 * (clan->mkills * 5);
    nw -= 50 * (clan->mdeaths * 5);

    /* Members are worth 250 each */
    nw += clan->members * 25000;

    return nw /= 100;
}

