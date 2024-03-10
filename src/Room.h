#ifndef __ROOM_H__
#define __ROOM_H__

#include <list>
#include <vector>

class Character;
class Clan;
class ExtraDescription;
class Object;

typedef struct	area_data		AREA_DATA;
typedef struct	exit_data		EXIT_DATA;
typedef struct	reset_data		RESET_DATA;


/*
 * Room type.
 */
class ROOM_INDEX_DATA
{
    public:
        ROOM_INDEX_DATA();
        virtual ~ROOM_INDEX_DATA();
        ROOM_INDEX_DATA *	next;
        Character *		people;
        std::list<Object *> contents;
        std::vector<ExtraDescription *>	extra_descr;
        AREA_DATA *		area;
        EXIT_DATA *		exit	[6];
        RESET_DATA *	reset_first;	/* OLC */
        RESET_DATA *	reset_last;	/* OLC */
        char *		name;
        char *		description;
        char *		owner;
        short int		vnum;
        int			room_flags;
        short int		light;
        short int		sector_type;
        short int		heal_rate;
        short int 		mana_rate;
        Clan *		clan;
};

#endif