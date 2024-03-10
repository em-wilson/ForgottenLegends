#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include "merc.h"
#include "NonPlayerCharacter.h"
#include "Object.h"
#include "Room.h"

Character * find_target args( ( Character *ch, int dir, char * search ) );
Character * find_char	args( ( Character *ch, char *search, ROOM_INDEX_DATA *scan_room ) );

void do_shoot( Character *ch, char *argument )
{
    Character *victim;
    Object *bow;
    bool found_arrow;
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    sh_int door;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if (get_skill(ch, gsn_bow) < 1)
    {
        send_to_char("You are not skilled enough at the bow to use this skill.\n\r",ch);
        return;
    }

    if ((bow = ch->getEquipment(WEAR_WIELD)) == NULL || bow->getItemType() != ITEM_BOW)
    {
        send_to_char("You need a bow for this skill.\n\r",ch);
        return;
    }

    else if (!str_cmp(arg1, "n") || !str_cmp(arg1, "north")) door = 0;
    else if (!str_cmp(arg1, "e") || !str_cmp(arg1, "east"))  door = 1;
    else if (!str_cmp(arg1, "s") || !str_cmp(arg1, "south")) door = 2;
    else if (!str_cmp(arg1, "w") || !str_cmp(arg1, "west"))  door = 3;
    else if (!str_cmp(arg1, "u") || !str_cmp(arg1, "up" ))   door = 4;
    else if (!str_cmp(arg1, "d") || !str_cmp(arg1, "down"))  door = 5;
    else
    {
	send_to_char("Syntax: shoot <n|e|s|w|u|d> [target].\n\r",ch);
	return;
    }

    if (arg2[0] == '\0') {
        strcpy(arg2, "random");
    }

    if ((victim = find_target( ch, door, arg2 )) == NULL)
    {
	send_to_char( "That target doesn't exist.\n\r",ch);
	return;
    }

    if ( is_safe( ch, victim ) )
    {
	act( "You take aim at $N, but reconsider.", ch, NULL, victim, TO_CHAR, POS_RESTING );
	return;
    }

    found_arrow = FALSE;

    for ( auto arrow : ch->getCarrying() )
    {
        if (arrow->getItemType() != ITEM_ARROW) {
            continue;
        } else {
            --arrow->getValues().at(0);
            if (arrow->getValues().at(0) < 1)
            {
                send_to_char("You use your final arrow in this quiver.\n\r",ch);
                obj_from_char( arrow );
            } else {
                snprintf(buf, sizeof(buf),"Arrows remaining in this quiver: %d\n\r", arrow->getValues().at(0) );
                send_to_char(buf,ch);
            }

            found_arrow = TRUE;
            break;
        }
    }

    if (!found_arrow)
    {
        send_to_char("You have no more free arrows.\n\r",ch);
        return;
    }

    if ( get_skill(ch, gsn_bow) < number_range( 1, 120 ) )
    {
        send_to_char("You miss your mark.\n\r", ch );
        check_improve(ch,gsn_bow,FALSE,2);
        WAIT_STATE(ch, PULSE_VIOLENCE);
        return;
    }

    act( "You let loose an arrow at $N!", ch, NULL, victim, TO_CHAR, POS_RESTING);
    act( "$n let loose an arrow at you!", ch, NULL, victim, TO_VICT, POS_RESTING);
    act( "$n lets loose an arrow at $N!", ch, NULL, victim, TO_ROOM, POS_RESTING);
    act( "$n is struck by an arrow!", victim, NULL, ch, TO_ROOM, POS_RESTING);
    check_improve(ch,gsn_bow,FALSE,2);
    WAIT_STATE(ch,PULSE_VIOLENCE);

    if (IS_NPC(victim) && IS_SET(victim->act, ACT_SENTINEL) && ch->level >= 10)
    {
        int num;

        /* Create them */
        for (num = 0; num < (ch->level / 10); num++)
        {
            Character *mob;

            mob = new NonPlayerCharacter( get_mob_index( MOB_VNUM_CITYGUARD ) );
            char_to_room( mob, victim->in_room );

            mob->hunting = ch;
        }

        /* Send the message */
        if (num < 2)
            snprintf(buf, sizeof(buf), "$n calls a guard upon $N!");
        else
            snprintf(buf, sizeof(buf), "$n calls %d guards upon $N!", num);
        act( buf, victim, NULL, ch, TO_ROOM, POS_RESTING );

        if (num < 2)
            snprintf(buf, sizeof(buf), "$n calls a guard upon you!");
        else
            snprintf(buf, sizeof(buf), "$n calls %d guards upon you!", num);
        act( buf, victim, NULL, ch, TO_VICT, POS_RESTING );
    } else {
    	victim->hunting = ch;
    }

    damage( ch, victim, dice(bow->getValues().at(1), bow->getValues().at(2)), gsn_bow, DAM_PIERCE, TRUE );
    return;
}

Character * find_target( Character *ch, int dir, char * search )
{
    Character * victim = NULL;
    ROOM_INDEX_DATA *scan_room;
    EXIT_DATA *pExit;
    sh_int depth;

    Object * bow = ch->getEquipment(WEAR_WIELD );

    if (bow->getItemType() != ITEM_BOW) {
    	return NULL;
    }

    int range = bow->getValues().at(0);
    scan_room = ch->in_room;

    for (depth = 0; depth < range; depth++)
    {
        if ((pExit = scan_room->exit[dir]) != NULL)
        {
            scan_room = pExit->u1.to_room;
            victim = find_char( ch, search, scan_room );
        }
    }
    return victim;
}

Character * find_char( Character *ch, char *search, ROOM_INDEX_DATA *scan_room )
{
    Character *victim;
    Character *rch;

    victim = NULL;
    if (scan_room == NULL) return victim;

    for (rch=scan_room->people; rch != NULL; rch=rch->next_in_room)
    {
	if (rch == ch) continue;
	if (!IS_NPC(rch) && rch->invis_level > ch->getTrust()) continue;

	/*
	* Are we looking for someone in particular?
	*/
	if (str_cmp(search, "random"))
	{
	    if (is_name( search, rch->getName().c_str() ) && ch->can_see( rch))
		return rch;
	}

	if (ch->can_see( rch))
	{
	    if (number_range(ch->level, MAX_LEVEL) == number_range( ch->level, MAX_LEVEL))
		victim = rch;
	}
    }

    return victim;
}

void dispatch_guard( Character *victim, Character *ch )
{
}
