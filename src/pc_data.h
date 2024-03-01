#if !defined(PCDATA_H)
#define PCDATA_H

#include <list>
#include "clans/Clan.h"

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
    sh_int		perm_hit;
    sh_int		perm_mana;
    sh_int		perm_move;
    sh_int		true_sex;
    int			last_level;
    sh_int		condition	[4];
    sh_int		learned		[MAX_SKILL];
    sh_int		sk_level	[MAX_SKILL];
    sh_int		sk_rating	[MAX_SKILL];
    bool		group_known	[MAX_GROUP];
    sh_int		points;
    bool              	confirm_delete;
    char *		alias[MAX_ALIAS];
    char * 		alias_sub[MAX_ALIAS];
    int 		security;	/* OLC */ /* Builder security */
    BOARD_DATA *        board;                  /* The current board        */
    time_t              last_note[MAX_BOARD];   /* last note for the boards */
    Note *         in_progress;

    PC_DATA();
    ~PC_DATA();
};



#endif
