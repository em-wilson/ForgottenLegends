#if !defined(CHARACTER_H)
#define CHARACTER_H

#include <list>
#include <string>
#include <vector>
#include <unordered_map>
#include "clans/Clan.h"

#include "IAffectableEntity.h"
#include "IObjectContainer.h"

class Character;
class Object;
class Portal;
class Race;
class ROOM_INDEX_DATA;

#define args( list )			    list
typedef bool    SPEC_FUN	        args( ( Character *ch ) );
typedef struct	affect_data		    AFFECT_DATA;
typedef struct	area_data		    AREA_DATA;
typedef struct  descriptor_data     DESCRIPTOR_DATA;
typedef struct  gen_data		    GEN_DATA;
typedef struct  mem_data            MEM_DATA;
typedef struct	mob_index_data		MOB_INDEX_DATA;

/*
 * One character (PC or NPC).
 */
class Character : public IAffectableEntity, IObjectContainer
{
    public:
        virtual ~Character();

        // IObjectContainer
        void addObject(Object *obj);
        void removeObject(Object *obj);

        // IAffectableEntity
        AFFECT_DATA * findAffectBySn(int sn);
        void removeAffect(AFFECT_DATA *af);
        virtual void giveAffect( AFFECT_DATA *paf ); // replaces affect_to_char
        void modifyAffect(AFFECT_DATA *paf, bool fAdd); // replaces affect_modify

        virtual bool isClanned() = 0;
        virtual Clan * getClan() = 0;
        bool isSameGroup(Character *ch);
        virtual void perform_autoloot() = 0;

        Character *		next;
        Character *		next_in_room;
        Character *		master;
        Character *		leader;
        Character *		fighting;
        Character *		reply;
        Character *		pet;
        Character *		mprog_target;
        Character *		hunting;
        MEM_DATA *		memory;
        SPEC_FUN *		spec_fun;
        MOB_INDEX_DATA *	pIndexData;
        DESCRIPTOR_DATA *	desc;
        std::list<AFFECT_DATA *>	affected;
        // Object *		on;
        ROOM_INDEX_DATA *	in_room;
        ROOM_INDEX_DATA *	was_in_room;
        AREA_DATA *		zone;
        GEN_DATA *		gen_data;
        bool		valid;
        bool		confirm_reclass;
        long		id;
        short int		version;
        char *		short_descr;
        char *		long_descr;
        char *		prompt;
        short int		group;
        short int		sex;
        short int		class_num;
        short int		level;
        short int		trust;
        short int		drac;
        short int		breathe;
        int			played;
        int			lines;  /* for the pager */
        time_t		logon;
        short int		timer;
        short int		wait;
        short int		daze;
        short int		hit;
        short int		max_hit;
        short int		mana;
        short int		max_mana;
        short int		move;
        short int		max_move;
        long		gold;
        long		silver;
        int			exp;
        long		act;
        long		comm;   /* RT added to pad the vector */
        long		done;   /* What classes have they completed? */
        short int		reclass_num; /* How many times have the reclassed? */
        long		wiznet; /* wiz stuff */
        long		imm_flags;
        long		res_flags;
        long		vuln_flags;
        short int		invis_level;
        short int		incog_level;
        long			affected_by;
        short int		position;
        short int		practice;
        short int		train;
        short int		carry_weight;
        short int		carry_number;
        short int		saving_throw;
        short int		alignment;
        short int		hitroll;
        short int		damroll;
        short int		armor[4];
        short int		wimpy;
        /* stats */
        std::unordered_map<int, short int> perm_stat;
        std::unordered_map<int, short int> mod_stat;
        /* parts stuff */
        long		form;
        long		parts;
        short int		size;
        char*		material;
        /* mobile stuff */
        long		off_flags;
        short int		damage[3];
        short int		dam_type;
        short int		start_pos;
        short int		default_pos;

        short int		mprog_delay;

        /* werefolk stuff */
        short int		morph_form;
        short int		orig_form;
        int			xp;

        /*
        * Advancement stuff.
        */
        void affect_check(int where,int vector);
        bool can_see( Object *obj );
        bool can_see( Character *victim );
        virtual void gain_exp( int gain );
        virtual int hit_gain( ) = 0;
        virtual int mana_gain( ) = 0;
        virtual int move_gain( ) = 0;
        virtual void gain_condition( int iCond, int value ) = 0;
        Object * getEquipment( int iWear );
        void unequip( Object *obj );
        void setName( string name );
        std::string getName();
        void setDescription( const char * description );
        std::vector<Object *> getCarrying();
        char * getDescription();
        Race * getRace();
        void setRace(Race * value);
        void getOntoObject(Object *obj);
        Object * onObject();

        virtual bool isDenied();
        virtual bool isImmortal();
        virtual bool isNPC() = 0;

        virtual int getRange();
        virtual int getTrust();

        virtual bool didJustDie();


        virtual void update();
    
    protected:
        Character();

    private:
        std::list<Object *> _carrying;
        std::string _name;
        std::string _description;
        Object * _on;
        Race * _race;
};
#endif