#include "Note.h"


/* Includes for board system */
/* This is version 2 of the board system, (c) 1995-96 erwin@pip.dknet.dk */


#define DEF_NORMAL  0 /* No forced change, but default (any string)   */
#define DEF_INCLUDE 1 /* 'names' MUST be included (only ONE name!)    */
#define DEF_EXCLUDE 2 /* 'names' must NOT be included (one name only) */

#define MAX_BOARD 	  7

#define DEFAULT_BOARD 0 /* default board is board #0 in the boards      */
                        /* It should be readable by everyone!           */
                        
#define MAX_LINE_LENGTH 80 /* enforce a max length of 80 on text lines, reject longer lines */
						   /* This only applies in the Body of the note */                        
						   
#define MAX_NOTE_TEXT (4*MAX_STRING_LENGTH - 1000)
						
#define BOARD_NOTFOUND -1 /* Error code from board_lookup() and board_number */

/* Data about a board */
struct board_data
{
	const char *short_name; /* Max 8 chars */
	const char *long_name;  /* Explanatory text, should be no more than 40 ? chars */
	
	int read_level; /* minimum level to see board */
	int write_level;/* minimum level to post notes */

	const char *names;       /* Default recipient */
	int force_type; /* Default action (DEF_XXX) */
	
	int purge_days; /* Default expiration */

	/* Non-constant data */
		
	Note *note_first; /* pointer to board's first note */
	bool changed; /* currently unused */
		
};

typedef struct board_data BOARD_DATA;

class ROOM_INDEX_DATA;


/* External variables */

extern BOARD_DATA boards[MAX_BOARD]; /* Declare */

/* Prototypes */

void finish_note (BOARD_DATA *board, Note *note); /* attach a note to a board */
void load_boards (void); /* load all boards */
int board_lookup (const char *name); /* Find a board with that name */
bool is_note_to (Character *ch, Note *note); /* is tha note to ch? */
void personal_message (const char *sender, const char *to, const char *subject, const int expire_days, const char *text);
void make_note (const char* board_name, const char *sender, const char *to, const char *subject, const int expire_days, const char *text);
void save_notes ();
bool is_note_room (ROOM_INDEX_DATA *room);

/* for nanny */
void handle_ConnectedStateNoteTo 		(DESCRIPTOR_DATA *d, char * argument);
void handle_ConnectedStateNoteSubject 	(DESCRIPTOR_DATA *d, char * argument);
void handle_ConnectedStateNoteExpire 	(DESCRIPTOR_DATA *d, char * argument);
void handle_ConnectedStateNoteText 		(DESCRIPTOR_DATA *d, char * argument);
void handle_ConnectedStateNoteFinish 	(DESCRIPTOR_DATA *d, char * argument);


/* Commands */

DECLARE_DO_FUN (do_note		);
DECLARE_DO_FUN (do_board	);
