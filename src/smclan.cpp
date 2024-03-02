/****************************************************************************
 * [S]imulated [M]edieval [A]dventure multi[U]ser [G]ame      |   \\._.//   *
 * -----------------------------------------------------------|   (0...0)   *
 * SMAUG 1.4 (C) 1994, 1995, 1996, 1998  by Derek Snider      |    ).:.(    *
 * -----------------------------------------------------------|    {o o}    *
 * SMAUG code team: Thoric, Altrag, Blodkai, Narn, Haus,      |   / ' ' \   *
 * Scryn, Rennard, Swordbearer, Gorog, Grishnakh, Nivek,      |~'~.VxvxV.~'~*
 * Tricops and Fireblade                                      |             *
 * ------------------------------------------------------------------------ *
 *			     Special clan module			    *
 ****************************************************************************/

#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <limits.h>
#include <time.h>
#include "merc.h"
#include "interp.h"
#include "tables.h"
#include "clans/ClanManager.h"
#include "PlayerCharacter.h"

using std::string;

#define MAX_NEST	100
//static	OBJ_DATA *	rgObjNest	[MAX_NEST];
void show_flag_cmds( Character *ch, const struct flag_type *flag_table );
extern ClanManager * clan_manager;

/* local routines */
void	fread_clan	args( ( Clan *clan, FILE *fp ) );
bool	load_clan_file	args( ( char *clanfile ) );
int flag_value                  args ( ( const struct flag_type *flag_table,
                                         char *argument) );
char *flag_string               args ( ( const struct flag_type *flag_table,
                                         int bits ) );

/*
 * Read in actual clan data.
 */

#define KEYFN( literal, fn, value )					\
				if ( !str_cmp( word, literal ) )	\
				{					\
				    fn(value);			\
				    fMatch = TRUE;			\
				    break;				\
				}

/*
 * Reads in PKill and PDeath still for backward compatibility but now it
 * should be written to PKillRange and PDeathRange for multiple level pkill
 * tracking support. --Shaddai
 * Added a hardcoded limit memlimit to the amount of members a clan can 
 * have set using setclan.  --Shaddai
 */

void fread_clan( Clan *clan, FILE *fp )
{
    char buf[MAX_STRING_LENGTH];
    char *word;
    bool fMatch;

    for ( ; ; )
    {
		word   = feof( fp ) ? (char*)"End" : fread_word( fp );
		fMatch = FALSE;

		switch ( UPPER(word[0]) )
		{
			case '*':
				fMatch = TRUE;
				fread_to_eol( fp );
				break;

			case '#':
				if ( !str_cmp( word, "#END" ) )
				{
					return;
				}

			case 'D':
				KEYFN( "Death",		clan->setDeathRoomVnum,		fread_number( fp ) 			);
				break;

			// case 'E':
			// 	if ( !str_cmp( word, "End" ) )
			// 	{
			// 		return;
			// 	}

			case 'F':
				KEYFN( "Filename",	clan->setFilename,			fread_string_nohash( fp ) 	);
				KEYFN( "Flags",		clan->setFlags,				fread_flag( fp ) 			);

			case 'L':
				KEYFN( "Leader",	clan->setLeader,			fread_string( fp ) 			);
				break;

			case 'M':
				KEYFN( "MDeaths",	clan->setMdeaths,			fread_number( fp ) 			);
				KEYFN( "Members",	clan->setMemberCount,		fread_number( fp ) 			);
				KEYFN( "MKills",	clan->setMkills,			fread_number( fp ) 			);
				KEYFN( "Motto",		clan->setMotto,				fread_string( fp ) 			);
				KEYFN( "Money",		clan->setMoney,				fread_number( fp ) 			);
				break;
		
			case 'N':
				KEYFN( "Name",		clan->setName,				fread_string( fp )			);
				KEYFN( "NumberOne",	clan->setFirstOfficer,		fread_string( fp )			);
				KEYFN( "NumberTwo",	clan->setSecondOfficer,		fread_string( fp ) 			);
				break;

			case 'P':
				KEYFN( "PDeaths",	clan->setPdeaths,			fread_number( fp ) 			);
				KEYFN( "PKills",	clan->setPkills,			fread_number( fp ) 			);
				KEYFN( "Played",	clan->setPlayTime,			fread_number( fp ) 			);
				break;

			case 'W':
				KEYFN( "Whoname",	clan->setWhoname,			fread_string( fp ) 			);
				break;
		}
		
		if ( !fMatch )
		{
			snprintf(buf, sizeof(buf), "Fread_clan: no match: %s", word );
			bug( buf, 0 );
		}
    }
}

/*
 * Load a clan file
 */

bool load_clan_file( char *clanfile )
{
    char filename[256];
    Clan *clan = new Clan();
    FILE *fp;
    bool found;

    found = FALSE;
    snprintf(filename, sizeof(filename), "%s%s", CLAN_DIR, clanfile );

    if ( ( fp = fopen( filename, "r" ) ) != NULL )
    {

		found = TRUE;
		for ( ; ; )
		{
			char letter;
			char *word;

			letter = fread_letter( fp );
			if ( letter == '*' )
			{
			fread_to_eol( fp );
			continue;
			}

			if ( letter != '#' )
			{
				bug( "Load_clan_file: # not found.", 0 );
				break;
			}

			word = fread_word( fp );
			if ( !str_cmp( word, "CLAN"	) )
			{
				fread_clan( clan, fp );
				break;
			}
			else
			if ( !str_cmp( word, "END"	) )
				break;
			else
			{
			char buf[MAX_STRING_LENGTH];

			snprintf(buf, sizeof(buf), "Load_clan_file: bad section: %s.", word );
			bug( buf, 0 );
			break;
			}
		}
		fclose( fp );
    }

    if ( found ) {
		clan_manager->add_clan(clan);
	} else {
		delete clan;
	}

    return found;
}

/*
 * This allows imms to delete clans
 * -Blizzard
 */
void do_delclan( Character *ch, char *argument )
{
	Clan *clan;

    if ((clan = clan_manager->get_clan(argument)) == NULL)
    {
		send_to_char("That clan does not exist.\n\r",ch);
		return;
    }

	// TODO: If any online characters are members of this clan, they
	// 		 need to be removed from it first.
	clan_manager->delete_clan(clan);
	send_to_char("Clan deleted.\n\r",ch);
}

/*
 * Load in all the clan files.
 */
void load_clans( )
{
    FILE *fpList;
    char *filename;
    char clanlist[256];
    char buf[MAX_STRING_LENGTH];

    log_string( "Loading clans..." );

    snprintf(clanlist, sizeof(clanlist), "%s%s", CLAN_DIR, CLAN_LIST );
    fclose( fpReserve );
    if ( ( fpList = fopen( clanlist, "r" ) ) == NULL )
    {
	perror( clanlist );
	exit( 1 );
    }

    for ( ; ; )
    {
	filename = feof( fpList ) ? (char*)"$" : fread_word( fpList );
	log_string( filename );
	if ( filename[0] == '$' )
	  break;

	if ( !load_clan_file( filename ) )
	{
	  snprintf(buf, sizeof(buf), "Cannot load clan file: %s", filename );
	  bug( buf, 0 );
	}
    }
    fclose( fpList );
    log_string(" Done clans " );
    fpReserve = fopen( NULL_FILE, "r" );
    return;
}

void do_induct( Character *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    Clan *clan;

    if ( IS_NPC( ch ) || !ch->pcdata->clan )
    {
		send_to_char( "Huh?\n\r", ch );
		return;
    }

    clan = ch->pcdata->clan;
    
    if ( !str_cmp( ch->getName(), clan->getLeader().c_str()  )
    ||   !str_cmp( ch->getName(), clan->getFirstOfficer().c_str() )
    ||   !str_cmp( ch->getName(), clan->getSecondOfficer().c_str() ) )
	;
    else
    {
		send_to_char( "Huh?\n\r", ch );
		return;
    }

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
		send_to_char( "Induct whom?\n\r", ch );
		return;
    }

	Character *victim;
    if ( ( victim = get_char_world( ch, arg ) ) == NULL )
    {
		send_to_char( "That player is not here.\n\r", ch);
		return;
    }

    if ( victim->isNPC() )
    {
		send_to_char( "Not on NPC's.\n\r", ch );
		return;
    }

	if ( victim->level < 10 )
	{
	    send_to_char( "This player is not worthy of joining yet.\n\r", ch );
	    return;
	}

	if (!((PlayerCharacter *) victim)->wantsToJoinClan(ch->pcdata->clan))
	{
		send_to_char("They do not wish to join your clan.\n\r",ch);
		return;
	}

    if ( victim->pcdata->clan )
    {
		if (victim->pcdata->clan == ch->pcdata->clan)
		{
			send_to_char("They are already in your clan!\n\r",ch);
			return;
		}
    }
    
	clan_manager->add_player(victim, clan);

    victim->pcdata->clan = clan;
    act( "You induct $N into $t", ch, clan->getName().c_str(), victim, TO_CHAR );
    act( "$n inducts you into $t", ch, clan->getName().c_str(), victim, TO_VICT );
    save_char_obj( victim );
    clan_manager->save_clan( clan );
    return;
}

void do_outcast( Character *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    Character *victim;
    Clan *clan;

    if ( IS_NPC( ch ) || !ch->pcdata->clan )
    {
	send_to_char( "Huh?\n\r", ch );
	return;
    }

    clan = ch->pcdata->clan;

    if ( !str_cmp( ch->getName(), clan->getLeader().c_str()  )
    ||   !str_cmp( ch->getName(), clan->getFirstOfficer().c_str() )
    ||   !str_cmp( ch->getName(), clan->getSecondOfficer().c_str() ) )
	;
    else
    {
		send_to_char( "Huh?\n\r", ch );
		return;
    }


    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
		send_to_char( "Outcast whom?\n\r", ch );
		return;
    }

    if ( ( victim = get_char_world( ch, arg ) ) == NULL )
    {
		send_to_char( "That player is not here.\n\r", ch);
		return;
    }

    if ( IS_NPC(victim) )
    {
		send_to_char( "Not on NPC's.\n\r", ch );
		return;
    }

    if ( victim == ch )
    {
	    send_to_char( "Kick yourself out of your own clan?\n\r", ch );
	    return;
    }
 
    if ( victim->pcdata->clan != ch->pcdata->clan )
    {
	    send_to_char( "This player does not belong to your clan!\n\r", ch );
	    return;
    }
    if ( !str_cmp(victim->getName(), clan->getLeader().c_str() ) )
    {
		send_to_char("You cannot loner your own leader!\n\r",ch);
		return;
    }
    if ( !str_cmp( victim->getName(), clan->getFirstOfficer().c_str() ) )
    {
		clan->removeFirstOfficer();
    }
    if ( !str_cmp( victim->getName(), clan->getSecondOfficer().c_str() ) )
    {
		clan->removeSecondOfficer();
    }
    Clan *outcast = clan_manager->get_clan((char*)"outcast");
    act( "You outcast $N from $t", ch, clan->getName().c_str(), victim, TO_CHAR );
    act( "$n outcasts you from $t", ch, clan->getName().c_str(), victim, TO_VICT );
	clan_manager->add_player(victim, outcast);
    save_char_obj( victim );	/* clan gets saved when pfile is saved */
    return;
}

void do_setclan( Character *ch, char *argument )
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];

    if ( IS_NPC( ch ) )
    {
		send_to_char( "Huh?\n\r", ch );
		return;
    }

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    if ( arg1[0] == '\0' )
    {
		send_to_char( "Usage: set clan <clan> <field> <argument>\n\r", ch);
		send_to_char( "\n\rField being one of:\n\r", ch );
		send_to_char( " leader recruiter1 recruiter2\n\r", ch ); 
		send_to_char( " members deathroom flags\n\r", ch );
		send_to_char( " name filename motto desc\n\r", ch );
		send_to_char( " pkill pdeath money\n\r", ch );
		return;
    }

    Clan *clan = clan_manager->get_clan( arg1 );
    if ( !clan )
    {
		send_to_char( "No such clan.\n\r", ch );
		return;
    }
    if ( !str_prefix( arg2, "leader" ) )
    {
		clan->setLeader( argument );
		send_to_char( "Done.\n\r", ch );
		clan_manager->save_clan( clan );
		return;
    }
    if ( !str_prefix( arg2, "recruiter1" ) )
    {
		clan->setFirstOfficer( argument );
		send_to_char( "Done.\n\r", ch );
		clan_manager->save_clan( clan );
		return;
    }
    if ( !str_prefix( arg2, "recruiter2" ) )
    {
		clan->setSecondOfficer( argument );
		send_to_char( "Done.\n\r", ch );
		clan_manager->save_clan( clan );
		return;
    }
    if ( !str_prefix( arg2, "members" ) )
    {
		clan->setMemberCount( atoi( argument ) );
		send_to_char( "Done.\n\r", ch );
		clan_manager->save_clan( clan );
		return;
    }
    if ( !str_prefix( arg2, "deathroom" ) )
    {
		clan->setDeathRoomVnum( atoi( argument ) );
		send_to_char( "Death room set.\n\r", ch);
		clan_manager->save_clan( clan );
		return;
    }
    if ( !str_prefix( arg2, "flags" ) )
    {
		int value;

		if (argument[0] == '\0')
		{
			show_flag_cmds( ch, clan_flags );
			return;
		}

		if ( ( value = flag_value( clan_flags, argument ) ) != NO_FLAG )
		{
			clan->toggleFlag(value);
			send_to_char("Clan flags toggled.\n\r",ch);
			clan_manager->save_clan(clan);
			return;
		}

		send_to_char("That flag does not exist.\n\rList of flags:\n\r", ch);
		do_setclan(ch,(char*)"flags");
		return;
    }
    if ( !str_prefix( arg2, "whoname" ) )
    {
		clan->setWhoname( argument );
		send_to_char("Whoname set.\n\r",ch);
		clan_manager->save_clan( clan );
		return;
    }
    if ( !str_prefix( arg2, "name" ) )
    {
		clan->setName( argument );
		send_to_char( "Done.\n\r", ch );
		clan_manager->save_clan( clan );
		return;
    }
    if ( !str_prefix( arg2, "filename" ) )
    {
		clan->setFilename( argument );
		send_to_char( "Done.\n\r", ch );
		clan_manager->save_clan( clan );
		clan_manager->write_clan_list( );
		return;
    }
    if ( !str_prefix( arg2, "motto" ) )
    {
		clan->setMotto( argument );
		send_to_char( "Done.\n\r", ch );
		clan_manager->save_clan( clan );
		return;
    }
    if ( !str_prefix( "played", arg2) )
    {
		clan->setPlayTime(atoi( argument ));
		send_to_char ("Ok.\n\r", ch );
		clan_manager->save_clan( clan );
		return;
    }
    if ( !str_prefix( "pkill", arg2) )
    {
		clan->setPkills(atoi( argument ));
		send_to_char ("Ok.\n\r", ch );
		clan_manager->save_clan( clan );
		return;
    }
    if ( !str_prefix( "pdeath", arg2) )
    {
		clan->setPdeaths(atoi( argument ));
		send_to_char ("Ok.\n\r", ch );
		clan_manager->save_clan( clan );
		return;
    }
    if ( !str_prefix( "money", arg2) )
    {
		clan->setMoney(atoi( argument ));
		send_to_char( "Money set.\n\r",ch);
		clan_manager->save_clan( clan );
		return;
    }
    do_setclan( ch, (char*)"" );
    return;
}

void do_makeclan( Character *ch, char *argument )
{
	if (ch->getTrust() < MAX_LEVEL ) {
		send_to_char("Only implementors can run this command.", ch);
		return;
	}

    if ( !argument || argument[0] == '\0' )
    {
	send_to_char( "Usage: makeclan <clan name>\n\r", ch );
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

void do_clist( Character *ch, char *argument )
{
    auto clans = clan_manager->get_all_clans();
    DESCRIPTOR_DATA *d;
    int count = 0, year = 0, hour = 0;
    char buf[MAX_STRING_LENGTH];

    if ( argument[0] == '\0' )
    {
        send_to_char( "\n\rClan          Leader       Pkills       Times Pkilled\n\r_________________________________________________________________________\n\r", ch );
        send_to_char( "PK Clans\n\r",ch);
        for ( auto clan : clans )
        {
			if (clan->hasFlag(CLAN_NOSHOW) || !clan->hasFlag(CLAN_PK))
				continue;
			snprintf(buf, sizeof(buf), "%-13s %-13s", clan->getName().c_str(), clan->getLeader().c_str() );
			send_to_char(buf,ch);
			snprintf(buf, sizeof(buf), "%-13d%-13d\n\r", clan->getPkills(), clan->getPdeaths() );
			send_to_char(buf,ch);
			count++;
        }

        send_to_char( "\n\rNon-PK Clans\n\r",ch);
        for ( auto clan : clans )
        {
			if (clan->hasFlag(CLAN_PK) || clan->hasFlag(CLAN_NOSHOW))
				continue;
			snprintf(buf, sizeof(buf), "%-13s %-13s", clan->getName().c_str(), clan->getLeader().c_str() );
			send_to_char(buf,ch);
			snprintf(buf, sizeof(buf), "%-13d%-13d\n\r", clan->getPkills(), clan->getPdeaths() );
			send_to_char(buf,ch);
            count++;
        }

		if (IS_IMMORTAL(ch))
		{
			send_to_char( "\n\rClans Players Can't See:\n\r",ch);
	        for ( auto clan : clans )
			{
				if (!clan->hasFlag(CLAN_NOSHOW))
					continue;
				snprintf(buf, sizeof(buf), "%-13s %-13s", clan->getName().c_str(), clan->getLeader().c_str() );
				send_to_char(buf,ch);
				snprintf(buf, sizeof(buf), "%-13d%-13d\n\r", clan->getPkills(), clan->getPdeaths() );
				send_to_char(buf,ch);
				count++;
			}
		}
        if ( !count )
          send_to_char( "There are no Clans currently formed.\n\r", ch );
        else
          send_to_char( "_________________________________________________________________________\n\r\n\rUse 'clist <clan>' for detailed information and a breakdown of victories.\n\r", ch );
        return;
    }

    Clan *clan = clan_manager->get_clan( argument );
    if (!clan || (clan->hasFlag(CLAN_NOSHOW) && !IS_IMMORTAL(ch)))
    {
        send_to_char( "No such clan.\n\r", ch );
        return;
    }

    snprintf(buf, sizeof(buf), "\n\r%s, %s\n\r'%s'\n\r\n\r", clan->getName().c_str(), clan->getWhoname().c_str(), clan->getMotto().c_str() );
    send_to_char(buf,ch);
    snprintf(buf, sizeof(buf), "Pkills: %d\n\r", clan->getPkills());
    send_to_char(buf,ch);
    snprintf(buf, sizeof(buf), "Pdeaths: %d\n\r", clan->getPdeaths());
    send_to_char(buf,ch);
    snprintf(buf, sizeof(buf), "Leader:    %s\n\rRecruiter:  %s\n\rRecruiter: %s\n\rMembers    :  %d\n\r",
                        clan->getLeader().c_str(),
                        clan->getFirstOfficer().c_str(),
                        clan->getSecondOfficer().c_str(),
			clan->countMembers() );
    send_to_char(buf,ch);

    if (IS_CLANNED(ch) || IS_IMMORTAL(ch))
    {
	if (IS_IMMORTAL(ch) || ch->pcdata->clan == clan)
	{
	    snprintf(buf, sizeof(buf), "Funds:    %ld (gold)\n\r", clan->getMoney() / 100);
	    send_to_char(buf,ch);
	}
	else if (clan->getMoney() > ch->pcdata->clan->getMoney())
	    send_to_char("This clan has more money than yours.\n\r",ch);
	else
	    send_to_char("Your clan has more money than this.\n\r",ch);
    }

    hour = clan->getPlayTime() / 3600;

    for (d = descriptor_list; d; d = d->next)
	if (IS_CLANNED(d->character) && d->character->pcdata->clan == clan)
	   hour += (current_time - d->character->logon) / 3600;

    if (hour > 8760)
	year = hour / 8760;

    snprintf(buf, sizeof(buf), "Member Online Time: %d years %d hours\n\r", year, hour);
    send_to_char(buf,ch);

    if (IS_IMMORTAL(ch))
    {
	send_to_char("Immortal Stuff:\n\r",ch);
	send_to_char("===============\n\r",ch);

	snprintf(buf, sizeof(buf), "Flags: %s\n\r", flag_string( clan_flags, clan->getFlags()));
	send_to_char(buf,ch);
    }
    return;
}
