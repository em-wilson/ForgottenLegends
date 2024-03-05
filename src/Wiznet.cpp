#include <sys/types.h>
#include "merc.h"
#include "recycle.h"
#include "ConnectedState.h"
#include "Wiznet.h"

Wiznet *Wiznet::instance()
{
	static Wiznet *_inst = NULL;
	if ( NULL == _inst ) {
		_inst = new Wiznet();
	}
	return _inst;
}

void Wiznet::report(std::string str, Character *ch, OBJ_DATA *obj,
	    long flag, long flag_skip, int min_level) 
{
    DESCRIPTOR_DATA *d;

    for ( d = descriptor_list; d != NULL; d = d->next )
    {
        if (d->connected == ConnectedState::Playing
		&&  IS_IMMORTAL(d->character) 
		&&  IS_SET(d->character->wiznet,WIZ_ON) 
		&&  (!flag || IS_SET(d->character->wiznet,flag))
		&&  (!flag_skip || !IS_SET(d->character->wiznet,flag_skip))
		&&  d->character->getTrust() >= min_level
		&&  d->character != ch)
	        {
			    if (IS_SET(d->character->wiznet,WIZ_PREFIX))
			  		send_to_char("--> ",d->character);
	            act_new(str.c_str(),d->character,obj,ch,TO_CHAR,POS_DEAD);
	        }
    }
 
    return;
}