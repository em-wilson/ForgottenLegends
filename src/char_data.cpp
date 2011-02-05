#include <sys/types.h>
#include "merc.h"
#include "recycle.h"
#include "char_data.h"

CHAR_DATA::CHAR_DATA()
{
    this->next = NULL;
    this->next_in_room = NULL;
    this->master = NULL;
    this->leader = NULL;
    this->fighting = NULL;
    this->reply = NULL;
    this->pet = NULL;
    this->mprog_target = NULL;
    this->hunting = NULL;
    this->memory = NULL;
    this->spec_fun = NULL;
    this->pIndexData = NULL;
    this->desc = NULL;
    this->affected = NULL;
    this->carrying = NULL;
    this->on = NULL;
    this->in_room = NULL;
    this->was_in_room = NULL;
    this->was_note_room = NULL;
    this->zone = NULL;
    this->pcdata = NULL;
    this->gen_data = NULL;
    this->valid = false;
    this->confirm_reclass = false;
    this->name = &str_empty[0];
    this->id = 0;
    this->version = 0;
    this->short_descr = &str_empty[0];
    this->long_descr = &str_empty[0];
    this->description = &str_empty[0];
    this->prompt = &str_empty[0];
    this->prefix = &str_empty[0];
    this->group = 0;
    this->sex = 0;
    this->class_num = 0;
    this->race = 0;
    this->level = 1;
    this->trust = 0;
    this->drac = 0;
    this->breathe = 0;
    this->played = 0;
    this->auction = NULL;
    this->lines = PAGELEN;
    this->logon = current_time;
    this->timer = 0;
    this->wait = 0;
    this->daze = 0;
    this->hit = 50;
    this->max_hit = 50;
    this->mana = 100;
    this->max_mana = 100;
    this->move = 100;
    this->max_move = 100;
    this->gold = 0;
    this->silver = 0;
    this->exp = 0;
    this->act = 0;
    this->comm = 0;
    this->done = 0;
    this->reclass_num = 0;
    this->wiznet = 0;
    this->imm_flags = 0;
    this->res_flags = 0;
    this->vuln_flags = 0;
    this->invis_level = 0;
    this->incog_level = 0;
    this->affected_by = 0;
    this->position = POS_STANDING;
    this->practice = 0;
    this->train = 0;
    this->carry_weight = 0;
    this->carry_number = 0;
    this->saving_throw = 0;
    this->alignment = 0;
    this->hitroll = 0;
    this->damroll = 0;
    for ( int armor = 0; armor < 4; armor++ ) {
    	this->armor[armor] = 100;
    }
    this->wimpy = 0;
    for ( int stat = 0; stat < MAX_STATS; stat++ ) {
    	this->perm_stat[stat] = 13;
    	this->mod_stat[stat] = 0;
    }
    this->form = 0;
    this->parts = 0;
    this->size = 0;
    this->material = NULL;
    this->off_flags = NULL;
    for ( int damage = 0; damage < 3; damage++ ) {
    	this->damage[damage] = 0;
    }
    this->dam_type = 0;
    this->start_pos = 0;
    this->default_pos = 0;
    this->mprog_delay = 0;
    this->morph_form = 0;
    this->orig_form = 0;
    this->xp = 0;
    this->confirm_pk = false;
    this->makeclan = false;;
    this->join = NULL;
    this->pkills = 0;
    this->pkilled = 0;
    this->mkills = 0;
    this->mkilled = 0;
    this->range = 0;
    this->clan_cust = 0;
    this->adrenaline = 0;
    this->jkilled = 0;
    VALIDATE(this);
}

CHAR_DATA::~CHAR_DATA()
{
	OBJ_DATA *obj_next;
	AFFECT_DATA *paf_next;
    if ( this->name ) free_string( this->name );
    if ( this->short_descr ) free_string( this->short_descr );
    if ( this->long_descr ) free_string( this->long_descr );
    if ( this->description ) free_string( this->description );
    if ( this->prompt ) free_string( this->prompt );
    if ( this->prefix ) free_string( this->prefix );
    if ( this->material ) free_string( this->material );

    if (IS_NPC(this)) {
		mobile_count--;
    }

    for (OBJ_DATA *obj = this->carrying; obj != NULL; obj = obj_next)
    {
		obj_next = obj->next_content;
		extract_obj(obj);
    }

    for (AFFECT_DATA *paf = this->affected; paf != NULL; paf = paf_next)
    {
		paf_next = paf->next;
		affect_remove(this,paf);
    }

    if (this->pcdata != NULL) {
        delete this->pcdata;
        this->pcdata = NULL;
    }

    INVALIDATE(this);
}