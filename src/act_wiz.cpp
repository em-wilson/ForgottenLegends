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

#include <sstream>
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "merc.h"
#include "board.h"
#include "recycle.h"
#include "tables.h"
#include "lookup.h"
#include "clans/ClanManager.h"
#include "ConnectedState.h"
#include "ExtraDescription.h"
#include "ILogger.h"
#include "NonPlayerCharacter.h"
#include "Object.h"
#include "ObjectHelper.h"
#include "PlayerCharacter.h"
#include "RaceManager.h"
#include "Room.h"
#include "SocketHelper.h"
#include "Wiznet.h"

/* command procedures needed */
DECLARE_DO_FUN(do_rstat);
DECLARE_DO_FUN(do_mstat);
DECLARE_DO_FUN(do_ostat);
DECLARE_DO_FUN(do_rset);
DECLARE_DO_FUN(do_mset);
DECLARE_DO_FUN(do_setclan);
DECLARE_DO_FUN(do_oset);
DECLARE_DO_FUN(do_sset);
DECLARE_DO_FUN(do_mfind);
DECLARE_DO_FUN(do_ofind);
DECLARE_DO_FUN(do_slookup);
DECLARE_DO_FUN(do_mload);
DECLARE_DO_FUN(do_oload);
DECLARE_DO_FUN(do_quit);
DECLARE_DO_FUN(do_look);
DECLARE_DO_FUN(do_stand);

using std::stringstream;

extern ClanManager *clan_manager;
extern RaceManager *race_manager;

void do_wiznet(Character *ch, char *argument)
{
    int flag;
    char buf[MAX_STRING_LENGTH];

    if (argument[0] == '\0')
    {
        if (IS_SET(ch->wiznet, WIZ_ON))
        {
            send_to_char("Signing off of Wiznet.\n\r", ch);
            REMOVE_BIT(ch->wiznet, WIZ_ON);
        }
        else
        {
            send_to_char("Welcome to Wiznet!\n\r", ch);
            SET_BIT(ch->wiznet, WIZ_ON);
        }
        return;
    }

    if (!str_prefix(argument, "on"))
    {
        send_to_char("Welcome to Wiznet!\n\r", ch);
        SET_BIT(ch->wiznet, WIZ_ON);
        return;
    }

    if (!str_prefix(argument, "off"))
    {
        send_to_char("Signing off of Wiznet.\n\r", ch);
        REMOVE_BIT(ch->wiznet, WIZ_ON);
        return;
    }

    /* show wiznet status */
    if (!str_prefix(argument, "status"))
    {
        buf[0] = '\0';

        if (!IS_SET(ch->wiznet, WIZ_ON))
            strcat(buf, "off ");

        for (flag = 0; wiznet_table[flag].name != NULL; flag++)
            if (IS_SET(ch->wiznet, wiznet_table[flag].flag))
            {
                strcat(buf, wiznet_table[flag].name);
                strcat(buf, " ");
            }

        strcat(buf, "\n\r");

        send_to_char("Wiznet status:\n\r", ch);
        send_to_char(buf, ch);
        return;
    }

    if (!str_prefix(argument, "show"))
    /* list of all wiznet options */
    {
        buf[0] = '\0';

        for (flag = 0; wiznet_table[flag].name != NULL; flag++)
        {
            if (wiznet_table[flag].level <= ch->getTrust())
            {
                strcat(buf, wiznet_table[flag].name);
                strcat(buf, " ");
            }
        }

        strcat(buf, "\n\r");

        send_to_char("Wiznet options available to you are:\n\r", ch);
        send_to_char(buf, ch);
        return;
    }

    flag = wiznet_lookup(argument);

    if (flag == -1 || ch->getTrust() < wiznet_table[flag].level)
    {
        send_to_char("No such option.\n\r", ch);
        return;
    }

    if (IS_SET(ch->wiznet, wiznet_table[flag].flag))
    {
        snprintf(buf, sizeof(buf), "You will no longer see %s on wiznet.\n\r",
                 wiznet_table[flag].name);
        send_to_char(buf, ch);
        REMOVE_BIT(ch->wiznet, wiznet_table[flag].flag);
        return;
    }
    else
    {
        snprintf(buf, sizeof(buf), "You will now see %s on wiznet.\n\r",
                 wiznet_table[flag].name);
        send_to_char(buf, ch);
        SET_BIT(ch->wiznet, wiznet_table[flag].flag);
        return;
    }
}

void do_guild(Character *ch, char *argument)
{
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    Character *v;

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);

    if (arg1[0] == '\0' || arg2[0] == '\0')
    {
        send_to_char("Syntax: guild <char> <cln name>\n\r", ch);
        return;
    }
    if ((v = get_char_world(ch, arg1)) == NULL || v->isNPC())
    {
        send_to_char("They aren't playing.\n\r", ch);
        return;
    }

    PlayerCharacter *victim = (PlayerCharacter*)v;

    if (!str_prefix(arg2, "none"))
    {
        send_to_char("They are now clanless.\n\r", ch);
        send_to_char("You are now a member of no clan!\n\r", victim);
        clan_manager->remove_player(victim);
        save_char_obj(victim);
        return;
    }

    Clan *clan = clan_manager->get_clan(arg2);
    if (!clan)
    {
        send_to_char("No such clan exists.\n\r", ch);
        return;
    }

    clan_manager->add_player(victim, clan);
    save_char_obj(victim);

    snprintf(buf, sizeof(buf), "They are now a member of clan %s.\n\r", clan->getName().c_str());
    send_to_char(buf, ch);
    snprintf(buf, sizeof(buf), "You are now a member of clan %s.\n\r", clan->getName().c_str());
    send_to_char(buf, victim);
}

/* equips a character */
void do_outfit(Character *caller, char *argument)
{
    Object *obj;
    int i, sn, vnum;

    if (caller->level > 5 || caller->isNPC())
    {
        send_to_char("Find it yourself!\n\r", caller);
        return;
    }

    PlayerCharacter *ch = (PlayerCharacter*)caller;

    if (ch->carry_number + 4 > can_carry_n(ch))
    {
        send_to_char("You are carrying too much.  Try droping some items.\n\r", ch);
        return;
    }

    if ((obj = ch->getEquipment(WEAR_LIGHT)) == NULL)
    {
        obj = ObjectHelper::createFromIndex(get_obj_index(OBJ_VNUM_SCHOOL_BANNER), 0);
        obj->setCost(0);
        ch->addObject(obj);
        equip_char(ch, obj, WEAR_LIGHT);
    }

    if ((obj = ch->getEquipment(WEAR_BODY)) == NULL)
    {
        obj = ObjectHelper::createFromIndex(get_obj_index(OBJ_VNUM_SCHOOL_VEST), 0);
        obj->setCost(0);
        ch->addObject(obj);
        equip_char(ch, obj, WEAR_BODY);
    }

    /* do the weapon thing */
    if ((obj = ch->getEquipment(WEAR_WIELD)) == NULL)
    {
        sn = 0;
        vnum = OBJ_VNUM_SCHOOL_SWORD; /* just in case! */

        for (i = 0; weapon_table[i].name != NULL; i++)
        {
            if (ch->learned[sn] <
                ch->learned[*weapon_table[i].gsn])
            {
                sn = *weapon_table[i].gsn;
                vnum = weapon_table[i].vnum;
            }
        }

        obj = ObjectHelper::createFromIndex(get_obj_index(vnum), 0);
        ch->addObject(obj);
        equip_char(ch, obj, WEAR_WIELD);
    }

    if (((obj = ch->getEquipment(WEAR_WIELD)) == NULL || !IS_WEAPON_STAT(obj, WEAPON_TWO_HANDS)) && (obj = ch->getEquipment(WEAR_SHIELD)) == NULL)
    {
        obj = ObjectHelper::createFromIndex(get_obj_index(OBJ_VNUM_SCHOOL_SHIELD), 0);
        obj->setCost(0);
        ch->addObject(obj);
        equip_char(ch, obj, WEAR_SHIELD);
    }

    send_to_char("You have been equipped by Mota.\n\r", ch);
}

/* RT nochannels command, for those spammers */
void do_nochannels(Character *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
    Character *victim;

    one_argument(argument, arg);

    if (arg[0] == '\0')
    {
        send_to_char("Nochannel whom?", ch);
        return;
    }

    if ((victim = get_char_world(ch, arg)) == NULL)
    {
        send_to_char("They aren't here.\n\r", ch);
        return;
    }

    if (victim->getTrust() >= ch->getTrust())
    {
        send_to_char("You failed.\n\r", ch);
        return;
    }

    if (IS_SET(victim->comm, COMM_NOCHANNELS))
    {
        REMOVE_BIT(victim->comm, COMM_NOCHANNELS);
        send_to_char("The gods have restored your channel priviliges.\n\r",
                     victim);
        send_to_char("NOCHANNELS removed.\n\r", ch);
        stringstream ss;
        ss << "$N restores channels to " << victim->getName();
        Wiznet::instance()->report(ss.str(), ch, NULL, WIZ_PENALTIES, WIZ_SECURE, 0);
    }
    else
    {
        SET_BIT(victim->comm, COMM_NOCHANNELS);
        send_to_char("The gods have revoked your channel priviliges.\n\r",
                     victim);
        send_to_char("NOCHANNELS set.\n\r", ch);
        stringstream ss;
        ss << "$N revokes " << victim->getName() << "'s channels.";
        Wiznet::instance()->report(ss.str(), ch, NULL, WIZ_PENALTIES, WIZ_SECURE, 0);
    }

    return;
}

void do_smote(Character *ch, char *argument)
{
    Character *vch;
    char *letter, *name;
    char last[MAX_INPUT_LENGTH], temp[MAX_STRING_LENGTH];
    unsigned int matches = 0;

    if (!ch->isNPC() && IS_SET(ch->comm, COMM_NOEMOTE))
    {
        send_to_char("You can't show your emotions.\n\r", ch);
        return;
    }

    if (argument[0] == '\0')
    {
        send_to_char("Emote what?\n\r", ch);
        return;
    }

    if (strstr(argument, ch->getName().c_str()) == NULL)
    {
        send_to_char("You must include your name in an smote.\n\r", ch);
        return;
    }

    send_to_char(argument, ch);
    send_to_char("\n\r", ch);

    for (vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room)
    {
        if (vch->desc == NULL || vch == ch)
            continue;

        if ((letter = strstr(argument, vch->getName().c_str())) == NULL)
        {
            send_to_char(argument, vch);
            send_to_char("\n\r", vch);
            continue;
        }

        strcpy(temp, argument);
        temp[strlen(argument) - strlen(letter)] = '\0';
        last[0] = '\0';
        name = str_dup(vch->getName().c_str());

        for (; *letter != '\0'; letter++)
        {
            if (*letter == '\'' && matches == strlen(vch->getName().c_str()))
            {
                strcat(temp, "r");
                continue;
            }

            if (*letter == 's' && matches == strlen(vch->getName().c_str()))
            {
                matches = 0;
                continue;
            }

            if (matches == vch->getName().length())
            {
                matches = 0;
            }

            if (*letter == *name)
            {
                matches++;
                name++;
                if (matches == vch->getName().length())
                {
                    strcat(temp, "you");
                    last[0] = '\0';
                    free_string(name);
                    name = str_dup(vch->getName().c_str());
                    continue;
                }
                strncat(last, letter, 1);
                continue;
            }

            matches = 0;
            strcat(temp, last);
            strncat(temp, letter, 1);
            last[0] = '\0';
            free_string(name);
            name = str_dup(vch->getName().c_str());
        }

        free_string(name);
        send_to_char(temp, vch);
        send_to_char("\n\r", vch);
    }

    return;
}

void do_bamfin(Character *caller, char *argument)
{
    char buf[MAX_STRING_LENGTH];

    if (caller->isNPC()) {
        return;
    }

    PlayerCharacter* ch = (PlayerCharacter*)caller;
    smash_tilde(argument);

    if (argument[0] == '\0')
    {
        snprintf(buf, sizeof(buf), "Your poofin is %s\n\r", ch->bamfin);
        send_to_char(buf, ch);
        return;
    }

    if (strstr(argument, ch->getName().c_str()) == NULL)
    {
        send_to_char("You must include your name.\n\r", ch);
        return;
    }

    free_string(ch->bamfin);
    ch->bamfin = str_dup(argument);

    snprintf(buf, sizeof(buf), "Your poofin is now %s\n\r", ch->bamfin);
    send_to_char(buf, ch);
}

void do_bamfout(Character *caller, char *argument)
{
    char buf[MAX_STRING_LENGTH];

    if (caller->isNPC()) {
        return;
    }

    PlayerCharacter* ch = (PlayerCharacter*)caller;
    smash_tilde(argument);

    if (argument[0] == '\0')
    {
        snprintf(buf, sizeof(buf), "Your poofout is %s\n\r", ch->bamfout);
        send_to_char(buf, ch);
        return;
    }

    if (strstr(argument, ch->getName().c_str()) == NULL)
    {
        send_to_char("You must include your name.\n\r", ch);
        return;
    }

    free_string(ch->bamfout);
    ch->bamfout = str_dup(argument);

    snprintf(buf, sizeof(buf), "Your poofout is now %s\n\r", ch->bamfout);
    send_to_char(buf, ch);
}

void do_deny(Character *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    Character *victim;

    one_argument(argument, arg);
    if (arg[0] == '\0')
    {
        send_to_char("Deny whom?\n\r", ch);
        return;
    }

    if ((victim = get_char_world(ch, arg)) == NULL)
    {
        send_to_char("They aren't here.\n\r", ch);
        return;
    }

    if (IS_NPC(victim))
    {
        send_to_char("Not on NPC's.\n\r", ch);
        return;
    }

    if (victim->getTrust() >= ch->getTrust())
    {
        send_to_char("You failed.\n\r", ch);
        return;
    }

    SET_BIT(victim->act, PLR_DENY);
    send_to_char("You are denied access!\n\r", victim);
    stringstream ss;
    ss << "$N denies access to " << victim->getName();
    Wiznet::instance()->report(ss.str(), ch, NULL, WIZ_PENALTIES, WIZ_SECURE, 0);
    send_to_char("OK.\n\r", ch);
    save_char_obj(victim);
    stop_fighting(victim, TRUE);
    do_quit(victim, (char *)"");

    return;
}

void do_disconnect(Character *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    DESCRIPTOR_DATA *d;
    Character *victim;

    one_argument(argument, arg);
    if (arg[0] == '\0')
    {
        send_to_char("Disconnect whom?\n\r", ch);
        return;
    }

    if (is_number(arg))
    {
        int desc;

        desc = atoi(arg);
        for (d = descriptor_list; d != NULL; d = d->next)
        {
            if (d->descriptor == desc)
            {
                SocketHelper::close_socket(d);
                send_to_char("Ok.\n\r", ch);
                return;
            }
        }
    }

    if ((victim = get_char_world(ch, arg)) == NULL)
    {
        send_to_char("They aren't here.\n\r", ch);
        return;
    }

    if (victim->desc == NULL)
    {
        act("$N doesn't have a descriptor.", ch, NULL, victim, TO_CHAR, POS_RESTING);
        return;
    }

    if (victim->getTrust() > ch->getTrust())
    {
        send_to_char("Bang.  You're dead.\n\r", ch);
        return;
    }

    for (d = descriptor_list; d != NULL; d = d->next)
    {
        if (d == victim->desc)
        {
            SocketHelper::close_socket(d);
            send_to_char("Ok.\n\r", ch);
            return;
        }
    }

    bug("Do_disconnect: desc not found.", 0);
    send_to_char("Descriptor not found!\n\r", ch);
    return;
}

void do_pardon(Character *ch, char *argument)
{
    char arg1[MAX_INPUT_LENGTH];
    Character *victim;

    argument = one_argument(argument, arg1);

    if (arg1[0] == '\0')
    {
        send_to_char("Syntax: pardon <character>.\n\r", ch);
        return;
    }

    if ((victim = get_char_world(ch, arg1)) == NULL)
    {
        send_to_char("They aren't here.\n\r", ch);
        return;
    }

    if (IS_NPC(victim))
    {
        send_to_char("Not on NPC's.\n\r", ch);
        return;
    }

    //    if ( !str_cmp( arg2, "thief" ) )
    //    {
    if (IS_SET(victim->act, PLR_THIEF))
    {
        REMOVE_BIT(victim->act, PLR_THIEF);
        send_to_char("Thief flag removed.\n\r", ch);
        send_to_char("You are no longer a THIEF.\n\r", victim);
    }
    return;
    //  }

    send_to_char("Syntax: pardon <character>.\n\r", ch);
    return;
}

void do_wanted(Character *ch, char *argument)
{
    char arg1[MAX_INPUT_LENGTH];
    Character *victim;

    argument = one_argument(argument, arg1);

    if (arg1[0] == '\0')
    {
        send_to_char("Syntax: wanted <character>.\n\r", ch);
        return;
    }

    if ((victim = get_char_world(ch, arg1)) == NULL)
    {
        send_to_char("They aren't here.\n\r", ch);
        return;
    }

    if (IS_NPC(victim))
    {
        send_to_char("Not on NPC's.\n\r", ch);
        return;
    }

    //    if ( !str_cmp( arg2, "thief" ) )
    //  {
    if (!IS_SET(victim->act, PLR_THIEF))
    {
        SET_BIT(victim->act, PLR_THIEF);
        send_to_char("Thief flag added.\n\r", ch);
        send_to_char("You are now a THIEF.\n\r", victim);
    }
    return;
    //    }

    send_to_char("Syntax: wanted <character>.\n\r", ch);
    return;
}

void do_echo(Character *ch, char *argument)
{
    DESCRIPTOR_DATA *d;

    if (argument[0] == '\0')
    {
        send_to_char("Global echo what?\n\r", ch);
        return;
    }

    for (d = descriptor_list; d; d = d->next)
    {
        if (d->connected == ConnectedState::Playing)
        {
            if (d->character->getTrust() >= ch->getTrust())
                send_to_char("global> ", d->character);
            send_to_char(argument, d->character);
            send_to_char("\n\r", d->character);
        }
    }

    return;
}

void do_recho(Character *ch, char *argument)
{
    DESCRIPTOR_DATA *d;

    if (argument[0] == '\0')
    {
        send_to_char("Local echo what?\n\r", ch);

        return;
    }

    for (d = descriptor_list; d; d = d->next)
    {
        if (d->connected == ConnectedState::Playing && d->character->in_room == ch->in_room)
        {
            if (d->character->getTrust() >= ch->getTrust())
                send_to_char("local> ", d->character);
            send_to_char(argument, d->character);
            send_to_char("\n\r", d->character);
        }
    }

    return;
}

void do_zecho(Character *ch, char *argument)
{
    DESCRIPTOR_DATA *d;

    if (argument[0] == '\0')
    {
        send_to_char("Zone echo what?\n\r", ch);
        return;
    }

    for (d = descriptor_list; d; d = d->next)
    {
        if (d->connected == ConnectedState::Playing && d->character->in_room != NULL && ch->in_room != NULL && d->character->in_room->area == ch->in_room->area)
        {
            if (d->character->getTrust() >= ch->getTrust())
                send_to_char("zone> ", d->character);
            send_to_char(argument, d->character);
            send_to_char("\n\r", d->character);
        }
    }
}

void do_pecho(Character *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    Character *victim;

    argument = one_argument(argument, arg);

    if (argument[0] == '\0' || arg[0] == '\0')
    {
        send_to_char("Personal echo what?\n\r", ch);
        return;
    }

    if ((victim = get_char_world(ch, arg)) == NULL)
    {
        send_to_char("Target not found.\n\r", ch);
        return;
    }

    if (victim->getTrust() >= ch->getTrust() && ch->getTrust() != MAX_LEVEL)
        send_to_char("personal> ", victim);

    send_to_char(argument, victim);
    send_to_char("\n\r", victim);
    send_to_char("personal> ", ch);
    send_to_char(argument, ch);
    send_to_char("\n\r", ch);
}

void do_transfer(Character *ch, char *argument)
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    ROOM_INDEX_DATA *location;
    DESCRIPTOR_DATA *d;
    Character *victim;

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);

    if (arg1[0] == '\0')
    {
        send_to_char("Transfer whom (and where)?\n\r", ch);
        return;
    }

    if (!str_cmp(arg1, "all"))
    {
        for (d = descriptor_list; d != NULL; d = d->next)
        {
            if (d->connected == ConnectedState::Playing && d->character != ch && d->character->in_room != NULL && ch->can_see(d->character))
            {
                stringstream ss;
                ss << d->character->getName() << " " << arg2;
                do_transfer(ch, ss.str().data());
            }
        }
        return;
    }

    /*
     * Thanks to Grodyn for the optional location parameter.
     */
    if (arg2[0] == '\0')
    {
        location = ch->in_room;
    }
    else
    {
        if ((location = find_location(ch, arg2)) == NULL)
        {
            send_to_char("No such location.\n\r", ch);
            return;
        }

        if (!is_room_owner(ch, location) && room_is_private(location) && ch->getTrust() < MAX_LEVEL)
        {
            send_to_char("That room is private right now.\n\r", ch);
            return;
        }

        /* Can't go to the note room */
        if (is_note_room(location))
        {
            send_to_char("That is the room that characters are sent to for note writing.\n\r", ch);
            send_to_char("Entering that room would crash the mud if there were people\n\r", ch);
            send_to_char("in it.  Command will NOT be completed.\n\r", ch);
            return;
        }
    }

    if ((victim = get_char_world(ch, arg1)) == NULL)
    {
        send_to_char("They aren't here.\n\r", ch);
        return;
    }

    if (victim->in_room == NULL)
    {
        send_to_char("They are in limbo.\n\r", ch);
        return;
    }

    if (victim->in_room == get_room_index(ROOM_VNUM_NOTE))
    {
        send_to_char("They are writing a note.\n\r", ch);
        return;
    }

    if (victim->fighting != NULL)
        stop_fighting(victim, TRUE);
    act("$n disappears in a mushroom cloud.", victim, NULL, NULL, TO_ROOM, POS_RESTING);
    char_from_room(victim);
    char_to_room(victim, location);
    act("$n arrives from a puff of smoke.", victim, NULL, NULL, TO_ROOM, POS_RESTING);
    if (ch != victim)
        act("$n has transferred you.", ch, NULL, victim, TO_VICT, POS_RESTING);
    do_look(victim, (char *)"auto");
    send_to_char("Ok.\n\r", ch);
}

void do_at(Character *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    ROOM_INDEX_DATA *location;
    ROOM_INDEX_DATA *original;
    Object *on;
    Character *wch;

    argument = one_argument(argument, arg);

    if (arg[0] == '\0' || argument[0] == '\0')
    {
        send_to_char("At where what?\n\r", ch);
        return;
    }

    if ((location = find_location(ch, arg)) == NULL)
    {
        send_to_char("No such location.\n\r", ch);
        return;
    }

    if (!is_room_owner(ch, location) && room_is_private(location) && ch->getTrust() < MAX_LEVEL)
    {
        send_to_char("That room is private right now.\n\r", ch);
        return;
    }

    /* Can't go to the note room */
    if (location == get_room_index(ROOM_VNUM_NOTE))
    {
        send_to_char("That is the room that characters are sent to for note writing.\n\r", ch);
        send_to_char("Entering that room would crash the mud if there were people\n\r", ch);
        send_to_char("in it.  Command will NOT be completed.\n\r", ch);
        return;
    }

    original = ch->in_room;
    on = ch->onObject();
    char_from_room(ch);
    char_to_room(ch, location);
    interpret(ch, argument);

    /*
     * See if 'ch' still exists before continuing!
     * Handles 'at XXXX quit' case.
     */
    for (std::list<Character *>::iterator it = char_list.begin(); it != char_list.end(); it++)
    {
        wch = *it;
        if (wch == ch)
        {
            char_from_room(ch);
            char_to_room(ch, original);
            ch->getOntoObject(on);
            break;
        }
    }

    return;
}

void do_goto(Character *ch, char *argument)
{
    ROOM_INDEX_DATA *location;
    Character *rch;
    int count = 0;

    if (argument[0] == '\0')
    {
        send_to_char("Goto where?\n\r", ch);
        return;
    }

    if ((location = find_location(ch, argument)) == NULL)
    {
        send_to_char("No such location.\n\r", ch);
        return;
    }

    count = 0;
    for (rch = location->people; rch != NULL; rch = rch->next_in_room)
        count++;

    if (!is_room_owner(ch, location) && room_is_private(location) && (count > 1 || ch->getTrust() < MAX_LEVEL))
    {
        send_to_char("That room is private right now.\n\r", ch);
        return;
    }

    if (ch->fighting != NULL)
        stop_fighting(ch, TRUE);

    for (rch = ch->in_room->people; rch != NULL; rch = rch->next_in_room)
    {
        if (rch->getTrust() >= ch->invis_level)
        {
            if (!ch->isNPC() && ((PlayerCharacter*)ch)->bamfout[0] != '\0')
                act("$t", ch, ((PlayerCharacter*)ch)->bamfout, rch, TO_VICT, POS_RESTING);
            else
                act("$n leaves in a swirling mist.", ch, NULL, rch, TO_VICT, POS_RESTING);
        }
    }

    /* Can't go to the note room */
    if (location == get_room_index(ROOM_VNUM_NOTE))
    {
        send_to_char("That is the room that characters are sent to for note writing.\n\r", ch);
        send_to_char("Entering that room would crash the mud if there were people\n\r", ch);
        send_to_char("in it.  Command will NOT be completed.\n\r", ch);
        return;
    }
    char_from_room(ch);
    char_to_room(ch, location);

    for (rch = ch->in_room->people; rch != NULL; rch = rch->next_in_room)
    {
        if (rch->getTrust() >= ch->invis_level)
        {
            if (!ch->isNPC() && ((PlayerCharacter*)ch)->bamfin[0] != '\0')
                act("$t", ch, ((PlayerCharacter*)ch)->bamfin, rch, TO_VICT, POS_RESTING);
            else
                act("$n appears in a swirling mist.", ch, NULL, rch, TO_VICT, POS_RESTING);
        }
    }

    do_look(ch, (char *)"auto");
    return;
}

void do_violate(Character *ch, char *argument)
{
    ROOM_INDEX_DATA *location;
    Character *rch;

    if (argument[0] == '\0')
    {
        send_to_char("Goto where?\n\r", ch);
        return;
    }

    if ((location = find_location(ch, argument)) == NULL)
    {
        send_to_char("No such location.\n\r", ch);
        return;
    }

    if (!room_is_private(location))
    {
        send_to_char("That room isn't private, use goto.\n\r", ch);
        return;
    }

    if (ch->fighting != NULL)
        stop_fighting(ch, TRUE);

    for (rch = ch->in_room->people; rch != NULL; rch = rch->next_in_room)
    {
        if (rch->getTrust() >= ch->invis_level)
        {
            if (!ch->isNPC() && ((PlayerCharacter*)ch)->bamfout[0] != '\0')
                act("$t", ch, ((PlayerCharacter*)ch)->bamfout, rch, TO_VICT, POS_RESTING);
            else
                act("$n leaves in a swirling mist.", ch, NULL, rch, TO_VICT, POS_RESTING);
        }
    }

    /* Can't go to the note room */
    if (location == get_room_index(ROOM_VNUM_NOTE))
    {
        send_to_char("That is the room that characters are sent to for note writing.\n\r", ch);
        send_to_char("Entering that room would crash the mud if there were people\n\r", ch);
        send_to_char("in it.  Command will NOT be completed.\n\r", ch);
        return;
    }
    char_from_room(ch);
    char_to_room(ch, location);

    for (rch = ch->in_room->people; rch != NULL; rch = rch->next_in_room)
    {
        if (rch->getTrust() >= ch->invis_level)
        {
            if (!ch->isNPC() && ((PlayerCharacter*)ch)->bamfin[0] != '\0')
                act("$t", ch, ((PlayerCharacter*)ch)->bamfin, rch, TO_VICT, POS_RESTING);
            else
                act("$n appears in a swirling mist.", ch, NULL, rch, TO_VICT, POS_RESTING);
        }
    }

    do_look(ch, (char *)"auto");
    return;
}

/* RT to replace the 3 stat commands */

void do_stat(Character *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    char *string;
    Object *obj;
    ROOM_INDEX_DATA *location;
    Character *victim;

    string = one_argument(argument, arg);
    if (arg[0] == '\0')
    {
        send_to_char("Syntax:\n\r", ch);
        send_to_char("  stat <name>\n\r", ch);
        send_to_char("  stat obj <name>\n\r", ch);
        send_to_char("  stat mob <name>\n\r", ch);
        send_to_char("  stat room <number>\n\r", ch);
        return;
    }

    if (!str_cmp(arg, "room"))
    {
        do_rstat(ch, string);
        return;
    }

    if (!str_cmp(arg, "obj"))
    {
        do_ostat(ch, string);
        return;
    }

    if (!str_cmp(arg, "char") || !str_cmp(arg, "mob"))
    {
        do_mstat(ch, string);
        return;
    }

    /* do it the old way */

    obj = get_obj_world(ch, argument);
    if (obj != NULL)
    {
        do_ostat(ch, argument);
        return;
    }

    victim = get_char_world(ch, argument);
    if (victim != NULL)
    {
        do_mstat(ch, argument);
        return;
    }

    location = find_location(ch, argument);
    if (location != NULL)
    {
        do_rstat(ch, argument);
        return;
    }

    send_to_char("Nothing by that name found anywhere.\n\r", ch);
}

void do_rstat(Character *ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    ROOM_INDEX_DATA *location;
    Character *rch;
    int door;

    one_argument(argument, arg);
    location = (arg[0] == '\0') ? ch->in_room : find_location(ch, arg);
    if (location == NULL)
    {
        send_to_char("No such location.\n\r", ch);
        return;
    }

    if (!is_room_owner(ch, location) && ch->in_room != location && room_is_private(location) && !IS_TRUSTED(ch, IMPLEMENTOR))
    {
        send_to_char("That room is private right now.\n\r", ch);
        return;
    }

    snprintf(buf, sizeof(buf), "Name: '%s'\n\rArea: '%s'\n\r",
             location->name,
             location->area->name);
    send_to_char(buf, ch);

    snprintf(buf, sizeof(buf),
             "Vnum: %d  Sector: %d  Light: %d  Healing: %d  Mana: %d\n\r",
             location->vnum,
             location->sector_type,
             location->light,
             location->heal_rate,
             location->mana_rate);
    send_to_char(buf, ch);

    snprintf(buf, sizeof(buf),
             "Room flags: %d.\n\rDescription:\n\r%s",
             location->room_flags,
             location->description);
    send_to_char(buf, ch);

    if (!location->extra_descr.empty())
    {
        send_to_char("Extra description keywords: '", ch);
        bool first = true;
        for (auto ed : location->extra_descr)
        {
            send_to_char(ed->getKeyword().c_str(), ch);
            if (!first)
            {
                send_to_char(" ", ch);
            }
            else
            {
                first = false;
            }
        }
        send_to_char("'.\n\r", ch);
    }

    send_to_char("Characters:", ch);
    for (rch = location->people; rch; rch = rch->next_in_room)
    {
        if (ch->can_see(rch))
        {
            send_to_char(" ", ch);
            one_argument(rch->getName().data(), buf);
            send_to_char(buf, ch);
        }
    }

    send_to_char(".\n\rObjects:   ", ch);
    for (auto obj : location->contents)
    {
        send_to_char(" ", ch);
        one_argument(obj->getName().data(), buf);
        send_to_char(buf, ch);
    }
    send_to_char(".\n\r", ch);

    for (door = 0; door <= 5; door++)
    {
        EXIT_DATA *pexit;

        if ((pexit = location->exit[door]) != NULL)
        {
            snprintf(buf, sizeof(buf),
                     "Door: %d.  To: %d.  Key: %d.  Exit flags: %d.\n\rKeyword: '%s'.  Description: %s",

                     door,
                     (pexit->u1.to_room == NULL ? -1 : pexit->u1.to_room->vnum),
                     pexit->key,
                     pexit->exit_info,
                     pexit->keyword,
                     pexit->description[0] != '\0'
                         ? pexit->description
                         : "(none).\n\r");
            send_to_char(buf, ch);
        }
    }

    return;
}

void do_ostat(Character *ch, char *argument)
{
    stringstream ss;
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    AFFECT_DATA *paf;
    Object *obj;

    one_argument(argument, arg);

    if (arg[0] == '\0')
    {
        send_to_char("Stat what?\n\r", ch);
        return;
    }

    if ((obj = get_obj_world(ch, argument)) == NULL)
    {
        send_to_char("Nothing like that in hell, earth, or heaven.\n\r", ch);
        return;
    }

    string format = obj->getObjectIndexData()->new_format ? "new" : "old";
    ss << "Name(s): " << obj->getName() << std::endl;
    ss << "Vnum: " << obj->getObjectIndexData()->vnum << "  Format: " << format << "  Type: " << item_name(obj->getItemType()) << "  Resets: " << obj->getObjectIndexData()->reset_num << std::endl;
    ss << "Short description: " << obj->getShortDescription() << std::endl;
    ss << "Long description: " << obj->getDescription() << std::endl;
    ss << "Wear bits: " << wear_bit_name(obj->getWearFlags()) << std::endl;
    ss << "Extra bits: " << extra_bit_name(obj->getExtraFlags()) << std::endl;
    ss << "Number: 1/" << get_obj_number(obj) << "  Weight: " << obj->getWeight() << "/" << get_obj_weight(obj) << "/" << get_true_weight(obj) << " (10th pounds)" << std::endl;
    ss << "Level: " << obj->getLevel() << "  Cost: " << obj->getCost() << "  Condition: " << obj->getCondition() << "  Timer: " << obj->getTimer() << std::endl;
    ss << "In room: "
       << (obj->getInRoom() == NULL ? 0 : obj->getInRoom()->vnum)
       << "  In object: "
       << (obj->getInObject() == NULL ? "(none)" : obj->getInObject()->getShortDescription())
       << "  Carried by: "
       << (obj->getCarriedBy() == NULL ? "(none)" : ch->can_see(obj->getCarriedBy()) ? obj->getCarriedBy()->getName()
                                                                                     : "someone")
       << "  Wear_loc: " << obj->getWearLocation() << std::endl;
    ss << "Values: "
       << obj->getValues().at(0) << " "
       << obj->getValues().at(1) << " "
       << obj->getValues().at(2) << " "
       << obj->getValues().at(3) << " "
       << obj->getValues().at(4) << std::endl;
    send_to_char(ss.str().c_str(), ch);

    /* now give out vital statistics as per identify */

    switch (obj->getItemType())
    {
    case ITEM_SCROLL:
    case ITEM_POTION:
    case ITEM_PILL:
        snprintf(buf, sizeof(buf), "Level %d spells of:", obj->getValues().at(0));
        send_to_char(buf, ch);

        if (obj->getValues().at(1) >= 0 && obj->getValues().at(1) < MAX_SKILL)
        {
            send_to_char(" '", ch);
            send_to_char(skill_table[obj->getValues().at(1)].name, ch);
            send_to_char("'", ch);
        }

        if (obj->getValues().at(2) >= 0 && obj->getValues().at(2) < MAX_SKILL)
        {
            send_to_char(" '", ch);
            send_to_char(skill_table[obj->getValues().at(2)].name, ch);
            send_to_char("'", ch);
        }

        if (obj->getValues().at(3) >= 0 && obj->getValues().at(3) < MAX_SKILL)
        {
            send_to_char(" '", ch);
            send_to_char(skill_table[obj->getValues().at(3)].name, ch);
            send_to_char("'", ch);
        }

        if (obj->getValues().at(4) >= 0 && obj->getValues().at(4) < MAX_SKILL)
        {
            send_to_char(" '", ch);
            send_to_char(skill_table[obj->getValues().at(4)].name, ch);
            send_to_char("'", ch);
        }

        send_to_char(".\n\r", ch);
        break;

    case ITEM_WAND:
    case ITEM_STAFF:
        snprintf(buf, sizeof(buf), "Has %d(%d) charges of level %d",
                 obj->getValues().at(1), obj->getValues().at(2), obj->getValues().at(0));
        send_to_char(buf, ch);

        if (obj->getValues().at(3) >= 0 && obj->getValues().at(3) < MAX_SKILL)
        {
            send_to_char(" '", ch);
            send_to_char(skill_table[obj->getValues().at(3)].name, ch);
            send_to_char("'", ch);
        }

        send_to_char(".\n\r", ch);
        break;

    case ITEM_DRINK_CON:
        snprintf(buf, sizeof(buf), "It holds %s-colored %s.\n\r",
                 liq_table[obj->getValues().at(2)].liq_color,
                 liq_table[obj->getValues().at(2)].liq_name);
        send_to_char(buf, ch);
        break;

    case ITEM_WEAPON:
        send_to_char("Weapon type is ", ch);
        switch (obj->getValues().at(0))
        {
        case (WEAPON_EXOTIC):
            send_to_char("exotic\n\r", ch);
            break;
        case (WEAPON_SWORD):
            send_to_char("sword\n\r", ch);
            break;
        case (WEAPON_DAGGER):
            send_to_char("dagger\n\r", ch);
            break;
        case (WEAPON_SPEAR):
            send_to_char("spear/staff\n\r", ch);
            break;
        case (WEAPON_MACE):
            send_to_char("mace/club\n\r", ch);
            break;
        case (WEAPON_AXE):
            send_to_char("axe\n\r", ch);
            break;
        case (WEAPON_FLAIL):
            send_to_char("flail\n\r", ch);
            break;
        case (WEAPON_WHIP):
            send_to_char("whip\n\r", ch);
            break;
        case (WEAPON_POLEARM):
            send_to_char("polearm\n\r", ch);
            break;
        default:
            send_to_char("unknown\n\r", ch);
            break;
        }
        if (obj->getObjectIndexData()->new_format)
            snprintf(buf, sizeof(buf), "Damage is %dd%d (average %d)\n\r",
                     obj->getValues().at(1), obj->getValues().at(2),
                     (1 + obj->getValues().at(2)) * obj->getValues().at(1) / 2);
        else
            snprintf(buf, sizeof(buf), "Damage is %d to %d (average %d)\n\r",
                     obj->getValues().at(1), obj->getValues().at(2),
                     (obj->getValues().at(1) + obj->getValues().at(2)) / 2);
        send_to_char(buf, ch);

        snprintf(buf, sizeof(buf), "Damage noun is %s.\n\r",
                 (obj->getValues().at(3) > 0 && obj->getValues().at(3) < MAX_DAMAGE_MESSAGE) ? attack_table[obj->getValues().at(3)].noun : "undefined");
        send_to_char(buf, ch);

        if (obj->getValues().at(4)) /* weapon flags */
        {
            snprintf(buf, sizeof(buf), "Weapons flags: %s\n\r",
                     weapon_bit_name(obj->getValues().at(4)));
            send_to_char(buf, ch);
        }
        break;

    case ITEM_BOW:
        snprintf(buf, sizeof(buf), "Weapon range is %d.\n\r", obj->getValues().at(0));
        send_to_char(buf, ch);

        if (obj->getObjectIndexData()->new_format)
            snprintf(buf, sizeof(buf), "Damage is %dd%d (average %d)\n\r",
                     obj->getValues().at(1), obj->getValues().at(2),
                     (1 + obj->getValues().at(2)) * obj->getValues().at(1) / 2);
        else
            snprintf(buf, sizeof(buf), "Damage is %d to %d (average %d)\n\r",
                     obj->getValues().at(1), obj->getValues().at(2),
                     (obj->getValues().at(1) + obj->getValues().at(2)) / 2);
        send_to_char(buf, ch);

        snprintf(buf, sizeof(buf), "Damage noun is %s.\n\r",
                 (obj->getValues().at(3) > 0 && obj->getValues().at(3) < MAX_DAMAGE_MESSAGE) ? attack_table[obj->getValues().at(3)].noun : "undefined");
        send_to_char(buf, ch);

        if (obj->getValues().at(4)) /* weapon flags */
        {
            snprintf(buf, sizeof(buf), "Weapons flags: %s\n\r",
                     weapon_bit_name(obj->getValues().at(4)));
            send_to_char(buf, ch);
        }
        break;

    case ITEM_ARMOR:
        snprintf(buf, sizeof(buf),
                 "Armor class is %d pierce, %d bash, %d slash, and %d vs. magic\n\r",
                 obj->getValues().at(0), obj->getValues().at(1), obj->getValues().at(2), obj->getValues().at(3));
        send_to_char(buf, ch);
        break;

    case ITEM_CONTAINER:
        snprintf(buf, sizeof(buf), "Capacity: %d#  Maximum weight: %d#  flags: %s\n\r",
                 obj->getValues().at(0), obj->getValues().at(3), cont_bit_name(obj->getValues().at(1)));
        send_to_char(buf, ch);
        if (obj->getValues().at(4) != 100)
        {
            snprintf(buf, sizeof(buf), "Weight multiplier: %d%%\n\r",
                     obj->getValues().at(4));
            send_to_char(buf, ch);
        }
        break;
    }

    if (!obj->getExtraDescriptions().empty())
    {
        send_to_char("Extra description keywords: '", ch);

        int i = 0;
        for (auto ed : obj->getExtraDescriptions())
        {
            if (i > 0)
            {
                send_to_char(" ", ch);
            }
            send_to_char(ed->getKeyword().c_str(), ch);
        }

        i = 0;
        for (auto ed : obj->getObjectIndexData()->extra_descr)
        {
            if (i > 0)
            {
                send_to_char(" ", ch);
            }
            send_to_char(ed->getKeyword().c_str(), ch);
        }

        send_to_char("'\n\r", ch);
    }

    for (auto paf : obj->getAffectedBy())
    {
        snprintf(buf, sizeof(buf), "Affects %s by %d, level %d",
                 affect_loc_name(paf->location), paf->modifier, paf->level);
        send_to_char(buf, ch);
        if (paf->duration > -1)
            snprintf(buf, sizeof(buf), ", %d hours.\n\r", paf->duration);
        else
            snprintf(buf, sizeof(buf), ".\n\r");
        send_to_char(buf, ch);
        if (paf->bitvector)
        {
            switch (paf->where)
            {
            case TO_AFFECTS:
                snprintf(buf, sizeof(buf), "Adds %s affect.\n",
                         affect_bit_name(paf->bitvector));
                break;
            case TO_WEAPON:
                snprintf(buf, sizeof(buf), "Adds %s weapon flags.\n",
                         weapon_bit_name(paf->bitvector));
                break;
            case TO_OBJECT:
                snprintf(buf, sizeof(buf), "Adds %s object flag.\n",
                         extra_bit_name(paf->bitvector));
                break;
            case TO_IMMUNE:
                snprintf(buf, sizeof(buf), "Adds immunity to %s.\n",
                         imm_bit_name(paf->bitvector));
                break;
            case TO_RESIST:
                snprintf(buf, sizeof(buf), "Adds resistance to %s.\n\r",
                         imm_bit_name(paf->bitvector));
                break;
            case TO_VULN:
                snprintf(buf, sizeof(buf), "Adds vulnerability to %s.\n\r",
                         imm_bit_name(paf->bitvector));
                break;
            default:
                snprintf(buf, sizeof(buf), "Unknown bit %d: %d\n\r",
                         paf->where, paf->bitvector);
                break;
            }
            send_to_char(buf, ch);
        }
    }

    if (!obj->isEnchanted())
    {
        for (auto paf : obj->getObjectIndexData()->affected)
        {
            snprintf(buf, sizeof(buf), "Affects %s by %d, level %d.\n\r",
                     affect_loc_name(paf->location), paf->modifier, paf->level);
            send_to_char(buf, ch);
            if (paf->bitvector)
            {
                switch (paf->where)
                {
                case TO_AFFECTS:
                    snprintf(buf, sizeof(buf), "Adds %s affect.\n",
                             affect_bit_name(paf->bitvector));
                    break;
                case TO_OBJECT:
                    snprintf(buf, sizeof(buf), "Adds %s object flag.\n",
                             extra_bit_name(paf->bitvector));
                    break;
                case TO_IMMUNE:
                    snprintf(buf, sizeof(buf), "Adds immunity to %s.\n",
                             imm_bit_name(paf->bitvector));
                    break;
                case TO_RESIST:
                    snprintf(buf, sizeof(buf), "Adds resistance to %s.\n\r",
                             imm_bit_name(paf->bitvector));
                    break;
                case TO_VULN:
                    snprintf(buf, sizeof(buf), "Adds vulnerability to %s.\n\r",
                             imm_bit_name(paf->bitvector));
                    break;
                default:
                    snprintf(buf, sizeof(buf), "Unknown bit %d: %d\n\r",
                             paf->where, paf->bitvector);
                    break;
                }
                send_to_char(buf, ch);
            }
        }
    }

    return;
}

void do_mstat(Character *ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    AFFECT_DATA *paf;
    Character *victim;

    one_argument(argument, arg);

    if (arg[0] == '\0')
    {
        send_to_char("Stat whom?\n\r", ch);
        return;
    }

    if ((victim = get_char_world(ch, argument)) == NULL)
    {
        send_to_char("They aren't here.\n\r", ch);
        return;
    }

    stringstream ss;
    ss << "Name: " << victim->getName() << "\n\r";
    send_to_char(ss.str().c_str(), ch);

    snprintf(buf, sizeof(buf),
             "Vnum: %d  Format: %s  Race: %s  Group: %d  Sex: %s  Room: %d\n\r",
             IS_NPC(victim) ? victim->pIndexData->vnum : 0,
             IS_NPC(victim) ? victim->pIndexData->new_format ? "new" : "old" : "pc",
             victim->getRace()->getName().c_str(),
             IS_NPC(victim) ? victim->group : 0, sex_table[victim->sex].name,
             victim->in_room == NULL ? 0 : victim->in_room->vnum);
    send_to_char(buf, ch);

    if (IS_NPC(victim))
    {
        snprintf(buf, sizeof(buf), "Count: %d  Killed: %d\n\r",
                 victim->pIndexData->count, victim->pIndexData->killed);
        send_to_char(buf, ch);
    }

    snprintf(buf, sizeof(buf),
             "Str: %d(%d)  Int: %d(%d)  Wis: %d(%d)  Dex: %d(%d)  Con: %d(%d)\n\r",
             victim->perm_stat[STAT_STR],
             get_curr_stat(victim, STAT_STR),
             victim->perm_stat[STAT_INT],
             get_curr_stat(victim, STAT_INT),
             victim->perm_stat[STAT_WIS],
             get_curr_stat(victim, STAT_WIS),
             victim->perm_stat[STAT_DEX],
             get_curr_stat(victim, STAT_DEX),
             victim->perm_stat[STAT_CON],
             get_curr_stat(victim, STAT_CON));
    send_to_char(buf, ch);

    snprintf(buf, sizeof(buf), "Hp: %d/%d  Mana: %d/%d  Move: %d/%d  Practices: %d\n\r",
             victim->hit, victim->max_hit,
             victim->mana, victim->max_mana,
             victim->move, victim->max_move,
             IS_NPC(victim) ? 0 : victim->practice);
    send_to_char(buf, ch);

    snprintf(buf, sizeof(buf),
             "Lv: %d  Class: %s  Align: %d  Gold: %ld  Silver: %ld  Exp: %d\n\r",
             victim->level,
             IS_NPC(victim) ? "mobile" : class_table[victim->class_num].name,
             victim->alignment,
             victim->gold, victim->silver, victim->exp);
    send_to_char(buf, ch);

    snprintf(buf, sizeof(buf), "Armor: pierce: %d  bash: %d  slash: %d  magic: %d\n\r",
             GET_AC(victim, AC_PIERCE), GET_AC(victim, AC_BASH),
             GET_AC(victim, AC_SLASH), GET_AC(victim, AC_EXOTIC));
    send_to_char(buf, ch);

    snprintf(buf, sizeof(buf),
             "Hit: %d  Dam: %d  Saves: %d  Size: %s  Position: %s  Wimpy: %d\n\r",
             GET_HITROLL(victim), GET_DAMROLL(victim), victim->saving_throw,
             size_table[victim->size].name, position_table[victim->position].name,
             victim->wimpy);
    send_to_char(buf, ch);

    if (!victim->isNPC())
    {
        snprintf(buf, sizeof(buf),
                 "Mobkillls: %d  Mobkilled: %d  Pkills: %d  Pkilled: %d\n\r",
                 ((PlayerCharacter *)victim)->getMobKillCount(),
                 ((PlayerCharacter *)victim)->getKilledByMobCount(),
                 ((PlayerCharacter *)victim)->getPlayerKillCount(),
                 ((PlayerCharacter *)victim)->getKilledByPlayersCount());
        send_to_char(buf, ch);
    }

    if (IS_NPC(victim) && victim->pIndexData->new_format)
    {
        snprintf(buf, sizeof(buf), "Damage: %dd%d  Message:  %s\n\r",
                 victim->damage[DICE_NUMBER], victim->damage[DICE_TYPE],
                 attack_table[victim->dam_type].noun);
        send_to_char(buf, ch);
    }
    snprintf(buf, sizeof(buf), "Hunting: %s\n\r",
             victim->hunting ? victim->hunting->getName().c_str() : "(none)");
    send_to_char(buf, ch);

    snprintf(buf, sizeof(buf), "Fighting: %s\n\r",
             victim->fighting ? victim->fighting->getName().c_str() : "(none)");
    send_to_char(buf, ch);

    if (!IS_NPC(victim))
    {
        snprintf(buf, sizeof(buf),
                 "Thirst: %d  Hunger: %d  Full: %d  Drunk: %d\n\r",
                 ((PlayerCharacter*)victim)->condition[COND_THIRST],
                 ((PlayerCharacter*)victim)->condition[COND_HUNGER],
                 ((PlayerCharacter*)victim)->condition[COND_FULL],
                 ((PlayerCharacter*)victim)->condition[COND_DRUNK]);
        send_to_char(buf, ch);
    }

    snprintf(buf, sizeof(buf), "Carry number: %d  Carry weight: %ld\n\r",
             victim->carry_number, get_carry_weight(victim) / 10);
    send_to_char(buf, ch);

    if (!IS_NPC(victim))
    {
        snprintf(buf, sizeof(buf),
                 "Age: %d  Played: %d  Last Level: %d  Timer: %d\n\r",
                 get_age(victim),
                 (int)(victim->played + current_time - victim->logon) / 3600,
                 ((PlayerCharacter*)victim)->last_level,
                 victim->timer);
        send_to_char(buf, ch);
    }

    snprintf(buf, sizeof(buf), "Act: %s\n\r", act_bit_name(victim->act));
    send_to_char(buf, ch);

    if (victim->comm)
    {
        snprintf(buf, sizeof(buf), "Comm: %s\n\r", comm_bit_name(victim->comm));
        send_to_char(buf, ch);
    }

    if (IS_NPC(victim) && victim->off_flags)
    {
        snprintf(buf, sizeof(buf), "Offense: %s\n\r", off_bit_name(victim->off_flags));
        send_to_char(buf, ch);
    }

    if (victim->imm_flags)
    {
        snprintf(buf, sizeof(buf), "Immune: %s\n\r", imm_bit_name(victim->imm_flags));
        send_to_char(buf, ch);
    }

    if (victim->res_flags)
    {
        snprintf(buf, sizeof(buf), "Resist: %s\n\r", imm_bit_name(victim->res_flags));
        send_to_char(buf, ch);
    }

    if (victim->vuln_flags)
    {
        snprintf(buf, sizeof(buf), "Vulnerable: %s\n\r", imm_bit_name(victim->vuln_flags));
        send_to_char(buf, ch);
    }

    snprintf(buf, sizeof(buf), "Form: %s\n\rParts: %s\n\r",
             form_bit_name(victim->form), part_bit_name(victim->parts));
    send_to_char(buf, ch);

    if (victim->affected_by)
    {
        snprintf(buf, sizeof(buf), "Affected by %s\n\r",
                 affect_bit_name(victim->affected_by));
        send_to_char(buf, ch);
    }

    snprintf(buf, sizeof(buf), "Master: %s  Leader: %s  Pet: %s\n\r",
             victim->master ? victim->master->getName().c_str() : "(none)",
             victim->leader ? victim->leader->getName().c_str() : "(none)",
             victim->pet ? victim->pet->getName().c_str() : "(none)");
    send_to_char(buf, ch);

    snprintf(buf, sizeof(buf), "Short description: %s\n\rLong  description: %s",
             victim->short_descr,
             victim->long_descr[0] != '\0' ? victim->long_descr : "(none)\n\r");
    send_to_char(buf, ch);

    if (IS_NPC(victim) && victim->spec_fun != 0)
    {
        snprintf(buf, sizeof(buf), "Mobile has special procedure %s.\n\r",
                 spec_name(victim->spec_fun));
        send_to_char(buf, ch);
    }

    for (auto paf : victim->affected)
    {
        snprintf(buf, sizeof(buf),
                 "Spell: '%s' modifies %s by %d for %d hours with bits %s, level %d.\n\r",
                 skill_table[(int)paf->type].name,
                 affect_loc_name(paf->location),
                 paf->modifier,
                 paf->duration,
                 affect_bit_name(paf->bitvector),
                 paf->level);
        send_to_char(buf, ch);
    }

    return;
}

/* ofind and mfind replaced with vnum, vnum skill also added */

void do_vnum(Character *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    char *string;

    string = one_argument(argument, arg);

    if (arg[0] == '\0')
    {
        send_to_char("Syntax:\n\r", ch);
        send_to_char("  vnum obj <name>\n\r", ch);
        send_to_char("  vnum mob <name>\n\r", ch);
        send_to_char("  vnum skill <skill or spell>\n\r", ch);
        return;
    }

    if (!str_cmp(arg, "obj"))
    {
        do_ofind(ch, string);
        return;
    }

    if (!str_cmp(arg, "mob") || !str_cmp(arg, "char"))
    {
        do_mfind(ch, string);
        return;
    }

    if (!str_cmp(arg, "skill") || !str_cmp(arg, "spell"))
    {
        do_slookup(ch, string);
        return;
    }
    /* do both */
    do_mfind(ch, argument);
    do_ofind(ch, argument);
}

void do_mfind(Character *ch, char *argument)
{
    extern int top_mob_index;
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    MOB_INDEX_DATA *pMobIndex;
    int vnum;
    int nMatch;
    bool fAll;
    bool found;

    one_argument(argument, arg);
    if (arg[0] == '\0')
    {
        send_to_char("Find whom?\n\r", ch);
        return;
    }

    fAll = FALSE; /* !str_cmp( arg, "all" ); */
    found = FALSE;
    nMatch = 0;

    /*
     * Yeah, so iterating over all vnum's takes 10,000 loops.
     * Get_mob_index is fast, and I don't feel like threading another link.
     * Do you?
     * -- Furey
     */
    for (vnum = 0; nMatch < top_mob_index; vnum++)
    {
        if ((pMobIndex = get_mob_index(vnum)) != NULL)
        {
            nMatch++;
            if (fAll || is_name(argument, pMobIndex->player_name))
            {
                found = TRUE;
                snprintf(buf, sizeof(buf), "[%5d] %s\n\r",
                         pMobIndex->vnum, pMobIndex->short_descr);
                send_to_char(buf, ch);
            }
        }
    }

    if (!found)
        send_to_char("No mobiles by that name.\n\r", ch);

    return;
}

void do_ofind(Character *ch, char *argument)
{
    extern int top_obj_index;
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    OBJ_INDEX_DATA *pObjIndex;
    int vnum;
    int nMatch;
    bool fAll;
    bool found;

    one_argument(argument, arg);
    if (arg[0] == '\0')
    {
        send_to_char("Find what?\n\r", ch);
        return;
    }

    fAll = FALSE; /* !str_cmp( arg, "all" ); */
    found = FALSE;
    nMatch = 0;

    /*
     * Yeah, so iterating over all vnum's takes 10,000 loops.
     * Get_obj_index is fast, and I don't feel like threading another link.
     * Do you?
     * -- Furey
     */
    for (vnum = 0; nMatch < top_obj_index; vnum++)
    {
        if ((pObjIndex = get_obj_index(vnum)) != NULL)
        {
            nMatch++;
            if (fAll || is_name(argument, pObjIndex->name))
            {
                found = TRUE;
                snprintf(buf, sizeof(buf), "[%5d] %s\n\r",
                         pObjIndex->vnum, pObjIndex->short_descr);
                send_to_char(buf, ch);
            }
        }
    }

    if (!found)
        send_to_char("No objects by that name.\n\r", ch);

    return;
}

void do_owhere(Character *ch, char *argument)
{
    char buf[MAX_INPUT_LENGTH];
    BUFFER *buffer;
    Object *in_obj;
    bool found;
    int number = 0, max_found;

    found = FALSE;
    number = 0;
    max_found = 200;

    buffer = new_buf();

    if (argument[0] == '\0')
    {
        send_to_char("Find what?\n\r", ch);
        return;
    }

    for (auto obj : object_list)
    {
        if (!ch->can_see(obj) || !is_name(argument, obj->getName().c_str()) || ch->level < obj->getLevel())
            continue;

        found = TRUE;
        number++;

        for (in_obj = obj; in_obj->getInObject() != NULL; in_obj = in_obj->getInObject())
            ;

        if (in_obj->getCarriedBy() != NULL && ch->can_see(in_obj->getCarriedBy()) && in_obj->getCarriedBy()->in_room != NULL)
        {
            snprintf(buf, sizeof(buf), "%3d) %s is carried by %s [Room %d]\n\r",
                     number, obj->getShortDescription().c_str(), PERS(in_obj->getCarriedBy(), ch),
                     in_obj->getCarriedBy()->in_room->vnum);
        }
        else if (in_obj->getInRoom() != NULL && can_see_room(ch, in_obj->getInRoom()))
        {
            snprintf(buf, sizeof(buf), "%3d) %s is in %s [Room %d]\n\r",
                     number, obj->getShortDescription().c_str(), in_obj->getInRoom()->name,
                     in_obj->getInRoom()->vnum);
        }
        else
        {
            snprintf(buf, sizeof(buf), "%3d) %s is somewhere\n\r", number, obj->getShortDescription().c_str());
        }

        buf[0] = UPPER(buf[0]);
        add_buf(buffer, buf);

        if (number >= max_found)
            break;
    }

    if (!found)
        send_to_char("Nothing like that in heaven or earth.\n\r", ch);
    else
        page_to_char(buf_string(buffer), ch);

    free_buf(buffer);
}

void do_mwhere(Character *ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    BUFFER *buffer;
    Character *victim;
    bool found;
    int count = 0;

    if (argument[0] == '\0')
    {
        DESCRIPTOR_DATA *d;

        /* show characters logged */

        buffer = new_buf();
        for (d = descriptor_list; d != NULL; d = d->next)
        {
            if (d->character != NULL && d->connected == ConnectedState::Playing && d->character->in_room != NULL && ch->can_see(d->character) && can_see_room(ch, d->character->in_room))
            {
                victim = d->character;
                count++;
                if (d->original != NULL)
                    snprintf(buf, sizeof(buf), "%3d) %s (in the body of %s) is in %s [%d]\n\r",
                             count, d->original->getName().c_str(), victim->short_descr,
                             victim->in_room->name, victim->in_room->vnum);
                else
                    snprintf(buf, sizeof(buf), "%3d) %s is in %s [%d]\n\r",
                             count, victim->getName().c_str(), victim->in_room->name,
                             victim->in_room->vnum);
                add_buf(buffer, buf);
            }
        }

        page_to_char(buf_string(buffer), ch);
        free_buf(buffer);
        return;
    }

    found = FALSE;
    buffer = new_buf();
    for (std::list<Character *>::iterator it = char_list.begin(); it != char_list.end(); it++)
    {
        victim = *it;
        if (victim->in_room != NULL && is_name(argument, victim->getName().c_str()))
        {
            found = TRUE;
            count++;
            snprintf(buf, sizeof(buf), "%3d) [%5d] %-28s [%5d] %s\n\r", count,
                     IS_NPC(victim) ? victim->pIndexData->vnum : 0,
                     IS_NPC(victim) ? victim->short_descr : victim->getName().c_str(),
                     victim->in_room->vnum,
                     victim->in_room->name);
            add_buf(buffer, buf);
        }
    }

    if (!found)
        act("You didn't find any $T.", ch, NULL, argument, TO_CHAR, POS_RESTING);
    else
        page_to_char(buf_string(buffer), ch);

    free_buf(buffer);

    return;
}

void do_reboo(Character *ch, char *argument)
{
    send_to_char("If you want to REBOOT, spell it out.\n\r", ch);
    return;
}

void do_reboot(Character *ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    extern bool merc_down;
    DESCRIPTOR_DATA *d, *d_next;
    Character *vch;

    if (ch->invis_level < LEVEL_HERO)
    {
        snprintf(buf, sizeof(buf), "Reboot by %s.", ch->getName().c_str());
        do_echo(ch, buf);
    }

    merc_down = TRUE;
    for (d = descriptor_list; d != NULL; d = d_next)
    {
        d_next = d->next;
        vch = d->original ? d->original : d->character;
        if (vch != NULL)
            save_char_obj(vch);
        SocketHelper::close_socket(d);
    }

    return;
}

void do_shutdow(Character *ch, char *argument)
{
    send_to_char("If you want to SHUTDOWN, spell it out.\n\r", ch);
    return;
}

void do_shutdown(Character *ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    extern bool merc_down;
    DESCRIPTOR_DATA *d, *d_next;
    Character *vch;

    if (ch->invis_level < LEVEL_HERO)
        snprintf(buf, sizeof(buf), "Shutdown by %s.", ch->getName().c_str());
    append_file(ch, SHUTDOWN_FILE, buf);
    strcat(buf, "\n\r");
    if (ch->invis_level < LEVEL_HERO)
        do_echo(ch, buf);
    merc_down = TRUE;
    for (d = descriptor_list; d != NULL; d = d_next)
    {
        d_next = d->next;
        vch = d->original ? d->original : d->character;
        if (vch != NULL)
            save_char_obj(vch);
        SocketHelper::close_socket(d);
    }
    return;
}

void do_protect(Character *ch, char *argument)
{
    Character *victim;

    if (argument[0] == '\0')
    {
        send_to_char("Protect whom from snooping?\n\r", ch);
        return;
    }

    if ((victim = get_char_world(ch, argument)) == NULL)
    {
        send_to_char("You can't find them.\n\r", ch);
        return;
    }

    if (IS_SET(victim->comm, COMM_SNOOP_PROOF))
    {
        act("$N is no longer snoop-proof.", ch, NULL, victim, TO_CHAR, POS_DEAD);
        send_to_char("Your snoop-proofing was just removed.\n\r", victim);
        REMOVE_BIT(victim->comm, COMM_SNOOP_PROOF);
    }
    else
    {
        act("$N is now snoop-proof.", ch, NULL, victim, TO_CHAR, POS_DEAD);
        send_to_char("You are now immune to snooping.\n\r", victim);
        SET_BIT(victim->comm, COMM_SNOOP_PROOF);
    }
}

void do_snoop(Character *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    DESCRIPTOR_DATA *d;
    Character *victim;
    char buf[MAX_STRING_LENGTH];

    one_argument(argument, arg);

    if (arg[0] == '\0')
    {
        send_to_char("Snoop whom?\n\r", ch);
        return;
    }

    if ((victim = get_char_world(ch, arg)) == NULL)
    {
        send_to_char("They aren't here.\n\r", ch);
        return;
    }

    if (victim->desc == NULL)
    {
        send_to_char("No descriptor to snoop.\n\r", ch);
        return;
    }

    if (victim == ch)
    {
        send_to_char("Cancelling all snoops.\n\r", ch);
        Wiznet::instance()->report("$N stops being such a snoop.",
                                   ch, NULL, WIZ_SNOOPS, WIZ_SECURE, ch->getTrust());
        for (d = descriptor_list; d != NULL; d = d->next)
        {
            if (d->snoop_by == ch->desc)
                d->snoop_by = NULL;
        }
        return;
    }

    if (victim->desc->snoop_by != NULL)
    {
        send_to_char("Busy already.\n\r", ch);
        return;
    }

    if (!is_room_owner(ch, victim->in_room) && ch->in_room != victim->in_room && room_is_private(victim->in_room) && !IS_TRUSTED(ch, IMPLEMENTOR))
    {
        send_to_char("That character is in a private room.\n\r", ch);
        return;
    }

    if (victim->getTrust() >= ch->getTrust() || IS_SET(victim->comm, COMM_SNOOP_PROOF))
    {
        send_to_char("You failed.\n\r", ch);
        return;
    }

    if (ch->desc != NULL)
    {
        for (d = ch->desc->snoop_by; d != NULL; d = d->snoop_by)
        {
            if (d->character == victim || d->original == victim)
            {
                send_to_char("No snoop loops.\n\r", ch);
                return;
            }
        }
    }

    victim->desc->snoop_by = ch->desc;
    snprintf(buf, sizeof(buf), "$N starts snooping on %s",
             (ch->isNPC() ? victim->short_descr : victim->getName().c_str()));
    Wiznet::instance()->report(buf, ch, NULL, WIZ_SNOOPS, WIZ_SECURE, ch->getTrust());
    send_to_char("Ok.\n\r", ch);
    return;
}

void do_switch(Character *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
    Character *victim;

    one_argument(argument, arg);

    if (arg[0] == '\0')
    {
        send_to_char("Switch into whom?\n\r", ch);
        return;
    }

    if (ch->desc == NULL)
        return;

    if (ch->desc->original != NULL)
    {
        send_to_char("You are already switched.\n\r", ch);
        return;
    }

    if ((victim = get_char_world(ch, arg)) == NULL)
    {
        send_to_char("They aren't here.\n\r", ch);
        return;
    }

    if (victim == ch)
    {
        send_to_char("Ok.\n\r", ch);
        return;
    }

    if (!IS_NPC(victim))
    {
        send_to_char("You can only switch into mobiles.\n\r", ch);
        return;
    }

    if (!is_room_owner(ch, victim->in_room) && ch->in_room != victim->in_room && room_is_private(victim->in_room) && !IS_TRUSTED(ch, IMPLEMENTOR))
    {
        send_to_char("That character is in a private room.\n\r", ch);
        return;
    }

    if (victim->desc != NULL)
    {
        send_to_char("Character in use.\n\r", ch);
        return;
    }

    snprintf(buf, sizeof(buf), "$N switches into %s", victim->short_descr);
    Wiznet::instance()->report(buf, ch, NULL, WIZ_SWITCHES, WIZ_SECURE, ch->getTrust());

    ch->desc->character = victim;
    ch->desc->original = ch;
    victim->desc = ch->desc;
    ch->desc = NULL;
    /* change communications to match */
    if (ch->prompt != NULL)
        victim->prompt = str_dup(ch->prompt);
    victim->comm = ch->comm;
    victim->lines = ch->lines;
    send_to_char("Ok.\n\r", victim);
    return;
}

void do_return(Character *ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];

    if (ch->desc == NULL)
        return;

    if (ch->desc->original == NULL)
    {
        send_to_char("You aren't switched.\n\r", ch);
        return;
    }

    send_to_char(
        "You return to your original body. Type replay to see any missed tells.\n\r",
        ch);
    if (ch->prompt != NULL)
    {
        free_string(ch->prompt);
        ch->prompt = NULL;
    }

    snprintf(buf, sizeof(buf), "$N returns from %s.", ch->short_descr);
    Wiznet::instance()->report(buf, ch->desc->original, 0, WIZ_SWITCHES, WIZ_SECURE, ch->getTrust());
    ch->desc->character = ch->desc->original;
    ch->desc->original = NULL;
    ch->desc->character->desc = ch->desc;
    ch->desc = NULL;
    return;
}

/* trust levels for load and clone */
bool obj_check(Character *ch, Object *obj)
{
    if (IS_TRUSTED(ch, GOD) || (IS_TRUSTED(ch, IMMORTAL) && obj->getLevel() <= 20 && obj->getCost() <= 1000) || (IS_TRUSTED(ch, DEMI) && obj->getLevel() <= 10 && obj->getCost() <= 500) || (IS_TRUSTED(ch, ANGEL) && obj->getLevel() <= 5 && obj->getCost() <= 250) || (IS_TRUSTED(ch, AVATAR) && obj->getLevel() == 0 && obj->getCost() <= 100))
        return TRUE;
    else
        return FALSE;
}

/* for clone, to insure that cloning goes many levels deep */
void recursive_clone(Character *ch, Object *obj, Object *clone)
{
    for (auto c_obj : obj->getContents())
    {
        if (obj_check(ch, c_obj))
        {
            Object *t_obj = c_obj->clone();
            clone->addObject(t_obj);
            recursive_clone(ch, c_obj, t_obj);
        }
    }
}

/* command that is similar to load */
void do_clone(Character *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    char *rest;
    Character *mob;
    Object *obj;

    rest = one_argument(argument, arg);

    if (arg[0] == '\0')
    {
        send_to_char("Clone what?\n\r", ch);
        return;
    }

    if (!str_prefix(arg, "object"))
    {
        mob = NULL;
        obj = get_obj_here(ch, rest);
        if (obj == NULL)
        {
            send_to_char("You don't see that here.\n\r", ch);
            return;
        }
    }
    else if (!str_prefix(arg, "mobile") || !str_prefix(arg, "character"))
    {
        obj = NULL;
        mob = get_char_room(ch, rest);
        if (mob == NULL)
        {
            send_to_char("You don't see that here.\n\r", ch);
            return;
        }
    }
    else /* find both */
    {
        mob = get_char_room(ch, argument);
        obj = get_obj_here(ch, argument);
        if (mob == NULL && obj == NULL)
        {
            send_to_char("You don't see that here.\n\r", ch);
            return;
        }
    }

    /* clone an object */
    if (obj != NULL)
    {
        Object *clone;

        if (!obj_check(ch, obj))
        {
            send_to_char(
                "Your powers are not great enough for such a task.\n\r", ch);
            return;
        }

        clone = obj->clone();
        if (obj->getCarriedBy() != NULL)
            ch->addObject(clone);
        else
            obj_to_room(clone, ch->in_room);
        recursive_clone(ch, obj, clone);

        act("$n has created $p.", ch, clone, NULL, TO_ROOM, POS_RESTING);
        act("You clone $p.", ch, clone, NULL, TO_CHAR, POS_RESTING);
        Wiznet::instance()->report("$N clones $p.", ch, clone, WIZ_LOAD, WIZ_SECURE, ch->getTrust());
        return;
    }
    else if (mob != NULL)
    {
        Character *clone;
        char buf[MAX_STRING_LENGTH];

        if (!IS_NPC(mob))
        {
            send_to_char("You can only clone mobiles.\n\r", ch);
            return;
        }

        if ((mob->level > 20 && !IS_TRUSTED(ch, GOD)) || (mob->level > 10 && !IS_TRUSTED(ch, IMMORTAL)) || (mob->level > 5 && !IS_TRUSTED(ch, DEMI)) || (mob->level > 0 && !IS_TRUSTED(ch, ANGEL)) || !IS_TRUSTED(ch, AVATAR))
        {
            send_to_char(
                "Your powers are not great enough for such a task.\n\r", ch);
            return;
        }

        clone = new NonPlayerCharacter(mob->pIndexData);
        clone_mobile(mob, clone);

        for (auto obj : mob->getCarrying())
        {
            if (obj_check(ch, obj))
            {
                Object *new_obj = obj->clone();
                recursive_clone(ch, obj, new_obj);
                clone->addObject(new_obj);
                new_obj->setWearLocation(obj->getWearLocation());
            }
        }
        char_to_room(clone, ch->in_room);
        act("$n has created $N.", ch, NULL, clone, TO_ROOM, POS_RESTING);
        act("You clone $N.", ch, NULL, clone, TO_CHAR, POS_RESTING);
        snprintf(buf, sizeof(buf), "$N clones %s.", clone->short_descr);
        Wiznet::instance()->report(buf, ch, NULL, WIZ_LOAD, WIZ_SECURE, ch->getTrust());
        return;
    }
}

/* RT to replace the two load commands */

void do_load(Character *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];

    argument = one_argument(argument, arg);

    if (arg[0] == '\0')
    {
        send_to_char("Syntax:\n\r", ch);
        send_to_char("  load mob <vnum>\n\r", ch);
        send_to_char("  load obj <vnum> <level>\n\r", ch);
        return;
    }

    if (!str_cmp(arg, "mob") || !str_cmp(arg, "char"))
    {
        do_mload(ch, argument);
        return;
    }

    if (!str_cmp(arg, "obj"))
    {
        do_oload(ch, argument);
        return;
    }
    /* echo syntax */
    do_load(ch, (char *)"");
}

void do_mload(Character *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    MOB_INDEX_DATA *pMobIndex;
    Character *victim;
    char buf[MAX_STRING_LENGTH];

    one_argument(argument, arg);

    if (arg[0] == '\0' || !is_number(arg))
    {
        send_to_char("Syntax: load mob <vnum>.\n\r", ch);
        return;
    }

    if ((pMobIndex = get_mob_index(atoi(arg))) == NULL)
    {
        send_to_char("No mob has that vnum.\n\r", ch);
        return;
    }

    victim = new NonPlayerCharacter(pMobIndex);
    char_to_room(victim, ch->in_room);
    act("$n has created $N!", ch, NULL, victim, TO_ROOM, POS_RESTING);
    snprintf(buf, sizeof(buf), "$N loads %s.", victim->short_descr);
    Wiznet::instance()->report(buf, ch, NULL, WIZ_LOAD, WIZ_SECURE, ch->getTrust());
    send_to_char("Ok.\n\r", ch);
    return;
}

void do_oload(Character *ch, char *argument)
{
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    OBJ_INDEX_DATA *pObjIndex;
    Object *obj;
    int level;

    argument = one_argument(argument, arg1);
    one_argument(argument, arg2);

    if (arg1[0] == '\0' || !is_number(arg1))
    {
        send_to_char("Syntax: load obj <vnum> <level>.\n\r", ch);
        return;
    }

    level = ch->getTrust(); /* default */

    if (arg2[0] != '\0') /* load with a level */
    {
        if (!is_number(arg2))
        {
            send_to_char("Syntax: oload <vnum> <level>.\n\r", ch);
            return;
        }
        level = atoi(arg2);
        if (level < 0 || level > ch->getTrust())
        {
            send_to_char("Level must be be between 0 and your level.\n\r", ch);
            return;
        }
    }

    if ((pObjIndex = get_obj_index(atoi(arg1))) == NULL)
    {
        send_to_char("No object has that vnum.\n\r", ch);
        return;
    }

    if (pObjIndex->item_type == ITEM_NUKE && ch->getTrust() != MAX_LEVEL)
    {
        send_to_char("Only impelementors can load nuclear weapons.\n\r", ch);
        return;
    }

    obj = ObjectHelper::createFromIndex(pObjIndex, level);
    if (obj->canWear(ITEM_TAKE))
        ch->addObject(obj);
    else
        obj_to_room(obj, ch->in_room);
    act("$n has created $p!", ch, obj, NULL, TO_ROOM, POS_RESTING);
    Wiznet::instance()->report("$N loads $p.", ch, obj, WIZ_LOAD, WIZ_SECURE, ch->getTrust());
    send_to_char("Ok.\n\r", ch);
    return;
}

void do_purge(Character *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    char buf[100];
    Character *victim;
    Object *obj;
    DESCRIPTOR_DATA *d;

    one_argument(argument, arg);

    if (arg[0] == '\0')
    {
        /* 'purge' */
        Character *vnext;
        Object *obj_next;

        for (victim = ch->in_room->people; victim != NULL; victim = vnext)
        {
            vnext = victim->next_in_room;
            if (IS_NPC(victim) && !IS_SET(victim->act, ACT_NOPURGE) && victim != ch /* safety precaution */)
                extract_char(victim, TRUE);
        }

        for (auto obj : ch->in_room->contents)
        {
            if (!obj->hasStat(ITEM_NOPURGE))
                extract_obj(obj);
        }

        act("$n purges the room!", ch, NULL, NULL, TO_ROOM, POS_RESTING);
        send_to_char("Ok.\n\r", ch);
        return;
    }

    if ((victim = get_char_world(ch, arg)) == NULL)
    {
        send_to_char("They aren't here.\n\r", ch);
        return;
    }

    if (!IS_NPC(victim))
    {

        if (ch == victim)
        {
            send_to_char("Ho ho ho.\n\r", ch);
            return;
        }

        if (ch->getTrust() <= victim->getTrust())
        {
            send_to_char("Maybe that wasn't a good idea...\n\r", ch);
            snprintf(buf, sizeof(buf), "%s tried to purge you!\n\r", ch->getName().c_str());
            send_to_char(buf, victim);
            return;
        }

        act("$n disintegrates $N.", ch, 0, victim, TO_NOTVICT, POS_RESTING);

        if (victim->level > 1)
            save_char_obj(victim);
        d = victim->desc;
        extract_char(victim, TRUE);
        if (d != NULL)
            SocketHelper::close_socket(d);

        return;
    }

    act("$n purges $N.", ch, NULL, victim, TO_NOTVICT, POS_RESTING);
    extract_char(victim, TRUE);
    return;
}

void do_advance(Character *ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    Character *v;
    int level;
    int iLevel;

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);

    if (arg1[0] == '\0' || arg2[0] == '\0' || !is_number(arg2))
    {
        send_to_char("Syntax: advance <char> <level>.\n\r", ch);
        return;
    }

    if ((v = get_char_world(ch, arg1)) == NULL)
    {
        send_to_char("That player is not here.\n\r", ch);
        return;
    }

    if (v->isNPC())
    {
        send_to_char("Not on NPC's.\n\r", ch);
        return;
    }

    PlayerCharacter *victim = (PlayerCharacter*)v;

    if ((level = atoi(arg2)) < 1 || level > 60)
    {
        send_to_char("Level must be 1 to 60.\n\r", ch);
        return;
    }

    if (level > ch->getTrust())
    {
        send_to_char("Limited to your trust level.\n\r", ch);
        return;
    }

    /*
     * Lower level:
     *   Reset to level 1.
     *   Then raise again.
     *   Currently, an imp can lower another imp.
     *   -- Swiftest
     */
    if (level <= victim->level)
    {
        int temp_prac;

        send_to_char("Lowering a player's level!\n\r", ch);
        send_to_char("**** OOOOHHHHHHHHHH  NNNNOOOO ****\n\r", victim);
        temp_prac = victim->practice;
        victim->level = 1;
        victim->exp = exp_per_level(victim, victim->points);
        victim->max_hit = 10;
        victim->max_mana = 100;
        victim->max_move = 100;
        victim->practice = 0;
        victim->hit = victim->max_hit;
        victim->mana = victim->max_mana;
        victim->move = victim->max_move;
        victim->advance_level(TRUE);
        victim->practice = temp_prac;
    }
    else
    {
        send_to_char("Raising a player's level!\n\r", ch);
        send_to_char("**** OOOOHHHHHHHHHH  YYYYEEEESSS ****\n\r", victim);
    }

    for (iLevel = victim->level; iLevel < level; iLevel++)
    {
        victim->level += 1;
        victim->advance_level(TRUE);
    }
    snprintf(buf, sizeof(buf), "You are now level %d.\n\r", victim->level);
    send_to_char(buf, victim);
    victim->exp = exp_per_level(victim, victim->points) * UMAX(1, victim->level);
    victim->trust = 0;
    save_char_obj(victim);
    return;
}

void do_trust(Character *ch, char *argument)
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    Character *victim;
    int level;

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);

    if (arg1[0] == '\0' || arg2[0] == '\0' || !is_number(arg2))
    {
        send_to_char("Syntax: trust <char> <level>.\n\r", ch);
        return;
    }

    if ((victim = get_char_world(ch, arg1)) == NULL)
    {
        send_to_char("That player is not here.\n\r", ch);
        return;
    }

    if ((level = atoi(arg2)) < 0 || level > 60)
    {
        send_to_char("Level must be 0 (reset) or 1 to 60.\n\r", ch);
        return;
    }

    if (level > ch->getTrust())
    {
        send_to_char("Limited to your trust.\n\r", ch);
        return;
    }

    victim->trust = level;
    return;
}

void do_restore(Character *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
    Character *victim;
    Character *vch;
    DESCRIPTOR_DATA *d;

    one_argument(argument, arg);
    if (arg[0] == '\0' || !str_cmp(arg, "room"))
    {
        /* cure room */

        for (vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room)
        {
            affect_strip(vch, gsn_plague);
            affect_strip(vch, gsn_poison);
            affect_strip(vch, gsn_blindness);
            affect_strip(vch, gsn_sleep);
            affect_strip(vch, gsn_curse);

            vch->hit = vch->max_hit;
            vch->mana = vch->max_mana;
            vch->move = vch->max_move;
            update_pos(vch);
            act("$n has restored you.", ch, NULL, vch, TO_VICT, POS_RESTING);
        }

        snprintf(buf, sizeof(buf), "$N restored room %d.", ch->in_room->vnum);
        Wiznet::instance()->report(buf, ch, NULL, WIZ_RESTORE, WIZ_SECURE, ch->getTrust());

        send_to_char("Room restored.\n\r", ch);
        return;
    }

    if (ch->getTrust() >= MAX_LEVEL - 1 && !str_cmp(arg, "all"))
    {
        /* cure all */

        for (d = descriptor_list; d != NULL; d = d->next)
        {
            victim = d->character;

            if (victim == NULL || IS_NPC(victim))
                continue;

            affect_strip(victim, gsn_plague);
            affect_strip(victim, gsn_poison);
            affect_strip(victim, gsn_blindness);
            affect_strip(victim, gsn_sleep);
            affect_strip(victim, gsn_curse);

            victim->hit = victim->max_hit;
            victim->mana = victim->max_mana;
            victim->move = victim->max_move;
            update_pos(victim);
            if (victim->in_room != NULL)
                act("$n has restored you.", ch, NULL, victim, TO_VICT, POS_RESTING);
        }
        send_to_char("All active players restored.\n\r", ch);
        return;
    }

    if ((victim = get_char_world(ch, arg)) == NULL)
    {
        send_to_char("They aren't here.\n\r", ch);
        return;
    }

    affect_strip(victim, gsn_plague);
    affect_strip(victim, gsn_poison);
    affect_strip(victim, gsn_blindness);
    affect_strip(victim, gsn_sleep);
    affect_strip(victim, gsn_curse);
    victim->hit = victim->max_hit;
    victim->mana = victim->max_mana;
    victim->move = victim->max_move;
    update_pos(victim);
    act("$n has restored you.", ch, NULL, victim, TO_VICT, POS_RESTING);
    snprintf(buf, sizeof(buf), "$N restored %s",
             IS_NPC(victim) ? victim->short_descr : victim->getName().c_str());
    Wiznet::instance()->report(buf, ch, NULL, WIZ_RESTORE, WIZ_SECURE, ch->getTrust());
    send_to_char("Ok.\n\r", ch);
    return;
}

void do_freeze(Character *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
    Character *victim;

    one_argument(argument, arg);

    if (arg[0] == '\0')
    {
        send_to_char("Freeze whom?\n\r", ch);
        return;
    }

    if ((victim = get_char_world(ch, arg)) == NULL)
    {
        send_to_char("They aren't here.\n\r", ch);
        return;
    }

    if (IS_NPC(victim))
    {
        send_to_char("Not on NPC's.\n\r", ch);
        return;
    }

    if (victim->getTrust() >= ch->getTrust())
    {
        send_to_char("You failed.\n\r", ch);
        return;
    }

    if (IS_SET(victim->act, PLR_FREEZE))
    {
        REMOVE_BIT(victim->act, PLR_FREEZE);
        send_to_char("You can play again.\n\r", victim);
        send_to_char("FREEZE removed.\n\r", ch);
        snprintf(buf, sizeof(buf), "$N thaws %s.", victim->getName().c_str());
        Wiznet::instance()->report(buf, ch, NULL, WIZ_PENALTIES, WIZ_SECURE, 0);
    }
    else
    {
        SET_BIT(victim->act, PLR_FREEZE);
        send_to_char("You can't do ANYthing!\n\r", victim);
        send_to_char("FREEZE set.\n\r", ch);
        snprintf(buf, sizeof(buf), "$N puts %s in the deep freeze.", victim->getName().c_str());
        Wiznet::instance()->report(buf, ch, NULL, WIZ_PENALTIES, WIZ_SECURE, 0);
    }

    save_char_obj(victim);

    return;
}

void do_log(Character *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    Character *victim;

    one_argument(argument, arg);

    if (arg[0] == '\0')
    {
        send_to_char("Log whom?\n\r", ch);
        return;
    }

    if (!str_cmp(arg, "all"))
    {
        if (fLogAll)
        {
            fLogAll = FALSE;
            send_to_char("Log ALL off.\n\r", ch);
        }
        else
        {
            fLogAll = TRUE;
            send_to_char("Log ALL on.\n\r", ch);
        }
        return;
    }

    if ((victim = get_char_world(ch, arg)) == NULL)
    {
        send_to_char("They aren't here.\n\r", ch);
        return;
    }

    if (IS_NPC(victim))
    {
        send_to_char("Not on NPC's.\n\r", ch);
        return;
    }

    /*
     * No level check, gods can log anyone.
     */
    if (IS_SET(victim->act, PLR_LOG))
    {
        REMOVE_BIT(victim->act, PLR_LOG);
        send_to_char("LOG removed.\n\r", ch);
    }
    else
    {
        SET_BIT(victim->act, PLR_LOG);
        send_to_char("LOG set.\n\r", ch);
    }

    return;
}

void do_noemote(Character *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
    Character *victim;

    one_argument(argument, arg);

    if (arg[0] == '\0')
    {
        send_to_char("Noemote whom?\n\r", ch);
        return;
    }

    if ((victim = get_char_world(ch, arg)) == NULL)
    {
        send_to_char("They aren't here.\n\r", ch);
        return;
    }

    if (victim->getTrust() >= ch->getTrust())
    {
        send_to_char("You failed.\n\r", ch);
        return;
    }

    if (IS_SET(victim->comm, COMM_NOEMOTE))
    {
        REMOVE_BIT(victim->comm, COMM_NOEMOTE);
        send_to_char("You can emote again.\n\r", victim);
        send_to_char("NOEMOTE removed.\n\r", ch);
        snprintf(buf, sizeof(buf), "$N restores emotes to %s.", victim->getName().c_str());
        Wiznet::instance()->report(buf, ch, NULL, WIZ_PENALTIES, WIZ_SECURE, 0);
    }
    else
    {
        SET_BIT(victim->comm, COMM_NOEMOTE);
        send_to_char("You can't emote!\n\r", victim);
        send_to_char("NOEMOTE set.\n\r", ch);
        snprintf(buf, sizeof(buf), "$N revokes %s's emotes.", victim->getName().c_str());
        Wiznet::instance()->report(buf, ch, NULL, WIZ_PENALTIES, WIZ_SECURE, 0);
    }

    return;
}

void do_noshout(Character *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
    Character *victim;

    one_argument(argument, arg);

    if (arg[0] == '\0')
    {
        send_to_char("Noshout whom?\n\r", ch);
        return;
    }

    if ((victim = get_char_world(ch, arg)) == NULL)
    {
        send_to_char("They aren't here.\n\r", ch);
        return;
    }

    if (IS_NPC(victim))
    {
        send_to_char("Not on NPC's.\n\r", ch);
        return;
    }

    if (victim->getTrust() >= ch->getTrust())
    {
        send_to_char("You failed.\n\r", ch);
        return;
    }

    if (IS_SET(victim->comm, COMM_NOSHOUT))
    {
        REMOVE_BIT(victim->comm, COMM_NOSHOUT);
        send_to_char("You can shout again.\n\r", victim);
        send_to_char("NOSHOUT removed.\n\r", ch);
        snprintf(buf, sizeof(buf), "$N restores shouts to %s.", victim->getName().c_str());
        Wiznet::instance()->report(buf, ch, NULL, WIZ_PENALTIES, WIZ_SECURE, 0);
    }
    else
    {
        SET_BIT(victim->comm, COMM_NOSHOUT);
        send_to_char("You can't shout!\n\r", victim);
        send_to_char("NOSHOUT set.\n\r", ch);
        snprintf(buf, sizeof(buf), "$N revokes %s's shouts.", victim->getName().c_str());
        Wiznet::instance()->report(buf, ch, NULL, WIZ_PENALTIES, WIZ_SECURE, 0);
    }

    return;
}

void do_notell(Character *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
    Character *victim;

    one_argument(argument, arg);

    if (arg[0] == '\0')
    {
        send_to_char("Notell whom?", ch);
        return;
    }

    if ((victim = get_char_world(ch, arg)) == NULL)
    {
        send_to_char("They aren't here.\n\r", ch);
        return;
    }

    if (victim->getTrust() >= ch->getTrust())
    {
        send_to_char("You failed.\n\r", ch);
        return;
    }

    if (IS_SET(victim->comm, COMM_NOTELL))
    {
        REMOVE_BIT(victim->comm, COMM_NOTELL);
        send_to_char("You can tell again.\n\r", victim);
        send_to_char("NOTELL removed.\n\r", ch);
        snprintf(buf, sizeof(buf), "$N restores tells to %s.", victim->getName().c_str());
        Wiznet::instance()->report(buf, ch, NULL, WIZ_PENALTIES, WIZ_SECURE, 0);
    }
    else
    {
        SET_BIT(victim->comm, COMM_NOTELL);
        send_to_char("You can't tell!\n\r", victim);
        send_to_char("NOTELL set.\n\r", ch);
        snprintf(buf, sizeof(buf), "$N revokes %s's tells.", victim->getName().c_str());
        Wiznet::instance()->report(buf, ch, NULL, WIZ_PENALTIES, WIZ_SECURE, 0);
    }

    return;
}

void do_peace(Character *ch, char *argument)
{
    Character *rch;

    for (rch = ch->in_room->people; rch != NULL; rch = rch->next_in_room)
    {
        if (rch->fighting != NULL)
            stop_fighting(rch, TRUE);
        if (IS_NPC(rch) && IS_SET(rch->act, ACT_AGGRESSIVE))
            REMOVE_BIT(rch->act, ACT_AGGRESSIVE);
        if (rch->hunting != NULL)
            rch->hunting = NULL;
    }

    send_to_char("Ok.\n\r", ch);
    return;
}

void do_wizlock(Character *ch, char *argument)
{
    extern bool wizlock;
    wizlock = !wizlock;

    if (wizlock)
    {
        Wiznet::instance()->report("$N has wizlocked the game.", ch, NULL, 0, 0, 0);
        send_to_char("Game wizlocked.\n\r", ch);
    }
    else
    {
        Wiznet::instance()->report("$N removes wizlock.", ch, NULL, 0, 0, 0);
        send_to_char("Game un-wizlocked.\n\r", ch);
    }

    return;
}

/* RT anti-newbie code */

void do_newlock(Character *ch, char *argument)
{
    extern bool newlock;
    newlock = !newlock;

    if (newlock)
    {
        Wiznet::instance()->report("$N locks out new characters.", ch, NULL, 0, 0, 0);
        send_to_char("New characters have been locked out.\n\r", ch);
    }
    else
    {
        Wiznet::instance()->report("$N allows new characters back in.", ch, NULL, 0, 0, 0);
        send_to_char("Newlock removed.\n\r", ch);
    }

    return;
}

void do_slookup(Character *ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    int sn;

    one_argument(argument, arg);
    if (arg[0] == '\0')
    {
        send_to_char("Lookup which skill or spell?\n\r", ch);
        return;
    }

    if (!str_cmp(arg, "all"))
    {
        for (sn = 0; sn < MAX_SKILL; sn++)
        {
            if (skill_table[sn].name == NULL)
                break;
            snprintf(buf, sizeof(buf), "Sn: %3d  Slot: %3d  Skill/spell: '%s'\n\r",
                     sn, skill_table[sn].slot, skill_table[sn].name);
            send_to_char(buf, ch);
        }
    }
    else
    {
        if ((sn = skill_lookup(arg)) < 0)
        {
            send_to_char("No such skill or spell.\n\r", ch);
            return;
        }

        snprintf(buf, sizeof(buf), "Sn: %3d  Slot: %3d  Skill/spell: '%s'\n\r",
                 sn, skill_table[sn].slot, skill_table[sn].name);
        send_to_char(buf, ch);
    }

    return;
}

/* RT set replaces sset, mset, oset, and rset */

void do_set(Character *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];

    argument = one_argument(argument, arg);

    if (arg[0] == '\0')
    {
        send_to_char("Syntax:\n\r", ch);
        send_to_char("  set mob   <name> <field> <value>\n\r", ch);
        send_to_char("  set obj   <name> <field> <value>\n\r", ch);
        send_to_char("  set room  <room> <field> <value>\n\r", ch);
        send_to_char("  set clan  <name> <field> <value>\n\r", ch);
        send_to_char("  set skill <name> <spell or skill> <value>\n\r", ch);
        return;
    }

    if (!str_prefix(arg, "clan") || !str_prefix(arg, "guild"))
    {
        do_setclan(ch, argument);
        return;
    }

    if (!str_prefix(arg, "mobile") || !str_prefix(arg, "character"))
    {
        do_mset(ch, argument);
        return;
    }

    if (!str_prefix(arg, "skill") || !str_prefix(arg, "spell"))
    {
        do_sset(ch, argument);
        return;
    }

    if (!str_prefix(arg, "object"))
    {
        do_oset(ch, argument);
        return;
    }

    if (!str_prefix(arg, "room"))
    {
        do_rset(ch, argument);
        return;
    }
    /* echo syntax */
    do_set(ch, (char *)"");
}

void do_sset(Character *ch, char *argument)
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char arg3[MAX_INPUT_LENGTH];
    PlayerCharacter *victim;
    int value;
    int sn;
    bool fAll;

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);
    argument = one_argument(argument, arg3);

    if (arg1[0] == '\0' || arg2[0] == '\0' || arg3[0] == '\0')
    {
        send_to_char("Syntax:\n\r", ch);
        send_to_char("  set skill <name> <spell or skill> <value>\n\r", ch);
        send_to_char("  set skill <name> all <value>\n\r", ch);
        send_to_char("   (use the name of the skill, not the number)\n\r", ch);
        return;
    }

    if ((victim = get_player_world(ch, arg1)) == NULL)
    {
        send_to_char("They aren't here.\n\r", ch);
        return;
    }

    fAll = !str_cmp(arg2, "all");
    sn = 0;
    if (!fAll && (sn = skill_lookup(arg2)) < 0)
    {
        send_to_char("No such skill or spell.\n\r", ch);
        return;
    }

    /*
     * Snarf the value.
     */
    if (!is_number(arg3))
    {
        send_to_char("Value must be numeric.\n\r", ch);
        return;
    }

    value = atoi(arg3);
    if (value < 0 || value > 100)
    {
        send_to_char("Value range is 0 to 100.\n\r", ch);
        return;
    }

    if (fAll)
    {
        for (sn = 0; sn < MAX_SKILL; sn++)
        {
            if (skill_table[sn].name != NULL && victim->learned[sn] > 0)
                victim->learned[sn] = value;
        }
    }
    else
    {
        if (victim->learned[sn] < 1)
        {
            send_to_char("To avoid crashes, you can't set characters with skills they don't have.\n\r", ch);
            return;
        }
        victim->learned[sn] = value;
    }

    return;
}

void do_mset(Character *ch, char *argument)
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char arg3[MAX_INPUT_LENGTH];
    char buf[100];
    Character *victim;
    int value;

    smash_tilde(argument);
    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);
    strcpy(arg3, argument);

    if (arg1[0] == '\0' || arg2[0] == '\0' || arg3[0] == '\0')
    {
        send_to_char("Syntax:\n\r", ch);
        send_to_char("  set char <name> <field> <value>\n\r", ch);
        send_to_char("  Field being one of:\n\r", ch);
        send_to_char("    str int wis dex con sex class level\n\r", ch);
        send_to_char("    race group gold silver hp mana move prac\n\r", ch);
        send_to_char("    align train thirst hunger drunk full\n\r", ch);
        send_to_char("    were\n\r", ch);
        return;
    }

    if ((victim = get_char_world(ch, arg1)) == NULL)
    {
        send_to_char("They aren't here.\n\r", ch);
        return;
    }

    /* clear zones for mobs */
    victim->zone = NULL;

    /*
     * Snarf the value (which need not be numeric).
     */
    value = is_number(arg3) ? atoi(arg3) : -1;

    /*
     * Set something.
     */
    if (!str_cmp(arg2, "str"))
    {
        if (value < 3 || value > get_max_train(victim, STAT_STR))
        {
            snprintf(buf, sizeof(buf),
                     "Strength range is 3 to %d\n\r.",
                     get_max_train(victim, STAT_STR));
            send_to_char(buf, ch);
            return;
        }

        victim->perm_stat[STAT_STR] = value;
        return;
    }

    if (!str_cmp(arg2, "were"))
    {
        argument = one_argument(argument, arg3);

        if (arg3[0] == '\0')
        {
            snprintf(buf, sizeof(buf), "Syntax: set char %s were [field] [blah]\n\r", arg1);
            send_to_char(buf, ch);
            send_to_char("  orig:  sets default race\n\r", ch);
            send_to_char("  morph: sets morph race\n\r", ch);
            send_to_char("  Both tags with no argument brings up a list of races.\n\r", ch);
            return;
        }

        if (!str_cmp(arg3, "orig"))
        {
            if (argument[0] == '\0')
            {
                int race;

                send_to_char("Race list:\n\r", ch);
                for (auto race : race_manager->getAllRaces())
                {
                    if (race == race_manager->getRaceByName("werefolk"))
                    {
                        continue;
                    }

                    if (race->isPlayerRace())
                    {
                        snprintf(buf, sizeof(buf), "  %s\n\r", race->getName().c_str());
                        send_to_char(buf, ch);
                    }
                }
                return;
            }

            Race *race;
            if (!(race = race_manager->getRaceByName(argument)) || race == race_manager->getRaceByName("werefolk"))
            {
                send_to_char("That is not a valid race.\n\r", ch);
                return;
            }

            victim->orig_form = race->getLegacyId();
            snprintf(buf, sizeof(buf), "Your original form is now: %s.\n\r", race->getName().c_str());
            send_to_char(buf, ch);
            send_to_char("Their original form has been changed.\n\r", ch);
            return;
        }

        if (!str_cmp(arg3, "morph"))
        {
            if (argument[0] == '\0')
            {
                int race;

                send_to_char("Available races:\n\r", ch);
                for (race = 0; morph_table[race].name[0] != '\0'; race++)
                {
                    send_to_char("  ", ch);
                    send_to_char(morph_table[race].name, ch);
                    send_to_char("\n\r", ch);
                }

                return;
            }

            if (morph_lookup(argument) == -1)
            {
                send_to_char("Illegal race.\n\r", ch);
                return;
            }

            victim->morph_form = morph_lookup(argument);
            snprintf(buf, sizeof(buf), "Your morph form is now: %s\n\r", morph_table[victim->morph_form].name);
            send_to_char(buf, victim);
            send_to_char("OK.\n\r", ch);
            return;
        }

        snprintf(buf, sizeof(buf), "Syntax: set char %s were [field] [blah]\n\r", arg1);
        send_to_char(buf, ch);
        send_to_char("  orig:  sets default race\n\r", ch);
        send_to_char("  morph: sets morph race\n\r", ch);
        send_to_char("  Both tags with no argument brings up a list of races.\n\r", ch);
        return;
    }

    if (!str_cmp(arg2, "int"))
    {
        if (value < 3 || value > get_max_train(victim, STAT_INT))
        {
            snprintf(buf, sizeof(buf),
                     "Intelligence range is 3 to %d.\n\r",
                     get_max_train(victim, STAT_INT));
            send_to_char(buf, ch);
            return;
        }

        victim->perm_stat[STAT_INT] = value;
        return;
    }

    if (!str_cmp(arg2, "wis"))
    {
        if (value < 3 || value > get_max_train(victim, STAT_WIS))
        {
            snprintf(buf, sizeof(buf),
                     "Wisdom range is 3 to %d.\n\r", get_max_train(victim, STAT_WIS));
            send_to_char(buf, ch);
            return;
        }

        victim->perm_stat[STAT_WIS] = value;
        return;
    }

    if (!str_cmp(arg2, "dex"))
    {
        if (value < 3 || value > get_max_train(victim, STAT_DEX))
        {
            snprintf(buf, sizeof(buf),
                     "Dexterity ranges is 3 to %d.\n\r",
                     get_max_train(victim, STAT_DEX));
            send_to_char(buf, ch);
            return;
        }

        victim->perm_stat[STAT_DEX] = value;
        return;
    }

    if (!str_cmp(arg2, "con"))
    {
        if (value < 3 || value > get_max_train(victim, STAT_CON))
        {
            snprintf(buf, sizeof(buf),
                     "Constitution range is 3 to %d.\n\r",
                     get_max_train(victim, STAT_CON));
            send_to_char(buf, ch);
            return;
        }

        victim->perm_stat[STAT_CON] = value;
        return;
    }

    if (!str_prefix(arg2, "sex"))
    {
        if (value < 0 || value > 2)
        {
            send_to_char("Sex range is 0 to 2.\n\r", ch);
            return;
        }
        victim->sex = value;
        if (!IS_NPC(victim))
            ((PlayerCharacter*)victim)->true_sex = value;
        return;
    }

    if (!str_prefix(arg2, "class"))
    {
        int class_num;

        if (IS_NPC(victim))
        {
            send_to_char("Mobiles have no class.\n\r", ch);
            return;
        }

        class_num = class_lookup(arg3);
        if (class_num == -1)
        {
            char buf[MAX_STRING_LENGTH];

            strcpy(buf, "Possible classes are: ");
            for (class_num = 0; class_num < MAX_CLASS; class_num++)
            {
                if (class_num > 0)
                    strcat(buf, " ");
                strcat(buf, class_table[class_num].name);
            }
            strcat(buf, ".\n\r");

            send_to_char(buf, ch);
            return;
        }

        victim->class_num = class_num;
        return;
    }

    if (!str_prefix(arg2, "level"))
    {
        if (!IS_NPC(victim))
        {
            send_to_char("Not on PC's.\n\r", ch);
            return;
        }

        if (value < 0 || value > 60)
        {
            send_to_char("Level range is 0 to 60.\n\r", ch);
            return;
        }
        victim->level = value;
        return;
    }

    if (!str_prefix(arg2, "gold"))
    {
        victim->gold = value;
        return;
    }

    if (!str_prefix(arg2, "silver"))
    {
        victim->silver = value;
        return;
    }

    if (!str_prefix(arg2, "hp"))
    {
        if (value < -10 || value > 30000)
        {
            send_to_char("Hp range is -10 to 30,000 hit points.\n\r", ch);
            return;
        }
        victim->max_hit = value;
        if (!IS_NPC(victim))
            ((PlayerCharacter*)victim)->perm_hit = value;
        return;
    }

    if (!str_prefix(arg2, "mana"))
    {
        if (value < 0 || value > 30000)
        {
            send_to_char("Mana range is 0 to 30,000 mana points.\n\r", ch);
            return;
        }
        victim->max_mana = value;
        if (!IS_NPC(victim))
            ((PlayerCharacter*)victim)->perm_mana = value;
        return;
    }

    if (!str_prefix(arg2, "move"))
    {
        if (value < 0 || value > 30000)
        {
            send_to_char("Move range is 0 to 30,000 move points.\n\r", ch);
            return;
        }
        victim->max_move = value;
        if (!IS_NPC(victim))
            ((PlayerCharacter*)victim)->perm_move = value;
        return;
    }

    if (!str_prefix(arg2, "practice"))
    {
        if (value < 0 || value > 250)
        {
            send_to_char("Practice range is 0 to 250 sessions.\n\r", ch);
            return;
        }
        victim->practice = value;
        return;
    }

    if (!str_prefix(arg2, "train"))
    {
        if (value < 0 || value > 50)
        {
            send_to_char("Training session range is 0 to 50 sessions.\n\r", ch);
            return;
        }
        victim->train = value;
        return;
    }

    if (!str_prefix(arg2, "align"))
    {
        if (value < -1000 || value > 1000)
        {
            send_to_char("Alignment range is -1000 to 1000.\n\r", ch);
            return;
        }
        victim->alignment = value;
        return;
    }

    if (!str_prefix(arg2, "thirst"))
    {
        if (IS_NPC(victim))
        {
            send_to_char("Not on NPC's.\n\r", ch);
            return;
        }

        if (value < -1 || value > 100)
        {
            send_to_char("Thirst range is -1 to 100.\n\r", ch);
            return;
        }

        ((PlayerCharacter*)victim)->condition[COND_THIRST] = value;
        return;
    }

    if (!str_prefix(arg2, "drunk"))
    {
        if (IS_NPC(victim))
        {
            send_to_char("Not on NPC's.\n\r", ch);
            return;
        }

        if (value < -1 || value > 100)
        {
            send_to_char("Drunk range is -1 to 100.\n\r", ch);
            return;
        }

        ((PlayerCharacter*)victim)->condition[COND_DRUNK] = value;
        return;
    }

    if (!str_prefix(arg2, "full"))
    {
        if (IS_NPC(victim))
        {
            send_to_char("Not on NPC's.\n\r", ch);
            return;
        }

        if (value < -1 || value > 100)
        {
            send_to_char("Full range is -1 to 100.\n\r", ch);
            return;
        }

        ((PlayerCharacter*)victim)->condition[COND_FULL] = value;
        return;
    }

    if (!str_prefix(arg2, "hunger"))
    {
        if (IS_NPC(victim))
        {
            send_to_char("Not on NPC's.\n\r", ch);
            return;
        }

        if (value < -1 || value > 100)
        {
            send_to_char("Full range is -1 to 100.\n\r", ch);
            return;
        }

        ((PlayerCharacter*)victim)->condition[COND_HUNGER] = value;
        return;
    }

    if (!str_prefix(arg2, "race"))
    {
        Race *race = race_manager->getRaceByName(arg3);

        if (!race)
        {
            send_to_char("That is not a valid race.\n\r", ch);
            return;
        }

        if (!IS_NPC(victim) && !race->isPlayerRace())
        {
            send_to_char("That is not a valid player race.\n\r", ch);
            return;
        }

        victim->setRace(race);
        return;
    }

    if (!str_prefix(arg2, "group"))
    {
        if (!IS_NPC(victim))
        {
            send_to_char("Only on NPCs.\n\r", ch);
            return;
        }
        victim->group = value;
        return;
    }

    /*
     * Generate usage message.
     */
    do_mset(ch, (char *)"");
    return;
}

void do_string(Character *ch, char *argument)
{
    char type[MAX_INPUT_LENGTH];
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char arg3[MAX_INPUT_LENGTH];
    Character *victim;
    Object *obj;

    smash_tilde(argument);
    argument = one_argument(argument, type);
    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);
    strcpy(arg3, argument);

    if (type[0] == '\0' || arg1[0] == '\0' || arg2[0] == '\0' || arg3[0] == '\0')
    {
        send_to_char("Syntax:\n\r", ch);
        send_to_char("  string char <name> <field> <string>\n\r", ch);
        send_to_char("    fields: name short long desc title spec\n\r", ch);
        send_to_char("  string obj  <name> <field> <string>\n\r", ch);
        send_to_char("    fields: name short long extended\n\r", ch);
        return;
    }

    if (!str_prefix(type, "character") || !str_prefix(type, "mobile"))
    {
        if ((victim = get_char_world(ch, arg1)) == NULL)
        {
            send_to_char("They aren't here.\n\r", ch);
            return;
        }

        /* clear zone for mobs */
        victim->zone = NULL;

        /* string something */

        if (!str_prefix(arg2, "name"))
        {
            if (!IS_NPC(victim))
            {
                send_to_char("Not on PC's.\n\r", ch);
                return;
            }
            victim->setName(arg3);
            return;
        }

        if (!str_prefix(arg2, "description"))
        {
            victim->setDescription(arg3);
            return;
        }

        if (!str_prefix(arg2, "short"))
        {
            free_string(victim->short_descr);
            victim->short_descr = str_dup(arg3);
            return;
        }

        if (!str_prefix(arg2, "long"))
        {
            free_string(victim->long_descr);
            strcat(arg3, "\n\r");
            victim->long_descr = str_dup(arg3);
            return;
        }

        if (!str_prefix(arg2, "title"))
        {
            if (IS_NPC(victim))
            {
                send_to_char("Not on NPC's.\n\r", ch);
                return;
            }

            set_title(victim, arg3);
            return;
        }

        if (!str_prefix(arg2, "spec"))
        {
            if (!IS_NPC(victim))
            {
                send_to_char("Not on PC's.\n\r", ch);
                return;
            }

            if ((victim->spec_fun = spec_lookup(arg3)) == 0)
            {
                send_to_char("No such spec fun.\n\r", ch);
                return;
            }

            return;
        }
    }

    if (!str_prefix(type, "object"))
    {
        /* string an obj */

        if ((obj = get_obj_world(ch, arg1)) == NULL)
        {
            send_to_char("Nothing like that in heaven or earth.\n\r", ch);
            return;
        }

        if (!str_prefix(arg2, "name"))
        {
            obj->setName(arg3);
            return;
        }

        if (!str_prefix(arg2, "short"))
        {
            obj->setShortDescription(arg3);
            return;
        }

        if (!str_prefix(arg2, "long"))
        {
            obj->setDescription(arg3);
            return;
        }

        if (!str_prefix(arg2, "ed") || !str_prefix(arg2, "extended"))
        {
            ExtraDescription *ed;

            argument = one_argument(argument, arg3);
            if (argument == NULL)
            {
                send_to_char("Syntax: oset <object> ed <keyword> <string>\n\r", ch);
                return;
            }

            strcat(argument, "\n\r");

            ed = new ExtraDescription();
            ed->setKeyword(arg3);
            ed->setDescription(argument);
            obj->addExtraDescription(ed);
            return;
        }
    }

    /* echo bad use message */
    do_string(ch, (char *)"");
}

void do_oset(Character *ch, char *argument)
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char arg3[MAX_INPUT_LENGTH];
    Object *obj;
    int value;

    smash_tilde(argument);
    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);
    strcpy(arg3, argument);

    if (arg1[0] == '\0' || arg2[0] == '\0' || arg3[0] == '\0')
    {
        send_to_char("Syntax:\n\r", ch);
        send_to_char("  set obj <object> <field> <value>\n\r", ch);
        send_to_char("  Field being one of:\n\r", ch);
        send_to_char("    value0 value1 value2 value3 value4 (v1-v4)\n\r", ch);
        send_to_char("    extra wear level weight cost timer\n\r", ch);
        return;
    }

    if ((obj = get_obj_world(ch, arg1)) == NULL)
    {
        send_to_char("Nothing like that in heaven or earth.\n\r", ch);
        return;
    }

    /*
     * Snarf the value (which need not be numeric).
     */
    value = atoi(arg3);

    /*
     * Set something.
     */
    if (!str_cmp(arg2, "value0") || !str_cmp(arg2, "v0"))
    {
        obj->getValues().at(0) = UMIN(50, value);
        return;
    }

    if (!str_cmp(arg2, "value1") || !str_cmp(arg2, "v1"))
    {
        obj->getValues().at(1) = value;
        return;
    }

    if (!str_cmp(arg2, "value2") || !str_cmp(arg2, "v2"))
    {
        obj->getValues().at(2) = value;
        return;
    }

    if (!str_cmp(arg2, "value3") || !str_cmp(arg2, "v3"))
    {
        obj->getValues().at(3) = value;
        return;
    }

    if (!str_cmp(arg2, "value4") || !str_cmp(arg2, "v4"))
    {
        obj->getValues().at(4) = value;
        return;
    }

    if (!str_prefix(arg2, "extra"))
    {
        obj->setExtraFlags(value);
        return;
    }

    if (!str_prefix(arg2, "wear"))
    {
        obj->setWearFlags(value);
        return;
    }

    if (!str_prefix(arg2, "level"))
    {
        obj->setLevel(value);
        return;
    }

    if (!str_prefix(arg2, "weight"))
    {
        obj->setWeight(value);
        return;
    }

    if (!str_prefix(arg2, "cost"))
    {
        obj->setCost(value);
        return;
    }

    if (!str_prefix(arg2, "timer"))
    {
        obj->setTimer(value);
        return;
    }

    /*
     * Generate usage message.
     */
    do_oset(ch, (char *)"");
    return;
}

void do_rset(Character *ch, char *argument)
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char arg3[MAX_INPUT_LENGTH];
    ROOM_INDEX_DATA *location;
    int value;

    smash_tilde(argument);
    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);
    strcpy(arg3, argument);

    if (arg1[0] == '\0' || arg2[0] == '\0' || arg3[0] == '\0')
    {
        send_to_char("Syntax:\n\r", ch);
        send_to_char("  set room <location> <field> <value>\n\r", ch);
        send_to_char("  Field being one of:\n\r", ch);
        send_to_char("    flags sector\n\r", ch);
        return;
    }

    if ((location = find_location(ch, arg1)) == NULL)
    {
        send_to_char("No such location.\n\r", ch);
        return;
    }

    if (!is_room_owner(ch, location) && ch->in_room != location && room_is_private(location) && !IS_TRUSTED(ch, IMPLEMENTOR))
    {
        send_to_char("That room is private right now.\n\r", ch);
        return;
    }

    /*
     * Snarf the value.
     */
    if (!is_number(arg3))
    {
        send_to_char("Value must be numeric.\n\r", ch);
        return;
    }
    value = atoi(arg3);

    /*
     * Set something.
     */
    if (!str_prefix(arg2, "flags"))
    {
        location->room_flags = value;
        return;
    }

    if (!str_prefix(arg2, "sector"))
    {
        location->sector_type = value;
        return;
    }

    /*
     * Generate usage message.
     */
    do_rset(ch, (char *)"");
    return;
}

/* Written by Stimpy, ported to rom2.4 by Silverhand 3/12
 *
 *	Added the other COMM_ stuff that wasn't defined before 4/16
-Silverhand
 */
void do_sockets(Character *ch, char *argument)
{
    Character *vch;
    DESCRIPTOR_DATA *d;
    char buf[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];
    int count;
    char st[16];
    char s[100];
    char idle[10];

    count = 0;
    buf[0] = '\0';
    buf2[0] = '\0';

    strcat(buf2, "\n\r[Num Connected_State Login@ Idl] Player Name  Host\n\r");
    strcat(buf2,
           "--------------------------------------------------------------------------\n\r");
    for (d = descriptor_list; d; d = d->next)
    {
        if (d->character && ch->can_see(d->character))
        {
            /* NB: You may need to edit the CON_ values */
            switch (d->connected)
            {
            case ConnectedState::Playing:
                strcpy(st, "    PLAYING    ");
                break;
            case ConnectedState::GetName:
                strcpy(st, "   Get Name    ");
                break;
            case ConnectedState::GetOldPassword:
                strcpy(st, "Get Old Passwd ");
                break;
            case ConnectedState::ConfirmNewName:
                strcpy(st, " Confirm Name  ");
                break;
            case ConnectedState::GetNewPassword:
                strcpy(st, "Get New Passwd ");
                break;
            case ConnectedState::ConfirmNewPassword:
                strcpy(st, "Confirm Passwd ");
                break;
            case ConnectedState::GetNewRace:
                strcpy(st, "  Get New Race ");
                break;
            case ConnectedState::GetNewSex:
                strcpy(st, "  Get New Sex  ");
                break;
            case ConnectedState::GetNewClass:
                strcpy(st, " Get New Class ");
                break;
            case ConnectedState::GetAlignment:
                strcpy(st, " Get New Align ");
                break;
            case ConnectedState::DefaultChoice:
                strcpy(st, " Choosing Cust ");
                break;
            case ConnectedState::GenGroups:
                strcpy(st, " Customization ");
                break;
            case ConnectedState::PickWeapon:
                strcpy(st, " Picking Weapon");
                break;
            case ConnectedState::ReadImotd:
                strcpy(st, " Reading IMOTD ");
                break;
            case ConnectedState::BreakConnect:
                strcpy(st, "   LINKDEAD    ");
                break;
            case ConnectedState::ReadMotd:
                strcpy(st, "  Reading MOTD ");
                break;
            case ConnectedState::GetMorph:
                strcpy(st, "Choosing  Morph");
                break;
            case ConnectedState::GetMorphOrig:
                strcpy(st, "Choosing  Morph");
                break;
            case ConnectedState::NoteTo:
                strcpy(st, " Writing Notes ");
                break;
            case ConnectedState::NoteSubject:
                strcpy(st, " Writing Notes ");
                break;
            case ConnectedState::NoteExpire:
                strcpy(st, " Writing Notes ");
                break;
            case ConnectedState::NoteText:
                strcpy(st, " Writing Notes ");
                break;
            case ConnectedState::NoteFinish:
                strcpy(st, " Writing Notes ");
                break;
            default:
                strcpy(st, "   !UNKNOWN!   ");
                break;
            }
            count++;

            /* Format "login" value... */
            vch = d->original ? d->original : d->character;
            strftime(s, 100, "%I:%M%p", localtime(&vch->logon));

            if (vch->timer > 0)
                snprintf(idle, sizeof(idle), "%-2d", vch->timer);
            else
                snprintf(idle, sizeof(idle), "  ");

            snprintf(buf, sizeof(buf), "[%3d %s %7s %2s] %-12s %-32.32s\n\r",
                     d->descriptor,
                     st,
                     s,
                     idle,
                     (d->original)    ? d->original->getName().c_str()
                     : (d->character) ? d->character->getName().c_str()
                                      : "(None!)",
                     d->host);

            strcat(buf2, buf);
        }
    }

    snprintf(buf, sizeof(buf), "\n\r%d user%s\n\r", count, count == 1 ? "" : "s");
    strcat(buf2, buf);
    send_to_char(buf2, ch);
    return;
}

/*
 * Thanks to Grodyn for pointing out bugs in this function.
 */
void do_force(Character *ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];

    argument = one_argument(argument, arg);

    if (arg[0] == '\0' || argument[0] == '\0')
    {
        send_to_char("Force whom to do what?\n\r", ch);
        return;
    }

    one_argument(argument, arg2);

    if (!str_cmp(arg2, "delete") || !str_prefix(arg2, "mob"))
    {
        send_to_char("That will NOT be done.\n\r", ch);
        return;
    }

    snprintf(buf, sizeof(buf), "$n forces you to '%s'.", argument);

    if (!str_cmp(arg, "all"))
    {
        Character *vch;

        if (ch->getTrust() < MAX_LEVEL - 3)
        {
            send_to_char("Not at your level!\n\r", ch);
            return;
        }

        for (std::list<Character *>::iterator it = char_list.begin(); it != char_list.end(); it++)
        {
            vch = *it;

            if (!IS_NPC(vch) && vch->getTrust() < ch->getTrust())
            {
                act(buf, ch, NULL, vch, TO_VICT, POS_RESTING);
                interpret(vch, argument);
            }
        }
    }
    else if (!str_cmp(arg, "players"))
    {
        Character *vch;

        if (ch->getTrust() < MAX_LEVEL - 2)
        {
            send_to_char("Not at your level!\n\r", ch);
            return;
        }

        for (std::list<Character *>::iterator it = char_list.begin(); it != char_list.end(); it++)
        {
            vch = *it;

            if (!IS_NPC(vch) && vch->getTrust() < ch->getTrust() && vch->level < LEVEL_HERO)
            {
                act(buf, ch, NULL, vch, TO_VICT, POS_RESTING);
                interpret(vch, argument);
            }
        }
    }
    else if (!str_cmp(arg, "gods"))
    {
        Character *vch;

        if (ch->getTrust() < MAX_LEVEL - 2)
        {
            send_to_char("Not at your level!\n\r", ch);
            return;
        }

        for (std::list<Character *>::iterator it = char_list.begin(); it != char_list.end(); it++)
        {
            vch = *it;

            if (!IS_NPC(vch) && vch->getTrust() < ch->getTrust() && vch->level >= LEVEL_HERO)
            {
                act(buf, ch, NULL, vch, TO_VICT, POS_RESTING);
                interpret(vch, argument);
            }
        }
    }
    else
    {
        Character *victim;

        if ((victim = get_char_world(ch, arg)) == NULL)
        {
            send_to_char("They aren't here.\n\r", ch);
            return;
        }

        if (victim == ch)
        {
            send_to_char("Aye aye, right away!\n\r", ch);
            return;
        }

        if (!is_room_owner(ch, victim->in_room) && ch->in_room != victim->in_room && room_is_private(victim->in_room) && !IS_TRUSTED(ch, IMPLEMENTOR))
        {
            send_to_char("That character is in a private room.\n\r", ch);
            return;
        }

        if (victim->getTrust() >= ch->getTrust())
        {
            send_to_char("Do it yourself!\n\r", ch);
            return;
        }

        if (!IS_NPC(victim) && ch->getTrust() < MAX_LEVEL - 3)
        {
            send_to_char("Not at your level!\n\r", ch);
            return;
        }

        act(buf, ch, NULL, victim, TO_VICT, POS_RESTING);
        interpret(victim, argument);
    }

    send_to_char("Ok.\n\r", ch);
    return;
}

/*
 * New routines by Dionysos.
 */
void do_invis(Character *ch, char *argument)
{
    int level;
    char arg[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;

    /* RT code for taking a level argument */
    one_argument(argument, arg);

    if (arg[0] == '\0')
    /* take the default path */
    {
        if (ch->invis_level)
        {
            ch->invis_level = 0;
            act("$n slowly fades into existence.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
            send_to_char("You slowly fade back into existence.\n\r", ch);
        }
        else
        {
            ch->invis_level = ch->getTrust();
            act("$n slowly fades into thin air.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
            send_to_char("You slowly vanish into thin air.\n\r", ch);
        }
    }
    else
    /* do the level thing */
    {
        level = atoi(arg);
        if (level < 2 || level > ch->getTrust())
        {
            send_to_char("Invis level must be between 2 and your level.\n\r", ch);
            return;
        }
        else
        {
            ch->reply = NULL;
            ch->invis_level = level;
            act("$n slowly fades into thin air.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
            send_to_char("You slowly vanish into thin air.\n\r", ch);
        }
    }

    for (d = descriptor_list; d; d = d->next)
        if (d->character->reply == ch)
            d->character->reply = NULL;

    return;
}

void do_incognito(Character *ch, char *argument)
{
    int level;
    char arg[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;

    /* RT code for taking a level argument */
    one_argument(argument, arg);

    if (arg[0] == '\0')
    /* take the default path */
    {
        if (ch->incog_level)
        {
            ch->incog_level = 0;
            act("$n is no longer cloaked.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
            send_to_char("You are no longer cloaked.\n\r", ch);
        }
        else
        {
            ch->incog_level = ch->getTrust();
            act("$n cloaks $s presence.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
            send_to_char("You cloak your presence.\n\r", ch);
        }
    }
    else
    /* do the level thing */
    {
        level = atoi(arg);
        if (level < 2 || level > ch->getTrust())
        {
            send_to_char("Incog level must be between 2 and your level.\n\r", ch);
            return;
        }
        else
        {
            ch->reply = NULL;
            ch->incog_level = level;
            act("$n cloaks $s presence.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
            send_to_char("You cloak your presence.\n\r", ch);
        }
    }

    for (d = descriptor_list; d; d = d->next)
        if (d->character->reply == ch)
            d->character->reply = NULL;

    return;
}

void do_holylight(Character *ch, char *argument)
{
    if (ch->isNPC())
        return;

    if (IS_SET(ch->act, PLR_HOLYLIGHT))
    {
        REMOVE_BIT(ch->act, PLR_HOLYLIGHT);
        send_to_char("Holy light mode off.\n\r", ch);
    }
    else
    {
        SET_BIT(ch->act, PLR_HOLYLIGHT);
        send_to_char("Holy light mode on.\n\r", ch);
    }

    return;
}

#define CH(descriptor) ((descriptor)->original ? (descriptor)->original : (descriptor)->character)

/* This file holds the copyover data */
#define COPYOVER_FILE "copyover.data"

/* This is the executable file */
#define EXE_FILE "../src/rom"

void do_copyove(Character *ch, char *argument)
{
    send_to_char("If you want to {BCOPYOVER{x, spell it out!\n\r", ch);
    return;
}

/*  Copyover - Original idea: Fusion of MUD++
 *  Adapted to Diku by Erwin S. Andreasen, <erwin@pip.dknet.dk>
 *  http://pip.dknet.dk/~pip1773
 *  Changed into a ROM patch after seeing the 100th request for it :)
 */
void do_copyover(Character *ch, char *argument)
{
    FILE *fp;
    DESCRIPTOR_DATA *d, *d_next;
    char buf[100], buf2[100];
    extern int port, control; /* db.c */

    fp = fopen(COPYOVER_FILE, "w");

    if (!fp)
    {
        send_to_char("Copyover file not writeable, aborted.\n\r", ch);
        logger->log_stringf((char *)"Could not write to copyover file: %s", COPYOVER_FILE);
        perror("do_copyover:fopen");
        return;
    }

    /* Consider changing all saved areas here, if you use OLC */

    /* do_asave (NULL, ""); - autosave changed areas */

    snprintf(buf, sizeof(buf), "\n\r *** COPYOVER by %s - please remain seated!\n\r", ch->getName().c_str());

    /* For each playing descriptor, save its state */
    for (d = descriptor_list; d; d = d_next)
    {
        Character *och = CH(d);
        d_next = d->next; /* We delete from the list , so need to save this */

        if (!d->character || d->connected > ConnectedState::Playing) /* drop those logging on */
        {
            SocketHelper::write_to_descriptor(d->descriptor, "\n\rSorry, we are rebooting. Come back in a few minutes.\n\r", 0);
            SocketHelper::close_socket(d); /* throw'em out */
        }
        else
        {
            fprintf(fp, "%d %s %s\n", d->descriptor, och->getName().c_str(), d->host);

            if (IS_CLANNED(och))
            {
                clan_manager->add_playtime(och->getClan(), current_time - och->logon);
            }
            save_char_obj(och);

            SocketHelper::write_to_descriptor(d->descriptor, buf, 0);
        }
    }

    fprintf(fp, "-1\n");
    fclose(fp);

    /* Close reserve and other always-open files and release other resources */

    fclose(fpReserve);

    /* exec - descriptors are inherited */

    snprintf(buf, sizeof(buf), "%d", port);
    snprintf(buf, sizeof(buf), "%d", control);
    execl(EXE_FILE, "rom", buf, "copyover", buf2, (char *)NULL);

    /* Failed - sucessful exec will not return */

    perror("do_copyover: execl");
    send_to_char("Copyover FAILED!\n\r", ch);

    /* Here you might want to reopen fpReserve */
    fpReserve = fopen(NULL_FILE, "r");
}

/* Recover from a copyover - load players */
void copyover_recover()
{
    DESCRIPTOR_DATA *d;
    FILE *fp;
    char name[100];
    char host[MSL];
    int desc;
    bool fOld;

    logger->log_stringf("Copyover recovery initiated");

    fp = fopen(COPYOVER_FILE, "r");

    if (!fp) /* there are some descriptors open which will hang forever then ? */
    {
        perror("copyover_recover:fopen");
        logger->log_stringf("Copyover file not found. Exitting.\n\r");
        exit(1);
    }

    unlink(COPYOVER_FILE); /* In case something crashes - doesn't prevent reading  */

    for (;;)
    {
        fscanf(fp, "%d %s %s\n", &desc, name, host);
        if (desc == -1)
            break;

        /* Write something, and check if it goes error-free */
        if (!SocketHelper::write_to_descriptor(desc, "\n\rRestoring from copyover...\n\r", 0))
        {
            close(desc); /* nope */
            continue;
        }

        d = new_descriptor();
        d->descriptor = desc;

        d->host = str_dup(host);
        d->next = descriptor_list;
        descriptor_list = d;
        d->connected = ConnectedState::CopyoverRecover; /* -15, so close_socket frees the char */

        /* Now, find the pfile */

        fOld = load_char_obj(d, name);

        if (!fOld) /* Player file not found?! */
        {
            SocketHelper::write_to_descriptor(desc, "\n\rSomehow, your character was lost in the copyover. Sorry.\n\r", 0);
            SocketHelper::close_socket(d);
        }
        else /* ok! */
        {
            SocketHelper::write_to_descriptor(desc, "\n\rCopyover recovery complete.\n\r", 0);

            /* Just In Case */
            if (!d->character->in_room)
                d->character->in_room = get_room_index(ROOM_VNUM_TEMPLE);

            char_list.push_back(d->character);

            char_to_room(d->character, d->character->in_room);
            do_look(d->character, (char *)"auto");
            act("$n materializes!", d->character, NULL, NULL, TO_ROOM, POS_RESTING);
            d->connected = ConnectedState::Playing;

            if (d->character->pet != NULL)
            {
                char_to_room(d->character->pet, d->character->in_room);
                act("$n materializes!.", d->character->pet, NULL, NULL, TO_ROOM, POS_RESTING);
            }
        }
    }
    fclose(fp);
}

void do_giveskill(Character *ch, char *argument)
{
    Character *victim;
    int sn;
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char arg3[MAX_INPUT_LENGTH];
    char arg4[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];

    if (ch->isNPC())
        return;

    if (argument[0] == '\0')
    {
        send_to_char("Syntax: giveskill <character> <skill> <level> <practiced> <difficulty>\n\r", ch);
        return;
    }

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);
    argument = one_argument(argument, arg3);
    argument = one_argument(argument, arg4);

    if ((victim = get_char_world(ch, arg1)) == NULL)
    {
        send_to_char("They aren't here.\n\r", ch);
        return;
    }

    if ((victim->getTrust() >= ch->getTrust() && ch != victim) || IS_NPC(victim))
    {
        send_to_char("Not on them.\n\r", ch);
        return;
    }

    if (arg2[0] == '\0')
    {
        do_giveskill(ch, (char *)"");
        return;
    }

    if ((sn = skill_lookup(arg2)) < 0)
    {
        send_to_char("That skill does not exist.\n\r", ch);
        return;
    }

    if (arg3[0] == '\0')
    {
        do_giveskill(ch, (char *)"");
        return;
    }

    if (atoi(arg3) < 1 || atoi(arg3) > MAX_LEVEL)
    {
        snprintf(buf, sizeof(buf), "Level range for this skill is 1 through %d.\n\r", MAX_LEVEL);
        send_to_char(buf, ch);
        return;
    }

    if (arg4[0] == '\0')
    {
        do_giveskill(ch, (char *)"");
        return;
    }

    if (atoi(arg4) < -1 || atoi(arg4) > 100 || atoi(arg4) == 0)
    {
        send_to_char("You must enter a learned range from 1 to 100.\n\r", ch);
        send_to_char("If you are removing this skill, enter -1.\n\r", ch);
        return;
    }

    if (argument[0] == '\0')
    {
        do_giveskill(ch, (char *)"");
        return;
    }

    if (atoi(argument) < 1 || atoi(argument) > 10)
    {
        send_to_char("You must enter a difficulty level between 1 and 10.\n\r", ch);
        return;
    }

    ((PlayerCharacter*)victim)->sk_level[sn] = atoi(arg3);
    ((PlayerCharacter*)victim)->learned[sn] = atoi(arg4);
    ((PlayerCharacter*)victim)->sk_rating[sn] = atoi(argument);
    send_to_char("Done.\n\r", ch);
    return;
}

void do_mplist(Character *ch, char *argument)
{
    MPROG_CODE *mProg;
    AREA_DATA *pArea;
    char buf[MAX_STRING_LENGTH];
    BUFFER *buf1;
    bool found;
    int vnum;
    int col = 0;

    pArea = ch->in_room->area;
    buf1 = new_buf();
    found = FALSE;

    for (vnum = pArea->min_vnum; vnum <= pArea->max_vnum; vnum++)
    {
        if ((mProg = get_mprog_index(vnum)))
        {
            found = TRUE;
            snprintf(buf, sizeof(buf), "[%5d] ",
                     mProg->vnum);
            add_buf(buf1, buf);
            if (++col % 6 == 0)
                add_buf(buf1, "\n\r");
        }
    }

    if (!found)
    {
        send_to_char("No mprogs found in this area.\n\r", ch);
        return;
    }

    if (col % 6 != 0)
        add_buf(buf1, "\n\r");

    page_to_char(buf_string(buf1), ch);
    free_buf(buf1);
    return;
}

void do_bonus(Character *ch, char *argument)
{
    Character *victim;
    char buf[MAX_STRING_LENGTH];
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    int value;

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);

    if (arg1[0] == '\0' || arg2[0] == '\0' || !is_number(arg2))
    {
        send_to_char("Syntax: bonus <char> <Exp>.\n\r", ch);
        return;
    }

    if ((victim = get_char_world(ch, arg1)) == NULL)
    {
        send_to_char("That Player is not here.\n\r", ch);
        return;
    }

    if (IS_NPC(victim))
    {
        send_to_char("Not on NPC's.\n\r", ch);
        return;
    }

    if (ch == victim)
    {
        send_to_char("You may not bonus yourself.\n\r", ch);
        return;
    }

    if (IS_IMMORTAL(victim) || victim->level >= LEVEL_IMMORTAL)
    {
        send_to_char("You can't bonus immortals silly!\n\r", ch);
        return;
    }

    value = atoi(arg2);

    if (value < -5000 || value > 5000)
    {
        send_to_char("Value range is -5000 to 5000.\n\r", ch);
        return;
    }

    if (value == 0)
    {
        send_to_char("The value must not be equal to 0.\n\r", ch);
        return;
    }

    victim->gain_exp(value);

    snprintf(buf, sizeof(buf), "You have bonused %s a whopping %d experience points.\n\r",
             victim->getName().c_str(), value);
    send_to_char(buf, ch);

    if (value > 0)
    {
        snprintf(buf, sizeof(buf), "You have been bonused %d experience points.\n\r", value);
        send_to_char(buf, victim);
    }
    else
    {
        snprintf(buf, sizeof(buf), "You have been penalized %d experience points.\n\r", value);
        send_to_char(buf, victim);
    }

    return;
}

void do_laston(Character *ch, char *argument)
{
    Character *victim;
    FILE *fp;
    char strsave[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    char arg1[MAX_STRING_LENGTH];
    long laston = 0;
    int level = 0;
    bool finished = FALSE;
    int d, h, m, s;

    argument = one_argument(argument, arg1);

    if (arg1[0] == '\0')
    {
        send_to_char("Laston whom?\n\r", ch);
        return;
    }

    if ((victim = get_char_world(ch, arg1)) != NULL)
    {
        send_to_char("That person is still playing.\n\r", ch);
        return;
    }

    snprintf(strsave, sizeof(strsave), "%s%s", PLAYER_DIR, capitalize(arg1));
    if ((fp = fopen(strsave, "r")) == NULL)
    {
        snprintf(buf, sizeof(buf), "{W%s {bhas not been found...{x\n\r", capitalize(arg1));
        send_to_char(buf, ch);
        return;
    }

    for (;;)
    {
        char *word;
        fread_to_eol(fp);
        word = feof(fp) ? (char *)"End" : fread_word(fp);
        switch (UPPER(word[0]))
        {
        case 'E':
            if (!str_cmp(word, "End"))
            {
                finished = TRUE;
                break;
            }
            break;
        case 'L':
            if (!str_cmp(word, "Levl"))
            {
                level = fread_number(fp);
            }
            if (!str_cmp(word, "LogO"))
            {
                laston = fread_number(fp);
            }
            break;
        case '#':
            finished = TRUE;
            break;
        default:
            fread_to_eol(fp);
            break;
        }
        if (finished)
            break;
    }

    s = current_time - laston;
    d = s / 86400;
    s -= d * 86400;
    h = s / 3600;
    s -= h * 3600;
    m = s / 60;
    s -= m * 60;

    snprintf(buf, sizeof(buf), "{bName{w:  {W%10s  {bLevel{w:  {W%10d\n\r",
             capitalize(arg1), level);
    send_to_char(buf, ch);

    if (IS_IMMORTAL(ch))
    {
        snprintf(buf, sizeof(buf), "\n\r{bLast connected: {W%d {bdays, {W%d {bhours, {W%d {bminutes and {W%d {bseconds ago.{x\n\r", d, h, m, s);
        send_to_char(buf, ch);
    }

    return;
}
