#include <list>
#include <sys/time.h>
#include "merc.h"
#include "ClanReader.h"

/*
 * Read in actual clan data.
 */

#define KEYFN( literal, fn, value )					\
				if ( !str_cmp( word, literal ) )	\
				{					                \
				    fn(value);			            \
				    fMatch = TRUE;			        \
				    break;				            \
				}



using std::list;

ClanReader::ClanReader() { }

ClanReader::~ClanReader() { }

list<Clan *> ClanReader::loadClans() {
    list<Clan *> clans = list<Clan *>();
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

		try {
			Clan *clan = load_clan_file(filename);
			clans.push_back(clan);
		} catch (ClanNotFoundInFileException) {
			snprintf(buf, sizeof(buf), "Cannot load clan file: %s", filename );
			bug( buf, 0 );
		}
    }
    fclose( fpList );
    log_string(" Done clans " );
    fpReserve = fopen( NULL_FILE, "r" );
    return clans;
}

/*
 * Load a clan file
 */

Clan * ClanReader::load_clan_file( char *clanfile )
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

    if (!found) {
		throw ClanNotFoundInFileException();
    }

    return clan;
}


/*
 * Reads in PKill and PDeath still for backward compatibility but now it
 * should be written to PKillRange and PDeathRange for multiple level pkill
 * tracking support. --Shaddai
 * Added a hardcoded limit memlimit to the amount of members a clan can 
 * have set using setclan.  --Shaddai
 */

void ClanReader::fread_clan( Clan *clan, FILE *fp )
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
