#include <string.h>
#include <sys/types.h>
#include "merc.h"
#include "PlayerCharacter.h"
#include "Wiznet.h"

PlayerCharacter::PlayerCharacter()
        : Character() {
        this->pcdata = new PC_DATA();

        this->id				= PlayerCharacter::get_pc_id();
        this->race				= race_lookup("human");
        this->act				= PLR_NOSUMMON;
        this->comm				= COMM_COMBINE
                | COMM_PROMPT;
        this->prompt 				= str_dup("<%X to level>%c<%h/%Hhp %m/%Mm %v/%Vmv> ");
        this->range				= 5;
        this->reclass_num			= 0;
        this->pcdata->confirm_delete		= FALSE;
        this->pcdata->board                   = &boards[DEFAULT_BOARD];
        this->pcdata->pwd			= str_dup( "" );
        this->pcdata->bamfin			= str_dup( "" );
        this->pcdata->bamfout			= str_dup( "" );
        this->pcdata->title			= str_dup( "" );
        for (int stat =0; stat < MAX_STATS; stat++)
                this->perm_stat[stat]		= 13;
        this->pcdata->condition[COND_THIRST]	= 48;
        this->pcdata->condition[COND_FULL]	= 48;
        this->pcdata->condition[COND_HUNGER]	= 48;
        this->pcdata->security		= 0;	/* OLC */

        this->was_note_room = NULL;
        this->join = NULL;
        this->pkills = 0;
        this->pkilled = 0;
        this->mkills = 0;
        this->mkilled = 0;
        this->range = 0;
        this->adrenaline = 0;
        this->jkilled = 0;
}

PlayerCharacter::~PlayerCharacter() {

}

void PlayerCharacter::gain_exp( int gain ) {
        char buf[MAX_STRING_LENGTH];

        if ( IS_NPC(this) || this->level >= LEVEL_HERO )
                return;

        this->exp = UMAX( exp_per_level(this,this->pcdata->points), this->exp + gain );
        while ( this->level < LEVEL_HERO && this->exp >= exp_per_level(this,this->pcdata->points) * (this->level+1) )
        {
                send_to_char( "You raise a level!!  ", this );
                this->level += 1;
                sprintf(buf,"%s gained level %d",this->getName(),this->level);
                log_string(buf);
                sprintf(buf,"$N has attained level %d!",this->level);
                this->advance_level(FALSE);
                save_char_obj(this);
                Wiznet::instance()->report( buf, this, NULL, WIZ_LEVELS, 0, 0 );
        }

        return;
}

bool PlayerCharacter::isNPC() {
        return false;
}

ROOM_INDEX_DATA * PlayerCharacter::getWasNoteRoom() {
        return this->was_note_room;
}

void PlayerCharacter::setWasNoteRoom( ROOM_INDEX_DATA *room ) {
        this->was_note_room = room;
        return;
}

void PlayerCharacter::setClanCust(int value) {
        this->clan_cust = value;
}

void PlayerCharacter::setJoin(CLAN_DATA *clan) {
        this->join = clan;
}

void PlayerCharacter::incrementPlayerKills(int amount) {
        this->pkills += amount;
}

void PlayerCharacter::incrementMobKills(int amount) {
        this->mkills += amount;
}

int PlayerCharacter::getAdrenaline() {
        return this->adrenaline;
}

int PlayerCharacter::getPlayerKillCount() {
        return this->pkills;
}

int PlayerCharacter::getKilledByPlayersCount() {
        return this->pkilled;
}

int PlayerCharacter::getMobKillCount() {
        return this->mkills;
}

int PlayerCharacter::getKilledByMobCount() {
        return this->mkilled;
}

int PlayerCharacter::getRange() {
        return this->range;
}

void PlayerCharacter::incrementRange(int amount) {
        this->range += amount;
}

void PlayerCharacter::incrementKilledByPlayerCount(int amount) {
        this->pkilled += amount;
}

void PlayerCharacter::setJustKilled(int tickTimer) {
        this->jkilled = tickTimer;
}

void PlayerCharacter::incrementKilledByMobCount(int amount) {
        this->mkilled += amount;
}

bool PlayerCharacter::didJustDie() {
        return this->jkilled > 0;
}

void PlayerCharacter::writeToFile(FILE *fp) {
        AFFECT_DATA *paf;
        int sn, gn, pos, i;

        // If NPC - this should be #MOB
        fprintf( fp, "#PLAYER\n" );

        fprintf( fp, "Name %s~\n",	this->getName()		);
        fprintf( fp, "Id   %ld\n", this->id			);
        fprintf( fp, "LogO %ld\n",	current_time		);
        fprintf( fp, "Vers %d\n",   5			);
        if (this->short_descr[0] != '\0')
                fprintf( fp, "ShD  %s~\n",	this->short_descr	);
        if( this->long_descr[0] != '\0')
                fprintf( fp, "LnD  %s~\n",	this->long_descr	);
        if ( this->getDescription() )
                fprintf( fp, "Desc %s~\n",	this->getDescription()	);
        if (this->prompt != NULL || !str_cmp(this->prompt,"<%X to level>%c<%h/%Hhp %m/%Mm %v/%Vmv> "))
                fprintf( fp, "Prom %s~\n",      this->prompt  	);
        fprintf( fp, "Race %s~\n", pc_race_table[this->race].name );

        if (this->race == race_lookup("werefolk"))
        {
                fprintf( fp, "O_fo %d\n", this->orig_form		);
                fprintf( fp, "M_fo %d\n", this->morph_form		);
        }

        if (this->race == race_lookup("draconian") && this->level >= 10)
        {
                fprintf( fp, "Drac %d\n", this->drac );
                fprintf( fp, "Brea %d\n", this->breathe);
        }

        fprintf( fp, "Kills %d %d %d %d\n", this->pkills, this->pkilled,
                this->mkills, this->mkilled);
        if (this->pcdata->clan)
                fprintf( fp, "Clan %s~\n", this->pcdata->clan->name);
        fprintf( fp, "Sex  %d\n",	this->sex			);
        fprintf( fp, "Cla  %d\n",	this->class_num		);
        fprintf( fp, "Levl %d\n",	this->level		);
        if (this->trust != 0)
                fprintf( fp, "Tru  %d\n",	this->trust	);
        fprintf( fp, "Sec  %d\n",    this->pcdata->security	);	/* OLC */
        fprintf( fp, "Plyd %d\n",
                this->played + (int) (current_time - this->logon)	);
        fprintf( fp, "Scro %d\n", 	this->lines		);
        fprintf( fp, "Room %d\n",
                (  this->in_room == get_room_index( ROOM_VNUM_LIMBO )
                        && this->was_in_room != NULL )
                        ? this->was_in_room->vnum
                        : this->in_room == NULL ? 3001 : this->in_room->vnum );

        fprintf( fp, "HMV  %d %d %d %d %d %d\n",
                this->hit, this->max_hit, this->mana, this->max_mana, this->move, this->max_move );
        if (this->gold > 0)
                fprintf( fp, "Gold %ld\n",	this->gold		);
        else
                fprintf( fp, "Gold %d\n", 0			);
        if (this->silver > 0)
                fprintf( fp, "Silv %ld\n",this->silver		);
        else
                fprintf( fp, "Silv %d\n",0			);
        fprintf( fp, "Exp  %d\n",	this->exp			);
        if (this->act != 0)
                fprintf( fp, "Act  %s\n",   print_flags(this->act));
        if (this->affected_by != 0)
                fprintf( fp, "AfBy %s\n",   print_flags(this->affected_by));
        if (this->reclass_num > 0)
                fprintf( fp, "Rnum %d\n",	this->reclass_num);
        if (this->done != 0)
                fprintf( fp, "Done %s\n",   print_flags(this->done));
        fprintf( fp, "Comm %s\n",       print_flags(this->comm));
        if (this->wiznet)
                fprintf( fp, "Wizn %s\n",   print_flags(this->wiznet));
        if (this->invis_level)
                fprintf( fp, "Invi %d\n", 	this->invis_level	);
        if (this->incog_level)
                fprintf(fp,"Inco %d\n",this->incog_level);
        fprintf( fp, "Pos  %d\n",
                this->position == POS_FIGHTING ? POS_STANDING : this->position );
        if (this->practice != 0)
                fprintf( fp, "Prac %d\n",	this->practice	);
        if (this->train != 0)
                fprintf( fp, "Trai %d\n",	this->train	);
        if (this->saving_throw != 0)
                fprintf( fp, "Save  %d\n",	this->saving_throw);
        fprintf( fp, "Alig  %d\n",	this->alignment		);
        if (this->hitroll != 0)
                fprintf( fp, "Hit   %d\n",	this->hitroll	);
        if (this->damroll != 0)
                fprintf( fp, "Dam   %d\n",	this->damroll	);
        fprintf( fp, "ACs %d %d %d %d\n",
                this->armor[0],this->armor[1],this->armor[2],this->armor[3]);
        if (this->wimpy !=0 )
                fprintf( fp, "Wimp  %d\n",	this->wimpy	);
        fprintf( fp, "Attr %d %d %d %d %d\n",
                this->perm_stat[STAT_STR],
                this->perm_stat[STAT_INT],
                this->perm_stat[STAT_WIS],
                this->perm_stat[STAT_DEX],
                this->perm_stat[STAT_CON] );

        fprintf (fp, "AMod %d %d %d %d %d\n",
                this->mod_stat[STAT_STR],
                this->mod_stat[STAT_INT],
                this->mod_stat[STAT_WIS],
                this->mod_stat[STAT_DEX],
                this->mod_stat[STAT_CON] );

//        if ( IS_NPC(ch) )
//        {
//                fprintf( fp, "Vnum %d\n",	this->pIndexData->vnum	);
//        }
//        else
//        {
                fprintf( fp, "Pass %s~\n",	this->pcdata->pwd		);
                if (this->pcdata->bamfin[0] != '\0')
                        fprintf( fp, "Bin  %s~\n",	this->pcdata->bamfin);
                if (this->pcdata->bamfout[0] != '\0')
                        fprintf( fp, "Bout %s~\n",	this->pcdata->bamfout);
                fprintf( fp, "Titl %s~\n",	this->pcdata->title	);

                if (IS_CLANNED(this))
                        fprintf( fp, "Rang %d\n",	this->range		);

                fprintf( fp, "Pnts %d\n",   	this->pcdata->points      );
                fprintf( fp, "TSex %d\n",	this->pcdata->true_sex	);
                fprintf( fp, "LLev %d\n",	this->pcdata->last_level	);
                fprintf( fp, "HMVP %d %d %d\n", this->pcdata->perm_hit,
                        this->pcdata->perm_mana,
                        this->pcdata->perm_move);
                fprintf( fp, "Cnd  %d %d %d %d\n",
                        this->pcdata->condition[0],
                        this->pcdata->condition[1],
                        this->pcdata->condition[2],
                        this->pcdata->condition[3] );

                /* write alias */
                for (pos = 0; pos < MAX_ALIAS; pos++)
                {
                        if (this->pcdata->alias[pos] == NULL
                                ||  this->pcdata->alias_sub[pos] == NULL)
                                break;

                        fprintf(fp,"Alias %s %s~\n",this->pcdata->alias[pos],
                                this->pcdata->alias_sub[pos]);
                }

                /* Save note board status */
                /* Save number of boards in case that number changes */
                fprintf (fp, "Boards       %d ", MAX_BOARD);
                for (i = 0; i < MAX_BOARD; i++)
                        fprintf (fp, "%s %ld ", boards[i].short_name, this->pcdata->last_note[i]);
                fprintf (fp, "\n");

                for ( sn = 0; sn < MAX_SKILL; sn++ )
                {
                        if ( skill_table[sn].name != NULL && this->pcdata->learned[sn] > 0 )
                        {
                                fprintf( fp, "Skill %d %d %d '%s'\n",
                                        this->pcdata->learned[sn], this->pcdata->sk_level[sn],
                                        this->pcdata->sk_rating[sn], skill_table[sn].name );
                        }
                }

                for ( gn = 0; gn < MAX_GROUP; gn++ )
                {
                        if ( group_table[gn].name != NULL && this->pcdata->group_known[gn])
                        {
                                fprintf( fp, "Gr '%s'\n",group_table[gn].name);
                        }
                }
//        }

        for ( paf = this->affected; paf != NULL; paf = paf->next )
        {
                if (paf->type < 0 || paf->type>= MAX_SKILL)
                        continue;

                fprintf( fp, "Affc '%s' %3d %3d %3d %3d %3d %10d\n",
                        skill_table[paf->type].name,
                        paf->where,
                        paf->level,
                        paf->duration,
                        paf->modifier,
                        paf->location,
                        paf->bitvector
                );
        }

        fprintf( fp, "End\n\n" );
        return;
}

void PlayerCharacter::setAdrenaline(int value) {
        this->adrenaline = value;
}

bool PlayerCharacter::wantsToJoinClan( CLAN_DATA *clan ) {
        return this->join == clan;
}

long PlayerCharacter::get_pc_id() {
        static long last_pc_id;
        int val;

        val = (current_time <= last_pc_id) ? last_pc_id + 1 : current_time;
        last_pc_id = val;
        return val;
}

void PlayerCharacter::setKills(int pkills, int pkilled, int mkills, int mkilled) {
        this->pkills = pkills;
        this->pkilled = pkilled;
        this->mkills = mkills;
        this->mkilled = mkilled;
}

void PlayerCharacter::setRange(int range) {
        this->range = range;
}

void PlayerCharacter::update() {
        if ( this->adrenaline > 0 )
                --this->adrenaline;

        if ( this->jkilled > 0 )
                --this->jkilled;

        if ( this->level < LEVEL_IMMORTAL )
        {
                OBJ_DATA *obj;

                if ( ( obj = get_eq_char( this, WEAR_LIGHT ) ) != NULL
                        &&   obj->item_type == ITEM_LIGHT
                        &&   obj->value[2] > 0 )
                {
                        if ( --obj->value[2] == 0 && this->in_room != NULL )
                        {
                                --this->in_room->light;
                                ::act( "$p goes out.", this, obj, NULL, TO_ROOM );
                                ::act( "$p flickers and goes out.", this, obj, NULL, TO_CHAR );
                                extract_obj( obj );
                        }
                        else if ( obj->value[2] <= 5 && this->in_room != NULL)
                                ::act("$p flickers.",this,obj,NULL,TO_CHAR);
                }

                if ( ++this->timer >= 12 )
                {
                        if ( this->was_in_room == NULL && this->in_room != NULL )
                        {
                                this->was_in_room = this->in_room;
                                if ( this->fighting != NULL )
                                        stop_fighting( this, TRUE );
                                ::act( "$n disappears into the void.",
                                        this, NULL, NULL, TO_ROOM );
                                send_to_char( "You disappear into the void.\n\r", this );
                                if (this->level > 1)
                                        save_char_obj( this );
                                char_from_room( this );
                                char_to_room( this, get_room_index( ROOM_VNUM_LIMBO ) );
                        }
                }

                this->gain_condition( COND_DRUNK,  -1 );
                this->gain_condition( COND_FULL, this->size > SIZE_MEDIUM ? -4 : -2 );
                this->gain_condition( COND_THIRST, -1 );
                this->gain_condition( COND_HUNGER, this->size > SIZE_MEDIUM ? -2 : -1);
        }
        Character::update();
}


/*
 * Regeneration stuff.
 */
int PlayerCharacter::hit_gain( )
{
        if (this->in_room == NULL)
                return 0;

        int gain = UMAX(3,get_curr_stat(this,STAT_CON) - 3 + this->level/2);
        gain += class_table[this->class_num].hp_max - 10;
        if (this->hit < this->max_hit) {
            int number = number_percent();
            if (number < get_skill(this, gsn_fast_healing)) {
                gain += number * gain / 100;
            }
            check_improve(this, gsn_fast_healing, TRUE, 8);
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





int PlayerCharacter::mana_gain( )
{
        int gain;

        if (this->in_room == NULL)
                return 0;

        gain = (get_curr_stat(this,STAT_WIS)
                + get_curr_stat(this,STAT_INT) + this->level) / 2;
        if (this->mana < this->max_mana) {
            int number = number_percent();
            if (number < get_skill(this,gsn_meditation))
            {
                gain += number * gain / 100;
            }
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

        gain = gain * this->in_room->mana_rate / 100;

        if (this->on != NULL && this->on->item_type == ITEM_FURNITURE)
                gain = gain * this->on->value[4] / 100;

        if ( IS_AFFECTED( this, AFF_POISON ) )
                gain /= 4;

        if (IS_AFFECTED(this, AFF_PLAGUE))
                gain /= 8;

        if (IS_AFFECTED(this,AFF_HASTE) || IS_AFFECTED(this,AFF_SLOW))
                gain /=2 ;

        if (this->pcdata->clan)
                gain *= 1.5;

        return UMIN(gain, this->max_mana - this->mana);
}



int PlayerCharacter::move_gain( )
{
        int gain;

        if (this->in_room == NULL)
                return 0;

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

        gain = gain * this->in_room->heal_rate/100;

        if (this->on != NULL && this->on->item_type == ITEM_FURNITURE)
                gain = gain * this->on->value[3] / 100;

        if ( IS_AFFECTED(this, AFF_POISON) )
                gain /= 4;

        if (IS_AFFECTED(this, AFF_PLAGUE))
                gain /= 8;

        if (IS_AFFECTED(this,AFF_HASTE) || IS_AFFECTED(this,AFF_SLOW))
                gain /=2 ;

        if (this->pcdata->clan)
                gain *= 1.5;

        return UMIN(gain, this->max_move - this->move);
}