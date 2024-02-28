#include <time.h>
#include "merc.h"
#include "ClanManager.h"

extern  CLAN_DATA * first_clan;

/*
 * Get pointer to clan structure from clan name.
 */
CLAN_DATA * ClanManager::get_clan( char *name )
{
    CLAN_DATA *clan;
    
    for ( clan = first_clan; clan; clan = clan->next )
       if ( !str_prefix( name, clan->name ) )
         return clan;

    return NULL;
}
