#if !defined(PCDATA_H)
#define PCDATA_H

#include <list>
#include <unordered_map>
#include "clans/Clan.h"

#define MAX_ALIAS		    5           // Temporary - if updating this, also update merc.h

typedef struct board_data BOARD_DATA;
typedef struct 	buf_type	 	BUFFER;
class Note;

/*
 * Data which only PC's have.
 */
class PC_DATA
{
public:
	static std::list<PC_DATA *> active;
    PC_DATA *		next;
    Clan *		clan;
    BUFFER * 		buffer;
    bool		valid;
    char *		pwd;
    char *		bamfin;
    char *		bamfout;
    char *		title;
    short int		perm_hit;
    short int		perm_mana;
    short int		perm_move;
    short int		true_sex;
    int			last_level;
    short int		condition	[4];
    std::unordered_map<int, short int> learned;
    std::unordered_map<int, short int> sk_level;
    std::unordered_map<int, short int> sk_rating;
    std::unordered_map<int, bool> group_known;
    short int		points;
    bool              	confirm_delete;
    char *		alias[MAX_ALIAS];
    char * 		alias_sub[MAX_ALIAS];
    int 		security;	/* OLC */ /* Builder security */
    BOARD_DATA *        board;                  /* The current board        */
    std::unordered_map<int, time_t> last_note_read;   /* last note for the boards */
    Note *         in_progress;

    PC_DATA();
    ~PC_DATA();
};



#endif
