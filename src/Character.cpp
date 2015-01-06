#include <string.h>
#include <sys/types.h>
#include "merc.h"
#include "Wiznet.h"

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



/*
 * Regeneration stuff.
 */
int Character::hit_gain( )
{
    int gain;
    int number;

    if (this->in_room == NULL)
    return 0;

    if ( IS_NPC(this) )
    {
    gain =  5 + this->level;
    if (IS_AFFECTED(this,AFF_REGENERATION))
        gain *= 2;

    switch(this->position)
    {
        default :       gain /= 2;          break;
        case POS_SLEEPING:  gain = 3 * gain/2;      break;
        case POS_RESTING:                   break;
        case POS_FIGHTING:  gain /= 3;          break;
    }

    
    }
    else
    {
    gain = UMAX(3,get_curr_stat(this,STAT_CON) - 3 + this->level/2); 
    gain += class_table[this->class_num].hp_max - 10;
    number = number_percent();
    if (number < get_skill(this,gsn_fast_healing))
    {
        gain += number * gain / 100;
        if (this->hit < this->max_hit)
        check_improve(this,gsn_fast_healing,TRUE,8);
    }

    switch ( this->position )
    {
        default:        gain /= 4;          break;
        case POS_SLEEPING:                  break;
        case POS_RESTING:   gain /= 2;          break;
        case POS_FIGHTING:  gain /= 6;          break;
    }

    if ( this->pcdata->condition[COND_HUNGER]   == 0 )
        gain /= 2;

    if ( this->pcdata->condition[COND_THIRST] == 0 )
        gain /= 2;

    }

    gain = gain * this->in_room->heal_rate / 100;
    
    if (this->on != NULL && this->on->item_type == ITEM_FURNITURE)
    gain = gain * this->on->value[3] / 100;

    if ( IS_AFFECTED(this, AFF_POISON) )
    gain /= 4;

    if (IS_AFFECTED(this, AFF_PLAGUE))
    gain /= 8;

    if (IS_AFFECTED(this,AFF_HASTE) || IS_AFFECTED(this,AFF_SLOW))
    gain /=2 ;

    if (!IS_NPC(this) && this->pcdata->clan)
    gain *= 1.5;

    return UMIN(gain, this->max_hit - this->hit);
}



int Character::mana_gain( )
{
    int gain;
    int number;

    if (this->in_room == NULL)
    return 0;

    if ( IS_NPC(this) )
    {
    gain = 5 + this->level;
    switch (this->position)
    {
        default:        gain /= 2;      break;
        case POS_SLEEPING:  gain = 3 * gain/2;  break;
        case POS_RESTING:               break;
        case POS_FIGHTING:  gain /= 3;      break;
        }
    }
    else
    {
    gain = (get_curr_stat(this,STAT_WIS) 
          + get_curr_stat(this,STAT_INT) + this->level) / 2;
    number = number_percent();
    if (number < get_skill(this,gsn_meditation))
    {
        gain += number * gain / 100;
        if (this->mana < this->max_mana)
            check_improve(this,gsn_meditation,TRUE,8);
    }
    if (!class_table[this->class_num].fMana)
        gain /= 2;

    switch ( this->position )
    {
        default:        gain /= 4;          break;
        case POS_SLEEPING:                  break;
        case POS_RESTING:   gain /= 2;          break;
        case POS_FIGHTING:  gain /= 6;          break;
    }

    if ( this->pcdata->condition[COND_HUNGER]   == 0 )
        gain /= 2;

    if ( this->pcdata->condition[COND_THIRST] == 0 )
        gain /= 2;

    }

    gain = gain * this->in_room->mana_rate / 100;

    if (this->on != NULL && this->on->item_type == ITEM_FURNITURE)
    gain = gain * this->on->value[4] / 100;

    if ( IS_AFFECTED( this, AFF_POISON ) )
    gain /= 4;

    if (IS_AFFECTED(this, AFF_PLAGUE))
        gain /= 8;

    if (IS_AFFECTED(this,AFF_HASTE) || IS_AFFECTED(this,AFF_SLOW))
        gain /=2 ;

    if (!IS_NPC(this) && this->pcdata->clan)
    gain *= 1.5;

    return UMIN(gain, this->max_mana - this->mana);
}



int Character::move_gain( )
{
    int gain;

    if (this->in_room == NULL)
    return 0;

    if ( IS_NPC(this) )
    {
    gain = this->level;
    }
    else
    {
    gain = UMAX( 15, this->level );

    switch ( this->position )
    {
    case POS_SLEEPING: gain += get_curr_stat(this,STAT_DEX);      break;
    case POS_RESTING:  gain += get_curr_stat(this,STAT_DEX) / 2;  break;
    }

    if ( this->pcdata->condition[COND_HUNGER]   == 0 )
        gain /= 2;

    if ( this->pcdata->condition[COND_THIRST] == 0 )
        gain /= 2;
    }

    gain = gain * this->in_room->heal_rate/100;

    if (this->on != NULL && this->on->item_type == ITEM_FURNITURE)
    gain = gain * this->on->value[3] / 100;

    if ( IS_AFFECTED(this, AFF_POISON) )
    gain /= 4;

    if (IS_AFFECTED(this, AFF_PLAGUE))
        gain /= 8;

    if (IS_AFFECTED(this,AFF_HASTE) || IS_AFFECTED(this,AFF_SLOW))
        gain /=2 ;

    if (!IS_NPC(this) && this->pcdata->clan)
    gain *= 1.5;

    return UMIN(gain, this->max_move - this->move);
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