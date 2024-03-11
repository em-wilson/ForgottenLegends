#include <string.h>
#include <sys/types.h>
#include "merc.h"
#include "board.h"
#include "ILogger.h"
#include "IRaceManager.h"
#include "Object.h"
#include "ObjectHelper.h"
#include "PlayerRace.h"
#include "PlayerCharacter.h"
#include "RaceManager.h"
#include "recycle.h"
#include "Room.h"
#include "Wiznet.h"

/* global functions */
void do_get(Character *ch, char *argument);
void do_sacrifice(Character *ch, char *argument);

PlayerCharacter::PlayerCharacter(IRaceManager *race_manager)
    : Character()
{
    // START PCDATA
    for (int alias = 0; alias < MAX_ALIAS; alias++)
    {
        this->alias[alias] = NULL;
        this->alias_sub[alias] = NULL;
    }

    for (int i = 0; i < 4; i++)
    {
        this->condition[i] = 0;
    }

    for (int skill = 0; skill < MAX_SKILL; skill++)
    {
        this->learned[skill] = 0;
        this->sk_level[skill] = 0;
        this->sk_rating[skill] = 0;
    }

    this->_clan = NULL;
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
    this->board = NULL;
    this->in_progress = NULL;
    for (int board = 0; board < MAX_BOARD; board++)
    {
        this->last_note_read.emplace(board, 0);
    }

    for (int group = 0; group < MAX_GROUP; group++)
    {
        this->group_known[group] = false;
    }

    VALIDATE(this);
    // END PCDATA

    this->id = PlayerCharacter::get_pc_id();
    if (race_manager)
    {
        this->setRace(race_manager->getRaceByName("human"));
    }
    this->act = PLR_NOSUMMON;
    this->comm = COMM_COMBINE | COMM_PROMPT;
    this->prompt = str_dup("<%X to level>%c<%h/%Hhp %m/%Mm %v/%Vmv> ");
    this->range = 5;
    this->reclass_num = 0;
    this->confirm_delete = FALSE;
    this->board = &boards[DEFAULT_BOARD];
    this->bamfin = str_dup("");
    this->bamfout = str_dup("");
    this->title = str_dup("");
    for (int stat = 0; stat < MAX_STATS; stat++)
        this->perm_stat[stat] = 13;
    this->condition[COND_THIRST] = 48;
    this->condition[COND_FULL] = 48;
    this->condition[COND_HUNGER] = 48;

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

PlayerCharacter::~PlayerCharacter()
{
    if (this->bamfin)
        free_string(this->bamfin);
    if (this->bamfout)
        free_string(this->bamfout);
    if (this->title)
        free_string(this->title);
    if (this->buffer)
        free_buf(this->buffer);

    for (int alias = 0; alias < MAX_ALIAS; alias++)
    {
        if (this->alias[alias])
        {
            free_string(this->alias[alias]);
        }

        if (this->alias_sub[alias])
        {
            free_string(this->alias_sub[alias]);
        }
    }
    INVALIDATE(this);
}

void PlayerCharacter::gain_exp(int gain)
{
    char buf[MAX_STRING_LENGTH];

    if (IS_NPC(this) || this->level >= LEVEL_HERO)
        return;

    this->exp = UMAX(exp_per_level(this, this->points), this->exp + gain);
    while (this->level < LEVEL_HERO && this->exp >= exp_per_level(this, this->points) * (this->level + 1))
    {
        send_to_char("You raise a level!!  ", this);
        this->level += 1;
        snprintf(buf, sizeof(buf), "%s gained level %d", this->getName().c_str(), this->level);
        logger->log_string(buf);
        snprintf(buf, sizeof(buf), "$N has attained level %d!", this->level);
        this->advance_level(FALSE);
        save_char_obj(this);
        Wiznet::instance()->report(buf, this, NULL, WIZ_LEVELS, 0, 0);
    }

    return;
}

bool PlayerCharacter::isNPC()
{
    return false;
}

ROOM_INDEX_DATA *PlayerCharacter::getWasNoteRoom()
{
    return this->was_note_room;
}

void PlayerCharacter::setWasNoteRoom(ROOM_INDEX_DATA *room)
{
    this->was_note_room = room;
    return;
}

void PlayerCharacter::setClanCust(int value)
{
    this->clan_cust = value;
}

void PlayerCharacter::setJoin(Clan *clan)
{
    this->join = clan;
}

void PlayerCharacter::incrementPlayerKills(int amount)
{
    this->pkills += amount;
}

void PlayerCharacter::incrementMobKills(int amount)
{
    this->mkills += amount;
}

int PlayerCharacter::getAdrenaline()
{
    return this->adrenaline;
}

int PlayerCharacter::getPlayerKillCount()
{
    return this->pkills;
}

int PlayerCharacter::getKilledByPlayersCount()
{
    return this->pkilled;
}

int PlayerCharacter::getMobKillCount()
{
    return this->mkills;
}

int PlayerCharacter::getKilledByMobCount()
{
    return this->mkilled;
}

int PlayerCharacter::getRange()
{
    return this->range;
}

void PlayerCharacter::incrementRange(int amount)
{
    this->range += amount;
}

void PlayerCharacter::incrementKilledByPlayerCount(int amount)
{
    this->pkilled += amount;
}

void PlayerCharacter::setJustKilled(int tickTimer)
{
    this->jkilled = tickTimer;
}

void PlayerCharacter::incrementKilledByMobCount(int amount)
{
    this->mkilled += amount;
}

bool PlayerCharacter::didJustDie()
{
    return this->jkilled > 0;
}

void PlayerCharacter::writeToFile(FILE *fp, IRaceManager *race_manager)
{
    int sn, gn, pos, i;

    // If NPC - this should be #MOB
    fprintf(fp, "#PLAYER\n");

    fprintf(fp, "Name %s~\n", this->getName().c_str());
    fprintf(fp, "Id   %ld\n", this->id);
    fprintf(fp, "LogO %ld\n", current_time);
    fprintf(fp, "Vers %d\n", 5);
    if (this->short_descr[0] != '\0')
        fprintf(fp, "ShD  %s~\n", this->short_descr);
    if (this->long_descr[0] != '\0')
        fprintf(fp, "LnD  %s~\n", this->long_descr);
    if (this->getDescription())
        fprintf(fp, "Desc %s~\n", this->getDescription());
    if (this->prompt != NULL || !str_cmp(this->prompt, "<%X to level>%c<%h/%Hhp %m/%Mm %v/%Vmv> "))
        fprintf(fp, "Prom %s~\n", this->prompt);
    fprintf(fp, "Race %s~\n", this->getRace()->getPlayerRace()->getName().c_str());

    if (this->getRace() == race_manager->getRaceByName("werefolk"))
    {
        fprintf(fp, "O_fo %d\n", this->orig_form);
        fprintf(fp, "M_fo %d\n", this->morph_form);
    }

    if (this->getRace() == race_manager->getRaceByName("draconian") && this->level >= 10)
    {
        fprintf(fp, "Drac %d\n", this->drac);
        fprintf(fp, "Brea %d\n", this->breathe);
    }

    fprintf(fp, "Kills %d %d %d %d\n", this->pkills, this->pkilled,
            this->mkills, this->mkilled);
    if (this->isClanned())
        fprintf(fp, "Clan %s~\n", this->getClan()->getName().c_str());
    fprintf(fp, "Sex  %d\n", this->sex);
    fprintf(fp, "Cla  %d\n", this->class_num);
    fprintf(fp, "Levl %d\n", this->level);
    if (this->trust != 0)
        fprintf(fp, "Tru  %d\n", this->trust);
    fprintf(fp, "Plyd %d\n",
            this->played + (int)(current_time - this->logon));
    fprintf(fp, "Scro %d\n", this->lines);
    fprintf(fp, "Room %d\n",
            (this->in_room == get_room_index(ROOM_VNUM_LIMBO) && this->was_in_room != NULL)
                ? this->was_in_room->vnum
            : this->in_room == NULL ? 3001
                                    : this->in_room->vnum);

    fprintf(fp, "HMV  %d %d %d %d %d %d\n",
            this->hit, this->max_hit, this->mana, this->max_mana, this->move, this->max_move);
    if (this->gold > 0)
        fprintf(fp, "Gold %ld\n", this->gold);
    else
        fprintf(fp, "Gold %d\n", 0);
    if (this->silver > 0)
        fprintf(fp, "Silv %ld\n", this->silver);
    else
        fprintf(fp, "Silv %d\n", 0);
    fprintf(fp, "Exp  %d\n", this->exp);
    if (this->act != 0)
        fprintf(fp, "Act  %s\n", print_flags(this->act));
    if (this->affected_by != 0)
        fprintf(fp, "AfBy %s\n", print_flags(this->affected_by));
    if (this->reclass_num > 0)
        fprintf(fp, "Rnum %d\n", this->reclass_num);
    if (this->done != 0)
        fprintf(fp, "Done %s\n", print_flags(this->done));
    fprintf(fp, "Comm %s\n", print_flags(this->comm));
    if (this->wiznet)
        fprintf(fp, "Wizn %s\n", print_flags(this->wiznet));
    if (this->invis_level)
        fprintf(fp, "Invi %d\n", this->invis_level);
    if (this->incog_level)
        fprintf(fp, "Inco %d\n", this->incog_level);
    fprintf(fp, "Pos  %d\n",
            this->position == POS_FIGHTING ? POS_STANDING : this->position);
    if (this->practice != 0)
        fprintf(fp, "Prac %d\n", this->practice);
    if (this->train != 0)
        fprintf(fp, "Trai %d\n", this->train);
    if (this->saving_throw != 0)
        fprintf(fp, "Save  %d\n", this->saving_throw);
    fprintf(fp, "Alig  %d\n", this->alignment);
    if (this->hitroll != 0)
        fprintf(fp, "Hit   %d\n", this->hitroll);
    if (this->damroll != 0)
        fprintf(fp, "Dam   %d\n", this->damroll);
    fprintf(fp, "ACs %d %d %d %d\n",
            this->armor[0], this->armor[1], this->armor[2], this->armor[3]);
    if (this->wimpy != 0)
        fprintf(fp, "Wimp  %d\n", this->wimpy);
    fprintf(fp, "Attr %d %d %d %d %d\n",
            this->perm_stat[STAT_STR],
            this->perm_stat[STAT_INT],
            this->perm_stat[STAT_WIS],
            this->perm_stat[STAT_DEX],
            this->perm_stat[STAT_CON]);

    fprintf(fp, "AMod %d %d %d %d %d\n",
            this->mod_stat[STAT_STR],
            this->mod_stat[STAT_INT],
            this->mod_stat[STAT_WIS],
            this->mod_stat[STAT_DEX],
            this->mod_stat[STAT_CON]);

    //        if ( ch->isNPC() )
    //        {
    //                fprintf( fp, "Vnum %d\n",	this->pIndexData->vnum	);
    //        }
    //        else
    //        {
    fprintf(fp, "Pass %s~\n", this->getPassword().c_str());
    if (this->bamfin[0] != '\0')
        fprintf(fp, "Bin  %s~\n", this->bamfin);
    if (this->bamfout[0] != '\0')
        fprintf(fp, "Bout %s~\n", this->bamfout);
    fprintf(fp, "Titl %s~\n", this->title);

    if (IS_CLANNED(this))
        fprintf(fp, "Rang %d\n", this->range);

    fprintf(fp, "Pnts %d\n", this->points);
    fprintf(fp, "TSex %d\n", this->true_sex);
    fprintf(fp, "LLev %d\n", this->last_level);
    fprintf(fp, "HMVP %d %d %d\n", this->perm_hit,
            this->perm_mana,
            this->perm_move);
    fprintf(fp, "Cnd  %d %d %d %d\n",
            this->condition[0],
            this->condition[1],
            this->condition[2],
            this->condition[3]);

    /* write alias */
    for (pos = 0; pos < MAX_ALIAS; pos++)
    {
        if (this->alias[pos] == NULL || this->alias_sub[pos] == NULL)
            break;

        fprintf(fp, "Alias %s %s~\n", this->alias[pos],
                this->alias_sub[pos]);
    }

    /* Save note board status */
    /* Save number of boards in case that number changes */
    fprintf(fp, "Boards       %d ", MAX_BOARD);
    for (i = 0; i < MAX_BOARD; i++)
        fprintf(fp, "%s %ld ", boards[i].short_name, this->last_note_read.at(i));
    fprintf(fp, "\n");

    for (sn = 0; sn < MAX_SKILL; sn++)
    {
        if (skill_table[sn].name != NULL && this->learned[sn] > 0)
        {
            fprintf(fp, "Skill %d %d %d '%s'\n",
                    this->learned[sn], this->sk_level[sn],
                    this->sk_rating[sn], skill_table[sn].name);
        }
    }

    for (gn = 0; gn < MAX_GROUP; gn++)
    {
        if (group_table[gn].name != NULL && this->group_known[gn])
        {
            fprintf(fp, "Gr '%s'\n", group_table[gn].name);
        }
    }
    //        }

    for (auto paf : this->affected)
    {
        if (paf->type < 0 || paf->type >= MAX_SKILL)
            continue;

        fprintf(fp, "Affc '%s' %3d %3d %3d %3d %3d %10d\n",
                skill_table[paf->type].name,
                paf->where,
                paf->level,
                paf->duration,
                paf->modifier,
                paf->location,
                paf->bitvector);
    }

    fprintf(fp, "End\n\n");
    return;
}

void PlayerCharacter::setAdrenaline(int value)
{
    this->adrenaline = value;
}

bool PlayerCharacter::wantsToJoinClan(Clan *clan)
{
    return this->join == clan;
}

long PlayerCharacter::get_pc_id()
{
    static long last_pc_id;
    int val;

    val = (current_time <= last_pc_id) ? last_pc_id + 1 : current_time;
    last_pc_id = val;
    return val;
}

void PlayerCharacter::setKills(int pkills, int pkilled, int mkills, int mkilled)
{
    this->pkills = pkills;
    this->pkilled = pkilled;
    this->mkills = mkills;
    this->mkilled = mkilled;
}

void PlayerCharacter::setRange(int range)
{
    this->range = range;
}

void PlayerCharacter::update()
{
    if (this->adrenaline > 0)
        --this->adrenaline;

    if (this->jkilled > 0)
        --this->jkilled;

    if (this->level < LEVEL_IMMORTAL)
    {
        Object *obj;

        if ((obj = this->getEquipment(WEAR_LIGHT)) != NULL && obj->getItemType() == ITEM_LIGHT && obj->getValues().at(2) > 0)
        {
            if (--obj->getValues().at(2) == 0 && this->in_room != NULL)
            {
                --this->in_room->light;
                ::act("$p goes out.", this, obj, NULL, TO_ROOM, POS_RESTING);
                ::act("$p flickers and goes out.", this, obj, NULL, TO_CHAR, POS_RESTING);
                extract_obj(obj);
            }
            else if (obj->getValues().at(2) <= 5 && this->in_room != NULL)
                ::act("$p flickers.", this, obj, NULL, TO_CHAR, POS_RESTING);
        }

        if (++this->timer >= 12)
        {
            if (this->was_in_room == NULL && this->in_room != NULL)
            {
                this->was_in_room = this->in_room;
                if (this->fighting != NULL)
                    stop_fighting(this, TRUE);
                ::act("$n disappears into the void.",
                      this, NULL, NULL, TO_ROOM, POS_RESTING);
                send_to_char("You disappear into the void.\n\r", this);
                if (this->level > 1)
                    save_char_obj(this);
                char_from_room(this);
                char_to_room(this, get_room_index(ROOM_VNUM_LIMBO));
            }
        }

        this->gain_condition(COND_DRUNK, -1);
        this->gain_condition(COND_FULL, this->size > SIZE_MEDIUM ? -4 : -2);
        this->gain_condition(COND_THIRST, -1);
        this->gain_condition(COND_HUNGER, this->size > SIZE_MEDIUM ? -2 : -1);
    }
    Character::update();
}

/*
 * Regeneration stuff.
 */
int PlayerCharacter::hit_gain()
{
    if (this->in_room == NULL)
        return 0;

    int gain = UMAX(3, get_curr_stat(this, STAT_CON) - 3 + this->level / 2);
    gain += class_table[this->class_num].hp_max - 10;
    if (this->hit < this->max_hit)
    {
        int number = number_percent();
        if (number < get_skill(this, gsn_fast_healing))
        {
            gain += number * gain / 100;
        }
        check_improve(this, gsn_fast_healing, TRUE, 8);
    }

    switch (this->position)
    {
    default:
        gain /= 4;
        break;
    case POS_SLEEPING:
        break;
    case POS_RESTING:
        gain /= 2;
        break;
    case POS_FIGHTING:
        gain /= 6;
        break;
    }

    if (this->condition[COND_HUNGER] == 0)
        gain /= 2;

    if (this->condition[COND_THIRST] == 0)
        gain /= 2;

    gain = gain * this->in_room->heal_rate / 100;

    if (this->onObject() != NULL && this->onObject()->getItemType() == ITEM_FURNITURE)
        gain = gain * this->onObject()->getValues().at(3) / 100;

    if (IS_AFFECTED(this, AFF_POISON))
        gain /= 4;

    if (IS_AFFECTED(this, AFF_PLAGUE))
        gain /= 8;

    if (IS_AFFECTED(this, AFF_HASTE) || IS_AFFECTED(this, AFF_SLOW))
        gain /= 2;

    if (this->isClanned())
        gain *= 1.5;

    return UMIN(gain, this->max_hit - this->hit);
}

int PlayerCharacter::mana_gain()
{
    int gain;

    if (this->in_room == NULL)
        return 0;

    gain = (get_curr_stat(this, STAT_WIS) + get_curr_stat(this, STAT_INT) + this->level) / 2;
    if (this->mana < this->max_mana)
    {
        int number = number_percent();
        if (number < get_skill(this, gsn_meditation))
        {
            gain += number * gain / 100;
        }
        check_improve(this, gsn_meditation, TRUE, 8);
    }
    if (!class_table[this->class_num].fMana)
        gain /= 2;

    switch (this->position)
    {
    default:
        gain /= 4;
        break;
    case POS_SLEEPING:
        break;
    case POS_RESTING:
        gain /= 2;
        break;
    case POS_FIGHTING:
        gain /= 6;
        break;
    }

    if (this->condition[COND_HUNGER] == 0)
        gain /= 2;

    if (this->condition[COND_THIRST] == 0)
        gain /= 2;

    gain = gain * this->in_room->mana_rate / 100;

    if (this->onObject() != NULL && this->onObject()->getItemType() == ITEM_FURNITURE)
        gain = gain * this->onObject()->getValues().at(4) / 100;

    if (IS_AFFECTED(this, AFF_POISON))
        gain /= 4;

    if (IS_AFFECTED(this, AFF_PLAGUE))
        gain /= 8;

    if (IS_AFFECTED(this, AFF_HASTE) || IS_AFFECTED(this, AFF_SLOW))
        gain /= 2;

    if (this->isClanned())
        gain *= 1.5;

    return UMIN(gain, this->max_mana - this->mana);
}

int PlayerCharacter::move_gain()
{
    int gain;

    if (this->in_room == NULL)
        return 0;

    gain = UMAX(15, this->level);

    switch (this->position)
    {
    case POS_SLEEPING:
        gain += get_curr_stat(this, STAT_DEX);
        break;
    case POS_RESTING:
        gain += get_curr_stat(this, STAT_DEX) / 2;
        break;
    }

    if (this->condition[COND_HUNGER] == 0)
        gain /= 2;

    if (this->condition[COND_THIRST] == 0)
        gain /= 2;

    gain = gain * this->in_room->heal_rate / 100;

    if (this->onObject() != NULL && this->onObject()->getItemType() == ITEM_FURNITURE)
        gain = gain * this->onObject()->getValues().at(3) / 100;

    if (IS_AFFECTED(this, AFF_POISON))
        gain /= 4;

    if (IS_AFFECTED(this, AFF_PLAGUE))
        gain /= 8;

    if (IS_AFFECTED(this, AFF_HASTE) || IS_AFFECTED(this, AFF_SLOW))
        gain /= 2;

    if (this->isClanned())
        gain *= 1.5;

    return UMIN(gain, this->max_move - this->move);
}

void PlayerCharacter::perform_autoloot()
{
    Object *corpse = NULL;

    if ((corpse = ObjectHelper::findInList(this, (char *)"corpse", this->in_room->contents)) != NULL && corpse->getItemType() == ITEM_CORPSE_NPC && this->can_see(corpse))
    {
        Object *coins;

        corpse = ObjectHelper::findInList(this, (char *)"corpse", this->in_room->contents);

        if (IS_SET(this->act, PLR_AUTOLOOT) &&
            corpse && !corpse->getContents().empty()) /* exists and not empty */
            do_get(this, (char *)"all corpse");

        if (IS_SET(this->act, PLR_AUTOGOLD) &&
            corpse && !corpse->getContents().empty() && /* exists and not empty */
            !IS_SET(this->act, PLR_AUTOLOOT))
            if ((coins = ObjectHelper::findInList(this, (char *)"gcash", corpse->getContents())) != NULL)
                do_get(this, (char *)"all.gcash corpse");

        if (IS_SET(this->act, PLR_AUTOSAC))
        {
            if (IS_SET(this->act, PLR_AUTOLOOT) && corpse && !corpse->getContents().empty())
            {
                return; /* leave if corpse has treasure */
            }
            else
            {
                do_sacrifice(this, (char *)"corpse");
            }
        }
    }
}

bool PlayerCharacter::isClanned() { return _clan != NULL; }
Clan *PlayerCharacter::getClan() { return _clan; }
void PlayerCharacter::setClan(Clan *value) { _clan = value; }

/*
 * Advancement stuff.
 */
void PlayerCharacter::advance_level( bool hide )
{
    char buf[MAX_STRING_LENGTH];
    int add_hp, add_mana, add_move, dracnum, add_prac;

    this->last_level = ( this->played + (int) (current_time - this->logon) ) / 3600;

    snprintf(buf, sizeof(buf), "the %s",
    title_table [this->class_num] [this->level] [this->sex == SEX_FEMALE ? 1 : 0] );
    set_title( this, buf );

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

    // high exp characters get a cumulative bonus
    add_hp += this->exp / 10000;
    add_mana += this->exp / 10000;
    add_move += this->exp / 10000;

    add_hp = add_hp * 9/10;
    add_mana = add_mana * 9/10;
    add_move = add_move * 9/10;

    add_hp  = UMAX(  2, add_hp   );
    add_mana    = UMAX(  2, add_mana );
    add_move    = UMAX(  6, add_move );

    this->max_hit     += add_hp;
    this->max_mana    += add_mana;
    this->max_move    += add_move;
    // Refresh your health
    this->hit          = UMAX(this->hit, this->max_hit);
    this->mana          = UMAX(this->mana, this->max_mana);
    this->move          = UMAX(this->move, this->max_move);
    this->practice    += add_prac;
    this->train       += 1;

    this->perm_hit    += add_hp;
    this->perm_mana   += add_mana;
    this->perm_move   += add_move;

    for (int sn = 0; sn < MAX_SKILL; sn++) {
        if (skill_table[sn].name == NULL)
            break;

        if (this->learned[sn] > 0
            && this->learned[sn] < 100
            && this->sk_level[sn] < this->level ) {
            this->learned[sn]++;
        }
    }

    if (this->level == 10 && (this->getRace()->getName() == "draconian"))
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
    snprintf(buf, sizeof(buf), "You scream in agony as %s scales pierce your tender skin!\n\r", draconian_table[this->drac].colour);
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
        snprintf(buf, sizeof(buf),
        "{BYou have leveled!{x\n\r"
        "You gain %d hit point%s, %d mana, %d move, and %d practice%s.\n\r",
        add_hp, add_hp == 1 ? "" : "s", add_mana, add_move,
        add_prac, add_prac == 1 ? "" : "s");
    send_to_char( buf, this );
    }
    return;
}

void PlayerCharacter::gain_condition( int iCond, int value )
{
    int condition;

    if ( value == 0 || IS_NPC(this) || this->level >= LEVEL_IMMORTAL)
    return;

    condition               = this->condition[iCond];
    if (condition == -1)
        return;
    this->condition[iCond]    = URANGE( 0, condition + value, 48 );

    if ( this->condition[iCond] == 0 )
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

std::string PlayerCharacter::getPassword() { return _password; }
void PlayerCharacter::setPassword(std::string value) { _password = value; }