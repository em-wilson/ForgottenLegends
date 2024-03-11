/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
 *  Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
 *                                                                         *
 *  Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael          *
 *  Chastain, Michael Quan, and Mitchell Tse.                              *
 *                                                                         *
 *  In order to use any part of this Merc Diku Mud, you must comply with   *
 *  both the original Diku license in 'license.doc' as well the Merc       *
 *  license in 'license.txt'.  In particular, you may not remove either of *
 *  these copyright notices.                                               *
 *                                                                         *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 ***************************************************************************/

/***************************************************************************
 *	ROM 2.4 is copyright 1993-1996 Russ Taylor							   *
 *	ROM has been brought to you by the ROM consortium					   *
 *	    Russ Taylor (rtaylor@efn.org)									   *
 *	    Gabrielle Taylor												   *
 *	    Brian Moore (zump@rom.org)										   *
 *	By using this code, you have agreed to follow the terms of the	 	   *
 *	ROM license, in the file Rom24/doc/rom.license						   *
 ***************************************************************************/

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include "merc.h"
#include "board.h"
#include "recycle.h"
#include "clans/ClanManager.h"
#include "ExtraDescription.h"
#include "ILogger.h"
#include "NonPlayerCharacter.h"
#include "Object.h"
#include "ObjectHelper.h"
#include "PlayerRace.h"
#include "PlayerCharacter.h"
#include "RaceManager.h"
#include "StringHelper.h"

extern ClanManager *clan_manager;
extern RaceManager *race_manager;

extern int _filbuf args((FILE *));
int rename(const char *oldfname, const char *newfname);

char *print_flags(int flag)
{
	int count, pos = 0;
	static char buf[52];

	for (count = 0; count < 32; count++)
	{
		if (IS_SET(flag, 1 << count))
		{
			if (count < 26)
				buf[pos] = 'A' + count;
			else
				buf[pos] = 'a' + (count - 26);
			pos++;
		}
	}

	if (pos == 0)
	{
		buf[pos] = '0';
		pos++;
	}

	buf[pos] = '\0';

	return buf;
}

/*
 * Array of containers read for proper re-nesting of objects.
 */
#define MAX_NEST 100
static Object *rgObjNest[MAX_NEST];

/*
 * Local functions.
 */
void fwrite_obj args((Character * ch, Object *obj,
					  FILE *fp, int iNest));
void fwrite_pet args((Character * pet, FILE *fp));
void fread_char args((Character * ch, FILE *fp));
void fread_pet args((Character * ch, FILE *fp));
void fread_obj args((Character * ch, FILE *fp));

/*
 * Save a character and inventory.
 * Would be cool to save NPC's too for quest purposes,
 *   some of the infrastructure is provided.
 */
void save_char_obj(Character *ch)
{
	char strsave[MAX_INPUT_LENGTH];
	FILE *fp;

	if (IS_NPC(ch))
		return;

	if (ch->desc != NULL && ch->desc->original != NULL)
		ch = ch->desc->original;

	/* create god log */
	if (IS_IMMORTAL(ch) || ch->level >= LEVEL_IMMORTAL)
	{
		fclose(fpReserve);
		snprintf(strsave, sizeof(strsave), "%s%s", GOD_DIR, StringHelper::capitalize(ch->getName()).c_str());
		if ((fp = fopen(strsave, "w")) == NULL)
		{
			bug("Save_char_obj: fopen", 0);
			perror(strsave);
		}

		fprintf(fp, "Lev %2d Trust %2d  %s%s\n",
				ch->level, ch->getTrust(), ch->getName().c_str(), ch->pcdata->title);
		fclose(fp);
		fpReserve = fopen(NULL_FILE, "r");
	}

	fclose(fpReserve);
	snprintf(strsave, sizeof(strsave), "%s%s", PLAYER_DIR, StringHelper::capitalize(ch->getName()).c_str());
	if ((fp = fopen(TEMP_FILE, "w")) == NULL)
	{
		bug("Save_char_obj: fopen", 0);
		perror(strsave);
	}
	else
	{
		((PlayerCharacter *)ch)->writeToFile(fp);
		for (auto carrying : ch->getCarrying())
		{
			fwrite_obj(ch, carrying, fp, 0);
		}
		/* save the pets */
		if (ch->pet != NULL && ch->pet->in_room == ch->in_room)
		{
			fwrite_pet(ch->pet, fp);
		}
		fprintf(fp, "#END\n");
	}
	fclose(fp);
	rename(TEMP_FILE, strsave);
	fpReserve = fopen(NULL_FILE, "r");
	return;
}

/* write a pet */
void fwrite_pet(Character *pet, FILE *fp)
{
	AFFECT_DATA *paf;

	fprintf(fp, "#PET\n");

	fprintf(fp, "Vnum %d\n", pet->pIndexData->vnum);

	fprintf(fp, "Name %s~\n", pet->getName().c_str());
	fprintf(fp, "LogO %ld\n", current_time);
	if (pet->short_descr != pet->pIndexData->short_descr)
		fprintf(fp, "ShD  %s~\n", pet->short_descr);
	if (pet->long_descr != pet->pIndexData->long_descr)
		fprintf(fp, "LnD  %s~\n", pet->long_descr);
	if (pet->getDescription() && pet->getDescription() != pet->pIndexData->description)
		fprintf(fp, "Desc %s~\n", pet->getDescription());
	if (pet->getRace()->getLegacyId() != pet->pIndexData->race)
		fprintf(fp, "Race %s~\n", pet->getRace()->getName().c_str());
	fprintf(fp, "Sex  %d\n", pet->sex);
	if (pet->level != pet->pIndexData->level)
		fprintf(fp, "Levl %d\n", pet->level);
	fprintf(fp, "HMV  %d %d %d %d %d %d\n",
			pet->hit, pet->max_hit, pet->mana, pet->max_mana, pet->move, pet->max_move);
	if (pet->gold > 0)
		fprintf(fp, "Gold %ld\n", pet->gold);
	if (pet->silver > 0)
		fprintf(fp, "Silv %ld\n", pet->silver);
	if (pet->exp > 0)
		fprintf(fp, "Exp  %d\n", pet->exp);
	if (pet->act != pet->pIndexData->act)
		fprintf(fp, "Act  %s\n", print_flags(pet->act));
	if (pet->affected_by != pet->pIndexData->affected_by)
		fprintf(fp, "AfBy %s\n", print_flags(pet->affected_by));
	if (pet->comm != 0)
		fprintf(fp, "Comm %s\n", print_flags(pet->comm));
	fprintf(fp, "Pos  %d\n", pet->position = POS_FIGHTING ? POS_STANDING : pet->position);
	if (pet->saving_throw != 0)
		fprintf(fp, "Save %d\n", pet->saving_throw);
	if (pet->alignment != pet->pIndexData->alignment)
		fprintf(fp, "Alig %d\n", pet->alignment);
	if (pet->hitroll != pet->pIndexData->hitroll)
		fprintf(fp, "Hit  %d\n", pet->hitroll);
	if (pet->damroll != pet->pIndexData->damage[DICE_BONUS])
		fprintf(fp, "Dam  %d\n", pet->damroll);
	fprintf(fp, "ACs  %d %d %d %d\n",
			pet->armor[0], pet->armor[1], pet->armor[2], pet->armor[3]);
	fprintf(fp, "Attr %d %d %d %d %d\n",
			pet->perm_stat[STAT_STR], pet->perm_stat[STAT_INT],
			pet->perm_stat[STAT_WIS], pet->perm_stat[STAT_DEX],
			pet->perm_stat[STAT_CON]);
	fprintf(fp, "AMod %d %d %d %d %d\n",
			pet->mod_stat[STAT_STR], pet->mod_stat[STAT_INT],
			pet->mod_stat[STAT_WIS], pet->mod_stat[STAT_DEX],
			pet->mod_stat[STAT_CON]);

	for (auto paf : pet->affected)
	{
		if (paf->type < 0 || paf->type >= MAX_SKILL)
			continue;

		fprintf(fp, "Affc '%s' %3d %3d %3d %3d %3d %10d\n",
				skill_table[paf->type].name,
				paf->where, paf->level, paf->duration, paf->modifier, paf->location,
				paf->bitvector);
	}

	fprintf(fp, "End\n");
	return;
}

/*
 * Write an object and its contents.
 */
void fwrite_obj(Character *ch, Object *obj, FILE *fp, int iNest)
{
	ExtraDescription *ed;
	AFFECT_DATA *paf;

	/*
	 * Castrate storage characters.
	 */
	if ((ch->level < obj->getLevel() - 2 && obj->getItemType() != ITEM_CONTAINER) || obj->getItemType() == ITEM_KEY || (obj->getItemType() == ITEM_MAP && !obj->getValues().at(0)))
		return;

	fprintf(fp, "#O\n");
	fprintf(fp, "Vnum %d\n", obj->getObjectIndexData()->vnum);
	if (!obj->getObjectIndexData()->new_format)
		fprintf(fp, "Oldstyle\n");
	if (obj->isEnchanted())
		fprintf(fp, "Enchanted\n");
	fprintf(fp, "Nest %d\n", iNest);

	/* these data are only used if they do not match the defaults */

	if (obj->getName() != obj->getObjectIndexData()->name)
		fprintf(fp, "Name %s~\n", obj->getName().c_str());
	if (obj->getShortDescription() != obj->getObjectIndexData()->short_descr)
		fprintf(fp, "ShD  %s~\n", obj->getShortDescription().c_str());
	if (obj->getDescription() != obj->getObjectIndexData()->description)
		fprintf(fp, "Desc %s~\n", obj->getDescription().c_str());
	if (obj->getExtraFlags() != obj->getObjectIndexData()->extra_flags)
		fprintf(fp, "ExtF %d\n", obj->getExtraFlags());
	if (obj->getWearFlags() != obj->getObjectIndexData()->wear_flags)
		fprintf(fp, "WeaF %d\n", obj->getWearFlags());
	if (obj->getItemType() != obj->getObjectIndexData()->item_type)
		fprintf(fp, "Ityp %d\n", obj->getItemType());
	if (obj->getWeight() != obj->getObjectIndexData()->weight)
		fprintf(fp, "Wt   %d\n", obj->getWeight());
	if (obj->getCondition() != obj->getObjectIndexData()->condition)
		fprintf(fp, "Cond %d\n", obj->getCondition());

	/* variable data */

	fprintf(fp, "Wear %d\n", obj->getWearLocation());
	if (obj->getLevel() != obj->getObjectIndexData()->level)
		fprintf(fp, "Lev  %d\n", obj->getLevel());
	if (obj->getTimer() != 0)
		fprintf(fp, "Time %d\n", obj->getTimer());
	fprintf(fp, "Cost %d\n", obj->getCost());
	if (obj->getValues().at(0) != obj->getObjectIndexData()->value[0] || obj->getValues().at(1) != obj->getObjectIndexData()->value[1] || obj->getValues().at(2) != obj->getObjectIndexData()->value[2] || obj->getValues().at(3) != obj->getObjectIndexData()->value[3] || obj->getValues().at(4) != obj->getObjectIndexData()->value[4])
		fprintf(fp, "Val  %d %d %d %d %d\n",
				obj->getValues().at(0), obj->getValues().at(1), obj->getValues().at(2), obj->getValues().at(3),
				obj->getValues().at(4));

	switch (obj->getItemType())
	{
	case ITEM_POTION:
	case ITEM_SCROLL:
	case ITEM_PILL:
		if (obj->getValues().at(1) > 0)
		{
			fprintf(fp, "Spell 1 '%s'\n",
					skill_table[obj->getValues().at(1)].name);
		}

		if (obj->getValues().at(2) > 0)
		{
			fprintf(fp, "Spell 2 '%s'\n",
					skill_table[obj->getValues().at(2)].name);
		}

		if (obj->getValues().at(3) > 0)
		{
			fprintf(fp, "Spell 3 '%s'\n",
					skill_table[obj->getValues().at(3)].name);
		}

		break;

	case ITEM_STAFF:
	case ITEM_WAND:
		if (obj->getValues().at(3) > 0)
		{
			fprintf(fp, "Spell 3 '%s'\n",
					skill_table[obj->getValues().at(3)].name);
		}

		break;
	}

	for (auto paf : obj->getAffectedBy())
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

	for (auto ed : obj->getExtraDescriptions())
	{
		fprintf(fp, "ExDe %s~ %s~\n",
				ed->getKeyword().c_str(), ed->getDescription().c_str());
	}

	fprintf(fp, "End\n\n");

	for (auto contains : obj->getContents())
	{
		fwrite_obj(ch, contains, fp, iNest + 1);
	}

	return;
}

/*
 * Load a char and inventory into a new ch structure.
 */
bool load_char_obj(DESCRIPTOR_DATA *d, char *name)
{
	char strsave[MAX_INPUT_LENGTH];
	char buf[100];
	PlayerCharacter *ch = new PlayerCharacter();
	FILE *fp;
	bool found;

	d->character = ch;
	ch->desc = d;
	ch->setName(name);

	found = FALSE;
	fclose(fpReserve);

	/* decompress if .gz file exists */
	snprintf(strsave, sizeof(strsave), "%s%s%s", PLAYER_DIR, capitalize(name), ".gz");
	if ((fp = fopen(strsave, "r")) != NULL)
	{
		fclose(fp);
		snprintf(buf, sizeof(buf), "gzip -dfq %s", strsave);
		system(buf);
	}

	snprintf(strsave, sizeof(strsave), "%s%s", PLAYER_DIR, capitalize(name));
	if ((fp = fopen(strsave, "r")) != NULL)
	{
		int iNest;

		for (iNest = 0; iNest < MAX_NEST; iNest++)
			rgObjNest[iNest] = NULL;

		found = TRUE;
		for (;;)
		{
			char letter;
			char *word;

			letter = fread_letter(fp);
			if (letter == '*')
			{
				fread_to_eol(fp);
				continue;
			}

			if (letter != '#')
			{
				bug("Load_char_obj: # not found.", 0);
				break;
			}

			word = fread_word(fp);
			if (!str_cmp(word, "PLAYER"))
				fread_char(ch, fp);
			else if (!str_cmp(word, "OBJECT"))
				fread_obj(ch, fp);
			else if (!str_cmp(word, "O"))
				fread_obj(ch, fp);
			else if (!str_cmp(word, "PET"))
				fread_pet(ch, fp);
			else if (!str_cmp(word, "END"))
				break;
			else
			{
				bug("Load_char_obj: bad section.", 0);
				break;
			}
		}
		fclose(fp);
	}

	fpReserve = fopen(NULL_FILE, "r");

	/* initialize race */
	if (found)
	{
		int i;

		if (!ch->getRace())
		{
			ch->setRace(race_manager->getRaceByName("human"));
		}

		ch->size = ch->getRace()->getPlayerRace()->getSize();
		ch->dam_type = 17; /*punch */

		for (auto skill : ch->getRace()->getPlayerRace()->getSkills())
		{
			group_add(ch, skill.c_str(), FALSE);
		}
		ch->affected_by = ch->affected_by | ch->getRace()->getAffectFlags();
		ch->imm_flags = ch->imm_flags | ch->getRace()->getImmunityFlags();
		ch->res_flags = ch->res_flags | ch->getRace()->getResistanceFlags();
		ch->vuln_flags = ch->vuln_flags | ch->getRace()->getVulnerabilityFlags();
		ch->form = ch->getRace()->getForm();
		ch->parts = ch->getRace()->getParts();
	}

	/* RT initialize skills */

	if (found && ch->version < 2) /* need to add the new skills */
	{
		group_add(ch, "rom basics", FALSE);
		group_add(ch, class_table[ch->class_num].base_group, FALSE);
		group_add(ch, class_table[ch->class_num].default_group, TRUE);
		ch->pcdata->learned[gsn_recall] = 50;
	}

	/* fix levels */
	if (found && ch->version < 3 && (ch->level > 35 || ch->trust > 35))
	{
		switch (ch->level)
		{
		case (40):
			ch->level = 60;
			break; /* imp -> imp */
		case (39):
			ch->level = 58;
			break; /* god -> supreme */
		case (38):
			ch->level = 56;
			break; /* deity -> god */
		case (37):
			ch->level = 53;
			break; /* angel -> demigod */
		}

		switch (ch->trust)
		{
		case (40):
			ch->trust = 60;
			break; /* imp -> imp */
		case (39):
			ch->trust = 58;
			break; /* god -> supreme */
		case (38):
			ch->trust = 56;
			break; /* deity -> god */
		case (37):
			ch->trust = 53;
			break; /* angel -> demigod */
		case (36):
			ch->trust = 51;
			break; /* hero -> hero */
		}
	}

	/* ream gold */
	if (found && ch->version < 4)
	{
		ch->gold /= 100;
	}
	return found;
}

/*
 * Read in a char.
 */

#if defined(KEY)
#undef KEY
#endif

#define KEY(literal, field, value) \
	if (!str_cmp(word, literal))   \
	{                              \
		field = value;             \
		fMatch = TRUE;             \
		break;                     \
	}

/* give to a function */
#if defined(KEYF)
#undef KEYF
#endif

#define KEYF(literal, fn, value) \
	if (!str_cmp(word, literal)) \
	{                            \
		fn(value);               \
		fMatch = TRUE;           \
		break;                   \
	}

/* provided to free strings */
#if defined(KEYS)
#undef KEYS
#endif

#define KEYS(literal, field, value) \
	if (!str_cmp(word, literal))    \
	{                               \
		free_string(field);         \
		field = value;              \
		fMatch = TRUE;              \
		break;                      \
	}

/* set a string variable in a class */
#if defined(CKEYS)
#undef CKEYS
#endif

#define CKEYS(literal, func, value) \
	if (!str_cmp(word, literal))    \
	{                               \
		char *val = value;          \
		func(val);                  \
		free_string(val);           \
		fMatch = TRUE;              \
		break;                      \
	}

void fread_char(Character *ch, FILE *fp)
{
	char buf[MAX_STRING_LENGTH];
	char *word;
	bool fMatch;
	int count = 0;
	int lastlogoff = current_time;
	int percent;

	snprintf(buf, sizeof(buf), "Loading %s.", ch->getName().c_str());
	logger->log_string(buf);

	for (;;)
	{
		word = feof(fp) ? (char *)"End" : fread_word(fp);
		fMatch = FALSE;

		switch (UPPER(word[0]))
		{
		case '*':
			fMatch = TRUE;
			fread_to_eol(fp);
			break;

		case 'A':
			KEY("Act", ch->act, fread_flag(fp));
			KEY("AffectedBy", ch->affected_by, fread_flag(fp));
			KEY("AfBy", ch->affected_by, fread_flag(fp));
			KEY("Alignment", ch->alignment, fread_number(fp));
			KEY("Alig", ch->alignment, fread_number(fp));

			if (!str_cmp(word, "Alia"))
			{
				if (count >= MAX_ALIAS)
				{
					fread_to_eol(fp);
					fMatch = TRUE;
					break;
				}

				ch->pcdata->alias[count] = str_dup(fread_word(fp));
				ch->pcdata->alias_sub[count] = str_dup(fread_word(fp));
				count++;
				fMatch = TRUE;
				break;
			}

			if (!str_cmp(word, "Alias"))
			{
				if (count >= MAX_ALIAS)
				{
					fread_to_eol(fp);
					fMatch = TRUE;
					break;
				}

				ch->pcdata->alias[count] = str_dup(fread_word(fp));
				ch->pcdata->alias_sub[count] = fread_string(fp);
				count++;
				fMatch = TRUE;
				break;
			}

			if (!str_cmp(word, "AC") || !str_cmp(word, "Armor"))
			{
				fread_to_eol(fp);
				fMatch = TRUE;
				break;
			}

			if (!str_cmp(word, "ACs"))
			{
				int i;

				for (i = 0; i < 4; i++)
					ch->armor[i] = fread_number(fp);
				fMatch = TRUE;
				break;
			}

			if (!str_cmp(word, "AffD"))
			{
				AFFECT_DATA *paf;
				int sn;

				paf = new_affect();

				sn = skill_lookup(fread_word(fp));
				if (sn < 0)
					bug("Fread_char: unknown skill.", 0);
				else
					paf->type = sn;

				paf->level = fread_number(fp);
				paf->duration = fread_number(fp);
				paf->modifier = fread_number(fp);
				paf->location = fread_number(fp);
				paf->bitvector = fread_number(fp);
				ch->affected.push_back(paf);
				fMatch = TRUE;
				break;
			}

			if (!str_cmp(word, "Affc"))
			{
				AFFECT_DATA *paf;
				int sn;

				paf = new_affect();

				sn = skill_lookup(fread_word(fp));
				if (sn < 0)
					bug("Fread_char: unknown skill.", 0);
				else
					paf->type = sn;

				paf->where = fread_number(fp);
				paf->level = fread_number(fp);
				paf->duration = fread_number(fp);
				paf->modifier = fread_number(fp);
				paf->location = fread_number(fp);
				paf->bitvector = fread_number(fp);
				ch->affected.push_back(paf);
				fMatch = TRUE;
				break;
			}

			if (!str_cmp(word, "AttrMod") || !str_cmp(word, "AMod"))
			{
				int stat;
				for (stat = 0; stat < MAX_STATS; stat++)
					ch->mod_stat[stat] = fread_number(fp);
				fMatch = TRUE;
				break;
			}

			if (!str_cmp(word, "AttrPerm") || !str_cmp(word, "Attr"))
			{
				int stat;

				for (stat = 0; stat < MAX_STATS; stat++)
					ch->perm_stat[stat] = fread_number(fp);
				fMatch = TRUE;
				break;
			}
			break;

		case 'B':
			KEY("Bamfin", ch->pcdata->bamfin, fread_string(fp));
			KEY("Bamfout", ch->pcdata->bamfout, fread_string(fp));
			KEY("Bin", ch->pcdata->bamfin, fread_string(fp));
			KEY("Bout", ch->pcdata->bamfout, fread_string(fp));
			KEY("Brea", ch->breathe, fread_number(fp));

			/* Read in board status */
			if (!str_cmp(word, "Boards"))
			{
				int i, num = fread_number(fp); /* number of boards saved */
				char *boardname;

				for (; num; num--) /* for each of the board saved */
				{
					boardname = fread_word(fp);
					i = board_lookup(boardname); /* find board number */

					if (i == BOARD_NOTFOUND) /* Does board still exist ? */
					{
						snprintf(buf, sizeof(buf), "fread_char: %s had unknown board name: %s. Skipped.",
								 ch->getName().c_str(), boardname);
						logger->log_string(buf);
						fread_number(fp); /* read last_note and skip info */
					}
					else /* Save it */
						ch->pcdata->last_note_read.emplace(i, fread_number(fp));
				} /* for */

				fMatch = TRUE;
			} /* Boards */
			break;

		case 'C':
			KEY("Class", ch->class_num, fread_number(fp));
			KEY("Cla", ch->class_num, fread_number(fp));
			KEY("Clan", ch->pcdata->clan, clan_manager->get_clan(fread_string(fp)));

			if (!str_cmp(word, "Condition") || !str_cmp(word, "Cond"))
			{
				ch->pcdata->condition[0] = fread_number(fp);
				ch->pcdata->condition[1] = fread_number(fp);
				ch->pcdata->condition[2] = fread_number(fp);
				fMatch = TRUE;
				break;
			}
			if (!str_cmp(word, "Cnd"))
			{
				ch->pcdata->condition[0] = fread_number(fp);
				ch->pcdata->condition[1] = fread_number(fp);
				ch->pcdata->condition[2] = fread_number(fp);
				ch->pcdata->condition[3] = fread_number(fp);
				fMatch = TRUE;
				break;
			}
			KEY("Comm", ch->comm, fread_flag(fp));

			break;

		case 'D':
			KEY("Damroll", ch->damroll, fread_number(fp));
			KEY("Dam", ch->damroll, fread_number(fp));
			CKEYS("Description", ch->setDescription, fread_string(fp));
			CKEYS("Desc", ch->setDescription, fread_string(fp));
			KEY("Done", ch->done, fread_flag(fp));
			KEY("Drac", ch->drac, fread_number(fp));
			break;

		case 'E':
			if (!str_cmp(word, "End"))
			{
				/* adjust hp mana move up  -- here for speed's sake */
				percent = (current_time - lastlogoff) * 25 / (2 * 60 * 60);

				percent = UMIN(percent, 100);

				if (percent > 0 && !IS_AFFECTED(ch, AFF_POISON) && !IS_AFFECTED(ch, AFF_PLAGUE))
				{
					ch->hit += (ch->max_hit - ch->hit) * percent / 100;
					ch->mana += (ch->max_mana - ch->mana) * percent / 100;
					ch->move += (ch->max_move - ch->move) * percent / 100;
				}
				return;
			}
			KEY("Exp", ch->exp, fread_number(fp));
			break;

		case 'G':
			KEY("Gold", ch->gold, fread_number(fp));
			if (!str_cmp(word, "Group") || !str_cmp(word, "Gr"))
			{
				int gn;
				char *temp;

				temp = fread_word(fp);
				gn = group_lookup(temp);
				/* gn    = group_lookup( fread_word( fp ) ); */
				if (gn < 0)
				{
					fprintf(stderr, "%s", temp);
					bug("Fread_char: unknown group. ", 0);
				}
				else
					gn_add(ch, gn);
				fMatch = TRUE;
			}
			break;

		case 'H':
			KEY("Hitroll", ch->hitroll, fread_number(fp));
			KEY("Hit", ch->hitroll, fread_number(fp));

			if (!str_cmp(word, "HpManaMove") || !str_cmp(word, "HMV"))
			{
				ch->hit = fread_number(fp);
				ch->max_hit = fread_number(fp);
				ch->mana = fread_number(fp);
				ch->max_mana = fread_number(fp);
				ch->move = fread_number(fp);
				ch->max_move = fread_number(fp);
				fMatch = TRUE;
				break;
			}

			if (!str_cmp(word, "HpManaMovePerm") || !str_cmp(word, "HMVP"))
			{
				ch->pcdata->perm_hit = fread_number(fp);
				ch->pcdata->perm_mana = fread_number(fp);
				ch->pcdata->perm_move = fread_number(fp);
				fMatch = TRUE;
				break;
			}

			break;

		case 'I':
			KEY("Id", ch->id, fread_number(fp));
			KEY("InvisLevel", ch->invis_level, fread_number(fp));
			KEY("Inco", ch->incog_level, fread_number(fp));
			KEY("Invi", ch->invis_level, fread_number(fp));
			break;

		case 'K':
			if (!ch->isNPC() && !str_cmp(word, "Kills"))
			{
				fMatch = TRUE;
				int pkills = fread_number(fp);
				int pkilled = fread_number(fp);
				int mkills = fread_number(fp);
				int mkilled = fread_number(fp);
				((PlayerCharacter *)ch)->setKills(pkills, pkilled, mkills, mkilled);
			}
			break;

		case 'L':
			KEY("LastLevel", ch->pcdata->last_level, fread_number(fp));
			KEY("LLev", ch->pcdata->last_level, fread_number(fp));
			KEY("Level", ch->level, fread_number(fp));
			KEY("Lev", ch->level, fread_number(fp));
			KEY("Levl", ch->level, fread_number(fp));
			KEY("LogO", lastlogoff, fread_number(fp));
			KEY("LongDescr", ch->long_descr, fread_string(fp));
			KEY("LnD", ch->long_descr, fread_string(fp));
			break;

		case 'M':
			KEY("M_fo", ch->morph_form, fread_number(fp));
			break;

		case 'N':
			CKEYS("Name", ch->setName, fread_string(fp));
			break;

		case 'O':
			KEY("O_fo", ch->orig_form, fread_number(fp));
			break;

		case 'P':
			KEYF("Password", ch->pcdata->setPassword, fread_string(fp));
			KEYF("Pass", ch->pcdata->setPassword, fread_string(fp));
			KEY("Played", ch->played, fread_number(fp));
			KEY("Plyd", ch->played, fread_number(fp));
			KEY("Points", ch->pcdata->points, fread_number(fp));
			KEY("Pnts", ch->pcdata->points, fread_number(fp));
			KEY("Position", ch->position, fread_number(fp));
			KEY("Pos", ch->position, fread_number(fp));
			KEY("Practice", ch->practice, fread_number(fp));
			KEY("Prac", ch->practice, fread_number(fp));
			KEYS("Prompt", ch->prompt, fread_string(fp));
			KEY("Prom", ch->prompt, fread_string(fp));
			break;

		case 'R':
			KEYF("Race", ch->setRace, race_manager->getRaceByName(fread_string(fp)));
			KEY("Rnum", ch->reclass_num, fread_number(fp));

			if (!str_cmp(word, "Range"))
			{
				((PlayerCharacter *)ch)->setRange(fread_number(fp));
			}

			if (!str_cmp(word, "Room"))
			{
				ch->in_room = get_room_index(fread_number(fp));
				if (ch->in_room == NULL)
					ch->in_room = get_room_index(ROOM_VNUM_LIMBO);
				fMatch = TRUE;
				break;
			}

			break;

		case 'S':
			KEY("SavingThrow", ch->saving_throw, fread_number(fp));
			KEY("Save", ch->saving_throw, fread_number(fp));
			KEY("Scro", ch->lines, fread_number(fp));
			KEY("Sex", ch->sex, fread_number(fp));
			KEY("ShortDescr", ch->short_descr, fread_string(fp));
			KEY("ShD", ch->short_descr, fread_string(fp));
			KEY("Sec", ch->pcdata->security, fread_number(fp)); /* OLC */
			KEY("Silv", ch->silver, fread_number(fp));

			if (!str_cmp(word, "Sk"))
			{
				int sn;
				int value;
				char *temp;

				value = fread_number(fp);
				temp = fread_word(fp);
				sn = skill_lookup(temp);
				/* sn    = skill_lookup( fread_word( fp ) ); */
				if (sn < 0)
				{
					fprintf(stderr, "%s", temp);
					bug("Fread_char: unknown skill. ", 0);
				}
				else
				{
					ch->pcdata->learned[sn] = value;
					ch->pcdata->sk_level[sn] = skill_level(ch, sn);
					ch->pcdata->sk_rating[sn] = skill_rating(ch, sn);
				}
				fMatch = TRUE;
			}

			if (!str_cmp(word, "Skill"))
			{
				int sn;
				int value, l_value, r_value;
				char *temp;

				value = fread_number(fp);
				l_value = fread_number(fp);
				r_value = fread_number(fp);
				temp = fread_word(fp);
				sn = skill_lookup(temp);
				/* sn    = skill_lookup( fread_word( fp ) ); */
				if (sn < 0)
				{
					fprintf(stderr, "%s", temp);
					bug("Fread_char: unknown skill. ", 0);
				}
				else
					ch->pcdata->learned[sn] = value;
				ch->pcdata->sk_level[sn] = l_value;
				ch->pcdata->sk_rating[sn] = r_value;
				fMatch = TRUE;
			}
			break;

		case 'T':
			KEY("TrueSex", ch->pcdata->true_sex, fread_number(fp));
			KEY("TSex", ch->pcdata->true_sex, fread_number(fp));
			KEY("Trai", ch->train, fread_number(fp));
			KEY("Trust", ch->trust, fread_number(fp));
			KEY("Tru", ch->trust, fread_number(fp));

			if (!str_cmp(word, "Title") || !str_cmp(word, "Titl"))
			{
				ch->pcdata->title = fread_string(fp);
				if (ch->pcdata->title[0] != '.' && ch->pcdata->title[0] != ',' && ch->pcdata->title[0] != '!' && ch->pcdata->title[0] != '?')
				{
					snprintf(buf, sizeof(buf), " %s", ch->pcdata->title);
					free_string(ch->pcdata->title);
					ch->pcdata->title = str_dup(buf);
				}
				fMatch = TRUE;
				break;
			}

			break;

		case 'V':
			KEY("Version", ch->version, fread_number(fp));
			KEY("Vers", ch->version, fread_number(fp));
			if (!str_cmp(word, "Vnum"))
			{
				ch->pIndexData = get_mob_index(fread_number(fp));
				fMatch = TRUE;
				break;
			}
			break;

		case 'W':
			KEY("Wimpy", ch->wimpy, fread_number(fp));
			KEY("Wimp", ch->wimpy, fread_number(fp));
			KEY("Wizn", ch->wiznet, fread_flag(fp));
			break;
		}

		if (!fMatch)
		{
			snprintf(buf, sizeof(buf), "Fread_char: %s: no match in file.", word);
			bug(buf, 0);
			fread_to_eol(fp);
		}
	}
}

/* load a pet from the forgotten reaches */
void fread_pet(Character *ch, FILE *fp)
{
	char *word;
	Character *pet;
	bool fMatch;
	int lastlogoff = current_time;
	int percent;

	/* first entry had BETTER be the vnum or we barf */
	word = feof(fp) ? (char *)"END" : fread_word(fp);
	if (!str_cmp(word, "Vnum"))
	{
		int vnum;

		vnum = fread_number(fp);
		if (get_mob_index(vnum) == NULL)
		{
			bug("Fread_pet: bad vnum %d.", vnum);
			pet = new NonPlayerCharacter(get_mob_index(MOB_VNUM_FIDO));
		}
		else
			pet = new NonPlayerCharacter(get_mob_index(vnum));
	}
	else
	{
		bug("Fread_pet: no vnum in file.", 0);
		pet = new NonPlayerCharacter(get_mob_index(MOB_VNUM_FIDO));
	}

	for (;;)
	{
		word = feof(fp) ? (char *)"END" : fread_word(fp);
		fMatch = FALSE;

		switch (UPPER(word[0]))
		{
		case '*':
			fMatch = TRUE;
			fread_to_eol(fp);
			break;

		case 'A':
			KEY("Act", pet->act, fread_flag(fp));
			KEY("AfBy", pet->affected_by, fread_flag(fp));
			KEY("Alig", pet->alignment, fread_number(fp));

			if (!str_cmp(word, "ACs"))
			{
				int i;

				for (i = 0; i < 4; i++)
					pet->armor[i] = fread_number(fp);
				fMatch = TRUE;
				break;
			}

			if (!str_cmp(word, "AffD"))
			{
				AFFECT_DATA *paf;
				int sn;

				paf = new_affect();

				sn = skill_lookup(fread_word(fp));
				if (sn < 0)
					bug("Fread_char: unknown skill.", 0);
				else
					paf->type = sn;

				paf->level = fread_number(fp);
				paf->duration = fread_number(fp);
				paf->modifier = fread_number(fp);
				paf->location = fread_number(fp);
				paf->bitvector = fread_number(fp);
				pet->affected.push_back(paf);
				fMatch = TRUE;
				break;
			}

			if (!str_cmp(word, "Affc"))
			{
				AFFECT_DATA *paf;
				int sn;

				paf = new_affect();

				sn = skill_lookup(fread_word(fp));
				if (sn < 0)
					bug("Fread_char: unknown skill.", 0);
				else
					paf->type = sn;

				paf->where = fread_number(fp);
				paf->level = fread_number(fp);
				paf->duration = fread_number(fp);
				paf->modifier = fread_number(fp);
				paf->location = fread_number(fp);
				paf->bitvector = fread_number(fp);
				pet->affected.push_back(paf);
				fMatch = TRUE;
				break;
			}

			if (!str_cmp(word, "AMod"))
			{
				int stat;

				for (stat = 0; stat < MAX_STATS; stat++)
					pet->mod_stat[stat] = fread_number(fp);
				fMatch = TRUE;
				break;
			}

			if (!str_cmp(word, "Attr"))
			{
				int stat;

				for (stat = 0; stat < MAX_STATS; stat++)
					pet->perm_stat[stat] = fread_number(fp);
				fMatch = TRUE;
				break;
			}
			break;

		case 'C':
			KEY("Comm", pet->comm, fread_flag(fp));
			break;

		case 'D':
			KEY("Dam", pet->damroll, fread_number(fp));
			CKEYS("Desc", pet->setDescription, fread_string(fp));
			break;

		case 'E':
			if (!str_cmp(word, (char *)"End"))
			{
				pet->leader = ch;
				pet->master = ch;
				ch->pet = pet;
				/* adjust hp mana move up  -- here for speed's sake */
				percent = (current_time - lastlogoff) * 25 / (2 * 60 * 60);

				if (percent > 0 && !IS_AFFECTED(ch, AFF_POISON) && !IS_AFFECTED(ch, AFF_PLAGUE))
				{
					percent = UMIN(percent, 100);
					pet->hit += (pet->max_hit - pet->hit) * percent / 100;
					pet->mana += (pet->max_mana - pet->mana) * percent / 100;
					pet->move += (pet->max_move - pet->move) * percent / 100;
				}
				return;
			}
			KEY("Exp", pet->exp, fread_number(fp));
			break;

		case 'G':
			KEY("Gold", pet->gold, fread_number(fp));
			break;

		case 'H':
			KEY("Hit", pet->hitroll, fread_number(fp));

			if (!str_cmp(word, "HMV"))
			{
				pet->hit = fread_number(fp);
				pet->max_hit = fread_number(fp);
				pet->mana = fread_number(fp);
				pet->max_mana = fread_number(fp);
				pet->move = fread_number(fp);
				pet->max_move = fread_number(fp);
				fMatch = TRUE;
				break;
			}
			break;

		case 'L':
			KEY("Levl", pet->level, fread_number(fp));
			KEY("LnD", pet->long_descr, fread_string(fp));
			KEY("LogO", lastlogoff, fread_number(fp));
			break;

		case 'N':
			CKEYS("Name", pet->setName, fread_string(fp));
			break;

		case 'P':
			KEY("Pos", pet->position, fread_number(fp));
			break;

		case 'R':
			KEYF("Race", pet->setRace, race_manager->getRaceByName(fread_string(fp)));
			break;

		case 'S':
			KEY("Save", pet->saving_throw, fread_number(fp));
			KEY("Sex", pet->sex, fread_number(fp));
			KEY("ShD", pet->short_descr, fread_string(fp));
			KEY("Silv", pet->silver, fread_number(fp));
			break;

			if (!fMatch)
			{
				bug("Fread_pet: no match.", 0);
				fread_to_eol(fp);
			}
		}
	}
}

extern Object *obj_free;

void fread_obj(Character *ch, FILE *fp)
{
	Object *obj;
	char *word;
	int iNest;
	bool fMatch;
	bool fNest;
	bool fVnum;
	bool first;
	bool new_format; /* to prevent errors */
	bool make_new;	 /* update object */

	fVnum = FALSE;
	obj = NULL;
	first = TRUE; /* used to counter fp offset */
	new_format = FALSE;
	make_new = FALSE;

	word = feof(fp) ? (char *)"End" : fread_word(fp);
	if (!str_cmp(word, "Vnum"))
	{
		int vnum;
		first = FALSE; /* fp will be in right place */

		vnum = fread_number(fp);
		if (get_obj_index(vnum) == NULL)
		{
			bug("Fread_obj: bad vnum %d.", vnum);
		}
		else
		{
			obj = ObjectHelper::createFromIndex(get_obj_index(vnum), -1);
			new_format = TRUE;
		}
	}

	if (obj == NULL) /* either not found or old style */
	{
		throw std::runtime_error("Could not load object with no Vnum - index template is required.");
	}

	fNest = FALSE;
	fVnum = TRUE;
	iNest = 0;

	for (;;)
	{
		if (first)
			first = FALSE;
		else
			word = feof(fp) ? (char *)"End" : fread_word(fp);
		fMatch = FALSE;

		switch (UPPER(word[0]))
		{
		case '*':
			fMatch = TRUE;
			fread_to_eol(fp);
			break;

		case 'A':
			if (!str_cmp(word, "AffD"))
			{
				AFFECT_DATA *paf;
				int sn;

				paf = new_affect();

				sn = skill_lookup(fread_word(fp));
				if (sn < 0)
					bug("Fread_obj: unknown skill.", 0);
				else
					paf->type = sn;

				paf->level = fread_number(fp);
				paf->duration = fread_number(fp);
				paf->modifier = fread_number(fp);
				paf->location = fread_number(fp);
				paf->bitvector = fread_number(fp);
				obj->addAffect(paf);
				fMatch = TRUE;
				break;
			}
			if (!str_cmp(word, "Affc"))
			{
				AFFECT_DATA *paf;
				int sn;

				paf = new_affect();

				sn = skill_lookup(fread_word(fp));
				if (sn < 0)
					bug("Fread_obj: unknown skill.", 0);
				else
					paf->type = sn;

				paf->where = fread_number(fp);
				paf->level = fread_number(fp);
				paf->duration = fread_number(fp);
				paf->modifier = fread_number(fp);
				paf->location = fread_number(fp);
				paf->bitvector = fread_number(fp);
				obj->addAffect(paf);
				fMatch = TRUE;
				break;
			}
			break;

		case 'C':
			KEYF("Cond", obj->setCondition, fread_number(fp));
			KEYF("Cost", obj->setCost, fread_number(fp));
			break;

		case 'D':
			KEYF("Description", obj->setDescription, fread_string(fp));
			KEYF("Desc", obj->setDescription, fread_string(fp));
			break;

		case 'E':

			if (!str_cmp(word, "Enchanted"))
			{
				obj->setIsEnchanted(true);
				fMatch = TRUE;
				break;
			}

			KEYF("ExtraFlags", obj->setExtraFlags, fread_number(fp));
			KEYF("ExtF", obj->setExtraFlags, fread_number(fp));

			if (!str_cmp(word, "ExtraDescr") || !str_cmp(word, "ExDe"))
			{
				ExtraDescription *ed = new ExtraDescription();

				ed->setKeyword(fread_string(fp));
				ed->setDescription(fread_string(fp));
				obj->addExtraDescription(ed);
				fMatch = TRUE;
			}

			if (!str_cmp(word, (char *)"End"))
			{
				if (!fNest || (fVnum && obj->getObjectIndexData() == NULL))
				{
					bug("Fread_obj: incomplete object.", 0);
					delete obj;
					return;
				}
				else
				{
					if (!fVnum)
					{
						delete obj;
						obj = ObjectHelper::createFromIndex(get_obj_index(OBJ_VNUM_DUMMY), 0);
					}

					if (!new_format)
					{
						object_list.push_back(obj);
						obj->getObjectIndexData()->count++;
					}

					if (!obj->getObjectIndexData()->new_format && obj->getItemType() == ITEM_ARMOR && obj->getValues().at(1) == 0)
					{
						obj->getValues().at(1) = obj->getValues().at(0);
						obj->getValues().at(2) = obj->getValues().at(0);
					}
					if (make_new)
					{
						int wear;

						wear = obj->getWearLocation();
						extract_obj(obj);

						obj = ObjectHelper::createFromIndex(obj->getObjectIndexData(), 0);
						obj->setWearLocation(wear);
					}
					if (iNest == 0 || rgObjNest[iNest] == NULL)
					{
						ch->addObject(obj);
					}
					else
					{
						rgObjNest[iNest - 1]->addObject(obj);
					}
					return;
				}
			}
			break;

		case 'I':
			KEYF("ItemType", obj->setItemType, fread_number(fp));
			KEYF("Ityp", obj->setItemType, fread_number(fp));
			break;

		case 'L':
			KEYF("Level", obj->setLevel, fread_number(fp));
			KEYF("Lev", obj->setLevel, fread_number(fp));
			break;

		case 'N':
			KEYF("Name", obj->setName, fread_string(fp));

			if (!str_cmp(word, "Nest"))
			{
				iNest = fread_number(fp);
				if (iNest < 0 || iNest >= MAX_NEST)
				{
					bug("Fread_obj: bad nest %d.", iNest);
				}
				else
				{
					rgObjNest[iNest] = obj;
					fNest = TRUE;
				}
				fMatch = TRUE;
			}
			break;

		case 'O':
			if (!str_cmp(word, "Oldstyle"))
			{
				if (obj->getObjectIndexData() != NULL && obj->getObjectIndexData()->new_format)
					make_new = TRUE;
				fMatch = TRUE;
			}
			break;

		case 'S':
			KEYF("ShortDescr", obj->setShortDescription, fread_string(fp));
			KEYF("ShD", obj->setShortDescription, fread_string(fp));

			if (!str_cmp(word, "Spell"))
			{
				int iValue;
				int sn;

				iValue = fread_number(fp);
				sn = skill_lookup(fread_word(fp));
				if (iValue < 0 || iValue > 3)
				{
					bug("Fread_obj: bad iValue %d.", iValue);
				}
				else if (sn < 0)
				{
					bug("Fread_obj: unknown skill.", 0);
				}
				else
				{
					obj->getValues().at(iValue) = sn;
				}
				fMatch = TRUE;
				break;
			}

			break;

		case 'T':
			KEYF("Timer", obj->setTimer, fread_number(fp));
			KEYF("Time", obj->setTimer, fread_number(fp));
			break;

		case 'V':
			if (!str_cmp(word, "Values") || !str_cmp(word, "Vals"))
			{
				obj->getValues().at(0) = fread_number(fp);
				obj->getValues().at(1) = fread_number(fp);
				obj->getValues().at(2) = fread_number(fp);
				obj->getValues().at(3) = fread_number(fp);
				if (obj->getItemType() == ITEM_WEAPON && obj->getValues().at(0) == 0)
					obj->getValues().at(0) = obj->getObjectIndexData()->value[0];
				fMatch = TRUE;
				break;
			}

			if (!str_cmp(word, "Val"))
			{
				obj->getValues().at(0) = fread_number(fp);
				obj->getValues().at(1) = fread_number(fp);
				obj->getValues().at(2) = fread_number(fp);
				obj->getValues().at(3) = fread_number(fp);
				obj->getValues().at(4) = fread_number(fp);
				fMatch = TRUE;
				break;
			}

			if (!str_cmp(word, "Vnum"))
			{
				int vnum;

				vnum = fread_number(fp);
				if ((obj->setObjectIndexData(get_obj_index(vnum))) == NULL)
					bug("Fread_obj: bad vnum %d.", vnum);
				else
					fVnum = TRUE;
				fMatch = TRUE;
				break;
			}
			break;

		case 'W':
			KEYF("WearFlags", obj->setWearFlags, fread_number(fp));
			KEYF("WeaF", obj->setWearFlags, fread_number(fp));
			KEYF("WearLoc", obj->setWearLocation, fread_number(fp));
			KEYF("Wear", obj->setWearLocation, fread_number(fp));
			KEYF("Weight", obj->setWeight, fread_number(fp));
			KEYF("Wt", obj->setWeight, fread_number(fp));
			break;
		}

		if (!fMatch)
		{
			bug("Fread_obj: no match.", 0);
			fread_to_eol(fp);
		}
	}
}
