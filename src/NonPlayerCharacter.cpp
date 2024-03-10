#include <string.h>
#include <sys/types.h>
#include <stdlib.h>
#include "merc.h"
#include "recycle.h"
#include "NonPlayerCharacter.h"
#include "Object.h"
#include "RaceManager.h"
#include "Room.h"

extern RaceManager * race_manager;

NonPlayerCharacter::NonPlayerCharacter()
	: Character() {
	mobile_count++;
}

NonPlayerCharacter::NonPlayerCharacter(MOB_INDEX_DATA *pMobIndex)
        : Character() {
        int i;
        AFFECT_DATA af;

        mobile_count++;

        if ( pMobIndex == NULL )
        {
                bug( "Create_mobile: NULL pMobIndex.", 0 );
                exit( 1 );
        }

        this->pIndexData	= pMobIndex;

        this->setName( pMobIndex->player_name );    /* OLC */
        this->short_descr	= str_dup( pMobIndex->short_descr );    /* OLC */
        this->long_descr	= str_dup( pMobIndex->long_descr );     /* OLC */
        this->setDescription( pMobIndex->description );    /* OLC */
        this->id		= get_mob_id();
        this->spec_fun	= pMobIndex->spec_fun;
        this->prompt		= NULL;
        this->mprog_target   = NULL;

        if (pMobIndex->wealth == 0)
        {
                this->silver = 0;
                this->gold   = 0;
        }
        else
        {
                long wealth;

                wealth = number_range(pMobIndex->wealth/2, 3 * pMobIndex->wealth/2);
                this->gold = number_range(wealth/200,wealth/100);
                this->silver = wealth - (this->gold * 100);
        }

        if (pMobIndex->new_format)
                /* load in new style */
        {
                /* read from prototype */
                this->group		= pMobIndex->group;
                this->act 		= pMobIndex->act;
                this->comm		= COMM_NOCHANNELS|COMM_NOSHOUT|COMM_NOTELL;
                this->affected_by	= pMobIndex->affected_by;
                this->alignment		= pMobIndex->alignment;
                this->level		= pMobIndex->level;
                this->hitroll		= pMobIndex->hitroll;
                this->damroll		= pMobIndex->damage[DICE_BONUS];
                this->max_hit		= dice(pMobIndex->hit[DICE_NUMBER],
                        pMobIndex->hit[DICE_TYPE])
                        + pMobIndex->hit[DICE_BONUS];
                this->hit		= this->max_hit;
                this->max_mana		= dice(pMobIndex->mana[DICE_NUMBER],
                        pMobIndex->mana[DICE_TYPE])
                        + pMobIndex->mana[DICE_BONUS];
                this->mana		= this->max_mana;
                this->damage[DICE_NUMBER]= pMobIndex->damage[DICE_NUMBER];
                this->damage[DICE_TYPE]	= pMobIndex->damage[DICE_TYPE];
                this->dam_type		= pMobIndex->dam_type;
                if (this->dam_type == 0)
                        switch(number_range(1,3))
                        {
                                case (1): this->dam_type = 3;        break;  /* slash */
                                case (2): this->dam_type = 7;        break;  /* pound */
                                case (3): this->dam_type = 11;       break;  /* pierce */
                        }
                for (i = 0; i < 4; i++)
                        this->armor[i]	= pMobIndex->ac[i];
                this->off_flags		= pMobIndex->off_flags;
                this->imm_flags		= pMobIndex->imm_flags;
                this->res_flags		= pMobIndex->res_flags;
                this->vuln_flags		= pMobIndex->vuln_flags;
                this->start_pos		= pMobIndex->start_pos;
                this->default_pos	= pMobIndex->default_pos;
                this->sex		= pMobIndex->sex;
                if (this->sex == 3) /* random sex */
                        this->sex = number_range(1,2);
                this->setRace(race_manager->getRaceByLegacyId(pMobIndex->race));
                this->form		= pMobIndex->form;
                this->parts		= pMobIndex->parts;
                this->size		= pMobIndex->size;
                this->material		= str_dup(pMobIndex->material);

                /* computed on the spot */

                for (i = 0; i < MAX_STATS; i ++)
                        this->perm_stat[i] = UMIN(25,11 + this->level/4);

                if (IS_SET(this->act,ACT_WARRIOR))
                {
                        this->perm_stat[STAT_STR] += 3;
                        this->perm_stat[STAT_INT] -= 1;
                        this->perm_stat[STAT_CON] += 2;
                }

                if (IS_SET(this->act,ACT_THIEF))
                {
                        this->perm_stat[STAT_DEX] += 3;
                        this->perm_stat[STAT_INT] += 1;
                        this->perm_stat[STAT_WIS] -= 1;
                }

                if (IS_SET(this->act,ACT_CLERIC))
                {
                        this->perm_stat[STAT_WIS] += 3;
                        this->perm_stat[STAT_DEX] -= 1;
                        this->perm_stat[STAT_STR] += 1;
                }

                if (IS_SET(this->act,ACT_MAGE))
                {
                        this->perm_stat[STAT_INT] += 3;
                        this->perm_stat[STAT_STR] -= 1;
                        this->perm_stat[STAT_DEX] += 1;
                }

                if (IS_SET(this->off_flags,OFF_FAST))
                        this->perm_stat[STAT_DEX] += 2;

                this->perm_stat[STAT_STR] += this->size - SIZE_MEDIUM;
                this->perm_stat[STAT_CON] += (this->size - SIZE_MEDIUM) / 2;

                /* let's get some spell action */
                if (IS_AFFECTED(this,AFF_SANCTUARY))
                {
                        af.where	 = TO_AFFECTS;
                        af.type      = skill_lookup("sanctuary");
                        af.level     = this->level;
                        af.duration  = -1;
                        af.location  = APPLY_NONE;
                        af.modifier  = 0;
                        af.bitvector = AFF_SANCTUARY;
                        this->giveAffect( &af );
                }

                if (IS_AFFECTED(this,AFF_HASTE))
                {
                        af.where	 = TO_AFFECTS;
                        af.type      = skill_lookup("haste");
                        af.level     = this->level;
                        af.duration  = -1;
                        af.location  = APPLY_DEX;
                        af.modifier  = 1 + (this->level >= 18) + (this->level >= 25) +
                                (this->level >= 32);
                        af.bitvector = AFF_HASTE;
                        this->giveAffect( &af );
                }

                if (IS_AFFECTED(this,AFF_PROTECT_EVIL))
                {
                        af.where	 = TO_AFFECTS;
                        af.type	 = skill_lookup("protection evil");
                        af.level	 = this->level;
                        af.duration	 = -1;
                        af.location	 = APPLY_SAVES;
                        af.modifier	 = -1;
                        af.bitvector = AFF_PROTECT_EVIL;
                        this->giveAffect(&af);
                }

                if (IS_AFFECTED(this,AFF_PROTECT_GOOD))
                {
                        af.where	 = TO_AFFECTS;
                        af.type      = skill_lookup("protection good");
                        af.level     = this->level;
                        af.duration  = -1;
                        af.location  = APPLY_SAVES;
                        af.modifier  = -1;
                        af.bitvector = AFF_PROTECT_GOOD;
                        this->giveAffect(&af);
                }
        }
        else /* read in old format and convert */
        {
                this->act		= pMobIndex->act;
                this->affected_by	= pMobIndex->affected_by;
                this->alignment		= pMobIndex->alignment;
                this->level		= pMobIndex->level;
                this->hitroll		= pMobIndex->hitroll;
                this->damroll		= 0;
                this->max_hit		= this->level * 8 + number_range(
                        this->level * this->level/4,
                        this->level * this->level);
                this->max_hit *= .9;
                this->hit		= this->max_hit;
                this->max_mana		= 100 + dice(this->level,10);
                this->mana		= this->max_mana;
                switch(number_range(1,3))
                {
                        case (1): this->dam_type = 3; 	break;  /* slash */
                        case (2): this->dam_type = 7;	break;  /* pound */
                        case (3): this->dam_type = 11;	break;  /* pierce */
                }
                for (i = 0; i < 3; i++)
                        this->armor[i]	= interpolate(this->level,100,-100);
                this->armor[3]		= interpolate(this->level,100,0);
                this->setRace(race_manager->getRaceByLegacyId(pMobIndex->race));
                this->off_flags		= pMobIndex->off_flags;
                this->imm_flags		= pMobIndex->imm_flags;
                this->res_flags		= pMobIndex->res_flags;
                this->vuln_flags		= pMobIndex->vuln_flags;
                this->start_pos		= pMobIndex->start_pos;
                this->default_pos	= pMobIndex->default_pos;
                this->sex		= pMobIndex->sex;
                this->form		= pMobIndex->form;
                this->parts		= pMobIndex->parts;
                this->size		= SIZE_MEDIUM;
                this->material		= (char*)"";

                for (i = 0; i < MAX_STATS; i ++)
                        this->perm_stat[i] = 11 + this->level/4;
        }

        this->position = this->start_pos;


        /* link the mob to the world list */
        char_list.push_back(this);
        pMobIndex->count++;
}

NonPlayerCharacter::~NonPlayerCharacter() {
        mobile_count--;
}

bool NonPlayerCharacter::isNPC() {
        return true;
}

void NonPlayerCharacter::update() {
        if ( this->position >= POS_STUNNED ) {
                /* check to see if we need to go home */
                if (IS_NPC(this) && this->zone != NULL && this->zone != this->in_room->area
                        && this->desc == NULL && this->fighting == NULL
                        && !IS_AFFECTED(this, AFF_CHARM) && number_percent() < 5) {
                        ::act("$n wanders on home.", this, NULL, NULL, TO_ROOM, POS_RESTING );
                        extract_char(this, TRUE);
                        return;
                }
        }
        Character::update();
}

/*
 * Regeneration stuff.
 */
int NonPlayerCharacter::hit_gain( )
{
        if (this->in_room == NULL)
                return 0;

        int gain =  5 + this->level;
        if (IS_AFFECTED(this,AFF_REGENERATION))
                gain *= 2;

        switch(this->position)
        {
                default :       gain /= 2;          break;
                case POS_SLEEPING:  gain = 3 * gain/2;      break;
                case POS_RESTING:                   break;
                case POS_FIGHTING:  gain /= 3;          break;
        }


        gain = gain * this->in_room->heal_rate / 100;

        if (this->onObject() != NULL && this->onObject()->getItemType() == ITEM_FURNITURE)
                gain = gain * this->onObject()->getValues().at(3) / 100;

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





int NonPlayerCharacter::mana_gain( )
{
        if (this->in_room == NULL)
                return 0;

        int gain = 5 + this->level;
        switch (this->position)
        {
                default:        gain /= 2;      break;
                case POS_SLEEPING:  gain = 3 * gain/2;  break;
                case POS_RESTING:               break;
                case POS_FIGHTING:  gain /= 3;      break;
        }


        gain = gain * this->in_room->mana_rate / 100;

        if (this->onObject() != NULL && this->onObject()->getItemType() == ITEM_FURNITURE)
                gain = gain * this->onObject()->getValues().at(4) / 100;

        if ( IS_AFFECTED( this, AFF_POISON ) )
                gain /= 4;

        if (IS_AFFECTED(this, AFF_PLAGUE))
                gain /= 8;

        if (IS_AFFECTED(this,AFF_HASTE) || IS_AFFECTED(this,AFF_SLOW))
                gain /=2 ;

        return UMIN(gain, this->max_mana - this->mana);
}



int NonPlayerCharacter::move_gain( )
{
        int gain;

        if (this->in_room == NULL)
                return 0;

        gain = this->level;

        gain = gain * this->in_room->heal_rate/100;

        if (this->onObject() != NULL && this->onObject()->getItemType() == ITEM_FURNITURE)
                gain = gain * this->onObject()->getValues().at(3) / 100;

        if ( IS_AFFECTED(this, AFF_POISON) )
                gain /= 4;

        if (IS_AFFECTED(this, AFF_PLAGUE))
                gain /= 8;

        if (IS_AFFECTED(this,AFF_HASTE) || IS_AFFECTED(this,AFF_SLOW))
                gain /=2 ;

        return UMIN(gain, this->max_move - this->move);
}

/*
 * Retrieve a character's trusted level for permission checking.
 */
int NonPlayerCharacter::getTrust( )
{
    if ( this->desc != NULL && this->desc->original != NULL ) {
        return this->desc->original->getTrust();
    }

    if (this->level >= LEVEL_HERO) {
        return LEVEL_HERO;
    }
    return this->level;
}