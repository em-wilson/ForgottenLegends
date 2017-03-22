#include <string.h>
#include <sys/types.h>
#include <ForceFeedback/ForceFeedback.h>
#include "merc.h"

Character::Character()
{
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
    this->zone = NULL;
    this->pcdata = NULL;
    this->gen_data = NULL;
    this->valid = false;
    this->confirm_reclass = false;
    this->_name = std::string();
    this->_description = std::string();
    this->id = 0;
    this->version = 0;
    this->short_descr = &str_empty[0];
    this->long_descr = &str_empty[0];
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
    VALIDATE(this);
}

Character::~Character()
{
	OBJ_DATA *obj_next;
	AFFECT_DATA *paf_next;
    if ( this->short_descr ) free_string( this->short_descr );
    if ( this->long_descr ) free_string( this->long_descr );
    if ( this->prompt ) free_string( this->prompt );
    if ( this->prefix ) free_string( this->prefix );
    if ( this->material ) free_string( this->material );

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

void Character::gain_exp( int gain ) {
    return;
}

void Character::setName( const char * name )
{
    _name = name;
}

char * Character::getName()
{
    return _name.empty() ? NULL : (char*)_name.c_str();
}

void Character::setDescription( const char * description )
{
    _description = description;
}

char * Character::getDescription()
{
    return _description.empty() ? NULL : (char*)_description.c_str();
}

/*
 * Advancement stuff.
 */
void Character::advance_level( bool hide )
{
    char buf[MAX_STRING_LENGTH];
    int add_hp, add_mana, add_move, dracnum, add_prac;

    this->pcdata->last_level = 
    ( this->played + (int) (current_time - this->logon) ) / 3600;

/*
    sprintf( buf, "the %s",
    title_table [this->class_num] [this->level] [this->sex == SEX_FEMALE ? 1 : 0] );
    set_title( this, buf );
*/

    add_hp  = con_app[get_curr_stat(this,STAT_CON)].hitp + number_range(
            class_table[this->class_num].hp_min,
            class_table[this->class_num].hp_max );
    add_mana    = number_range(2,(2*get_curr_stat(this,STAT_INT)
                  + get_curr_stat(this,STAT_WIS))/5);
    if (!class_table[this->class_num].fMana)
    add_mana /= 2;
    add_move    = number_range( 1, (get_curr_stat(this,STAT_CON)
                  + get_curr_stat(this,STAT_DEX))/6 );
    add_prac    = wis_app[get_curr_stat(this,STAT_WIS)].practice;

    add_hp = add_hp * 9/10;
    add_mana = add_mana * 9/10;
    add_move = add_move * 9/10;

    add_hp  = UMAX(  2, add_hp   );
    add_mana    = UMAX(  2, add_mana );
    add_move    = UMAX(  6, add_move );

    this->max_hit     += add_hp;
    this->max_mana    += add_mana;
    this->max_move    += add_move;
    this->practice    += add_prac;
    this->train       += 1;

    this->pcdata->perm_hit    += add_hp;
    this->pcdata->perm_mana   += add_mana;
    this->pcdata->perm_move   += add_move;

    if (this->level == 10 && (this->race == race_lookup("draconian")))
    {
/*
    if (this->alignment > 500)
        dracnum = number_range(0, 4);
    else if (this->alignment < 500)
        dracnum = number_range(10, 14);
    else
        dracnum = number_range(5, 9);
*/
    dracnum = number_range(0,14);
    this->drac = dracnum;
    sprintf(buf, "You scream in agony as %s scales pierce your tender skin!\n\r", draconian_table[this->drac].colour);
    send_to_char(buf,this);
    if (this->perm_stat[STAT_STR] < 25)
        this->perm_stat[STAT_STR]++;
    else
        this->train++;

    if (this->perm_stat[draconian_table[this->drac].attr_prime] < 25)
        this->perm_stat[draconian_table[this->drac].attr_prime]++;
    else
        this->train++;
    }

    if (!hide)
    {
        sprintf(buf,
        "{BYou have leveled!{x\n\r"
        "You gain %d hit point%s, %d mana, %d move, and %d practice%s.\n\r",
        add_hp, add_hp == 1 ? "" : "s", add_mana, add_move,
        add_prac, add_prac == 1 ? "" : "s");
    send_to_char( buf, this );
    }
    return;
}



void Character::gain_condition( int iCond, int value )
{
    int condition;

    if ( value == 0 || IS_NPC(this) || this->level >= LEVEL_IMMORTAL)
    return;

    condition               = this->pcdata->condition[iCond];
    if (condition == -1)
        return;
    this->pcdata->condition[iCond]    = URANGE( 0, condition + value, 48 );

    if ( this->pcdata->condition[iCond] == 0 )
    {
    switch ( iCond )
    {
    case COND_HUNGER:
        send_to_char( "You are hungry.\n\r",  this );
        break;

    case COND_THIRST:
        send_to_char( "You are thirsty.\n\r", this );
        break;

    case COND_DRUNK:
        if ( condition != 0 )
        send_to_char( "You are sober.\n\r", this );
        break;
    }
    }

    return;
}

int Character::getRange() {
    return 0;
}

bool Character::didJustDie() {
    return false;
}

void Character::update() {
    AFFECT_DATA *paf;
    AFFECT_DATA *paf_next;

    if ( this->desc && this->desc->connected == CON_PLAYING )
        send_to_char("\n\r",this);

    if ( this->position >= POS_STUNNED )
    {
        if ( this->hit  < this->max_hit )
            this->hit  += this->hit_gain();
        else
            this->hit = this->max_hit;

        if ( this->mana < this->max_mana )
            this->mana += this->mana_gain();
        else
            this->mana = this->max_mana;

        if ( this->move < this->max_move )
            this->move += this->move_gain();
        else
            this->move = this->max_move;
    }

    if ( this->position == POS_STUNNED )
        update_pos( this );

    for ( paf = this->affected; paf != NULL; paf = paf_next )
    {
        paf_next	= paf->next;
        if ( paf->duration > 0 )
        {
            paf->duration--;
            if (number_range(0,4) == 0 && paf->level > 0)
                paf->level--;  /* spell strength fades with time */
        }
        else if ( paf->duration < 0 )
            ;
        else
        {
            if ( paf_next == NULL
                    ||   paf_next->type != paf->type
                    ||   paf_next->duration > 0 )
            {
                if ( paf->type > 0 && skill_table[paf->type].msg_off )
                {
                    send_to_char( skill_table[paf->type].msg_off, this );
                    send_to_char( "\n\r", this );
                }
            }

            affect_remove( this, paf );
        }
    }

    /*
     * Careful with the damages here,
     *   MUST NOT refer to ch after damage taken,
     *   as it may be lethal damage (on NPC).
     */

    if (is_affected(this, gsn_plague) )
    {
        if (this->in_room == NULL)
            return;

        ::act("$n writhes in agony as plague sores erupt from $s skin.",
                this,NULL,NULL,TO_ROOM);
        send_to_char("You writhe in agony from the plague.\n\r",this);

        AFFECT_DATA *af;
        for ( af = this->affected; af != NULL; af = af->next )
        {
            if (af->type == gsn_plague)
                break;
        }

        if (af == NULL)
        {
            REMOVE_BIT(this->affected_by,AFF_PLAGUE);
            return;
        }

        if (af->level == 1)
            return;

        AFFECT_DATA plague;
        plague.where		= TO_AFFECTS;
        plague.type 		= gsn_plague;
        plague.level 		= af->level - 1;
        plague.duration 	= number_range(1,2 * plague.level);
        plague.location		= APPLY_STR;
        plague.modifier 	= -5;
        plague.bitvector 	= AFF_PLAGUE;

        for ( Character* vch = this->in_room->people; vch != NULL; vch = vch->next_in_room)
        {
            if (!saves_spell(plague.level - 2,vch,DAM_DISEASE)
                    &&  !IS_IMMORTAL(vch)
                    &&  !IS_AFFECTED(vch,AFF_PLAGUE) && number_bits(4) == 0)
            {
                send_to_char("You feel hot and feverish.\n\r",vch);
                ::act("$n shivers and looks very ill.",vch,NULL,NULL,TO_ROOM);
                affect_join(vch,&plague);
            }
        }

        int dam = UMIN(this->level,af->level/5+1);
        this->mana -= dam;
        this->move -= dam;
        damage_old( this, this, dam, gsn_plague,DAM_DISEASE,FALSE);
    }
    else if ( IS_AFFECTED(this, AFF_POISON)
            &&   !IS_AFFECTED(this,AFF_SLOW))

    {
        AFFECT_DATA *poison;

        poison = affect_find(this->affected,gsn_poison);

        if (poison != NULL)
        {
            ::act( "$n shivers and suffers.", this, NULL, NULL, TO_ROOM );
            send_to_char( "You shiver and suffer.\n\r", this );
            damage_old(this,this,poison->level/10 + 1,gsn_poison,
                    DAM_POISON,FALSE);
        }
    }

    else if ( this->position == POS_INCAP && number_range(0,1) == 0)
    {
        ::damage( this, this, 1, TYPE_UNDEFINED, DAM_NONE,FALSE);
    }
    else if ( this->position == POS_MORTAL )
    {
        ::damage( this, this, 1, TYPE_UNDEFINED, DAM_NONE,FALSE);
    }
}
