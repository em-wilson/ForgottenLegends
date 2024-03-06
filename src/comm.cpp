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
 *  Thanks to abaddon for proof-reading our comm.c and pointing out bugs.  *
 *  Any remaining bugs are, of course, our work, not his.  :)              *
 *                                                                         *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 ***************************************************************************/

/***************************************************************************
 *	ROM 2.4 is copyright 1993-1996 Russ Taylor			  				   *
 *	ROM has been brought to you by the ROM consortium		   			   *
 *	    Russ Taylor (rtaylor@efn.org)				   					   *
 *	    Gabrielle Taylor						   						   *
 *	    Brian Moore (zump@rom.org)										   *
 *	By using this code, you have agreed to follow the terms of the		   *
 *	ROM license, in the file Rom24/doc/rom.license			   			   *
 ***************************************************************************/

/*
 * This file contains all of the OS-dependent stuff:
 *   startup, signals, BSD sockets for tcp/ip, i/o, timing.
 *
 * The data flow for input is:
 *    Game_loop ---> Read_from_descriptor ---> Read
 *    Game_loop ---> Read_from_buffer
 *
 * The data flow for output is:
 *    Game_loop ---> Process_Output ---> Write_to_descriptor -> Write
 *
 * The OS-dependent functions are Read_from_descriptor and Write_to_descriptor.
 * -- Furey  26 Jan 1993
 */

#include <sys/types.h>
#include <sys/time.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <stdarg.h>
#include <unistd.h>

#include "merc.h"
#include "board.h"
#include "Game.h"
#include "recycle.h"
#include "ConnectedState.h"
#include "clans/ClanManager.h"
#include "IConnectedStateHandler.h"
#include "ILogger.h"
#include "RaceManager.h"
#include "SocketHelper.h"
#include "Wiznet.h"

/* command procedures needed */
DECLARE_DO_FUN(do_help);
DECLARE_DO_FUN(do_look);
DECLARE_DO_FUN(do_skills);
DECLARE_DO_FUN(do_outfit);

#include <signal.h>

/*
 * Socket and TCP/IP stuff.
 */
#include <fcntl.h>
#include <netdb.h>
#include <sys/socket.h>
#include "telnet.h"
#include "PlayerCharacter.h"

const unsigned char go_ahead_str[] = {IAC, GA, '\0'};

#if !defined(isascii)
#define isascii(c) ((c) < 0200)
#endif

/*
 * Global variables.
 */
DESCRIPTOR_DATA *descriptor_list; /* All open descriptors		*/
DESCRIPTOR_DATA *d_next;		  /* Next descriptor in loop	*/
FILE *fpReserve;				  /* Reserved file handle		*/
bool merc_down;					  /* Shutdown			*/
bool wizlock;					  /* Game is wizlocked		*/
bool newlock;					  /* Game is newlocked		*/
time_t current_time;			  /* time of this pulse */
bool MOBtrigger = TRUE;			  /* act() switch                 */

/*
 * OS-dependent local functions.
 */
void game_loop_unix args((Game * game, int control));
void init_descriptor args((int control));
bool read_from_descriptor args((DESCRIPTOR_DATA * d));

/*
 * Other local functions (OS-independent).
 */
void nanny args((Game * game, DESCRIPTOR_DATA *d, char *argument));
bool process_output args((DESCRIPTOR_DATA * d, bool fPrompt));
void read_from_buffer args((DESCRIPTOR_DATA * d));
void stop_idling args((Character * ch));
void bust_a_prompt args((Character * ch));

extern ClanManager *clan_manager;
extern RaceManager *race_manager;

/* Needs to be global because of do_copyover */
extern int port, control;

void game_loop_unix(Game *game, int control)
{
	static struct timeval null_time;
	struct timeval last_time;

	signal(SIGPIPE, SIG_IGN);
	gettimeofday(&last_time, NULL);
	current_time = (time_t)last_time.tv_sec;

	/* Main loop */
	while (!merc_down)
	{
		fd_set in_set;
		fd_set out_set;
		fd_set exc_set;
		DESCRIPTOR_DATA *d;
		int maxdesc;

		/*
		 * Poll all active descriptors.
		 */
		FD_ZERO(&in_set);
		FD_ZERO(&out_set);
		FD_ZERO(&exc_set);
		FD_SET(control, &in_set);
		maxdesc = control;
		for (d = descriptor_list; d; d = d->next)
		{
			maxdesc = UMAX(maxdesc, d->descriptor);
			FD_SET(d->descriptor, &in_set);
			FD_SET(d->descriptor, &out_set);
			FD_SET(d->descriptor, &exc_set);
		}

		if (select(maxdesc + 1, &in_set, &out_set, &exc_set, &null_time) < 0)
		{
			perror("Game_loop: select: poll");
			exit(1);
		}

		/*
		 * New connection?
		 */
		if (FD_ISSET(control, &in_set))
			init_descriptor(control);

		/*
		 * Kick out the freaky folks.
		 */
		for (d = descriptor_list; d != NULL; d = d_next)
		{
			d_next = d->next;
			if (FD_ISSET(d->descriptor, &exc_set))
			{
				FD_CLR(d->descriptor, &in_set);
				FD_CLR(d->descriptor, &out_set);
				if (d->character && d->connected == ConnectedState::Playing)
					save_char_obj(d->character);
				d->outtop = 0;
				SocketHelper::close_socket(d);
			}
		}

		/*
		 * Process input.
		 */
		for (d = descriptor_list; d != NULL; d = d_next)
		{
			d_next = d->next;
			d->fcommand = FALSE;

			if (FD_ISSET(d->descriptor, &in_set))
			{
				if (d->character != NULL)
					d->character->timer = 0;
				if (!read_from_descriptor(d))
				{
					FD_CLR(d->descriptor, &out_set);
					if (d->character != NULL && d->connected == ConnectedState::Playing)
						save_char_obj(d->character);
					d->outtop = 0;
					SocketHelper::close_socket(d);
					continue;
				}
			}

			if (d->character != NULL && d->character->daze > 0)
				--d->character->daze;

			if (d->character != NULL && d->character->wait > 0)
			{
				--d->character->wait;
				continue;
			}

			read_from_buffer(d);
			if (d->incomm[0] != '\0')
			{
				d->fcommand = TRUE;
				stop_idling(d->character);

				/* OLC */
				if (d->showstr_point)
				{
					show_string(d, d->incomm);
				}
				else if (d->pString)
				{
					string_add(d->character, d->incomm);
				}
				else
				{
					switch (d->connected)
					{
					case ConnectedState::Playing:
						if (!run_olc_editor(d))
							substitute_alias(d, d->incomm);
						break;
					default:
						nanny(game, d, d->incomm);
						break;
					}
				}
				d->incomm[0] = '\0';
			}
		}

		/*
		 * Autonomous game motion.
		 */
		update_handler();

		/*
		 * Output.
		 */
		for (d = descriptor_list; d != NULL; d = d_next)
		{
			d_next = d->next;

			if ((d->fcommand || d->outtop > 0) && FD_ISSET(d->descriptor, &out_set))
			{
				if (!process_output(d, TRUE))
				{
					if (d->character != NULL && d->connected == ConnectedState::Playing)
						save_char_obj(d->character);
					d->outtop = 0;
					SocketHelper::close_socket(d);
				}
			}
		}

		/*
		 * Synchronize to a clock.
		 * Sleep( last_time + 1/PULSE_PER_SECOND - now ).
		 * Careful here of signed versus unsigned arithmetic.
		 */
		{
			struct timeval now_time;
			long secDelta;
			long usecDelta;

			gettimeofday(&now_time, NULL);
			usecDelta = ((int)last_time.tv_usec) - ((int)now_time.tv_usec) + 1000000 / PULSE_PER_SECOND;
			secDelta = ((int)last_time.tv_sec) - ((int)now_time.tv_sec);
			while (usecDelta < 0)
			{
				usecDelta += 1000000;
				secDelta -= 1;
			}

			while (usecDelta >= 1000000)
			{
				usecDelta -= 1000000;
				secDelta += 1;
			}

			if (secDelta > 0 || (secDelta == 0 && usecDelta > 0))
			{
				struct timeval stall_time;

				stall_time.tv_usec = usecDelta;
				stall_time.tv_sec = secDelta;
				if (select(0, NULL, NULL, NULL, &stall_time) < 0)
				{
					perror("Game_loop: select: stall");
					exit(1);
				}
			}
		}

		gettimeofday(&last_time, NULL);
		current_time = (time_t)last_time.tv_sec;
	}

	return;
}

void init_descriptor(int control)
{
	char buf[MAX_STRING_LENGTH];
	DESCRIPTOR_DATA *dnew;
	struct sockaddr_in sock;
	struct hostent *from;
	int desc;
	socklen_t size;

	size = sizeof(sock);
	getsockname(control, (struct sockaddr *)&sock, &size);
	if ((desc = accept(control, (struct sockaddr *)&sock, &size)) < 0)
	{
		perror("New_descriptor: accept");
		return;
	}

#if !defined(FNDELAY)
#define FNDELAY O_NDELAY
#endif

	if (fcntl(desc, F_SETFL, FNDELAY) == -1)
	{
		perror("New_descriptor: fcntl: FNDELAY");
		return;
	}

	/*
	 * Cons a new descriptor.
	 */
	dnew = new_descriptor(); /* new_descriptor now also allocates things */
	dnew->descriptor = desc;

	size = sizeof(sock);
	if (getpeername(desc, (struct sockaddr *)&sock, &size) < 0)
	{
		perror("New_descriptor: getpeername");
		dnew->host = str_dup("(unknown)");
	}
	else
	{
		/*
		 * Would be nice to use inet_ntoa here but it takes a struct arg,
		 * which ain't very compatible between gcc and system libraries.
		 */
		int addr;

		addr = ntohl(sock.sin_addr.s_addr);
		snprintf(buf, sizeof(buf), "%d.%d.%d.%d",
				 (addr >> 24) & 0xFF, (addr >> 16) & 0xFF,
				 (addr >> 8) & 0xFF, (addr) & 0xFF);
		snprintf(log_buf, 2 * MAX_INPUT_LENGTH, "Sock.sinaddr:  %s", buf);
		logger->log_string(log_buf);
		from = gethostbyaddr((char *)&sock.sin_addr,
							 sizeof(sock.sin_addr), AF_INET);
		dnew->host = str_dup(from ? from->h_name : buf);
	}

	/*
	 * Swiftest: I added the following to ban sites.  I don't
	 * endorse banning of sites, but Copper has few descriptors now
	 * and some people from certain sites keep abusing access by
	 * using automated 'autodialers' and leaving connections hanging.
	 *
	 * Furey: added suffix check by request of Nickel of HiddenWorlds.
	 */
	if (check_ban(dnew->host, BAN_ALL))
	{
		SocketHelper::write_to_descriptor(desc,
							"Your site has been banned from this mud.\n\r", 0);
		close(desc);
		free_descriptor(dnew);
		return;
	}
	/*
	 * Init descriptor data.
	 */
	dnew->next = descriptor_list;
	descriptor_list = dnew;

	/*
	 * Send the greeting.
	 */
	{
		extern char *help_greeting;
		if (help_greeting[0] == '.')
			SocketHelper::write_to_buffer(dnew, help_greeting + 1, 0);
		else
			SocketHelper::write_to_buffer(dnew, help_greeting, 0);
	}

	return;
}

bool read_from_descriptor(DESCRIPTOR_DATA *d)
{
	unsigned int iStart;

	/* Hold horses if pending command already. */
	if (d->incomm[0] != '\0')
		return TRUE;

	/* Check for overflow. */
	iStart = strlen((const char *)d->inbuf);
	if (iStart >= sizeof(d->inbuf) - 10)
	{
		snprintf(log_buf, 2 * MAX_INPUT_LENGTH, "%s input overflow!", d->host);
		logger->log_string(log_buf);
		SocketHelper::write_to_descriptor(d->descriptor,
							(char *)"\n\r*** PUT A LID ON IT!!! ***\n\r", 0);
		return FALSE;
	}

	/* Snarf input. */
	for (;;)
	{
		int nRead;

		nRead = read(d->descriptor, d->inbuf + iStart,
					 sizeof(d->inbuf) - 10 - iStart);
		if (nRead > 0)
		{
			iStart += nRead;
			if (d->inbuf[iStart - 1] == '\n' || d->inbuf[iStart - 1] == '\r')
				break;
		}
		else if (nRead == 0)
		{
			logger->log_string("EOF encountered on read.");
			return FALSE;
		}
		else if (errno == EWOULDBLOCK)
			break;
		else
		{
			perror("Read_from_descriptor");
			return FALSE;
		}
	}

	d->inbuf[iStart] = '\0';
	return TRUE;
}

/*
 * Transfer one line from input buffer to input line.
 */
void read_from_buffer(DESCRIPTOR_DATA *d)
{
	int i, j, k;

	/*
	 * Hold horses if pending command already.
	 */
	if (d->incomm[0] != '\0')
		return;

	/*
	 * Look for at least one new line.
	 */
	for (i = 0; d->inbuf[i] != '\n' && d->inbuf[i] != '\r'; i++)
	{
		if (d->inbuf[i] == '\0')
			return;
	}

	/*
	 * Canonical input processing.
	 */
	for (i = 0, k = 0; d->inbuf[i] != '\n' && d->inbuf[i] != '\r'; i++)
	{
		if (k >= MAX_INPUT_LENGTH - 2)
		{
			SocketHelper::write_to_descriptor(d->descriptor, (char *)"Line too long.\n\r", 0);

			/* skip the rest of the line */
			for (; d->inbuf[i] != '\0'; i++)
			{
				if (d->inbuf[i] == '\n' || d->inbuf[i] == '\r')
					break;
			}
			d->inbuf[i] = '\n';
			d->inbuf[i + 1] = '\0';
			break;
		}

		if (d->inbuf[i] == '\b' && k > 0)
			--k;
		else if (isascii(d->inbuf[i]) && isprint(d->inbuf[i]))
			d->incomm[k++] = d->inbuf[i];
	}

	/*
	 * Finish off the line.
	 */
	if (k == 0)
		d->incomm[k++] = ' ';
	d->incomm[k] = '\0';

	/*
	 * Deal with bozos with #repeat 1000 ...
	 */

	if (k > 1 || d->incomm[0] == '!')
	{
		if (d->incomm[0] != '!' && strcmp(d->incomm, d->inlast))
		{
			d->repeat = 0;
		}
		else
		{
			if (++d->repeat >= 25 && d->character && d->connected == ConnectedState::Playing)
			{
				snprintf(log_buf, 2 * MAX_INPUT_LENGTH, "%s input spamming!", d->host);
				logger->log_string(log_buf);
				Wiznet::instance()->report((char *)"Spam spam spam $N spam spam spam spam spam!",
										   d->character, NULL, WIZ_SPAM, 0, d->character->getTrust());
				if (d->incomm[0] == '!')
					Wiznet::instance()->report(d->inlast, d->character, NULL, WIZ_SPAM, 0,
											   d->character->getTrust());
				else
					Wiznet::instance()->report(d->incomm, d->character, NULL, WIZ_SPAM, 0,
											   d->character->getTrust());

				d->repeat = 0;
				/*
						SocketHelper::write_to_descriptor( d->descriptor,
							"\n\r*** PUT A LID ON IT!!! ***\n\r", 0 );
						strcpy( d->incomm, "quit" );
				*/
			}
		}
	}

	/*
	 * Do '!' substitution.
	 */
	if (d->incomm[0] == '!')
		strcpy(d->incomm, d->inlast);
	else
		strcpy(d->inlast, d->incomm);

	/*
	 * Shift the input buffer.
	 */
	while (d->inbuf[i] == '\n' || d->inbuf[i] == '\r')
		i++;
	for (j = 0; (d->inbuf[j] = d->inbuf[i + j]) != '\0'; j++)
		;
	return;
}

/*
 * Low level output function.
 */
bool process_output(DESCRIPTOR_DATA *d, bool fPrompt)
{
	extern bool merc_down;

	/*
	 * Bust a prompt.
	 */
	if (!merc_down)
	{
		if (d->showstr_point)
		{
			SocketHelper::write_to_buffer(d, "[Hit Return to continue]\n\r", 0);
		}
		else if (fPrompt && d->pString && d->connected == ConnectedState::Playing)
		{
			SocketHelper::write_to_buffer(d, "> ", 2);
		}
		else if (fPrompt && d->connected == ConnectedState::Playing)
		{
			Character *ch;
			Character *victim;

			ch = d->character;

			/* battle prompt */
			if ((victim = ch->fighting) != NULL && can_see(ch, victim))
			{
				int percent;
				char wound[100];
				char *pbuff;
				char buf[MAX_STRING_LENGTH];
				char buffer[MAX_STRING_LENGTH * 2];

				if (victim->max_hit > 0)
				{
					percent = victim->hit * 100 / victim->max_hit;
				}
				else
				{
					percent = -1;
				}

				if (percent >= 100)
				{
					snprintf(wound, sizeof(wound), "is in excellent condition.");
				}
				else if (percent >= 90)
				{
					snprintf(wound, sizeof(wound), "has a few scratches.");
				}
				else if (percent >= 75)
				{
					snprintf(wound, sizeof(wound), "has some small wounds and bruises.");
				}
				else if (percent >= 50)
				{
					snprintf(wound, sizeof(wound), "has quite a few wounds.");
				}
				else if (percent >= 30)
				{
					snprintf(wound, sizeof(wound), "has some big nasty wounds and scratches.");
				}
				else if (percent >= 15)
				{
					snprintf(wound, sizeof(wound), "looks pretty hurt.");
				}
				else if (percent >= 0)
				{
					snprintf(wound, sizeof(wound), "is in awful condition.");
				}
				else
				{
					snprintf(wound, sizeof(wound), "is bleeding to death.");
				}

				snprintf(buf, sizeof(buf), "%s %s \n\r",
						 IS_NPC(victim) ? victim->short_descr : victim->getName().c_str(), wound);
				buf[0] = UPPER(buf[0]);
				pbuff = buffer;
				colourconv(pbuff, buf, d->character);
				SocketHelper::write_to_buffer(d, buffer, 0);
			}

			ch = d->original ? d->original : d->character;
			if (!IS_SET(ch->comm, COMM_COMPACT))
				SocketHelper::write_to_buffer(d, "\n\r", 2);

			if (IS_SET(ch->comm, COMM_PROMPT))
				bust_a_prompt(d->character);

			if (IS_SET(ch->comm, COMM_TELNET_GA))
				SocketHelper::write_to_buffer(d, (const char *)go_ahead_str, 0);
		}

		/*
		 * Short-circuit if nothing to write.
		 */
		if (d->outtop == 0)
			return TRUE;

		/*
		 * Snoop-o-rama.
		 */
		if (d->snoop_by != NULL)
		{
			if (d->character != NULL)
				SocketHelper::write_to_buffer(d->snoop_by, d->character->getName().c_str(), 0);
			SocketHelper::write_to_buffer(d->snoop_by, "> ", 2);
			SocketHelper::write_to_buffer(d->snoop_by, d->outbuf, d->outtop);
		}

		/*
		 * OS-dependent output.
		 */
		if (!SocketHelper::write_to_descriptor(d->descriptor, d->outbuf, d->outtop))
		{
			d->outtop = 0;
			return FALSE;
		}
		else
		{
			d->outtop = 0;
			return TRUE;
		}
	}
	return TRUE;
}

/*
 * Bust a prompt (player settable prompt)
 * coded by Morgenes for Aldara Mud
 */
void bust_a_prompt(Character *ch)
{
	char buf[MAX_STRING_LENGTH];
	char buf2[MAX_STRING_LENGTH];
	const char *str;
	const char *i;
	char *point;
	char *pbuff;
	char buffer[MAX_STRING_LENGTH * 2];
	char doors[MAX_INPUT_LENGTH];
	EXIT_DATA *pexit;
	bool found;
	const char *dir_name[] = {"N", "E", "S", "W", "U", "D"};
	int door;

	point = buf;
	str = ch->prompt;
	if (!str || str[0] == '\0')
	{
		snprintf(buf, sizeof(buf), "{c<%d to level>\n\r<%d/%dhp %d/%dm %d/%dmv>{x ",
				 (ch->level + 1) * exp_per_level(ch, ch->pcdata->points) - ch->exp,
				 ch->hit, ch->max_hit, ch->mana, ch->max_mana, ch->move,
				 ch->max_move);
		send_to_char(buf, ch);
		return;
	}

	if (IS_SET(ch->comm, COMM_AFK))
	{
		send_to_char("<AFK> ", ch);
		return;
	}

	while (*str != '\0')
	{
		if (*str != '%')
		{
			*point++ = *str++;
			continue;
		}
		++str;
		switch (*str)
		{
		default:
			i = " ";
			break;
		case 'e':
			found = FALSE;
			doors[0] = '\0';
			for (door = 0; door < 6; door++)
			{
				if ((pexit = ch->in_room->exit[door]) != NULL && pexit->u1.to_room != NULL && (can_see_room(ch, pexit->u1.to_room) || (IS_AFFECTED(ch, AFF_INFRARED) && !IS_AFFECTED(ch, AFF_BLIND))) && !IS_SET(pexit->exit_info, EX_CLOSED))
				{
					found = TRUE;
					strcat(doors, dir_name[door]);
				}
			}
			if (!found)
				strcat(buf, "none");
			snprintf(buf2, sizeof(buf2), "%s", doors);
			i = buf2;
			break;
		case 'c':
			snprintf(buf2, sizeof(buf2), "%s", "\n\r");
			i = buf2;
			break;
		case 'h':
			snprintf(buf2, sizeof(buf2), "%d", ch->hit);
			i = buf2;
			break;
		case 'H':
			snprintf(buf2, sizeof(buf2), "%d", ch->max_hit);
			i = buf2;
			break;
		case 'm':
			snprintf(buf2, sizeof(buf2), "%d", ch->mana);
			i = buf2;
			break;
		case 'M':
			snprintf(buf2, sizeof(buf2), "%d", ch->max_mana);
			i = buf2;
			break;
		case 'v':
			snprintf(buf2, sizeof(buf2), "%d", ch->move);
			i = buf2;
			break;
		case 'V':
			snprintf(buf2, sizeof(buf2), "%d", ch->max_move);
			i = buf2;
			break;
		case 'x':
			snprintf(buf2, sizeof(buf2), "%d", ch->exp);
			i = buf2;
			break;
		case 'X':
			snprintf(buf2, sizeof(buf2), "%d", IS_NPC(ch) ? 0 : (ch->level + 1) * exp_per_level(ch, ch->pcdata->points) - ch->exp);
			i = buf2;
			break;
		case 'g':
			snprintf(buf2, sizeof(buf2), "%ld", ch->gold);
			i = buf2;
			break;
		case 's':
			snprintf(buf2, sizeof(buf2), "%ld", ch->silver);
			i = buf2;
			break;
		case 'r':
			if (ch->in_room != NULL)
				snprintf(buf2, sizeof(buf2), "%s",
						 ((!IS_NPC(ch) && IS_SET(ch->act, PLR_HOLYLIGHT)) ||
						  (!IS_AFFECTED(ch, AFF_BLIND) && !room_is_dark(ch->in_room)))
							 ? ch->in_room->name
							 : "darkness");
			else
				snprintf(buf2, sizeof(buf2), " ");
			i = buf2;
			break;
		case 'R':
			if (IS_IMMORTAL(ch) && ch->in_room != NULL)
				snprintf(buf2, sizeof(buf2), "%d", ch->in_room->vnum);
			else
				snprintf(buf2, sizeof(buf2), " ");
			i = buf2;
			break;
		case 'z':
			if (IS_IMMORTAL(ch) && ch->in_room != NULL)
				snprintf(buf2, sizeof(buf2), "%s", ch->in_room->area->name);
			else
				snprintf(buf2, sizeof(buf2), " ");
			i = buf2;
			break;
		case '%':
			snprintf(buf2, sizeof(buf2), "%%");
			i = buf2;
			break;
		case 'o':
			snprintf(buf2, sizeof(buf2), "%s", olc_ed_name(ch));
			i = buf2;
			break;
		case 'O':
			snprintf(buf2, sizeof(buf2), "%s", olc_ed_vnum(ch));
			i = buf2;
			break;
		}
		++str;
		while ((*point = *i) != '\0')
			++point, ++i;
	}
	*point = '\0';
	pbuff = buffer;
	colourconv(pbuff, buf, ch);
	SocketHelper::write_to_buffer(ch->desc, buffer, 0);

	return;
}

/*
 * Deal with sockets that haven't logged in yet.
 */
void nanny(Game *game, DESCRIPTOR_DATA *d, char *argument)
{
	DESCRIPTOR_DATA *d_old, *d_next;
	char buf[MAX_STRING_LENGTH];
	char arg[MAX_INPUT_LENGTH];
	int must_have;
	Character *ch;
	int iClass, race, omorph, i, weapon;

	must_have = 0;

	/* Delete leading spaces UNLESS character is writing a note */
	if (d->connected != ConnectedState::NoteText)
	{
		while (isspace(*argument))
			argument++;
	}

	ch = d->character;

	auto handler = game->getConnectedStateManager()->createHandler(d);
	if (!handler) {
		// bug("Nanny: bad d->connected %d.", d->connected);
		// SocketHelper::close_socket(d);
		// return;
	} else {
		handler->handle(d, argument);
		return;
	}

	switch (d->connected)
	{
	default:
		bug("Nanny: bad d->connected %d.", d->connected);
		SocketHelper::close_socket(d);
		return;

	case ConnectedState::GetMorph:
		one_argument(argument, arg);

		if (!strcmp(arg, "help"))
		{
			argument = one_argument(argument, arg);
			if (argument[0] == '\0')
				do_help(ch, (char *)"werefolk");
			else
				do_help(ch, argument);
			SocketHelper::write_to_buffer(d,
							"What is your morph race (help for more information)? ", 0);
			break;
		}

		if (morph_lookup(argument) == -1)
		{
			int morph;
			SocketHelper::write_to_buffer(d, "That is not a valid race.\n\r", 0);
			SocketHelper::write_to_buffer(d, "The following races are available:\n\r  ", 0);
			for (morph = 0; morph_table[morph].name[0] != '\0'; morph++)
			{
				SocketHelper::write_to_buffer(d, morph_table[morph].name, 0);
				SocketHelper::write_to_buffer(d, " ", 1);
			}
			SocketHelper::write_to_buffer(d, "\n\r", 0);
			SocketHelper::write_to_buffer(d,
							"What is your morph race? (help for more information) ", 0);
			break;
		}

		ch->morph_form = morph_lookup(argument);
		/* add skills */
		for (i = 0; i < 5; i++)
		{
			if (morph_table[ch->morph_form].skills[i] == NULL)
				break;
			group_add(ch, morph_table[ch->morph_form].skills[i], FALSE);
		}

		SocketHelper::write_to_buffer(d, "The following races are available:\n\r  ", 0);
		for (auto race : race_manager->getAllRaces() ) {
			if (race->isPlayerRace() && race != race_manager->getRaceByName("werefolk")) {
				SocketHelper::write_to_buffer(d, race->getName().c_str(), 0);
				SocketHelper::write_to_buffer(d, " ", 1);
			}
		}

		SocketHelper::write_to_buffer(d, "\n\r", 0);
		SocketHelper::write_to_buffer(d, "What is your default race (help for more information)? ", 0);
		d->connected = ConnectedState::GetMorphOrig;
		break;

	case ConnectedState::GetMorphOrig:
		one_argument(argument, arg);

		if (!strcmp(arg, "help"))
		{
			argument = one_argument(argument, arg);
			if (argument[0] == '\0')
				do_help(ch, (char *)"race help");
			else
				do_help(ch, argument);
			SocketHelper::write_to_buffer(d,
							"What is your race (help for more information)? ", 0);
			break;
		}

		Race *omorph;

		if (!(omorph = race_manager->getRaceByName(argument)) || omorph == race_manager->getRaceByName("werefolk"))
		{
			SocketHelper::write_to_buffer(d, "That is not a valid race.\n\r", 0);
			SocketHelper::write_to_buffer(d, "The following races are available:\n\r  ", 0);
			for (auto race : race_manager->getAllRaces() ) {
				if (race->isPlayerRace() && race != race_manager->getRaceByName("werefolk")) {
					SocketHelper::write_to_buffer(d, race->getName().c_str(), 0);
					SocketHelper::write_to_buffer(d, " ", 1);
				}
			}

			SocketHelper::write_to_buffer(d, "\n\r", 0);
			SocketHelper::write_to_buffer(d,
							"What is your default race? (help for more information) ", 0);
			break;
		}

		ch->orig_form = omorph->getLegacyId();

		SocketHelper::write_to_buffer(d, "What is your sex (M/F)? ", 0);
		d->connected = ConnectedState::GetNewSex;
		break;

	case ConnectedState::GetNewSex:
		switch (argument[0])
		{
		case 'm':
		case 'M':
			ch->sex = SEX_MALE;
			ch->pcdata->true_sex = SEX_MALE;
			break;
		case 'f':
		case 'F':
			ch->sex = SEX_FEMALE;
			ch->pcdata->true_sex = SEX_FEMALE;
			break;
		default:
			SocketHelper::write_to_buffer(d, "That's not a sex.\n\rWhat IS your sex? ", 0);
			return;
		}

		strcpy(buf, "Select a class [");
		for (iClass = 0; iClass < MAX_CLASS; iClass++)
		{
			if (!can_be_class(ch, iClass))
				continue;

			if (iClass > 0)
				strcat(buf, " ");
			strcat(buf, class_table[iClass].name);
		}
		strcat(buf, "]: ");
		SocketHelper::write_to_buffer(d, buf, 0);
		d->connected = ConnectedState::GetNewClass;
		break;

	case ConnectedState::GetNewClass:
		iClass = class_lookup(argument);

		if (iClass == -1 || !can_be_class(ch, iClass))
		{
			SocketHelper::write_to_buffer(d,
							"That's not a class.\n\rWhat IS your class? ", 0);
			return;
		}

		ch->class_num = iClass;
		SET_BIT(ch->done, class_table[ch->class_num].flag);

		snprintf(log_buf, 2 * MAX_INPUT_LENGTH, "%s@%s new player.", ch->getName().c_str(), d->host);
		logger->log_string(log_buf);
		Wiznet::instance()->report((char *)"Newbie alert!  $N sighted.", ch, NULL, WIZ_NEWBIE, 0, 0);
		Wiznet::instance()->report(log_buf, NULL, NULL, WIZ_SITES, 0, ch->getTrust());

		SocketHelper::write_to_buffer(d, "\n\r", 0);

		group_add(ch, "rom basics", FALSE);
		group_add(ch, class_table[ch->class_num].base_group, FALSE);
		ch->pcdata->learned[gsn_recall] = 50;

		/*
		** The werefolk have to know what they're doing.
		** The check looks between 10 and 70 for skill,
		** so we'll start these guys off at 25
		*/
		if (ch->getRace() == race_manager->getRaceByName("werefolk"))
			ch->pcdata->learned[gsn_morph] = 25;

		SocketHelper::write_to_buffer(d, "Do you wish to customize this character?\n\r", 0);
		SocketHelper::write_to_buffer(d, "Customization takes time, but allows a wider range of skills and abilities.\n\r", 0);
		SocketHelper::write_to_buffer(d, "Customize (Y/N)? ", 0);
		d->connected = ConnectedState::DefaultChoice;
		break;

	case ConnectedState::GetReclass:
		iClass = class_lookup(argument);

		if (iClass == -1 || IS_SET(ch->done, class_table[iClass].flag) || !can_be_class(ch, iClass))
		{
			SocketHelper::write_to_buffer(d,
							"That class is not an option.\n\rWhat IS your class? ", 0);
			return;
		}

		ch->class_num = iClass;
		SET_BIT(ch->done, class_table[ch->class_num].flag);

		snprintf(log_buf, 2 * MAX_INPUT_LENGTH, "%s@%s reclasses as the %s class.", ch->getName().c_str(), d->host, class_table[ch->class_num].name);
		logger->log_string(log_buf);
		Wiznet::instance()->report((char *)"Reclass alert!  $N sighted.", ch, NULL, WIZ_NEWBIE, 0, 0);
		Wiznet::instance()->report(log_buf, NULL, NULL, WIZ_SITES, 0, ch->getTrust());

		group_add(ch, class_table[ch->class_num].base_group, FALSE);
		ch->gen_data = new_gen_data();
		ch->gen_data->points_chosen = ch->pcdata->points;
		do_help(ch, (char *)"group header");
		list_group_costs(ch);
		SocketHelper::write_to_buffer(d, "You already have the following skills:\n\r", 0);
		must_have = ch->pcdata->points + 15;
		do_skills(ch, (char *)"all");
		do_help(ch, (char *)"menu choice");
		d->connected = ConnectedState::ReclassCust;
		break;

	case ConnectedState::ReclassCust:
		send_to_char("\n\r", ch);
		if (!str_cmp(argument, "done"))
		{
			if (ch->pcdata->points < must_have)
				ch->pcdata->points = must_have;
			snprintf(buf, sizeof(buf), "Creation points: %d\n\r", ch->pcdata->points);
			send_to_char(buf, ch);
			snprintf(buf, sizeof(buf), "Experience per level: %d\n\r",
					 exp_per_level(ch, ch->gen_data->points_chosen));
			ch->exp = exp_per_level(ch, ch->gen_data->points_chosen) * ch->level;
			free_gen_data(ch->gen_data);
			ch->gen_data = NULL;
			send_to_char(buf, ch);
			char_from_room(ch);
			char_to_room(ch, ((PlayerCharacter *)ch)->getWasNoteRoom());
			d->connected = ConnectedState::Playing;
			break;
		}
		if (!parse_gen_groups(ch, argument))
			send_to_char(
				"Choices are: list,learned,premise,add,drop,info,help, and done.\n\r", ch);

		do_help(ch, (char *)"menu choice");
		break;

	case ConnectedState::DefaultChoice:
		SocketHelper::write_to_buffer(d, "\n\r", 2);
		switch (argument[0])
		{
		case 'y':
		case 'Y':
			ch->gen_data = new_gen_data();
			ch->gen_data->points_chosen = ch->pcdata->points;
			do_help(ch, (char *)"group header");
			list_group_costs(ch);
			SocketHelper::write_to_buffer(d, (char *)"You already have the following skills:\n\r", 0);
			do_skills(ch, (char *)"all");
			do_help(ch, (char *)"menu choice");
			d->connected = ConnectedState::GenGroups;
			break;
		case 'n':
		case 'N':
			group_add(ch, class_table[ch->class_num].default_group, TRUE);
			SocketHelper::write_to_buffer(d, "\n\r", 2);
			SocketHelper::write_to_buffer(d,
							"Please pick a weapon from the following choices:\n\r", 0);
			buf[0] = '\0';
			for (i = 0; weapon_table[i].name != NULL; i++)
				if (ch->pcdata->learned[*weapon_table[i].gsn] > 0)
				{
					strcat(buf, weapon_table[i].name);
					strcat(buf, " ");
				}
			strcat(buf, "\n\rYour choice? ");
			SocketHelper::write_to_buffer(d, buf, 0);
			d->connected = ConnectedState::PickWeapon;
			break;
		default:
			SocketHelper::write_to_buffer(d, "Please answer (Y/N)? ", 0);
			return;
		}
		break;

	case ConnectedState::PickWeapon:
		SocketHelper::write_to_buffer(d, "\n\r", 2);
		weapon = weapon_lookup(argument);
		if (weapon == -1 || ch->pcdata->learned[*weapon_table[weapon].gsn] <= 0)
		{
			SocketHelper::write_to_buffer(d,
							"That's not a valid selection. Choices are:\n\r", 0);
			buf[0] = '\0';
			for (i = 0; weapon_table[i].name != NULL; i++)
				if (ch->pcdata->learned[*weapon_table[i].gsn] > 0)
				{
					strcat(buf, weapon_table[i].name);
					strcat(buf, " ");
				}
			strcat(buf, "\n\rYour choice? ");
			SocketHelper::write_to_buffer(d, buf, 0);
			return;
		}

		ch->pcdata->learned[*weapon_table[weapon].gsn] = 40;
		SocketHelper::write_to_buffer(d, "\n\r", 2);
		do_help(ch, (char *)"motd");
		d->connected = ConnectedState::ReadMotd;
		break;

	case ConnectedState::GenGroups:
		send_to_char("\n\r", ch);
		if (!str_cmp(argument, "done"))
		{
			snprintf(buf, sizeof(buf), "Creation points: %d\n\r", ch->pcdata->points);
			send_to_char(buf, ch);
			snprintf(buf, sizeof(buf), "Experience per level: %d\n\r",
					 exp_per_level(ch, ch->gen_data->points_chosen));
			if (ch->pcdata->points < 40)
				ch->train = (40 - ch->pcdata->points + 1) / 2;
			free_gen_data(ch->gen_data);
			ch->gen_data = NULL;
			send_to_char(buf, ch);
			SocketHelper::write_to_buffer(d, "\n\r", 2);
			SocketHelper::write_to_buffer(d,
							"Please pick a weapon from the following choices:\n\r", 0);
			buf[0] = '\0';
			for (i = 0; weapon_table[i].name != NULL; i++)
				if (ch->pcdata->learned[*weapon_table[i].gsn] > 0)
				{
					strcat(buf, weapon_table[i].name);
					strcat(buf, " ");
				}
			strcat(buf, "\n\rYour choice? ");
			SocketHelper::write_to_buffer(d, buf, 0);
			d->connected = ConnectedState::PickWeapon;
			break;
		}

		if (!parse_gen_groups(ch, argument))
			send_to_char(
				"Choices are: list,learned,premise,add,drop,info,help, and done.\n\r", ch);

		do_help(ch, (char *)"menu choice");
		break;

	case ConnectedState::ReadImotd:
		SocketHelper::write_to_buffer(d, "\n\r", 2);
		do_help(ch, (char *)"motd");
		d->connected = ConnectedState::ReadMotd;
		break;

		/* states for new note system, (c)1995-96 erwin@pip.dknet.dk */
		/* ch MUST be PC here; have nwrite check for PC status! */

	case ConnectedState::NoteTo:
		handle_ConnectedStateNoteTo(d, argument);
		break;

	case ConnectedState::NoteSubject:
		handle_ConnectedStateNoteSubject(d, argument);
		break; /* subject */

	case ConnectedState::NoteExpire:
		handle_ConnectedStateNoteExpire(d, argument);
		break;

	case ConnectedState::NoteText:
		handle_ConnectedStateNoteText(d, argument);
		break;

	case ConnectedState::NoteFinish:
		handle_ConnectedStateNoteFinish(d, argument);
		break;

	case ConnectedState::ReadMotd:
		if (ch->pcdata == NULL || ch->pcdata->getPassword().empty())
		{
			SocketHelper::write_to_buffer(d, "Warning! Null password!\n\r", 0);
			SocketHelper::write_to_buffer(d, "Please report old password with bug.\n\r", 0);
			SocketHelper::write_to_buffer(d,
							"Type 'password null <new password>' to fix.\n\r", 0);
		}

		char_list.push_back(ch);
		d->connected = ConnectedState::Playing;
		send_to_char("The early bird may get the worm, but it's the second mouse that gets\n\r", ch);
		send_to_char("the cheese.\n\r\n\r\n\r", ch);
		reset_char(ch);

		if (ch->level == 0)
		{

			ch->perm_stat[class_table[ch->class_num].attr_prime] += 3;

			ch->level = 1;
			ch->exp = exp_per_level(ch, ch->pcdata->points);
			ch->hit = ch->max_hit;
			ch->mana = ch->max_mana;
			ch->move = ch->max_move;
			ch->train = 5;
			ch->practice = 5;
			snprintf(buf, sizeof(buf), "the %s", capitalize(class_table[ch->class_num].name));
			set_title(ch, buf);

			do_outfit(ch, (char *)"");
			obj_to_char(create_object(get_obj_index(OBJ_VNUM_MAP), 0), ch);

			char_to_room(ch, get_room_index(ROOM_VNUM_SCHOOL));
			send_to_char("\n\r", ch);
			do_help(ch, (char *)"NEWBIE INFO");
			SET_BIT(ch->act, PLR_COLOUR);
			SET_BIT(ch->act, PLR_AUTOASSIST);
			SET_BIT(ch->act, PLR_AUTOLOOT);
			SET_BIT(ch->act, PLR_AUTOSAC);
			SET_BIT(ch->act, PLR_AUTOGOLD);
			SET_BIT(ch->act, PLR_AUTOEXIT);
			send_to_char("\n\r", ch);
		}
		else if (ch->in_room != NULL)
		{
			char_to_room(ch, ch->in_room);
		}
		else if (IS_IMMORTAL(ch))
		{
			char_to_room(ch, get_room_index(ROOM_VNUM_CHAT));
		}
		else
		{
			char_to_room(ch, get_room_index(ROOM_VNUM_TEMPLE));
		}

		/* We don't want them showing up in the note room */
		if (ch->in_room == get_room_index(ROOM_VNUM_NOTE))
		{
			char_from_room(ch);
			char_to_room(ch, get_room_index(ROOM_VNUM_LIMBO));
		}

		act("$n has entered the game.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
		do_look(ch, (char *)"auto");

		Wiznet::instance()->report((char *)"$N has left real life behind.", ch, NULL,
								   WIZ_LOGINS, WIZ_SITES, ch->getTrust());

		if (ch->pet != NULL)
		{
			char_to_room(ch->pet, ch->in_room);
			act("$n has entered the game.", ch->pet, NULL, NULL, TO_ROOM, POS_RESTING);
		}

		send_to_char("\n", ch);
		do_board(ch, (char *)""); /* Show board status */
		break;
	}

	return;
}

void stop_idling(Character *ch)
{
	if (ch == NULL || ch->desc == NULL || ch->desc->connected != ConnectedState::Playing || ch->was_in_room == NULL || ch->in_room != get_room_index(ROOM_VNUM_LIMBO))
		return;

	ch->timer = 0;
	char_from_room(ch);
	char_to_room(ch, ch->was_in_room);
	ch->was_in_room = NULL;
	act("$n has returned from the void.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
	return;
}

/*
 * Write to one char.
 */
void send_to_char_bw(const char *txt, Character *ch)
{
	if (txt && ch->desc)
		SocketHelper::write_to_buffer(ch->desc, txt, strlen(txt));
	return;
}

/*
 * Write to one char, new colour version, by Lope.
 */
void send_to_char(const char *txt, Character *ch)
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
			SocketHelper::write_to_buffer(ch->desc, buf, point2 - buf);
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
			SocketHelper::write_to_buffer(ch->desc, buf, point2 - buf);
		}
	}
	return;
}

/*
 * Send a page to one char.
 */
void page_to_char_bw(const char *txt, Character *ch)
{
	if (!txt || !ch->desc)

		if (ch->lines == 0)
		{
			send_to_char(txt, ch);
			return;
		}

	ch->desc->showstr_head = new char[strlen(txt) + 1];
	strcpy(ch->desc->showstr_head, txt);
	ch->desc->showstr_point = ch->desc->showstr_head;
	show_string(ch->desc, (char *)"");
}

/*
 * Page to one char, new colour version, by Lope.
 */
void page_to_char(const char *txt, Character *ch)
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
			ch->desc->showstr_head = new char[strlen(buf) + 1];
			strcpy(ch->desc->showstr_head, buf);
			ch->desc->showstr_point = ch->desc->showstr_head;
			show_string(ch->desc, (char *)"");
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
			ch->desc->showstr_head = new char[strlen(buf) + 1];
			strcpy(ch->desc->showstr_head, buf);
			ch->desc->showstr_point = ch->desc->showstr_head;
			show_string(ch->desc, (char *)"");
		}
	}
	return;
}

/* string pager */
void show_string(struct descriptor_data *d, char *input)
{
	char buffer[4 * MAX_STRING_LENGTH];
	char buf[MAX_INPUT_LENGTH];
	char *scan, *chk;
	int lines = 0, toggle = 1;
	int show_lines;

	one_argument(input, buf);
	if (buf[0] != '\0')
	{
		if (d->showstr_head)
		{
			delete d->showstr_head;
			d->showstr_head = NULL;
		}
		d->showstr_point = 0;
		return;
	}

	if (d->character)
		show_lines = d->character->lines;
	else
		show_lines = 0;

	for (scan = buffer;; scan++, d->showstr_point++)
	{
		if (((*scan = *d->showstr_point) == '\n' || *scan == '\r') && (toggle = -toggle) < 0)
			lines++;

		else if (!*scan || (show_lines > 0 && lines >= show_lines))
		{
			*scan = '\0';
			SocketHelper::write_to_buffer(d, buffer, strlen(buffer));
			for (chk = d->showstr_point; isspace(*chk); chk++)
				;
			if (!*chk)
			{
				if (d->showstr_head)
				{
					delete[] d->showstr_head;
					d->showstr_head = NULL;
				}
				d->showstr_point = 0;
			}
			return;
		}
	}
	return;
}

/* quick sex fixer */
void fix_sex(Character *ch)
{
	if (ch->sex < 0 || ch->sex > 2)
		ch->sex = IS_NPC(ch) ? 0 : ch->pcdata->true_sex;
}

void act_string(const char *format, Character *to, Character *ch, Character *vch, OBJ_DATA *obj1, OBJ_DATA *obj2, const void *arg1, const void *arg2)
{
	static char *const he_she[] = {(char *)"it", (char *)"he", (char *)"she"};
	static char *const him_her[] = {(char *)"it", (char *)"him", (char *)"her"};
	static char *const his_her[] = {(char *)"its", (char *)"his", (char *)"her"};

	const char *str;
	const char *i = NULL;
	char *point;
	char buf[MAX_STRING_LENGTH];
	char fname[MAX_INPUT_LENGTH];
	char *pbuff;
	char buffer[MAX_STRING_LENGTH * 2];

	point = buf;
	str = format;
	while (*str != '\0')
	{
		if (*str != '$')
		{
			*point++ = *str++;
			continue;
		}
		++str;
		i = " <@@@> ";
		if (!arg2 && *str >= 'A' && *str <= 'Z')
		{
			bug("Act: missing arg2 for code %d.", *str);
			i = " <@@@> ";
		}
		else
		{
			switch (*str)
			{
			default:
				bug("Act: bad code %d.", *str);
				i = "";
				break;
			/* Thx alex for 't' idea */
			case 't':
				i = (char *)arg1;
				break;
			case 'T':
				i = (char *)arg2;
				break;
			case 'n':
				i = (char *)PERS(ch, to);
				break;
			case 'N':
				i = (char *)PERS(vch, to);
				break;
			case 'e':
				i = he_she[URANGE(0, ch->sex, 2)];
				break;
			case 'E':
				i = he_she[URANGE(0, vch->sex, 2)];
				break;
			case 'm':
				i = him_her[URANGE(0, ch->sex, 2)];
				break;
			case 'M':
				i = him_her[URANGE(0, vch->sex, 2)];
				break;
			case 's':
				i = his_her[URANGE(0, ch->sex, 2)];
				break;
			case 'S':
				i = his_her[URANGE(0, vch->sex, 2)];
				break;

			case 'p':
				i = can_see_obj(to, obj1)
						? obj1->short_descr
						: (char *)"something";
				break;

			case 'P':
				i = can_see_obj(to, obj2)
						? obj2->short_descr
						: (char *)"something";
				break;

			case 'd':
				if (arg2 == NULL || ((char *)arg2)[0] == '\0')
				{
					i = "door";
				}
				else
				{
					one_argument((char *)arg2, fname);
					i = fname;
				}
				break;
			}
		}

		++str;
		while ((*point = *i) != '\0')
		{
			++point, ++i;
		}
	}

	*point++ = '\n';
	*point++ = '\r';
	*point = '\0';
	buf[0] = UPPER(buf[0]);
	if (to->desc)
	{
		pbuff = buffer;
		colourconv(pbuff, buf, to);
		SocketHelper::write_to_buffer(to->desc, buffer, 0);
	}
	else if (MOBtrigger)
	{
		mp_act_trigger(buf, to, ch, arg1, arg2, TRIG_ACT);
	}
}

/*
 * The colour version of the act( ) function, -Lope
 */
void act(const char *format, Character *ch, const void *arg1,
			 const void *arg2, int type, int min_pos)
{
	Character *to;
	Character *vch = (Character *)arg2;
	OBJ_DATA *obj1 = (OBJ_DATA *)arg1;
	OBJ_DATA *obj2 = (OBJ_DATA *)arg2;

	/*
	 * Discard null and zero-length messages.
	 */
	if (!format || !*format)
		return;

	/* discard null rooms and chars */
	if (!ch || !ch->in_room)
		return;

	to = ch->in_room->people;
	if (type == TO_VICT )
	{
		if (!vch)
		{
			bug("Act: null vch with TO_VICT.", 0);
			return;
		}

		if (!vch->in_room)
			return;

		to = vch->in_room->people;
	}

	if (type == TO_AREA || type == TO_AREA_OUTSIDE_ROOM )
	{
		for (std::list<Character *>::iterator it = char_list.begin(); it != char_list.end(); it++)
		{
			to = *it;
			if (to->in_room == NULL)
				continue;

			if (to == ch)
				continue;
			if (type == TO_AREA_OUTSIDE_ROOM && to->in_room == ch->in_room)
				continue;
			if (to->in_room->area != ch->in_room->area)
				continue;

			act_string(format, to, ch, vch, obj1, obj2, arg1, arg2);
		}
	}
	else
	{
		for (; to; to = to->next_in_room)
		{
			if (!to->desc || to->position < min_pos)
				continue;

			if ((type == TO_CHAR ) && to != ch)
				continue;
			if (type == TO_VICT && (to != vch || to == ch))
				continue;
			if (type == TO_ROOM && to == ch)
				continue;
			if (type == TO_NOTVICT && (to == ch || to == vch))
				continue;

			act_string(format, to, ch, vch, obj1, obj2, arg1, arg2);
		}
	}
	return;
}

int colour(char type, Character *ch, char *string)
{
	char code[20];

	if (IS_NPC(ch))
		return (0);

	switch (type)
	{
	default:
		snprintf(code, sizeof(code), CLEAR);
		break;
	case 'x':
		snprintf(code, sizeof(code), CLEAR);
		break;
	case 'b':
		snprintf(code, sizeof(code), C_BLUE);
		break;
	case 'c':
		snprintf(code, sizeof(code), C_CYAN);
		break;
	case 'g':
		snprintf(code, sizeof(code), C_GREEN);
		break;
	case 'm':
		snprintf(code, sizeof(code), C_MAGENTA);
		break;
	case 'r':
		snprintf(code, sizeof(code), C_RED);
		break;
	case 'w':
		snprintf(code, sizeof(code), C_WHITE);
		break;
	case 'y':
		snprintf(code, sizeof(code), C_YELLOW);
		break;
	case 'B':
		snprintf(code, sizeof(code), C_B_BLUE);
		break;
	case 'C':
		snprintf(code, sizeof(code), C_B_CYAN);
		break;
	case 'G':
		snprintf(code, sizeof(code), C_B_GREEN);
		break;
	case 'M':
		snprintf(code, sizeof(code), C_B_MAGENTA);
		break;
	case 'R':
		snprintf(code, sizeof(code), C_B_RED);
		break;
	case 'W':
		snprintf(code, sizeof(code), C_B_WHITE);
		break;
	case 'Y':
		snprintf(code, sizeof(code), C_B_YELLOW);
		break;
	case 'D':
		snprintf(code, sizeof(code), C_D_GREY);
		break;
	/* Blizzy's random colour code */
	case 'e':
		switch (number_range(1, 14))
		{
		case 1:
			snprintf(code, sizeof(code), C_BLUE);
			break;
		case 2:
			snprintf(code, sizeof(code), C_CYAN);
			break;
		default:
			snprintf(code, sizeof(code), C_GREEN);
			break;
		case 3:
			snprintf(code, sizeof(code), C_MAGENTA);
			break;
		case 4:
			snprintf(code, sizeof(code), C_RED);
			break;
		case 5:
			snprintf(code, sizeof(code), C_WHITE);
			break;
		case 6:
			snprintf(code, sizeof(code), C_YELLOW);
			break;
		case 7:
			snprintf(code, sizeof(code), C_B_BLUE);
			break;
		case 8:
			snprintf(code, sizeof(code), C_B_CYAN);
			break;
		case 9:
			snprintf(code, sizeof(code), C_B_GREEN);
			break;
		case 10:
			snprintf(code, sizeof(code), C_B_MAGENTA);
			break;
		case 11:
			snprintf(code, sizeof(code), C_B_RED);
			break;
		case 12:
			snprintf(code, sizeof(code), C_B_WHITE);
			break;
		case 13:
			snprintf(code, sizeof(code), C_B_YELLOW);
			break;
		case 14:
			snprintf(code, sizeof(code), C_D_GREY);
			break;
		}
		break;

	case '*':
		snprintf(code, sizeof(code), "%c", 007);
		break;
	case '/':
		snprintf(code, sizeof(code), "%c", 012);
		break;
	case '{':
		snprintf(code, sizeof(code), "%c", '{');
		break;
	}

	char *p = code;
	while (*p != '\0')
	{
		*string = *p++;
		*++string = '\0';
	}

	return (strlen(code));
}

void colourconv(char *buffer, const char *txt, Character *ch)
{
	const char *point;
	int skip = 0;

	if (ch->desc && txt)
	{
		if (IS_SET(ch->act, PLR_COLOUR))
		{
			for (point = txt; *point; point++)
			{
				if (*point == '{')
				{
					point++;
					skip = colour(*point, ch, buffer);
					while (skip-- > 0)
						++buffer;
					continue;
				}
				*buffer = *point;
				*++buffer = '\0';
			}
			*buffer = '\0';
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
				*buffer = *point;
				*++buffer = '\0';
			}
			*buffer = '\0';
		}
	}
	return;
}

int strlen_color(char *argument)
{
	char *str;
	int strlength;

	if (argument == NULL || argument[0] == '\0')
		return 0;

	strlength = 0;
	str = argument;

	while (*str != '\0')
	{
		if (*str != '{')
		{
			str++;
			strlength++;
			continue;
		}

		if (*(++str) == '{')
			strlength++;

		str++;
	}
	return strlength;
}

/* source: EOD, by John Booth <???> */
void printf_to_char(Character *ch, const char *fmt, ...)
{
	char buf[MAX_STRING_LENGTH];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buf, MSL, fmt, args);
	va_end(args);

	send_to_char(buf, ch);
}
