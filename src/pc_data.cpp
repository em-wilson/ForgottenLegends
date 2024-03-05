#include <list>
#include <string>
#include <sys/types.h>
#include "merc.h"
#include "board.h"
#include "recycle.h"
#include "pc_data.h"

using std::list;
using std::string;

list<PC_DATA*> PC_DATA::active;

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
    	this->last_note_read.emplace(board, 0);
    }

    for ( int group = 0; group < MAX_GROUP; group++ ) {
    	this->group_known[group] = false;
    }

    VALIDATE(this);

    PC_DATA::active.push_back(this);
}
	

PC_DATA::~PC_DATA()
{
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

string PC_DATA::getPassword() { return _pwd; }
void PC_DATA::setPassword(string value) { _pwd = value; }
