#include "AbstractStateHandler.h"
#include "ActionTarget.h"
#include "Descriptor.h"
#include "Character.h"
#include "Wiznet.h"

extern std::list<Character *> char_list;
void act(const char *format, Character *ch, const void *arg1, const void *arg2, int type);
void free_string( char *pstr );
bool str_cmp( const char *astr, const char *bstr );
char *str_dup( const char *str );
void send_to_char(const char *txt, Character *ch);
ROOM_INDEX_DATA *get_room_index( int vnum );
void char_from_room( Character *ch );
bool is_note_room( ROOM_INDEX_DATA *room );
ROOM_INDEX_DATA *get_limbo_room();
void char_to_room( Character *ch, ROOM_INDEX_DATA *pRoomIndex );
void log_string( const char *str );

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
	char log_buf[2*MAX_INPUT_LENGTH];
	Character *ch;

	for (std::list<Character *>::iterator it = char_list.begin(); it != char_list.end(); it++)
	{
		ch = *it;
		if (!ch->isNPC() && (!fConn || !ch->desc) && !str_cmp(d->character->getName(), ch->getName()))
		{
			if (!fConn)
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
				if (is_note_room(ch->in_room))
				{
					char_from_room(ch);
					char_to_room(ch, get_limbo_room());
				}

				act("$n has reconnected.", ch, NULL, NULL, ActionTarget::ToRoom);

				snprintf(log_buf, 2 * MAX_INPUT_LENGTH, "%s@%s reconnected.", ch->getName(), d->host);
				log_string(log_buf);
				Wiznet::instance()->report((char *)"$N groks the fullness of $S link.",
										   ch, NULL, WIZ_LINKS, 0, 0);
				d->connected = ConnectedState::Playing;
				/* Inform the character of a note in progress and the possbility
				 * of continuation!
				 */
				if (ch->pcdata->in_progress)
					send_to_char("You have a note in progress. Type NOTE WRITE to continue it.\n\r", ch);
			}
			return true;
		}
	}

	return false;
}
