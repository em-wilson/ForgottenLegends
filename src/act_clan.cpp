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
 *	ROM 2.4 is copyright 1993-1996 Russ Taylor			   				               *
 *	ROM has been brought to you by the ROM consortium		 		            	   *
 *	    Russ Taylor (rtaylor@efn.org)				 				                    	   *
 *	    Gabrielle Taylor											                               *
 *	    Brian Moore (zump@rom.org)					                                 *
 *	By using this code, you have agreed to follow the terms of the	         *
 *	ROM license, in the file Rom24/doc/rom.license			                     *
 ***************************************************************************/

#include <sys/time.h>
#include "merc.h"
#include "tables.h"
#include "clans/ClanManager.h"
#include "ConnectedState.h"
#include "PlayerCharacter.h"

DECLARE_DO_FUN(do_clist);

extern ClanManager *clan_manager;

void show_flag_cmds(Character *ch, const struct flag_type *flag_table);
int flag_value args((const struct flag_type *flag_table, char *argument));
char *flag_string args((const struct flag_type *flag_table, int bits));

void do_join(Character *caller, char *argument)
{
    if (caller->isNPC())
    {
        send_to_char("NPCs may not join clans\n\r", caller);
        return;
    }

    PlayerCharacter *ch = (PlayerCharacter *)caller;
    char buf[MAX_STRING_LENGTH];

    if (argument[0] == '\0')
    {
        send_to_char("Clans Available fogr you to Join:\n\r", ch);
        send_to_char("================================\n\r", ch);

        do_clist(ch, (char *)"");

        send_to_char("\n\r", ch);
        return;
    }

    Clan *join = clan_manager->get_clan(argument);
    if (!join)
    {
        snprintf(buf, sizeof(buf), "%s: that clan does not exist.\n\r", argument);
        send_to_char(buf, ch);
        return;
    }

    ch->setJoin(join);
    snprintf(buf, sizeof(buf), "You are now eligable to join %s.\n\r", join->getName().c_str());
    send_to_char(buf, ch);
    return;
}

void do_cedit(Character *caller, char *argument)
{
    if (caller->isNPC())
    {
        send_to_char("NPCs may not join clans\n\r", caller);
        return;
    }

    PlayerCharacter *ch = (PlayerCharacter *)caller;
    if (!ch->isClanned() || ch->getName() != ch->getClan()->getLeader())
    {
        send_to_char("Only clan leaders may use this command.", ch);
        return;
    }
    else
    {
        send_to_char("Clan Customization Menu:\n\r"
                     "1: Admin Options (leader only)\n\r"
                     "2: Room Options\n\r"
                     "3: Mobile Options\n\r"
                     "4: Shop Options\n\r"
                     "5: Exit Editor\n\r",
                     ch);
        ch->setClanCust(3);
        ch->desc->connected = ConnectedState::ClanCreate;
        return;
    }
}

/*
 * This allows imms to delete clans
 * -Blizzard
 */
void do_delclan(Character *ch, char *argument)
{
    Clan *clan;

    if ((clan = clan_manager->get_clan(argument)) == NULL)
    {
        send_to_char("That clan does not exist.\n\r", ch);
        return;
    }

    // TODO: If any online characters are members of this clan, they
    // 		 need to be removed from it first.
    clan_manager->delete_clan(clan);
    send_to_char("Clan deleted.\n\r", ch);
}

void do_induct(Character *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];

    if (ch->isNPC())
    {
        send_to_char("Huh?\n\r", ch);
        return;
    }

    PlayerCharacter *pch = (PlayerCharacter *)ch;
    Clan *clan = pch->getClan();

    if (!clan)
    {
        send_to_char("You are not in a clan.\n\r", ch);
        return;
    }

    if (!clan_manager->isClanLeader(ch))
    {
        send_to_char("Only clan leaders can do that.\n\r", ch);
        return;
    }

    argument = one_argument(argument, arg);

    if (arg[0] == '\0')
    {
        send_to_char("Induct whom?\n\r", ch);
        return;
    }

    Character *victim;
    if ((victim = get_char_world(ch, arg)) == NULL)
    {
        send_to_char("That player is not here.\n\r", ch);
        return;
    }

    if (victim->isNPC())
    {
        send_to_char("Not on NPC's.\n\r", ch);
        return;
    }

    PlayerCharacter *pvict = (PlayerCharacter *)victim;
    if (victim->level < 10)
    {
        send_to_char("This player is not worthy of joining yet.\n\r", ch);
        return;
    }

    if (!pvict->wantsToJoinClan(clan))
    {
        send_to_char("They do not wish to join your clan.\n\r", ch);
        return;
    }

    if (pvict->isClanned() && pvict->getClan() == pch->getClan())
    {
        send_to_char("They are already in your clan!\n\r", ch);
        return;
    }

    clan_manager->add_player(pvict, clan);
    act("You induct $N into $t", ch, clan->getName().c_str(), victim, TO_CHAR, POS_RESTING);
    act("$n inducts you into $t", ch, clan->getName().c_str(), victim, TO_VICT, POS_RESTING);
    save_char_obj(victim);
    clan_manager->save_clan(clan);
    return;
}

void do_outcast(Character *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    Character *victim;

    if (ch->isNPC())
    {
        send_to_char("Huh?\n\r", ch);
        return;
    }

    PlayerCharacter *pch = (PlayerCharacter *)ch;
    Clan *clan = pch->getClan();

    if (!clan)
    {
        send_to_char("You are not in a clan.\n\r", ch);
        return;
    }

    if (!clan_manager->isClanLeader(ch))
    {
        send_to_char("Only clan leadership can do that.\n\r", ch);
        return;
    }

    argument = one_argument(argument, arg);

    if (arg[0] == '\0')
    {
        send_to_char("Outcast whom?\n\r", ch);
        return;
    }

    if ((victim = get_char_world(ch, arg)) == NULL)
    {
        send_to_char("That player is not here.\n\r", ch);
        return;
    }

    if (IS_NPC(victim))
    {
        send_to_char("Not on NPC's.\n\r", ch);
        return;
    }
    PlayerCharacter *pvict = (PlayerCharacter *)victim;

    if (victim == ch)
    {
        send_to_char("Kick yourself out of your own clan?\n\r", ch);
        return;
    }

    if (pvict->getClan() != pch->getClan())
    {
        send_to_char("This player does not belong to your clan!\n\r", ch);
        return;
    }
    if (victim->getName() == clan->getLeader())
    {
        send_to_char("You cannot loner your own leader!\n\r", ch);
        return;
    }
    if (victim->getName() == clan->getFirstOfficer())
    {
        clan->removeFirstOfficer();
    }
    if (victim->getName() == clan->getSecondOfficer())
    {
        clan->removeSecondOfficer();
    }
    Clan *outcast = clan_manager->get_clan((char *)"outcast");
    act("You outcast $N from $t", ch, clan->getName().c_str(), victim, TO_CHAR, POS_RESTING);
    act("$n outcasts you from $t", ch, clan->getName().c_str(), victim, TO_VICT, POS_RESTING);
    clan_manager->add_player(pvict, outcast);
    save_char_obj(victim); /* clan gets saved when pfile is saved */
    return;
}

void do_setclan(Character *ch, char *argument)
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];

    if (ch->isNPC())
    {
        send_to_char("Huh?\n\r", ch);
        return;
    }

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);
    if (arg1[0] == '\0')
    {
        send_to_char("Usage: set clan <clan> <field> <argument>\n\r", ch);
        send_to_char("\n\rField being one of:\n\r", ch);
        send_to_char(" leader recruiter1 recruiter2\n\r", ch);
        send_to_char(" members deathroom flags\n\r", ch);
        send_to_char(" name filename motto desc\n\r", ch);
        send_to_char(" pkill pdeath money\n\r", ch);
        return;
    }

    Clan *clan = clan_manager->get_clan(arg1);
    if (!clan)
    {
        send_to_char("No such clan.\n\r", ch);
        return;
    }
    if (!str_prefix(arg2, "leader"))
    {
        clan->setLeader(argument);
        send_to_char("Done.\n\r", ch);
        clan_manager->save_clan(clan);
        return;
    }
    if (!str_prefix(arg2, "recruiter1"))
    {
        clan->setFirstOfficer(argument);
        send_to_char("Done.\n\r", ch);
        clan_manager->save_clan(clan);
        return;
    }
    if (!str_prefix(arg2, "recruiter2"))
    {
        clan->setSecondOfficer(argument);
        send_to_char("Done.\n\r", ch);
        clan_manager->save_clan(clan);
        return;
    }
    if (!str_prefix(arg2, "members"))
    {
        clan->setMemberCount(atoi(argument));
        send_to_char("Done.\n\r", ch);
        clan_manager->save_clan(clan);
        return;
    }
    if (!str_prefix(arg2, "deathroom"))
    {
        clan->setDeathRoomVnum(atoi(argument));
        send_to_char("Death room set.\n\r", ch);
        clan_manager->save_clan(clan);
        return;
    }
    if (!str_prefix(arg2, "flags"))
    {
        int value;

        if (argument[0] == '\0')
        {
            show_flag_cmds(ch, clan_flags);
            return;
        }

        if ((value = flag_value(clan_flags, argument)) != NO_FLAG)
        {
            clan->toggleFlag(value);
            send_to_char("Clan flags toggled.\n\r", ch);
            clan_manager->save_clan(clan);
            return;
        }

        send_to_char("That flag does not exist.\n\rList of flags:\n\r", ch);
        do_setclan(ch, (char *)"flags");
        return;
    }
    if (!str_prefix(arg2, "whoname"))
    {
        clan->setWhoname(argument);
        send_to_char("Whoname set.\n\r", ch);
        clan_manager->save_clan(clan);
        return;
    }
    if (!str_prefix(arg2, "name"))
    {
        clan->setName(argument);
        send_to_char("Done.\n\r", ch);
        clan_manager->save_clan(clan);
        return;
    }
    if (!str_prefix(arg2, "filename"))
    {
        clan->setFilename(argument);
        send_to_char("Done.\n\r", ch);
        clan_manager->save_clan(clan);
        clan_manager->write_clan_list();
        return;
    }
    if (!str_prefix(arg2, "motto"))
    {
        clan->setMotto(argument);
        send_to_char("Done.\n\r", ch);
        clan_manager->save_clan(clan);
        return;
    }
    if (!str_prefix("played", arg2))
    {
        clan->setPlayTime(atoi(argument));
        send_to_char("Ok.\n\r", ch);
        clan_manager->save_clan(clan);
        return;
    }
    if (!str_prefix("pkill", arg2))
    {
        clan->setPkills(atoi(argument));
        send_to_char("Ok.\n\r", ch);
        clan_manager->save_clan(clan);
        return;
    }
    if (!str_prefix("pdeath", arg2))
    {
        clan->setPdeaths(atoi(argument));
        send_to_char("Ok.\n\r", ch);
        clan_manager->save_clan(clan);
        return;
    }
    if (!str_prefix("money", arg2))
    {
        clan->setMoney(atoi(argument));
        send_to_char("Money set.\n\r", ch);
        clan_manager->save_clan(clan);
        return;
    }
    do_setclan(ch, (char *)"");
    return;
}

void do_makeclan(Character *ch, char *argument)
{
    if (ch->getTrust() < MAX_LEVEL)
    {
        send_to_char("Only implementors can run this command.", ch);
        return;
    }

    if (!argument || argument[0] == '\0')
    {
        send_to_char("Usage: makeclan <clan name>\n\r", ch);
        return;
    }

    argument[0] = LOWER(argument[0]);

    Clan *clan = new Clan();
    clan->setName(argument);
    clan_manager->add_clan(clan);

    send_to_char("Clan created.\n\r", ch);
    return;
}

/*
 * Added multiple level pkill and pdeath support. --Shaddai
 */

void do_clist(Character *ch, char *argument)
{
    auto clans = clan_manager->get_all_clans();
    DESCRIPTOR_DATA *d;
    int count = 0, year = 0, hour = 0;
    char buf[MAX_STRING_LENGTH];

    if (argument[0] == '\0')
    {
        send_to_char("\n\rClan          Leader       Pkills       Times Pkilled\n\r_________________________________________________________________________\n\r", ch);
        send_to_char("PK Clans\n\r", ch);
        for (auto clan : clans)
        {
            if (clan->hasFlag(CLAN_NOSHOW) || !clan->hasFlag(CLAN_PK))
                continue;
            snprintf(buf, sizeof(buf), "%-13s %-13s", clan->getName().c_str(), clan->getLeader().c_str());
            send_to_char(buf, ch);
            snprintf(buf, sizeof(buf), "%-13d%-13d\n\r", clan->getPkills(), clan->getPdeaths());
            send_to_char(buf, ch);
            count++;
        }

        send_to_char("\n\rNon-PK Clans\n\r", ch);
        for (auto clan : clans)
        {
            if (clan->hasFlag(CLAN_PK) || clan->hasFlag(CLAN_NOSHOW))
                continue;
            snprintf(buf, sizeof(buf), "%-13s %-13s", clan->getName().c_str(), clan->getLeader().c_str());
            send_to_char(buf, ch);
            snprintf(buf, sizeof(buf), "%-13d%-13d\n\r", clan->getPkills(), clan->getPdeaths());
            send_to_char(buf, ch);
            count++;
        }

        if (IS_IMMORTAL(ch))
        {
            send_to_char("\n\rClans Players Can't See:\n\r", ch);
            for (auto clan : clans)
            {
                if (!clan->hasFlag(CLAN_NOSHOW))
                    continue;
                snprintf(buf, sizeof(buf), "%-13s %-13s", clan->getName().c_str(), clan->getLeader().c_str());
                send_to_char(buf, ch);
                snprintf(buf, sizeof(buf), "%-13d%-13d\n\r", clan->getPkills(), clan->getPdeaths());
                send_to_char(buf, ch);
                count++;
            }
        }
        if (!count)
            send_to_char("There are no Clans currently formed.\n\r", ch);
        else
            send_to_char("_________________________________________________________________________\n\r\n\rUse 'clist <clan>' for detailed information and a breakdown of victories.\n\r", ch);
        return;
    }

    Clan *clan = clan_manager->get_clan(argument);
    if (!clan || (clan->hasFlag(CLAN_NOSHOW) && !IS_IMMORTAL(ch)))
    {
        send_to_char("No such clan.\n\r", ch);
        return;
    }

    snprintf(buf, sizeof(buf), "\n\r%s, %s\n\r'%s'\n\r\n\r", clan->getName().c_str(), clan->getWhoname().c_str(), clan->getMotto().c_str());
    send_to_char(buf, ch);
    snprintf(buf, sizeof(buf), "Pkills: %d\n\r", clan->getPkills());
    send_to_char(buf, ch);
    snprintf(buf, sizeof(buf), "Pdeaths: %d\n\r", clan->getPdeaths());
    send_to_char(buf, ch);
    snprintf(buf, sizeof(buf), "Leader:    %s\n\rRecruiter:  %s\n\rRecruiter: %s\n\rMembers    :  %d\n\r",
             clan->getLeader().c_str(),
             clan->getFirstOfficer().c_str(),
             clan->getSecondOfficer().c_str(),
             clan->countMembers());
    send_to_char(buf, ch);

    if (ch->isClanned() || IS_IMMORTAL(ch))
    {
        PlayerCharacter *pch = (PlayerCharacter*)ch;
        if (IS_IMMORTAL(ch) || pch->getClan() == clan)
        {
            snprintf(buf, sizeof(buf), "Funds:    %ld (gold)\n\r", clan->getMoney() / 100);
            send_to_char(buf, ch);
        }
        else if (clan->getMoney() > pch->getClan()->getMoney())
            send_to_char("This clan has more money than yours.\n\r", ch);
        else
            send_to_char("Your clan has more money than this.\n\r", ch);
    }

    hour = clan->getPlayTime() / 3600;

    for (d = descriptor_list; d; d = d->next) {
        if (d->character->isClanned() && d->character->getClan() == clan)
            hour += (current_time - d->character->logon) / 3600;
    }

    if (hour > 8760)
        year = hour / 8760;

    snprintf(buf, sizeof(buf), "Member Online Time: %d years %d hours\n\r", year, hour);
    send_to_char(buf, ch);

    if (IS_IMMORTAL(ch))
    {
        send_to_char("Immortal Stuff:\n\r", ch);
        send_to_char("===============\n\r", ch);

        snprintf(buf, sizeof(buf), "Flags: %s\n\r", flag_string(clan_flags, clan->getFlags()));
        send_to_char(buf, ch);
    }
    return;
}