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
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <vector>
#include "merc.h"
#include "NonPlayerCharacter.h"
#include "Object.h"
#include "ObjectHelper.h"
#include "PlayerCharacter.h"
#include "Room.h"
#include "ShopKeeper.h"
#include "Wiznet.h"

/* command procedures needed */
DECLARE_SPELL_FUN(      spell_identify          );
DECLARE_DO_FUN(do_split		);
DECLARE_DO_FUN(do_yell		);
DECLARE_DO_FUN(do_say		);
DECLARE_DO_FUN(do_wake		);


using std::vector;

/*
 * Local functions.
 */
#define CD Character
#define OD Object
bool	remove_obj	args( (Character *ch, int iWear, bool fReplace ) );
void	wear_obj	args( (Character *ch, Object *obj, bool fReplace ) );
int	get_cost	args( (Character *keeper, Object *obj, bool fBuy ) );
void 	obj_to_keeper	args( (Object *obj, Character *ch ) );
OD *	get_obj_keeper	args( (Character *ch,Character *keeper,char *argument));

#undef OD
#undef	CD

/* RT part of the corpse looting code */

bool can_loot(Character *ch, Object *obj)
{
    Character *owner, *wch;

	if (ch->isImmortal()) { return true; }

    if (!obj->hasOwner()) { return true; }

    owner = NULL;
    for (std::list<Character*>::iterator it = char_list.begin(); it != char_list.end(); it++)
    {
    	wch = *it;
		if (ObjectHelper::isObjectOwnedBy(obj, wch)) {
            owner = wch;
		}
    }

    if (owner == NULL)
	return TRUE;

    if (!str_cmp(ch->getName().c_str(),owner->getName().c_str()))
	return TRUE;

    if (!IS_NPC(owner) && IS_SET(owner->act,PLR_CANLOOT))
	return TRUE;

    if (ch->isSameGroup(owner))
	return TRUE;

    return FALSE;
}


void get_obj( Character *ch, Object *obj, Object *container )
{
    /* variables for AUTOSPLIT */
    Character *gch;
    int members;
    char buffer[100];

    if ( !obj->canWear( ITEM_TAKE) )
    {
	send_to_char( "You can't take that.\n\r", ch );
	return;
    }

    if ( ch->carry_number + get_obj_number( obj ) > can_carry_n( ch ) )
    {
		act( "$d: you can't carry that many items.", ch, NULL, obj, TO_CHAR, POS_RESTING );
		return;
    }

    if ((!obj->getInObject() || obj->getInObject()->getCarriedBy() != ch)
    &&  (get_carry_weight(ch) + get_obj_weight(obj) > can_carry_w(ch)))
    {
		act( "$d: you can't carry that much weight.", ch, NULL, obj, TO_CHAR, POS_RESTING );
		return;
    }

    if (!can_loot(ch,obj))
    {
	act("Corpse looting is not permitted.",ch,NULL,NULL,TO_CHAR, POS_RESTING );
	return;
    }

    if (obj->getInRoom() != NULL)
    {
		for (gch = obj->getInRoom()->people; gch != NULL; gch = gch->next_in_room) {
			if (gch->onObject() == obj)
			{
				act("$N appears to be using $p.", ch,obj,gch,TO_CHAR, POS_RESTING);
				return;
			}
		}
    }
		

    if ( container != NULL )
    {
    	if (container->getObjectIndexData()->vnum == OBJ_VNUM_PIT
	&&  ch->getTrust() < obj->getLevel())
	{
	    send_to_char("You are not powerful enough to use it.\n\r",ch);
	    return;
	}

    	if (container->getObjectIndexData()->vnum == OBJ_VNUM_PIT
			&&  !container->canWear(ITEM_TAKE)
			&&  !obj->hasStat(ITEM_HAD_TIMER)) {
		    obj->setTimer(0);
		}
	act( "You get $p from $P.", ch, obj, container, TO_CHAR, POS_RESTING );
	act( "$n gets $p from $P.", ch, obj, container, TO_ROOM, POS_RESTING );
	obj->removeExtraFlag(ITEM_HAD_TIMER);
	obj_from_obj( obj );
    }
    else
    {
	act( "You get $p.", ch, obj, container, TO_CHAR, POS_RESTING );
	act( "$n gets $p.", ch, obj, container, TO_ROOM, POS_RESTING );
	obj_from_room( obj );
    }

    if ( obj->getItemType() == ITEM_MONEY)
    {
	ch->silver += obj->getValues().at(0);
	ch->gold += obj->getValues().at(1);
        if (IS_SET(ch->act,PLR_AUTOSPLIT))
        { /* AUTOSPLIT code */
    	  members = 0;
    	  for (gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room )
    	  {
            if (!IS_AFFECTED(gch,AFF_CHARM) && gch->isSameGroup( ch ) )
              members++;
    	  }

	  if ( members > 1 && (obj->getValues().at(0) > 1 || obj->getValues().at(1)))
	  {
	    snprintf(buffer, sizeof(buffer),"%d %d",obj->getValues().at(0),obj->getValues().at(1));
	    do_split(ch,buffer);	
	  }
        }
 
	extract_obj( obj );
    }
    else
    {
	ch->addObject( obj );
    }

    return;
}



void do_get( Character *ch, char *argument )
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    Object *obj;
    Object *obj_next;
    Object *container;
    bool found;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if (!str_cmp(arg2,"from"))
	argument = one_argument(argument,arg2);

    /* Get type. */
    if ( arg1[0] == '\0' )
    {
	send_to_char( "Get what?\n\r", ch );
	return;
    }

    if ( arg2[0] == '\0' )
    {
	if ( str_cmp( arg1, "all" ) && str_prefix( "all.", arg1 ) )
	{
	    /* 'get obj' */
	    obj = ObjectHelper::findInList( ch, arg1, ch->in_room->contents );
	    if ( obj == NULL )
	    {
		act( "I see no $T here.", ch, NULL, arg1, TO_CHAR, POS_RESTING );
		return;
	    }

	    get_obj( ch, obj, NULL );
	}
	else
	{
	    /* 'get all' or 'get all.obj' */
	    found = FALSE;
	    for ( auto obj : ch->in_room->contents )
	    {
			if ( ( arg1[3] == '\0' || is_name( &arg1[4], obj->getName().c_str() ) )
			&&   ch->can_see( obj ) )
			{
				found = TRUE;
				get_obj( ch, obj, NULL );
			}
			}

			if ( !found ) 
			{
			if ( arg1[3] == '\0' )
				send_to_char( "I see nothing here.\n\r", ch );
			else
				act( "I see no $T here.", ch, NULL, &arg1[4], TO_CHAR, POS_RESTING );
			}
		}
    }
    else
    {
	/* 'get ... container' */
	if ( !str_cmp( arg2, "all" ) || !str_prefix( "all.", arg2 ) )
	{
	    send_to_char( "You can't do that.\n\r", ch );
	    return;
	}

	if ( ( container = get_obj_here( ch, arg2 ) ) == NULL )
	{
	    act( "I see no $T here.", ch, NULL, arg2, TO_CHAR, POS_RESTING );
	    return;
	}

	switch ( container->getItemType() )
	{
	default:
	    send_to_char( "That's not a container.\n\r", ch );
	    return;

	case ITEM_CONTAINER:
	case ITEM_CORPSE_NPC:
	    break;

	case ITEM_CORPSE_PC:
	    {

		if (!can_loot(ch,container))
		{
		    send_to_char( "You can't do that.\n\r", ch );
		    return;
		}
	    }
	}

	if ( IS_SET(container->getValues().at(1), CONT_CLOSED) )
	{
	    act( "The $d is closed.", ch, NULL, container, TO_CHAR, POS_RESTING );
	    return;
	}

	if ( str_cmp( arg1, "all" ) && str_prefix( "all.", arg1 ) )
	{
	    /* 'get obj container' */
	    obj = ObjectHelper::findInList( ch, arg1, container->getContents() );
	    if ( obj == NULL )
	    {
		act( "I see nothing like that in the $T.",
		    ch, NULL, arg2, TO_CHAR, POS_RESTING );
		return;
	    }
	    get_obj( ch, obj, container );
	}
	else
	{
	    /* 'get all container' or 'get all.obj container' */
	    found = FALSE;
	    for ( auto obj : container->getContents() )
	    {
		if ( ( arg1[3] == '\0' || is_name( &arg1[4], obj->getName().c_str() ) )
		&&   ch->can_see( obj ) )
		{
		    found = TRUE;
		    if (container->getObjectIndexData()->vnum == OBJ_VNUM_PIT
		    &&  !IS_IMMORTAL(ch))
		    {
			send_to_char("Don't be so greedy!\n\r",ch);
			return;
		    }
		    get_obj( ch, obj, container );
		}
	    }

	    if ( !found )
	    {
		if ( arg1[3] == '\0' )
		    act( "I see nothing in the $T.",
			ch, NULL, arg2, TO_CHAR, POS_RESTING );
		else
		    act( "I see nothing like that in the $T.",
			ch, NULL, arg2, TO_CHAR, POS_RESTING );
	    }
	}
    }

    return;
}



void do_put( Character *ch, char *argument )
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    Object *container;
    Object *obj;
    Object *obj_next;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if (!str_cmp(arg2,"in") || !str_cmp(arg2,"on"))
	argument = one_argument(argument,arg2);

    if ( arg1[0] == '\0' || arg2[0] == '\0' )
    {
	send_to_char( "Put what in what?\n\r", ch );
	return;
    }

    if ( !str_cmp( arg2, "all" ) || !str_prefix( "all.", arg2 ) )
    {
	send_to_char( "You can't do that.\n\r", ch );
	return;
    }

    if ( ( container = get_obj_here( ch, arg2 ) ) == NULL )
    {
	act( "I see no $T here.", ch, NULL, arg2, TO_CHAR, POS_RESTING );
	return;
    }

    if ( container->getItemType() != ITEM_CONTAINER )
    {
	send_to_char( "That's not a container.\n\r", ch );
	return;
    }

    if ( IS_SET(container->getValues().at(1), CONT_CLOSED) )
    {
	act( "The $d is closed.", ch, NULL, container, TO_CHAR, POS_RESTING );
	return;
    }

    if ( str_cmp( arg1, "all" ) && str_prefix( "all.", arg1 ) )
    {
	/* 'put obj container' */
	if ( ( obj = get_obj_carry( ch, arg1, ch ) ) == NULL )
	{
	    send_to_char( "You do not have that item.\n\r", ch );
	    return;
	}

	if ( obj == container )
	{
	    send_to_char( "You can't fold it into itself.\n\r", ch );
	    return;
	}

	if ( !can_drop_obj( ch, obj ) )
	{
	    send_to_char( "You can't let go of it.\n\r", ch );
	    return;
	}

    	if (WEIGHT_MULT(obj) != 100)
    	{
           send_to_char("You have a feeling that would be a bad idea.\n\r",ch);
            return;
        }

	if (get_obj_weight( obj ) + get_true_weight( container )
	     > (container->getValues().at(0) * 10) 
	||  get_obj_weight(obj) > (container->getValues().at(3) * 10))
	{
	    send_to_char( "It won't fit.\n\r", ch );
	    return;
	}
	
	if (container->getObjectIndexData()->vnum == OBJ_VNUM_PIT 
	&&  !container->canWear(ITEM_TAKE))
	{
	    if (obj->getTimer() > 0) {
			obj->addExtraFlag(ITEM_HAD_TIMER);
		} else {
			obj->setTimer(number_range(100,200));
		}
	}

	obj_from_char( obj );
	container->addObject( obj );

	if (IS_SET(container->getValues().at(1),CONT_PUT_ON))
	{
	    act("$n puts $p on $P.",ch,obj,container, TO_ROOM, POS_RESTING);
	    act("You put $p on $P.",ch,obj,container, TO_CHAR, POS_RESTING);
	}
	else
	{
	    act( "$n puts $p in $P.", ch, obj, container, TO_ROOM, POS_RESTING );
	    act( "You put $p in $P.", ch, obj, container, TO_CHAR, POS_RESTING );
	}
    }
    else
    {
	/* 'put all container' or 'put all.obj container' */
	for ( auto obj : ch->getCarrying() )
	{
	    if ( ( arg1[3] == '\0' || is_name( &arg1[4], obj->getName().c_str() ) )
	    &&   ch->can_see( obj )
	    &&   WEIGHT_MULT(obj) == 100
	    &&   obj->getWearLocation() == WEAR_NONE
	    &&   obj != container
	    &&   can_drop_obj( ch, obj )
	    &&   get_obj_weight( obj ) + get_true_weight( container )
		 <= (container->getValues().at(0) * 10) 
	    &&   get_obj_weight(obj) < (container->getValues().at(3) * 10))
	    {
	    	if (container->getObjectIndexData()->vnum == OBJ_VNUM_PIT
	    	&&  !obj->canWear(ITEM_TAKE) )
		{
	    	    if (obj->getTimer() > 0) {
					obj->addExtraFlag(ITEM_HAD_TIMER);
				} else {
	    	    	obj->setTimer(number_range(100,200));
				}
		}
		obj_from_char( obj );
		container->addObject( obj );

        	if (IS_SET(container->getValues().at(1),CONT_PUT_ON))
        	{
            	    act("$n puts $p on $P.",ch,obj,container, TO_ROOM, POS_RESTING);
            	    act("You put $p on $P.",ch,obj,container, TO_CHAR, POS_RESTING);
        	}
		else
		{
		    act( "$n puts $p in $P.", ch, obj, container, TO_ROOM, POS_RESTING );
		    act( "You put $p in $P.", ch, obj, container, TO_CHAR, POS_RESTING );
		}
	    }
	}
    }

    return;
}



void do_drop( Character *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    Object *obj;
    Object *obj_next;
    bool found;

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	send_to_char( "Drop what?\n\r", ch );
	return;
    }

    if ( is_number( arg ) )
    {
	/* 'drop NNNN coins' */
	int amount, gold = 0, silver = 0;

	amount   = atoi(arg);
	argument = one_argument( argument, arg );
	if ( amount <= 0
	|| ( str_cmp( arg, "coins" ) && str_cmp( arg, "coin" ) && 
	     str_cmp( arg, "gold"  ) && str_cmp( arg, "silver") ) )
	{
	    send_to_char( "Sorry, you can't do that.\n\r", ch );
	    return;
	}

	if ( !str_cmp( arg, "coins") || !str_cmp(arg,"coin") 
	||   !str_cmp( arg, "silver"))
	{
	    if (ch->silver < amount)
	    {
		send_to_char("You don't have that much silver.\n\r",ch);
		return;
	    }

	    ch->silver -= amount;
	    silver = amount;
	}

	else
	{
	    if (ch->gold < amount)
	    {
		send_to_char("You don't have that much gold.\n\r",ch);
		return;
	    }

	    ch->gold -= amount;
  	    gold = amount;
	}

	for ( auto obj : ch->in_room->contents )
	{
	    switch ( obj->getObjectIndexData()->vnum )
	    {
	    case OBJ_VNUM_SILVER_ONE:
		silver += 1;
		extract_obj(obj);
		break;

	    case OBJ_VNUM_GOLD_ONE:
		gold += 1;
		extract_obj( obj );
		break;

	    case OBJ_VNUM_SILVER_SOME:
		silver += obj->getValues().at(0);
		extract_obj(obj);
		break;

	    case OBJ_VNUM_GOLD_SOME:
		gold += obj->getValues().at(1);
		extract_obj( obj );
		break;

	    case OBJ_VNUM_COINS:
		silver += obj->getValues().at(0);
		gold += obj->getValues().at(1);
		extract_obj(obj);
		break;
	    }
	}

	obj_to_room( create_money( gold, silver ), ch->in_room );
	act( "$n drops some coins.", ch, NULL, NULL, TO_ROOM, POS_RESTING );
	send_to_char( "OK.\n\r", ch );
	return;
    }

    if ( str_cmp( arg, "all" ) && str_prefix( "all.", arg ) )
    {
	/* 'drop obj' */
	if ( ( obj = get_obj_carry( ch, arg, ch ) ) == NULL )
	{
	    send_to_char( "You do not have that item.\n\r", ch );
	    return;
	}

	if ( !can_drop_obj( ch, obj ) )
	{
	    send_to_char( "You can't let go of it.\n\r", ch );
	    return;
	}

	obj_from_char( obj );
	obj_to_room( obj, ch->in_room );
	act( "$n drops $p.", ch, obj, NULL, TO_ROOM, POS_RESTING );
	act( "You drop $p.", ch, obj, NULL, TO_CHAR, POS_RESTING );
	if (obj->hasStat(ITEM_MELT_DROP))
	{
	    act("$p dissolves into smoke.",ch,obj,NULL,TO_ROOM,POS_RESTING);
	    act("$p dissolves into smoke.",ch,obj,NULL,TO_CHAR,POS_RESTING);
	    extract_obj(obj);
	}
    }
    else
    {
	/* 'drop all' or 'drop all.obj' */
	found = FALSE;
	for ( auto obj : ch->getCarrying() )
	{
	    if ( ( arg[3] == '\0' || is_name( &arg[4], obj->getName().c_str() ) )
	    &&   ch->can_see( obj )
	    &&   obj->getWearLocation() == WEAR_NONE
	    &&   can_drop_obj( ch, obj ) )
	    {
		found = TRUE;
		obj_from_char( obj );
		obj_to_room( obj, ch->in_room );
		act( "$n drops $p.", ch, obj, NULL, TO_ROOM, POS_RESTING );
		act( "You drop $p.", ch, obj, NULL, TO_CHAR, POS_RESTING );
        	if (obj->hasStat(ITEM_MELT_DROP))
        	{
             	    act("$p dissolves into smoke.",ch,obj,NULL,TO_ROOM,POS_RESTING);
            	    act("$p dissolves into smoke.",ch,obj,NULL,TO_CHAR,POS_RESTING);
            	    extract_obj(obj);
        	}
	    }
	}

	if ( !found )
	{
	    if ( arg[3] == '\0' )
		act( "You are not carrying anything.",
		    ch, NULL, arg, TO_CHAR, POS_RESTING );
	    else
		act( "You are not carrying any $T.",
		    ch, NULL, &arg[4], TO_CHAR, POS_RESTING );
	}
    }

    return;
}



void do_give( Character *ch, char *argument )
{
    char arg1 [MAX_INPUT_LENGTH];
    char arg2 [MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    Character *victim;
    Object  *obj;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( arg1[0] == '\0' || arg2[0] == '\0' )
    {
	send_to_char( "Give what to whom?\n\r", ch );
	return;
    }

    if ( is_number( arg1 ) )
    {
	/* 'give NNNN coins victim' */
	int amount;
	bool silver;

	amount   = atoi(arg1);
	if ( amount <= 0
	|| ( str_cmp( arg2, "coins" ) && str_cmp( arg2, "coin" ) && 
	     str_cmp( arg2, "gold"  ) && str_cmp( arg2, "silver")) )
	{
	    send_to_char( "Sorry, you can't do that.\n\r", ch );
	    return;
	}

	silver = str_cmp(arg2,"gold");

	argument = one_argument( argument, arg2 );
	if ( arg2[0] == '\0' )
	{
	    send_to_char( "Give what to whom?\n\r", ch );
	    return;
	}

	if ( ( victim = get_char_room( ch, arg2 ) ) == NULL )
	{
	    send_to_char( "They aren't here.\n\r", ch );
	    return;
	}

	if ( (!silver && ch->gold < amount) || (silver && ch->silver < amount) )
	{
	    send_to_char( "You haven't got that much.\n\r", ch );
	    return;
	}

	if (silver)
	{
	    ch->silver		-= amount;
	    victim->silver 	+= amount;
	}
	else
	{
	    ch->gold		-= amount;
	    victim->gold	+= amount;
	}

	snprintf(buf, sizeof(buf),"$n gives you %d %s.",amount, silver ? "silver" : "gold");
	act( buf, ch, NULL, victim, TO_VICT, POS_RESTING    );
	act( "$n gives $N some coins.",  ch, NULL, victim, TO_NOTVICT, POS_RESTING );
	snprintf(buf, sizeof(buf),"You give $N %d %s.",amount, silver ? "silver" : "gold");
	act( buf, ch, NULL, victim, TO_CHAR, POS_RESTING    );

	/*
	 * Bribe trigger
	 */
	if ( IS_NPC(victim) && HAS_TRIGGER( victim, TRIG_BRIBE ) )
	    mp_bribe_trigger( victim, ch, silver ? amount : amount * 100 );

	if (IS_NPC(victim) && IS_SET(victim->act,ACT_IS_CHANGER))
	{
	    int change;

	    change = (silver ? 95 * amount / 100 / 100 
		 	     : 95 * amount);


	    if (!silver && change > victim->silver)
	    	victim->silver += change;

	    if (silver && change > victim->gold)
		victim->gold += change;

	    if (change < 1 && victim->can_see(ch))
	    {
		act(
	"$n tells you 'I'm sorry, you did not give me enough to change.'"
		    ,victim,NULL,ch,TO_VICT, POS_RESTING);
		ch->reply = victim;
		snprintf(buf, sizeof(buf),"%d %s %s", 
			amount, silver ? "silver" : "gold",ch->getName().c_str());
		do_give(victim,buf);
	    }
	    else if (victim->can_see(ch))
	    {
		snprintf(buf, sizeof(buf),"%d %s %s", 
			change, silver ? "gold" : "silver",ch->getName().c_str());
		do_give(victim,buf);
		if (silver)
		{
		    snprintf(buf, sizeof(buf),"%d silver %s", 
			(95 * amount / 100 - change * 100),ch->getName().c_str());
		    do_give(victim,buf);
		}
		act("$n tells you 'Thank you, come again.'",
		    victim,NULL,ch,TO_VICT, POS_RESTING);
		ch->reply = victim;
	    }
	}
	return;
    }

    if ( ( obj = get_obj_carry( ch, arg1, ch ) ) == NULL )
    {
	send_to_char( "You do not have that item.\n\r", ch );
	return;
    }

    if ( obj->getWearLocation() != WEAR_NONE )
    {
	send_to_char( "You must remove it first.\n\r", ch );
	return;
    }

    if ( ( victim = get_char_room( ch, arg2 ) ) == NULL )
    {
	send_to_char( "They aren't here.\n\r", ch );
	return;
    }

    if (IS_NPC(victim) && victim->pIndexData->pShop != NULL)
    {
	act("$N tells you 'Sorry, you'll have to sell that.'",
	    ch,NULL,victim,TO_CHAR, POS_RESTING);
	ch->reply = victim;
	return;
    }

    if ( !can_drop_obj( ch, obj ) )
    {
	send_to_char( "You can't let go of it.\n\r", ch );
	return;
    }

    if ( victim->carry_number + get_obj_number( obj ) > can_carry_n( victim ) )
    {
	act( "$N has $S hands full.", ch, NULL, victim, TO_CHAR, POS_RESTING );
	return;
    }

    if (get_carry_weight(victim) + get_obj_weight(obj) > can_carry_w( victim ) )
    {
	act( "$N can't carry that much weight.", ch, NULL, victim, TO_CHAR, POS_RESTING );
	return;
    }

    if ( !victim->can_see( obj ) )
    {
	act( "$N can't see it.", ch, NULL, victim, TO_CHAR, POS_RESTING );
	return;
    }

    obj_from_char( obj );
    victim->addObject( obj );
    MOBtrigger = FALSE;
    act( "$n gives $p to $N.", ch, obj, victim, TO_NOTVICT, POS_RESTING );
    act( "$n gives you $p.",   ch, obj, victim, TO_VICT, 	POS_RESTING    );
    act( "You give $p to $N.", ch, obj, victim, TO_CHAR, 	POS_RESTING    );
    MOBtrigger = TRUE;

    /*
     * Give trigger
     */
    if ( IS_NPC(victim) && HAS_TRIGGER( victim, TRIG_GIVE ) )
	mp_give_trigger( victim, ch, obj );

    return;
}


/* for poisoning weapons and food/drink */
void do_envenom(Character *ch, char *argument)
{
    Object *obj;
    AFFECT_DATA af;
    int percent,skill;

    /* find out what */
    if (NULL == argument || argument[0] == '\0')
    {
	send_to_char("Envenom what item?\n\r",ch);
	return;
    }

    obj =  ObjectHelper::findInList(ch,argument,ch->getCarrying());

    if (obj== NULL)
    {
	send_to_char("You don't have that item.\n\r",ch);
	return;
    }

    if ((skill = get_skill(ch,gsn_envenom)) < 1)
    {
	send_to_char("Are you crazy? You'd poison yourself!\n\r",ch);
	return;
    }

    if (obj->getItemType() == ITEM_FOOD || obj->getItemType() == ITEM_DRINK_CON)
    {
	if (obj->hasStat(ITEM_BLESS) || obj->hasStat(ITEM_BURN_PROOF))
	{
	    act("You fail to poison $p.",ch,obj,NULL,TO_CHAR, POS_RESTING);
	    return;
	}

	if (number_percent() < skill)  /* success! */
	{
	    act("$n treats $p with deadly poison.",ch,obj,NULL,TO_ROOM, POS_RESTING);
	    act("You treat $p with deadly poison.",ch,obj,NULL,TO_CHAR, POS_RESTING);
	    if (!obj->getValues().at(3))
	    {
		obj->getValues().at(3) = 1;
		check_improve(ch,gsn_envenom,TRUE,4);
	    }
	    WAIT_STATE(ch,skill_table[gsn_envenom].beats);
	    return;
	}

	act("You fail to poison $p.",ch,obj,NULL,TO_CHAR, POS_RESTING);
	if (!obj->getValues().at(3))
	    check_improve(ch,gsn_envenom,FALSE,4);
	WAIT_STATE(ch,skill_table[gsn_envenom].beats);
	return;
     }

    if (obj->getItemType() == ITEM_WEAPON)
    {
        if (IS_WEAPON_STAT(obj,WEAPON_FLAMING)
        ||  IS_WEAPON_STAT(obj,WEAPON_FROST)
        ||  IS_WEAPON_STAT(obj,WEAPON_VAMPIRIC)
        ||  IS_WEAPON_STAT(obj,WEAPON_SHARP)
        ||  IS_WEAPON_STAT(obj,WEAPON_VORPAL)
        ||  IS_WEAPON_STAT(obj,WEAPON_SHOCKING)
        ||  obj->hasStat(ITEM_BLESS) || obj->hasStat(ITEM_BURN_PROOF))
        {
            act("You can't seem to envenom $p.",ch,obj,NULL,TO_CHAR, POS_RESTING);
            return;
        }

	if (obj->getValues().at(3) < 0 
	||  attack_table[obj->getValues().at(3)].damage == DAM_BASH)
	{
	    send_to_char("You can only envenom edged weapons.\n\r",ch);
	    return;
	}

        if (IS_WEAPON_STAT(obj,WEAPON_POISON))
        {
            act("$p is already envenomed.",ch,obj,NULL,TO_CHAR, POS_RESTING);
            return;
        }

	percent = number_percent();
	if (percent < skill)
	{
 
            af.where     = TO_WEAPON;
            af.type      = gsn_poison;
            af.level     = ch->level * percent / 100;
            af.duration  = ch->level/2 * percent / 100;
            af.location  = 0;
            af.modifier  = 0;
            af.bitvector = WEAPON_POISON;
			obj->giveAffect(&af);
 
            act("$n coats $p with deadly venom.",ch,obj,NULL,TO_ROOM, POS_RESTING);
	    act("You coat $p with venom.",ch,obj,NULL,TO_CHAR, POS_RESTING);
	    check_improve(ch,gsn_envenom,TRUE,3);
	    WAIT_STATE(ch,skill_table[gsn_envenom].beats);
            return;
        }
	else
	{
	    act("You fail to envenom $p.",ch,obj,NULL,TO_CHAR, POS_RESTING);
	    check_improve(ch,gsn_envenom,FALSE,3);
	    WAIT_STATE(ch,skill_table[gsn_envenom].beats);
	    return;
	}
    }
 
    act("You can't poison $p.",ch,obj,NULL,TO_CHAR, POS_RESTING);
    return;
}

void do_fill( Character *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    Object *obj;
    bool found;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	send_to_char( "Fill what?\n\r", ch );
	return;
    }

    if ( ( obj = get_obj_carry( ch, arg, ch ) ) == NULL )
    {
	send_to_char( "You do not have that item.\n\r", ch );
	return;
    }

    found = FALSE;
	Object *fountain = ObjectHelper::getFountainFromList(vector<Object *>(ch->in_room->contents.begin(), ch->in_room->contents.end()));

    if ( !fountain )
    {
		send_to_char( "There is no fountain here!\n\r", ch );
		return;
    }

    if ( obj->getItemType() != ITEM_DRINK_CON )
    {
		send_to_char( "You can't fill that.\n\r", ch );
		return;
    }

    if ( obj->getValues().at(1) != 0 && obj->getValues().at(2) != fountain->getValues().at(2) )
    {
		send_to_char( "There is already another liquid in it.\n\r", ch );
		return;
    }

    if ( obj->getValues().at(1) >= obj->getValues().at(0) )
    {
	send_to_char( "Your container is full.\n\r", ch );
	return;
    }

    snprintf(buf, sizeof(buf),"You fill $p with %s from $P.",
	liq_table[fountain->getValues().at(2)].liq_name);
    act( buf, ch, obj,fountain, TO_CHAR, POS_RESTING );
    snprintf(buf, sizeof(buf),"$n fills $p with %s from $P.",
	liq_table[fountain->getValues().at(2)].liq_name);
    act(buf,ch,obj,fountain,TO_ROOM, POS_RESTING);
    obj->getValues().at(2) = fountain->getValues().at(2);
    obj->getValues().at(1) = obj->getValues().at(0);
    return;
}

void do_pour (Character *ch, char *argument)
{
    char arg[MAX_STRING_LENGTH],buf[MAX_STRING_LENGTH];
    Object *out, *in;
    Character *vch = NULL;
    int amount;

    argument = one_argument(argument,arg);
    
    if (arg[0] == '\0' || argument[0] == '\0')
    {
	send_to_char("Pour what into what?\n\r",ch);
	return;
    }
    

    if ((out = get_obj_carry(ch,arg, ch)) == NULL)
    {
	send_to_char("You don't have that item.\n\r",ch);
	return;
    }

    if (out->getItemType() != ITEM_DRINK_CON)
    {
	send_to_char("That's not a drink container.\n\r",ch);
	return;
    }

    if (!str_cmp(argument,"out"))
    {
	if (out->getValues().at(1) == 0)
	{
	    send_to_char("It's already empty.\n\r",ch);
	    return;
	}

	out->getValues().at(1) = 0;
	out->getValues().at(3) = 0;
	snprintf(buf, sizeof(buf),"You invert $p, spilling %s all over the ground.",
		liq_table[out->getValues().at(2)].liq_name);
	act(buf,ch,out,NULL,TO_CHAR, POS_RESTING);
	
	snprintf(buf, sizeof(buf),"$n inverts $p, spilling %s all over the ground.",
		liq_table[out->getValues().at(2)].liq_name);
	act(buf,ch,out,NULL,TO_ROOM, POS_RESTING);
	return;
    }

    if ((in = get_obj_here(ch,argument)) == NULL)
    {
	vch = get_char_room(ch,argument);

	if (vch == NULL)
	{
	    send_to_char("Pour into what?\n\r",ch);
	    return;
	}

	in = vch->getEquipment(WEAR_HOLD);

	if (in == NULL)
	{
	    send_to_char("They aren't holding anything.",ch);
 	    return;
	}
    }

    if (in->getItemType() != ITEM_DRINK_CON)
    {
	send_to_char("You can only pour into other drink containers.\n\r",ch);
	return;
    }
    
    if (in == out)
    {
	send_to_char("You cannot change the laws of physics!\n\r",ch);
	return;
    }

    if (in->getValues().at(1) != 0 && in->getValues().at(2) != out->getValues().at(2))
    {
	send_to_char("They don't hold the same liquid.\n\r",ch);
	return;
    }

    if (out->getValues().at(1) == 0)
    {
	act("There's nothing in $p to pour.",ch,out,NULL,TO_CHAR, POS_RESTING);
	return;
    }

    if (in->getValues().at(1) >= in->getValues().at(0))
    {
	act("$p is already filled to the top.",ch,in,NULL,TO_CHAR, POS_RESTING);
	return;
    }

    amount = UMIN(out->getValues().at(1),in->getValues().at(0) - in->getValues().at(1));

    in->getValues().at(1) += amount;
    out->getValues().at(1) -= amount;
    in->getValues().at(2) = out->getValues().at(2);
    
    if (vch == NULL)
    {
    	snprintf(buf, sizeof(buf),"You pour %s from $p into $P.",
	    liq_table[out->getValues().at(2)].liq_name);
    	act(buf,ch,out,in,TO_CHAR, POS_RESTING);
    	snprintf(buf, sizeof(buf),"$n pours %s from $p into $P.",
	    liq_table[out->getValues().at(2)].liq_name);
    	act(buf,ch,out,in,TO_ROOM, POS_RESTING);
    }
    else
    {
        snprintf(buf, sizeof(buf),"You pour some %s for $N.",
            liq_table[out->getValues().at(2)].liq_name);
        act(buf,ch,NULL,vch,TO_CHAR, POS_RESTING);
	snprintf(buf, sizeof(buf),"$n pours you some %s.",
	    liq_table[out->getValues().at(2)].liq_name);
	act(buf,ch,NULL,vch,TO_VICT, POS_RESTING);
        snprintf(buf, sizeof(buf),"$n pours some %s for $N.",
            liq_table[out->getValues().at(2)].liq_name);
        act(buf,ch,NULL,vch,TO_NOTVICT, POS_RESTING);
	
    }
}

void do_drink( Character *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    Object *obj;
    int amount;
    int liquid;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
		for ( auto o : ch->in_room->contents )
		{
			if ( o->getItemType() == ITEM_FOUNTAIN ) {
				obj = o;
				break;
			}
		}

		if ( obj == NULL )
		{
			send_to_char( "Drink what?\n\r", ch );
			return;
		}
    }
    else
    {
		if ( ( obj = get_obj_here( ch, arg ) ) == NULL )
		{
			send_to_char( "You can't find it.\n\r", ch );
			return;
		}
    }

    if ( !ch->isNPC() && ((PlayerCharacter*)ch)->condition[COND_DRUNK] > 10 )
    {
	send_to_char( "You fail to reach your mouth.  *Hic*\n\r", ch );
	return;
    }

    switch ( obj->getItemType() )
    {
    default:
	send_to_char( "You can't drink from that.\n\r", ch );
	return;

    case ITEM_FOUNTAIN:
        if ( ( liquid = obj->getValues().at(2) )  < 0 )
        {
            bug( "Do_drink: bad liquid number %d.", liquid );
            liquid = obj->getValues().at(2) = 0;
        }
	amount = liq_table[liquid].liq_affect[4] * 3;
	break;

    case ITEM_DRINK_CON:
	if ( obj->getValues().at(1) <= 0 )
	{
	    send_to_char( "It is already empty.\n\r", ch );
	    return;
	}

	if ( ( liquid = obj->getValues().at(2) )  < 0 )
	{
	    bug( "Do_drink: bad liquid number %d.", liquid );
	    liquid = obj->getValues().at(2) = 0;
	}

        amount = liq_table[liquid].liq_affect[4];
        amount = UMIN(amount, obj->getValues().at(1));
	break;
     }
    if (!ch->isNPC() && !IS_IMMORTAL(ch) 
    &&  ((PlayerCharacter*)ch)->condition[COND_FULL] > 45)
    {
	send_to_char("You're too full to drink more.\n\r",ch);
	return;
    }

    act( "$n drinks $T from $p.",
	ch, obj, liq_table[liquid].liq_name, TO_ROOM, POS_RESTING );
    act( "You drink $T from $p.",
	ch, obj, liq_table[liquid].liq_name, TO_CHAR, POS_RESTING );

    ch->gain_condition( COND_DRUNK, amount * liq_table[liquid].liq_affect[COND_DRUNK] / 36 );
    ch->gain_condition( COND_FULL, amount * liq_table[liquid].liq_affect[COND_FULL] / 4 );
    ch->gain_condition( COND_THIRST, amount * liq_table[liquid].liq_affect[COND_THIRST] / 10 );
    ch->gain_condition( COND_HUNGER, amount * liq_table[liquid].liq_affect[COND_HUNGER] / 2 );

    if ( !ch->isNPC() && ((PlayerCharacter*)ch)->condition[COND_DRUNK]  > 10 )
	send_to_char( "You feel drunk.\n\r", ch );
    if ( !ch->isNPC() && ((PlayerCharacter*)ch)->condition[COND_FULL]   > 40 )
	send_to_char( "You are full.\n\r", ch );
    if ( !ch->isNPC() && ((PlayerCharacter*)ch)->condition[COND_THIRST] > 40 )
	send_to_char( "Your thirst is quenched.\n\r", ch );
	
    if ( obj->getValues().at(3) != 0 )
    {
	/* The drink was poisoned ! */
	AFFECT_DATA af;

	act( "$n chokes and gags.", ch, NULL, NULL, TO_ROOM, POS_RESTING );
	send_to_char( "You choke and gag.\n\r", ch );
	af.where     = TO_AFFECTS;
	af.type      = gsn_poison;
	af.level	 = number_fuzzy(amount); 
	af.duration  = 3 * amount;
	af.location  = APPLY_NONE;
	af.modifier  = 0;
	af.bitvector = AFF_POISON;
	affect_join( ch, &af );
    }
	
    if (obj->getValues().at(0) > 0)
        obj->getValues().at(1) -= amount;

    return;
}



void do_eat( Character *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    Object *obj;

    one_argument( argument, arg );
    if ( arg[0] == '\0' )
    {
	send_to_char( "Eat what?\n\r", ch );
	return;
    }

    if ( ( obj = get_obj_carry( ch, arg, ch ) ) == NULL )
    {
	send_to_char( "You do not have that item.\n\r", ch );
	return;
    }

    if ( !IS_IMMORTAL(ch) )
    {
	if ( obj->getItemType() != ITEM_FOOD && obj->getItemType() != ITEM_PILL )
	{
	    send_to_char( "That's not edible.\n\r", ch );
	    return;
	}

	if ( !ch->isNPC() && ((PlayerCharacter*)ch)->condition[COND_FULL] > 40 )
	{   
	    send_to_char( "You are too full to eat more.\n\r", ch );
	    return;
	}
    }

    act( "$n eats $p.",  ch, obj, NULL, TO_ROOM, POS_RESTING );
    act( "You eat $p.", ch, obj, NULL, TO_CHAR, POS_RESTING );

    switch ( obj->getItemType() )
    {

    case ITEM_FOOD:
	if ( !ch->isNPC() )
	{
	    int condition;

	    condition = ((PlayerCharacter*)ch)->condition[COND_HUNGER];
	    ch->gain_condition( COND_FULL, obj->getValues().at(0) );
	    ch->gain_condition( COND_HUNGER, obj->getValues().at(1));
	    if ( condition == 0 && ((PlayerCharacter*)ch)->condition[COND_HUNGER] > 0 )
		send_to_char( "You are no longer hungry.\n\r", ch );
	    else if ( ((PlayerCharacter*)ch)->condition[COND_FULL] > 40 )
		send_to_char( "You are full.\n\r", ch );
	}

	if ( obj->getValues().at(3) != 0 )
	{
	    /* The food was poisoned! */
	    AFFECT_DATA af;

	    act( "$n chokes and gags.", ch, 0, 0, TO_ROOM, POS_RESTING );
	    send_to_char( "You choke and gag.\n\r", ch );

	    af.where	 = TO_AFFECTS;
	    af.type      = gsn_poison;
	    af.level 	 = number_fuzzy(obj->getValues().at(0));
	    af.duration  = 2 * obj->getValues().at(0);
	    af.location  = APPLY_NONE;
	    af.modifier  = 0;
	    af.bitvector = AFF_POISON;
	    affect_join( ch, &af );
	}
	break;

    case ITEM_PILL:
	obj_cast_spell( obj->getValues().at(1), obj->getValues().at(0), true, ch, ch, NULL );
	obj_cast_spell( obj->getValues().at(2), obj->getValues().at(0), true, ch, ch, NULL );
	obj_cast_spell( obj->getValues().at(3), obj->getValues().at(0), true, ch, ch, NULL );
	break;
    }

    extract_obj( obj );
    return;
}



/*
 * Remove an object.
 */
bool remove_obj( Character *ch, int iWear, bool fReplace )
{
    Object *obj;

    if ( ( obj = ch->getEquipment(iWear ) ) == NULL )
	return TRUE;

    if ( !fReplace )
	return FALSE;

    if ( IS_SET(obj->getExtraFlags(), ITEM_NOREMOVE) )
    {
	act( "You can't remove $p.", ch, obj, NULL, TO_CHAR, POS_RESTING );
	return FALSE;
    }

    ch->unequip( obj );
    act( "$n stops using $p.", ch, obj, NULL, TO_ROOM, POS_RESTING );
    act( "You stop using $p.", ch, obj, NULL, TO_CHAR, POS_RESTING );
    return TRUE;
}



/*
 * Wear one object.
 * Optional replacement of existing objects.
 * Big repetitive code, ick.
 */
void wear_obj( Character *ch, Object *obj, bool fReplace )
{
    char buf[MAX_STRING_LENGTH];

    if ( ch->level < obj->getLevel() )
    {
	snprintf(buf, sizeof(buf), "You must be level %d to use this object.\n\r",
	    obj->getLevel() );
	send_to_char( buf, ch );
	act( "$n tries to use $p, but is too inexperienced.", ch, obj, NULL, TO_ROOM, POS_RESTING );
	return;
    }

    if ( obj->getItemType() == ITEM_LIGHT )
    {
	if ( !remove_obj( ch, WEAR_LIGHT, fReplace ) )
	    return;
	act( "$n lights $p and holds it.", ch, obj, NULL, TO_ROOM, POS_RESTING );
	act( "You light $p and hold it.",  ch, obj, NULL, TO_CHAR, POS_RESTING );
	equip_char( ch, obj, WEAR_LIGHT );
	return;
    }

    if ( obj->canWear( ITEM_WEAR_FINGER ) )
    {
	if ( ch->getEquipment(WEAR_FINGER_L ) != NULL
	&&   ch->getEquipment(WEAR_FINGER_R ) != NULL
	&&   !remove_obj( ch, WEAR_FINGER_L, fReplace )
	&&   !remove_obj( ch, WEAR_FINGER_R, fReplace ) )
	    return;

	if ( ch->getEquipment(WEAR_FINGER_L ) == NULL )
	{
	    act( "$n wears $p on $s left finger.",    ch, obj, NULL, TO_ROOM, POS_RESTING );
	    act( "You wear $p on your left finger.",  ch, obj, NULL, TO_CHAR, POS_RESTING );
	    equip_char( ch, obj, WEAR_FINGER_L );
	    return;
	}

	if ( ch->getEquipment(WEAR_FINGER_R ) == NULL )
	{
	    act( "$n wears $p on $s right finger.",   ch, obj, NULL, TO_ROOM, POS_RESTING );
	    act( "You wear $p on your right finger.", ch, obj, NULL, TO_CHAR, POS_RESTING );
	    equip_char( ch, obj, WEAR_FINGER_R );
	    return;
	}

	bug( "Wear_obj: no free finger.", 0 );
	send_to_char( "You already wear two rings.\n\r", ch );
	return;
    }

    if ( obj->canWear( ITEM_WEAR_NECK ) )
    {
	if ( ch->getEquipment(WEAR_NECK_1 ) != NULL
	&&   ch->getEquipment(WEAR_NECK_2 ) != NULL
	&&   !remove_obj( ch, WEAR_NECK_1, fReplace )
	&&   !remove_obj( ch, WEAR_NECK_2, fReplace ) )
	    return;

	if ( ch->getEquipment(WEAR_NECK_1 ) == NULL )
	{
	    act( "$n wears $p around $s neck.",   ch, obj, NULL, TO_ROOM, POS_RESTING );
	    act( "You wear $p around your neck.", ch, obj, NULL, TO_CHAR, POS_RESTING );
	    equip_char( ch, obj, WEAR_NECK_1 );
	    return;
	}

	if ( ch->getEquipment(WEAR_NECK_2 ) == NULL )
	{
	    act( "$n wears $p around $s neck.",   ch, obj, NULL, TO_ROOM, POS_RESTING );
	    act( "You wear $p around your neck.", ch, obj, NULL, TO_CHAR, POS_RESTING );
	    equip_char( ch, obj, WEAR_NECK_2 );
	    return;
	}

	bug( "Wear_obj: no free neck.", 0 );
	send_to_char( "You already wear two neck items.\n\r", ch );
	return;
    }

    if ( obj->canWear( ITEM_WEAR_BODY ) )
    {
	if ( !remove_obj( ch, WEAR_BODY, fReplace ) )
	    return;
	act( "$n wears $p on $s torso.",   ch, obj, NULL, TO_ROOM, POS_RESTING );
	act( "You wear $p on your torso.", ch, obj, NULL, TO_CHAR, POS_RESTING );
	equip_char( ch, obj, WEAR_BODY );
	return;
    }

    if ( obj->canWear( ITEM_WEAR_HEAD ) )
    {
	if ( !remove_obj( ch, WEAR_HEAD, fReplace ) )
	    return;
	act( "$n wears $p on $s head.",   ch, obj, NULL, TO_ROOM, POS_RESTING );
	act( "You wear $p on your head.", ch, obj, NULL, TO_CHAR, POS_RESTING );
	equip_char( ch, obj, WEAR_HEAD );
	return;
    }

    if ( obj->canWear( ITEM_WEAR_BACK ) )
    {
	if ( !remove_obj( ch, WEAR_BACK, fReplace ) )
	    return;
	act( "$n wears $p on $s back.",   ch, obj, NULL, TO_ROOM, POS_RESTING );
	act( "You wear $p on your back.", ch, obj, NULL, TO_CHAR, POS_RESTING );
	equip_char( ch, obj, WEAR_BACK );
	return;
    }

    if ( obj->canWear( ITEM_WEAR_LEGS ) )
    {
	if ( !remove_obj( ch, WEAR_LEGS, fReplace ) )
	    return;
	act( "$n wears $p on $s legs.",   ch, obj, NULL, TO_ROOM, POS_RESTING );
	act( "You wear $p on your legs.", ch, obj, NULL, TO_CHAR, POS_RESTING );
	equip_char( ch, obj, WEAR_LEGS );
	return;
    }

    if ( obj->canWear( ITEM_WEAR_FEET ) )
    {
	if ( !remove_obj( ch, WEAR_FEET, fReplace ) )
	    return;
	act( "$n wears $p on $s feet.",   ch, obj, NULL, TO_ROOM, POS_RESTING );
	act( "You wear $p on your feet.", ch, obj, NULL, TO_CHAR, POS_RESTING );
	equip_char( ch, obj, WEAR_FEET );
	return;
    }

    if ( obj->canWear( ITEM_WEAR_HANDS ) )
    {
	if ( !remove_obj( ch, WEAR_HANDS, fReplace ) )
	    return;
	act( "$n wears $p on $s hands.",   ch, obj, NULL, TO_ROOM, POS_RESTING );
	act( "You wear $p on your hands.", ch, obj, NULL, TO_CHAR, POS_RESTING );
	equip_char( ch, obj, WEAR_HANDS );
	return;
    }

    if ( obj->canWear( ITEM_WEAR_ARMS ) )
    {
	if ( !remove_obj( ch, WEAR_ARMS, fReplace ) )
	    return;
	act( "$n wears $p on $s arms.",   ch, obj, NULL, TO_ROOM, POS_RESTING );
	act( "You wear $p on your arms.", ch, obj, NULL, TO_CHAR, POS_RESTING );
	equip_char( ch, obj, WEAR_ARMS );
	return;
    }

    if ( obj->canWear( ITEM_WEAR_BOMB ) )
    {
        if ( !remove_obj( ch, WEAR_BOMB, fReplace ) )
            return;
        act( "$n wears $p as a bomb.",   ch, obj, NULL, TO_ROOM, POS_RESTING );
        act( "You wear $p as a bomb.", ch, obj, NULL, TO_CHAR, POS_RESTING );
        equip_char( ch, obj, WEAR_BOMB );
        return;
    }

    if ( obj->canWear( ITEM_WEAR_ABOUT ) )
    {
	if ( !remove_obj( ch, WEAR_ABOUT, fReplace ) )
	    return;
	act( "$n wears $p about $s torso.",   ch, obj, NULL, TO_ROOM, POS_RESTING );
	act( "You wear $p about your torso.", ch, obj, NULL, TO_CHAR, POS_RESTING );
	equip_char( ch, obj, WEAR_ABOUT );
	return;
    }

    if ( obj->canWear( ITEM_WEAR_WAIST ) )
    {
	if ( !remove_obj( ch, WEAR_WAIST, fReplace ) )
	    return;
	act( "$n wears $p about $s waist.",   ch, obj, NULL, TO_ROOM, POS_RESTING );
	act( "You wear $p about your waist.", ch, obj, NULL, TO_CHAR, POS_RESTING );
	equip_char( ch, obj, WEAR_WAIST );
	return;
    }

    if ( obj->canWear( ITEM_WEAR_WRIST ) )
    {
	if ( ch->getEquipment(WEAR_WRIST_L ) != NULL
	&&   ch->getEquipment(WEAR_WRIST_R ) != NULL
	&&   !remove_obj( ch, WEAR_WRIST_L, fReplace )
	&&   !remove_obj( ch, WEAR_WRIST_R, fReplace ) )
	    return;

	if ( ch->getEquipment(WEAR_WRIST_L ) == NULL )
	{
	    act( "$n wears $p around $s left wrist.", ch, obj, NULL, TO_ROOM, POS_RESTING );
	    act( "You wear $p around your left wrist.", ch, obj, NULL, TO_CHAR, POS_RESTING );
	    equip_char( ch, obj, WEAR_WRIST_L );
	    return;
	}

	if ( ch->getEquipment(WEAR_WRIST_R ) == NULL )
	{
	    act( "$n wears $p around $s right wrist.", ch, obj, NULL, TO_ROOM, POS_RESTING );
	    act( "You wear $p around your right wrist.", ch, obj, NULL, TO_CHAR, POS_RESTING );
	    equip_char( ch, obj, WEAR_WRIST_R );
	    return;
	}

	bug( "Wear_obj: no free wrist.", 0 );
	send_to_char( "You already wear two wrist items.\n\r", ch );
	return;
    }

    if ( obj->canWear( ITEM_WEAR_SHIELD ) )
    {
	Object *weapon;

	if ( !remove_obj( ch, WEAR_SHIELD, fReplace ) )
	    return;

	weapon = ch->getEquipment(WEAR_WIELD);
	if (weapon != NULL && ch->size < SIZE_LARGE 
	&&  IS_WEAPON_STAT(weapon,WEAPON_TWO_HANDS))
	{
	    send_to_char("Your hands are tied up with your weapon!\n\r",ch);
	    return;
	}

	act( "$n wears $p as a shield.", ch, obj, NULL, TO_ROOM, POS_RESTING );
	act( "You wear $p as a shield.", ch, obj, NULL, TO_CHAR, POS_RESTING );
	equip_char( ch, obj, WEAR_SHIELD );
	return;
    }

    if ( obj->canWear( ITEM_WIELD ) )
    {
	int sn,skill;

	if ( !remove_obj( ch, WEAR_WIELD, fReplace ) )
	    return;

	if ( !ch->isNPC() 
	&& get_obj_weight(obj) > (str_app[get_curr_stat(ch,STAT_STR)].wield  
		* 10))
	{
	    send_to_char( "It is too heavy for you to wield.\n\r", ch );
	    return;
	}

	if (!ch->isNPC() && ch->size < SIZE_LARGE 
	&&  IS_WEAPON_STAT(obj,WEAPON_TWO_HANDS)
 	&&  ch->getEquipment(WEAR_SHIELD) != NULL)
	{
	    send_to_char("You need two hands free for that weapon.\n\r",ch);
	    return;
	}

	act( "$n wields $p.", ch, obj, NULL, TO_ROOM, POS_RESTING );
	act( "You wield $p.", ch, obj, NULL, TO_CHAR, POS_RESTING );
	equip_char( ch, obj, WEAR_WIELD );

        sn = get_weapon_sn(ch);

	if (sn == gsn_hand_to_hand)
	   return;

        skill = get_weapon_skill(ch,sn);
 
        if (skill >= 100)
            act("$p feels like a part of you!",ch,obj,NULL,TO_CHAR, POS_RESTING);
        else if (skill > 85)
            act("You feel quite confident with $p.",ch,obj,NULL,TO_CHAR, POS_RESTING);
        else if (skill > 70)
            act("You are skilled with $p.",ch,obj,NULL,TO_CHAR, POS_RESTING);
        else if (skill > 50)
            act("Your skill with $p is adequate.",ch,obj,NULL,TO_CHAR, POS_RESTING);
        else if (skill > 25)
            act("$p feels a little clumsy in your hands.",ch,obj,NULL,TO_CHAR, POS_RESTING);
        else if (skill > 1)
            act("You fumble and almost drop $p.",ch,obj,NULL,TO_CHAR, POS_RESTING);
        else
            act("You don't even know which end is up on $p.", ch,obj,NULL,TO_CHAR, POS_RESTING);

	return;
    }

    if ( obj->canWear( ITEM_HOLD ) )
    {
	if ( !remove_obj( ch, WEAR_HOLD, fReplace ) )
	    return;
	act( "$n holds $p in $s hand.",   ch, obj, NULL, TO_ROOM, POS_RESTING );
	act( "You hold $p in your hand.", ch, obj, NULL, TO_CHAR, POS_RESTING );
	equip_char( ch, obj, WEAR_HOLD );
	return;
    }

    if ( obj->canWear(ITEM_WEAR_FLOAT) )
    {
		if (!remove_obj(ch,WEAR_FLOAT, fReplace) ) {
			return;
		}
		act("$n releases $p to float next to $m.",ch,obj,NULL,TO_ROOM, POS_RESTING);
		act("You release $p and it floats next to you.",ch,obj,NULL,TO_CHAR, POS_RESTING);
		equip_char(ch,obj,WEAR_FLOAT);
		return;
    }

    if ( fReplace )
	send_to_char( "You can't wear, wield, or hold that.\n\r", ch );

    return;
}

void do_second (Character *ch, char *argument)
/* wear object as a secondary weapon */
{
    Object *obj;
    char buf[MAX_STRING_LENGTH]; /* overkill, but what the heck */

    if (argument[0] == '\0') /* empty */
    {
        send_to_char ("Wear which weapon in your off-hand?\n\r",ch);
        return;
    }

    obj = get_obj_carry (ch, argument, ch); /* find the obj withing ch's inventory */

    if (obj == NULL)
    {
        send_to_char ("You have no such thing in your backpack.\n\r",ch);
        return;
    }


    /* check if the char is using a shield or a held weapon */

    if ( (ch->getEquipment(WEAR_SHIELD) != NULL) ||
         (ch->getEquipment(WEAR_HOLD)   != NULL) )
    {
        send_to_char ("You cannot use a secondary weapon while using a shield or holding an item\n\r",ch);
        return;
    }


    if ( ch->level < obj->getLevel() )
    {
        snprintf(buf, sizeof(buf), "You must be level %d to use this object.\n\r",
            obj->getLevel() );
        send_to_char( buf, ch );
        act( "$n tries to use $p, but is too inexperienced.", ch, obj, NULL, TO_ROOM, POS_RESTING );
        return;
    }

/* check that the character is using a first weapon at all */
    if (ch->getEquipment( WEAR_WIELD) == NULL) /* oops - != here was a bit wrong :) */
    {
        send_to_char ("You need to wield a primary weapon, before using a secondary one!\n\r",ch);
        return;
    }


/* check for str - secondary weapons have to be lighter */
    if ( get_obj_weight( obj ) > ( str_app[get_curr_stat(ch, STAT_STR)].wield / 2) )
    {
        send_to_char( "This weapon is too heavy to be used as a secondary weapon by you.\n\r", ch );
        return;
    }

/* check if the secondary weapon is at least half as light as the primary weapon */
    if ( (get_obj_weight (obj)*2) > get_obj_weight(ch->getEquipment(WEAR_WIELD)) )
    {
        send_to_char ("Your secondary weapon has to be considerably lighter than the primary one.\n\r",ch);
        return;
    }

/* at last - the char uses the weapon */



    if (!remove_obj(ch, WEAR_SECONDARY, TRUE)) /* remove the current weapon if any */
        return;                                /* remove obj tells about any no_remove */

/* char CAN use the item! that didn't take long at aaall */

    act ("$n wields $p in $s off-hand.",ch,obj,NULL,TO_ROOM, POS_RESTING);
    act ("You wield $p in your off-hand.",ch,obj,NULL,TO_CHAR, POS_RESTING);
    equip_char ( ch, obj, WEAR_SECONDARY);
    return;
}

void do_wear( Character *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    Object *obj;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	send_to_char( "Wear, wield, or hold what?\n\r", ch );
	return;
    }

    if ( !str_cmp( arg, "all" ) )
    {
	Object *obj_next;

	for ( auto obj : ch->getCarrying() )
	{
	    if ( obj->getWearLocation() == WEAR_NONE && ch->can_see( obj ) )
		wear_obj( ch, obj, FALSE );
	}
	return;
    }
    else
    {
	if ( ( obj = get_obj_carry( ch, arg, ch ) ) == NULL )
	{
	    send_to_char( "You do not have that item.\n\r", ch );
	    return;
	}

	wear_obj( ch, obj, TRUE );
    }

    return;
}



void do_remove( Character *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    Object *obj;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	send_to_char( "Remove what?\n\r", ch );
	return;
    }

    if ( ( obj = get_obj_wear( ch, arg ) ) == NULL )
    {
	send_to_char( "You do not have that item.\n\r", ch );
	return;
    }

    remove_obj( ch, obj->getWearLocation(), TRUE );
    return;
}



void do_sacrifice( Character *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    Object *obj;
    int silver;
    
    /* variables for AUTOSPLIT */
    Character *gch;
    int members;
    char buffer[100];


    one_argument( argument, arg );

    if ( arg[0] == '\0' || !str_cmp( arg, ch->getName().c_str() ) )
    {
		act( "$n offers $mself to Mota, who graciously declines.", ch, NULL, NULL, TO_ROOM, POS_RESTING );
		send_to_char( "Mota appreciates your offer and may accept it later.\n\r", ch );
	return;
    }

    obj = ObjectHelper::findInList( ch, arg, ch->in_room->contents );
    if ( obj == NULL )
    {
	send_to_char( "You can't find it.\n\r", ch );
	return;
    }

    if ( obj->getItemType() == ITEM_CORPSE_PC )
    {
	if (obj->getContents().size() > 0)
        {
	   send_to_char(
	     "Mota wouldn't like that.\n\r",ch);
	   return;
        }
    }


    if ( !obj->canWear(ITEM_TAKE) || obj->canWear(ITEM_NO_SAC))
    {
		act( "$p is not an acceptable sacrifice.", ch, obj, 0, TO_CHAR, POS_RESTING );
		return;
    }

    if (obj->getInRoom() != NULL)
    {
	for (gch = obj->getInRoom()->people; gch != NULL; gch = gch->next_in_room)
	    if (gch->onObject() == obj)
	    {
		act("$N appears to be using $p.",
		    ch,obj,gch,TO_CHAR, POS_RESTING);
		return;
	    }
    }
		
    silver = UMAX(1,obj->getLevel() * 3);

    if (obj->getItemType() != ITEM_CORPSE_NPC && obj->getItemType() != ITEM_CORPSE_PC)
    	silver = UMIN(silver,obj->getCost());

    if (silver == 1)
        send_to_char(
	    "Mota gives you one silver coin for your sacrifice.\n\r", ch );
    else
    {
	snprintf(buf, sizeof(buf),"Mota gives you %d silver coins for your sacrifice.\n\r",
		silver);
	send_to_char(buf,ch);
    }
    
    ch->silver += silver;
    
    if (IS_SET(ch->act,PLR_AUTOSPLIT) )
    { /* AUTOSPLIT code */
    	members = 0;
	for (gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room )
    	{
    	    if ( gch->isSameGroup( ch ) )
            members++;
    	}

	if ( members > 1 && silver > 1)
	{
	    snprintf(buffer, sizeof(buffer),"%d",silver);
	    do_split(ch,buffer);	
	}
    }

    act( "$n sacrifices $p to Mota.", ch, obj, NULL, TO_ROOM, POS_RESTING );
    Wiznet::instance()->report("$N sends up $p as a burnt offering.",
	   ch,obj,WIZ_SACCING,0,0);
    extract_obj( obj );
    return;
}



void do_quaff( Character *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    Object *obj;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	send_to_char( "Quaff what?\n\r", ch );
	return;
    }

    if ( ( obj = get_obj_carry( ch, arg, ch ) ) == NULL )
    {
	send_to_char( "You do not have that potion.\n\r", ch );
	return;
    }

    if ( obj->getItemType() != ITEM_POTION )
    {
	send_to_char( "You can quaff only potions.\n\r", ch );
	return;
    }

    if (ch->level < obj->getLevel())
    {
	send_to_char("This liquid is too powerful for you to drink.\n\r",ch);
	return;
    }

    act( "$n quaffs $p.", ch, obj, NULL, TO_ROOM, POS_RESTING );
    act( "You quaff $p.", ch, obj, NULL ,TO_CHAR, POS_RESTING );

    obj_cast_spell( obj->getValues().at(1), obj->getValues().at(0), true, ch, ch, NULL );
    obj_cast_spell( obj->getValues().at(2), obj->getValues().at(0), true, ch, ch, NULL );
    obj_cast_spell( obj->getValues().at(3), obj->getValues().at(0), true, ch, ch, NULL );

    extract_obj( obj );
    return;
}



void do_recite( Character *ch, char *argument ) {
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    Character *victim;
    Object *scroll;
    Object *obj;

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);

    if ((scroll = get_obj_carry(ch, arg1, ch)) == NULL) {
        send_to_char("You do not have that scroll.\n\r", ch);
        return;
    }

    if (scroll->getItemType() != ITEM_SCROLL) {
        send_to_char("You can recite only scrolls.\n\r", ch);
        return;
    }

    if (ch->level < scroll->getLevel()) {
        send_to_char(
                "This scroll is too complex for you to comprehend.\n\r", ch);
        return;
    }

    obj = NULL;
    if (arg2[0] == '\0') {
        victim = ch;
    } else {
        if ((victim = get_char_room(ch, arg2)) == NULL
            && (obj = get_obj_here(ch, arg2)) == NULL) {
            send_to_char("You can't find it.\n\r", ch);
            return;
        }
    }

    act("$n recites $p.", ch, scroll, NULL, TO_ROOM, POS_RESTING);
    act("You recite $p.", ch, scroll, NULL, TO_CHAR, POS_RESTING);

    if (number_percent() >= 20 + get_skill(ch, gsn_scrolls) * 4 / 5) {
        send_to_char("You mispronounce a syllable.\n\r", ch);
        check_improve(ch, gsn_scrolls, FALSE, 2);
    } else {
        obj_cast_spell(scroll->getValues().at(1), scroll->getValues().at(0), true, ch, victim, obj);
        obj_cast_spell(scroll->getValues().at(2), scroll->getValues().at(0), true, ch, victim, obj);
        obj_cast_spell(scroll->getValues().at(3), scroll->getValues().at(0), true, ch, victim, obj);
        check_improve(ch, gsn_scrolls, TRUE, 2);
    }

    extract_obj(scroll);
    return;
}



void do_brandish( Character *ch, char *argument )
{
    Character *vch;
    Character *vch_next;
    Object *staff;
    int sn;

    if ( ( staff = ch->getEquipment(WEAR_HOLD ) ) == NULL )
    {
	send_to_char( "You hold nothing in your hand.\n\r", ch );
	return;
    }

    if ( staff->getItemType() != ITEM_STAFF )
    {
	send_to_char( "You can brandish only with a staff.\n\r", ch );
	return;
    }

    if ( ( sn = staff->getValues().at(3) ) < 0
    ||   sn >= MAX_SKILL
    ||   skill_table[sn].spell_fun == 0 )
    {
	bug( "Do_brandish: bad sn %d.", sn );
	return;
    }

    WAIT_STATE( ch, 2 * PULSE_VIOLENCE );

    if ( staff->getValues().at(2) > 0 )
    {
	act( "$n brandishes $p.", ch, staff, NULL, TO_ROOM, POS_RESTING );
	act( "You brandish $p.",  ch, staff, NULL, TO_CHAR, POS_RESTING );
	if ( ch->level < staff->getLevel() 
	||   number_percent() >= 20 + get_skill(ch,gsn_staves) * 4/5)
 	{
	    act ("You fail to invoke $p.",ch,staff,NULL,TO_CHAR, POS_RESTING);
	    act ("...and nothing happens.",ch,NULL,NULL,TO_ROOM, POS_RESTING);
	    check_improve(ch,gsn_staves,FALSE,2);
	}
	
	else for ( vch = ch->in_room->people; vch; vch = vch_next )
	{
	    vch_next	= vch->next_in_room;

	    switch ( skill_table[sn].target )
	    {
	    default:
		bug( "Do_brandish: bad target for sn %d.", sn );
		return;

	    case TAR_IGNORE:
		if ( vch != ch )
		    continue;
		break;

	    case TAR_CHAR_OFFENSIVE:
		if ( ch->isNPC() ? IS_NPC(vch) : !IS_NPC(vch) )
		    continue;
		break;
		
	    case TAR_CHAR_DEFENSIVE:
		if ( ch->isNPC() ? !IS_NPC(vch) : IS_NPC(vch) )
		    continue;
		break;

	    case TAR_CHAR_SELF:
		if ( vch != ch )
		    continue;
		break;
	    }

	    obj_cast_spell( staff->getValues().at(3), staff->getValues().at(0), true, ch, vch, NULL );
	    check_improve(ch,gsn_staves,TRUE,2);
	}
    }

    if ( --staff->getValues().at(2) <= 0 )
    {
	act( "$n's $p blazes bright and is gone.", ch, staff, NULL, TO_ROOM, POS_RESTING );
	act( "Your $p blazes bright and is gone.", ch, staff, NULL, TO_CHAR, POS_RESTING );
	extract_obj( staff );
    }

    return;
}



void do_zap( Character *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    Character *victim;
    Object *wand;
    Object *obj;

    one_argument( argument, arg );
    if ( arg[0] == '\0' && ch->fighting == NULL )
    {
	send_to_char( "Zap whom or what?\n\r", ch );
	return;
    }

    if ( ( wand = ch->getEquipment(WEAR_HOLD ) ) == NULL )
    {
	send_to_char( "You hold nothing in your hand.\n\r", ch );
	return;
    }

    if ( wand->getItemType() != ITEM_WAND )
    {
	send_to_char( "You can zap only with a wand.\n\r", ch );
	return;
    }

    obj = NULL;
    if ( arg[0] == '\0' )
    {
	if ( ch->fighting != NULL )
	{
	    victim = ch->fighting;
	}
	else
	{
	    send_to_char( "Zap whom or what?\n\r", ch );
	    return;
	}
    }
    else
    {
	if ( ( victim = get_char_room ( ch, arg ) ) == NULL
	&&   ( obj    = get_obj_here  ( ch, arg ) ) == NULL )
	{
	    send_to_char( "You can't find it.\n\r", ch );
	    return;
	}
    }

    WAIT_STATE( ch, 2 * PULSE_VIOLENCE );

    if ( wand->getValues().at(2) > 0 )
    {
	if ( victim != NULL )
	{
	    act( "$n zaps $N with $p.", ch, wand, victim, TO_NOTVICT, POS_RESTING );
	    act( "You zap $N with $p.", ch, wand, victim, TO_CHAR, POS_RESTING );
	    act( "$n zaps you with $p.",ch, wand, victim, TO_VICT, POS_RESTING );
	}
	else
	{
	    act( "$n zaps $P with $p.", ch, wand, obj, TO_ROOM, POS_RESTING );
	    act( "You zap $P with $p.", ch, wand, obj, TO_CHAR, POS_RESTING );
	}

 	if (ch->level < wand->getLevel() 
	||  number_percent() >= 20 + get_skill(ch,gsn_wands) * 4/5) 
	{
	    act( "Your efforts with $p produce only smoke and sparks.", ch,wand,NULL,TO_CHAR, POS_RESTING);
	    act( "$n's efforts with $p produce only smoke and sparks.", ch,wand,NULL,TO_ROOM, POS_RESTING);
	    check_improve(ch,gsn_wands,FALSE,2);
	}
	else
	{
	    obj_cast_spell( wand->getValues().at(3), wand->getValues().at(0), true, ch, victim, obj );
	    check_improve(ch,gsn_wands,TRUE,2);
	}
    }

    if ( --wand->getValues().at(2) <= 0 )
    {
		act( "$n's $p explodes into fragments.", ch, wand, NULL, TO_ROOM, POS_RESTING );
		act( "Your $p explodes into fragments.", ch, wand, NULL, TO_CHAR, POS_RESTING );
		extract_obj( wand );
    }

    return;
}



void do_steal( Character *ch, char *argument )
{
    char buf  [MAX_STRING_LENGTH];
    char arg1 [MAX_INPUT_LENGTH];
    char arg2 [MAX_INPUT_LENGTH];
    Character *victim;
    Object *obj;
    int percent;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( arg1[0] == '\0' || arg2[0] == '\0' )
    {
	send_to_char( "Steal what from whom?\n\r", ch );
	return;
    }

    if ( ( victim = get_char_room( ch, arg2 ) ) == NULL )
    {
	send_to_char( "They aren't here.\n\r", ch );
	return;
    }

    if ( victim == ch )
    {
	send_to_char( "That's pointless.\n\r", ch );
	return;
    }

    if (is_safe(ch,victim))
	return;

    if ( IS_NPC(victim) 
	  && victim->position == POS_FIGHTING)
    {
	send_to_char(  "Kill stealing is not permitted.\n\r"
		       "You'd better not -- you might get hit.\n\r",ch);
	return;
    }

    WAIT_STATE( ch, skill_table[gsn_steal].beats );
    percent  = number_percent();

    if (!IS_AWAKE(victim))
    	percent -= 10;
    else if (!victim->can_see(ch))
    	percent += 25;
    else 
	percent += 50;

    if ( ((ch->level + 7 < victim->level || ch->level -7 > victim->level) 
    && !IS_NPC(victim) && !ch->isNPC() )
    || ( !ch->isNPC() && percent > get_skill(ch,gsn_steal))
    || ( !ch->isNPC() && !ch->isClanned()) )
    {
	/*
	 * Failure.
	 */
	send_to_char( "Oops.\n\r", ch );
	affect_strip(ch,gsn_sneak);
	REMOVE_BIT(ch->affected_by,AFF_SNEAK);

	act( "$n tried to steal from you.\n\r", ch, NULL, victim, TO_VICT, POS_RESTING    );
	act( "$n tried to steal from $N.\n\r",  ch, NULL, victim, TO_NOTVICT, POS_RESTING );
	switch(number_range(0,3))
	{
	case 0 :
	   snprintf(buf, sizeof(buf), "%s is a lousy thief!", ch->getName().c_str() );
	   break;
        case 1 :
	   snprintf(buf, sizeof(buf), "%s couldn't rob %s way out of a paper bag!",
		    ch->getName().c_str(),(ch->sex == 2) ? "her" : "his");
	   break;
	case 2 :
	    snprintf(buf, sizeof(buf),"%s tried to rob me!",ch->getName().c_str() );
	    break;
	case 3 :
	    snprintf(buf, sizeof(buf),"Keep your hands out of there, %s!",ch->getName().c_str() );
	    break;
        }
        if (!IS_AWAKE(victim))
            do_wake(victim,(char*)"");
	if (IS_AWAKE(victim))
	    do_yell( victim, buf );
	if ( !ch->isNPC() )
	{
	    if ( IS_NPC(victim) )
	    {
	        check_improve(ch,gsn_steal,FALSE,2);
		multi_hit( victim, ch, TYPE_UNDEFINED );
	    }
	    else
	    {
		snprintf(buf, sizeof(buf),"$N tried to steal from %s.",victim->getName().c_str());
		Wiznet::instance()->report(buf,ch,NULL,WIZ_FLAGS,0,0);
		if ( !IS_SET(ch->act, PLR_THIEF) && !IS_IMMORTAL(ch))
		{
		    SET_BIT(ch->act, PLR_THIEF);
		    send_to_char( "*** You are now a THIEF!! ***\n\r", ch );
		    save_char_obj( ch );
		}
	    }
	}

	return;
    }

    if ( !str_cmp( arg1, "coin"  )
    ||   !str_cmp( arg1, "coins" )
    ||   !str_cmp( arg1, "gold"  ) 
    ||	 !str_cmp( arg1, "silver"))
    {
	int gold, silver;

	gold = victim->gold * number_range(1, ch->level) / 60;
	silver = victim->silver * number_range(1,ch->level) / 60;
	if ( gold <= 0 && silver <= 0 )
	{
	    send_to_char( "You couldn't get any coins.\n\r", ch );
	    return;
	}

	ch->gold     	+= gold;
	ch->silver   	+= silver;
	victim->silver 	-= silver;
	victim->gold 	-= gold;
	if (silver <= 0)
	    snprintf(buf, sizeof(buf), "Bingo!  You got %d gold coins.\n\r", gold );
	else if (gold <= 0)
	    snprintf(buf, sizeof(buf), "Bingo!  You got %d silver coins.\n\r",silver);
	else
	    snprintf(buf, sizeof(buf), "Bingo!  You got %d silver and %d gold coins.\n\r",
		    silver,gold);

	send_to_char( buf, ch );
	check_improve(ch,gsn_steal,TRUE,2);
	return;
    }

    if ( ( obj = get_obj_carry( victim, arg1, ch ) ) == NULL )
    {
	send_to_char( "You can't find it.\n\r", ch );
	return;
    }
	
    if ( !can_drop_obj( ch, obj )
    ||   IS_SET(obj->getExtraFlags(), ITEM_INVENTORY)
    ||   obj->getLevel() > ch->level )
    {
	send_to_char( "You can't pry it away.\n\r", ch );
	return;
    }

    if ( ch->carry_number + get_obj_number( obj ) > can_carry_n( ch ) )
    {
	send_to_char( "You have your hands full.\n\r", ch );
	return;
    }

    if ( ch->carry_weight + get_obj_weight( obj ) > can_carry_w( ch ) )
    {
	send_to_char( "You can't carry that much weight.\n\r", ch );
	return;
    }

    obj_from_char( obj );
    ch->addObject( obj );
    act("You pocket $p.",ch,obj,NULL,TO_CHAR, POS_RESTING);
    check_improve(ch,gsn_steal,TRUE,2);
    send_to_char( "Got it!\n\r", ch );
    return;
}

void do_buy( Character *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    int cost,roll;

    if ( argument[0] == '\0' )
    {
	send_to_char( "Buy what?\n\r", ch );
	return;
    }

    if ( IS_SET(ch->in_room->room_flags, ROOM_PET_SHOP) )
    {
	char arg[MAX_INPUT_LENGTH];
	char buf[MAX_STRING_LENGTH];
	Character *pet;
	ROOM_INDEX_DATA *pRoomIndexNext;
	ROOM_INDEX_DATA *in_room;

	smash_tilde(argument);

	if ( ch->isNPC() )
	    return;

	argument = one_argument(argument,arg);

	/* hack to make new thalos pets work */
	if (ch->in_room->vnum == 9621)
	    pRoomIndexNext = get_room_index(9706);
	else
	    pRoomIndexNext = get_room_index( ch->in_room->vnum + 1 );
	if ( pRoomIndexNext == NULL )
	{
	    bug( "Do_buy: bad pet shop at vnum %d.", ch->in_room->vnum );
	    send_to_char( "Sorry, you can't buy that here.\n\r", ch );
	    return;
	}

	in_room     = ch->in_room;
	ch->in_room = pRoomIndexNext;
	pet         = get_char_room( ch, arg );
	ch->in_room = in_room;

	if ( pet == NULL || !IS_SET(pet->act, ACT_PET) )
	{
	    send_to_char( "Sorry, you can't buy that here.\n\r", ch );
	    return;
	}

	if ( ch->pet != NULL )
	{
	    send_to_char("You already own a pet.\n\r",ch);
	    return;
	}

 	cost = 10 * pet->level * pet->level;

	if ( (ch->silver + 100 * ch->gold) < cost )
	{
	    send_to_char( "You can't afford it.\n\r", ch );
	    return;
	}

	if ( ch->level < pet->level )
	{
	    send_to_char(
		"You're not powerful enough to master this pet.\n\r", ch );
	    return;
	}

	/* haggle */
	roll = number_percent();
	if (roll < get_skill(ch,gsn_haggle) && number_range(1, 25) < get_curr_stat(ch, STAT_INT))
	{
	    cost -= cost / 2 * roll / 100;
	    snprintf(buf, sizeof(buf),"You haggle the price down to %d coins.\n\r",cost);
	    send_to_char(buf,ch);
	    check_improve(ch,gsn_haggle,TRUE,4);
	
	}

	deduct_cost(ch,cost);
	pet			= new NonPlayerCharacter( pet->pIndexData );
	SET_BIT(pet->act, ACT_PET);
	SET_BIT(pet->affected_by, AFF_CHARM);
	pet->comm = COMM_NOTELL|COMM_NOSHOUT|COMM_NOCHANNELS;

	argument = one_argument( argument, arg );
	if ( arg[0] != '\0' )
	{
	    snprintf(buf, sizeof(buf), "%s %s", pet->getName().c_str(), arg );
	    pet->setName(buf);
	}

	snprintf(buf, sizeof(buf), "%sA neck tag says 'I belong to %s'.\n\r",
	    pet->getDescription(), ch->getName().c_str() );
    pet->setDescription( buf );

	char_to_room( pet, ch->in_room );
	add_follower( pet, ch );
	pet->leader = ch;
	ch->pet = pet;
	send_to_char( "Enjoy your pet.\n\r", ch );
	act( "$n bought $N as a pet.", ch, NULL, pet, TO_ROOM, POS_RESTING );
	return;
    }
    else
    {
		ShopKeeper *keeper;

		if ( ( keeper = ShopKeeper::find( ch ) ) == NULL ) {
			return;
		}

		keeper->sell(ch, argument);
		delete keeper;
    }
}



void do_list( Character *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];

    if ( IS_SET(ch->in_room->room_flags, ROOM_PET_SHOP) )
    {
	ROOM_INDEX_DATA *pRoomIndexNext;
	Character *pet;
	bool found;

        /* hack to make new thalos pets work */
        if (ch->in_room->vnum == 9621)
            pRoomIndexNext = get_room_index(9706);
        else
            pRoomIndexNext = get_room_index( ch->in_room->vnum + 1 );

	if ( pRoomIndexNext == NULL )
	{
	    bug( "Do_list: bad pet shop at vnum %d.", ch->in_room->vnum );
	    send_to_char( "You can't do that here.\n\r", ch );
	    return;
	}

	found = FALSE;
	for ( pet = pRoomIndexNext->people; pet; pet = pet->next_in_room )
	{
	    if ( IS_SET(pet->act, ACT_PET) )
	    {
		if ( !found )
		{
		    found = TRUE;
		    send_to_char( "Pets for sale:\n\r", ch );
		}
		snprintf(buf, sizeof(buf), "[%2d] %8d - %s\n\r",
		    pet->level,
		    10 * pet->level * pet->level,
		    pet->short_descr );
		send_to_char( buf, ch );
	    }
	}
	if ( !found )
	    send_to_char( "Sorry, we're out of pets right now.\n\r", ch );
	return;
    }
    else
    {
		ShopKeeper *keeper;
		char arg[MAX_INPUT_LENGTH];

		if ( ( keeper = ShopKeeper::find( ch ) ) == NULL ) {
			return;
		}
		one_argument(argument,arg);

		keeper->list(ch, arg);
		delete keeper;
		return;
    }
}



void do_sell( Character *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    ShopKeeper *keeper;
    Object *obj;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	send_to_char( "Sell what?\n\r", ch );
	return;
    }

    if ( ( keeper = ShopKeeper::find( ch ) ) == NULL )
	return;

	keeper->buy(ch, arg);
	delete keeper;
    return;
}



void do_value( Character *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    ShopKeeper *keeper;
    Object *obj;
    int cost;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	send_to_char( "Value what?\n\r", ch );
	return;
    }

    if ( ( keeper = ShopKeeper::find( ch ) ) == NULL )
	return;

    if ( ( obj = get_obj_carry( ch, arg, ch ) ) == NULL )
    {
		act( "$n tells you 'You don't have that item'.", keeper->getCharacter(), NULL, ch, TO_VICT, POS_RESTING );
		ch->reply = keeper->getCharacter();
		delete keeper;
		return;
    }

    if (!keeper->getCharacter()->can_see(obj))
    {
        act("$n doesn't see what you are offering.",keeper->getCharacter(),NULL,ch,TO_VICT, POS_RESTING);
		delete keeper;
        return;
    }

    if ( !can_drop_obj( ch, obj ) )
    {
	send_to_char( "You can't let go of it.\n\r", ch );
	return;
    }

    if ( ( cost = keeper->get_cost( obj, FALSE ) ) <= 0 )
    {
		act( "$n looks uninterested in $p.", keeper->getCharacter(), obj, ch, TO_VICT, POS_RESTING );
		delete keeper;
		return;
    }

    snprintf(buf, sizeof(buf), 
	"$n tells you 'I'll give you %d silver and %d gold coins for $p'.", 
	cost - (cost/100) * 100, cost/100 );
    act( buf, keeper->getCharacter(), obj, ch, TO_VICT, POS_RESTING );
    ch->reply = keeper->getCharacter();
	delete keeper;
    return;
}

/* put an item on auction, or see the stats on the current item or bet */
void do_auction (Character *ch, char *argument)
{
    char arg1[MAX_INPUT_LENGTH];

    argument = one_argument (argument, arg1);

	if (ch->isNPC()) /* NPC can be extracted at any time and thus can't auction! */
		return;

}
