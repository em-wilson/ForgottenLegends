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

#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "merc.h"
#include "board.h"
#include "recycle.h"
#include "clans/ClanManager.h"
#include "ConnectedState.h"
#include "PlayerCharacter.h"
#include "Room.h"
#include "SocketHelper.h"

DECLARE_DO_FUN( do_help                );

extern ClanManager * clan_manager;

/*
 
 Note Board system, (c) 1995-96 Erwin S. Andreasen, erwin@pip.dknet.dk
 =====================================================================
 
 Basically, the notes are split up into several boards. The boards do not
 exist physically, they can be read anywhere and in any position.
 
 Each of the note boards has its own file. Each of the boards can have its own
 "rights": who can read/write.
 
 Each character has an extra field added, namele the timestamp of the last note
 read by him/her on a certain board.
 
 The note entering system is changed too, making it more interactive. When
 entering a note, a character is put AFK and into a special CON_ state.
 Everything typed goes into the note.
 
 For the immortals it is possible to purge notes based on age. An Archive
 options is available which moves the notes older than X days into a special
 board. The file of this board should then be moved into some other directory
 during e.g. the startup script and perhaps renamed depending on date.
 
 Note that write_level MUST be >= read_level or else there will be strange
 output in certain functions.
 
 Board DEFAULT_BOARD must be at least readable by *everyone*.
 
*/ 

#define L_IMM (LEVEL_IMMORTAL)
#define L_SUP (MAX_LEVEL - 1) /* if not already defined */


BOARD_DATA boards[MAX_BOARD] =
{

{ "General",	"General discussion",           0,	2,	"all",	DEF_INCLUDE,21, NULL, FALSE },
{ "Ideas",	"Suggestion for improvement",		0,	2,	"all",	DEF_NORMAL, 60, NULL, FALSE }, 
{ "Announce",	"Announcements from Immortals", 0,	L_IMM,	"all",  DEF_NORMAL, 60, NULL, FALSE },
{ "Bugs",	"Typos, bugs, errors",				0,	1,	"imm",	DEF_NORMAL, 60, NULL, FALSE },
{ "Personal", 	"Personal messages",			0,	1,	"all",	DEF_EXCLUDE,28, NULL, FALSE },
{ "Clans",	"Clan notes",						5,	5,	"all",  DEF_NORMAL, 14, NULL, FALSE },
{ "Immortal",	"Immortal discussion",		L_IMM,	L_IMM,	"all",	DEF_NORMAL, 60, NULL, FALSE },

};

/* The prompt that the character is given after finishing a note with ~ or END */
const char * szFinishPrompt = "(C)ontinue, (V)iew, (P)ost or (F)orget it?";

long last_note_stamp = 0; /* To generate unique timestamps on notes */

#define BOARD_NOACCESS -1
#define BOARD_NOTFOUND -1

static bool next_board (Character *ch);


/* append this note to the given file */
static void append_note (FILE *fp, Note *note)
{
	fprintf (fp, "Sender  %s~\n", note->sender);
	fprintf (fp, "Date    %s~\n", note->date);
	fprintf (fp, "Stamp   %ld\n", note->date_stamp);
	fprintf (fp, "Expire  %ld\n", note->expire);
	fprintf (fp, "To      %s~\n", note->to_list);
	fprintf (fp, "Subject %s~\n", note->subject);
	fprintf (fp, "Text\n%s~\n\n", note->getText().c_str());
}

bool is_note_room( ROOM_INDEX_DATA *room ) {
    return room == get_room_index(ROOM_VNUM_NOTE);
}

/* Save a note in a given board */
void finish_note (BOARD_DATA *board, Note *note)
{
	FILE *fp;
	Note *p;
	char filename[200];
	
	/* The following is done in order to generate unique date_stamps */

	if (last_note_stamp >= current_time)
		note->date_stamp = ++last_note_stamp;
	else
	{
	    note->date_stamp = current_time;
	    last_note_stamp = current_time;
	}
	
	if (board->note_first) /* are there any notes in there now? */
	{
		for (p = board->note_first; p->next; p = p->next )
			; /* empty */
		
		p->next = note;
	}
	else /* nope. empty list. */
		board->note_first = note;

	/* append note to note file */		
	
	snprintf(filename, sizeof(filename), "%s%s%s", DATA_DIR, NOTE_DIR, board->short_name);
	
	fp = fopen (filename, "a");
	if (!fp)
	{
		bug ("Could not open one of the note files in append mode",0);
		board->changed = TRUE; /* set it to TRUE hope it will be OK later? */
		return;
	}
	
	append_note (fp, note);
	fclose (fp);
}

/* Find the number of a board */
int board_number (const BOARD_DATA *board)
{
	int i;
	
	for (i = 0; i < MAX_BOARD; i++)
		if (board == &boards[i])
			return i;

	return -1;
}

/* Find a board number based on  a string */
int board_lookup (const char *name)
{
	int i;
	
	for (i = 0; i < MAX_BOARD; i++)
		if (!str_cmp (boards[i].short_name, name))
			return i;

	return -1;
}

/* Remove list from the list. Do not free note */
static void unlink_note (BOARD_DATA *board, Note *note)
{
	Note *p;
	
	if (board->note_first == note)
		board->note_first = note->next;
	else
	{
		for (p = board->note_first; p && p->next != note; p = p->next);
		if (!p)
			bug ("unlink_note: could not find note.",0);
		else
			p->next = note->next;
	}
}

/* Find the nth note on a board. Return NULL if ch has no access to that note */
static Note* find_note (Character *ch, BOARD_DATA *board, int num)
{
	int count = 0;
	Note *p;
	
	for (p = board->note_first; p ; p = p->next)
			if (++count == num)
				break;
	
	if ( (count == num) && is_note_to (ch, p))
		return p;
	else
		return NULL;
	
}

/* save a single board */
static void save_board (BOARD_DATA *board)
{
	FILE *fp;
	char filename[200];
	char buf[200];
	Note *note;
	
	snprintf(filename, sizeof(filename), "%s%s%s", DATA_DIR, NOTE_DIR, board->short_name);
	
	fp = fopen (filename, "w");
	if (!fp)
	{
		snprintf(buf, sizeof(buf), "Error writing to: %s", filename);
		bug (buf, 0);
	}
	else
	{
		for (note = board->note_first; note ; note = note->next)
			append_note (fp, note);
			
		fclose (fp);
	}
}

/* Show one not to a character */
static void show_note_to_char (Character *ch, Note *note, int num)
{
	char buf[4*MAX_STRING_LENGTH];
	BUFFER *output;

	output = new_buf();
	/* Ugly colors ? */	
	snprintf(buf, sizeof(buf),
			 "[%4d] " "{Y%s{x: {g%s{x\n\r"
	         "{YDate{x:  %s\n\r"
			 "{YTo{x:    %s\n\r"
	         "{r==========================================================================={x\n\r"
	         "%s\n\r",
	         num, note->sender, note->subject,
	         note->date,
	         note->to_list,
	         note->getText().c_str());
	add_buf(output,buf);

	page_to_char (buf_string(output),ch);	         
}

/* Save changed boards */
void save_notes ()
{
	int i;
	 
	for (i = 0; i < MAX_BOARD; i++)
		if (boards[i].changed) /* only save changed boards */
			save_board (&boards[i]);
}

/* Load a single board */
static void load_board (BOARD_DATA *board)
{
	FILE *fp, *fp_archive;
	Note *last_note;
	char filename[200];
	
	snprintf(filename, sizeof(filename), "%s%s%s", DATA_DIR, NOTE_DIR, board->short_name);
	
	fp = fopen (filename, "r");
	
	/* Silently return */
	if (!fp)
		return;		
		
	/* Start note fetching. copy of db.c:load_notes() */

    last_note = NULL;

    for ( ; ; )
    {
        Note *pnote;
        char letter;

        do
        {
            letter = getc( fp );
            if ( feof(fp) )
            {
                fclose( fp );
                return;
            }
        }
        while ( isspace(letter) );
        ungetc( letter, fp );

        pnote             = new Note();

        if ( str_cmp( fread_word( fp ), "sender" ) )
            break;
        pnote->sender     = fread_string( fp );

        if ( str_cmp( fread_word( fp ), "date" ) )
            break;
        pnote->date       = fread_string( fp );

        if ( str_cmp( fread_word( fp ), "stamp" ) )
            break;
        pnote->date_stamp = fread_number( fp );

        if ( str_cmp( fread_word( fp ), "expire" ) )
            break;
        pnote->expire = fread_number( fp );

        if ( str_cmp( fread_word( fp ), "to" ) )
            break;
        pnote->to_list    = fread_string( fp );

        if ( str_cmp( fread_word( fp ), "subject" ) )
            break;
        pnote->subject    = fread_string( fp );

        if ( str_cmp( fread_word( fp ), "text" ) )
            break;
        char *note_text		= fread_string( fp );
        pnote->setText( note_text );
        free_string( note_text );
        
        pnote->next = NULL; /* jic */
        
        /* Should this note be archived right now ? */
        
        if (pnote->expire < current_time)
        {
			char archive_name[200];

			snprintf(archive_name, sizeof(archive_name), "%s%s%s.old", DATA_DIR, NOTE_DIR, board->short_name);
			fp_archive = fopen (archive_name, "a");
			if (!fp_archive)
				bug ("Could not open archive boards for writing",0);
			else
			{
				append_note (fp_archive, pnote);
				fclose (fp_archive); /* it might be more efficient to close this later */
			}

			delete pnote;
			board->changed = TRUE;
			continue;
			
        }
        

        if ( board->note_first == NULL )
            board->note_first = pnote;
        else
            last_note->next     = pnote;

        last_note         = pnote;
    }

    bug( "Load_notes: bad key word.", 0 );
    return; /* just return */
}

/* Initialize structures. Load all boards. */
void load_boards ()
{
	int i;
	
	for (i = 0; i < MAX_BOARD; i++)
		load_board (&boards[i]);
}

/* Returns TRUE if the specified note is address to ch */
bool is_note_to (Character *ch, Note *note)
{
	if (!str_cmp (ch->getName().c_str(), note->sender))
		return TRUE;
	
	if (is_full_name ("all", note->to_list))
		return TRUE;
		
	if (IS_IMMORTAL(ch) && ( 
		is_full_name ("imm", note->to_list) ||
		is_full_name ("imms", note->to_list) ||
		is_full_name ("immortal", note->to_list) ||
		is_full_name ("god", note->to_list) ||
		is_full_name ("gods", note->to_list) ||
		is_full_name ("immortals", note->to_list)))
		return TRUE;

	if ((ch->getTrust() == MAX_LEVEL) && (
		is_full_name ("imp", note->to_list) ||
		is_full_name ("imps", note->to_list) ||
		is_full_name ("implementor", note->to_list) ||
		is_full_name ("implementors", note->to_list)))
		return TRUE;
		
	if (is_full_name (ch->getName().c_str(), note->to_list))
		return TRUE;

	/* Allow a note to e.g. 40 to send to characters level 40 and above */		
	if (is_number(note->to_list) && ch->getTrust() >= atoi(note->to_list))
		return TRUE;
		
	/* Allow clan notes */
	if ( (IS_IMMORTAL(ch) && clan_manager->get_clan(note->to_list) )
		|| (ch->isClanned() && is_full_name(ch->getClan()->getName().c_str(), note->to_list)))
		return TRUE;

	return FALSE;
}

/* Return the number of unread notes 'ch' has in 'board' */
/* Returns BOARD_NOACCESS if ch has no access to board */
int unread_notes (Character *ch, BOARD_DATA *board)
{
	Note *note;
	time_t last_read;
	int count = 0;
	
	if (board->read_level > ch->getTrust())
		return BOARD_NOACCESS;
		
	last_read = ((PlayerCharacter*)ch)->last_note_read.at(board_number(board));
	
	for (note = board->note_first; note; note = note->next)
		if (is_note_to(ch, note) && ((long)last_read < (long)note->date_stamp))
			count++;
			
	return count;
}

/*
 * COMMANDS
 */

/* Start writing a note */
static void do_nwrite (Character *caller, char *argument)
{
	char *strtime;
	char buf[200];
	
	if (caller->isNPC()) /* NPC cannot post notes */
		return;

	PlayerCharacter *ch = (PlayerCharacter*)caller;
		
	if ( ch->getAdrenaline() > 0 )
	{
	    send_to_char("You are too excited from the chase to write a legible note.\n\r",ch);
	    return;
	}

	if (ch->getTrust() < ((PlayerCharacter*)ch)->board->write_level)
	{
		send_to_char ("You cannot post notes on this board.\n\r",ch);
		return;
	}
	
	/* continue previous note, if any text was written*/ 
	if (((PlayerCharacter*)ch)->in_progress && (((PlayerCharacter*)ch)->in_progress->getText().empty()))
	{
		send_to_char ("Note in progress cancelled because you did not manage to write any text \n\r"
		              "before losing link.\n\r\n\r",ch);
		delete ((PlayerCharacter*)ch)->in_progress;
		((PlayerCharacter*)ch)->in_progress = NULL;
	}
	
	
	if (!((PlayerCharacter*)ch)->in_progress)
	{
		((PlayerCharacter*)ch)->in_progress = new Note();
		((PlayerCharacter*)ch)->in_progress->sender = str_dup (ch->getName().c_str());

		/* convert to ascii. ctime returns a string which last character is \n, so remove that */	
		strtime = ctime (&current_time);
		strtime[strlen(strtime)-1] = '\0';
	
		((PlayerCharacter*)ch)->in_progress->date = str_dup (strtime);
	}

	act ("{G$n starts writing a note.{x", ch, NULL, NULL, TO_ROOM, POS_RESTING);

	/* Make AFK if not already */
	if (!IS_SET(ch->comm, COMM_AFK))
	    SET_BIT(ch->comm, COMM_AFK);

	/* Change their rooms so no one can crash the mud */
	ch->setWasNoteRoom(ch->in_room);
	char_from_room( ch );
	char_to_room( ch, get_room_index(ROOM_VNUM_NOTE));
	
	/* Begin writing the note ! */
	snprintf(buf, sizeof(buf), "You are now %s a new note on the %s board.\n\r"
	              "If you are using tintin, type #verbose to turn off alias expansion!\n\r\n\r",
	               ((PlayerCharacter*)ch)->in_progress->getText().empty() ? "posting" : "continuing",
	               ((PlayerCharacter*)ch)->board->short_name);
	send_to_char (buf,ch);
	
	snprintf(buf, sizeof(buf), "{YFrom{x:    %s\n\r\n\r", ch->getName().c_str());
	send_to_char (buf,ch);

	if (((PlayerCharacter*)ch)->in_progress->getText().empty()) /* Are we continuing an old note or not? */
	{
		switch (((PlayerCharacter*)ch)->board->force_type)
		{
		case DEF_NORMAL:
			snprintf(buf, sizeof(buf), "If you press Return, default recipient \"%s\" will be chosen.\n\r",
					  ((PlayerCharacter*)ch)->board->names);
			break;
		case DEF_INCLUDE:
			snprintf(buf, sizeof(buf), "The recipient list MUST include \"%s\". If not, it will be added automatically.\n\r",
						   ((PlayerCharacter*)ch)->board->names);
			break;
	
		case DEF_EXCLUDE:
			snprintf(buf, sizeof(buf), "The recipient of this note must NOT include: \"%s\".",
						   ((PlayerCharacter*)ch)->board->names);
	
			break;
		}			
		
		send_to_char (buf,ch);
		send_to_char ("\n\r{YTo{x:      ",ch);
	
		ch->desc->connected = ConnectedState::NoteTo;
		/* nanny takes over from here */
		
	}
	else /* we are continuing, print out all the fields and the note so far*/
	{
		snprintf(buf, sizeof(buf), "{YTo{x: %s\n\r"
		              "{YExpires{x: %s\n\r"
		              "{YSubject{x: %s\n\r", 
		               ((PlayerCharacter*)ch)->in_progress->to_list,
		               ctime(&((PlayerCharacter*)ch)->in_progress->expire),
		               ((PlayerCharacter*)ch)->in_progress->subject);
		send_to_char (buf,ch);
		send_to_char ("{GYour note so far:{x\n\r",ch);
		send_to_char (((PlayerCharacter*)ch)->in_progress->getText().c_str(),ch);
		
		send_to_char ("\n\rEnter text. Type ~ or END on an empty line to end note.\n\r"
		                    "=======================================================\n\r",ch);
		

		ch->desc->connected = ConnectedState::NoteText;		            

	}
	
}


/* Read next note in current group. If no more notes, go to next board */
static void do_nread (Character *ch, char *argument)
{
	Note *p;
	int count = 0, number;
	time_t *last_note = &((PlayerCharacter*)ch)->last_note_read.at(board_number(((PlayerCharacter*)ch)->board));
	
	if (!str_cmp(argument, "again"))
	{ /* read last note again */
	
	}
	else if (is_number (argument))
	{
		number = atoi(argument);
		
		for (p = ((PlayerCharacter*)ch)->board->note_first; p; p = p->next)
			if (++count == number)
				break;
		
		if (!p || !is_note_to(ch, p))
			send_to_char ("No such note.\n\r",ch);
		else
		{
			show_note_to_char (ch,p,count);
			*last_note =  UMAX (*last_note, p->date_stamp);
		}
	}
	else /* just next one */
	{
		char buf[200];
		
		count = 1;
		for (p = ((PlayerCharacter*)ch)->board->note_first; p ; p = p->next, count++)
			if ((p->date_stamp > *last_note) && is_note_to(ch,p))
			{
				show_note_to_char (ch,p,count);
				/* Advance if new note is newer than the currently newest for that char */
				*last_note =  UMAX (*last_note, p->date_stamp);
				return;
			}
		
		send_to_char ("No new notes in this board.  ",ch);
		
		if (next_board (ch))
			{
			snprintf(buf, sizeof(buf), "Changed to next board, {M%s{x.\n\r", ((PlayerCharacter*)ch)->board->short_name);
                        send_to_char(buf, ch);
                        do_nread (ch, (char*)"");
                        return;
                        }
                else
                        {
                        snprintf(buf, sizeof(buf), "There are no more boards.\n\r");
			send_to_char("\n\r",ch);
                        do_board (ch, (char*)"1");
                        return;
			}
			
		send_to_char (buf,ch);
	}
}

/* Remove a note */
static void do_nremove (Character *ch, char *argument)
{
	Note *p;
	
	if (!is_number(argument))
	{
		send_to_char ("Remove which note?\n\r",ch);
		return;
	}

	p = find_note (ch, ((PlayerCharacter*)ch)->board, atoi(argument));
	if (!p)
	{
		send_to_char ("No such note.\n\r",ch);
		return;
	}
	
	if (str_cmp(ch->getName().c_str(),p->sender) && (ch->getTrust() < MAX_LEVEL))
	{
		send_to_char ("You are not authorized to remove this note.\n\r",ch);
		return;
	}
	
	unlink_note (((PlayerCharacter*)ch)->board,p);
	delete p;
	send_to_char ("Note removed!\n\r",ch);
	
	save_board(((PlayerCharacter*)ch)->board); /* save the board */
}


/* List all notes or if argument given, list N of the last notes */
/* Shows REAL note numbers! */
static void do_nlist (Character *ch, char *argument)
{
	int count= 0, show = 0, num = 0, has_shown = 0;
	time_t last_note;
	Note *p;
	char buf[MAX_STRING_LENGTH];
	
	
	if (is_number(argument))	 /* first, count the number of notes */
	{
		show = atoi(argument);
		
		for (p = ((PlayerCharacter*)ch)->board->note_first; p; p = p->next)
			if (is_note_to(ch,p))
				count++;
	}
	
	send_to_char ("Notes on this board:\n\r"
	              "{rNum{x> Author        Subject\n\r",ch);
	              
	last_note = ((PlayerCharacter*)ch)->last_note_read.at(board_number (((PlayerCharacter*)ch)->board) );
	
	for (p = ((PlayerCharacter*)ch)->board->note_first; p; p = p->next)
	{
		num++;
		if (is_note_to(ch,p))
		{
			has_shown++; /* note that we want to see X VISIBLE note, not just last X */
			if (!show || ((count-show) < has_shown))
			{
				snprintf(buf, sizeof(buf), "%3d> {B%c{x {Y%-13s{x {y%s{x\n\r",
				               num, 
				               last_note < p->date_stamp ? '*' : ' ',
				               p->sender, p->subject);
				send_to_char (buf,ch);
			}
		}
				              
	}
}

/* catch up with some notes */
static void do_ncatchup (Character *ch, char *argument)
{
	Note *p;

	/* Find last note */	
	for (p = ((PlayerCharacter*)ch)->board->note_first; p && p->next; p = p->next);
	
	if (!p)
		send_to_char ("Alas, there are no notes in that board.\n\r",ch);
	else
	{
		((PlayerCharacter*)ch)->last_note_read.emplace(board_number(((PlayerCharacter*)ch)->board), p->date_stamp);
		send_to_char ("All mesages skipped.\n\r",ch);
	}
}

/* Dispatch function for backwards compatibility */
void do_note (Character *ch, char *argument)
{
	char arg[MAX_INPUT_LENGTH];

	if (ch->isNPC())
		return;
	
	argument = one_argument (argument, arg);
	
	if ((!arg[0]) || (!str_cmp(arg, "read"))) /* 'note' or 'note read X' */
		do_nread (ch, argument);
		
	else if (!str_cmp (arg, "list"))
		do_nlist (ch, argument);

	else if (!str_cmp (arg, "write"))
		do_nwrite (ch, argument);

	else if (!str_cmp (arg, "remove"))
		do_nremove (ch, argument);
		
	else if (!str_cmp (arg, "purge"))
		send_to_char ("Obsolete.\n\r",ch);
	
	else if (!str_cmp (arg, "archive"))
		send_to_char ("Obsolete.\n\r",ch);
	
	else if (!str_cmp (arg, "catchup"))
		do_ncatchup (ch, argument);
	else 
		do_help (ch, (char*)"note");
}

/* Show all accessible boards with their numbers of unread messages OR
   change board. New board name can be given as a number or as a name (e.g.
    board personal or board 4 */
void do_board (Character *ch, char *argument)
{
	int i, count, number;
	char buf[200];
	
	if (ch->isNPC())
		return;
	
	if (!argument[0]) /* show boards */
	{
		int unread;
		
		count = 1;
		send_to_char ("{RNum         Name Unread Description{x\n\r"
		              "{r=== ============ ====== ==========={x\n\r",ch);
		for (i = 0; i < MAX_BOARD; i++)
		{
			unread = unread_notes (ch,&boards[i]); /* how many unread notes? */
			if (unread != BOARD_NOACCESS)
			{ 
				snprintf(buf, sizeof(buf), "%2d> " "{Y%12s{x [%s%4d{x] " "{Y%s{x\n\r", 
				                   count, boards[i].short_name, unread ? "{r" : "{g", 
				                    unread, boards[i].long_name);
				send_to_char (buf,ch);	                    
				count++;
			} /* if has access */
			
		} /* for each board */
		
		snprintf(buf, sizeof(buf), "\n\rYou current board is %s.\n\r", ((PlayerCharacter*)ch)->board->short_name);
		send_to_char (buf,ch);

		/* Inform of rights */		
		if (((PlayerCharacter*)ch)->board->read_level > ch->getTrust())
			send_to_char ("You cannot read nor write notes on this board.\n\r",ch);
		else if (((PlayerCharacter*)ch)->board->write_level > ch->getTrust())
			send_to_char ("You can only read notes from this board.\n\r",ch);
		else
			send_to_char ("You can both read and write on this board.\n\r",ch);

		return;			
	} /* if empty argument */
	
    if (((PlayerCharacter*)ch)->in_progress)
    {
    	send_to_char ("Please finish your interrupted note first.\n\r",ch);
        return;
    }
	
	/* Change board based on its number */
	if (is_number(argument))
	{
		count = 0;
		number = atoi(argument);
		for (i = 0; i < MAX_BOARD; i++)
			if (unread_notes(ch,&boards[i]) != BOARD_NOACCESS)		
				if (++count == number)
					break;
		
		if (count == number) /* found the board.. change to it */
		{
			((PlayerCharacter*)ch)->board = &boards[i];
			snprintf(buf, sizeof(buf), "Current board changed to %s. %s.\n\r",boards[i].short_name,
			              (ch->getTrust() < boards[i].write_level) 
			              ? "You can only read here" 
			              : "You can both read and write here");
			send_to_char (buf,ch);
		}			
		else /* so such board */
			send_to_char ("No such board.\n\r",ch);
			
		return;
	}

	/* Non-number given, find board with that name */
	
	for (i = 0; i < MAX_BOARD; i++)
		if (!str_cmp(boards[i].short_name, argument))
			break;
			
	if (i == MAX_BOARD)
	{
		send_to_char ("No such board.\n\r",ch);
		return;
	}

	/* Does ch have access to this board? */	
	if (unread_notes(ch,&boards[i]) == BOARD_NOACCESS)
	{
		send_to_char ("No such board.\n\r",ch);
		return;
	}
	
	((PlayerCharacter*)ch)->board = &boards[i];
	snprintf(buf, sizeof(buf), "Current board changed to %s. %s.\n\r",boards[i].short_name,
	              (ch->getTrust() < boards[i].write_level) 
	              ? "You can only read here" 
	              : "You can both read and write here");
	send_to_char (buf,ch);
}

/* Send a note to someone on the personal board */
void personal_message (const char *sender, const char *to, const char *subject, const int expire_days, const char *text)
{
	make_note ("Personal", sender, to, subject, expire_days, text);
}

void make_note (const char* board_name, const char *sender, const char *to, const char *subject, const int expire_days, const char *text)
{
	int board_index = board_lookup (board_name);
	BOARD_DATA *board;
	Note *note;
	char *strtime;
	
	if (board_index == BOARD_NOTFOUND)
	{
		bug ("make_note: board not found",0);
		return;
	}
	
	if (strlen(text) > MAX_NOTE_TEXT)
	{
		bug ("make_note: text too long (%d bytes)", strlen(text));
		return;
	}
	
	
	board = &boards [board_index];
	
	note = new Note(); /* allocate new note */
	
	note->sender = str_dup (sender);
	note->to_list = str_dup(to);
	note->subject = str_dup (subject);
	note->expire = current_time + expire_days * 60 * 60 * 24;
	note->setText( text );

	/* convert to ascii. ctime returns a string which last character is \n, so remove that */	
	strtime = ctime (&current_time);
	strtime[strlen(strtime)-1] = '\0';
	
	note->date = str_dup (strtime);
	
	finish_note (board, note);
	
}

/* tries to change to the next accessible board */
static bool next_board (Character *ch)
{
	int i = board_number (((PlayerCharacter*)ch)->board) + 1;
	
	while ((i < MAX_BOARD) && (unread_notes(ch,&boards[i]) == BOARD_NOACCESS))
		i++;
		
	if (i == MAX_BOARD)
		return FALSE;
	else
	{
		((PlayerCharacter*)ch)->board = &boards[i];
		return TRUE;
	}
}

void handle_ConnectedStateNoteTo (DESCRIPTOR_DATA *d, char * argument)
{
	char buf [MAX_INPUT_LENGTH];
	Character *ch = d->character;

	if (!((PlayerCharacter*)ch)->in_progress)
	{
		d->connected = ConnectedState::Playing;
		bug ("nanny: In ConnectedState::NoteTo, but no note in progress",0);
		return;
	}

	strcpy (buf, argument);
	smash_tilde (buf); /* change ~ to - as we save this field as a string later */

	switch (((PlayerCharacter*)ch)->board->force_type)
	{
		case DEF_NORMAL: /* default field */
			if (!buf[0]) /* empty string? */
			{
				((PlayerCharacter*)ch)->in_progress->to_list = str_dup (((PlayerCharacter*)ch)->board->names);
				snprintf(buf, sizeof(buf), "Assumed default recipient: %s\n\r", ((PlayerCharacter*)ch)->board->names);
				send_to_char(buf, ch);
			}
			else
				((PlayerCharacter*)ch)->in_progress->to_list = str_dup (buf);
				
			break;
		
		case DEF_INCLUDE: /* forced default */
			if (!is_full_name (((PlayerCharacter*)ch)->board->names, buf))
			{
				strcat (buf, " ");
				strcat (buf, ((PlayerCharacter*)ch)->board->names);
				((PlayerCharacter*)ch)->in_progress->to_list = str_dup(buf);

				snprintf(buf, sizeof(buf), "\n\rYou did not specify %s as recipient, so it was automatically added.\n\r"
				         "{YNew To{x:  %s\n\r",
						 ((PlayerCharacter*)ch)->board->names, ((PlayerCharacter*)ch)->in_progress->to_list);
				send_to_char(buf,ch);
			}
			else
				((PlayerCharacter*)ch)->in_progress->to_list = str_dup (buf);
			break;
		
		case DEF_EXCLUDE: /* forced exclude */
			if (!buf[0])
			{
				snprintf(buf, sizeof(buf), "You must specify a recipient.\n\r" "{YTo{x:      ");
				send_to_char(buf,ch);
				return;
			}
			
			if (is_full_name (((PlayerCharacter*)ch)->board->names, buf))
			{
				snprintf(buf, sizeof(buf), "You are not allowed to send notes to %s on this board. Try again.\n\r"
				         "{YTo{x:      ", ((PlayerCharacter*)ch)->board->names);
				send_to_char(buf,ch);
				return; /* return from nanny, not changing to the next state! */
			}
			else
				((PlayerCharacter*)ch)->in_progress->to_list = str_dup (buf);
			break;
		
	}		

	snprintf(buf, sizeof(buf), "\n\r{YSubject{x: ");
	send_to_char(buf,ch);
	d->connected = ConnectedState::NoteSubject;
}

void handle_ConnectedStateNoteSubject (DESCRIPTOR_DATA *d, char * argument)
{
	char buf [MAX_INPUT_LENGTH];
	Character *ch = d->character;

	if (!((PlayerCharacter*)ch)->in_progress)
	{
		d->connected = ConnectedState::Playing;
		bug ("nanny: In ConnectedState::NoteSubject, but no note in progress",0);
		return;
	}

	strcpy (buf, argument);
	smash_tilde (buf); /* change ~ to - as we save this field as a string later */
	
	/* Do not allow empty subjects */
	
	if (!buf[0])
	{
		char buf1[MSL];
		send_to_char("Please find a meaningful subject!\n\r",ch);
		snprintf(buf1, sizeof(buf1), "{YSubject{x: ");
		send_to_char(buf1,ch);
	}
	else  if (strlen(buf)>60)
	{
		SocketHelper::write_to_buffer (d, "No, no. This is just the Subject. You're note writing the note yet. Twit.\n\r",0);
	}
	else
	/* advance to next stage */
	{
		((PlayerCharacter*)ch)->in_progress->subject = str_dup(buf);
		if (IS_IMMORTAL(ch)) /* immortals get to choose number of expire days */
		{
			snprintf(buf, sizeof(buf),"\n\rHow many days do you want this note to expire in?\n\r"
			             "Press Enter for default value for this board, %d days.\n\r"
           				 "{YExpire{x:  ",
		                 ((PlayerCharacter*)ch)->board->purge_days);
			send_to_char(buf,ch);
			d->connected = ConnectedState::NoteExpire;
		}
		else
		{
			((PlayerCharacter*)ch)->in_progress->expire = 
				current_time + ((PlayerCharacter*)ch)->board->purge_days * 24L * 3600L;				
			snprintf(buf, sizeof(buf), "This note will expire %s\r",ctime(&((PlayerCharacter*)ch)->in_progress->expire));
			SocketHelper::write_to_buffer (d,buf,0);
			snprintf(buf, sizeof(buf), "\n\rEnter text. Type ~ or END on an empty line to end note.\n\r"
			                    "=======================================================\n\r");
			send_to_char(buf,ch);
			d->connected = ConnectedState::NoteText;
		}
	}
}

void handle_ConnectedStateNoteExpire(DESCRIPTOR_DATA *d, char * argument)
{
	Character *ch = d->character;
	char buf[MAX_STRING_LENGTH];
	time_t expire;
	int days;

	if (!((PlayerCharacter*)ch)->in_progress)
	{
		d->connected = ConnectedState::Playing;
		bug ("nanny: In ConnectedState::NoteExpire, but no note in progress",0);
		return;
	}
	
	/* Numeric argument. no tilde smashing */
	strcpy (buf, argument);
	if (!buf[0]) /* assume default expire */
		days = 	((PlayerCharacter*)ch)->board->purge_days;
	else /* use this expire */
		if (!is_number(buf))
		{
			char buf1[MSL];
			SocketHelper::write_to_buffer (d,"Write the number of days!\n\r",0);
			snprintf(buf1, sizeof(buf1), "{YExpire{x:  ");
			send_to_char(buf1,ch);
			return;
		}
		else
		{
			days = atoi (buf);
			if (days <= 0)
			{
				char buf1[MSL];
				SocketHelper::write_to_buffer (d, "This is a positive MUD. Use positive numbers only! :)\n\r",0);
				snprintf(buf1, sizeof(buf1), "{YExpire{x:  ");
				return;
			}
		}
			
	expire = current_time + (days*24L*3600L); /* 24 hours, 3600 seconds */

	((PlayerCharacter*)ch)->in_progress->expire = expire;
	
	/* note that ctime returns XXX\n so we only need to add an \r */

	snprintf(buf, sizeof(buf), "\n\rEnter text. Type ~ or END on an empty line to end note.\n\r"
	                    "=======================================================\n\r");
	send_to_char(buf,ch);

	d->connected = ConnectedState::NoteText;
}



void handle_ConnectedStateNoteText (DESCRIPTOR_DATA *d, char * argument)
{
	Character *ch = d->character;
	char buf[MAX_STRING_LENGTH];
	char letter[4*MAX_STRING_LENGTH];
	
	if (!((PlayerCharacter*)ch)->in_progress)
	{
		d->connected = ConnectedState::Playing;
		bug ("nanny: In ConnectedState::NoteText, but no note in progress",0);
		return;
	}

	/* First, check for EndOfNote marker */

	strcpy (buf, argument);
	if ((!str_cmp(buf, "~")) || (!str_cmp(buf, "END")))
	{
		SocketHelper::write_to_buffer (d, "\n\r\n\r",0);
		send_to_char(szFinishPrompt, ch);
		SocketHelper::write_to_buffer (d, "\n\r", 0);
		d->connected = ConnectedState::NoteFinish;
		return;
	}
	
	smash_tilde (buf); /* smash it now */

	/* Check for too long lines. Do not allow lines longer than 80 chars */
	
	if (strlen (buf) > MAX_LINE_LENGTH)
	{
		SocketHelper::write_to_buffer (d, "Too long line rejected. Do NOT go over 80 characters!\n\r",0);
		return;
	}
	
	/* Not end of note. Copy current text into temp buffer, add new line, and copy back */

	/* How would the system react to strcpy( , NULL) ? */		
	if (!((PlayerCharacter*)ch)->in_progress->getText().empty())
	{
		strcpy (letter, ((PlayerCharacter*)ch)->in_progress->getText().c_str());
		((PlayerCharacter*)ch)->in_progress->setText("");
	}
	else
		strcpy (letter, "");
		
	/* Check for overflow */
	
	if ((strlen(letter) + strlen (buf)) > MAX_NOTE_TEXT)
	{ /* Note too long, take appropriate steps */
		SocketHelper::write_to_buffer (d, "Note too long!\n\r", 0);
		delete ((PlayerCharacter*)ch)->in_progress;
		((PlayerCharacter*)ch)->in_progress = NULL;			/* important */
		d->connected = ConnectedState::Playing;
		return;			
	}
	
	/* Add new line to the buffer */
	
	strcat (letter, buf);
	strcat (letter, "\r\n"); /* new line. \r first to make note files better readable */

	/* allocate dynamically */		
	((PlayerCharacter*)ch)->in_progress->setText(letter);
}

void handle_ConnectedStateNoteFinish (DESCRIPTOR_DATA *d, char * argument)
{
	char buf[MSL];
	PlayerCharacter *ch = (PlayerCharacter*)d->character;
	
		if (!((PlayerCharacter*)ch)->in_progress)
		{
			d->connected = ConnectedState::Playing;
			bug ("nanny: In ConnectedState::NoteFinish, but no note in progress",0);
			return;
		}
		
		switch (tolower(argument[0]))
		{
			case 'c': /* keep writing */
				SocketHelper::write_to_buffer (d,"Continuing note...\n\r",0);
				d->connected = ConnectedState::NoteText;
				break;

			case 'v': /* view note so far */
				if (!((PlayerCharacter*)ch)->in_progress->getText().empty())
				{
					snprintf(buf, sizeof(buf), "{gYour note so far:{x\n\r");
					send_to_char(buf,ch);
					send_to_char(((PlayerCharacter*)ch)->in_progress->getText().c_str(), ch);
				}
				else
					SocketHelper::write_to_buffer (d,"You haven't written a thing!\n\r\n\r",0);
				send_to_char(szFinishPrompt, ch);
				SocketHelper::write_to_buffer (d, "\n\r",0);
				break;

			case 'p': /* post note */
				finish_note (((PlayerCharacter*)ch)->board, ((PlayerCharacter*)ch)->in_progress);
				SocketHelper::write_to_buffer (d, "Note posted.\n\r",0);
				d->connected = ConnectedState::Playing;

				/* remove AFK status */
				REMOVE_BIT(ch->comm, COMM_AFK);

				/* Send them home */
				char_from_room(ch);
				char_to_room(ch, ch->getWasNoteRoom());

				((PlayerCharacter*)ch)->in_progress = NULL;
				act ("{G$n finishes $s note.{x", ch, NULL, NULL, TO_ROOM, POS_RESTING);
				break;
				
			case 'f':
				SocketHelper::write_to_buffer (d, "Note cancelled!\n\r",0);
				delete ((PlayerCharacter*)ch)->in_progress;
				((PlayerCharacter*)ch)->in_progress = NULL;
				d->connected = ConnectedState::Playing;
				/* remove afk status */
				REMOVE_BIT(ch->comm, COMM_AFK);

				/* Send them home */
				char_from_room(ch);
				char_to_room(ch, ch->getWasNoteRoom());
				act ("{G$n decided not to finish $s note.{x", ch, NULL, NULL, TO_ROOM, POS_RESTING);
				break;
			
			default: /* invalid response */
				SocketHelper::write_to_buffer (d, "Huh? Valid answers are:\n\r\n\r",0);
				send_to_char(szFinishPrompt, ch);
				SocketHelper::write_to_buffer (d, "\n\r",0);
				
		}
}

