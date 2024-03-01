#include <string>
#include "../merc.h" // For fpReserve, NULL_FILE, and print_flags
#include "ClanWriter.h"

extern void bug( const char *str, int param );

using std::string;
using std::vector;

ClanWriter::ClanWriter(const string clanDir, const string clanListFile) {
    _clanDir = clanDir;
    _clanListFile = clanListFile;
}

void ClanWriter::write_clan_list(vector<Clan *> clans) {
    FILE *fpout;
    char filename[256];

    snprintf(filename, sizeof(filename), "%s%s", _clanDir.c_str(), _clanListFile.c_str() );
    fpout = fopen( filename, "w" );
    if ( !fpout )
    {
        bug( "FATAL: cannot open clan.lst for writing!\n\r", 0 );
        return;
    }	  

    for (auto clan : clans)
    {
    	fprintf( fpout, "%s\n", clan->getFilename().c_str() );
    }
    fprintf( fpout, "$\n" );
    fclose( fpout );

}

/*
 * Save a clan's data to its data file
 */
void ClanWriter::save_clan( Clan *clan )
{
    FILE *fp;
    char filename[256];
    char buf[4608];

    if ( !clan )
    {
        bug( "save_clan: null clan pointer!", 0 );
        return;
    }
        
    if ( clan->getFilename().empty() )
    {
        snprintf(buf, sizeof(buf), "save_clan: %s has no filename", clan->getName().c_str() );
        bug( buf, 0 );
        return;
    }
    
    snprintf(filename, sizeof(filename), "%s%s", _clanDir.c_str(), clan->getFilename().c_str() );
    
    fclose( fpReserve );
    if ( ( fp = fopen( filename, "w" ) ) == NULL )
    {
    	bug( "save_clan: fopen", 0 );
    	perror( filename );
    }
    else
    {
	fprintf( fp, "#CLAN\n" );
	fprintf( fp, "Name         %s~\n",	clan->getName().c_str()				);
	fprintf( fp, "Whoname      %s~\n",	clan->getWhoname().c_str()			);
	fprintf( fp, "Filename     %s~\n",	clan->getFilename().c_str()			);
	fprintf( fp, "Motto        %s~\n",	clan->getMotto().c_str()			);
	fprintf( fp, "Leader       %s~\n",	clan->getLeader().c_str()			);
	fprintf( fp, "NumberOne    %s~\n",	clan->getFirstOfficer().c_str()		);
	fprintf( fp, "NumberTwo    %s~\n",	clan->getSecondOfficer().c_str()	);
	fprintf( fp, "Flags        %s\n",	print_flags(clan->getFlags())		);
	fprintf( fp, "PKills       %d\n",	clan->getPkills()					);
	fprintf( fp, "PDeaths      %d\n",	clan->getPdeaths()					);
	fprintf( fp, "MKills       %d\n",	clan->getMkills()					);
	fprintf( fp, "MDeaths      %d\n",	clan->getMdeaths()					);
	fprintf( fp, "Played       %d\n",	clan->getPlayTime()					);
	fprintf( fp, "Members      %d\n",	clan->countMembers()				);
	fprintf( fp, "Death        %d\n",	clan->getDeathRoomVnum()			);
	fprintf( fp, "Money        %ld\n",	clan->getMoney()					);
	fprintf( fp, "End\n\n"													);
	fprintf( fp, "#END\n"													);
    }
    fclose( fp );
    fpReserve = fopen( NULL_FILE, "r" );
    return;
}