#include <list>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include "merc.h"
#include "ActionTarget.h"
#include "Character.h"
#include "ConnectedState.h"
#include "Descriptor.h"
#include "ILogger.h"
#include "RoomManager.h"
#include "SocketHelper.h"
#include "Wiznet.h"

/*
 * Global variables.
 */
extern DESCRIPTOR_DATA *descriptor_list; /* All open descriptors		*/
extern DESCRIPTOR_DATA *d_next;		  /* Next descriptor in loop	*/
extern bool merc_down; // Game is shut down
extern RoomManager *room_manager;
extern std::list<Character *> char_list;

void act(const char *format, Character *ch, const void *arg1, const void *arg2, int type);
bool is_note_room( ROOM_INDEX_DATA *room );
bool process_output(DESCRIPTOR_DATA *d, bool fPrompt);

/*
 * Look for link-dead player to reconnect.
 */
bool SocketHelper::check_reconnect(DESCRIPTOR_DATA *d, bool fConn)
{
	char log_buf[2*MAX_INPUT_LENGTH];

	for (auto ch : char_list)
	{
		if (!ch->isNPC() && (!fConn || !ch->desc) && d->character->getName() == ch->getName())
		{
			if (!fConn)
			{
                d->character->pcdata->setPassword(ch->pcdata->getPassword());
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
					room_manager->char_from_room(ch);
					room_manager->char_to_room(ch, room_manager->get_limbo_room());
				}

				act("$n has reconnected.", ch, NULL, NULL, ActionTarget::ToRoom, POS_RESTING);

				snprintf(log_buf, 2 * MAX_INPUT_LENGTH, "%s@%s reconnected.", ch->getName().c_str(), d->host);
				logger->log_string(log_buf);
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

void SocketHelper::close_socket(DESCRIPTOR_DATA *dclose)
{
	Character *ch;

	if (dclose->outtop > 0)
		process_output(dclose, FALSE);

	if (dclose->snoop_by != NULL)
	{
		write_to_buffer(dclose->snoop_by,
						"Your victim has left the game.\n\r", 0);
	}

	{
		DESCRIPTOR_DATA *d;

		for (d = descriptor_list; d != NULL; d = d->next)
		{
			if (d->snoop_by == dclose)
				d->snoop_by = NULL;
		}
	}

	if ((ch = dclose->character) != NULL)
	{
		snprintf(log_buf, 2 * MAX_INPUT_LENGTH, "Closing link to %s.", ch->getName().c_str());
		logger->log_string(log_buf);
		/* cut down on wiznet spam when rebooting */
		/* If ch is writing note or playing, just lose link otherwise clear char */
		if ((dclose->connected == ConnectedState::Playing && !merc_down) || ((dclose->connected >= ConnectedState::NoteTo) && (dclose->connected <= ConnectedState::NoteFinish)))
		{
			act("$n has lost $s link.", ch, NULL, NULL, TO_ROOM, POS_RESTING );
			Wiznet::instance()->report((char *)"Net death has claimed $N.", ch, NULL, WIZ_LINKS, 0, 0);
			ch->desc = NULL;
		}
		else
		{
			if (dclose->original)
			{
				char_list.remove(dclose->original);
				delete dclose->original;
				dclose->original = NULL;
			}
			else
			{
				char_list.remove(dclose->character);
				delete dclose->character;
				dclose->character = NULL;
			}
		}
	}

	if (d_next == dclose)
		d_next = d_next->next;

	if (dclose == descriptor_list)
	{
		descriptor_list = descriptor_list->next;
	}
	else
	{
		DESCRIPTOR_DATA *d;

		for (d = descriptor_list; d && d->next != dclose; d = d->next)
			;
		if (d != NULL)
			d->next = dclose->next;
		else
			bug("Close_socket: dclose not found.", 0);
	}

	close(dclose->descriptor);
	free_descriptor(dclose);
	return;
}

int SocketHelper::init_socket(int port)
{
	static struct sockaddr_in sa_zero;
	struct sockaddr_in sa;
	int x = 1;
	int fd;

	if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("Init_socket: socket");
		exit(1);
	}

	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR,
				   (char *)&x, sizeof(x)) < 0)
	{
		perror("Init_socket: SO_REUSEADDR");
		close(fd);
		exit(1);
	}

	sa = sa_zero;
	sa.sin_family = AF_INET;
	sa.sin_port = htons(port);

	if (bind(fd, (struct sockaddr *)&sa, sizeof(sa)) < 0)
	{
		perror("Init socket: bind");
		close(fd);
		exit(1);
	}

	if (listen(fd, 3) < 0)
	{
		perror("Init socket: listen");
		close(fd);
		exit(1);
	}

	return fd;
}

/*
 * Write to one char, new colour version, by Lope.
 */
void SocketHelper::send_to_char(const char *txt, Character *ch)
{
	const char *point;
	char *point2;
	char buf[MAX_STRING_LENGTH * 4];
	int skip = 0;

	buf[0] = '\0';
	point2 = buf;
	if (txt && ch->desc)
	{
		if (IS_SET(ch->act, PLR_COLOUR))
		{
			for (point = txt; *point; point++)
			{
				if (*point == '{')
				{
					point++;
					skip = colour(*point, ch, point2);
					while (skip-- > 0)
						++point2;
					continue;
				}
				*point2 = *point;
				*++point2 = '\0';
			}
			*point2 = '\0';
			write_to_buffer(ch->desc, buf, point2 - buf);
		}
		else
		{
			for (point = txt; *point; point++)
			{
				if (*point == '{')
				{
					point++;
					continue;
				}
				*point2 = *point;
				*++point2 = '\0';
			}
			*point2 = '\0';
			write_to_buffer(ch->desc, buf, point2 - buf);
		}
	}
	return;
}

/*
 * Append onto an output buffer.
 */
void SocketHelper::write_to_buffer(DESCRIPTOR_DATA *d, const char *txt, int length)
{
	/*
	 * Find length in case caller didn't.
	 */
	if (length <= 0)
		length = strlen(txt);

	/*
	 * Initial \n\r if needed.
	 */
	if (d->outtop == 0 && !d->fcommand)
	{
		d->outbuf[0] = '\n';
		d->outbuf[1] = '\r';
		d->outtop = 2;
	}

	/*
	 * Expand the buffer as needed.
	 */
	while (d->outtop + length >= d->outsize)
	{
		char *outbuf;

		if (d->outsize >= 32000)
		{
			bug("Buffer overflow. Closing.\n\r", 0);
			close_socket(d);
			return;
		}
		outbuf = new char[2 * d->outsize];
		strncpy(outbuf, d->outbuf, d->outtop);
		delete[] d->outbuf;
		d->outbuf = outbuf;
		d->outsize *= 2;
	}

	/*
	 * Copy.
	 */
	/*  strcpy( d->outbuf + d->outtop, txt ); */
	strncpy(d->outbuf + d->outtop, txt, length);
	d->outtop += length;
	return;
}

/*
 * Lowest level output function.
 * Write a block of text to the file descriptor.
 * If this gives errors on very long blocks (like 'ofind all'),
 *   try lowering the max block size.
 */
bool SocketHelper::write_to_descriptor(int desc, const char *txt, int length)
{
	int iStart;
	int nWrite;
	int nBlock;

	if (length <= 0)
		length = strlen(txt);

	for (iStart = 0; iStart < length; iStart += nWrite)
	{
		nBlock = UMIN(length - iStart, 4096);
		if ((nWrite = write(desc, txt + iStart, nBlock)) < 0)
		{
			perror("Write_to_descriptor");
			return FALSE;
		}
	}

	return TRUE;
}