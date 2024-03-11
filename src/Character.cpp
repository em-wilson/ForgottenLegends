#include <string.h>
#include <sys/types.h>
#include "merc.h"
#include "ConnectedState.h"
#include "Portal.h"
#include "Room.h"

using std::vector;

/** global functions */
AFFECT_DATA *new_affect(void);
void free_affect(AFFECT_DATA *af);

Character::Character()
{
    this->next_in_room = NULL;
    this->next = NULL;
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
    this->_on = nullptr;
    this->in_room = NULL;
    this->was_in_room = NULL;
    this->zone = NULL;
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
    this->group = 0;
    this->sex = 0;
    this->class_num = 0;
    this->_race = nullptr;
    this->level = 0;
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
	AFFECT_DATA *paf_next;
    if ( this->short_descr ) free_string( this->short_descr );
    if ( this->long_descr ) free_string( this->long_descr );
    if ( this->prompt ) free_string( this->prompt );
    if ( this->material ) free_string( this->material );

    for (auto *obj : this->getCarrying())
    {
		extract_obj(obj);
        _carrying.remove(obj);
    }

    for (auto paf : this->affected)
    {
		paf_next = paf->next;
        this->removeAffect(paf);
    }

    if ( this->desc ) {
        this->desc = NULL;
    }

    char_list.remove(this);

    INVALIDATE(this);
}

void Character::gain_exp( int gain ) {
    return;
}

vector<Object *> Character::getCarrying() { return std::vector<Object *>(_carrying.begin(), _carrying.end()); }

void Character::setName( string name )
{
    _name = name;
}

string Character::getName() { return _name; }

void Character::setDescription( const char * description )
{
    _description = description;
}

char * Character::getDescription()
{
    return _description.empty() ? NULL : (char*)_description.c_str();
}

/*
 * True if char can see victim.
 */
bool Character::can_see( Character *victim )
{
/* RT changed so that WIZ_INVIS has levels */
    if ( this == victim )
	return TRUE;
    
    if ( victim == NULL || this->getTrust() < victim->invis_level)
	return FALSE;


    if (this->getTrust() < victim->incog_level && this->in_room != victim->in_room)
	return FALSE;

    if ( (!IS_NPC(this) && IS_SET(this->act, PLR_HOLYLIGHT)) 
    ||   (IS_NPC(this) && IS_IMMORTAL(this)))
	return TRUE;

    if ( IS_AFFECTED(this, AFF_BLIND) )
	return FALSE;

    if ( room_is_dark( this->in_room ) && !IS_AFFECTED(this, AFF_INFRARED) )
	return FALSE;

    if ( IS_AFFECTED(victim, AFF_INVISIBLE)
    &&   !IS_AFFECTED(this, AFF_DETECT_INVIS) )
	return FALSE;

    /* sneaking */
    if ( IS_AFFECTED(victim, AFF_SNEAK)
    &&   !IS_AFFECTED(this,AFF_DETECT_HIDDEN)
    &&   victim->fighting == NULL)
    {
	int chance;
	chance = get_skill(victim,gsn_sneak);
	chance += get_curr_stat(victim,STAT_DEX) * 3/2;
 	chance -= get_curr_stat(this,STAT_INT) * 2;
	chance -= this->level - victim->level * 3/2;

	if (number_percent() < chance)
	    return FALSE;
    }

    if ( IS_AFFECTED(victim, AFF_HIDE)
    &&   !IS_AFFECTED(this, AFF_DETECT_HIDDEN)
    &&   victim->fighting == NULL)
	return FALSE;

    return TRUE;
}

/*
 * True if char can see obj.
 */
bool Character::can_see( Object *obj )
{
    if ( !this->isNPC() && IS_SET(this->act, PLR_HOLYLIGHT) )
	return TRUE;

    if ( IS_SET(obj->getExtraFlags(),ITEM_VIS_DEATH))
	return FALSE;

    if ( IS_AFFECTED( this, AFF_BLIND ) && obj->getItemType() != ITEM_POTION)
	return FALSE;

    if ( obj->getItemType() == ITEM_LIGHT && obj->getValues().at(2) != 0 )
	return TRUE;

    if ( IS_SET(obj->getExtraFlags(), ITEM_INVIS)
    &&   !IS_AFFECTED(this, AFF_DETECT_INVIS) )
        return FALSE;

    if ( obj->hasStat(ITEM_GLOW))
	return TRUE;

    if ( room_is_dark( this->in_room ) && !IS_AFFECTED(this, AFF_INFRARED) )
	return FALSE;

    return TRUE;
}



int Character::getRange() {
    return 0;
}

Race * Character::getRace() { return _race; }
void Character::setRace(Race *value) { _race = value; }

bool Character::didJustDie() {
    return false;
}

void Character::update() {
    AFFECT_DATA *paf;
    AFFECT_DATA *paf_next;

    if ( this->desc && this->desc->connected == ConnectedState::Playing )
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

    for (auto paf : this->affected)
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

            this->removeAffect(paf);
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

        ::act("$n writhes in agony as plague sores erupt from $s skin.", this,NULL,NULL,TO_ROOM, POS_RESTING);
        send_to_char("You writhe in agony from the plague.\n\r",this);

        AFFECT_DATA *af;
        for (auto paf : this->affected)
        {
            af = paf;
            if (paf->type == gsn_plague)
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
                ::act("$n shivers and looks very ill.",vch,NULL,NULL,TO_ROOM, POS_RESTING);
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

        poison = this->findAffectBySn(gsn_poison);

        if (poison != NULL)
        {
            ::act( "$n shivers and suffers.", this, NULL, NULL, TO_ROOM, POS_RESTING );
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

/*
 * Retrieve a character's trusted level for permission checking.
 */
int Character::getTrust( )
{
    if ( this->desc != NULL && this->desc->original != NULL ) {
        return this->desc->original->getTrust();
    }

    if (this->trust) {
	    return this->trust;
    }

    return this->level;
}


bool Character::isDenied() {
    return IS_SET(this->act, PLR_DENY);
}

bool Character::isImmortal() {
    return IS_IMMORTAL(this);
}

/*
 * Find a piece of eq on a character.
 */
Object * Character::getEquipment( int iWear )
{
    for ( auto obj : _carrying )
    {
        if (obj->getWearLocation() == iWear ) {
    	    return obj;
        }
    }

    return nullptr;
}

void Character::getOntoObject(Object *obj) { _on = obj; }
Object * Character::onObject() { return _on; }

AFFECT_DATA * Character::findAffectBySn(int sn) {
    for (auto paf_find : this->affected)
    {
        if ( paf_find->type == sn ) {
	        return paf_find;
        }
    }

    return nullptr;
}


/*
 * Remove an affect from a char.
 */
void Character::removeAffect( AFFECT_DATA *paf )
{
    int where;
    int vector;

    if ( this->affected.empty() )
    {
        bug( "Affect_remove: no affect.", 0 );
        return;
    }

    this->modifyAffect( paf, false );
    where = paf->where;
    vector = paf->bitvector;
    
    this->affected.remove(paf);

    free_affect(paf);

    this->affect_check(where,vector);
    return;
}

/*
 * Apply or remove an affect to a character.
 */
void Character::modifyAffect( AFFECT_DATA *paf, bool fAdd )
{
    Object *wield;
    int mod,i;

    mod = paf->modifier;

    if ( fAdd )
    {
        switch (paf->where)
        {
        case TO_AFFECTS:
            SET_BIT(this->affected_by, paf->bitvector);
            break;
        case TO_IMMUNE:
            SET_BIT(this->imm_flags,paf->bitvector);
            break;
        case TO_RESIST:
            SET_BIT(this->res_flags,paf->bitvector);
            break;
        case TO_VULN:
            SET_BIT(this->vuln_flags,paf->bitvector);
            break;
        }
    }
    else
    {
        switch (paf->where)
        {
        case TO_AFFECTS:
            REMOVE_BIT(this->affected_by, paf->bitvector);
            break;
        case TO_IMMUNE:
            REMOVE_BIT(this->imm_flags,paf->bitvector);
            break;
        case TO_RESIST:
            REMOVE_BIT(this->res_flags,paf->bitvector);
            break;
        case TO_VULN:
            REMOVE_BIT(this->vuln_flags,paf->bitvector);
            break;
        }
	    mod = 0 - mod;
    }

    switch ( paf->location )
    {
    default:
	bug( "Affect_modify: unknown location %d.", paf->location );
	return;

    case APPLY_NONE:						break;
    case APPLY_STR:           this->mod_stat[STAT_STR]	+= mod;	break;
    case APPLY_DEX:           this->mod_stat[STAT_DEX]	+= mod;	break;
    case APPLY_INT:           this->mod_stat[STAT_INT]	+= mod;	break;
    case APPLY_WIS:           this->mod_stat[STAT_WIS]	+= mod;	break;
    case APPLY_CON:           this->mod_stat[STAT_CON]	+= mod;	break;
    case APPLY_SEX:           this->sex			+= mod;	break;
    case APPLY_CLASS:						break;
    case APPLY_LEVEL:						break;
    case APPLY_AGE:						break;
    case APPLY_HEIGHT:						break;
    case APPLY_WEIGHT:						break;
    case APPLY_MANA:          this->max_mana		+= mod;	break;
    case APPLY_HIT:           this->max_hit		+= mod;	break;
    case APPLY_MOVE:          this->max_move		+= mod;	break;
    case APPLY_GOLD:						break;
    case APPLY_EXP:						break;
    case APPLY_AC:
        for (i = 0; i < 4; i ++)
            this->armor[i] += mod;
        break;
    case APPLY_HITROLL:       this->hitroll		+= mod;	break;
    case APPLY_DAMROLL:       this->damroll		+= mod;	break;
    case APPLY_SAVES:   this->saving_throw		+= mod;	break;
    case APPLY_SAVING_ROD:    this->saving_throw		+= mod;	break;
    case APPLY_SAVING_PETRI:  this->saving_throw		+= mod;	break;
    case APPLY_SAVING_BREATH: this->saving_throw		+= mod;	break;
    case APPLY_SAVING_SPELL:  this->saving_throw		+= mod;	break;
    case APPLY_SPELL_AFFECT:  					break;
    }

    /*
     * Check for weapon wielding.
     * Guard against recursion (for weapons with affects).
     */
    if ( !this->isNPC() && ( wield = this->getEquipment(WEAR_WIELD ) ) != NULL
    &&   get_obj_weight(wield) > (str_app[get_curr_stat(this,STAT_STR)].wield*10))
    {
	static int depth;

	if ( depth == 0 )
	{
	    depth++;
	    ::act( "You drop $p.", this, wield, NULL, TO_CHAR, POS_RESTING );
	    ::act( "$n drops $p.", this, wield, NULL, TO_ROOM, POS_RESTING );
	    obj_from_char( wield );
	    obj_to_room( wield, this->in_room );
	    depth--;
	}
    }

    return;
}

/* fix object affects when removing one */
void Character::affect_check(int where,int vector)
{
    if (where == TO_OBJECT || where == TO_WEAPON || vector == 0)
	return;

    for (auto paf : this->affected) {
        if (paf->where == where && paf->bitvector == vector)
        {
            switch (where)
            {
                case TO_AFFECTS:
                SET_BIT(this->affected_by,vector);
                break;
                case TO_IMMUNE:
                SET_BIT(this->imm_flags,vector);   
                break;
                case TO_RESIST:
                SET_BIT(this->res_flags,vector);
                break;
                case TO_VULN:
                SET_BIT(this->vuln_flags,vector);
                break;
            }
            return;
        }
    }

    for (auto obj : this->getCarrying())
    {
        if (obj->getWearLocation() == -1)
            continue;

            for (auto paf : obj->getAffectedBy()) {
                if (paf->where == where && paf->bitvector == vector)
                {
                    switch (where)
                    {
                        case TO_AFFECTS:
                            SET_BIT(this->affected_by,vector);
                            break;
                        case TO_IMMUNE:
                            SET_BIT(this->imm_flags,vector);
                            break;
                        case TO_RESIST:
                            SET_BIT(this->res_flags,vector);
                            break;
                        case TO_VULN:
                            SET_BIT(this->vuln_flags,vector);
                    
                    }
                    return;
                }
            }

        if (obj->isEnchanted())
	    continue;

        for (auto paf : obj->getObjectIndexData()->affected) {
            if (paf->where == where && paf->bitvector == vector)
            {
                switch (where)
                {
                    case TO_AFFECTS:
                        SET_BIT(this->affected_by,vector);
                        break;
                    case TO_IMMUNE:
                        SET_BIT(this->imm_flags,vector);
                        break;
                    case TO_RESIST:
                        SET_BIT(this->res_flags,vector);
                        break;
                    case TO_VULN:
                        SET_BIT(this->vuln_flags,vector);
                        break;
                }
                return;
            }
        }
    }
}

/*
 * Give an affect to a char.
 */
void Character::giveAffect( AFFECT_DATA *paf )
{
    auto paf_new = new_affect();

    *paf_new		= *paf;

    this->affected.push_back(paf_new);
    this->modifyAffect( paf_new, true );
    return;
}

/*
 * Give an obj to a char.
 */
void Character::addObject(Object *obj) {
    obj->setCarriedBy(this);
    obj->setInRoom(nullptr);
    obj->setInObject(nullptr);
    this->carry_number	+= get_obj_number( obj );
    this->carry_weight	+= get_obj_weight( obj );
    this->_carrying.push_back(obj);
}

/*
 * Take an obj from its character.
 */
void Character::removeObject(Object *obj) {

    if ( obj->getWearLocation() != WEAR_NONE ) {
    	this->unequip( obj );
    }

    this->_carrying.remove(obj);

    obj->setCarriedBy(NULL);
    this->carry_number	-= get_obj_number( obj );
    this->carry_weight	-= get_obj_weight( obj );
}


/*
 * Unequip a char with an obj.
 */
void Character::unequip( Object *obj )
{
    // AFFECT_DATA *paf = NULL;
    // AFFECT_DATA *lpaf = NULL;
    // AFFECT_DATA *lpaf_next = NULL;
    int i;

    if ( obj->getWearLocation() == WEAR_NONE )
    {
	bug( "Unequip_char: already unequipped.", 0 );
	return;
    }

    for (i = 0; i < 4; i++)
    	this->armor[i]	+= apply_ac( obj, obj->getWearLocation(),i );
    obj->setWearLocation(-1);

    if (!obj->isEnchanted())
    {
        for ( auto paf : obj->getObjectIndexData()->affected ) {
            if ( paf->location == APPLY_SPELL_AFFECT )
            {
                for ( auto lpaf : this->affected )
                {
                    if ((lpaf->type == paf->type) &&
                        (lpaf->level == paf->level) &&
                        (lpaf->location == APPLY_SPELL_AFFECT))
                    {
                        this->removeAffect( lpaf );
                    }
                }
            }
            else
            {
                this->modifyAffect( paf, false );
                this->affect_check(paf->where,paf->bitvector);
            }
        }
    }

    for ( auto paf : obj->getAffectedBy() ) {
        if ( paf->location == APPLY_SPELL_AFFECT )
        {
            bug ( "Norm-Apply: %d", 0 );
            for ( auto lpaf : this->affected )
            {
                if ((lpaf->type == paf->type) &&
                    (lpaf->level == paf->level) &&
                    (lpaf->location == APPLY_SPELL_AFFECT))
                {
                    bug ( "location = %d", lpaf->location );
                    bug ( "type = %d", lpaf->type );
                    this->removeAffect( lpaf );
                }
            }
        }
        else
        {
            this->modifyAffect( paf, false );
            this->affect_check(paf->where,paf->bitvector);	
        }
    }

    if ( obj->getItemType() == ITEM_LIGHT
    &&   obj->getValues().at(2) != 0
    &&   this->in_room != NULL
    &&   this->in_room->light > 0 )
	--this->in_room->light;

    return;
}

/*
 * It is very important that this be an equivalence relation:
 * (1) A ~ A
 * (2) if A ~ B then B ~ A
 * (3) if A ~ B  and B ~ C, then A ~ C
 */
bool Character::isSameGroup( Character *bch )
{
    auto ach = this;
    if ( ach == NULL || bch == NULL)
	return FALSE;

    if ( ach->leader != NULL ) ach = ach->leader;
    if ( bch->leader != NULL ) bch = bch->leader;
    return ach == bch;
}