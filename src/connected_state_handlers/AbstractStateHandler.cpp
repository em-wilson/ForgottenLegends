#include "AbstractStateHandler.h"
#include "Character.h"
#include "Wiznet.h"

extern std::list<Character *> char_list;

AbstractStateHandler::AbstractStateHandler(ConnectedState state) {
    _state = state;
}

bool AbstractStateHandler::canHandleState(ConnectedState state) {
    return state == _state;
}

/*
 * Look for link-dead player to reconnect.
 */
bool AbstractStateHandler::check_reconnect(DESCRIPTOR_DATA *d, char *name, bool fConn)
{
	Character *ch;

	for (std::list<Character *>::iterator it = char_list.begin(); it != char_list.end(); it++)
	{
		ch = *it;
		if (!IS_NPC(ch) && (!fConn || !ch->desc) && !str_cmp(d->character->getName(), ch->getName()))
		{
			if (fConn == FALSE)
			{
				free_string(d->character->pcdata->pwd);
				d->character->pcdata->pwd = str_dup(ch->pcdata->pwd);
			}
			else
			{
				delete d->character;
				d->character = ch;
				ch->desc = d;
				ch->timer = 0;
				send_to_char(
					"Reconnecting. Type replay to see missed tells.\n\r", ch);

				/* We don't want them showing up in the note room */
				if (ch->in_room == get_room_index(ROOM_VNUM_NOTE))
				{
					char_from_room(ch);
					char_to_room(ch, get_room_index(ROOM_VNUM_LIMBO));
				}

				act("$n has reconnected.", ch, NULL, NULL, TO_ROOM);

				snprintf(log_buf, 2 * MAX_INPUT_LENGTH, "%s@%s reconnected.", ch->getName(), d->host);
				log_string(log_buf);
				Wiznet::instance()->report((char *)"$N groks the fullness of $S link.",
										   ch, NULL, WIZ_LINKS, 0, 0);
				d->connected = CON_PLAYING;
				/* Inform the character of a note in progress and the possbility
				 * of continuation!
				 */
				if (ch->pcdata->in_progress)
					send_to_char("You have a note in progress. Type NOTE WRITE to continue it.\n\r", ch);
			}
			return TRUE;
		}
	}

	return FALSE;
}
