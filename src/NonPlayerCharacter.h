#include "Character.h"

typedef struct  mprog_list		MPROG_LIST;
typedef struct	shop_data		SHOP_DATA;


class NonPlayerCharacter : public Character {
public:
    NonPlayerCharacter();
    NonPlayerCharacter(MOB_INDEX_DATA *pMobIndex);
    ~NonPlayerCharacter();

    bool isClanned() override;
    Clan * getClan() override;
    bool isNPC() override;

    virtual int getTrust() override;
    virtual int hit_gain() override;
    virtual int mana_gain( ) override;
    virtual int move_gain( ) override;
    virtual void update() override;


    virtual void perform_autoloot() override;
    void gain_condition( int iCond, int value ) override;
};

/*
 * Prototype for a mob.
 * This is the in-memory version of #MOBILES.
 */
struct	mob_index_data
{
    MOB_INDEX_DATA *	next;
    SPEC_FUN *		spec_fun;
    SHOP_DATA *		pShop;
    MPROG_LIST *        mprogs;
    AREA_DATA *		area;		/* OLC */
    short int		vnum;
    short int		group;
    bool		new_format;
    short int		count;
    short int		killed;
    char *		player_name;
    char *		short_descr;
    char *		long_descr;
    char *		description;
    long		act;
    long		affected_by;
    short int		alignment;
    short int		level;
    short int		hitroll;
    short int		hit[3];
    short int		mana[3];
    short int		damage[3];
    short int		ac[4];
    short int 		dam_type;
    long		off_flags;
    long		imm_flags;
    long		res_flags;
    long		vuln_flags;
    short int		start_pos;
    short int		default_pos;
    short int		sex;
    short int		race;
    long		wealth;
    long		form;
    long		parts;
    short int		size;
    char *		material;
    long		mprog_flags;
};