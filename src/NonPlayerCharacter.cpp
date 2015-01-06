#include <string.h>
#include <sys/types.h>
#include <stdlib.h>
#include "merc.h"
#include "recycle.h"
#include "NonPlayerCharacter.h"

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
                this->race		= pMobIndex->race;
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
                        affect_to_char( this, &af );
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
                        affect_to_char( this, &af );
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
                        affect_to_char(this,&af);
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
                        affect_to_char(this,&af);
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
                this->race		= pMobIndex->race;
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