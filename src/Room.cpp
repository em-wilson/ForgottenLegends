#include <list>
#include <vector>
#include "merc.h"
#include "ExtraDescription.h"
#include "Room.h"

using std::list;
using std::vector;

extern int			top_room;

void free_exit( EXIT_DATA *pExit );
void free_reset_data( RESET_DATA *pReset );

ROOM_INDEX_DATA::ROOM_INDEX_DATA()
{
    int door;

    top_room++;

    this->next             =   NULL;
    this->people           =   NULL;
    this->contents         =   list<Object *>();
    this->extra_descr      =   vector<ExtraDescription *>();
    this->area             =   NULL;

    for ( door=0; door < MAX_DIR; door++ )
        this->exit[door]   =   NULL;

    this->name             =   &str_empty[0];
    this->description      =   &str_empty[0];
    this->owner	    =	&str_empty[0];
    this->vnum             =   0;
    this->room_flags       =   0;
    this->light            =   0;
    this->sector_type      =   0;
    this->clan		    =	0;
    this->heal_rate	    =   100;
    this->mana_rate	    =   100;
}



ROOM_INDEX_DATA::~ROOM_INDEX_DATA()
{
    int door;
    ExtraDescription *pExtra;
    RESET_DATA *pReset;

    free_string( this->name );
    free_string( this->description );
    free_string( this->owner );

    for ( door = 0; door < MAX_DIR; door++ )
    {
        if ( this->exit[door] )
            free_exit( this->exit[door] );
    }

    for ( auto pExtra : this->extra_descr )
    {
        delete pExtra;
    }
    this->extra_descr.clear();

    for ( pReset = this->reset_first; pReset; pReset = pReset->next )
    {
        free_reset_data( pReset );
    }

    return;
}