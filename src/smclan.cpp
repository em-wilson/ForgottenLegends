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
#include "PlayerCharacter.h"

#define MAX_NEST	100
//static	OBJ_DATA *	rgObjNest	[MAX_NEST];
void show_flag_cmds( Character *ch, const struct flag_type *flag_table );
CLAN_DATA * first_clan;
CLAN_DATA * last_clan;

/* local routines */
void	fread_clan	args( ( CLAN_DATA *clan, FILE *fp ) );
bool	load_clan_file	args( ( char *clanfile ) );
void	write_clan_list	args( ( void ) );
int flag_value                  args ( ( const struct flag_type *flag_table,
                                         char *argument) );
char *flag_string               args ( ( const struct flag_type *flag_table,
                                         int bits ) );

/*
 * Get pointer to clan structure from clan name.
 */
CLAN_DATA *get_clan( char *name )
{
    CLAN_DATA *clan;
    
    for ( clan = first_clan; clan; clan = clan->next )
       if ( !str_prefix( name, clan->name ) )
         return clan;

    return NULL;
}

void write_clan_list( )
{
    CLAN_DATA *tclan;
    FILE *fpout;
    char filename[256];

    snprintf(filename, sizeof(filename), "%s%s", CLAN_DIR, CLAN_LIST );
    fpout = fopen( filename, "w" );
    if ( !fpout )
    {
	bug( "FATAL: cannot open clan.lst for writing!\n\r", 0 );
 	return;
    }	  
    for ( tclan = first_clan; tclan; tclan = tclan->next )
	fprintf( fpout, "%s\n", tclan->filename );
    fprintf( fpout, "$\n" );
    fclose( fpout );
}

/*
 * Save a clan's data to its data file
 */
void save_clan( CLAN_DATA *clan )
{
    FILE *fp;
    char filename[256];
    char buf[MAX_STRING_LENGTH];

    if ( !clan )
    {
	bug( "save_clan: null clan pointer!", 0 );
	return;
    }
        
    if ( !clan->filename || clan->filename[0] == '\0' )
    {
	snprintf(buf, sizeof(buf), "save_clan: %s has no filename", clan->name );
	bug( buf, 0 );
	return;
    }
    
    snprintf(filename, sizeof(filename), "%s%s", CLAN_DIR, clan->filename );
    
    fclose( fpReserve );
    if ( ( fp = fopen( filename, "w" ) ) == NULL )
    {
    	bug( "save_clan: fopen", 0 );
    	perror( filename );
    }
    else
    {
	fprintf( fp, "#CLAN\n" );
	fprintf( fp, "Name         %s~\n",	clan->name		);
	fprintf( fp, "Whoname      %s~\n",	clan->whoname		);
	fprintf( fp, "Filename     %s~\n",	clan->filename		);
	fprintf( fp, "Motto        %s~\n",	clan->motto		);
	fprintf( fp, "Leader       %s~\n",	clan->leader		);
	fprintf( fp, "NumberOne    %s~\n",	clan->number1		);
	fprintf( fp, "NumberTwo    %s~\n",	clan->number2		);
	fprintf( fp, "Flags        %s\n",	print_flags(clan->flags));
	fprintf( fp, "PKills       %d\n",	clan->pkills		);
	fprintf( fp, "PDeaths      %d\n",	clan->pdeaths		);
	fprintf( fp, "MKills       %d\n",	clan->mkills		);
	fprintf( fp, "MDeaths      %d\n",	clan->mdeaths		);
	fprintf( fp, "Played       %d\n",	clan->played		);
	fprintf( fp, "Score        %d\n",	clan->score		);
	fprintf( fp, "Strikes      %d\n",	clan->strikes		);
	fprintf( fp, "Members      %d\n",	clan->members		);
	fprintf( fp, "Death        %d\n",	clan->death		);
	fprintf( fp, "Money        %ld\n",	clan->money		);
	fprintf( fp, "End\n\n"						);
	fprintf( fp, "#END\n"						);
    }
    fclose( fp );
    fpReserve = fopen( NULL_FILE, "r" );
    return;
}


/*
 * Read in actual clan data.
 */

#if defined(KEY)
#undef KEY
#endif

#define KEY( literal, field, value )					\
				if ( !str_cmp( word, literal ) )	\
				{					\
				    field  = value;			\
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

void fread_clan( CLAN_DATA *clan, FILE *fp )
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

	case 'D':
	    KEY( "Death",	clan->death,		fread_number( fp ) );
	    break;

	case 'E':
	    if ( !str_cmp( word, "End" ) )
	    {
		if (!clan->name)
		  clan->name		= str_dup( "" );
		if (!clan->leader)
		  clan->leader		= str_dup( "" );
		if (!clan->description)
		  clan->description 	= str_dup( "" );
		if (!clan->motto)
		  clan->motto		= str_dup( "" );
		if (!clan->number1)
		  clan->number1		= str_dup( "" );
		if (!clan->number2)
		  clan->number2		= str_dup( "" );
		if (!clan->money)
		  clan->money		= 0;
		if (!clan->played)
		  clan->played		= 0;
		return;
	    }
	    break;
	    
	case 'F':
	    KEY( "Filename",	clan->filename,		fread_string_nohash( fp ) );
	    KEY( "Flags",	clan->flags,		fread_flag( fp ) );

	case 'L':
	    KEY( "Leader",	clan->leader,		fread_string( fp ) );
	    break;

	case 'M':
	    KEY( "MDeaths",	clan->mdeaths,		fread_number( fp ) );
	    KEY( "Members",	clan->members,		fread_number( fp ) );
	    KEY( "MKills",	clan->mkills,		fread_number( fp ) );
	    KEY( "Motto",	clan->motto,		fread_string( fp ) );
	    KEY( "Money",	clan->money,		fread_number( fp ) );
	    break;
 
	case 'N':
	    KEY( "Name",	clan->name,		fread_string( fp ) );
	    KEY( "NumberOne",	clan->number1,		fread_string( fp ) );
	    KEY( "NumberTwo",	clan->number2,		fread_string( fp ) );
	    break;

	case 'P':
	    KEY( "PDeaths",	clan->pdeaths,		fread_number( fp ) );
	    KEY( "PKills",	clan->pkills,		fread_number( fp ) );
	    KEY( "Played",	clan->played,		fread_number( fp ) );
	    break;

	case 'S':
	    KEY( "Score",	clan->score,		fread_number( fp ) );
	    KEY( "Strikes",	clan->strikes,		fread_number( fp ) );
	    break;

	case 'W':
	    KEY( "Whoname",	clan->whoname,		fread_string( fp ) );
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
    CLAN_DATA *clan;
    FILE *fp;
    bool found;

    CREATE( clan, CLAN_DATA, 1 );

    /* Make sure we default these to 0 --Shaddai */
    clan->pdeaths = 0;
    clan->pkills = 0;

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

    if ( found )
	LINK( clan, first_clan, last_clan, next, prev );
    else
      DISPOSE( clan );

    return found;
}

/*
 * This allows imms to delete clans
 * -Blizzard
 */
void do_delclan( Character *ch, char *argument )
{
    CLAN_DATA *clan;
    char strsave[MAX_STRING_LENGTH];

    if ((clan = get_clan(argument)) == NULL)
    {
	send_to_char("That clan does not exist.\n\r",ch);
	return;
    }

    UNLINK( clan, first_clan, last_clan, next, prev );
    snprintf(strsave, sizeof(strsave), "%s%s%s", DATA_DIR, CLAN_DIR, clan->filename );
    DISPOSE( clan );
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
    
    
    first_clan	= NULL;
    last_clan	= NULL;

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
    CLAN_DATA *clan;

    if ( IS_NPC( ch ) || !ch->pcdata->clan )
    {
	send_to_char( "Huh?\n\r", ch );
	return;
    }

    clan = ch->pcdata->clan;
    
    if ( !str_cmp( ch->getName(), clan->leader  )
    ||   !str_cmp( ch->getName(), clan->number1 )
    ||   !str_cmp( ch->getName(), clan->number2 ) )
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

    if ( victim->pcdata->clan )
    {
		if (victim->pcdata->clan == ch->pcdata->clan)
		{
			send_to_char("They are already in your clan!\n\r",ch);
			return;
		}
		if (!((PlayerCharacter *) victim)->wantsToJoinClan(ch->pcdata->clan))
		{
			send_to_char("They do not wish to join your clan.\n\r",ch);
			return;
		}
	--victim->pcdata->clan->members;
	save_clan( victim->pcdata->clan );
    }
    clan->members++;

    victim->pcdata->clan = clan;
    act( "You induct $N into $t", ch, clan->name, victim, TO_CHAR );
    act( "$n inducts you into $t", ch, clan->name, victim, TO_VICT );
    save_char_obj( victim );
    save_clan( clan );
    return;
}

void do_outcast( Character *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    Character *victim;
    CLAN_DATA *clan;

    if ( IS_NPC( ch ) || !ch->pcdata->clan )
    {
	send_to_char( "Huh?\n\r", ch );
	return;
    }

    clan = ch->pcdata->clan;

    if ( !str_cmp( ch->getName(), clan->leader  )
    ||   !str_cmp( ch->getName(), clan->number1 )
    ||   !str_cmp( ch->getName(), clan->number2 ) )
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
    if ( !str_cmp(victim->getName(), ch->pcdata->clan->leader ) )
    {
	send_to_char("You cannot loner your own leader!\n\r",ch);
	return;
    }
    --clan->members;
    if ( !str_cmp( victim->getName(), ch->pcdata->clan->number1 ) )
    {
	free_string( ch->pcdata->clan->number1 );
	ch->pcdata->clan->number1 = str_dup( "" );
    }
    if ( !str_cmp( victim->getName(), ch->pcdata->clan->number2 ) )
    {
	free_string( ch->pcdata->clan->number2 );
	ch->pcdata->clan->number2 = str_dup( "" );
    }
    victim->pcdata->clan = get_clan((char*)"outcast");
    act( "You outcast $N from $t", ch, clan->name, victim, TO_CHAR );
    act( "$n outcasts you from $t", ch, clan->name, victim, TO_VICT );
    save_char_obj( victim );	/* clan gets saved when pfile is saved */
    save_clan( clan );
    victim->pcdata->clan->members++;
    save_clan( victim->pcdata->clan );
    return;
}

void do_setclan( Character *ch, char *argument )
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    CLAN_DATA *clan;

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
	send_to_char( " strikes pkill pdeath money\n\r", ch );
	return;
    }

    clan = get_clan( arg1 );
    if ( !clan )
    {
	send_to_char( "No such clan.\n\r", ch );
	return;
    }
    if ( !str_prefix( arg2, "leader" ) )
    {
	free_string( clan->leader );
	clan->leader = str_dup( argument );
	send_to_char( "Done.\n\r", ch );
	save_clan( clan );
	return;
    }
    if ( !str_prefix( arg2, "recruiter1" ) )
    {
	free_string( clan->number1 );
	clan->number1 = str_dup( argument );
	send_to_char( "Done.\n\r", ch );
	save_clan( clan );
	return;
    }
    if ( !str_prefix( arg2, "recruiter2" ) )
    {
	free_string( clan->number2 );
	clan->number2 = str_dup( argument );
	send_to_char( "Done.\n\r", ch );
	save_clan( clan );
	return;
    }
    if ( !str_prefix( arg2, "members" ) )
    {
	clan->members = atoi( argument );
	send_to_char( "Done.\n\r", ch );
	save_clan( clan );
	return;
    }
    if ( !str_prefix( arg2, "deathroom" ) )
    {
	clan->death = atoi( argument );
	send_to_char( "Death room set.\n\r", ch);
	save_clan( clan );
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
	    TOGGLE_BIT(clan->flags, value);
	    send_to_char("Clan flags toggled.\n\r",ch);
	    save_clan(clan);
	    return;
	}

	send_to_char("That flag does not exist.\n\rList of flags:\n\r", ch);
	do_setclan(ch,(char*)"flags");
	return;
    }
    if ( !str_prefix( arg2, "whoname" ) )
    {
	free_string( clan->whoname );
	clan->whoname = str_dup( argument );
	send_to_char("Whoname set.\n\r",ch);
	save_clan( clan );
	return;
    }
    if ( !str_prefix( arg2, "name" ) )
    {
	free_string( clan->name );
	clan->name = str_dup( argument );
	send_to_char( "Done.\n\r", ch );
	save_clan( clan );
	return;
    }
    if ( !str_prefix( arg2, "filename" ) )
    {
	free_string( clan->filename );
	clan->filename = str_dup( argument );
	send_to_char( "Done.\n\r", ch );
	save_clan( clan );
	write_clan_list( );
	return;
    }
    if ( !str_prefix( arg2, "motto" ) )
    {
	free_string( clan->motto );
	clan->motto = str_dup( argument );
	send_to_char( "Done.\n\r", ch );
	save_clan( clan );
	return;
    }
    if ( !str_prefix( "played", arg2) )
    {
	clan->played = atoi( argument );
	send_to_char ("Ok.\n\r", ch );
	save_clan( clan );
	return;
    }
    if ( !str_prefix( "pkill", arg2) )
    {
	clan->pkills = atoi( argument );
	send_to_char ("Ok.\n\r", ch );
	save_clan( clan );
	return;
    }
    if ( !str_prefix( "pdeath", arg2) )
    {
	clan->pdeaths = atoi( argument );
	send_to_char ("Ok.\n\r", ch );
	save_clan( clan );
	return;
    }
    if ( !str_prefix( "money", arg2) )
    {
	clan->money = atoi( argument );
	send_to_char( "Money set.\n\r",ch);
	save_clan( clan );
	return;
    }
    do_setclan( ch, (char*)"" );
    return;
}

void do_makeclan( Character *ch, char *argument )
{
    CLAN_DATA *clan;

	if (get_trust(ch) < MAX_LEVEL ) {
		send_to_char("Only implementors can run this command.", ch);
		return;
	}

    if ( !argument || argument[0] == '\0' )
    {
	send_to_char( "Usage: makeclan <clan name>\n\r", ch );
	return;
    }

    argument[0] = LOWER(argument[0]);

    CREATE( clan, CLAN_DATA, 1 );
    LINK( clan, first_clan, last_clan, next, prev );

    clan->name		= str_dup( argument );
    clan->motto		= str_dup( "" );
    clan->description	= str_dup( "" );
    clan->leader	= str_dup( "" );
    clan->number1	= str_dup( "" );
    clan->number2	= str_dup( "" );
    return;
}

/*
 * Added multiple level pkill and pdeath support. --Shaddai
 */

void do_clist( Character *ch, char *argument )
{
    CLAN_DATA *clan;
    DESCRIPTOR_DATA *d;
    int count = 0, year = 0, hour = 0;
    char buf[MAX_STRING_LENGTH];

    if ( argument[0] == '\0' )
    {
        send_to_char( "\n\rClan          Leader       Pkills       Times Pkilled\n\r_________________________________________________________________________\n\r", ch );
        send_to_char( "PK Clans\n\r",ch);
        for ( clan = first_clan; clan; clan = clan->next )
        {
	    if (IS_SET(clan->flags, CLAN_NOSHOW) || !IS_SET(clan->flags, CLAN_PK))
		continue;
            snprintf(buf, sizeof(buf), "%-13s %-13s", clan->name, clan->leader );
	    send_to_char(buf,ch);
            snprintf(buf, sizeof(buf), "%-13d%-13d\n\r", 
		clan->pkills, clan->pdeaths );
	    send_to_char(buf,ch);
            count++;
        }

        send_to_char( "\n\rNon-PK Clans\n\r",ch);
        for ( clan = first_clan; clan; clan = clan->next )
        {
	    if (IS_SET(clan->flags, CLAN_PK) || IS_SET(clan->flags, CLAN_NOSHOW))
		continue;
            snprintf(buf, sizeof(buf), "%-13s %-13s", clan->name, clan->leader );
	    send_to_char(buf,ch);
            snprintf(buf, sizeof(buf), "%-13d%-13d\n\r", 
		clan->pkills, clan->pdeaths );
	    send_to_char(buf,ch);
            count++;
        }

	if (IS_IMMORTAL(ch))
	{
        send_to_char( "\n\rClans Players Can't See:\n\r",ch);
        for ( clan = first_clan; clan; clan = clan->next )
        {
	    if (!IS_SET(clan->flags, CLAN_NOSHOW))
		continue;
            snprintf(buf, sizeof(buf), "%-13s %-13s", clan->name, clan->leader );
	    send_to_char(buf,ch);
            snprintf(buf, sizeof(buf), "%-13d%-13d\n\r", 
		clan->pkills, clan->pdeaths );
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

    clan = get_clan( argument );
    if (!clan || (IS_SET(clan->flags, CLAN_NOSHOW) && !IS_IMMORTAL(ch)))
    {
        send_to_char( "No such clan.\n\r", ch );
        return;
    }

    snprintf(buf, sizeof(buf), "\n\r%s, %s\n\r'%s'\n\r\n\r", clan->name, clan->whoname, clan->motto );
    send_to_char(buf,ch);
    snprintf(buf, sizeof(buf), "Pkills: %d\n\r", clan->pkills);
    send_to_char(buf,ch);
    snprintf(buf, sizeof(buf), "Pdeaths: %d\n\r", clan->pdeaths);
    send_to_char(buf,ch);
    snprintf(buf, sizeof(buf), "Leader:    %s\n\rRecruiter:  %s\n\rRecruiter: %s\n\rMembers    :  %d\n\r",
                        clan->leader,
                        clan->number1,
                        clan->number2,
			clan->members );
    send_to_char(buf,ch);

    if (IS_CLANNED(ch) || IS_IMMORTAL(ch))
    {
	if (IS_IMMORTAL(ch) || ch->pcdata->clan == clan)
	{
	    snprintf(buf, sizeof(buf), "Funds:    %ld (gold)\n\r", clan->money / 100);
	    send_to_char(buf,ch);
	}
	else if (clan->money > ch->pcdata->clan->money)
	    send_to_char("This clan has more money than yours.\n\r",ch);
	else
	    send_to_char("Your clan has more money than this.\n\r",ch);
    }

    hour = clan->played / 3600;

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

	snprintf(buf, sizeof(buf), "Flags: %s\n\r", flag_string( clan_flags, clan->flags));
	send_to_char(buf,ch);
    }
    return;
}
