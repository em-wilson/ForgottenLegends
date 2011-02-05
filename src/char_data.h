#if !defined(CHARDATA_H)
#define CHARDATA_H

#include <list>
#include "pc_data.h"

/*
 * One character (PC or NPC).
 */
class CHAR_DATA
{
public:
    CHAR_DATA *		next;
    CHAR_DATA *		next_in_room;
    CHAR_DATA *		master;
    CHAR_DATA *		leader;
    CHAR_DATA *		fighting;
    CHAR_DATA *		reply;
    CHAR_DATA *		pet;
    CHAR_DATA *		mprog_target;
    CHAR_DATA *		hunting;
    MEM_DATA *		memory;
    SPEC_FUN *		spec_fun;
    MOB_INDEX_DATA *	pIndexData;
    DESCRIPTOR_DATA *	desc;
    AFFECT_DATA *	affected;
    OBJ_DATA *		carrying;
    OBJ_DATA *		on;
    ROOM_INDEX_DATA *	in_room;
    ROOM_INDEX_DATA *	was_in_room;
    ROOM_INDEX_DATA *	was_note_room;
    AREA_DATA *		zone;
    PC_DATA *		pcdata;
    GEN_DATA *		gen_data;
    bool		valid;
    bool		confirm_reclass;
    char *		name;
    long		id;
    sh_int		version;
    char *		short_descr;
    char *		long_descr;
    char *		description;
    char *		prompt;
    char *		prefix;
    sh_int		group;
    sh_int		sex;
    sh_int		class_num;
    sh_int		race;
    sh_int		level;
    sh_int		trust;
    sh_int		drac;
    sh_int		breathe;
    int			played;
    AUCTION_DATA *	auction;
    int			lines;  /* for the pager */
    time_t		logon;
    sh_int		timer;
    sh_int		wait;
    sh_int		daze;
    sh_int		hit;
    sh_int		max_hit;
    sh_int		mana;
    sh_int		max_mana;
    sh_int		move;
    sh_int		max_move;
    long		gold;
    long		silver;
    int			exp;
    long		act;
    long		comm;   /* RT added to pad the vector */
    long		done;   /* What classes have they completed? */
    int			reclass_num; /* How many times have the reclassed? */
    long		wiznet; /* wiz stuff */
    long		imm_flags;
    long		res_flags;
    long		vuln_flags;
    sh_int		invis_level;
    sh_int		incog_level;
    long			affected_by;
    sh_int		position;
    sh_int		practice;
    sh_int		train;
    sh_int		carry_weight;
    sh_int		carry_number;
    sh_int		saving_throw;
    sh_int		alignment;
    sh_int		hitroll;
    sh_int		damroll;
    sh_int		armor[4];
    sh_int		wimpy;
    /* stats */
    sh_int		perm_stat[MAX_STATS];
    sh_int		mod_stat[MAX_STATS];
    /* parts stuff */
    long		form;
    long		parts;
    sh_int		size;
    char*		material;
    /* mobile stuff */
    long		off_flags;
    sh_int		damage[3];
    sh_int		dam_type;
    sh_int		start_pos;
    sh_int		default_pos;

    sh_int		mprog_delay;

    /* werefolk stuff */
    sh_int		morph_form;
    sh_int		orig_form;
    int			xp;

    /* clan stuff */
    bool		confirm_pk;
    bool		makeclan;
    CLAN_DATA *		join;
    int			pkills;
    int			pkilled;
    int			mkills;
    int			mkilled;
    int			range;
    int			clan_cust;
    int			adrenaline;
    int			jkilled; // If they were just killed

    CHAR_DATA();
    ~CHAR_DATA();
    /*
     * Advancement stuff.
     */
    void advance_level( bool hide );
    void gain_exp( int gain );
    int hit_gain( );
    int mana_gain( );
    int move_gain( );
    void gain_condition( int iCond, int value );
};
#endif