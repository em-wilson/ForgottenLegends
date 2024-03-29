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
 *	ROM 2.4 is copyright 1993-1996 Russ Taylor			   *
 *	ROM has been brought to you by the ROM consortium		   *
 *	    Russ Taylor (rtaylor@efn.org)				   *
 *	    Gabrielle Taylor						   *
 *	    Brian Moore (zump@rom.org)					   *
 *	By using this code, you have agreed to follow the terms of the	   *
 *	ROM license, in the file Rom24/doc/rom.license			   *
 ***************************************************************************/

#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "merc.h"
#include "ConnectedState.h"
#include "music.h"
#include "Object.h"
#include "recycle.h"
#include "Room.h"

int channel_songs[MAX_GLOBAL + 1];
struct song_data song_table[MAX_SONGS];
Character *dedicate;

void song_update(void)
{
	Object *obj;
	Character *victim;
	ROOM_INDEX_DATA *room;
	DESCRIPTOR_DATA *d;
	char buf[MAX_STRING_LENGTH];
	char *line;
	int i;

	/* do the global song, if any */
	if (channel_songs[1] >= MAX_SONGS)
		channel_songs[1] = -1;

	if (channel_songs[1] > -1)
	{
		if (channel_songs[0] >= MAX_LINES || channel_songs[0] >= song_table[channel_songs[1]].lines)
		{
			channel_songs[0] = -1;

			/* advance songs */
			for (i = 1; i < MAX_GLOBAL; i++)
				channel_songs[i] = channel_songs[i + 1];
			channel_songs[MAX_GLOBAL] = -1;
		}
		else
		{
			if (channel_songs[0] < 0)
			{
				snprintf(buf, sizeof(buf), "Music: %s, %s",
						 song_table[channel_songs[1]].group,
						 song_table[channel_songs[1]].name);
				channel_songs[0] = 0;
			}
			else
			{
				snprintf(buf, sizeof(buf), "Music: '%s'",
						 song_table[channel_songs[1]].lyrics[channel_songs[0]]);
				channel_songs[0]++;
			}

			for (d = descriptor_list; d != NULL; d = d->next)
			{
				victim = d->original ? d->original : d->character;

				if (d->connected == ConnectedState::Playing &&
					!IS_SET(victim->comm, COMM_NOMUSIC) &&
					!IS_SET(victim->comm, COMM_QUIET))
					act("$t", d->character, buf, NULL, TO_CHAR, POS_SLEEPING);
			}
		}
	}

	for (auto obj : object_list)
	{
		if (obj->getItemType() != ITEM_JUKEBOX || obj->getValues().at(1) < 0)
			continue;

		if (obj->getValues().at(1) >= MAX_SONGS)
		{
			obj->getValues().at(1) = -1;
			continue;
		}

		/* find which room to play in */

		if ((room = obj->getInRoom()) == NULL)
		{
			if (obj->getCarriedBy() == NULL)
				continue;
			else if ((room = obj->getCarriedBy()->in_room) == NULL)
				continue;
		}

		if (obj->getValues().at(0) < 0)
		{
			snprintf(buf, sizeof(buf), "$p starts playing %s, %s.",
					 song_table[obj->getValues().at(1)].group, song_table[obj->getValues().at(1)].name);
			if (room->people != NULL)
				act(buf, room->people, obj, NULL, TO_ALL, POS_RESTING);
			obj->getValues().at(0) = 0;
			continue;
		}
		else
		{
			if (obj->getValues().at(0) >= MAX_LINES || obj->getValues().at(0) >= song_table[obj->getValues().at(1)].lines)
			{

				obj->getValues().at(0) = -1;

				/* scroll songs forward */
				obj->getValues().at(1) = obj->getValues().at(2);
				obj->getValues().at(2) = obj->getValues().at(3);
				obj->getValues().at(3) = obj->getValues().at(4);
				obj->getValues().at(4) = -1;
				continue;
			}

			line = song_table[obj->getValues().at(1)].lyrics[obj->getValues().at(0)];
			obj->getValues().at(0)++;
		}

		snprintf(buf, sizeof(buf), "$p bops: '%s'", line);
		if (room->people != NULL)
			act(buf, room->people, obj, NULL, TO_ALL, POS_RESTING);
	}
}

void load_songs(void)
{
	FILE *fp;
	int count = 0, lines, i;
	char letter;

	/* reset global */
	for (i = 0; i <= MAX_GLOBAL; i++)
		channel_songs[i] = -1;

	if ((fp = fopen(MUSIC_FILE, "r")) == NULL)
	{
		bug("Couldn't open music file, no songs available.", 0);
		fclose(fp);
		return;
	}

	for (count = 0; count < MAX_SONGS; count++)
	{
		letter = fread_letter(fp);
		if (letter == '#')
		{
			if (count < MAX_SONGS)
				song_table[count].name = NULL;
			fclose(fp);
			return;
		}
		else
			ungetc(letter, fp);

		song_table[count].group = fread_string(fp);
		song_table[count].name = fread_string(fp);

		/* read lyrics */
		lines = 0;

		for (;;)
		{
			letter = fread_letter(fp);

			if (letter == '~')
			{
				song_table[count].lines = lines;
				break;
			}
			else
				ungetc(letter, fp);

			if (lines >= MAX_LINES)
			{
				bug("Too many lines in a song -- limit is  %d.", MAX_LINES);
				break;
			}

			song_table[count].lyrics[lines] = fread_string_eol(fp);
			lines++;
		}
	}
}

void do_play(Character *ch, char *argument)
{
	Object *juke;
	char *str, arg[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
	int song, i;
	bool global = FALSE, dedy = FALSE;

	str = one_argument(argument, arg);

	for (auto j : ch->in_room->contents) {
		juke = j;
		if (juke->getItemType() == ITEM_JUKEBOX && ch->can_see( juke))
			break;
	}

	if (argument[0] == '\0')
	{
		send_to_char("Play what?\n\r", ch);
		return;
	}

	if (juke == NULL)
	{
		send_to_char("You see nothing to play.\n\r", ch);
		return;
	}

	if (!str_cmp(arg, "list"))
	{
		BUFFER *buffer;
		char buf[MAX_STRING_LENGTH];
		int col = 0;
		bool artist = FALSE, match = FALSE;

		buffer = new_buf();
		argument = str;
		argument = one_argument(argument, arg);

		if (!str_cmp(arg, "artist"))
			artist = TRUE;

		if (argument[0] != '\0')
			match = TRUE;

		snprintf(buf, sizeof(buf), "%s has the following songs available:\n\r",
				 juke->getShortDescription().c_str());
		add_buf(buffer, capitalize(buf));

		for (i = 0; i < MAX_SONGS; i++)
		{
			if (song_table[i].name == NULL)
				break;

			if (artist && (!match || !str_prefix(argument, song_table[i].group)))
				snprintf(buf, sizeof(buf), "%-39s %-39s\n\r",
						 song_table[i].group, song_table[i].name);
			else if (!artist && (!match || !str_prefix(argument, song_table[i].name)))
				snprintf(buf, sizeof(buf), "%-35s ", song_table[i].name);
			else
				continue;
			add_buf(buffer, buf);
			if (!artist && ++col % 2 == 0)
				add_buf(buffer, "\n\r");
		}
		if (!artist && col % 2 != 0)
			add_buf(buffer, "\n\r");

		page_to_char(buf_string(buffer), ch);
		free_buf(buffer);
		return;
	}

	if (!str_cmp(arg, "loud"))
	{
		argument = str;
		global = TRUE;
	}

	if (argument[0] == '\0')
	{
		send_to_char("Play what?\n\r", ch);
		return;
	}

	if (!str_cmp(arg, "dedicate"))
	{

		char ros[MAX_STRING_LENGTH];

		str = one_argument(str, ros);

		if ((dedicate = get_char_world(ch, ros)) == NULL)
		{
			send_to_char("They aren't here.\n\r", ch);
			return;
		}
		argument = str;
		dedy = TRUE;
		global = TRUE;
	}

	if ((global && channel_songs[MAX_GLOBAL] > -1) || (!global && juke->getValues().at(4) > -1))
	{
		send_to_char("The jukebox is full up right now.\n\r", ch);
		return;
	}

	for (song = 0; song < MAX_SONGS; song++)
	{
		if (song_table[song].name == NULL)
		{
			send_to_char("That song isn't available.\n\r", ch);
			return;
		}
		if (!str_prefix(argument, song_table[song].name))
			break;
	}

	if (song >= MAX_SONGS || argument[0] == '\0')
	{
		send_to_char("That song isn't available.\n\r", ch);
		return;
	}

	send_to_char("Coming right up.\n\r", ch);
	if (dedy)
	{
		DESCRIPTOR_DATA *d;

		snprintf(buf, sizeof(buf), "This next song dedicated to %s by %s.",
				 dedicate->getName().c_str(), ch->getName().c_str());

		for (d = descriptor_list; d != NULL; d = d->next)
		{
			Character *victim;
			victim = d->original ? d->original : d->character;

			if (d->connected == ConnectedState::Playing &&
				!IS_SET(victim->comm, COMM_NOMUSIC) &&
				!IS_SET(victim->comm, COMM_QUIET))
				act("$t", d->character, buf, NULL, TO_CHAR, POS_SLEEPING);
		}
	}

	if (global)
	{
		for (i = 1; i <= MAX_GLOBAL; i++)
			if (channel_songs[i] < 0)
			{
				if (i == 1)
					channel_songs[0] = -1;
				channel_songs[i] = song;
				return;
			}
	}
	else
	{
		for (i = 1; i < 5; i++)
			if (juke->getValues().at(i) < 0)
			{
				if (i == 1)
					juke->getValues().at(0) = -1;
				juke->getValues().at(i) = song;
				return;
			}
	}
}
