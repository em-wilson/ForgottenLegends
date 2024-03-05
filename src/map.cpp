/************************************************************************/
/* mlkesl@stthomas.edu	=====>	Ascii Automapper utility		*/
/* Let me know if you use this. Give a newbie some credit,		*/
/* at least I'm not asking how to add classes...			*/
/* Also, if you fix something could ya send me mail about, thanks	*/
/* PLEASE mail me if you use this ot like it, that way I will keep it up*/
/************************************************************************/
/* MapArea -> 	when given a room, ch, x, and y,...			*/
/*	   	this function will fill in values of map as it should 	*/
/* ShowMap -> 	will simply spit out the contents of map array		*/
/*		Would look much nicer if you built your own areas	*/
/*		without all of the overlapping stock Rom has		*/
/* do_map  ->	core function, takes map size as argument		*/
/************************************************************************/
/* To install:: 							*/
/*	remove all occurances of "u1." (or union your exits)		*/
/*	add do_map prototypes to interp.c and merc.h (or interp.h)	*/
/*	customize the first switch with your own sectors		*/
/*	remove the color codes or change to suit your own color coding	*/
/* Other stuff::							*/
/*	make a skill, call from do_move (only if ch is not in city etc) */
/*	allow players to actually make ITEM_MAP objects			*/
/* 	change your areas to make them more suited to map code! :)	*/
/************************************************************************/
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "merc.h"

#define MAX_MAP 90
#define MAX_MAP_DIR 4

const char *map[MAX_MAP][MAX_MAP];
int offsets[4][2] = {{-1, 0}, {0, 1}, {1, 0}, {0, -1}};

void MapArea(ROOM_INDEX_DATA *room, Character *ch, int x, int y, int min, int max)
{
    ROOM_INDEX_DATA *prospect_room;
    Character *rch;
    EXIT_DATA *pexit;
    int door;

    /* marks the room as visited */
    switch (room->sector_type)
    {
    case SECT_INSIDE:
        map[x][y] = "{W%{x";
        break;
    case SECT_CITY:
        map[x][y] = "{W#{x";
        break;
    case SECT_FIELD:
        map[x][y] = "{G\"{x";
        break;
    case SECT_FOREST:
        map[x][y] = "{g@{x";
        break;
    case SECT_HILLS:
        map[x][y] = "{G^{x";
        break;
    case SECT_MOUNTAIN:
        map[x][y] = "{y^{x";
        break;
    case SECT_WATER_SWIM:
        map[x][y] = "{B~{x";
        break;
    case SECT_WATER_NOSWIM:
        map[x][y] = "{b~{x";
        break;
    case SECT_UNUSED:
        map[x][y] = "{DX{x";
        break;
    case SECT_AIR:
        map[x][y] = "{C:{x";
        break;
    case SECT_DESERT:
        map[x][y] = "{Y={x";
        break;
    case SECT_ROAD:
        map[x][y] = "{m+{x";
        break;
    default:
        map[x][y] = "{yo{x";
    }
    for (rch = room->people; rch != NULL; rch = rch->next_in_room)
    {
        if (!IS_NPC(rch))
            map[x][y] = "{b!{x";
    }

    for (door = 0; door < MAX_MAP_DIR; door++)
    {
        if (
            (pexit = room->exit[door]) != NULL && pexit->u1.to_room != NULL && can_see_room(ch, pexit->u1.to_room) /* optional */
            && !IS_SET(pexit->exit_info, EX_CLOSED))
        { /* if exit there */

            prospect_room = pexit->u1.to_room;

            if (prospect_room->exit[rev_dir[door]] &&
                prospect_room->exit[rev_dir[door]]->u1.to_room != room)
            { /* if not two way */
                if ((prospect_room->sector_type == SECT_CITY) || (prospect_room->sector_type == SECT_INSIDE))
                    map[x][y] = "{W@{x";
                else
                    map[x][y] = "{D?{x";
                return;
            } /* end two way */

            if ((x <= min) || (y <= min) || (x >= max) || (y >= max))
                return;
            if (map[x + offsets[door][0]][y + offsets[door][1]] == NULL)
            {
                MapArea(pexit->u1.to_room, ch,
                        x + offsets[door][0], y + offsets[door][1], min, max);
            }

        } /* end if exit there */
    }
    return;
}

void ShowMap(Character *ch, int min, int max)
{
    int x, y;

    for (x = min; x < max; ++x)
    {
        for (y = min; y < max; ++y)
        {
            if (map[x][y] == NULL)
                send_to_char(" ", ch);
            else
                send_to_char(map[x][y], ch);
        }
        send_to_char("\n\r", ch);
    }

    return;
}

void do_map(Character *ch, char *argument)
{
    int size, center, x, y, min, max;
    char arg1[10];

    one_argument(argument, arg1);
    size = atoi(arg1);

    size = URANGE(14, size, 90);
    center = MAX_MAP / 2;

    min = MAX_MAP / 2 - size / 2;
    max = MAX_MAP / 2 + size / 2;

    for (x = 0; x < MAX_MAP; ++x)
        for (y = 0; y < MAX_MAP; ++y)
            map[x][y] = NULL;

    /* starts the mapping with the center room */
    MapArea(ch->in_room, ch, center, center, min, max);

    /* marks the center, where ch is */
    map[center][center] = "{R*{x";
    ShowMap(ch, min, max);

    return;
}
