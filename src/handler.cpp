/**************************************************************************r
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
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <vector>
#include "merc.h"
#include "magic.h"
#include "NonPlayerCharacter.h"
#include "Object.h"
#include "ObjectHelper.h"
#include "PlayerCharacter.h"
#include "PlayerRace.h"
#include "RaceManager.h"
#include "recycle.h"
#include "Room.h"
#include "tables.h"

using std::vector;

/* command procedures needed */
DECLARE_DO_FUN(do_return	);


extern RaceManager * race_manager;

/* friend stuff -- for NPC's mostly */
bool is_friend(Character *ch,Character *victim)
{
    if (ch->isSameGroup(victim))
	return TRUE;

    
    if (!ch->isNPC())
	return FALSE;

    if (!IS_NPC(victim))
    {
	if (IS_SET(ch->off_flags,ASSIST_PLAYERS))
	    return TRUE;
	else
	    return FALSE;
    }

    if (IS_AFFECTED(ch,AFF_CHARM))
	return FALSE;

    if (IS_SET(ch->off_flags,ASSIST_ALL))
	return TRUE;

    if (ch->group && ch->group == victim->group)
	return TRUE;

    if (IS_SET(ch->off_flags,ASSIST_VNUM) 
    &&  ch->pIndexData == victim->pIndexData)
	return TRUE;

    if (IS_SET(ch->off_flags,ASSIST_RACE) && ch->getRace() == victim->getRace())
	return TRUE;
     
    if (IS_SET(ch->off_flags,ASSIST_ALIGN)
    &&  !IS_SET(ch->act,ACT_NOALIGN) && !IS_SET(victim->act,ACT_NOALIGN)
    &&  ((IS_GOOD(ch) && IS_GOOD(victim))
    ||	 (IS_EVIL(ch) && IS_EVIL(victim))
    ||   (IS_NEUTRAL(ch) && IS_NEUTRAL(victim))))
	return TRUE;

    return FALSE;
}

/* returns number of people on an object */
int count_users(Object *obj)
{
    Character *fch;
    int count = 0;

    if (obj->getInRoom() == NULL)
	return 0;

    for (fch = obj->getInRoom()->people; fch != NULL; fch = fch->next_in_room)
	if (fch->onObject() == obj)
	    count++;

    return count;
}
     
/* returns material number */
int material_lookup (const char *name)
{
    return 0;
}

int weapon_lookup (const char *name)
{
    int type;

    for (type = 0; weapon_table[type].name != NULL; type++)
    {
	if (LOWER(name[0]) == LOWER(weapon_table[type].name[0])
	&&  !str_prefix(name,weapon_table[type].name))
	    return type;
    }
 
    return -1;
}

int weapon_type (const char *name)
{
    int type;
 
    for (type = 0; weapon_table[type].name != NULL; type++)
    {
        if (LOWER(name[0]) == LOWER(weapon_table[type].name[0])
        &&  !str_prefix(name,weapon_table[type].name))
            return weapon_table[type].type;
    }
 
    return WEAPON_EXOTIC;
}

const char *item_name(int item_type)
{
    int type;

    for (type = 0; item_table[type].name != NULL; type++)
	if (item_type == item_table[type].type)
	    return item_table[type].name;
    return "none";
}

const char *weapon_name( int weapon_type)
{
    int type;
 
    for (type = 0; weapon_table[type].name != NULL; type++)
        if (weapon_type == weapon_table[type].type)
            return weapon_table[type].name;
    return "exotic";
}

int attack_lookup  (const char *name)
{
    int att;

    for ( att = 0; attack_table[att].name != NULL; att++)
    {
	if (LOWER(name[0]) == LOWER(attack_table[att].name[0])
	&&  !str_prefix(name,attack_table[att].name))
	    return att;
    }

    return 0;
}

/* returns a flag for wiznet */
long wiznet_lookup (const char *name)
{
    int flag;

    for (flag = 0; wiznet_table[flag].name != NULL; flag++)
    {
	if (LOWER(name[0]) == LOWER(wiznet_table[flag].name[0])
	&& !str_prefix(name,wiznet_table[flag].name))
	    return flag;
    }

    return -1;
}

/* returns class number */
int class_lookup (const char *name)
{
   int class_num;
 
   for ( class_num = 0; class_num < MAX_CLASS; class_num++)
   {
        if (LOWER(name[0]) == LOWER(class_table[class_num].name[0])
        &&  !str_prefix( name,class_table[class_num].name))
            return class_num;
   }
 
   return -1;
}

/* for immunity, vulnerabiltiy, and resistant
   the 'globals' (magic and weapons) may be overriden
   three other cases -- wood, silver, and iron -- are checked in fight.c */

int check_immune(Character *ch, int dam_type)
{
    int immune, def;
    int bit;

    immune = -1;
    def = IS_NORMAL;

    if (dam_type == DAM_NONE)
	return immune;

    if (dam_type <= 3)
    {
	if (IS_SET(ch->imm_flags,IMM_WEAPON))
	    def = IS_IMMUNE;
	else if (IS_SET(ch->res_flags,RES_WEAPON))
	    def = IS_RESISTANT;
	else if (IS_SET(ch->vuln_flags,VULN_WEAPON))
	    def = IS_VULNERABLE;
    }
    else /* magical attack */
    {	
	if (IS_SET(ch->imm_flags,IMM_MAGIC))
	    def = IS_IMMUNE;
	else if (IS_SET(ch->res_flags,RES_MAGIC))
	    def = IS_RESISTANT;
	else if (IS_SET(ch->vuln_flags,VULN_MAGIC))
	    def = IS_VULNERABLE;
    }

    /* set bits to check -- VULN etc. must ALL be the same or this will fail */
    switch (dam_type)
    {
	case(DAM_BASH):		bit = IMM_BASH;		break;
	case(DAM_PIERCE):	bit = IMM_PIERCE;	break;
	case(DAM_SLASH):	bit = IMM_SLASH;	break;
	case(DAM_FIRE):		bit = IMM_FIRE;		break;
	case(DAM_COLD):		bit = IMM_COLD;		break;
	case(DAM_LIGHTNING):	bit = IMM_LIGHTNING;	break;
	case(DAM_ACID):		bit = IMM_ACID;		break;
	case(DAM_POISON):	bit = IMM_POISON;	break;
	case(DAM_NEGATIVE):	bit = IMM_NEGATIVE;	break;
	case(DAM_HOLY):		bit = IMM_HOLY;		break;
	case(DAM_ENERGY):	bit = IMM_ENERGY;	break;
	case(DAM_MENTAL):	bit = IMM_MENTAL;	break;
	case(DAM_DISEASE):	bit = IMM_DISEASE;	break;
	case(DAM_DROWNING):	bit = IMM_DROWNING;	break;
	case(DAM_LIGHT):	bit = IMM_LIGHT;	break;
	case(DAM_CHARM):	bit = IMM_CHARM;	break;
	case(DAM_SOUND):	bit = IMM_SOUND;	break;
	default:		return def;
    }

    if (IS_SET(ch->imm_flags,bit))
	immune = IS_IMMUNE;
    else if (IS_SET(ch->res_flags,bit) && immune != IS_IMMUNE)
	immune = IS_RESISTANT;
    else if (IS_SET(ch->vuln_flags,bit))
    {
	if (immune == IS_IMMUNE)
	    immune = IS_RESISTANT;
	else if (immune == IS_RESISTANT)
	    immune = IS_NORMAL;
	else
	    immune = IS_VULNERABLE;
    }

    if (immune == -1)
	return def;
    else
      	return immune;
}

/* checks mob format */
bool is_old_mob(Character *ch)
{
    if (ch->pIndexData == NULL)
	return FALSE;
    else if (ch->pIndexData->new_format)
	return FALSE;
    return TRUE;
}
 
/* for returning skill information */
int get_skill(Character *ch, int sn)
{
    int skill;

    if (sn == -1) /* shorthand for level based skills */
    {
	skill = ch->level * 5 / 2;
    }

    else if (sn < -1 || sn > MAX_SKILL)
    {
	bug("Bad sn %d in get_skill.",sn);
	skill = 0;
    }

    else if (!ch->isNPC())
    {
	if (ch->level < ((PlayerCharacter*)ch)->sk_level[sn])
	    skill = 0;
	else
	    skill = ((PlayerCharacter*)ch)->learned[sn];
    }

    else /* mobiles */
    {


        if (skill_table[sn].spell_fun != spell_null)
	    skill = 40 + 2 * ch->level;

	else if (sn == gsn_sneak || sn == gsn_hide)
	    skill = ch->level * 2 + 20;

        else if ((sn == gsn_dodge && IS_SET(ch->off_flags,OFF_DODGE))
 	||       (sn == gsn_parry && IS_SET(ch->off_flags,OFF_PARRY)))
	    skill = ch->level * 2;

 	else if (sn == gsn_shield_block)
	    skill = 10 + 2 * ch->level;

	else if (sn == gsn_second_attack 
	&& (IS_SET(ch->act,ACT_WARRIOR) || IS_SET(ch->act,ACT_THIEF)))
	    skill = 10 + 3 * ch->level;

	else if (sn == gsn_third_attack && IS_SET(ch->act,ACT_WARRIOR))
	    skill = 4 * ch->level - 40;

	else if (sn == gsn_hand_to_hand)
	    skill = 40 + 2 * ch->level;

 	else if (sn == gsn_trip && IS_SET(ch->off_flags,OFF_TRIP))
	    skill = 10 + 3 * ch->level;

 	else if (sn == gsn_bash && IS_SET(ch->off_flags,OFF_BASH))
	    skill = 10 + 3 * ch->level;

	else if (sn == gsn_disarm 
	     &&  (IS_SET(ch->off_flags,OFF_DISARM) 
	     ||   IS_SET(ch->act,ACT_WARRIOR)
	     ||	  IS_SET(ch->act,ACT_THIEF)))
	    skill = 20 + 3 * ch->level;

	else if (sn == gsn_berserk && IS_SET(ch->off_flags,OFF_BERSERK))
	    skill = 3 * ch->level;

	else if (sn == gsn_kick)
	    skill = 10 + 3 * ch->level;

	else if (sn == gsn_backstab && IS_SET(ch->act,ACT_THIEF))
	    skill = 20 + 2 * ch->level;

  	else if (sn == gsn_rescue)
	    skill = 40 + ch->level; 

	else if (sn == gsn_recall)
	    skill = 40 + ch->level;

	else if (sn == gsn_sword
	||  sn == gsn_dagger
	||  sn == gsn_spear
	||  sn == gsn_mace
	||  sn == gsn_axe
	||  sn == gsn_flail
	||  sn == gsn_whip
	||  sn == gsn_polearm)
	    skill = 40 + 5 * ch->level / 2;

	else 
	   skill = 0;
    }

    if (ch->daze > 0)
    {
	if (skill_table[sn].spell_fun != spell_null)
	    skill /= 2;
	else
	    skill = 2 * skill / 3;
    }

    if ( !ch->isNPC() && ((PlayerCharacter*)ch)->condition[COND_DRUNK]  > 10 )
	skill = 9 * skill / 10;

    return URANGE(0,skill,100);
}

/* for returning weapon information */
int get_weapon_sn(Character *ch)
{
    Object *wield;
    int sn;

    wield = ch->getEquipment(WEAR_WIELD );
    if (wield == NULL || wield->getItemType() != ITEM_WEAPON)
        sn = gsn_hand_to_hand;
    else switch (wield->getValues().at(0))
    {
        default :               sn = -1;                break;
        case(WEAPON_SWORD):     sn = gsn_sword;         break;
        case(WEAPON_DAGGER):    sn = gsn_dagger;        break;
        case(WEAPON_SPEAR):     sn = gsn_spear;         break;
        case(WEAPON_MACE):      sn = gsn_mace;          break;
        case(WEAPON_AXE):       sn = gsn_axe;           break;
        case(WEAPON_FLAIL):     sn = gsn_flail;         break;
        case(WEAPON_WHIP):      sn = gsn_whip;          break;
        case(WEAPON_POLEARM):   sn = gsn_polearm;       break;
   }
   return sn;
}

int get_weapon_skill(Character *ch, int sn)
{
     int skill;

     /* -1 is exotic */
    if (ch->isNPC())
    {
	if (sn == -1)
	    skill = 3 * ch->level;
	else if (sn == gsn_hand_to_hand)
	    skill = 40 + 2 * ch->level;
	else 
	    skill = 40 + 5 * ch->level / 2;
    }
    
    else
    {
	if (sn == -1)
	    skill = 3 * ch->level;
	else
	    skill = ((PlayerCharacter*)ch)->learned[sn];
    }

    return URANGE(0,skill,100);
} 


/* used to de-screw characters */
void reset_char(Character *caller)
{
     int loc,mod,stat;
     Object *obj;
     AFFECT_DATA *af;
     int i;

     if (caller->isNPC())
	return;

    PlayerCharacter * ch = (PlayerCharacter*)caller;

    if (ch->perm_hit == 0 
    ||	ch->perm_mana == 0
    ||  ch->perm_move == 0
    ||	ch->last_level == 0)
    {
    /* do a FULL reset */
	for (loc = 0; loc < MAX_WEAR; loc++)
	{
	    obj = ch->getEquipment(loc);
	    if (obj == NULL)
		continue;
	    if (!obj->isEnchanted())
	    for ( auto af : obj->getObjectIndexData()->affected )
	    {
            mod = af->modifier;
            switch(af->location)
            {
                case APPLY_SEX:	ch->sex		-= mod;
                        if (ch->sex < 0 || ch->sex >2)
                            ch->sex = ch->isNPC() ?
                            0 :
                            ch->true_sex;
                                        break;
                case APPLY_MANA:	ch->max_mana	-= mod;		break;
                case APPLY_HIT:	ch->max_hit	-= mod;		break;
                case APPLY_MOVE:	ch->max_move	-= mod;		break;
            }
	    }

            for ( auto af : obj->getAffectedBy() )
            {
                mod = af->modifier;
                switch(af->location)
                {
                    case APPLY_SEX:     ch->sex         -= mod;         break;
                    case APPLY_MANA:    ch->max_mana    -= mod;         break;
                    case APPLY_HIT:     ch->max_hit     -= mod;         break;
                    case APPLY_MOVE:    ch->max_move    -= mod;         break;
                }
            }
	}
	/* now reset the permanent stats */
	ch->perm_hit 	= ch->max_hit;
	ch->perm_mana 	= ch->max_mana;
	ch->perm_move	= ch->max_move;
	ch->last_level	= ch->played/3600;
	if (ch->true_sex < 0 || ch->true_sex > 2)
	{
		if (ch->sex > 0 && ch->sex < 3)
	    	    ch->true_sex	= ch->sex;
		else
		    ch->true_sex 	= 0;
	}
    }

    /* now restore the character to his/her true condition */
    for (stat = 0; stat < MAX_STATS; stat++)
	ch->mod_stat[stat] = 0;

    if (ch->true_sex < 0 || ch->true_sex > 2)
	ch->true_sex = 0; 
    ch->sex		= ch->true_sex;
    ch->max_hit 	= ch->perm_hit;
    ch->max_mana	= ch->perm_mana;
    ch->max_move	= ch->perm_move;
   
    for (i = 0; i < 4; i++)
    	ch->armor[i]	= 100;

    ch->hitroll		= 0;
    ch->damroll		= 0;
    ch->saving_throw	= 0;

    /* now start adding back the effects */
    for (loc = 0; loc < MAX_WEAR; loc++)
    {
        obj = ch->getEquipment(loc);
        if (obj == NULL) {
            continue;
        }
        for (i = 0; i < 4; i++) {
            ch->armor[i] -= apply_ac( obj, loc, i );
        }

        if (!obj->isEnchanted()) {
            for ( auto af : obj->getObjectIndexData()->affected ) {
                mod = af->modifier;
                switch(af->location)
                {
                    case APPLY_STR:		ch->mod_stat[STAT_STR]	+= mod;	break;
                    case APPLY_DEX:		ch->mod_stat[STAT_DEX]	+= mod; break;
                    case APPLY_INT:		ch->mod_stat[STAT_INT]	+= mod; break;
                    case APPLY_WIS:		ch->mod_stat[STAT_WIS]	+= mod; break;
                    case APPLY_CON:		ch->mod_stat[STAT_CON]	+= mod; break;

                    case APPLY_SEX:		ch->sex			+= mod; break;
                    case APPLY_MANA:	ch->max_mana		+= mod; break;
                    case APPLY_HIT:		ch->max_hit		+= mod; break;
                    case APPLY_MOVE:	ch->max_move		+= mod; break;

                    case APPLY_AC:		
                        for (i = 0; i < 4; i ++)
                        ch->armor[i] += mod; 
                        break;
                    case APPLY_HITROLL:	ch->hitroll		+= mod; break;
                    case APPLY_DAMROLL:	ch->damroll		+= mod; break;
                
                    case APPLY_SAVES:		ch->saving_throw += mod; break;
                    case APPLY_SAVING_ROD: 		ch->saving_throw += mod; break;
                    case APPLY_SAVING_PETRI:	ch->saving_throw += mod; break;
                    case APPLY_SAVING_BREATH: 	ch->saving_throw += mod; break;
                    case APPLY_SAVING_SPELL:	ch->saving_throw += mod; break;
                }
            }
        }
 
        for ( auto af : obj->getAffectedBy() )
        {
            mod = af->modifier;
            switch(af->location)
            {
                case APPLY_STR:         ch->mod_stat[STAT_STR]  += mod; break;
                case APPLY_DEX:         ch->mod_stat[STAT_DEX]  += mod; break;
                case APPLY_INT:         ch->mod_stat[STAT_INT]  += mod; break;
                case APPLY_WIS:         ch->mod_stat[STAT_WIS]  += mod; break;
                case APPLY_CON:         ch->mod_stat[STAT_CON]  += mod; break;
 
                case APPLY_SEX:         ch->sex                 += mod; break;
                case APPLY_MANA:        ch->max_mana            += mod; break;
                case APPLY_HIT:         ch->max_hit             += mod; break;
                case APPLY_MOVE:        ch->max_move            += mod; break;
 
                case APPLY_AC:
                    for (i = 0; i < 4; i ++)
                        ch->armor[i] += mod;
                    break;
		        case APPLY_HITROLL:     ch->hitroll             += mod; break;
                case APPLY_DAMROLL:     ch->damroll             += mod; break;
 
                case APPLY_SAVES:         ch->saving_throw += mod; break;
                case APPLY_SAVING_ROD:          ch->saving_throw += mod; break;
                case APPLY_SAVING_PETRI:        ch->saving_throw += mod; break;
                case APPLY_SAVING_BREATH:       ch->saving_throw += mod; break;
                case APPLY_SAVING_SPELL:        ch->saving_throw += mod; break;
            }
	    }
    }
  
    /* now add back spell effects */
    for (auto af : ch->affected)
    {
        mod = af->modifier;
        switch(af->location)
        {
                case APPLY_STR:         ch->mod_stat[STAT_STR]  += mod; break;
                case APPLY_DEX:         ch->mod_stat[STAT_DEX]  += mod; break;
                case APPLY_INT:         ch->mod_stat[STAT_INT]  += mod; break;
                case APPLY_WIS:         ch->mod_stat[STAT_WIS]  += mod; break;
                case APPLY_CON:         ch->mod_stat[STAT_CON]  += mod; break;
 
                case APPLY_SEX:         ch->sex                 += mod; break;
                case APPLY_MANA:        ch->max_mana            += mod; break;
                case APPLY_HIT:         ch->max_hit             += mod; break;
                case APPLY_MOVE:        ch->max_move            += mod; break;
 
                case APPLY_AC:
                    for (i = 0; i < 4; i ++)
                        ch->armor[i] += mod;
                    break;
                case APPLY_HITROLL:     ch->hitroll             += mod; break;
                case APPLY_DAMROLL:     ch->damroll             += mod; break;
 
                case APPLY_SAVES:         ch->saving_throw += mod; break;
                case APPLY_SAVING_ROD:          ch->saving_throw += mod; break;
                case APPLY_SAVING_PETRI:        ch->saving_throw += mod; break;
                case APPLY_SAVING_BREATH:       ch->saving_throw += mod; break;
                case APPLY_SAVING_SPELL:        ch->saving_throw += mod; break;
        } 
    }

    /* make sure sex is RIGHT!!!! */
    if (ch->sex < 0 || ch->sex > 2)
	ch->sex = ch->true_sex;
}


/*
 * Retrieve a character's age.
 */
int get_age( Character *ch )
{
    return 17 + ( ch->played + (int) (current_time - ch->logon) ) / 72000;
}

/* command for retrieving stats */
int get_curr_stat( Character *ch, int stat )
{
    int max;

    if (ch->isNPC() || ch->level > LEVEL_IMMORTAL)
	max = 25;

    else
    {
	max = ch->getRace()->getPlayerRace()->getMaxStats().at(stat) + 4;

	if (class_table[ch->class_num].attr_prime == stat)
	    max += 2;

	if ( ch->getRace() == race_manager->getRaceByName("human"))
	    max += 1;

 	max = UMIN(max,25);
    }
  
    return URANGE(3,ch->perm_stat[stat] + ch->mod_stat[stat], max);
}

/* command for returning max training score */
int get_max_train( Character *ch, int stat )
{
    int max;

    if (ch->isNPC() || ch->level > LEVEL_IMMORTAL)
	return 25;

    max = ch->getRace()->getPlayerRace()->getMaxStats().at(stat);
    if (class_table[ch->class_num].attr_prime == stat)
    {
	if (ch->getRace() == race_manager->getRaceByName("human"))
	   max += 3;
	else
	   max += 2;
    }
    return UMIN(max,25);
}
   
	
/*
 * Retrieve a character's carry capacity.
 */
int can_carry_n( Character *ch )
{
    if ( !ch->isNPC() && ch->level >= LEVEL_IMMORTAL )
	return 1000;

    if ( ch->isNPC() && IS_SET(ch->act, ACT_PET) )
	return 0;

    return MAX_WEAR +  2 * get_curr_stat(ch,STAT_DEX) + ch->level;
}



/*
 * Retrieve a character's carry capacity.
 */
int can_carry_w( Character *ch )
{
    if ( !ch->isNPC() && ch->level >= LEVEL_IMMORTAL )
	return 10000000;

    if ( ch->isNPC() && IS_SET(ch->act, ACT_PET) )
	return 0;

    return str_app[get_curr_stat(ch,STAT_STR)].carry * 10 + ch->level * 25;
}



/*
 * See if a string is one of the names of an object.
 */

bool is_name ( const char *str, const char *namelist )
{
    char name[MAX_INPUT_LENGTH], part[MAX_INPUT_LENGTH];
    char *list;

    /* fix crash on NULL namelist */
    if (namelist == NULL || namelist[0] == '\0')
    	return FALSE;

    /* fixed to prevent is_name on "" returning TRUE */
    if (str[0] == '\0')
	return FALSE;

    char * string = str_dup(str);
    /* we need ALL parts of string to match part of namelist */
    for ( ; ; )  /* start parsing string */
    {
	str = one_argument((char*)str,part);

	if (part[0] == '\0' )
	    return TRUE;

	/* check to see if this is part of namelist */
	list = str_dup(namelist);
	for ( ; ; )  /* start parsing namelist */
	{
	    list = one_argument(list,name);
	    if (name[0] == '\0')  /* this name was not found */
		return FALSE;

	    if (!str_prefix(string,name))
		return TRUE; /* full pattern match */

	    if (!str_prefix(part,name))
		break;
	}
    }

    free_string(list);
    free_string(string);
}

bool is_exact_name(char *str, char *namelist )
{
    char name[MAX_INPUT_LENGTH];

    if (namelist == NULL)
	return FALSE;

    for ( ; ; )
    {
	namelist = one_argument( namelist, name );
	if ( name[0] == '\0' )
	    return FALSE;
	if ( !str_cmp( str, name ) )
	    return TRUE;
    }
}

/* enchanted stuff for eq */
void affect_enchant(Object *obj)
{
    /* okay, move all the old flags into new vectors if we have to */
    if (!obj->isEnchanted())
    {
        obj->setIsEnchanted(true);

        for (auto paf : obj->getObjectIndexData()->affected)
        {
	        auto af_new = new_affect();

	        af_new->where	= paf->where;
            af_new->type        = UMAX(0,paf->type);
            af_new->level       = paf->level;
            af_new->duration    = paf->duration;
            af_new->location    = paf->location;
            af_new->modifier    = paf->modifier;
            af_new->bitvector   = paf->bitvector;
            obj->addAffect(af_new);
        }
    }
}
           

/* find an effect in an affect list */
AFFECT_DATA  *affect_find(AFFECT_DATA *paf, int sn)
{
    AFFECT_DATA *paf_find;
    
    for ( paf_find = paf; paf_find != NULL; paf_find = paf_find->next )
    {
        if ( paf_find->type == sn )
	return paf_find;
    }

    return NULL;
}

/*
 * Strip all affects of a given sn.
 */
void affect_strip( Character *ch, int sn )
{
    for ( auto paf : ch->affected)
    {
        if ( paf->type == sn ) {
            ch->removeAffect( paf );
        }
    }
}



/*
 * Return true if a char is affected by a spell.
 */
bool is_affected( Character *ch, int sn ) {
    return ch->findAffectBySn(sn) != NULL;
}

/*
 * Add or enhance an affect.
 */
void affect_join( Character *ch, AFFECT_DATA *paf )
{
    for ( auto paf_old : ch->affected )
    {
        if ( paf_old->type == paf->type )
        {
            paf->level = (paf->level += paf_old->level) / 2;
            paf->duration += paf_old->duration;
            paf->modifier += paf_old->modifier;
            ch->removeAffect( paf_old );
            break;
        }
    }

    ch->giveAffect( paf );
}



/*
 * Move a char out of a room.
 */
void char_from_room( Character *ch )
{
    Object *obj;

    if ( ch->in_room == NULL )
    {
	bug( "Char_from_room: NULL.", 0 );
	return;
    }

    if ( !ch->isNPC() )
	--ch->in_room->area->nplayer;

    if ( ( obj = ch->getEquipment(WEAR_LIGHT ) ) != NULL
    &&   obj->getItemType() == ITEM_LIGHT
    &&   obj->getValues().at(2) != 0
    &&   ch->in_room->light > 0 )
	--ch->in_room->light;

    if ( ch == ch->in_room->people )
    {
	ch->in_room->people = ch->next_in_room;
    }
    else
    {
	Character *prev;

	for ( prev = ch->in_room->people; prev; prev = prev->next_in_room )
	{
	    if ( prev->next_in_room == ch )
	    {
		prev->next_in_room = ch->next_in_room;
		break;
	    }
	}

	if ( prev == NULL )
	    bug( "Char_from_room: ch not found.", 0 );
    }

    ch->in_room      = NULL;
    ch->next_in_room = NULL;
    ch->getOntoObject(nullptr);  /* sanity check! */
    return;
}



/*
 * Move a char into a room.
 */
void char_to_room( Character *ch, ROOM_INDEX_DATA *pRoomIndex )
{
    Object *obj;

    if ( pRoomIndex == NULL )
    {
	ROOM_INDEX_DATA *room;

	bug( "Char_to_room: NULL.", 0 );
	
	if ((room = get_room_index(ROOM_VNUM_TEMPLE)) != NULL)
	    char_to_room(ch,room);
	
	return;
    }

    ch->in_room		= pRoomIndex;
    ch->next_in_room	= pRoomIndex->people;
    pRoomIndex->people	= ch;

    if ( !ch->isNPC() )
    {
	if (ch->in_room->area->empty)
	{
	    ch->in_room->area->empty = FALSE;
	    ch->in_room->area->age = 0;
	}
	++ch->in_room->area->nplayer;
    }

    if ( ( obj = ch->getEquipment(WEAR_LIGHT ) ) != NULL
    &&   obj->getItemType() == ITEM_LIGHT
    &&   obj->getValues().at(2) != 0 )
	++ch->in_room->light;
	
    if (IS_AFFECTED(ch,AFF_PLAGUE))
    {
        AFFECT_DATA *af, plague;
        Character *vch;
        
        for ( auto aff : ch->affected )
        {
            af = aff;
            if (af->type == gsn_plague)
                break;
        }
        
        if (af == NULL)
        {
            REMOVE_BIT(ch->affected_by,AFF_PLAGUE);
            return;
        }
        
        if (af->level == 1)
            return;
        
	plague.where		= TO_AFFECTS;
        plague.type 		= gsn_plague;
        plague.level 		= af->level - 1; 
        plague.duration 	= number_range(1,2 * plague.level);
        plague.location		= APPLY_STR;
        plague.modifier 	= -5;
        plague.bitvector 	= AFF_PLAGUE;
        
        for ( vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room)
        {
            if (!saves_spell(plague.level - 2,vch,DAM_DISEASE) 
	    &&  !IS_IMMORTAL(vch) &&
            	!IS_AFFECTED(vch,AFF_PLAGUE) && number_bits(6) == 0)
            {
            	send_to_char("You feel hot and feverish.\n\r",vch);
            	act("$n shivers and looks very ill.",vch,NULL,NULL,TO_ROOM, POS_RESTING);
            	affect_join(vch,&plague);
            }
        }
    }


    return;
}



/*
 * Take an obj from its character.
 */
void obj_from_char( Object *obj )
{
    Character *ch;

    if ( ( ch = obj->getCarriedBy() ) == NULL )
    {
        bug( "Obj_from_char: null ch.", 0 );
        return;
    }

    ch->removeObject(obj);
}



/*
 * Find the ac value of an obj, including position effect.
 */
int apply_ac( Object *obj, int iWear, int type )
{
    if ( obj->getItemType() != ITEM_ARMOR )
	return 0;

    switch ( iWear )
    {
    case WEAR_BODY:	return 3 * obj->getValues().at(type);
    case WEAR_HEAD:	return 2 * obj->getValues().at(type);
    case WEAR_LEGS:	return 2 * obj->getValues().at(type);
    case WEAR_FEET:	return     obj->getValues().at(type);
    case WEAR_FINGER_L:	return	   obj->getValues().at(type);
    case WEAR_FINGER_R:	return	   obj->getValues().at(type);
    case WEAR_HANDS:	return     obj->getValues().at(type);
    case WEAR_ARMS:	return     obj->getValues().at(type);
    case WEAR_SHIELD:	return     obj->getValues().at(type);
    case WEAR_NECK_1:	return     obj->getValues().at(type);
    case WEAR_NECK_2:	return     obj->getValues().at(type);
    case WEAR_ABOUT:	return 2 * obj->getValues().at(type);
    case WEAR_WAIST:	return     obj->getValues().at(type);
    case WEAR_WRIST_L:	return     obj->getValues().at(type);
    case WEAR_WRIST_R:	return     obj->getValues().at(type);
    case WEAR_HOLD:	return     obj->getValues().at(type);
    case WEAR_BACK:	return	   obj->getValues().at(type);
    }

    return 0;
}



/*
 * Equip a char with an obj.
 */
void equip_char( Character *ch, Object *obj, int iWear )
{
    int i;

    if ( ch->getEquipment(iWear ) != NULL )
    {
	bug( "Equip_char: already equipped (%d).", iWear );
	return;
    }

/*
 * We don't want align-based equipment.
 * -Blizzard
    if ( ( obj->hasStat(ITEM_ANTI_EVIL)    && IS_EVIL(ch)    )
    ||   ( obj->hasStat(ITEM_ANTI_GOOD)    && IS_GOOD(ch)    )
    ||   ( obj->hasStat(ITEM_ANTI_NEUTRAL) && IS_NEUTRAL(ch) ) )
    {
	act( "You are zapped by $p and drop it.", ch, obj, NULL, TO_CHAR, POS_RESTING );
	act( "$n is zapped by $p and drops it.",  ch, obj, NULL, TO_ROOM, POS_RESTING );
	obj_from_char( obj );
	obj_to_room( obj, ch->in_room );
	return;
    }
*/

    for (i = 0; i < 4; i++)
    	ch->armor[i]      	-= apply_ac( obj, iWear,i );
    obj->setWearLocation(iWear);

    if (!obj->isEnchanted())
	for ( auto paf : obj->getObjectIndexData()->affected ) {
	    if ( paf->location != APPLY_SPELL_AFFECT )
	        ch->modifyAffect( paf, TRUE );
    }
    for ( auto paf : obj->getAffectedBy() ) {
        if ( paf->location == APPLY_SPELL_AFFECT )
                ch->giveAffect( paf );
        else
            ch->modifyAffect( paf, TRUE );
    }

    if ( obj->getItemType() == ITEM_LIGHT
    &&   obj->getValues().at(2) != 0
    &&   ch->in_room != NULL )
	++ch->in_room->light;

    return;
}



/*
 * Move an obj out of a room.
 */
void obj_from_room( Object *obj )
{
    ROOM_INDEX_DATA *in_room;
    Character *ch;

    if ( ( in_room = obj->getInRoom() ) == NULL )
    {
        bug( "obj_from_room: NULL.", 0 );
        return;
    }

    for (ch = in_room->people; ch != NULL; ch = ch->next_in_room)
	if (ch->onObject() == obj)
	    ch->getOntoObject(nullptr);

    in_room->contents.remove(obj);

    obj->setInRoom(nullptr);
    return;
}



/*
 * Move an obj into a room.
 */
void obj_to_room( Object *obj, ROOM_INDEX_DATA *pRoomIndex )
{
    pRoomIndex->contents.push_back(obj);
    obj->setInRoom(pRoomIndex);
    obj->setCarriedBy(nullptr);
    obj->setInObject(nullptr);
    return;
}



/*
 * Move an object out of an object.
 */
void obj_from_obj( Object *obj )
{
    Object *obj_from;

    if ( ( obj_from = obj->getInObject() ) == NULL )
    {
        bug( "Obj_from_obj: null obj_from.", 0 );
        return;
    }
    
    obj_from->removeObject(obj);
}



/*
 * Extract an obj from the world.
 */
void extract_obj( Object *obj )
{
    if ( obj->getInRoom() != NULL ) {
	    obj_from_room( obj );
    } else if ( obj->getCarriedBy() != NULL ) {
    	obj_from_char( obj );
    } else if ( obj->getInObject() != NULL ) {
    	obj_from_obj( obj );
    }

    for ( auto obj_content : obj->getContents() )
    {
        extract_obj( obj_content );
    }

    object_list.remove(obj);

    --obj->getObjectIndexData()->count;
    delete obj;
}



/*
 * Extract a char from the world.
 */
void extract_char( Character *ch, bool fPull )
{
    Character *wch;
    Object *obj;
    Object *obj_next;

    /* doesn't seem to be necessary
    if ( ch->in_room == NULL )
    {
	bug( "Extract_char: NULL.", 0 );
	return;
    }
    */
    
    nuke_pets(ch);
    ch->pet = NULL; /* just in case */

    if ( fPull )

	die_follower( ch );
    
    stop_fighting( ch, TRUE );

    if (ch->in_room != NULL)
        char_from_room( ch );

    /* Death room is set in the clan tabe now */
    if ( !fPull && ch->isClanned())
    {
        if (get_room_index(ch->getClan()->getDeathRoomVnum())) {
            char_to_room(ch,get_room_index(ch->getClan()->getDeathRoomVnum()));
        } else {
            char_to_room(ch,get_room_index(ROOM_VNUM_ALTAR));
        }
        return;
    }
    else if ( !fPull && !ch->isClanned())
    {
	char_to_room(ch,get_room_index(ROOM_VNUM_ALTAR));
	return;
    }

    if ( ch->isNPC() )
	--ch->pIndexData->count;

    if ( ch->desc != NULL && ch->desc->original != NULL )
    {
        do_return( ch, (char*)"" );
        ch->desc = NULL;
    }

    for (std::list<Character*>::iterator it = char_list.begin(); it != char_list.end(); it++)
    {
        wch = *it;
	if ( wch->reply == ch )
	    wch->reply = NULL;
	if ( ch->mprog_target == wch )
	    wch->mprog_target = NULL;
    }

    char_list.remove(ch);

    if ( ch->desc != NULL ) {
        ch->desc->character = NULL;
    }
    delete ch;
    return;
}



/*
 * Find a char in the room.
 */
Character *get_char_room( Character *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    Character *rch;
    int number;
    int count;

    number = number_argument( argument, arg );
    count  = 0;
    if ( !str_cmp( arg, "self" ) )
	return ch;
    for ( rch = ch->in_room->people; rch != NULL; rch = rch->next_in_room )
    {
	if ( !ch->can_see( rch ) || !is_name( arg, rch->getName().c_str() ) )
	    continue;
	if ( ++count == number )
	    return rch;
    }

    return NULL;
}


PlayerCharacter * get_player_world( Character *ch, char *argument) {
    auto f = get_char_world(ch, argument);
    if (f && !f->isNPC()) {
        return (PlayerCharacter*)f;
    }
    return nullptr;
}


/*
 * Find a char in the world.
 */
Character *get_char_world( Character *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    Character *wch;
    int number;
    int count;

    if ( ( wch = get_char_room( ch, argument ) ) != NULL )
	return wch;

    number = number_argument( argument, arg );
    count  = 0;
    for (std::list<Character*>::iterator it = char_list.begin(); it != char_list.end(); it++)
    {
        wch = *it;
        if ( wch->in_room == NULL || !ch->can_see( wch ) 
        ||   !is_name( arg, wch->getName().c_str() ) )
            continue;
        if ( ++count == number )
            return wch;
    }

    return NULL;
}



/*
 * Find some object with a given index data.
 * Used by area-reset 'P' command.
 */
Object *get_obj_type( OBJ_INDEX_DATA *pObjIndex )
{
    for ( auto obj : object_list )
    {
        if ( obj->getObjectIndexData() == pObjIndex )
            return obj;
    }

    return nullptr;
}


/*
 * Find an obj in player's inventory.
 */
Object *get_obj_carry( Character *ch, char *argument, Character *viewer )
{
    char arg[MAX_INPUT_LENGTH];
    int number;
    int count;

    number = number_argument( argument, arg );
    count  = 0;
    for ( auto obj : ch->getCarrying() )
    {
        if ( obj->getWearLocation() == WEAR_NONE
        &&   (viewer->can_see( obj ) ) 
        &&   is_name( arg, obj->getName().c_str() ) )
        {
            if ( ++count == number )
            return obj;
        }
    }

    return NULL;
}



/*
 * Find an obj in player's equipment.
 */
Object *get_obj_wear( Character *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    int number;
    int count;

    number = number_argument( argument, arg );
    count  = 0;
    for ( auto obj : ch->getCarrying() )
    {
        if ( obj->getWearLocation() != WEAR_NONE
        &&   ch->can_see( obj )
        &&   is_name( arg, obj->getName().c_str() ) )
        {
            if ( ++count == number )
            return obj;
        }
    }

    return NULL;
}



/*
 * Find an obj in the room or in inventory.
 */
Object *get_obj_here( Character *ch, char *argument )
{
    Object *obj;

    obj = ObjectHelper::findInList( ch, argument, ch->in_room->contents );
    if ( obj != NULL )
	return obj;

    if ( ( obj = get_obj_carry( ch, argument, ch ) ) != NULL )
	return obj;

    if ( ( obj = get_obj_wear( ch, argument ) ) != NULL )
	return obj;

    return NULL;
}



/*
 * Find an obj in the world.
 */
Object *get_obj_world( Character *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    Object *obj;
    int number;
    int count;

    if ( ( obj = get_obj_here( ch, argument ) ) != NULL ) {
    	return obj;
    }

    number = number_argument( argument, arg );
    count  = 0;
    for ( auto obj : object_list )
    {
        if ( ch->can_see( obj ) && is_name( arg, obj->getName().c_str() ) )
        {
            if ( ++count == number )
            return obj;
        }
    }

    return NULL;
}

/* deduct cost from a character */

void deduct_cost(Character *ch, int cost)
{
    int silver = 0, gold = 0;

    silver = UMIN(ch->silver,cost); 

    if (silver < cost)
    {
	gold = ((cost - silver + 99) / 100);
	silver = cost - 100 * gold;
    }

    ch->gold -= gold;
    ch->silver -= silver;

    if (ch->gold < 0)
    {
	bug("deduct costs: gold %d < 0",ch->gold);
	ch->gold = 0;
    }
    if (ch->silver < 0)
    {
	bug("deduct costs: silver %d < 0",ch->silver);
	ch->silver = 0;
    }
}   
/*
 * Create a 'money' obj.
 */
Object *create_money( int gold, int silver )
{
    char buf[MAX_STRING_LENGTH];
    Object *obj;

    if ( gold < 0 || silver < 0 || (gold == 0 && silver == 0) )
    {
	bug( "Create_money: zero or negative money.",UMIN(gold,silver));
	gold = UMAX(1,gold);
	silver = UMAX(1,silver);
    }

    if (gold == 0 && silver == 1)
    {
	obj = ObjectHelper::createFromIndex( get_obj_index( OBJ_VNUM_SILVER_ONE ), 0 );
    }
    else if (gold == 1 && silver == 0)
    {
	obj = ObjectHelper::createFromIndex( get_obj_index( OBJ_VNUM_GOLD_ONE), 0 );
    }
    else if (silver == 0)
    {
        obj = ObjectHelper::createFromIndex( get_obj_index( OBJ_VNUM_GOLD_SOME ), 0 );
        snprintf(buf, sizeof(buf), obj->getShortDescription().c_str(), gold );
        obj->setShortDescription(buf);
        obj->getValues().at(1)           = gold;
        obj->setCost(gold);
	    obj->setWeight(gold/5);
    }
    else if (gold == 0)
    {
        obj = ObjectHelper::createFromIndex( get_obj_index( OBJ_VNUM_SILVER_SOME ), 0 );
        snprintf(buf, sizeof(buf), obj->getShortDescription().c_str(), silver );
        obj->setShortDescription( buf );
        obj->getValues().at(0)           = silver;
        obj->setCost(silver);
	    obj->setWeight(silver/20);
    }
 
    else
    {
        obj = ObjectHelper::createFromIndex( get_obj_index( OBJ_VNUM_COINS ), 0 );
        snprintf(buf, sizeof(buf), obj->getShortDescription().c_str(), silver, gold );
        obj->setShortDescription( buf );
        obj->getValues().at(0)		= silver;
        obj->getValues().at(1)		= gold;
        obj->setCost(100 * gold + silver);
        obj->setWeight(gold / 5 + silver / 20);
    }

    return obj;
}



/*
 * Return # of objects which an object counts as.
 * Thanks to Tony Chamberlain for the correct recursive code here.
 */
int get_obj_number( Object *obj )
{
    int number;
 
    if (obj->getItemType() == ITEM_CONTAINER || obj->getItemType() == ITEM_MONEY
    ||  obj->getItemType() == ITEM_GEM || obj->getItemType() == ITEM_JEWELRY)
        number = 0;
    else
        number = 1;
 
    for ( auto obj : obj->getContents() ) {
        number += get_obj_number( obj );
    }
 
    return number;
}


/*
 * Return weight of an object, including weight of contents.
 */
int get_obj_weight( Object *obj )
{
    int weight;

    weight = obj->getWeight();
    for ( auto tobj : obj->getContents() ) {
    	weight += get_obj_weight( tobj ) * WEIGHT_MULT(obj) / 100;
    }

    return weight;
}

int get_true_weight(Object *obj)
{
    int weight;
 
    weight = obj->getWeight();
    for ( auto obj : obj->getContents() ) {
        weight += get_obj_weight( obj );
    }
 
    return weight;
}

/* Search a room for char or obj */
ROOM_INDEX_DATA *find_location( Character *ch, char *arg )
{
    Character *victim;
    Object *obj;

    if ( is_number(arg) )
        return get_room_index( atoi( arg ) );

    if ( ( victim = get_char_world( ch, arg ) ) != NULL )
        return victim->in_room;

    if ( ( obj = get_obj_world( ch, arg ) ) != NULL )
        return obj->getInRoom();

    return NULL;
}

/*
 * True if room is dark.
 */
bool room_is_dark( ROOM_INDEX_DATA *pRoomIndex )
{
    if ( pRoomIndex->light > 0 )
	return FALSE;

    if ( IS_SET(pRoomIndex->room_flags, ROOM_DARK) )
	return TRUE;

    if ( pRoomIndex->sector_type == SECT_INSIDE
    ||   pRoomIndex->sector_type == SECT_CITY )
	return FALSE;

    if ( weather_info.sunlight == SUN_SET
    ||   weather_info.sunlight == SUN_DARK )
	return TRUE;

    return FALSE;
}


bool is_room_owner(Character *ch, ROOM_INDEX_DATA *room)
{
    if (room->owner == NULL || room->owner[0] == '\0')
	return FALSE;

    return is_name(ch->getName().c_str(),room->owner);
}

/*
 * True if room is private.
 */
bool room_is_private( ROOM_INDEX_DATA *pRoomIndex )
{
    Character *rch;
    int count;


    if (pRoomIndex->owner != NULL && pRoomIndex->owner[0] != '\0')
	return TRUE;

    count = 0;
    for ( rch = pRoomIndex->people; rch != NULL; rch = rch->next_in_room )
	count++;

    if ( IS_SET(pRoomIndex->room_flags, ROOM_PRIVATE)  && count >= 2 )
	return TRUE;

    if ( IS_SET(pRoomIndex->room_flags, ROOM_SOLITARY) && count >= 1 )
	return TRUE;
    
    if ( IS_SET(pRoomIndex->room_flags, ROOM_IMP_ONLY) )
	return TRUE;

    return FALSE;
}

/* visibility on a room -- for entering and exits */
bool can_see_room( Character *ch, ROOM_INDEX_DATA *pRoomIndex )
{
    if (IS_SET(pRoomIndex->room_flags, ROOM_IMP_ONLY) 
    &&  ch->getTrust() < MAX_LEVEL)
	return FALSE;

    if (IS_SET(pRoomIndex->room_flags, ROOM_GODS_ONLY)
    &&  !IS_IMMORTAL(ch))
	return FALSE;

    if (IS_SET(pRoomIndex->room_flags, ROOM_HEROES_ONLY)
    &&  !IS_IMMORTAL(ch))
	return FALSE;

    if (IS_SET(pRoomIndex->room_flags,ROOM_NEWBIES_ONLY)
    &&  ch->level > 5 && !IS_IMMORTAL(ch))
	return FALSE;

    if (ch->isNPC() && pRoomIndex->clan)
	return FALSE;

    if (!IS_IMMORTAL(ch) && pRoomIndex->clan && ch->getClan() != pRoomIndex->clan)
	return FALSE;

    return TRUE;
}



/*
 * True if char can drop obj.
 */
bool can_drop_obj( Character *ch, Object *obj )
{
    if ( !IS_SET(obj->getExtraFlags(), ITEM_NODROP) )
	return TRUE;

    if ( !ch->isNPC() && ch->level >= LEVEL_IMMORTAL )
	return TRUE;

    return FALSE;
}


/*
 * Return ascii name of an affect location.
 */
const char *affect_loc_name( int location )
{
    switch ( location )
    {
    case APPLY_NONE:            return "none";
    case APPLY_STR:		return "strength";
    case APPLY_DEX:		return "dexterity";
    case APPLY_INT:		return "intelligence";
    case APPLY_WIS:		return "wisdom";
    case APPLY_CON:		return "constitution";
    case APPLY_SEX:		return "sex";
    case APPLY_CLASS:		return "class";
    case APPLY_LEVEL:		return "level";
    case APPLY_AGE:		return "age";
    case APPLY_MANA:		return "mana";
    case APPLY_HIT:		return "hp";
    case APPLY_MOVE:		return "moves";
    case APPLY_GOLD:		return "gold";
    case APPLY_EXP:		return "experience";
    case APPLY_AC:		return "armor class";
    case APPLY_HITROLL:		return "hit roll";
    case APPLY_DAMROLL:		return "damage roll";
    case APPLY_SAVES:		return "saves";
    case APPLY_SAVING_ROD:	return "save vs rod";
    case APPLY_SAVING_PETRI:	return "save vs petrification";
    case APPLY_SAVING_BREATH:	return "save vs breath";
    case APPLY_SAVING_SPELL:	return "save vs spell";
    case APPLY_SPELL_AFFECT:	return (char*)"none";
    }

    bug( "Affect_location_name: unknown location %d.", location );
    return "(unknown)";
}



/*
 * Return ascii name of an affect bit vector.
 */
char *affect_bit_name( int vector )
{
    static char buf[512];

    buf[0] = '\0';
    if ( vector & AFF_BLIND         ) strcat( buf, " blind"         );
    if ( vector & AFF_INVISIBLE     ) strcat( buf, " invisible"     );
    if ( vector & AFF_DETECT_EVIL   ) strcat( buf, " detect_evil"   );
    if ( vector & AFF_DETECT_GOOD   ) strcat( buf, " detect_good"   );
    if ( vector & AFF_DETECT_INVIS  ) strcat( buf, " detect_invis"  );
    if ( vector & AFF_DETECT_MAGIC  ) strcat( buf, " detect_magic"  );
    if ( vector & AFF_DETECT_HIDDEN ) strcat( buf, " detect_hidden" );
    if ( vector & AFF_SANCTUARY     ) strcat( buf, " sanctuary"     );
    if ( vector & AFF_FAERIE_FIRE   ) strcat( buf, " faerie_fire"   );
    if ( vector & AFF_INFRARED      ) strcat( buf, " infrared"      );
    if ( vector & AFF_CURSE         ) strcat( buf, " curse"         );
    if ( vector & AFF_POISON        ) strcat( buf, " poison"        );
    if ( vector & AFF_PROTECT_EVIL  ) strcat( buf, " prot_evil"     );
    if ( vector & AFF_PROTECT_GOOD  ) strcat( buf, " prot_good"     );
    if ( vector & AFF_SLEEP         ) strcat( buf, " sleep"         );
    if ( vector & AFF_SNEAK         ) strcat( buf, " sneak"         );
    if ( vector & AFF_HIDE          ) strcat( buf, " hide"          );
    if ( vector & AFF_CHARM         ) strcat( buf, " charm"         );
    if ( vector & AFF_FLYING        ) strcat( buf, " flying"        );
    if ( vector & AFF_PASS_DOOR     ) strcat( buf, " pass_door"     );
    if ( vector & AFF_BERSERK	    ) strcat( buf, " berserk"	    );
    if ( vector & AFF_CALM	    ) strcat( buf, " calm"	    );
    if ( vector & AFF_HASTE	    ) strcat( buf, " haste"	    );
    if ( vector & AFF_SLOW          ) strcat( buf, " slow"          );
    if ( vector & AFF_PLAGUE	    ) strcat( buf, " plague" 	    );
    if ( vector & AFF_DARK_VISION   ) strcat( buf, " dark_vision"   );
    return ( buf[0] != '\0' ) ? buf+1 : (char*)"none";
}



/*
 * Return ascii name of extra flags vector.
 */
char *extra_bit_name( int extra_flags )
{
    static char buf[512];

    buf[0] = '\0';
    if ( extra_flags & ITEM_GLOW         ) strcat( buf, " glow"         );
    if ( extra_flags & ITEM_HUM          ) strcat( buf, " hum"          );
    if ( extra_flags & ITEM_DARK         ) strcat( buf, " dark"         );
    if ( extra_flags & ITEM_LOCK         ) strcat( buf, " lock"         );
    if ( extra_flags & ITEM_EVIL         ) strcat( buf, " evil"         );
    if ( extra_flags & ITEM_INVIS        ) strcat( buf, " invis"        );
    if ( extra_flags & ITEM_MAGIC        ) strcat( buf, " magic"        );
    if ( extra_flags & ITEM_NODROP       ) strcat( buf, " nodrop"       );
    if ( extra_flags & ITEM_BLESS        ) strcat( buf, " bless"        );
    if ( extra_flags & ITEM_ANTI_GOOD    ) strcat( buf, " anti-good"    );
    if ( extra_flags & ITEM_ANTI_EVIL    ) strcat( buf, " anti-evil"    );
    if ( extra_flags & ITEM_ANTI_NEUTRAL ) strcat( buf, " anti-neutral" );
    if ( extra_flags & ITEM_NOREMOVE     ) strcat( buf, " noremove"     );
    if ( extra_flags & ITEM_INVENTORY    ) strcat( buf, " inventory"    );
    if ( extra_flags & ITEM_NOPURGE	 ) strcat( buf, " nopurge"	);
    if ( extra_flags & ITEM_VIS_DEATH	 ) strcat( buf, " vis_death"	);
    if ( extra_flags & ITEM_ROT_DEATH	 ) strcat( buf, " rot_death"	);
    if ( extra_flags & ITEM_NOLOCATE	 ) strcat( buf, " no_locate"	);
    if ( extra_flags & ITEM_SELL_EXTRACT ) strcat( buf, " sell_extract" );
    if ( extra_flags & ITEM_BURN_PROOF	 ) strcat( buf, " burn_proof"	);
    if ( extra_flags & ITEM_NOUNCURSE	 ) strcat( buf, " no_uncurse"	);
    if ( extra_flags & ITEM_ARTIFACT	 ) strcat( buf, " artifact"	);
    return ( buf[0] != '\0' ) ? buf+1 : (char*)"none";
}

/* return ascii name of an act vector */
char *act_bit_name( int act_flags )
{
    static char buf[512];

    buf[0] = '\0';

    if (IS_SET(act_flags,ACT_IS_NPC))
    { 
 	strcat(buf," npc");
    	if (act_flags & ACT_SENTINEL 	) strcat(buf, " sentinel");
    	if (act_flags & ACT_SCAVENGER	) strcat(buf, " scavenger");
	if (act_flags & ACT_AGGRESSIVE	) strcat(buf, " aggressive");
	if (act_flags & ACT_STAY_AREA	) strcat(buf, " stay_area");
	if (act_flags & ACT_WIMPY	) strcat(buf, " wimpy");
	if (act_flags & ACT_PET		) strcat(buf, " pet");
	if (act_flags & ACT_TRAIN	) strcat(buf, " train");
	if (act_flags & ACT_PRACTICE	) strcat(buf, " practice");
	if (act_flags & ACT_UNDEAD	) strcat(buf, " undead");
	if (act_flags & ACT_CLERIC	) strcat(buf, " cleric");
	if (act_flags & ACT_MAGE	) strcat(buf, " mage");
	if (act_flags & ACT_THIEF	) strcat(buf, " thief");
	if (act_flags & ACT_WARRIOR	) strcat(buf, " warrior");
	if (act_flags & ACT_NOALIGN	) strcat(buf, " no_align");
	if (act_flags & ACT_NOPURGE	) strcat(buf, " no_purge");
	if (act_flags & ACT_IS_HEALER	) strcat(buf, " healer");
	if (act_flags & ACT_IS_CHANGER  ) strcat(buf, " changer");
	if (act_flags & ACT_GAIN	) strcat(buf, " skill_train");
	if (act_flags & ACT_UPDATE_ALWAYS) strcat(buf," update_always");
    }
    else
    {
	strcat(buf," player");
	if (act_flags & PLR_IS_MORPHED	) strcat(buf, " morphed");
	if (act_flags & PLR_AUTOASSIST	) strcat(buf, " autoassist");
	if (act_flags & PLR_AUTOEXIT	) strcat(buf, " autoexit");
	if (act_flags & PLR_AUTOLOOT	) strcat(buf, " autoloot");
	if (act_flags & PLR_AUTOSAC	) strcat(buf, " autosac");
	if (act_flags & PLR_AUTOGOLD	) strcat(buf, " autogold");
	if (act_flags & PLR_AUTOSPLIT	) strcat(buf, " autosplit");
	if (act_flags & PLR_HOLYLIGHT	) strcat(buf, " holy_light");
	if (act_flags & PLR_CANLOOT	) strcat(buf, " loot_corpse");
	if (act_flags & PLR_NOSUMMON	) strcat(buf, " no_summon");
	if (act_flags & PLR_NOFOLLOW	) strcat(buf, " no_follow");
	if (act_flags & PLR_FREEZE	) strcat(buf, " frozen");
	if (act_flags & PLR_THIEF	) strcat(buf, " thief");
    }
    return ( buf[0] != '\0' ) ? buf+1 : (char*)"none";
}

char *comm_bit_name(int comm_flags)
{
    static char buf[512];

    buf[0] = '\0';

    if (comm_flags & COMM_QUIET		) strcat(buf, " quiet");
    if (comm_flags & COMM_DEAF		) strcat(buf, " deaf");
    if (comm_flags & COMM_NOWIZ		) strcat(buf, " no_wiz");
    if (comm_flags & COMM_NOAUCTION	) strcat(buf, " no_auction");
    if (comm_flags & COMM_NOGOSSIP	) strcat(buf, " no_gossip");
    if (comm_flags & COMM_NOQUESTION	) strcat(buf, " no_question");
    if (comm_flags & COMM_NOMUSIC	) strcat(buf, " no_music");
    if (comm_flags & COMM_NOQUOTE	) strcat(buf, " no_quote");
    if (comm_flags & COMM_COMPACT	) strcat(buf, " compact");
    if (comm_flags & COMM_BRIEF		) strcat(buf, " brief");
    if (comm_flags & COMM_PROMPT	) strcat(buf, " prompt");
    if (comm_flags & COMM_COMBINE	) strcat(buf, " combine");
    if (comm_flags & COMM_NOEMOTE	) strcat(buf, " no_emote");
    if (comm_flags & COMM_NOSHOUT	) strcat(buf, " no_shout");
    if (comm_flags & COMM_NOTELL	) strcat(buf, " no_tell");
    if (comm_flags & COMM_NOCHANNELS	) strcat(buf, " no_channels");
    

    return ( buf[0] != '\0' ) ? buf+1 : (char*)"none";
}

char *imm_bit_name(int imm_flags)
{
    static char buf[512];

    buf[0] = '\0';

    if (imm_flags & IMM_SUMMON		) strcat(buf, " summon");
    if (imm_flags & IMM_CHARM		) strcat(buf, " charm");
    if (imm_flags & IMM_MAGIC		) strcat(buf, " magic");
    if (imm_flags & IMM_WEAPON		) strcat(buf, " weapon");
    if (imm_flags & IMM_BASH		) strcat(buf, " blunt");
    if (imm_flags & IMM_PIERCE		) strcat(buf, " piercing");
    if (imm_flags & IMM_SLASH		) strcat(buf, " slashing");
    if (imm_flags & IMM_FIRE		) strcat(buf, " fire");
    if (imm_flags & IMM_COLD		) strcat(buf, " cold");
    if (imm_flags & IMM_LIGHTNING	) strcat(buf, " lightning");
    if (imm_flags & IMM_ACID		) strcat(buf, " acid");
    if (imm_flags & IMM_POISON		) strcat(buf, " poison");
    if (imm_flags & IMM_NEGATIVE	) strcat(buf, " negative");
    if (imm_flags & IMM_HOLY		) strcat(buf, " holy");
    if (imm_flags & IMM_ENERGY		) strcat(buf, " energy");
    if (imm_flags & IMM_MENTAL		) strcat(buf, " mental");
    if (imm_flags & IMM_DISEASE	) strcat(buf, " disease");
    if (imm_flags & IMM_DROWNING	) strcat(buf, " drowning");
    if (imm_flags & IMM_LIGHT		) strcat(buf, " light");
    if (imm_flags & VULN_IRON		) strcat(buf, " iron");
    if (imm_flags & VULN_WOOD		) strcat(buf, " wood");
    if (imm_flags & VULN_SILVER	) strcat(buf, " silver");

    return ( buf[0] != '\0' ) ? buf+1 : (char*)"none";
}

char *wear_bit_name(int wear_flags)
{
    static char buf[512];

    buf [0] = '\0';
    if (wear_flags & ITEM_TAKE		) strcat(buf, " take");
    if (wear_flags & ITEM_WEAR_FINGER	) strcat(buf, " finger");
    if (wear_flags & ITEM_WEAR_NECK	) strcat(buf, " neck");
    if (wear_flags & ITEM_WEAR_BODY	) strcat(buf, " torso");
    if (wear_flags & ITEM_WEAR_HEAD	) strcat(buf, " head");
    if (wear_flags & ITEM_WEAR_LEGS	) strcat(buf, " legs");
    if (wear_flags & ITEM_WEAR_FEET	) strcat(buf, " feet");
    if (wear_flags & ITEM_WEAR_HANDS	) strcat(buf, " hands");
    if (wear_flags & ITEM_WEAR_ARMS	) strcat(buf, " arms");
    if (wear_flags & ITEM_WEAR_SHIELD	) strcat(buf, " shield");
    if (wear_flags & ITEM_WEAR_ABOUT	) strcat(buf, " body");
    if (wear_flags & ITEM_WEAR_WAIST	) strcat(buf, " waist");
    if (wear_flags & ITEM_WEAR_WRIST	) strcat(buf, " wrist");
    if (wear_flags & ITEM_WIELD		) strcat(buf, " wield");
    if (wear_flags & ITEM_HOLD		) strcat(buf, " hold");
    if (wear_flags & ITEM_NO_SAC	) strcat(buf, " nosac");
    if (wear_flags & ITEM_WEAR_FLOAT	) strcat(buf, " float");
    if (wear_flags & ITEM_WEAR_BACK	) strcat(buf, " back");
    if (wear_flags & ITEM_WEAR_BOMB	) strcat(buf, " bomb");

    return ( buf[0] != '\0' ) ? buf+1 : (char*)"none";
}

char *form_bit_name(int form_flags)
{
    static char buf[512];

    buf[0] = '\0';
    if (form_flags & FORM_POISON	) strcat(buf, " poison");
    else if (form_flags & FORM_EDIBLE	) strcat(buf, " edible");
    if (form_flags & FORM_MAGICAL	) strcat(buf, " magical");
    if (form_flags & FORM_INSTANT_DECAY	) strcat(buf, " instant_rot");
    if (form_flags & FORM_OTHER		) strcat(buf, " other");
    if (form_flags & FORM_ANIMAL	) strcat(buf, " animal");
    if (form_flags & FORM_SENTIENT	) strcat(buf, " sentient");
    if (form_flags & FORM_UNDEAD	) strcat(buf, " undead");
    if (form_flags & FORM_CONSTRUCT	) strcat(buf, " construct");
    if (form_flags & FORM_MIST		) strcat(buf, " mist");
    if (form_flags & FORM_INTANGIBLE	) strcat(buf, " intangible");
    if (form_flags & FORM_BIPED		) strcat(buf, " biped");
    if (form_flags & FORM_CENTAUR	) strcat(buf, " centaur");
    if (form_flags & FORM_INSECT	) strcat(buf, " insect");
    if (form_flags & FORM_SPIDER	) strcat(buf, " spider");
    if (form_flags & FORM_CRUSTACEAN	) strcat(buf, " crustacean");
    if (form_flags & FORM_WORM		) strcat(buf, " worm");
    if (form_flags & FORM_BLOB		) strcat(buf, " blob");
    if (form_flags & FORM_MAMMAL	) strcat(buf, " mammal");
    if (form_flags & FORM_BIRD		) strcat(buf, " bird");
    if (form_flags & FORM_REPTILE	) strcat(buf, " reptile");
    if (form_flags & FORM_SNAKE		) strcat(buf, " snake");
    if (form_flags & FORM_DRAGON	) strcat(buf, " dragon");
    if (form_flags & FORM_AMPHIBIAN	) strcat(buf, " amphibian");
    if (form_flags & FORM_FISH		) strcat(buf, " fish");
    if (form_flags & FORM_COLD_BLOOD 	) strcat(buf, " cold_blooded");

    return ( buf[0] != '\0' ) ? buf+1 : (char*)"none";
}

char *part_bit_name(int part_flags)
{
    static char buf[512];

    buf[0] = '\0';
    if (part_flags & PART_HEAD		) strcat(buf, " head");
    if (part_flags & PART_ARMS		) strcat(buf, " arms");
    if (part_flags & PART_LEGS		) strcat(buf, " legs");
    if (part_flags & PART_HEART		) strcat(buf, " heart");
    if (part_flags & PART_BRAINS	) strcat(buf, " brains");
    if (part_flags & PART_GUTS		) strcat(buf, " guts");
    if (part_flags & PART_HANDS		) strcat(buf, " hands");
    if (part_flags & PART_FEET		) strcat(buf, " feet");
    if (part_flags & PART_FINGERS	) strcat(buf, " fingers");
    if (part_flags & PART_EAR		) strcat(buf, " ears");
    if (part_flags & PART_EYE		) strcat(buf, " eyes");
    if (part_flags & PART_LONG_TONGUE	) strcat(buf, " long_tongue");
    if (part_flags & PART_EYESTALKS	) strcat(buf, " eyestalks");
    if (part_flags & PART_TENTACLES	) strcat(buf, " tentacles");
    if (part_flags & PART_FINS		) strcat(buf, " fins");
    if (part_flags & PART_WINGS		) strcat(buf, " wings");
    if (part_flags & PART_TAIL		) strcat(buf, " tail");
    if (part_flags & PART_CLAWS		) strcat(buf, " claws");
    if (part_flags & PART_FANGS		) strcat(buf, " fangs");
    if (part_flags & PART_HORNS		) strcat(buf, " horns");
    if (part_flags & PART_SCALES	) strcat(buf, " scales");

    return ( buf[0] != '\0' ) ? buf+1 : (char*)"none";
}

char *weapon_bit_name(int weapon_flags)
{
    static char buf[512];

    buf[0] = '\0';
    if (weapon_flags & WEAPON_FLAMING	) strcat(buf, " flaming");
    if (weapon_flags & WEAPON_FROST	) strcat(buf, " frost");
    if (weapon_flags & WEAPON_VAMPIRIC	) strcat(buf, " vampiric");
    if (weapon_flags & WEAPON_SHARP	) strcat(buf, " sharp");
    if (weapon_flags & WEAPON_VORPAL	) strcat(buf, " vorpal");
    if (weapon_flags & WEAPON_TWO_HANDS ) strcat(buf, " two-handed");
    if (weapon_flags & WEAPON_SHOCKING 	) strcat(buf, " shocking");
    if (weapon_flags & WEAPON_POISON	) strcat(buf, " poison");

    return ( buf[0] != '\0' ) ? buf+1 : (char*)"none";
}

char *cont_bit_name( int cont_flags)
{
    static char buf[512];

    buf[0] = '\0';

    if (cont_flags & CONT_CLOSEABLE	) strcat(buf, " closable");
    if (cont_flags & CONT_PICKPROOF	) strcat(buf, " pickproof");
    if (cont_flags & CONT_CLOSED	) strcat(buf, " closed");
    if (cont_flags & CONT_LOCKED	) strcat(buf, " locked");

    return (buf[0] != '\0' ) ? buf+1 : (char*)"none";
}


char *off_bit_name(int off_flags)
{
    static char buf[512];

    buf[0] = '\0';

    if (off_flags & OFF_AREA_ATTACK	) strcat(buf, " area attack");
    if (off_flags & OFF_BACKSTAB	) strcat(buf, " backstab");
    if (off_flags & OFF_BASH		) strcat(buf, " bash");
    if (off_flags & OFF_BERSERK		) strcat(buf, " berserk");
    if (off_flags & OFF_DISARM		) strcat(buf, " disarm");
    if (off_flags & OFF_DODGE		) strcat(buf, " dodge");
    if (off_flags & OFF_FADE		) strcat(buf, " fade");
    if (off_flags & OFF_FAST		) strcat(buf, " fast");
    if (off_flags & OFF_KICK		) strcat(buf, " kick");
    if (off_flags & OFF_KICK_DIRT	) strcat(buf, " kick_dirt");
    if (off_flags & OFF_PARRY		) strcat(buf, " parry");
    if (off_flags & OFF_RESCUE		) strcat(buf, " rescue");
    if (off_flags & OFF_TAIL		) strcat(buf, " tail");
    if (off_flags & OFF_TRIP		) strcat(buf, " trip");
    if (off_flags & OFF_CRUSH		) strcat(buf, " crush");
    if (off_flags & ASSIST_ALL		) strcat(buf, " assist_all");
    if (off_flags & ASSIST_ALIGN	) strcat(buf, " assist_align");
    if (off_flags & ASSIST_RACE		) strcat(buf, " assist_race");
    if (off_flags & ASSIST_PLAYERS	) strcat(buf, " assist_players");
    if (off_flags & ASSIST_GUARD	) strcat(buf, " assist_guard");
    if (off_flags & ASSIST_VNUM		) strcat(buf, " assist_vnum");

    return ( buf[0] != '\0' ) ? buf+1 : (char*)"none";
}

/*
 * See if a string is one of the names of an object.
 */

bool is_full_name( const char *str, char *namelist )
{
    char name[MAX_INPUT_LENGTH];

    for ( ; ; )
    {
        namelist = one_argument( namelist, name );
        if ( name[0] == '\0' )
            return FALSE;
        if ( !str_cmp( str, name ) )
            return TRUE;
    }
}

int advatoi (const char *s)
{
  char string[MAX_INPUT_LENGTH]; /* a buffer to hold a copy of the argument */
  char *stringptr = string; /* a pointer to the buffer so we can move around */
  char tempstring[2];       /* a small temp buffer to pass to atoi*/
  int number = 0;           /* number to be returned */
  int multiplier = 0;       /* multiplier used to get the extra digits right */


  strcpy (string,s);        /* working copy */

  while ( isdigit (*stringptr)) /* as long as the current character is a digit */
  {
      strncpy (tempstring,stringptr,1);           /* copy first digit */
      number = (number * 10) + atoi (tempstring); /* add to current number */
      stringptr++;                                /* advance */
  }

  switch (UPPER(*stringptr)) {
      case 'K'  : multiplier = 1000;    number *= multiplier; stringptr++; break;
      case 'M'  : multiplier = 1000000; number *= multiplier; stringptr++; break;
      case '\0' : break;
      default   : return 0; /* not k nor m nor NUL - return 0! */
  }

  while ( isdigit (*stringptr) && (multiplier > 1)) /* if any digits follow k/m, add those too */
  {
      strncpy (tempstring,stringptr,1);           /* copy first digit */
      multiplier = multiplier / 10;  /* the further we get to right, the less are the digit 'worth' */
      number = number + (atoi (tempstring) * multiplier);
      stringptr++;
  }

  if (*stringptr != '\0' && !isdigit(*stringptr)) /* a non-digit character was found, other than NUL */
    return 0; /* If a digit is found, it means the multiplier is 1 - i.e. extra
                 digits that just have to be ignore, liked 14k4443 -> 3 is ignored */


  return (number);
}


int parsebet (const int currentbet, const char *argument)
{

  int newbet = 0;                /* a variable to temporarily hold the new bet */
  char string[MAX_INPUT_LENGTH]; /* a buffer to modify the bet string */
  char *stringptr = string;      /* a pointer we can move around */

  strcpy (string,argument);      /* make a work copy of argument */


  if (*stringptr)               /* check for an empty string */
  {

    if (isdigit (*stringptr)) /* first char is a digit assume e.g. 433k */
      newbet = advatoi (stringptr); /* parse and set newbet to that value */

    else
      if (*stringptr == '+') /* add ?? percent */
      {
        if (strlen (stringptr) == 1) /* only + specified, assume default */
          newbet = (currentbet * 125) / 100; /* default: add 25% */
        else
          newbet = (currentbet * (100 + atoi (++stringptr))) / 100; /* cut off the first char */
      }
      else
        {
        printf ("considering: * x \n\r");
        if ((*stringptr == '*') || (*stringptr == 'x')) /* multiply */
	  {
          if (strlen (stringptr) == 1) /* only x specified, assume default */
            newbet = currentbet * 2 ; /* default: twice */
          else /* user specified a number */
            newbet = currentbet * atoi (++stringptr); /* cut off the first char */
	  }
        }
  }

  return newbet;        /* return the calculated bet */
}


Character *get_char_area( Character *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    Character *ach;
    int number,count;

    if ( (ach = get_char_room( ch, argument )) != NULL )
        return ach;

    number = number_argument( argument, arg );
    count = 0;
    for (std::list<Character*>::iterator it = char_list.begin(); it != char_list.end(); it++)
    {
        ach = *it;
        if (ach->in_room->area != ch->in_room->area
        ||  !ch->can_see( ach ) || !is_name( arg, ach->getName().c_str() ))
            continue; 
        if (++count == number)
            return ach;
    }

    return ach;
}

bool can_be_class( Character *ch, int class_num )
{
    auto classData = class_table[class_num];
    if ( !classData.enabled ) {
        return FALSE;
    }

    return ALL_BITS( ch->done, classData.requirements );
}
