#include <sys/types.h>
#include "merc.h"
#include "recycle.h"
#include "pc_data.h"

std::list<PC_DATA*> PC_DATA::active;

PC_DATA::PC_DATA()
{
    for (int alias = 0; alias < MAX_ALIAS; alias++)
    {
		this->alias[alias] = NULL;
		this->alias_sub[alias] = NULL;
    }

    for ( int i = 0; i < 4; i++ ) {
    	this->condition[i] = 0;
    }

    for ( int skill = 0; skill < MAX_SKILL; skill++ ) {
    	this->learned[skill] = 0;
    	this->sk_level[skill] = 0;
    	this->sk_rating[skill] = 0;
    }

    this->next = NULL;
    this->clan = NULL;
    this->buffer = new_buf();
    this->pwd = NULL;
    this->bamfin = NULL;
    this->bamfout = NULL;
    this->title = NULL;
    this->perm_hit = 0;
    this->perm_mana = 0;
    this->perm_move = 0;
    this->true_sex = 0;
    this->last_level = 0;
    this->points = 0;
    this->confirm_delete = false;
    this->security = 0;
    this->board = NULL;
    this->in_progress = NULL;
    for ( int board = 0; board < MAX_BOARD; board++ ) {
    	this->last_note[board] = 0;
    }

    for ( int group = 0; group < MAX_GROUP; group++ ) {
    	this->group_known[group] = false;
    }

    VALIDATE(this);

    PC_DATA::active.push_back(this);
}
	

PC_DATA::~PC_DATA()
{
    if ( this->pwd ) free_string(this->pwd);
    if ( this->bamfin ) free_string(this->bamfin);
    if ( this->bamfout ) free_string(this->bamfout);
    if ( this->title ) free_string(this->title);
    if ( this->buffer ) free_buf(this->buffer);
    
    for (int alias = 0; alias < MAX_ALIAS; alias++)
    {
    	if ( this->alias[alias] ) {
			free_string(this->alias[alias]);
    	}

    	if ( this->alias_sub[alias] ) {
    		free_string(this->alias_sub[alias]);
    	}
    }
    INVALIDATE(this);
    PC_DATA::active.remove(this);
}
