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
#include <string.h>
#include <time.h>
#include "merc.h"
#include "Wiznet.h"
#include "PlayerCharacter.h"

/* command procedures needed */
DECLARE_DO_FUN(do_backstab	);
DECLARE_DO_FUN(do_emote		);
DECLARE_DO_FUN(do_berserk	);
DECLARE_DO_FUN(do_bash		);
DECLARE_DO_FUN(do_trip		);
DECLARE_DO_FUN(do_dirt		);
DECLARE_DO_FUN(do_flee		);
DECLARE_DO_FUN(do_kick		);
DECLARE_DO_FUN(do_disarm	);
DECLARE_DO_FUN(do_get		);
DECLARE_DO_FUN(do_recall	);
DECLARE_DO_FUN(do_yell		);
DECLARE_DO_FUN(do_sacrifice	);

/*
 * Local functions.
 */
void	check_assist	args( ( Character *ch, Character *victim ) );
bool	check_dodge	args( ( Character *ch, Character *victim ) );
void	check_killer	args( ( Character *ch, Character *victim ) );
bool	check_parry	args( ( Character *ch, Character *victim ) );
bool    check_second_attack args( (Character *ch, Character *victim, int dt ) );
bool    check_shield_block     args( ( Character *ch, Character *victim ) );
bool    check_third_attack args( (Character *ch, Character *victim, int dt ) );
void    dam_message 	args( ( Character *ch, Character *victim, int dam,
                            int dt, bool immune ) );
void	death_cry	args( ( Character *ch ) );
void	group_gain	args( ( Character *ch, Character *victim ) );
int	xp_compute	args( ( Character *gch, Character *victim, 
			    int total_levels ) );
bool	is_safe		args( ( Character *ch, Character *victim ) );
void	make_corpse	args( ( Character *ch ) );
void    one_hit     args( ( Character *ch, Character *victim, int dt, bool secondary ) );
void    mob_hit		args( ( Character *ch, Character *victim, int dt ) );
void	raw_kill	args( ( Character *victim ) );
void	set_fighting	args( ( Character *ch, Character *victim ) );
void	disarm		args( ( Character *ch, Character *victim ) );



/*
 * Control the fights going on.
 * Called periodically by update_handler.
 */
void violence_update( void )
{
    Character *ch;
    Character *victim;

    for (std::list<Character*>::iterator it = char_list.begin(); it != char_list.end(); it++)
    {
    	ch = *it;

	if ( ( victim = ch->fighting ) == NULL || ch->in_room == NULL )
	    continue;

	if ( IS_AWAKE(ch) && ch->in_room == victim->in_room )
	    multi_hit( ch, victim, TYPE_UNDEFINED );
	else
	    stop_fighting( ch, FALSE );

	if ( ( victim = ch->fighting ) == NULL )
	    continue;

	/*
	 * Fun for the whole family!
	 */
	check_assist(ch,victim);

	if ( IS_NPC( ch ) )
	{
	    if ( HAS_TRIGGER( ch, TRIG_FIGHT ) )
		mp_percent_trigger( ch, victim, NULL, NULL, TRIG_FIGHT );
	    if ( HAS_TRIGGER( ch, TRIG_HPCNT ) )
		mp_hprct_trigger( ch, victim );
	}
    }

    return;
}

/* for auto assisting */
void check_assist(Character *ch,Character *victim)
{
    Character *rch, *rch_next;

    for (rch = ch->in_room->people; rch != NULL; rch = rch_next)
    {
	rch_next = rch->next_in_room;
	
	if (IS_AWAKE(rch) && rch->fighting == NULL && can_see(rch,victim))
	{

	    /* quick check for ASSIST_PLAYER */
	    if (!IS_NPC(ch) && IS_NPC(rch) 
	    && IS_SET(rch->off_flags,ASSIST_PLAYERS)
	    &&  rch->level + 6 > victim->level)
	    {
		do_emote(rch,(char*)"screams and attacks!");
		multi_hit(rch,victim,TYPE_UNDEFINED);
		continue;
	    }

	    /* PCs next */
	    if (!IS_NPC(ch) || IS_AFFECTED(ch,AFF_CHARM))
	    {
		if ( ( (!IS_NPC(rch) && IS_SET(rch->act,PLR_AUTOASSIST))
		||     IS_AFFECTED(rch,AFF_CHARM)) 
		&&   is_same_group(ch,rch) 
		&&   !is_safe(rch, victim))
		    multi_hit (rch,victim,TYPE_UNDEFINED);
		
		continue;
	    }
  	
	    /* now check the NPC cases */
	    
 	    if (IS_NPC(ch) && !IS_AFFECTED(ch,AFF_CHARM))
	
	    {
		if ( (IS_NPC(rch) && IS_SET(rch->off_flags,ASSIST_ALL))

		||   (IS_NPC(rch) && rch->group && rch->group == ch->group)

		||   (IS_NPC(rch) && rch->race == ch->race 
		   && IS_SET(rch->off_flags,ASSIST_RACE))

		||   (IS_NPC(rch) && IS_SET(rch->off_flags,ASSIST_ALIGN)
		   &&   ((IS_GOOD(rch)    && IS_GOOD(ch))
		     ||  (IS_EVIL(rch)    && IS_EVIL(ch))
		     ||  (IS_NEUTRAL(rch) && IS_NEUTRAL(ch)))) 

		||   (rch->pIndexData == ch->pIndexData 
		   && IS_SET(rch->off_flags,ASSIST_VNUM)))

	   	{
		    Character *vch;
		    Character *target;
		    int number;

		    if (number_bits(1) == 0)
			continue;
		
		    target = NULL;
		    number = 0;
		    for (vch = ch->in_room->people; vch; vch = vch->next)
		    {
			if (can_see(rch,vch)
			&&  is_same_group(vch,victim)
			&&  number_range(0,number) == 0)
			{
			    target = vch;
			    number++;
			}
		    }

		    if (target != NULL)
		    {
			do_emote(rch,(char*)"screams and attacks!");
			multi_hit(rch,target,TYPE_UNDEFINED);
		    }
		}	
	    }
	}
    }
}


/*
 * Do one group of attacks.
 */
void multi_hit( Character *ch, Character *victim, int dt ) {
    /* decrement the wait */
    if (ch->desc == NULL)
        ch->wait = UMAX(0, ch->wait - PULSE_VIOLENCE);

    if (ch->desc == NULL)
        ch->daze = UMAX(0, ch->daze - PULSE_VIOLENCE);


    /* no attacks for stunnies -- just a check */
    if (ch->position < POS_RESTING)
        return;

    if (IS_NPC(ch)) {
        mob_hit(ch, victim, dt);
        return;
    }

    one_hit(ch, victim, dt, FALSE);

    if (ch->fighting != victim)
        return;

    if (IS_AFFECTED(ch, AFF_HASTE))
        one_hit(ch, victim, dt, FALSE);

    if (ch->fighting != victim || dt == gsn_backstab)
        return;

    if (get_eq_char(ch, WEAR_SECONDARY)) {
        one_hit(ch, victim, dt, TRUE);
        if (ch->fighting != victim)
            return;
    }

    if (check_second_attack(ch, victim, dt)) {
        check_third_attack(ch, victim, dt);
    }
}

bool check_second_attack(Character *ch, Character *victim, int dt ) {
    int chance = get_skill(ch, gsn_second_attack) / 2;
    chance += 5;

    if (IS_AFFECTED(ch, AFF_SLOW))
        chance /= 2;

    if (number_percent() < chance) {
        one_hit(ch, victim, dt, FALSE);
        check_improve(ch, gsn_second_attack, TRUE, 5);
        if (ch->fighting == victim) {
            // Still fighting and had a second attack? return true
            return true;
        }
    }

    return false;
}

bool check_third_attack(Character *ch, Character *victim, int dt ) {
    int chance = get_skill(ch, gsn_third_attack) / 4;
    chance += 5;

    if (IS_AFFECTED(ch, AFF_SLOW))
        chance = 0;

    if (number_percent() < chance) {
        one_hit(ch, victim, dt, FALSE);
        check_improve(ch, gsn_third_attack, TRUE, 6);
        if (ch->fighting == victim)
            return true;
    }

    return false;
}

/* procedure for all mobile attacks */
void mob_hit (Character *ch, Character *victim, int dt)
{
    int chance,number;
    Character *vch, *vch_next;

    one_hit(ch,victim,dt, FALSE);

    if (ch->fighting != victim)
	return;

    /* Area attack -- BALLS nasty! */
 
    if (IS_SET(ch->off_flags,OFF_AREA_ATTACK))
    {
	for (vch = ch->in_room->people; vch != NULL; vch = vch_next)
	{
	    vch_next = vch->next;
	    if ((vch != victim && vch->fighting == ch))
		
one_hit(ch,vch,dt, FALSE);
	}
    }

    if (IS_AFFECTED(ch,AFF_HASTE) 
    ||  (IS_SET(ch->off_flags,OFF_FAST) && !IS_AFFECTED(ch,AFF_SLOW)))
	one_hit(ch,victim,dt, FALSE);

    if (ch->fighting != victim || dt == gsn_backstab)
	return;

    chance = get_skill(ch,gsn_second_attack)/2;

    if (IS_AFFECTED(ch,AFF_SLOW) && !IS_SET(ch->off_flags,OFF_FAST))
	chance /= 2;

    if (number_percent() < chance)
    {
	one_hit(ch,victim,dt, FALSE);
	if (ch->fighting != victim)
	    return;
    }

    chance = get_skill(ch,gsn_third_attack)/4;

    if (IS_AFFECTED(ch,AFF_SLOW) && !IS_SET(ch->off_flags,OFF_FAST))
	chance = 0;

    if (number_percent() < chance)
    {
	one_hit(ch,victim,dt, FALSE);
	if (ch->fighting != victim)
	    return;
    } 

    /* oh boy!  Fun stuff! */

    if (ch->wait > 0)
	return;

    number = number_range(0,2);

    if (number == 1 && IS_SET(ch->act,ACT_MAGE))
    {
	/*  { mob_cast_mage(ch,victim); return; } */ ;
    }

    if (number == 2 && IS_SET(ch->act,ACT_CLERIC))
    {	
	/* { mob_cast_cleric(ch,victim); return; } */ ;
    }

    /* now for the skills */

    number = number_range(0,8);

    switch(number) 
    {
    case (0) :
	if (IS_SET(ch->off_flags,OFF_BASH))
	    do_bash(ch,(char*)"");
	break;

    case (1) :
	if (IS_SET(ch->off_flags,OFF_BERSERK) && !IS_AFFECTED(ch,AFF_BERSERK))
	    do_berserk(ch,(char*)"");
	break;


    case (2) :
	if (IS_SET(ch->off_flags,OFF_DISARM) 
	|| (get_weapon_sn(ch) != gsn_hand_to_hand 
	&& (IS_SET(ch->act,ACT_WARRIOR)
   	||  IS_SET(ch->act,ACT_THIEF))))
	    do_disarm(ch,(char*)"");
	break;

    case (3) :
	if (IS_SET(ch->off_flags,OFF_KICK))
	    do_kick(ch,(char*)"");
	break;

    case (4) :
	if (IS_SET(ch->off_flags,OFF_KICK_DIRT))
	    do_dirt(ch,(char*)"");
	break;

    case (5) :
	if (IS_SET(ch->off_flags,OFF_TAIL))
	{
	    /* do_tail(ch,(char*)"") */ ;
	}
	break; 

    case (6) :
	if (IS_SET(ch->off_flags,OFF_TRIP))
	    do_trip(ch,(char*)"");
	break;

    case (7) :
	if (IS_SET(ch->off_flags,OFF_CRUSH))
	{
	    /* do_crush(ch,(char*)"") */ ;
	}
	break;
    case (8) :
	if (IS_SET(ch->off_flags,OFF_BACKSTAB))
	{
	    do_backstab(ch,(char*)"");
	}
    }
}
	

/*
 * Hit one guy once.
 */
void one_hit( Character *ch, Character *victim, int dt, bool secondary )
{
    OBJ_DATA *wield;
    int victim_ac;
    int thac0;
    int thac0_00;
    int thac0_32;
    int dam;
    int diceroll;
    int sn,skill;
    int dam_type;
    bool result;

    sn = -1;


    /* just in case */
    if (victim == ch || ch == NULL || victim == NULL)
	return;

    /*
     * Can't beat a dead char!
     * Guard against weird room-leavings.
     */
    if ( victim->position == POS_DEAD || ch->in_room != victim->in_room )
	return;

    /*
     * Figure out the type of damage message.
     */
    wield = get_eq_char( ch, WEAR_WIELD );

    if ( dt == TYPE_UNDEFINED )
    {
	dt = TYPE_HIT;
	if ( wield != NULL && (wield->item_type == ITEM_WEAPON || wield->item_type == ITEM_BOW))
	    dt += wield->value[3];
	else
	    dt += ch->dam_type;
    }

    if (dt < TYPE_HIT)
    {
    	if (wield != NULL)
    	    dam_type = attack_table[wield->value[3]].damage;
    	else
    	    dam_type = attack_table[ch->dam_type].damage;
    }
    else
    	dam_type = attack_table[dt - TYPE_HIT].damage;

    if (dam_type == -1)
	dam_type = DAM_BASH;

    /* get the weapon skill */
    sn = get_weapon_sn(ch);
    skill = 20 + get_weapon_skill(ch,sn);

    /*
     * Calculate to-hit-armor-class-0 versus armor.
     */
    if ( IS_NPC(ch) )
    {
	thac0_00 = 20;
	thac0_32 = -4;   /* as good as a thief */ 
	if (IS_SET(ch->act,ACT_WARRIOR))
	    thac0_32 = -10;
	else if (IS_SET(ch->act,ACT_THIEF))
	    thac0_32 = -4;
	else if (IS_SET(ch->act,ACT_CLERIC))
	    thac0_32 = 2;
	else if (IS_SET(ch->act,ACT_MAGE))
	    thac0_32 = 6;
    }
    else
    {
	thac0_00 = class_table[ch->class_num].thac0_00;
	thac0_32 = class_table[ch->class_num].thac0_32;
    }
    thac0  = interpolate( ch->level, thac0_00, thac0_32 );

    if (thac0 < 0)
        thac0 = thac0/2;

    if (thac0 < -5)
        thac0 = -5 + (thac0 + 5) / 2;

    thac0 -= GET_HITROLL(ch) * skill/100;
    thac0 += 5 * (100 - skill) / 100;

    if (dt == gsn_backstab)
	thac0 -= 10 * (100 - get_skill(ch,gsn_backstab));

    switch(dam_type)
    {
	case(DAM_PIERCE):victim_ac = GET_AC(victim,AC_PIERCE)/10;	break;
	case(DAM_BASH):	 victim_ac = GET_AC(victim,AC_BASH)/10;		break;
	case(DAM_SLASH): victim_ac = GET_AC(victim,AC_SLASH)/10;	break;
	default:	 victim_ac = GET_AC(victim,AC_EXOTIC)/10;	break;
    }; 
	
    if (victim_ac < -15)
	victim_ac = (victim_ac + 15) / 5 - 15;
     
    if ( !can_see( ch, victim ) )
	victim_ac -= 4;

    if ( victim->position < POS_FIGHTING)
	victim_ac += 4;
 
    if (victim->position < POS_RESTING)
	victim_ac += 6;

    /*
     * The moment of excitement!
     */
    while ( ( diceroll = number_bits( 5 ) ) >= 20 )
	;

    if ( diceroll == 0
    || ( diceroll != 19 && diceroll < thac0 - victim_ac ) )
    {
	/* Miss. */
	damage( ch, victim, 0, dt, dam_type, TRUE );
	tail_chain( );
	return;
    }

    /*
     * Hit.
     * Calc damage.
     */
    if ( IS_NPC(ch) && (!ch->pIndexData->new_format || wield == NULL))
    {
	if (!ch->pIndexData->new_format)
	{
	    dam = number_range( ch->level / 2, ch->level * 3 / 2 );
	    if ( wield != NULL )
	    	dam += dam / 2;
	}
	else
	    dam = dice(ch->damage[DICE_NUMBER],ch->damage[DICE_TYPE]);
    }
    else
    {
	if (sn != -1)
	    check_improve(ch,sn,TRUE,5);
	if ( wield != NULL )
	{
	    if (wield->pIndexData->new_format)
		dam = dice(wield->value[1],wield->value[2]) * skill/100;
	    else
	    	dam = number_range( wield->value[1] * skill/100, 
				wield->value[2] * skill/100);

	    if (get_eq_char(ch,WEAR_SHIELD) == NULL)  /* no shield = more */
		dam = dam * 11/10;

	    /* weight counts */
	    dam += wield->weight / 10;

	    /* sharpness! */
	    if (IS_WEAPON_STAT(wield,WEAPON_SHARP))
	    {
		int percent;

		if ((percent = number_percent()) <= (skill / 8))
		    dam = 2 * dam + (dam * 2 * percent / 100);
	    }
	}
	else
	    dam = number_range( 1 + 4 * skill/100, 2 * ch->level/3 * skill/100);
    }

    /*
     * Bonuses.
     */
    if ( get_skill(ch,gsn_enhanced_damage) > 0 )
    {
        diceroll = number_percent();
        if (diceroll <= get_skill(ch,gsn_enhanced_damage))
        {
            check_improve(ch,gsn_enhanced_damage,TRUE,6);
            dam += 2 * ( dam * diceroll/300);
        }
    }

    if ( !IS_AWAKE(victim) )
	dam *= 2;
     else if (victim->position < POS_FIGHTING)
	dam = dam * 3 / 2;

    if ( dt == gsn_backstab && wield != NULL) 
    {
    	if ( wield->value[0] != 2 )
	    dam *= 2 + (ch->level / 10); 
	else 
	    dam *= 2 + (ch->level / 8);
    }

    dam += GET_DAMROLL(ch) * UMIN(100,skill) /100;

    if ( dam <= 0 )
	dam = 1;

    result = damage( ch, victim, dam, dt, dam_type, TRUE );
    
    /* but do we have a funky weapon? */
    if (result && wield != NULL)
    { 
	int dam;

	if (ch->fighting == victim && IS_WEAPON_STAT(wield,WEAPON_POISON))
	{
	    int level;
	    AFFECT_DATA *poison, af;

	    if ((poison = affect_find(wield->affected,gsn_poison)) == NULL)
		level = wield->level;
	    else
		level = poison->level;
	
	    if (!saves_spell(level / 2,victim,DAM_POISON)) 
	    {
		send_to_char("You feel poison coursing through your veins.",
		    victim);
		act("$n is poisoned by the venom on $p.",
		    victim,wield,NULL,TO_ROOM);

    		af.where     = TO_AFFECTS;
    		af.type      = gsn_poison;
    		af.level     = level * 3/4;
    		af.duration  = level / 2;
    		af.location  = APPLY_STR;
    		af.modifier  = -1;
    		af.bitvector = AFF_POISON;
    		affect_join( victim, &af );
	    }

	    /* weaken the poison if it's temporary */
	    if (poison != NULL)
	    {
	    	poison->level = UMAX(0,poison->level - 2);
	    	poison->duration = UMAX(0,poison->duration - 1);
	
	    	if (poison->level == 0 || poison->duration == 0)
		    act("The poison on $p has worn off.",ch,wield,NULL,TO_CHAR);
	    }
 	}


    	if (ch->fighting == victim && IS_WEAPON_STAT(wield,WEAPON_VAMPIRIC))
	{
	    dam = number_range(1, wield->level / 5 + 1);
	    act("$p draws life from $n.",victim,wield,NULL,TO_ROOM);
	    act("You feel $p drawing your life away.",
		victim,wield,NULL,TO_CHAR);
	    damage_old(ch,victim,dam,0,DAM_NEGATIVE,FALSE);
	    ch->alignment = UMAX(-1000,ch->alignment - 1);
	    ch->hit += dam/2;
	}

	if (ch->fighting == victim && IS_WEAPON_STAT(wield,WEAPON_FLAMING))
	{
	    dam = number_range(1,wield->level / 4 + 1);
	    act("$n is burned by $p.",victim,wield,NULL,TO_ROOM);
	    act("$p sears your flesh.",victim,wield,NULL,TO_CHAR);
	    fire_effect( (void *) victim,wield->level/2,dam,TARGET_CHAR);
	    damage(ch,victim,dam,0,DAM_FIRE,FALSE);
	}

	if (ch->fighting == victim && IS_WEAPON_STAT(wield,WEAPON_FROST))
	{
	    dam = number_range(1,wield->level / 6 + 2);
	    act("$p freezes $n.",victim,wield,NULL,TO_ROOM);
	    act("The cold touch of $p surrounds you with ice.",
		victim,wield,NULL,TO_CHAR);
	    cold_effect(victim,wield->level/2,dam,TARGET_CHAR);
	    damage(ch,victim,dam,0,DAM_COLD,FALSE);
	}

	if (ch->fighting == victim && IS_WEAPON_STAT(wield,WEAPON_SHOCKING))
	{
	    dam = number_range(1,wield->level/5 + 2);
	    act("$n is struck by lightning from $p.",victim,wield,NULL,TO_ROOM);
	    act("You are shocked by $p.",victim,wield,NULL,TO_CHAR);
	    shock_effect(victim,wield->level/2,dam,TARGET_CHAR);
	    damage(ch,victim,dam,0,DAM_LIGHTNING,FALSE);
	}
    }
    tail_chain( );
    return;
}


void perform_autoloot(Character *ch) {
	OBJ_DATA *corpse = NULL;

    if (IS_NPC(ch) && IS_SET(ch->act,ACT_PET) && ch->master != NULL) {
        // The master should be doing this
        perform_autoloot(ch->master);
    } else if (!IS_NPC(ch)
        &&  (corpse = get_obj_list(ch,(char*)"corpse",ch->in_room->contents)) != NULL
        &&  corpse->item_type == ITEM_CORPSE_NPC && can_see_obj(ch,corpse))
    {
        OBJ_DATA *coins;

        corpse = get_obj_list( ch, (char*)"corpse", ch->in_room->contents );

        if ( IS_SET(ch->act, PLR_AUTOLOOT) &&
             corpse && corpse->contains) /* exists and not empty */
            do_get( ch, (char*)"all corpse" );

        if (IS_SET(ch->act,PLR_AUTOGOLD) &&
            corpse && corpse->contains  && /* exists and not empty */
            !IS_SET(ch->act,PLR_AUTOLOOT))
            if ((coins = get_obj_list(ch,(char*)"gcash",corpse->contains))
                != NULL)
                do_get(ch, (char*)"all.gcash corpse");

        if ( IS_SET(ch->act, PLR_AUTOSAC) )
        {
            if ( IS_SET(ch->act,PLR_AUTOLOOT) && corpse && corpse->contains) {
                return;  /* leave if corpse has treasure */
            } else {
                do_sacrifice(ch, (char *) "corpse");
            }
        }
    }
}


/*
 * Inflict damage from a hit.
 */
bool damage(Character *ch,Character *victim,int dam,int dt,int dam_type,
	    bool show ) 
{
    bool immune;

    if ( victim->position == POS_DEAD )
	return FALSE;

    /*
     * Stop up any residual loopholes.
     */
    if ( dam > 1200 && dt >= TYPE_HIT)
    {
	bug( "Damage: %d: more than 1200 points!", dam );
	dam = 1200;
	if (!IS_IMMORTAL(ch))
	{
	    OBJ_DATA *obj;
	    obj = get_eq_char( ch, WEAR_WIELD );
	    send_to_char("You really shouldn't cheat.\n\r",ch);
	    if (obj != NULL)
	    	extract_obj(obj);
	}

    }
    
    if ( victim != ch )
    {
	/*
	 * Certain attacks are forbidden.
	 * Most other attacks are returned.
	 */
	if ( is_safe( ch, victim ) )
	    return FALSE;
	check_killer( ch, victim );

	if ( victim->position > POS_STUNNED )
	{
	    if ( victim->fighting == NULL )
	    {
		/*
		 * Ranged combat is icky with swords!
		 */
		if (dt != gsn_bow )
		    set_fighting( victim, ch );
		if ( IS_NPC( victim ) && HAS_TRIGGER( victim, TRIG_KILL ) )
		    mp_percent_trigger( victim, ch, NULL, NULL, TRIG_KILL );
	    }
	    if (victim->timer <= 4 && dt != gsn_bow)
	    	victim->position = POS_FIGHTING;
	}

	if ( victim->position > POS_STUNNED )
	{
	    if ( ch->fighting == NULL && dt != gsn_bow)
		set_fighting( ch, victim );

	    /*
	     * If victim is charmed, ch might attack victim's master.
	     taken out by Russ! */
/*
	    if ( IS_NPC(ch)
	    &&   IS_NPC(victim)
	    &&   IS_AFFECTED(victim, AFF_CHARM)
	    &&   victim->master != NULL
	    &&   victim->master->in_room == ch->in_room
	    &&   number_bits( 3 ) == 0
	    &&	 dt != gsn_bow )
	    {
		stop_fighting( ch, FALSE );
		multi_hit( ch, victim->master, TYPE_UNDEFINED );
		return FALSE;
	    }
*/
	}

	/*
	 * More charm stuff.
	 */
	if ( victim->master == ch )
	    stop_follower( victim );
    }

    /*
     * Inviso attacks ... not.
     */
    if ( IS_AFFECTED(ch, AFF_INVISIBLE) )
    {
	affect_strip( ch, gsn_invis );
	affect_strip( ch, gsn_mass_invis );
	REMOVE_BIT( ch->affected_by, AFF_INVISIBLE );
	act( "$n fades into existence.", ch, NULL, NULL, TO_ROOM );
    }

    /*
     * Damage modifiers.
     */

    if ( dam > 1 && !IS_NPC(victim) 
    &&   victim->pcdata->condition[COND_DRUNK]  > 10 )
	dam = 9 * dam / 10;

    if ( dam > 1 && IS_AFFECTED(victim, AFF_SANCTUARY) )
	dam /= 2;

    if ( dam > 1 && ((IS_AFFECTED(victim, AFF_PROTECT_EVIL) && IS_EVIL(ch) )
    ||		     (IS_AFFECTED(victim, AFF_PROTECT_GOOD) && IS_GOOD(ch) )))
	dam -= dam / 4;

    immune = FALSE;


    /*
     * Check for parry, and dodge.
     */
    if ( dt >= TYPE_HIT && ch != victim)
    {
        if ( check_parry( ch, victim ) )
	    return FALSE;
	if ( check_dodge( ch, victim ) )
	    return FALSE;
	if ( check_shield_block(ch,victim))
	    return FALSE;

    }

    switch(check_immune(victim,dam_type))
    {
	case(IS_IMMUNE):
	    immune = TRUE;
	    dam = 0;
	    break;
	case(IS_RESISTANT):	
	    dam -= dam/3;
	    break;
	case(IS_VULNERABLE):
	    dam += dam/2;
	    break;
    }

    if (show)
    	dam_message( ch, victim, dam, dt, immune );

    if (dam == 0)
	return FALSE;

    /*
     * Hurt the victim.
     * Inform the victim of his new state.
     */
    victim->hit -= dam;
    if ( !IS_NPC(victim)
    &&   victim->level >= LEVEL_IMMORTAL
    &&   victim->hit < 1 )
	victim->hit = 1;
    update_pos( victim );

    switch( victim->position )
    {
    case POS_MORTAL:
	act( "$n is mortally wounded, and will die soon, if not aided.",
	    victim, NULL, NULL, TO_ROOM );
	send_to_char( 
	    "You are mortally wounded, and will die soon, if not aided.\n\r",
	    victim );
	break;

    case POS_INCAP:
	act( "$n is incapacitated and will slowly die, if not aided.",
	    victim, NULL, NULL, TO_ROOM );
	send_to_char(
	    "You are incapacitated and will slowly die, if not aided.\n\r",
	    victim );
	break;

    case POS_STUNNED:
	act( "$n is stunned, but will probably recover.",
	    victim, NULL, NULL, TO_ROOM );
	send_to_char("You are stunned, but will probably recover.\n\r",
	    victim );
	break;

    case POS_DEAD:
	act( "$n is DEAD!!", victim, 0, 0, TO_ROOM );
	send_to_char( "You have been KILLED!!\n\r\n\r", victim );
	break;

    default:
	if ( dam > victim->max_hit / 4 )
	    send_to_char( "That really did HURT!\n\r", victim );
	if ( victim->hit < victim->max_hit / 4 )
	    send_to_char( "You sure are BLEEDING!\n\r", victim );
	break;
    }

    /*
     * Sleep spells and extremely wounded folks.
     */
    if ( !IS_AWAKE(victim) )
	stop_fighting( victim, FALSE );

    /*
     * Payoff for killing things.
     */
    if ( victim->position == POS_DEAD )
    {
	group_gain( ch, victim );

	if ( !IS_NPC(victim) )
	{
	    snprintf(log_buf, 2*MAX_INPUT_LENGTH, "%s killed by %s at %d",
		victim->getName(),
		(IS_NPC(ch) ? ch->short_descr : ch->getName()),
		ch->in_room->vnum );
	    log_string( log_buf );

	    /*
	     * Dying penalty:
	     * 2/3 way back to previous level.
	     */
	    if ( victim->exp > exp_per_level(victim,victim->pcdata->points) 
			       * victim->level )
			victim->gain_exp( (2 * (exp_per_level(victim,victim->pcdata->points)
			         * victim->level - victim->exp)/3) + 50 );
	}

        snprintf(log_buf, 2*MAX_INPUT_LENGTH, "%s got toasted by %s at %s [room %d]",
            (IS_NPC(victim) ? victim->short_descr : victim->getName()),
            (IS_NPC(ch) ? ch->short_descr : ch->getName()),
            ch->in_room->name, ch->in_room->vnum);
 
        if (IS_NPC(victim))
	{
	    if (!IS_NPC(ch))
			((PlayerCharacter*)ch)->incrementMobKills(1);

            Wiznet::instance()->report(log_buf,NULL,NULL,WIZ_MOBDEATHS,0,0);
	}
        else
	{
	    if (!IS_NPC(ch))
	    {
			((PlayerCharacter*)ch)->incrementPlayerKills(1);
			((PlayerCharacter*)ch)->incrementRange(1);
		if (IS_CLANNED(ch))
		{
		    ch->pcdata->clan->pkills++;
		    save_clan(ch->pcdata->clan);
		}
			((PlayerCharacter*)victim)->incrementKilledByPlayerCount(1);
			((PlayerCharacter*)victim)->setJustKilled(5);
			((PlayerCharacter*)victim)->incrementRange(-1);
		if (IS_CLANNED(victim))
		{
		    victim->pcdata->clan->pdeaths++;
		    save_clan(victim->pcdata->clan);
		}
	    }
	    else
			((PlayerCharacter*)ch)->incrementKilledByMobCount(1);
			((PlayerCharacter*)ch)->setJustKilled(5);

            Wiznet::instance()->report(log_buf,NULL,NULL,WIZ_DEATHS,0,0);
	}

	/*
	 * Death trigger
	 */
	if ( IS_NPC( victim ) && HAS_TRIGGER( victim, TRIG_DEATH) )
	{
	    victim->position = POS_STANDING;
	    mp_percent_trigger( victim, ch, NULL, NULL, TRIG_DEATH );
	}

	raw_kill( victim );
        /* dump the flags */
        if (ch != victim && !IS_NPC(ch) && !is_same_clan(ch,victim))
        {
            if (IS_SET(victim->act,PLR_THIEF))
                REMOVE_BIT(victim->act,PLR_THIEF);
        }

        /* RT new auto commands */
        perform_autoloot(ch);

	return TRUE;
    }

    if ( victim == ch )
	return TRUE;

    /*
     * Take care of link dead people.
     */
    if ( !IS_NPC(victim) && victim->desc == NULL )
    {
	if ( number_range( 0, victim->wait ) == 0 )
	{
	    do_recall( victim, (char*)"" );
	    return TRUE;
	}
    }

    /*
     * Wimp out?
     */
    if ( IS_NPC(victim) && dam > 0 && victim->wait < PULSE_VIOLENCE / 2)
    {
	if ( ( IS_SET(victim->act, ACT_WIMPY) && number_bits( 2 ) == 0
	&&   victim->hit < victim->max_hit / 5) 
	||   ( IS_AFFECTED(victim, AFF_CHARM) && victim->master != NULL
	&&     victim->master->in_room != victim->in_room ) )
	    do_flee( victim, (char*)"" );
    }

    if ( !IS_NPC(victim)
    &&   victim->hit > 0
    &&   victim->hit <= victim->wimpy
    &&   victim->wait < PULSE_VIOLENCE / 2 )
	do_flee( victim, (char*)"" );

    tail_chain( );
    return TRUE;
}






/*
 * Inflict damage from a hit.
 */
bool damage_old( Character *ch, Character *victim, int dam, int dt, int 
dam_type, bool show ) {
    bool immune;

    if ( victim->position == POS_DEAD )
	return FALSE;

    /*
     * Stop up any residual loopholes.
     */

    if ( dam > 1200 && dt >= TYPE_HIT)
    {
        bug( "Damage: %d: more than 1200 points!", dam );
        dam = 1200;
        if (!IS_IMMORTAL(ch))
        {
            OBJ_DATA *obj;
            obj = get_eq_char( ch, WEAR_WIELD );
            send_to_char("You really shouldn't cheat.\n\r",ch);
            if (obj != NULL)
                extract_obj(obj);
        }
 
    }

    
    /* damage reduction */
    if ( dam > 35)
	dam = (dam - 35)/2 + 35;
    if ( dam > 80)
	dam = (dam - 80)/2 + 80; 



   
    if ( victim != ch )
    {
	/*
	 * Certain attacks are forbidden.
	 * Most other attacks are returned.
	 */
	if ( is_safe( ch, victim ) )
	    return FALSE;
	check_killer( ch, victim );

	if ( victim->position > POS_STUNNED )
	{
	    if ( victim->fighting == NULL || dt != gsn_bow)
		set_fighting( victim, ch );
	    if (victim->timer <= 4 && dt != gsn_bow)
	    	victim->position = POS_FIGHTING;
	}

	if ( victim->position > POS_STUNNED )
	{
	    if ( ch->fighting == NULL || dt != gsn_bow)
		set_fighting( ch, victim );

	    /*
	     * If victim is charmed, ch might attack victim's master.
	     */
	    if ( IS_NPC(ch)
	    &&   IS_NPC(victim)
	    &&   IS_AFFECTED(victim, AFF_CHARM)
	    &&   victim->master != NULL
	    &&   victim->master->in_room == ch->in_room
	    &&   number_bits( 3 ) == 0 )
	    {
		stop_fighting( ch, FALSE );
		multi_hit( ch, victim->master, TYPE_UNDEFINED );
		return FALSE;
	    }
	}

	/*
	 * More charm stuff.
	 */
	if ( victim->master == ch )
	    stop_follower( victim );
    }

    /*
     * Inviso attacks ... not.
     */
    if ( IS_AFFECTED(ch, AFF_INVISIBLE) )
    {
	affect_strip( ch, gsn_invis );
	affect_strip( ch, gsn_mass_invis );
	REMOVE_BIT( ch->affected_by, AFF_INVISIBLE );
	act( "$n fades into existence.", ch, NULL, NULL, TO_ROOM );
    }

    /*
     * Damage modifiers.
     */

    if ( dam > 1 && !IS_NPC(victim)
    &&   victim->pcdata->condition[COND_DRUNK]  > 10 )
        dam = 9 * dam / 10;
 
    if ( dam > 1 && IS_AFFECTED(victim, AFF_SANCTUARY) )
        dam /= 2;
 
    if ( dam > 1 && ((IS_AFFECTED(victim, AFF_PROTECT_EVIL) && IS_EVIL(ch) )
    ||               (IS_AFFECTED(victim, AFF_PROTECT_GOOD) && IS_GOOD(ch) )))
        dam -= dam / 4;

    immune = FALSE;


    /*
     * Check for parry, and dodge.
     */
    if ( dt >= TYPE_HIT && ch != victim)
    {
        if ( check_parry( ch, victim ) )
	    return FALSE;
	if ( check_dodge( ch, victim ) )
	    return FALSE;
	if ( check_shield_block(ch,victim))
	    return FALSE;

    }

    switch(check_immune(victim,dam_type))
    {
	case(IS_IMMUNE):
	    immune = TRUE;
	    dam = 0;
	    break;
	case(IS_RESISTANT):	
	    dam -= dam/3;
	    break;
	case(IS_VULNERABLE):
	    dam += dam/2;
	    break;
    }

    if (show)
    	dam_message( ch, victim, dam, dt, immune );

    if (dam == 0)
	return FALSE;

    /*
     * Hurt the victim.
     * Inform the victim of his new state.
     */
    victim->hit -= dam;
    if ( !IS_NPC(victim)
    &&   victim->level >= LEVEL_IMMORTAL
    &&   victim->hit < 1 )
	victim->hit = 1;
    update_pos( victim );

    switch( victim->position )
    {
    case POS_MORTAL:
	act( "$n is mortally wounded, and will die soon, if not aided.",
	    victim, NULL, NULL, TO_ROOM );
	send_to_char( 
	    "You are mortally wounded, and will die soon, if not aided.\n\r",
	    victim );
	break;

    case POS_INCAP:
	act( "$n is incapacitated and will slowly die, if not aided.",
	    victim, NULL, NULL, TO_ROOM );
	send_to_char(
	    "You are incapacitated and will slowly die, if not aided.\n\r",
	    victim );
	break;

    case POS_STUNNED:
	act( "$n is stunned, but will probably recover.",
	    victim, NULL, NULL, TO_ROOM );
	send_to_char("You are stunned, but will probably recover.\n\r",
	    victim );
	break;

    case POS_DEAD:
	act( "$n is DEAD!!", victim, 0, 0, TO_ROOM );
	send_to_char( "You have been KILLED!!\n\r\n\r", victim );
	break;

    default:
	if ( dam > victim->max_hit / 4 )
	    send_to_char( "That really did HURT!\n\r", victim );
	if ( victim->hit < victim->max_hit / 4 )
	    send_to_char( "You sure are BLEEDING!\n\r", victim );
	break;
    }

    /*
     * Sleep spells and extremely wounded folks.
     */
    if ( !IS_AWAKE(victim) )
	stop_fighting( victim, FALSE );

    /*
     * Payoff for killing things.
     */
    if ( victim->position == POS_DEAD )
    {
	group_gain( ch, victim );

	if ( !IS_NPC(victim) )
	{
	    snprintf(log_buf, 2*MAX_INPUT_LENGTH, "%s killed by %s at %d",
		victim->getName(),
		(IS_NPC(ch) ? ch->short_descr : ch->getName()),
		victim->in_room->vnum );
	    log_string( log_buf );

	    /*
	     * Dying penalty:
	     * 2/3 way back to previous level.
	     */
	    if ( victim->exp > exp_per_level(victim,victim->pcdata->points) 
			       * victim->level )
			victim->gain_exp( (2 * (exp_per_level(victim,victim->pcdata->points)
			         * victim->level - victim->exp)/3) + 50 );
	}

        snprintf(log_buf, 2*MAX_INPUT_LENGTH, "%s got toasted by %s at %s [room %d]",
            (IS_NPC(victim) ? victim->short_descr : victim->getName()),
            (IS_NPC(ch) ? ch->short_descr : ch->getName()),
            ch->in_room->name, ch->in_room->vnum);
 
        if (IS_NPC(victim))
            Wiznet::instance()->report(log_buf,NULL,NULL,WIZ_MOBDEATHS,0,0);
        else
            Wiznet::instance()->report(log_buf,NULL,NULL,WIZ_DEATHS,0,0);

        raw_kill( victim );
        /* dump the flags */
        if (ch != victim && !IS_NPC(ch) && !is_same_clan(ch,victim))
        {
            if (IS_SET(victim->act,PLR_THIEF))
            REMOVE_BIT(victim->act,PLR_THIEF);
        }

        /* RT new auto commands */
        perform_autoloot(ch);

        return TRUE;
    }

    if ( victim == ch )
	return TRUE;

    /*
     * Take care of link dead people.
     */
    if ( !IS_NPC(victim) && victim->desc == NULL )
    {
	if ( number_range( 0, victim->wait ) == 0 )
	{
	    do_recall( victim, (char*)"" );
	    return TRUE;
	}
    }

    /*
     * Wimp out?
     */
    if ( IS_NPC(victim) && dam > 0 && victim->wait < PULSE_VIOLENCE / 2)
    {
	if ( ( IS_SET(victim->act, ACT_WIMPY) && number_bits( 2 ) == 0
	&&   victim->hit < victim->max_hit / 5) 
	||   ( IS_AFFECTED(victim, AFF_CHARM) && victim->master != NULL
	&&     victim->master->in_room != victim->in_room ) )
	    do_flee( victim, (char*)"" );
    }

    if ( !IS_NPC(victim)
    &&   victim->hit > 0
    &&   victim->hit <= victim->wimpy
    &&   victim->wait < PULSE_VIOLENCE / 2 )
	do_flee( victim, (char*)"" );

    tail_chain( );
    return TRUE;
}

bool is_safe(Character *ch, Character *victim)
{
    if (victim->in_room == NULL || ch->in_room == NULL)
	return TRUE;

    if (victim->fighting == ch || victim == ch)
	return FALSE;

    if (IS_IMMORTAL(ch) && ch->level > LEVEL_IMMORTAL)
	return FALSE;

	if (ch->didJustDie() || victim->didJustDie() )
	{
	    send_to_char("Players may not fight for 5 minutes after they die.\n\r",ch);
	    return TRUE;
	}

    /* killing mobiles */
    if (IS_NPC(victim))
    {

	/* safe room? */
	if (IS_SET(victim->in_room->room_flags,ROOM_SAFE))
	{
	    send_to_char("Not in this room.\n\r",ch);
	    return TRUE;
	}

	if (victim->pIndexData->pShop != NULL)
	{
	    send_to_char("The shopkeeper wouldn't like that.\n\r",ch);
	    return TRUE;
	}

	/* no killing healers, trainers, etc */
	if (IS_SET(victim->act,ACT_TRAIN)
	||  IS_SET(victim->act,ACT_PRACTICE)
	||  IS_SET(victim->act,ACT_IS_HEALER)
	||  IS_SET(victim->act,ACT_IS_CHANGER))
	{
	    send_to_char("I don't think Mota would approve.\n\r",ch);
	    return TRUE;
	}

	if (!IS_NPC(ch))
	{
	    /* no pets */
	    if (IS_SET(victim->act,ACT_PET))
	    {
		act("But $N looks so cute and cuddly...",
		    ch,NULL,victim,TO_CHAR);
		return TRUE;
	    }

	    /* no charmed creatures unless owner */
	    if (IS_AFFECTED(victim,AFF_CHARM) && ch != victim->master)
	    {
		send_to_char("You don't own that monster.\n\r",ch);
		return TRUE;
	    }
	}
    }
    /* killing players */
    else
    {
	/* NPC doing the killing */
	if (IS_NPC(ch))
	{
	    /* safe room check */
	    if (IS_SET(victim->in_room->room_flags,ROOM_SAFE))
	    {
		send_to_char("Not in this room.\n\r",ch);
		return TRUE;
	    }

	    /* charmed mobs and pets cannot attack players while owned */
	    if (IS_AFFECTED(ch,AFF_CHARM) && ch->master != NULL
	    &&  ch->master->fighting != victim)
	    {
		send_to_char("Players are your friends!\n\r",ch);
		return TRUE;
	    }
	}
	/* player doing the killing */
	else
	{
	    if (!IS_CLANNED(ch))
	    {
		send_to_char("Join a clan if you want to kill players.\n\r",ch);
		return TRUE;
	    }

	    if (!IS_CLANNED(victim))
	    {
		send_to_char("They aren't in a clan, leave them alone.\n\r",ch);
		return TRUE;
	    }

	    if (IS_CLANNED(ch) && !IS_SET(ch->pcdata->clan->flags, CLAN_PK))
	    {
		send_to_char("You must be in a pkilling clan to kill other players.\n\r",ch);
		return TRUE;
	    }

	    if (IS_CLANNED(victim) && !IS_SET(victim->pcdata->clan->flags, CLAN_PK))
	    {
		send_to_char("They are in a non pkilling clan.  Leave them alone.\n\r",ch);
		return TRUE;
	    }

	    if (IS_SET( ch->in_room->area->area_flags, AREA_NO_PK ))
	    {
		send_to_char("There is to be no pkilling in this area.\n\r",ch);
		return TRUE;
	    }

	    if (IS_SET(victim->act,PLR_THIEF))
		return FALSE;

	    if (ch->level > victim->level + victim->getRange() )
	    {
		send_to_char("Pick on someone your own size.\n\r",ch);
		return TRUE;
	    }
	}
    }
    return FALSE;
}
 
bool is_safe_spell(Character *ch, Character *victim, bool area )
{
    if (victim->in_room == NULL || ch->in_room == NULL)
        return TRUE;

    if (victim == ch && area)
	return TRUE;

    if (victim->fighting == ch || victim == ch)
	return FALSE;

    if (IS_IMMORTAL(ch) && ch->level > LEVEL_IMMORTAL && !area)
	return FALSE;

	if ( ch->didJustDie() || victim->didJustDie() )
	{
	    send_to_char("Players may not fight for 5 minutes after they die.\n\r",ch);
	    return TRUE;
	}

    /* killing mobiles */
    if (IS_NPC(victim))
    {
	/* safe room? */
	if (IS_SET(victim->in_room->room_flags,ROOM_SAFE))
	    return TRUE;

	if (victim->pIndexData->pShop != NULL)
	    return TRUE;

	/* no killing healers, trainers, etc */
	if (IS_SET(victim->act,ACT_TRAIN)
	||  IS_SET(victim->act,ACT_PRACTICE)
	||  IS_SET(victim->act,ACT_IS_HEALER)
	||  IS_SET(victim->act,ACT_IS_CHANGER))
	    return TRUE;

	if (!IS_NPC(ch))
	{
	    /* no pets */
	    if (IS_SET(victim->act,ACT_PET))
	   	return TRUE;

	    /* no charmed creatures unless owner */
	    if (IS_AFFECTED(victim,AFF_CHARM) && (area || ch != victim->master))
		return TRUE;

	    /* legal kill? -- cannot hit mob fighting non-group member */
	    if (victim->fighting != NULL && !is_same_group(ch,victim->fighting))
		return TRUE;
	}
	else
	{
	    /* area effect spells do not hit other mobs */
	    if (area && !is_same_group(victim,ch->fighting))
		return TRUE;
	}
    }
    /* killing players */
    else
    {
	if (area && IS_IMMORTAL(victim) && victim->level > LEVEL_IMMORTAL)
	    return TRUE;

	/* NPC doing the killing */
	if (IS_NPC(ch))
	{
	    /* charmed mobs and pets cannot attack players while owned */
	    if (IS_AFFECTED(ch,AFF_CHARM) && ch->master != NULL
	    &&  ch->master->fighting != victim)
		return TRUE;
	
	    /* safe room? */
	    if (IS_SET(victim->in_room->room_flags,ROOM_SAFE))
		return TRUE;
 
	    /* legal kill? -- mobs only hit players grouped with opponent*/
	    if (ch->fighting != NULL && !is_same_group(ch->fighting,victim))
		return TRUE;
	}

	/* player doing the killing */
	else
	{
	    if (!IS_CLANNED(ch))
		return TRUE;

	    if (!IS_CLANNED(victim))
		return TRUE;

	    if (IS_SET( ch->in_room->area->area_flags, AREA_NO_PK ))
		return TRUE;

	    if (IS_SET(victim->act,PLR_THIEF))
		return FALSE;

	    if (ch->level > victim->level + victim->getRange())
		return TRUE;
	}

    }
    return FALSE;
}
/*
 * See if an attack justifies a KILLER flag.
 */
void check_killer( Character *ch, Character *victim )
{
    char buf[MAX_STRING_LENGTH];
    /*
     * Follow charm thread to responsible character.
     * Attacking someone's charmed char is hostile!
     */
    while ( IS_AFFECTED(victim, AFF_CHARM) && victim->master != NULL )
	victim = victim->master;

    /*
     * NPC's are fair game.
     * So are killers and thieves.
     */
    if ( IS_NPC(victim) )
	return;

    if (!IS_NPC(victim) && !IS_NPC(ch))
    {
		((PlayerCharacter*)ch)->setAdrenaline(5);
		((PlayerCharacter*)victim)->setAdrenaline(5);
    }

    if (IS_SET(victim->act, PLR_THIEF))
	return;

    /*
     * Charm-o-rama.
     */
    if ( IS_SET(ch->affected_by, AFF_CHARM) )
    {
	if ( ch->master == NULL )
	{
	    char buf[MAX_STRING_LENGTH];

	    snprintf(buf, sizeof(buf), "Check_killer: %s bad AFF_CHARM",
		IS_NPC(ch) ? ch->short_descr : ch->getName() );
	    bug( buf, 0 );
	    affect_strip( ch, gsn_charm_person );
	    REMOVE_BIT( ch->affected_by, AFF_CHARM );
	    return;
	}

	stop_follower( ch );
	return;
    }

    /*
     * NPC's are cool of course (as long as not charmed).
     * Hitting yourself is cool too (bleeding).
     * So is being immortal (Alander's idea).
     * And current killers stay as they are.
     */
    if ( IS_NPC(ch)
    ||   ch == victim
    ||   ch->level >= LEVEL_IMMORTAL
    ||   !IS_CLANNED(ch)
    ||	 ch->fighting  == victim)
	return;

    snprintf(buf, sizeof(buf),"$N is attempting to murder %s",victim->getName());
    Wiznet::instance()->report(buf,ch,NULL,WIZ_FLAGS,0,0);
    save_char_obj( ch );
    return;
}



/*
 * Check for parry.
 */
bool check_parry( Character *ch, Character *victim )
{
    int chance;

    if ( !IS_AWAKE(victim) )
	return FALSE;

    chance = get_skill(victim,gsn_parry) / 2;

    if ( get_eq_char( victim, WEAR_WIELD ) == NULL )
    {
	if (IS_NPC(victim))
	    chance /= 2;
	else
	    return FALSE;
    }

    if (!can_see(ch,victim))
	chance /= 2;

    if ( number_percent( ) >= chance + victim->level - ch->level )
	return FALSE;

    act( "You {Dparry{x $n's attack.",  ch, NULL, victim, TO_VICT    );
    act( "$N {Dparries{x your attack.", ch, NULL, victim, TO_CHAR    );
    check_improve(victim,gsn_parry,TRUE,6);
    return TRUE;
}

/*
 * Check for shield block.
 */
bool check_shield_block( Character *ch, Character *victim )
{
    int chance;

    if ( !IS_AWAKE(victim) )
        return FALSE;


    chance = get_skill(victim,gsn_shield_block) / 5 + 3;


    if ( get_eq_char( victim, WEAR_SHIELD ) == NULL )
        return FALSE;

    if ( number_percent( ) >= chance + victim->level - ch->level )
        return FALSE;

    act( "You block $n's attack with your shield.",  ch, NULL, victim, 
TO_VICT    );
    act( "$N blocks your attack with a shield.", ch, NULL, victim, 
TO_CHAR    );
    check_improve(victim,gsn_shield_block,TRUE,6);
    return TRUE;
}


/*
 * Check for dodge.
 */
bool check_dodge( Character *ch, Character *victim )
{
    int chance;

    if ( !IS_AWAKE(victim) )
	return FALSE;

    chance = get_skill(victim,gsn_dodge) / 2;

    if (!can_see(victim,ch))
	chance /= 2;

    if ( number_percent( ) >= chance + victim->level - ch->level )
        return FALSE;

    act( "You {Ddodge{x $n's attack.", ch, NULL, victim, TO_VICT    );
    act( "$N {Ddodges{x your attack.", ch, NULL, victim, TO_CHAR    );
    check_improve(victim,gsn_dodge,TRUE,6);
    return TRUE;
}



/*
 * Set position of a victim.
 */
void update_pos( Character *victim )
{
    if ( victim->hit > 0 )
    {
    	if ( victim->position <= POS_STUNNED )
	    victim->position = POS_STANDING;
	return;
    }

    if ( IS_NPC(victim) && victim->hit < 1 )
    {
	victim->position = POS_DEAD;
	return;
    }

    if ( victim->hit <= -11 )
    {
	victim->position = POS_DEAD;
	return;
    }

         if ( victim->hit <= -6 ) victim->position = POS_MORTAL;
    else if ( victim->hit <= -3 ) victim->position = POS_INCAP;
    else                          victim->position = POS_STUNNED;

    return;
}



/*
 * Start fights.
 */
void set_fighting( Character *ch, Character *victim )
{
    if ( ch->fighting != NULL )
    {
    	return;
    }

    if ( IS_AFFECTED(ch, AFF_SLEEP) )
	affect_strip( ch, gsn_sleep );

    ch->fighting = victim;
    ch->position = POS_FIGHTING;

    return;
}



/*
 * Stop fights.
 */
void stop_fighting( Character *ch, bool fBoth )
{
    Character *fch;

    for (std::list<Character*>::iterator it = char_list.begin(); it != char_list.end(); it++)
    {
    	fch = *it;
	if ( fch == ch || ( fBoth && fch->fighting == ch ) )
	{
	    fch->fighting	= NULL;
	    fch->position	= IS_NPC(fch) ? fch->default_pos : POS_STANDING;
	    update_pos( fch );
	}
    }

    return;
}



/*
 * Make a corpse out of a character.
 */
void make_corpse( Character *ch )
{
    char buf[MAX_STRING_LENGTH];
    OBJ_DATA *corpse;
    OBJ_DATA *obj;
    OBJ_DATA *obj_next;
    char *name;

    if ( IS_NPC(ch) )
    {
	name		= ch->short_descr;
	corpse		= create_object(get_obj_index(OBJ_VNUM_CORPSE_NPC), 0);
	corpse->timer	= number_range( 3, 6 );
	if ( ch->gold > 0 )
	{
	    obj_to_obj( create_money( ch->gold, ch->silver ), corpse );
	    ch->gold = 0;
	    ch->silver = 0;
	}
	corpse->cost = 0;
    }
    else
    {
	name		= ch->getName();
	corpse		= create_object(get_obj_index(OBJ_VNUM_CORPSE_PC), 0);
	corpse->timer	= number_range( 25, 40 );
	REMOVE_BIT(ch->act,PLR_CANLOOT);
	if (!IS_CLANNED(ch))
	    corpse->owner = str_dup(ch->getName());
	else
	{
	    corpse->owner = NULL;
	    if (ch->gold > 1 || ch->silver > 1)
	    {
		obj_to_obj(create_money(ch->gold / 2, ch->silver/2), corpse);
		ch->gold -= ch->gold/2;
		ch->silver -= ch->silver/2;
	    }
	}
		
	corpse->cost = 0;
    }

    corpse->level = ch->level;

    snprintf(buf, sizeof(buf), corpse->short_descr, name );
    free_string( corpse->short_descr );
    corpse->short_descr = str_dup( buf );

    snprintf(buf, sizeof(buf), corpse->description, name );
    free_string( corpse->description );
    corpse->description = str_dup( buf );

    for ( obj = ch->carrying; obj != NULL; obj = obj_next )
    {
	bool floating = FALSE;

	obj_next = obj->next_content;
	if (obj->wear_loc == WEAR_FLOAT)
	    floating = TRUE;
	obj_from_char( obj );
	if (obj->item_type == ITEM_POTION)
	    obj->timer = number_range(500,1000);
	if (obj->item_type == ITEM_SCROLL)
	    obj->timer = number_range(1000,2500);
	if (IS_SET(obj->extra_flags,ITEM_ROT_DEATH) && !floating)
	{
	    obj->timer = number_range(5,10);
	    REMOVE_BIT(obj->extra_flags,ITEM_ROT_DEATH);
	}
	REMOVE_BIT(obj->extra_flags,ITEM_VIS_DEATH);

	if ( IS_SET( obj->extra_flags, ITEM_INVENTORY ) )
	    extract_obj( obj );
	else if (floating)
	{
	    if (IS_OBJ_STAT(obj,ITEM_ROT_DEATH)) /* get rid of it! */
	    { 
		if (obj->contains != NULL)
		{
		    OBJ_DATA *in, *in_next;

		    act("$p evaporates,scattering its contents.",
			ch,obj,NULL,TO_ROOM);
		    for (in = obj->contains; in != NULL; in = in_next)
		    {
			in_next = in->next_content;
			obj_from_obj(in);
			obj_to_room(in,ch->in_room);
		    }
		 }
		 else
		    act("$p evaporates.",
			ch,obj,NULL,TO_ROOM);
		 extract_obj(obj);
	    }
	    else
	    {
		act("$p falls to the floor.",ch,obj,NULL,TO_ROOM);
		obj_to_room(obj,ch->in_room);
	    }
	}
	else
	    obj_to_obj( obj, corpse );
    }

    obj_to_room( corpse, ch->in_room );
    return;
}



/*
 * Improved Death_cry contributed by Diavolo.
 */
void death_cry( Character *ch )
{
    ROOM_INDEX_DATA *was_in_room;
    char *msg;
    int door;
    int vnum;

    vnum = 0;
    msg = (char*)"You hear $n's death cry.";

    switch ( number_bits(4))
    {
    case  0: msg  = (char*)"$n hits the ground ... DEAD.";			break;
    case  1: 
	if (ch->material == 0)
	{
	    msg  =(char*) "$n splatters blood on your armor.";		
	    break;
	}
    case  2: 							
	if (IS_SET(ch->parts,PART_GUTS))
	{
	    msg = (char*)"$n spills $s guts all over the floor.";
	    vnum = OBJ_VNUM_GUTS;
	}
	break;
    case  3: 
	if (IS_SET(ch->parts,PART_HEAD))
	{
	    msg  = (char*)"$n's severed head plops on the ground.";
	    vnum = OBJ_VNUM_SEVERED_HEAD;				
	}
	break;
    case  4: 
	if (IS_SET(ch->parts,PART_HEART))
	{
	    msg  = (char*)"$n's heart is torn from $s chest.";
	    vnum = OBJ_VNUM_TORN_HEART;				
	}
	break;
    case  5: 
	if (IS_SET(ch->parts,PART_ARMS))
	{
	    msg  = (char*)"$n's arm is sliced from $s dead body.";
	    vnum = OBJ_VNUM_SLICED_ARM;				
	}
	break;
    case  6: 
	if (IS_SET(ch->parts,PART_LEGS))
	{
	    msg  = (char*)"$n's leg is sliced from $s dead body.";
	    vnum = OBJ_VNUM_SLICED_LEG;				
	}
	break;
    case 7:
	if (IS_SET(ch->parts,PART_BRAINS))
	{
	    msg = (char*)"$n's head is shattered, and $s brains splash all over you.";
	    vnum = OBJ_VNUM_BRAINS;
	}
    }

    act( msg, ch, NULL, NULL, TO_ROOM );

    if ( vnum != 0 )
    {
	char buf[MAX_STRING_LENGTH];
	OBJ_DATA *obj;
	char *name;

	name		= IS_NPC(ch) ? ch->short_descr : ch->getName();
	obj		= create_object( get_obj_index( vnum ), 0 );
	obj->timer	= number_range( 4, 7 );

	snprintf(buf, sizeof(buf), obj->short_descr, name );
	free_string( obj->short_descr );
	obj->short_descr = str_dup( buf );

	snprintf(buf, sizeof(buf), obj->description, name );
	free_string( obj->description );
	obj->description = str_dup( buf );

	if (obj->item_type == ITEM_FOOD)
	{
	    if (IS_SET(ch->form,FORM_POISON))
		obj->value[3] = 1;
	    else if (!IS_SET(ch->form,FORM_EDIBLE))
		obj->item_type = ITEM_TRASH;
	}

	obj_to_room( obj, ch->in_room );
    }

    if ( IS_NPC(ch) )
	msg = (char*)"You hear something's death cry.";
    else
	msg = (char*)"You hear someone's death cry.";

    was_in_room = ch->in_room;
    for ( door = 0; door <= 5; door++ )
    {
	EXIT_DATA *pexit;

	if ( ( pexit = was_in_room->exit[door] ) != NULL
	&&   pexit->u1.to_room != NULL
	&&   pexit->u1.to_room != was_in_room )
	{
	    ch->in_room = pexit->u1.to_room;
	    act( msg, ch, NULL, NULL, TO_ROOM );
	}
    }
    ch->in_room = was_in_room;

    return;
}



void raw_kill( Character *victim )
{
    int i;

    stop_fighting( victim, TRUE );
    death_cry( victim );
    make_corpse( victim );

    if ( IS_NPC(victim) )
    {
	victim->pIndexData->killed++;
	kill_table[URANGE(0, victim->level, MAX_LEVEL-1)].killed++;
	extract_char( victim, TRUE );
	return;
    }

    extract_char( victim, FALSE );
    while ( victim->affected )
	affect_remove( victim, victim->affected );
    victim->affected_by	= race_table[victim->race].aff;
    for (i = 0; i < 4; i++)
    	victim->armor[i]= 100;
    victim->position	= POS_RESTING;
    victim->hit		= UMAX( 1, victim->hit  );
    victim->mana	= UMAX( 1, victim->mana );
    victim->move	= UMAX( 1, victim->move );
/*  save_char_obj( victim ); we're stable enough to not need this :) */
    return;
}



void group_gain( Character *ch, Character *victim )
{
    char buf[MAX_STRING_LENGTH];
    Character *gch;
    Character *lch;
    int xp;
    int members;
    int group_levels;

    /*
     * Monsters don't get kill xp's or alignment changes.
     * P-killing doesn't help either.
     * Dying of mortal wounds or poison doesn't give xp to anyone!
     */
    if ( victim == ch )
	return;
    
    members = 0;
    group_levels = 0;
    for ( gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room )
    {
	if ( is_same_group( gch, ch ) )
        {
	    members++;
	    group_levels += IS_NPC(gch) ? gch->level / 2 : gch->level;
	}
    }

    if ( members == 0 )
    {
	bug( "Group_gain: members.", members );
	members = 1;
	group_levels = ch->level ;
    }

    lch = (ch->leader != NULL) ? ch->leader : ch;

    for ( gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room )
    {
	OBJ_DATA *obj;
	OBJ_DATA *obj_next;

	if ( !is_same_group( gch, ch ) || IS_NPC(gch))
	    continue;

	if ( gch->level - lch->level >= 5 )
	{
	    send_to_char( "You are too high for this group.\n\r", gch );
	    continue;
	}

	if ( gch->level - lch->level <= -5 )
	{
	    send_to_char( "You are too low for this group.\n\r", gch );
	    continue;
	}

	xp = xp_compute( gch, victim, group_levels );  
	snprintf(buf, sizeof(buf), "You receive %d experience points.\n\r", xp );
	send_to_char( buf, gch );
	gch->gain_exp( xp );

	for ( obj = gch->carrying; obj != NULL; obj = obj_next )
	{
	    obj_next = obj->next_content;
	    if ( obj->wear_loc == WEAR_NONE )
		continue;

/*
 * No need for anti-good eq
 * Removed by Blizzard
	    if ( ( IS_OBJ_STAT(obj, ITEM_ANTI_EVIL)    && IS_EVIL(gch) )
	    ||   ( IS_OBJ_STAT(obj, ITEM_ANTI_GOOD)    && IS_GOOD(gch) )
	    ||   ( IS_OBJ_STAT(obj, ITEM_ANTI_NEUTRAL) && IS_NEUTRAL(gch) ) )
	    {
		act( "You are zapped by $p.", gch, obj, NULL, TO_CHAR );
		act( "$n is zapped by $p.",   gch, obj, NULL, TO_ROOM );
		obj_from_char( obj );
		obj_to_room( obj, gch->in_room );
	    }
*/
	}
    }

    return;
}



/*
 * Compute xp for a kill.
 * Also adjust alignment of killer.
 * Edit this function to change xp computations.
 */
int xp_compute( Character *gch, Character *victim, int total_levels )
{
    int xp,level_range;

    level_range = victim->level + 10 - gch->level;
    xp = 0;

    if (level_range >= 0)
	xp = level_range * number_range(15,35);

     xp += number_range(1,5);

    if (!IS_NPC(victim))
	xp *= 2;

    if (IS_CLANNED(gch))
    {
	if (IS_SET(gch->pcdata->clan->flags, CLAN_PK))
	    xp *= 1.5;
	else
	    xp *= 1.25;
    }

    return xp * 1.5;
}


void dam_message( Character *ch, Character *victim,int dam,int dt,bool immune )
{
    char buf1[256], buf2[256], buf3[256];
    const char *vs;
    const char *vp;
    const char *pm;
    const char *attack;
    char punct;

    if (ch == NULL || victim == NULL)
	return;

	 if ( dam <=  5 ) 	pm = "clumsy";
    else if ( dam <=  10 )	pm = "wobbly";
    else if ( dam <=  15 )      pm = "weak";
    else if ( dam <=  20 ) 	pm = "lucky";
    else if ( dam <=  30 )	pm = "confident";
    else if ( dam <=  36 ) 	pm = "amateur";
    else if ( dam <=  42 )	pm = "careful";
    else if ( dam <=  48 ) 	pm = "blind";
    else if ( dam <=  52 )	pm = "careless";
    else if ( dam <=  55 )	pm = "strong";
    else if ( dam <=  60 ) 	pm = "swift";
    else if ( dam <=  72 ) 	pm = "calculated";
    else if ( dam <=  84 ) 	pm = "quick";
    else if ( dam <=  96 ) 	pm = "furious";
    else if ( dam <= 108 ) 	pm = "skillful";
    else if ( dam <= 120 ) 	pm = "masterful";
    else			pm = "heavenly";

	 if ( dam ==   0 ) { vs = "{Dmiss{x";	 vp = "{Dmisses{x";	}
    else if ( dam <=   4 ) { vs = "{rscratch{x"; vp = "{rscratches{x";	}
    else if ( dam <=   8 ) { vs = "{rgraze{x";	 vp = "{rgrazes{x";	}
    else if ( dam <=  12 ) { vs = "{rhit{x";	 vp = "{rhits{x";	}
    else if ( dam <=  16 ) { vs = "{rinjure{x";	 vp = "{rinjures{x";	}
    else if ( dam <=  20 ) { vs = "{rwound{x";	 vp = "{rwounds{x";	}
    else if ( dam <=  24 ) { vs = "{rmaul{x";    vp = "{rmauls{x";	}
    else if ( dam <=  28 ) { vs = "{rdecimate{x"; vp = "{rdecimates{x"; }
    else if ( dam <=  32 ) { vs = "{rdevastate{x"; vp = "{rdevastates{x"; }
    else if ( dam <=  36 ) { vs = "{rmaim{x";	   vp = "{rmaims{x";	}
    else if ( dam <=  40 ) { vs = "{rMUTILATE{x";  vp ="{rMUTILATES{x";	}
    else if ( dam <=  44 ) { vs = "{rDISEMBOWEL{x"; vp = "{rDISEMBOWELS{x"; }
    else if ( dam <=  48 ) { vs = "{rDISMEMBER{x";  vp = "{rDISMEMBERS{x";  }
    else if ( dam <=  52 ) { vs = "{rMASSACRE{x";   vp = "{rMASSACRES{x";   }
    else if ( dam <=  56 ) { vs = "{rMANGLE{x";	    vp = "{rMANGLES{x";     }
    else if ( dam <=  60 ) { vs = "{W***{r DEMOLISH{W ***{x";
			     vp = "{W***{r DEMOLISHES{W ***{x";		}
    else if ( dam <=  75 ) { vs = "{m***{r DEVASTATE{m ***{x";
			     vp = "{m***{r DEVASTATES{m ***{x";		}
    else if ( dam <= 100)  { vs = "{y==={r OBLITERATE{y ==={x";
			     vp = "{y==={r OBLITERATES{y ==={x";	}
    else if ( dam <= 125)  { vs = "{y>>>{r ANNIHILATE{y <<<{x";
			     vp = "{y>>>{r ANNIHILATES{y <<<{x";	}
    else if ( dam <= 150)  { vs = "{y<<<{r ERADICATE{y >>>{x";
			     vp = "{y<<<{r ERADICATES{y >>>{x";		}
    else                   { vs = "do {RUNSPEAKABLE{x things to";
			     vp = "does {RUNSPEAKABLE{x things to";	}

    punct   = (dam <= 24) ? '.' : '!';

    if ( dt == TYPE_HIT )
    {
	if (ch  == victim)
	{
	    snprintf(buf1, sizeof(buf1), "$n %s $melf%c",vp,punct);
	    snprintf(buf2, sizeof(buf2), "You %s yourself%c",vs,punct);
	}
	else
	{
	    snprintf(buf1, sizeof(buf1), "$n %s $N%c",  vp, punct );
	    snprintf(buf2, sizeof(buf2), "You %s $N%c", vs, punct );
	    snprintf(buf3, sizeof(buf3), "$n %s you%c", vp, punct );
	}
    }
    else
    {
	if ( dt >= 0 && dt < MAX_SKILL )
	    attack	= skill_table[dt].noun_damage;
	else if ( dt >= TYPE_HIT
	&& dt < TYPE_HIT + MAX_DAMAGE_MESSAGE) 
	    attack	= attack_table[dt - TYPE_HIT].noun;
	else
	{
	    bug( "Dam_message: bad dt %d.", dt );
	    dt  = TYPE_HIT;
	    attack  = attack_table[0].name;
	}

	if (immune)
	{
	    if (ch == victim)
	    {
		snprintf(buf1, sizeof(buf1),"$n is unaffected by $s own %s.",attack);
		snprintf(buf2, sizeof(buf2),"Luckily, you are immune to that.");
	    } 
	    else
	    {
	    	snprintf(buf1, sizeof(buf1),"$N is unaffected by $n's %s!",attack);
	    	snprintf(buf2, sizeof(buf2),"$N is unaffected by your %s!",attack);
	    	snprintf(buf3, sizeof(buf3),"$n's %s is powerless against you.",attack);
	    }
	}
	else if (skill_table[dt].pgsn != NULL)
	{
	    if (ch == victim)
	    {
		snprintf(buf1, sizeof(buf1), "$n's %s %s %s $m%c",pm, attack,vp,punct);
		snprintf(buf2, sizeof(buf2), "Your %s %s %s you%c",pm, attack,vp,punct);
	    }
	    else if (IS_IMMORTAL(ch))
	    {
                snprintf(buf1, sizeof(buf1), "$n's %s %s %s $N%c", pm, attack, vp, punct );
            	snprintf(buf2, sizeof(buf2), "Your %s %s %s $N%c (%d)", pm, attack, vp, punct, dam );
	    	if (IS_IMMORTAL(victim))
	            snprintf(buf3, sizeof(buf3), "$n's %s %s %s you%c (%d)", pm, attack, vp, punct, dam );
	    	else
	            snprintf(buf3, sizeof(buf3), "$n's %s %s %s you%c", pm, attack, vp, punct );
	      }
	    else
	    {
	    	snprintf(buf1, sizeof(buf1), "$n's %s %s %s $N%c",  pm, attack, vp, punct );
	    	snprintf(buf2, sizeof(buf2), "Your %s %s %s $N%c",  pm, attack, vp, punct );
		if (IS_IMMORTAL(victim))
		    snprintf(buf3, sizeof(buf3), "$n's %s %s %s you%c (%d)", pm, attack, vp, punct, dam );
		else
	    	    snprintf(buf3, sizeof(buf3), "$n's %s %s %s you%c", pm, attack, vp, punct );
	    }
	}
	else
	{
	    if (ch == victim)
	    {
		snprintf(buf1, sizeof(buf1), "$n's %s %s $m%c", attack,vp,punct);
		snprintf(buf2, sizeof(buf2), "Your %s %s you%c", attack,vp,punct);
	    }
	    else if (IS_IMMORTAL(ch))
	    {
        	snprintf(buf1, sizeof(buf1), "$n's %s %s $N%c", attack, vp, punct );
        	snprintf(buf2, sizeof(buf2), "Your %s %s $N%c (%d)", attack, vp, punct, dam );
		if (IS_IMMORTAL(victim))
        	    snprintf(buf3, sizeof(buf3), "$n's %s %s you%c (%d)", attack, vp, punct, dam );
		else
        	    snprintf(buf3, sizeof(buf3), "$n's %s %s you%c", attack, vp, punct );
	    }
	    else
	    {
	    	snprintf(buf1, sizeof(buf1), "$n's %s %s $N%c", attack, vp, punct );
	    	snprintf(buf2, sizeof(buf2), "Your %s %s $N%c", attack, vp, punct );
		if (IS_IMMORTAL(victim))
		    snprintf(buf3, sizeof(buf3), "$n's %s %s you%c (%d)", attack, vp, punct, dam );
		else
	    	    snprintf(buf3, sizeof(buf3), "$n's %s %s you%c", attack, vp, punct );
	    }
	}
    }

    if (ch == victim)
    {
	act(buf1,ch,NULL,NULL,TO_ROOM);
	act(buf2,ch,NULL,NULL,TO_CHAR);
    }
    else
    {
    	act( buf1, ch, NULL, victim, TO_NOTVICT );
    	act( buf2, ch, NULL, victim, TO_CHAR );
    	act( buf3, ch, NULL, victim, TO_VICT );
    }

    return;
}



/*
 * Disarm a creature.
 * Caller must check for successful attack.
 */
void disarm( Character *ch, Character *victim )
{
    OBJ_DATA *obj;

    if ( ( obj = get_eq_char( victim, WEAR_WIELD ) ) == NULL )
	return;

    if ( IS_OBJ_STAT(obj,ITEM_NOREMOVE))
    {
	act("$S weapon won't budge!",ch,NULL,victim,TO_CHAR);
	act("$n tries to disarm you, but your weapon won't budge!",
	    ch,NULL,victim,TO_VICT);
	act("$n tries to disarm $N, but fails.",ch,NULL,victim,TO_NOTVICT);
	return;
    }

    act( "$n {YDISARMS{x you and sends your weapon flying!", 
	 ch, NULL, victim, TO_VICT    );
    act( "You disarm $N!",  ch, NULL, victim, TO_CHAR    );
    act( "$n disarms $N!",  ch, NULL, victim, TO_NOTVICT );

    obj_from_char( obj );
    if ( IS_OBJ_STAT(obj,ITEM_NODROP) || IS_OBJ_STAT(obj,ITEM_INVENTORY) )
	obj_to_char( obj, victim );
    else
    {
	obj_to_room( obj, victim->in_room );
	if (IS_NPC(victim) && victim->wait == 0 && can_see_obj(victim,obj))
	    get_obj(victim,obj,NULL);
    }

    return;
}

void do_berserk( Character *ch, char *argument)
{
    int chance, hp_percent;

    if ((chance = get_skill(ch,gsn_berserk)) == 0
    ||  (IS_NPC(ch) && !IS_SET(ch->off_flags,OFF_BERSERK))
    ||  (!IS_NPC(ch)
    &&   ch->level < ch->pcdata->sk_level[gsn_berserk]))
    {
	send_to_char("You turn red in the face, but nothing happens.\n\r",ch);
	return;
    }

    if (IS_AFFECTED(ch,AFF_BERSERK) || is_affected(ch,gsn_berserk)
    ||  is_affected(ch,skill_lookup("frenzy")))
    {
	send_to_char("You get a little madder.\n\r",ch);
	return;
    }

    if (IS_AFFECTED(ch,AFF_CALM))
    {
	send_to_char("You're feeling to mellow to berserk.\n\r",ch);
	return;
    }

    if (ch->mana < 50)
    {
	send_to_char("You can't get up enough energy.\n\r",ch);
	return;
    }

    /* modifiers */

    /* fighting */
    if (ch->position == POS_FIGHTING)
	chance += 10;

    /* damage -- below 50% of hp helps, above hurts */
    hp_percent = 100 * ch->hit/ch->max_hit;
    chance += 25 - hp_percent/2;

    if (number_percent() < chance)
    {
	AFFECT_DATA af;

	WAIT_STATE(ch,PULSE_VIOLENCE);
	ch->mana -= 50;
	ch->move /= 2;

	/* heal a little damage */
	ch->hit += ch->level * 2;
	ch->hit = UMIN(ch->hit,ch->max_hit);

	send_to_char("Your pulse races as you are consumed by rage!\n\r",ch);
	act("$n gets a wild look in $s eyes.",ch,NULL,NULL,TO_ROOM);
	check_improve(ch,gsn_berserk,TRUE,2);

	af.where	= TO_AFFECTS;
	af.type		= gsn_berserk;
	af.level	= ch->level;
	af.duration	= number_fuzzy(ch->level / 8);
	af.modifier	= UMAX(1,ch->level/5);
	af.bitvector 	= AFF_BERSERK;

	af.location	= APPLY_HITROLL;
	affect_to_char(ch,&af);

	af.location	= APPLY_DAMROLL;
	affect_to_char(ch,&af);

	af.modifier	= UMAX(10,10 * (ch->level/5));
	af.location	= APPLY_AC;
	affect_to_char(ch,&af);
    }

    else
    {
	WAIT_STATE(ch,3 * PULSE_VIOLENCE);
	ch->mana -= 25;
	ch->move /= 2;

	send_to_char("Your pulse speeds up, but nothing happens.\n\r",ch);
	check_improve(ch,gsn_berserk,FALSE,2);
    }
}

void do_bash( Character *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    Character *victim;
    int chance;

    one_argument(argument,arg);
 
    if ( (chance = get_skill(ch,gsn_bash)) == 0
    ||	 (IS_NPC(ch) && !IS_SET(ch->off_flags,OFF_BASH))
    ||	 (!IS_NPC(ch)
    &&	  ch->level < ch->pcdata->sk_level[gsn_bash]))
    {	
	send_to_char("Bashing? What's that?\n\r",ch);
	return;
    }
 
    if (arg[0] == '\0')
    {
	victim = ch->fighting;
	if (victim == NULL)
	{
	    send_to_char("But you aren't fighting anyone!\n\r",ch);
	    return;
	}
    }

    else if ((victim = get_char_room(ch,arg)) == NULL)
    {
	send_to_char("They aren't here.\n\r",ch);
	return;
    }

    if (victim->position < POS_FIGHTING)
    {
	act("You'll have to let $M get back up first.",ch,NULL,victim,TO_CHAR);
	return;
    } 

    if (victim == ch)
    {
	send_to_char("You try to bash your brains out, but fail.\n\r",ch);
	return;
    }

    if (is_safe(ch,victim))
	return;

    if ( IS_NPC(victim) && 
	victim->fighting != NULL && 
	!is_same_group(ch,victim->fighting))
    {
        send_to_char("Kill stealing is not permitted.\n\r",ch);
        return;
    }

    if (IS_AFFECTED(ch,AFF_CHARM) && ch->master == victim)
    {
	act("But $N is your friend!",ch,NULL,victim,TO_CHAR);
	return;
    }

    /* modifiers */

    /* size  and weight */
    chance += ch->carry_weight / 250;
    chance -= victim->carry_weight / 200;

    if (ch->size < victim->size)
	chance += (ch->size - victim->size) * 15;
    else
	chance += (ch->size - victim->size) * 10; 


    /* stats */
    chance += get_curr_stat(ch,STAT_STR);
    chance -= (get_curr_stat(victim,STAT_DEX) * 4)/3;
    chance -= GET_AC(victim,AC_BASH) /25;
    /* speed */
    if (IS_SET(ch->off_flags,OFF_FAST) || IS_AFFECTED(ch,AFF_HASTE))
        chance += 10;
    if (IS_SET(victim->off_flags,OFF_FAST) || IS_AFFECTED(victim,AFF_HASTE))
        chance -= 30;

    /* level */
    chance += (ch->level - victim->level);

    if (!IS_NPC(victim) 
	&& chance < get_skill(victim,gsn_dodge) )
    {	/*
        act("$n tries to bash you, but you dodge it.",ch,NULL,victim,TO_VICT);
        act("$N dodges your bash, you fall flat on your face.",ch,NULL,victim,TO_CHAR);
        WAIT_STATE(ch,skill_table[gsn_bash].beats);
        return;*/
	chance -= 3 * (get_skill(victim,gsn_dodge) - chance);
    }

    /* now the attack */
    if (number_percent() < chance )
    {
    
	act("$n sends you sprawling with a powerful bash!",
		ch,NULL,victim,TO_VICT);
	act("You slam into $N, and send $M flying!",ch,NULL,victim,TO_CHAR);
	act("$n sends $N sprawling with a powerful bash.",
		ch,NULL,victim,TO_NOTVICT);
	check_improve(ch,gsn_bash,TRUE,1);

	DAZE_STATE(victim, 3 * PULSE_VIOLENCE);
	WAIT_STATE(ch,skill_table[gsn_bash].beats);
	victim->position = POS_RESTING;
	damage(ch,victim,number_range(2,2 + 2 * ch->size + chance/20),gsn_bash,
	    DAM_BASH,FALSE);
	
    }
    else
    {
	damage(ch,victim,0,gsn_bash,DAM_BASH,FALSE);
	act("You fall flat on your face!",
	    ch,NULL,victim,TO_CHAR);
	act("$n falls flat on $s face.",
	    ch,NULL,victim,TO_NOTVICT);
	act("You evade $n's bash, causing $m to fall flat on $s face.",
	    ch,NULL,victim,TO_VICT);
	check_improve(ch,gsn_bash,FALSE,1);
	ch->position = POS_RESTING;
	WAIT_STATE(ch,skill_table[gsn_bash].beats * 3/2); 
    }
	check_killer(ch,victim);
}

void do_dirt( Character *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    Character *victim;
    int chance;

    one_argument(argument,arg);

    if ( (chance = get_skill(ch,gsn_dirt)) == 0
    ||   (IS_NPC(ch) && !IS_SET(ch->off_flags,OFF_KICK_DIRT))
    ||   (!IS_NPC(ch)
    &&    ch->level < ch->pcdata->sk_level[gsn_dirt]))
    {
	send_to_char("You get your feet dirty.\n\r",ch);
	return;
    }

    if (arg[0] == '\0')
    {
	victim = ch->fighting;
	if (victim == NULL)
	{
	    send_to_char("But you aren't in combat!\n\r",ch);
	    return;
	}
    }

    else if ((victim = get_char_room(ch,arg)) == NULL)
    {
	send_to_char("They aren't here.\n\r",ch);
	return;
    }

    if (IS_AFFECTED(victim,AFF_BLIND))
    {
	act("$E's already been blinded.",ch,NULL,victim,TO_CHAR);
	return;
    }

    if (victim == ch)
    {
	send_to_char("Very funny.\n\r",ch);
	return;
    }

    if (is_safe(ch,victim))
	return;

    if (IS_NPC(victim) &&
	 victim->fighting != NULL && 
	!is_same_group(ch,victim->fighting))
    {
        send_to_char("Kill stealing is not permitted.\n\r",ch);
        return;
    }

    if (IS_AFFECTED(ch,AFF_CHARM) && ch->master == victim)
    {
	act("But $N is such a good friend!",ch,NULL,victim,TO_CHAR);
	return;
    }

    /* modifiers */

    /* dexterity */
    chance += get_curr_stat(ch,STAT_DEX);
    chance -= 2 * get_curr_stat(victim,STAT_DEX);

    /* speed  */
    if (IS_SET(ch->off_flags,OFF_FAST) || IS_AFFECTED(ch,AFF_HASTE))
	chance += 10;
    if (IS_SET(victim->off_flags,OFF_FAST) || IS_AFFECTED(victim,AFF_HASTE))
	chance -= 25;

    /* level */
    chance += (ch->level - victim->level) * 2;

    /* sloppy hack to prevent false zeroes */
    if (chance % 5 == 0)
	chance += 1;

    /* terrain */

    switch(ch->in_room->sector_type)
    {
	case(SECT_INSIDE):		chance -= 20;	break;
	case(SECT_CITY):		chance -= 10;	break;
	case(SECT_FIELD):		chance +=  5;	break;
	case(SECT_FOREST):				break;
	case(SECT_HILLS):				break;
	case(SECT_MOUNTAIN):		chance -= 10;	break;
	case(SECT_WATER_SWIM):		chance  =  0;	break;
	case(SECT_WATER_NOSWIM):	chance  =  0;	break;
	case(SECT_AIR):			chance  =  0;  	break;
	case(SECT_DESERT):		chance += 10;   break;
    }

    if (chance == 0)
    {
	send_to_char("There isn't any dirt to kick.\n\r",ch);
	return;
    }

    /* now the attack */
    if (number_percent() < chance)
    {
	AFFECT_DATA af;
	act("$n is blinded by the dirt in $s eyes!",victim,NULL,NULL,TO_ROOM);
	act("$n kicks dirt in your eyes!",ch,NULL,victim,TO_VICT);
        damage(ch,victim,number_range(2,5),gsn_dirt,DAM_NONE,FALSE);
	send_to_char("You can't see a thing!\n\r",victim);
	check_improve(ch,gsn_dirt,TRUE,2);
	WAIT_STATE(ch,skill_table[gsn_dirt].beats);

	af.where	= TO_AFFECTS;
	af.type 	= gsn_dirt;
	af.level 	= ch->level;
	af.duration	= 0;
	af.location	= APPLY_HITROLL;
	af.modifier	= -4;
	af.bitvector 	= AFF_BLIND;

	affect_to_char(victim,&af);
    }
    else
    {
	damage(ch,victim,0,gsn_dirt,DAM_NONE,TRUE);
	check_improve(ch,gsn_dirt,FALSE,2);
	WAIT_STATE(ch,skill_table[gsn_dirt].beats);
    }
	check_killer(ch,victim);
}

void do_trip( Character *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    Character *victim;
    int chance;

    one_argument(argument,arg);

    if ( (chance = get_skill(ch,gsn_trip)) == 0
    ||   (IS_NPC(ch) && !IS_SET(ch->off_flags,OFF_TRIP))
    ||   (!IS_NPC(ch) 
	  && ch->level < ch->pcdata->sk_level[gsn_trip]))
    {
	send_to_char("Tripping?  What's that?\n\r",ch);
	return;
    }


    if (arg[0] == '\0')
    {
	victim = ch->fighting;
	if (victim == NULL)
	{
	    send_to_char("But you aren't fighting anyone!\n\r",ch);
	    return;
 	}
    }

    else if ((victim = get_char_room(ch,arg)) == NULL)
    {
	send_to_char("They aren't here.\n\r",ch);
	return;
    }

    if (is_safe(ch,victim))
	return;

    if (IS_NPC(victim) &&
	 victim->fighting != NULL && 
	!is_same_group(ch,victim->fighting))
    {
	send_to_char("Kill stealing is not permitted.\n\r",ch);
	return;
    }
    
    if (IS_AFFECTED(victim,AFF_FLYING))
    {
	act("$S feet aren't on the ground.",ch,NULL,victim,TO_CHAR);
	return;
    }

    if (victim->position < POS_FIGHTING)
    {
	act("$N is already down.",ch,NULL,victim,TO_CHAR);
	return;
    }

    if (victim == ch)
    {
	send_to_char("You fall flat on your face!\n\r",ch);
	WAIT_STATE(ch,2 * skill_table[gsn_trip].beats);
	act("$n trips over $s own feet!",ch,NULL,NULL,TO_ROOM);
	return;
    }

    if (IS_AFFECTED(ch,AFF_CHARM) && ch->master == victim)
    {
	act("$N is your beloved master.",ch,NULL,victim,TO_CHAR);
	return;
    }

    /* modifiers */

    /* size */
    if (ch->size < victim->size)
        chance += (ch->size - victim->size) * 10;  /* bigger = harder to trip */

    /* dex */
    chance += get_curr_stat(ch,STAT_DEX);
    chance -= get_curr_stat(victim,STAT_DEX) * 3 / 2;

    /* speed */
    if (IS_SET(ch->off_flags,OFF_FAST) || IS_AFFECTED(ch,AFF_HASTE))
	chance += 10;
    if (IS_SET(victim->off_flags,OFF_FAST) || IS_AFFECTED(victim,AFF_HASTE))
	chance -= 20;

    /* level */
    chance += (ch->level - victim->level) * 2;


    /* now the attack */
    if (number_percent() < chance)
    {
	act("$n trips you and you go down!",ch,NULL,victim,TO_VICT);
	act("You trip $N and $N goes down!",ch,NULL,victim,TO_CHAR);
	act("$n trips $N, sending $M to the ground.",ch,NULL,victim,TO_NOTVICT);
	check_improve(ch,gsn_trip,TRUE,1);

	DAZE_STATE(victim,2 * PULSE_VIOLENCE);
        WAIT_STATE(ch,skill_table[gsn_trip].beats);
	victim->position = POS_RESTING;
	damage(ch,victim,number_range(2, 2 +  2 * victim->size),gsn_trip,
	    DAM_BASH,TRUE);
    }
    else
    {
	damage(ch,victim,0,gsn_trip,DAM_BASH,TRUE);
	WAIT_STATE(ch,skill_table[gsn_trip].beats*2/3);
	check_improve(ch,gsn_trip,FALSE,1);
    } 
	check_killer(ch,victim);
}



void do_kill( Character *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    Character *victim;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	send_to_char( "Kill whom?\n\r", ch );
	return;
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL )
    {
	send_to_char( "They aren't here.\n\r", ch );
	return;
    }

    if ( victim == ch )
    {
	send_to_char( "You hit yourself.  Ouch!\n\r", ch );
	multi_hit( ch, ch, TYPE_UNDEFINED );
	return;
    }

    if ( is_safe( ch, victim ) )
	return;

    if ( victim->fighting != NULL && 
	!is_same_group(ch,victim->fighting))
    {
        send_to_char("Kill stealing is not permitted.\n\r",ch);
        return;
    }

    if ( IS_AFFECTED(ch, AFF_CHARM) && ch->master == victim )
    {
	act( "$N is your beloved master.", ch, NULL, victim, TO_CHAR );
	return;
    }

    if ( ch->position == POS_FIGHTING )
    {
	send_to_char( "You do the best you can!\n\r", ch );
	return;
    }

    WAIT_STATE( ch, 1 * PULSE_VIOLENCE );
    check_killer( ch, victim );
    multi_hit( ch, victim, TYPE_UNDEFINED );
    return;
}



void do_murde( Character *ch, char *argument )
{
    send_to_char( "If you want to MURDER, spell it out.\n\r", ch );
    return;
}



void do_murder( Character *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    Character *victim;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	send_to_char( "Murder whom?\n\r", ch );
	return;
    }

    if (IS_AFFECTED(ch,AFF_CHARM) || (IS_NPC(ch) && IS_SET(ch->act,ACT_PET)))
	return;

    if ( ( victim = get_char_room( ch, arg ) ) == NULL )
    {
	send_to_char( "They aren't here.\n\r", ch );
	return;
    }

    if ( victim == ch )
    {
	send_to_char( "Suicide is a mortal sin.\n\r", ch );
	return;
    }

    if ( is_safe( ch, victim ) )
	return;

    if (IS_NPC(victim) &&
	 victim->fighting != NULL && 
	!is_same_group(ch,victim->fighting))
    {
        send_to_char("Kill stealing is not permitted.\n\r",ch);
        return;
    }

    if ( IS_AFFECTED(ch, AFF_CHARM) && ch->master == victim )
    {
	act( "$N is your beloved master.", ch, NULL, victim, TO_CHAR );
	return;
    }

    if ( ch->position == POS_FIGHTING )
    {
	send_to_char( "You do the best you can!\n\r", ch );
	return;
    }

    WAIT_STATE( ch, 1 * PULSE_VIOLENCE );
    if (IS_NPC(ch))
	snprintf(buf, sizeof(buf), "Help! I am being attacked by %s!",ch->short_descr);
    else
    	snprintf(buf, sizeof(buf), "Help!  I am being attacked by %s!", ch->getName() );
    do_yell( victim, buf );
    check_killer( ch, victim );
    multi_hit( ch, victim, TYPE_UNDEFINED );
    return;
}



void do_backstab( Character *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    Character *victim;
    OBJ_DATA *obj;

    one_argument( argument, arg );

    if (arg[0] == '\0')
    {
        send_to_char("Backstab whom?\n\r",ch);
        return;
    }

    if (ch->fighting != NULL)
    {
	send_to_char("You're facing the wrong end.\n\r",ch);
	return;
    }
 
    else if ((victim = get_char_room(ch,arg)) == NULL)
    {
        send_to_char("They aren't here.\n\r",ch);
        return;
    }

    if ( victim == ch )
    {
	send_to_char( "How can you sneak up on yourself?\n\r", ch );
	return;
    }

    if ( is_safe( ch, victim ) )
      return;

    if (IS_NPC(victim) &&
	 victim->fighting != NULL && 
	!is_same_group(ch,victim->fighting))
    {
        send_to_char("Kill stealing is not permitted.\n\r",ch);
        return;
    }

    if ( ( obj = get_eq_char( ch, WEAR_WIELD ) ) == NULL)
    {
	send_to_char( "You need to wield a weapon to backstab.\n\r", ch );
	return;
    }

    if ( (victim->hit < victim->max_hit / 3 && IS_AWAKE(victim))
	 || (victim->fighting == NULL
	 && get_skill(ch,gsn_backstab) < number_range(1,95)))
    {
	act( "$N is hurt and suspicious ... you can't sneak up.",
	    ch, NULL, victim, TO_CHAR );
	return;
    }

    check_killer( ch, victim );
    WAIT_STATE( ch, skill_table[gsn_backstab].beats );
    if ( number_range(1, 80) < get_skill(ch,gsn_backstab)
    || ( get_skill(ch,gsn_backstab) >= 2 && !IS_AWAKE(victim) ) )
    {
	check_improve(ch,gsn_backstab,TRUE,1);
	multi_hit( ch, victim, gsn_backstab );
    }
    else
    {
	check_improve(ch,gsn_backstab,FALSE,1);
	damage( ch, victim, 0, gsn_backstab,DAM_NONE,TRUE);
    }

    return;
}



void do_flee( Character *ch, char *argument )
{
    ROOM_INDEX_DATA *was_in;
    ROOM_INDEX_DATA *now_in;
    Character *victim;
    int attempt;

    if ( ( victim = ch->fighting ) == NULL )
    {
        if ( ch->position == POS_FIGHTING )
            ch->position = POS_STANDING;
	send_to_char( "You aren't fighting anyone.\n\r", ch );
	return;
    }

    was_in = ch->in_room;
    for ( attempt = 0; attempt < 6; attempt++ )
    {
	EXIT_DATA *pexit;
	int door;

	door = number_door( );
	if ( ( pexit = was_in->exit[door] ) == 0
	||   pexit->u1.to_room == NULL
	||   IS_SET(pexit->exit_info, EX_CLOSED)
	||   number_range(0,ch->daze) != 0
	|| ( IS_NPC(ch)
	&&   IS_SET(pexit->u1.to_room->room_flags, ROOM_NO_MOB) ) )
	    continue;

	move_char( ch, door, FALSE );
	if ( ( now_in = ch->in_room ) == was_in )
	    continue;

	ch->in_room = was_in;
	act( "$n has fled!", ch, NULL, NULL, TO_ROOM );
	ch->in_room = now_in;

	if ( !IS_NPC(ch) )
	{
	    send_to_char( "You flee from combat!\n\r", ch );
	if( (ch->class_num == 2) 
	    && (number_percent() < 3*(ch->level/2) ) )
		send_to_char( "You snuck away safely.\n\r", ch);
	else
	    {
	    send_to_char( "You lost 10 exp.\n\r", ch); 
	    ch->gain_exp( -10 );
	    }
	}

	stop_fighting( ch, TRUE );
	return;
    }

    send_to_char( "PANIC! You couldn't escape!\n\r", ch );
    return;
}



void do_rescue( Character *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    Character *victim;
    Character *fch;

    one_argument( argument, arg );
    if ( arg[0] == '\0' )
    {
	send_to_char( "Rescue whom?\n\r", ch );
	return;
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL )
    {
	send_to_char( "They aren't here.\n\r", ch );
	return;
    }

    if ( victim == ch )
    {
	send_to_char( "What about fleeing instead?\n\r", ch );
	return;
    }

    if ( !IS_NPC(ch) && IS_NPC(victim) && !is_same_group(ch,victim) )
    {
	act("$N doesn't need your help!",ch,NULL,victim,TO_CHAR);
	return;
    }

    if ( ch->fighting == victim )
    {
	send_to_char( "Too late.\n\r", ch );
	return;
    }

    if ( ( fch = victim->fighting ) == NULL )
    {
	send_to_char( "That person is not fighting right now.\n\r", ch );
	return;
    }

    if ( IS_NPC(fch) && !is_same_group(ch,victim))
    {
        send_to_char("Kill stealing is not permitted.\n\r",ch);
        return;
    }

    WAIT_STATE( ch, skill_table[gsn_rescue].beats );
    if ( number_percent( ) > get_skill(ch,gsn_rescue))
    {
	send_to_char( "You fail the rescue.\n\r", ch );
	check_improve(ch,gsn_rescue,FALSE,1);
	return;
    }

    act( "You rescue $N!",  ch, NULL, victim, TO_CHAR    );
    act( "$n rescues you!", ch, NULL, victim, TO_VICT    );
    act( "$n rescues $N!",  ch, NULL, victim, TO_NOTVICT );
    check_improve(ch,gsn_rescue,TRUE,1);

    stop_fighting( fch, FALSE );
    stop_fighting( victim, FALSE );

    check_killer( ch, fch );
    set_fighting( ch, fch );
    set_fighting( fch, ch );
    return;
}



void do_kick( Character *ch, char *argument ) {
    Character *victim;

    /* Have to have the skill, unless a bunny */
    if (!IS_NPC(ch)
        && ch->level < ch->pcdata->sk_level[gsn_kick]
        && !IS_BUNNY(ch)) {
        send_to_char(
                "You better leave the martial arts to fighters.\n\r", ch);
        return;
    }

    if (IS_NPC(ch) && !IS_SET(ch->off_flags, OFF_KICK))
        return;

    if (argument[0] == '\0') {
        victim = ch->fighting;
        if (victim == NULL) {
            send_to_char("But you aren't fighting anyone!\n\r", ch);
            return;
        }
    } else if ((victim = get_char_room(ch, argument)) == NULL) {
        send_to_char("They aren't here.\n\r", ch);
        return;
    }

    WAIT_STATE(ch, skill_table[gsn_kick].beats);
    if (get_skill(ch, gsn_kick) > number_percent()) {
        damage(ch, victim, number_range(1, ch->level), gsn_kick, DAM_BASH, TRUE);
        check_improve(ch, gsn_kick, TRUE, 1);

        /* Hyperkick! */
        if (IS_BUNNY(ch)) {
            damage(ch, victim, number_range(1, ch->level / 2), gsn_kick, DAM_BASH, TRUE);
            check_improve(ch, gsn_kick, TRUE, 2);
            damage(ch, victim, number_range(1, ch->level / 3), gsn_kick, DAM_PIERCE, TRUE);
            check_improve(ch, gsn_kick, TRUE, 4);
            damage(ch, victim, number_range(1, ch->level / 4), gsn_kick, DAM_SLASH, TRUE);
            check_improve(ch, gsn_kick, TRUE, 8);
            damage(ch, victim, number_range(1, ch->level / 5), gsn_kick, DAM_BASH, TRUE);
            check_improve(ch, gsn_kick, TRUE, 16);
            damage(ch, victim, number_range(1, ch->level / 6), gsn_kick, DAM_BASH, TRUE);
            check_improve(ch, gsn_kick, TRUE, 32);
            damage(ch, victim, number_range(1, ch->level / 7), gsn_kick, DAM_BASH, TRUE);
            check_improve(ch, gsn_kick, TRUE, 64);
            damage(ch, victim, number_range(1, ch->level / 2), gsn_kick, DAM_BASH, TRUE);
            check_improve(ch, gsn_kick, TRUE, 128);
        }
    } else {
        damage(ch, victim, 0, gsn_kick, DAM_BASH, TRUE);
        check_improve(ch, gsn_kick, FALSE, 1);
    }
    check_killer(ch, victim);
    return;
}




void do_disarm( Character *ch, char *argument )
{
    Character *victim;
    OBJ_DATA *obj;
    int chance,hth,ch_weapon,vict_weapon,ch_vict_weapon;

    hth = 0;

    if ((chance = get_skill(ch,gsn_disarm)) == 0)
    {
	send_to_char( "You don't know how to disarm opponents.\n\r", ch );
	return;
    }

    if ( get_eq_char( ch, WEAR_WIELD ) == NULL 
    &&   ((hth = get_skill(ch,gsn_hand_to_hand)) == 0
    ||    (IS_NPC(ch) && !IS_SET(ch->off_flags,OFF_DISARM))))
    {
	send_to_char( "You must wield a weapon to disarm.\n\r", ch );
	return;
    }

    if ( ( victim = ch->fighting ) == NULL )
    {
	send_to_char( "You aren't fighting anyone.\n\r", ch );
	return;
    }

    if ( ( obj = get_eq_char( victim, WEAR_WIELD ) ) == NULL )
    {
	send_to_char( "Your opponent is not wielding a weapon.\n\r", ch );
	return;
    }

    /* find weapon skills */
    ch_weapon = get_weapon_skill(ch,get_weapon_sn(ch));
    vict_weapon = get_weapon_skill(victim,get_weapon_sn(victim));
    ch_vict_weapon = get_weapon_skill(ch,get_weapon_sn(victim));

    /* modifiers */

    /* skill */
    if ( get_eq_char(ch,WEAR_WIELD) == NULL)
	chance = chance * hth/150;
    else
	chance = chance * ch_weapon/100;

    chance += (ch_vict_weapon/2 - vict_weapon) / 2; 

    /* dex vs. strength */
    chance += get_curr_stat(ch,STAT_DEX);
    chance -= 2 * get_curr_stat(victim,STAT_STR);

    /* level */
    chance += (ch->level - victim->level) * 2;
 
    /* and now the attack */
    if (number_percent() < chance)
    {
    	WAIT_STATE( ch, skill_table[gsn_disarm].beats );
	disarm( ch, victim );
	check_improve(ch,gsn_disarm,TRUE,1);
    }
    else
    {
	WAIT_STATE(ch,skill_table[gsn_disarm].beats);
	act("You fail to disarm $N.",ch,NULL,victim,TO_CHAR);
	act("$n tries to disarm you, but fails.",ch,NULL,victim,TO_VICT);
	act("$n tries to disarm $N, but fails.",ch,NULL,victim,TO_NOTVICT);
	check_improve(ch,gsn_disarm,FALSE,1);
    }
    check_killer(ch,victim);
    return;
}

void do_surrender( Character *ch, char *argument )
{
    Character *mob;
    if ( (mob = ch->fighting) == NULL )
    {
	send_to_char( "But you're not fighting!\n\r", ch );
	return;
    }
    act( "You surrender to $N!", ch, NULL, mob, TO_CHAR );
    act( "$n surrenders to you!", ch, NULL, mob, TO_VICT );
    act( "$n tries to surrender to $N!", ch, NULL, mob, TO_NOTVICT );
    stop_fighting( ch, TRUE );

    if ( !IS_NPC( ch ) && IS_NPC( mob ) 
    &&   ( !HAS_TRIGGER( mob, TRIG_SURR ) 
        || !mp_percent_trigger( mob, ch, NULL, NULL, TRIG_SURR ) ) )
    {
	act( "$N seems to ignore your cowardly act!", ch, NULL, mob, TO_CHAR );
	multi_hit( mob, ch, TYPE_UNDEFINED );
    }
}

void do_sla( Character *ch, char *argument )
{
    send_to_char( "If you want to SLAY, spell it out.\n\r", ch );
    return;
}



void do_slay( Character *ch, char *argument )
{
    Character *victim;
    char arg[MAX_INPUT_LENGTH];

    one_argument( argument, arg );
    if ( arg[0] == '\0' )
    {
	send_to_char( "Slay whom?\n\r", ch );
	return;
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL )
    {
	send_to_char( "They aren't here.\n\r", ch );
	return;
    }

    if ( ch == victim )
    {
	send_to_char( "Suicide is a mortal sin.\n\r", ch );
	return;
    }

    if ( !IS_NPC(victim) && (victim->level >= get_trust(ch) || ch->level < MAX_LEVEL - 3))
    {
	send_to_char( "You failed.\n\r", ch );
	return;
    }

    act( "You slay $M in cold blood!",  ch, NULL, victim, TO_CHAR    );
    act( "$n slays you in cold blood!", ch, NULL, victim, TO_VICT    );
    act( "$n slays $N in cold blood!",  ch, NULL, victim, TO_NOTVICT );
    raw_kill( victim );
    return;
}

void do_bite( Character *ch, char *argument )
{
    Character *victim;
    int dam;

    if ( !IS_NPC(ch)
    &&   ch->level < ch->pcdata->sk_level[gsn_bite])
    {
	send_to_char(
	    "Your teeth are too dull for that.\n\r", ch );
	return;
    }

    if (IS_NPC(ch))
	return;

    if (argument[0] == '\0')
    {
        victim = ch->fighting;
        if (victim == NULL)
        {
            send_to_char("But you aren't fighting anyone!\n\r",ch);
            return;
        }
    }
    else if ((victim = get_char_room(ch,argument)) == NULL)
    {
        send_to_char("They aren't here.\n\r",ch);
        return;
    }

    dam = 0;

    if ( IS_SET(ch->act, PLR_IS_MORPHED) )
    {
	if (IS_BUNNY(ch))
	    dam = number_range(ch->level / 2, get_skill(ch,gsn_bite) / 8) + (GET_DAMROLL(ch) / 2);
	else
	    dam = number_range(ch->level, get_skill(ch,gsn_bite) / 4) + (GET_DAMROLL(ch));
    }
    else
    {
	send_to_char("You must be in your morphed form to use this skill.\n\r", ch );
	return;
    }

    WAIT_STATE( ch, skill_table[gsn_bite].beats );
    if ( get_skill(ch,gsn_bite) > number_percent())
    {
	damage(ch,victim, dam, gsn_bite,DAM_PIERCE,TRUE);
	check_improve(ch,gsn_bite,TRUE,1);
    }
    else
    {
	damage( ch, victim, 0, gsn_bite,DAM_PIERCE,TRUE);
	check_improve(ch,gsn_bite,FALSE,1);
    }
	check_killer(ch,victim);
    return;
}

void do_breath( Character *ch, char *argument )
{
    Character *victim;
    char buf[MAX_STRING_LENGTH];
    int bonus = 0;

    if (ch->race != race_lookup("draconian") || ch->level < 10)
    {
	send_to_char("Only adult draconians may use breath weapons.\n\r",ch);
	return;
    }

    if (argument[0] == '\0')
    {
	if ((victim = ch->fighting) == NULL)
	{
	    send_to_char("You are not fighting anyone.\n\r",ch);
	    return;
	}
    }
    else if ((victim = get_char_room(ch, argument)) == NULL || !can_see(victim, ch))
    {
	send_to_char("They are not here.\n\r",ch);
	return;
    }

    if (is_safe(ch, victim))
    {
	act("You cannot attack $N.", ch, NULL, victim, TO_CHAR);
	return;
    }

    if (number_range(1,110) > get_skill(ch,gsn_breath))
    {
	send_to_char("You turn red in the face, but not much happens.\n\r",ch);
        check_improve(ch,gsn_breath,FALSE,5);
	return;
    }

    if (get_skill(ch,gsn_breath) >= 75)
        bonus = get_skill(ch,gsn_breath) - 74;
    
	bonus -= (ch->breathe * number_range( 10, 30) );

    snprintf(buf, sizeof(buf),"You breathe your deadly %s at $N!",
	draconian_table[ch->drac].breath );
    act(buf,ch,NULL,victim,TO_CHAR);

    snprintf(buf, sizeof(buf),"$n breathes $s deadly %s at $N!",
	draconian_table[ch->drac].breath );
    act(buf,ch,NULL,victim,TO_ROOM);

    snprintf(buf, sizeof(buf),"$n breathes $s deadly %s at you!",
	draconian_table[ch->drac].breath );
    act(buf,ch,NULL,victim,TO_VICT);
 
    damage(ch,victim,number_range(ch->level * 5, ch->level * 10) + bonus,gsn_breath,draconian_table[ch->drac].damt,TRUE);
    WAIT_STATE(ch,skill_table[gsn_bash].beats);
    check_improve(ch,gsn_breath,TRUE,5);
    return;
}

void do_endure(Character *ch, char *arg)
{
  AFFECT_DATA af;

  if (IS_NPC(ch))
    {
      send_to_char("You have no endurance whatsoever.\n\r",ch);
      return;
    }

  if ( ch->level < skill_table[gsn_endure].skill_level[ch->class_num] ||
       ch->pcdata->learned[gsn_endure] <= 1 )
    {
      send_to_char("You lack the concentration.\n\r",ch);
      return;
    }
       
  if (is_affected(ch,gsn_endure))
    {
      send_to_char("You cannot endure more concentration.\n\r",ch);
      return;
    }
  

  WAIT_STATE( ch, skill_table[gsn_endure].beats );

  af.where 	= TO_AFFECTS;
  af.type 	= gsn_endure;
  af.level 	= ch->level;
  af.duration = ch->level / 4;
  af.location = APPLY_SAVING_SPELL;
  af.modifier = -1 * (get_skill(ch,gsn_endure) / 10); 
  af.bitvector = 0;

  affect_to_char(ch,&af);

  send_to_char("You prepare yourself for magical encounters.\n\r",ch);
  act("$n concentrates for a moment, then resumes $s position.",
      ch,NULL,NULL,TO_ROOM);
  check_improve(ch,gsn_endure,TRUE,1);
}

void do_tame(Character *ch, char *argument)
{
  Character *victim;
  char arg[MAX_INPUT_LENGTH];

  one_argument(argument,arg);

  if (arg[0] == '\0')
    {
      send_to_char("You are beyond taming.\n\r",ch);
      act("$n tries to tame $mself but fails miserably.",ch,NULL,NULL,TO_ROOM);
      return;
    }

  if ( (victim = get_char_room(ch,arg)) == NULL)
    {
      send_to_char("They're not here.\n\r",ch);
      return;
    }

  if (IS_NPC(ch))
    {
      send_to_char("Why don't you tame yourself first?",ch);
      return;
    }

  if (!IS_NPC(victim))
    {
      act("$N is beyond taming.",ch,NULL,victim,TO_CHAR);
      return;
    }

  if (!IS_SET(victim->act,ACT_AGGRESSIVE))
    {
      act("$N is not usually aggressive.",ch,NULL,victim,TO_CHAR);
      return;
    }

  WAIT_STATE( ch, skill_table[gsn_tame].beats );
  
  if (number_percent() < get_skill(ch,gsn_tame) + 15
                         + 4*(ch->level - victim->level))
    {
      REMOVE_BIT(victim->act,ACT_AGGRESSIVE);
      SET_BIT(victim->affected_by,AFF_CALM);
      send_to_char("You calm down.\n\r",victim);
      act("You calm $N down.",ch,NULL,victim,TO_CHAR);
      act("$n calms $N down.",ch,NULL,victim,TO_NOTVICT);
      stop_fighting(victim,TRUE);
      check_improve(ch,gsn_tame,TRUE,1);
    }
  else
    {
      send_to_char("You failed.\n\r",ch);
      act("$n tries to calm down $N but fails.",ch,NULL,victim,TO_NOTVICT);
      act("$n tries to calm you down but fails.",ch,NULL,victim,TO_VICT);
      check_improve(ch,gsn_tame,FALSE,1);
    }
}

void do_assassinate( Character *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    Character *victim;
    char buf[MAX_STRING_LENGTH];

    one_argument( argument, arg );

	if (ch->master != NULL && IS_NPC(ch))
	return;

    if ( !IS_NPC(ch)
    &&   ch->level < skill_table[gsn_assassinate].skill_level[ch->class_num] )
      {
	send_to_char("You don't know how to assassinate.\n\r",ch);
	return;
      }
    
    if ( IS_AFFECTED( ch, AFF_CHARM ) )  
    {
	send_to_char( "You don't want to kill your beloved master.\n\r",ch);
	return;
    } 

    if ( arg[0] == '\0' )
    {
	send_to_char( "Assassinate whom?\n\r", ch );
	return;
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL )
    {
	send_to_char( "They aren't here.\n\r", ch );
	return;
    }

    if ( victim == ch )
    {
	send_to_char( "Suicide is against your way.\n\r", ch );
	return;
    }

    if ( is_safe( ch, victim ) )
      return;

    if ( IS_IMMORTAL( victim ) && !IS_NPC(victim) )
    {
	send_to_char( "Your hands pass through.\n\r", ch );
	return;
    }

    if ( victim->fighting != NULL )
    {
	send_to_char( "You can't assassinate a fighting person.\n\r", ch );
	return;
    }
 
    if ( (get_eq_char( ch, WEAR_WIELD ) != NULL) ||
	 (get_eq_char( ch, WEAR_HOLD  ) != NULL) )  {
	send_to_char( 
	"You need both hands free to assassinate somebody.\n\r", ch );
	return;
    }
 
    if ( (victim->hit < victim->max_hit) && 
	 (can_see(victim, ch)) &&
	 (IS_AWAKE(victim) ) )
    {
	act( "$N is hurt and suspicious ... you can't sneak up.",
	    ch, NULL, victim, TO_CHAR );
	return;
    }
    
    if (IS_SET(victim->imm_flags, IMM_WEAPON))
      {
	act("$N seems immune to your assassination attempt.", ch, NULL,
		 victim, TO_CHAR);
	act("$N seems immune to $n's assassination attempt.", ch, NULL,
		victim, TO_ROOM);
	return;
      }

    WAIT_STATE( ch, skill_table[gsn_assassinate].beats );
    if ( IS_NPC(ch) ||
	!IS_AWAKE(victim) 
	||   number_percent( ) < get_skill(ch,gsn_assassinate))
      multi_hit(ch,victim,gsn_assassinate);
    else
      {
	check_improve(ch,gsn_assassinate,FALSE,1);
	damage( ch, victim, 0, gsn_assassinate,DAM_NONE, TRUE );
      }
    /* Player shouts if he doesn't die */
    if (!(IS_NPC(victim)) && !(IS_NPC(ch))
	&& victim->position == POS_FIGHTING)
      {
	if (!can_see(victim, ch))
	  do_yell(victim, (char*)"Help! Someone tried to assassinate me!");
	else
	  {
	    snprintf(buf, sizeof(buf), "Help! %s tried to assassinate me!", ch->getName() );
	    do_yell( victim, buf );
	  }
      }
    return;
  }
void do_strangle(Character *ch, char *argument)
{
    Character *victim;
    AFFECT_DATA af;

    if ( IS_NPC(ch) ||
	 ch->level < skill_table[gsn_strangle].skill_level[ch->class_num] )
    {
	send_to_char("You lack the skill to strangle.\n\r",ch);
	return;
    }

    if ( IS_AFFECTED( ch, AFF_CHARM ) )  {
	send_to_char( "You don't want to grap your beloved masters' neck.\n\r",ch);
	return;
    } 

    if ( (victim = get_char_room(ch,argument)) == NULL)
      {
	send_to_char("You do not see that person here.\n\r",ch);
	return;
      }

    if (ch==victim)
      {
	send_to_char("Even you are not that stupid.\n\r",ch);
	return;
      }

    if (is_affected(victim,gsn_strangle))
      return;

    if (is_safe(ch,victim))
      return;

    WAIT_STATE(ch,skill_table[gsn_strangle].beats);

    if (IS_NPC(ch) || 
	number_percent() < 0.6 * get_skill(ch,gsn_strangle) )
      {
	act("You grab hold of $N's neck and put $M to sleep.",
	    ch,NULL,victim,TO_CHAR);
	act("$n grabs hold of your neck and puts you to sleep.",
	    ch,NULL,victim,TO_VICT);
	act("$n grabs hold of $N's neck and puts $M to sleep.",
	    ch,NULL,victim,TO_NOTVICT);
	check_improve(ch,gsn_strangle,TRUE,1);
	
	af.type = gsn_strangle;
        af.where = TO_AFFECTS;
	af.level = ch->level;
	af.duration = ch->level / 20 + 1;
	af.location = APPLY_NONE;
	af.modifier = 0;
	af.bitvector = AFF_SLEEP;
	affect_join ( victim,&af );

	if (IS_AWAKE(victim))
	  victim->position = POS_SLEEPING;
      }
    else 
      {
	char buf[MAX_STRING_LENGTH];

	damage(ch,victim,0,gsn_strangle,DAM_NONE, TRUE);
	check_improve(ch,gsn_strangle,FALSE,1);
	if (!can_see(victim, ch))
	  do_yell(victim, (char*)"Help! I'm being strangled by someone!");
	else
	  {
	    snprintf(buf, sizeof(buf), "Help! I'm being strangled by %s!", ch->getName() );
	    if (!IS_NPC(victim)) do_yell(victim,buf);
	  }
      }
}

void do_blackjack(Character *ch, char *argument)
{
    Character *victim;
    AFFECT_DATA af;
    int chance;

    if ( IS_NPC(ch) ||
	 ch->level < skill_table[gsn_blackjack].skill_level[ch->class_num] )

    {
	send_to_char("Huh?\n\r",ch);
	return;
    }

    if ( (victim = get_char_room(ch,argument)) == NULL)
      {
	send_to_char("You do not see that person here.\n\r",ch);
	return;
      }

    if (ch==victim)
      {
	send_to_char("You idiot?! Blackjack your self?!\n\r",ch);
	return;
      }

    if ( IS_AFFECTED( ch, AFF_CHARM ) )  {
	send_to_char( "You don't want to hit your beloved masters' head with a full filled jack.\n\r",ch);
	return;
    } 

    if (IS_AFFECTED(victim,AFF_SLEEP))  {
      act("$E is already asleep.",ch,NULL,victim,TO_CHAR);
      return;
    }

    if (is_safe(ch,victim))
      return;

    WAIT_STATE(ch,skill_table[gsn_blackjack].beats);

    chance = 0.5 * get_skill(ch,gsn_blackjack);
    chance += URANGE( 0, (get_curr_stat(ch,STAT_DEX)-20)*2, 10);
    chance += can_see(victim, ch) ? 0 : 5;
    if ( IS_NPC(victim) )
	if ( victim->pIndexData->pShop != NULL )
	   chance -= 40;
 
    if (IS_NPC(ch) || 
	number_percent() < chance)
      {
	act("You hit $N's head with a lead filled sack.",
	    ch,NULL,victim,TO_CHAR);
	act("You feel a sudden pain erupts through your skull!",
	    ch,NULL,victim,TO_VICT);
	act("$n whacks $N at the back of $S head with a heavy looking sack!  *OUCH*",
	    ch,NULL,victim,TO_NOTVICT);
	check_improve(ch,gsn_blackjack,TRUE,1);
	
	af.type = gsn_blackjack;
        af.where = TO_AFFECTS;
	af.level = ch->level;
	af.duration = ch->level / 15 + 1;
	af.location = APPLY_NONE;
	af.modifier = 0;
	af.bitvector = AFF_SLEEP;
	affect_join ( victim,&af );

	if (IS_AWAKE(victim))
	  victim->position = POS_SLEEPING;
      }
    else 
      {
	char buf[MAX_STRING_LENGTH];

	damage(ch,victim,ch->level / 2,gsn_blackjack,DAM_NONE, TRUE);
	check_improve(ch,gsn_blackjack,FALSE,1);
	if (!IS_NPC(victim))  
	{
	if (!can_see(victim, ch))
	  do_yell(victim, (char*)"Help! I'm being blackjacked by someone!");
	else
	  {
	    snprintf(buf, sizeof(buf), "Help! I'm being blackjacked by %s!", ch->getName() );
	    if (!IS_NPC(victim)) do_yell(victim,buf);
	  }
	}
      }
}
