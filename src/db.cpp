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

/***************************************************************************
*	ROM 2.4 is copyright 1993-1996 Russ Taylor			   *
*	ROM has been brought to you by the ROM consortium		   *
*	    Russ Taylor (rtaylor@efn.org)				   *
*	    Gabrielle Taylor						   *
*	    Brian Moore (zump@rom.org)					   *
*	By using this code, you have agreed to follow the terms of the	   *
*	ROM license, in the file Rom24/doc/rom.license			   *
***************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>

#include "merc.h"
#include "board.h"
#include "clans/ClanManager.h"
#include "clans/ClanWriter.h"
#include "db.h"
#include "EquipmentListGenerator.h"
#include "ExitFlag.h"
#include "ExtraDescription.h"
#include "interfaces/ILogger.h"
#include "lookup.h"
#include "music.h"
#include "NonPlayerCharacter.h"
#include "Object.h"
#include "ObjectHelper.h"
#include "RaceManager.h"
#include "recycle.h"
#include "Room.h"
#include "RoomManager.h"
#include "tables.h"
#include "Wiznet.h"


extern	int	_filbuf		args( (FILE *) );
ClanManager * clan_manager;
ILogger * logger;
RaceManager * race_manager;
RoomManager * room_manager;

#if !defined(OLD_RAND)
/* RedHat 5.x already contains this, so I'm commenting it out.
          -Blizzard, March 25, 1999
long random();
void srandom(unsigned int);
*/
time_t time(time_t *tloc);
#endif


/* externals for counting purposes */
extern	Object	*obj_free;
extern  DESCRIPTOR_DATA *descriptor_free;
extern  AFFECT_DATA	*affect_free;

/*
 * Globals.
 */
HELP_DATA *		help_first;
HELP_DATA *		help_last;

HELP_AREA *		had_list;

SHOP_DATA *		shop_first;
SHOP_DATA *		shop_last;

Note *		note_free;

MPROG_CODE *		mprog_list;

char			bug_buf		[2*MAX_INPUT_LENGTH];
std::list<Character *> char_list;
char *			help_greeting;
char			log_buf		[2*MAX_INPUT_LENGTH];
KILL_DATA		kill_table	[MAX_LEVEL];
std::list<Object *> object_list;
TIME_INFO_DATA		time_info;
WEATHER_DATA		weather_info;

sh_int			gsn_backstab;
sh_int			gsn_dodge;
sh_int			gsn_envenom;
sh_int			gsn_hide;
sh_int			gsn_peek;
sh_int			gsn_pick_lock;
sh_int			gsn_sneak;
sh_int			gsn_steal;
sh_int			gsn_dual_wield;
sh_int			gsn_disarm;
sh_int			gsn_enhanced_damage;
sh_int			gsn_kick;
sh_int			gsn_parry;
sh_int			gsn_rescue;
sh_int			gsn_second_attack;
sh_int			gsn_third_attack;

sh_int			gsn_blindness;
sh_int			gsn_charm_person;
sh_int			gsn_curse;
sh_int			gsn_invis;
sh_int			gsn_mass_invis;
sh_int			gsn_poison;
sh_int			gsn_plague;
sh_int			gsn_sleep;
sh_int			gsn_sanctuary;
sh_int			gsn_fly;
/* new gsns */

sh_int  		gsn_axe;
sh_int  		gsn_dagger;
sh_int  		gsn_flail;
sh_int  		gsn_mace;
sh_int  		gsn_polearm;
sh_int			gsn_shield_block;
sh_int  		gsn_spear;
sh_int  		gsn_sword;
sh_int  		gsn_whip;
 
sh_int  		gsn_bash;
sh_int  		gsn_berserk;
sh_int  		gsn_dirt;
sh_int  		gsn_hand_to_hand;
sh_int  		gsn_trip;
 
sh_int  		gsn_fast_healing;
sh_int  		gsn_haggle;
sh_int  		gsn_lore;
sh_int  		gsn_meditation;
 
sh_int  		gsn_scrolls;
sh_int  		gsn_staves;
sh_int  		gsn_wands;
sh_int  		gsn_recall;

/* Blizzard skills */
sh_int			gsn_bite;
sh_int			gsn_bow;
sh_int			gsn_breath;
sh_int			gsn_hunt;
sh_int			gsn_morph;

/* Paladin skills */
sh_int			gsn_tame;

/* Rogue skills */
sh_int			gsn_endure;
sh_int			gsn_assassinate;
sh_int			gsn_strangle;
sh_int			gsn_blackjack;

/*
 * Locals.
 */
MOB_INDEX_DATA *	mob_index_hash		[MAX_KEY_HASH];
OBJ_INDEX_DATA *	obj_index_hash		[MAX_KEY_HASH];
ROOM_INDEX_DATA *	room_index_hash		[MAX_KEY_HASH];
char *			string_hash		[MAX_KEY_HASH];

AREA_DATA *		area_first;
AREA_DATA *		area_last;
AREA_DATA *		current_area;

char *			string_space;
char *			top_string;
char			str_empty	[1];

int			top_affect;
int			top_area;
int			top_ed;
int			top_exit;
int			top_help;
int			top_mob_index;
int			top_obj_index;
int			top_reset;
int			top_room;
int			top_shop;
int                     top_vnum_room;		/* OLC */
int                     top_vnum_mob;		/* OLC */
int                     top_vnum_obj;		/* OLC */
int			top_mprog_index;	/* OLC */
int 			mobile_count = 0;
int			newmobs = 0;
int			newobjs = 0;


/*
 * Memory management.
 * Increase MAX_STRING if you have too.
 * Tune the others only if you understand what you're doing.
 */
#define			MAX_STRING	2423120
#define			MAX_PERM_BLOCK	131072
#define			MAX_MEM_LIST	11

int			nAllocString;
int			sAllocString;
int			nAllocPerm;
int			sAllocPerm;



/*
 * Semi-locals.
 */
bool			fBootDb;
FILE *			fpArea;
char			strArea[MAX_INPUT_LENGTH];



/*
 * Local booting procedures.
*/
void    init_mm         args( ( void ) );
void	load_area	args( ( FILE *fp ) );
void    new_load_area   args( ( FILE *fp ) );   /* OLC */
void	load_helps	args( ( FILE *fp, char *fname ) );
void 	load_mobiles	args( ( FILE *fp ) );
void 	load_objects	args( ( FILE *fp ) );
void	load_resets	args( ( FILE *fp ) );
void	load_rooms	args( ( FILE *fp ) );
void	load_shops	args( ( FILE *fp ) );
void 	load_socials	args( ( FILE *fp ) );
void	load_specials	args( ( FILE *fp ) );
void	load_bans	args( ( void ) );
void	load_mobprogs	args( ( FILE *fp ) );

void	fix_exits	args( ( void ) );
void    fix_mobprogs	args( ( void ) );

void	reset_area	args( ( AREA_DATA * pArea ) );

/*
 * Big mama top level function.
 */
void boot_db()
{

    /*
     * Init some data space stuff.
     */
    {
	if ( ( string_space = (char *)calloc( 1, MAX_STRING ) ) == NULL )
	{
	    bug( "Boot_db: can't alloc %d string space.", MAX_STRING );
	    exit( 1 );
	}
	top_string	= string_space;
	fBootDb		= TRUE;
    }

    /*
     * Init random number generator.
     */
    {
        init_mm( );
    }

    /*
     * Set time and weather.
     */
	{
		long lhour, lday, lmonth;

		lhour		= (current_time - 650336715)
				/ (PULSE_TICK / PULSE_PER_SECOND);
		time_info.hour	= lhour  % 24;
		lday		= lhour  / 24;
		time_info.day	= lday   % 35;
		lmonth		= lday   / 35;
		time_info.month	= lmonth % 17;
		time_info.year	= lmonth / 17;

		if ( time_info.hour <  5 ) weather_info.sunlight = SUN_DARK;
		else if ( time_info.hour <  6 ) weather_info.sunlight = SUN_RISE;
		else if ( time_info.hour < 19 ) weather_info.sunlight = SUN_LIGHT;
		else if ( time_info.hour < 20 ) weather_info.sunlight = SUN_SET;
		else                            weather_info.sunlight = SUN_DARK;

		weather_info.change	= 0;
		weather_info.mmhg	= 960;
		if ( time_info.month >= 7 && time_info.month <=12 )
			weather_info.mmhg += number_range( 1, 50 );
		else
			weather_info.mmhg += number_range( 1, 80 );

		if ( weather_info.mmhg <=  980 ) weather_info.sky = SKY_LIGHTNING;
		else if ( weather_info.mmhg <= 1000 ) weather_info.sky = SKY_RAINING;
		else if ( weather_info.mmhg <= 1020 ) weather_info.sky = SKY_CLOUDY;
		else                                  weather_info.sky = SKY_CLOUDLESS;

	}

    /*
     * Assign gsn's for skills which have them.
     */
    {
	int sn;

	for ( sn = 0; sn < MAX_SKILL; sn++ )
	{
	    if ( skill_table[sn].pgsn != NULL )
		*skill_table[sn].pgsn = sn;
	}
    }

    clan_manager->load_clans();
    /*
     * Read in all the area files.
     */
    {
	FILE *fpList;

	if ( ( fpList = fopen( AREA_LIST, "r" ) ) == NULL )
	{
	    perror( AREA_LIST );
	    exit( 1 );
	}

	for ( ; ; )
	{
	    strcpy( strArea, fread_word( fpList ) );
	    if ( strArea[0] == '$' )
		break;

	    if ( strArea[0] == '-' )
	    {
		fpArea = stdin;
	    }
	    else
	    {
		if ( ( fpArea = fopen( strArea, "r" ) ) == NULL )
		{
		    perror( strArea );
		    exit( 1 );
		}
	    }

	    current_area = NULL;

	    for ( ; ; )
	    {
		char *word;

		if ( fread_letter( fpArea ) != '#' )
		{
		    bug( "Boot_db: # not found.", 0 );
		    exit( 1 );
		}

		word = fread_word( fpArea );

		     if ( word[0] == '$'               )                 break;
		else if ( !str_cmp( word, "AREA"     ) ) load_area    (fpArea);
  /* OLC */     else if ( !str_cmp( word, "AREADATA" ) ) new_load_area(fpArea);
		else if ( !str_cmp( word, "HELPS"    ) ) load_helps   (fpArea, strArea);
		else if ( !str_cmp( word, "MOBILES"  ) ) load_mobiles (fpArea);
		else if ( !str_cmp( word, "MOBPROGS" ) ) load_mobprogs(fpArea);
	  	else if ( !str_cmp( word, "OBJECTS"  ) ) load_objects (fpArea);
		else if ( !str_cmp( word, "RESETS"   ) ) load_resets  (fpArea);
		else if ( !str_cmp( word, "ROOMS"    ) ) load_rooms   (fpArea);
		else if ( !str_cmp( word, "SHOPS"    ) ) load_shops   (fpArea);
		else if ( !str_cmp( word, "SOCIALS"  ) ) load_socials (fpArea);
		else if ( !str_cmp( word, "SPECIALS" ) ) load_specials(fpArea);
		else
		{
		    bug( "Boot_db: bad section name.", 0 );
		    exit( 1 );
		}
	    }

	    if ( fpArea != stdin )
		fclose( fpArea );
	    fpArea = NULL;
	}
	fclose( fpList );
    }

    /*
     * Fix up exits.
     * Declare db booting over.
     * Reset all areas once.
     * Load up the songs, notes and ban files.
     */
    {
	fix_exits( );
	fix_mobprogs( );
	fBootDb	= FALSE;
        convert_objects( );           /* ROM OLC */
	area_update( );
        load_boards(); /* Load all boards */
        save_notes();
	load_bans();
	load_songs();
    EquipmentListGenerator::Generate();
    }

    return;
}



/*
 * Snarf an 'area' header line.
 */
void load_area( FILE *fp )
{
    AREA_DATA *pArea;

    pArea		= (AREA_DATA*)alloc_perm( sizeof(*pArea) );
/*  pArea->reset_first	= NULL;
    pArea->reset_last	= NULL; */
    pArea->file_name	= fread_string(fp);

    pArea->area_flags   = AREA_LOADING;         /* OLC */
    pArea->security     = 9;                    /* OLC */ /* 9 -- Hugin */
    pArea->builders     = str_dup( "None" );    /* OLC */
    pArea->vnum         = top_area;             /* OLC */

    pArea->name		= fread_string( fp );
    pArea->credits	= fread_string( fp );
    pArea->min_vnum	= fread_number(fp);
    pArea->max_vnum	= fread_number(fp);
    pArea->age		= 15;
    pArea->nplayer	= 0;
    pArea->empty	= FALSE;

    if ( !area_first )
	area_first = pArea;
    if ( area_last )
    {
	area_last->next = pArea;
        REMOVE_BIT(area_last->area_flags, AREA_LOADING);        /* OLC */
    }
    area_last		= pArea;
    pArea->next		= NULL;
    current_area	= pArea;

    top_area++;
    return;
}

/*
 * OLC
 * Use these macros to load any new area formats that you choose to
 * support on your MUD.  See the new_load_area format below for
 * a short example.
 */
#if defined(KEY)
#undef KEY
#endif

#define KEY( literal, field, value )                \
                if ( !str_cmp( word, literal ) )    \
                {                                   \
                    field  = value;                 \
                    break;                          \
                                }

#define SKEY( string, field )                       \
                if ( !str_cmp( word, string ) )     \
                {                                   \
                    free_string( field );           \
                    field = fread_string( fp );     \
                    break;                          \
                                }



/* OLC
 * Snarf an 'area' header line.   Check this format.  MUCH better.  Add fields
 * too.
 *
 * #AREAFILE
 * Name   { All } Locke    Newbie School~
 * Repop  A teacher pops in the room and says, 'Repop coming!'~
 * Recall 3001
 * End
 */
void new_load_area( FILE *fp )
{
    AREA_DATA *pArea;
    char      *word;

    pArea               = (AREA_DATA*)alloc_perm( sizeof(*pArea) );
    pArea->age          = 15;
    pArea->nplayer      = 0;
    pArea->file_name     = str_dup( strArea );
    pArea->vnum         = top_area;
    pArea->name         = str_dup( "New Area" );
    pArea->builders     = str_dup( "" );
    pArea->security     = 9;                    /* 9 -- Hugin */
    pArea->min_vnum        = 0;
    pArea->max_vnum        = 0;
    pArea->area_flags   = 0;
/*  pArea->recall       = ROOM_VNUM_TEMPLE;        ROM OLC */

    for ( ; ; )
    {
       word   = feof( fp ) ? (char*)"End" : fread_word( fp );

       switch ( UPPER(word[0]) )
       {
           case 'N':
            SKEY( "Name", pArea->name );
            break;
           case 'S':
             KEY( "Security", pArea->security, fread_number( fp ) );
            break;
	   case 'F':
	     KEY( "Flags", pArea->area_flags, fread_flag( fp ) );
	    break;
           case 'V':
            if ( !str_cmp( word, "VNUMs" ) )
            {
                pArea->min_vnum = fread_number( fp );
                pArea->max_vnum = fread_number( fp );
            }
            break;
           case 'E':
             if ( !str_cmp( word, "End" ) )
             {
                 if ( area_first == NULL )
                    area_first = pArea;
                 if ( area_last  != NULL )
                    area_last->next = pArea;
                 area_last	= pArea;
                 pArea->next	= NULL;
		 current_area	= pArea;
                 top_area++;

                 return;
            }
            break;
           case 'B':
            SKEY( "Builders", pArea->builders );
            break;
	   case 'C':
	    SKEY( "Credits", pArea->credits );
	    break;
        }
    }
}

/*
 * Sets vnum range for area using OLC protection features.
 */
void assign_area_vnum( int vnum )
{
    if ( area_last->min_vnum == 0 || area_last->max_vnum == 0 )
        area_last->min_vnum = area_last->max_vnum = vnum;
    if ( vnum != URANGE( area_last->min_vnum, vnum, area_last->max_vnum ) )
    {
        if ( vnum < area_last->min_vnum )
            area_last->min_vnum = vnum;
        else
            area_last->max_vnum = vnum;
    }
    return;
}

/*
 * Snarf a help section.
 */
void load_helps( FILE *fp, char *fname )
{
    HELP_DATA *pHelp;
    int level;
    char *keyword;

    for ( ; ; )
    {
	HELP_AREA * had;

	level		= fread_number( fp );
	keyword		= fread_string( fp );

	if ( keyword[0] == '$' )
		break;

	if ( !had_list )
	{
		had			= new_had ();
		had->filename		= str_dup( fname );
		had->area		= current_area;
		if ( current_area )
			current_area->helps	= had;
		had_list		= had;
	}
	else
	if ( str_cmp( fname, had_list->filename ) )
	{
		had			= new_had ();
		had->filename		= str_dup( fname );
		had->area		= current_area;
		if ( current_area )
			current_area->helps	= had;
		had->next		= had_list;
		had_list		= had;
	}
	else
		had			= had_list;

	pHelp		= new_help( );
	pHelp->level	= level;
	pHelp->keyword	= keyword;
	pHelp->text	= fread_string( fp );

	if ( !str_cmp( pHelp->keyword, "greeting" ) )
	    help_greeting = pHelp->text;

	if ( help_first == NULL )
	    help_first = pHelp;
	if ( help_last  != NULL )
	    help_last->next = pHelp;

	help_last		= pHelp;
	pHelp->next		= NULL;

	if ( !had->first )
		had->first	= pHelp;
	if ( !had->last )
		had->last	= pHelp;

	had->last->next_area	= pHelp;
	had->last		= pHelp;
	pHelp->next_area	= NULL;

	top_help++;
    }

    return;
}

/*
 * Adds a reset to a room.  OLC
 * Similar to add_reset in olc.c
 */
void new_reset( ROOM_INDEX_DATA *pR, RESET_DATA *pReset )
{
    RESET_DATA *pr;

    if ( !pR )
       return;

    pr = pR->reset_last;

    if ( !pr )
    {
        pR->reset_first = pReset;
        pR->reset_last  = pReset;
    }
    else
    {
        pR->reset_last->next = pReset;
        pR->reset_last       = pReset;
        pR->reset_last->next = NULL;
    }

    top_reset++;
    return;
}

/*
 * Snarf a reset section.
 */
void load_resets( FILE *fp )
{
    RESET_DATA *pReset;
    int         iLastRoom = 0;
    int         iLastObj  = 0;

    if ( !area_last )
    {
	bug( "Load_resets: no #AREA seen yet.", 0 );
	exit( 1 );
    }

    for ( ; ; )
    {
	ROOM_INDEX_DATA *pRoomIndex;
	EXIT_DATA *pexit;
	char letter;
	OBJ_INDEX_DATA *temp_index;

	if ( ( letter = fread_letter( fp ) ) == 'S' )
	    break;

	if ( letter == '*' )
	{
	    fread_to_eol( fp );
	    continue;
	}

	pReset		= (RESET_DATA*)alloc_perm( sizeof(*pReset) );
	pReset->command	= letter;
	/* if_flag */	  fread_number( fp );
	pReset->arg1	= fread_number( fp );
	pReset->arg2	= fread_number( fp );
	pReset->arg3	= (letter == 'G' || letter == 'R')
			    ? 0 : fread_number( fp );
	pReset->arg4	= (letter == 'P' || letter == 'M')
			    ? fread_number(fp) : 0;
			  fread_to_eol( fp );

	/*
	 * Validate parameters.
	 * We're calling the index functions for the side effect.
	 */
	switch ( letter )
	{
	default:
	    bug( "Load_resets: bad command '%c'.", letter );
	    exit( 1 );
	    break;

	case 'M':
	    get_mob_index  ( pReset->arg1 );
            if ( ( pRoomIndex = get_room_index ( pReset->arg3 ) ) )
            {
                new_reset( pRoomIndex, pReset );
                iLastRoom = pReset->arg3;
            }
	    break;

	case 'O':
	    temp_index = get_obj_index  ( pReset->arg1 );
	    temp_index->reset_num++;
            if ( ( pRoomIndex = get_room_index ( pReset->arg3 ) ) )
            {
                new_reset( pRoomIndex, pReset );
                iLastObj = pReset->arg3;
            }
	    break;

	case 'P':
	    temp_index = get_obj_index  ( pReset->arg1 );
	    temp_index->reset_num++;
            if ( ( pRoomIndex = get_room_index ( iLastObj ) ) )
            {
                new_reset( pRoomIndex, pReset );
            }
	    break;

	case 'G':
	case 'E':
	    temp_index = get_obj_index  ( pReset->arg1 );
	    temp_index->reset_num++;
            if ( ( pRoomIndex = get_room_index ( iLastRoom ) ) )
            {
                new_reset( pRoomIndex, pReset );
                iLastObj = iLastRoom;
            }
	    break;

	case 'D':
	    pRoomIndex = get_room_index( pReset->arg1 );

	    if ( pReset->arg2 < 0
	    ||   pReset->arg2 > (MAX_DIR - 1)
            || !pRoomIndex
	    || !( pexit = pRoomIndex->exit[pReset->arg2] )
	    || !IS_SET( pexit->rs_flags, ExitFlag::ExitIsDoor ) )
	    {
		bug( "Load_resets: 'D': exit %d not door.", pReset->arg2 );
		exit( 1 );
	    }

            switch ( pReset->arg3 )
            {
                default:
                    bug( "Load_resets: 'D': bad 'locks': %d." , pReset->arg3);
                case 0: break;
                case 1: SET_BIT( pexit->rs_flags, ExitFlag::ExitClosed );
			SET_BIT( pexit->exit_info, ExitFlag::ExitClosed ); break;
                case 2: SET_BIT( pexit->rs_flags, ExitFlag::ExitClosed | ExitFlag::ExitLocked );
                	SET_BIT( pexit->exit_info, ExitFlag::ExitClosed | ExitFlag::ExitLocked ); break;
            }

	    break;

	case 'R':
	    pRoomIndex		= get_room_index( pReset->arg1 );

	    if ( pReset->arg2 < 0 || pReset->arg2 > MAX_DIR )
	    {
		bug( "Load_resets: 'R': bad exit %d.", pReset->arg2 );
		exit( 1 );
	    }

            if ( pRoomIndex )
                new_reset( pRoomIndex, pReset );

	    break;
	}

    }

    return;
}

/*
 * Snarf a room section.
 */
void load_rooms( FILE *fp )
{
    ROOM_INDEX_DATA *pRoomIndex;

    if ( area_last == NULL )
    {
	bug( "Load_resets: no #AREA seen yet.", 0 );
	exit( 1 );
    }

    for ( ; ; )
    {
	sh_int vnum;
	char letter;
	int door, iHash, sec;

	letter				= fread_letter( fp );
	if ( letter != '#' )
	{
	    bug( "Load_rooms: # not found.", 0 );
	    exit( 1 );
	}

	vnum				= fread_number( fp );
	if ( vnum == 0 )
	    break;

	fBootDb = FALSE;
	if ( get_room_index( vnum ) != NULL )
	{
	    bug( "Load_rooms: vnum %d duplicated.", vnum );
	    exit( 1 );
	}
	fBootDb = TRUE;

	pRoomIndex			= new ROOM_INDEX_DATA();
	pRoomIndex->owner		= str_dup("");
	pRoomIndex->people		= NULL;
	pRoomIndex->contents		= std::list<Object *>();
	pRoomIndex->extra_descr		= std::vector<ExtraDescription *>();
	pRoomIndex->area		= area_last;
	sec				= pRoomIndex->area->security;
	pRoomIndex->vnum		= vnum;
	pRoomIndex->name		= fread_string( fp );
	pRoomIndex->description		= fread_string( fp );
	/* Area number */		  fread_number( fp );
	pRoomIndex->room_flags		= fread_flag( fp );
	/* horrible hack */
  	if ( 3000 <= vnum && vnum < 3400)
	   SET_BIT(pRoomIndex->room_flags,ROOM_LAW);
	pRoomIndex->sector_type		= fread_number( fp );
	pRoomIndex->light		= 0;
	for ( door = 0; door <= 5; door++ )
	    pRoomIndex->exit[door] = NULL;

	/* defaults */
	pRoomIndex->heal_rate = 100;
	pRoomIndex->mana_rate = 100;

	for ( ; ; )
	{
	    letter = fread_letter( fp );

	    if ( letter == 'S' )
		break;

	    if ( letter == 'H') /* healing room */
		pRoomIndex->heal_rate = fread_number(fp);
	
	    else if ( letter == 'M') /* mana room */
		pRoomIndex->mana_rate = fread_number(fp);

           else if ( letter == 'C') /* clan */
           {
                if (pRoomIndex->clan)
                {
                    bug("Load_rooms: duplicate clan fields.",0);
                    exit(1);
                }
                pRoomIndex->clan = clan_manager->get_clan(fread_string(fp));
            }

	    else if ( letter == 'D' )
	    {
		EXIT_DATA *pexit;
		int locks;

		door = fread_number( fp );
		if ( door < 0 || door > 5 )
		{
		    bug( "Fread_rooms: vnum %d has bad door number.", vnum );
		    exit( 1 );
		}

		pexit			= (EXIT_DATA*)alloc_perm( sizeof(*pexit) );
		pexit->description	= fread_string( fp );
		if (sec == 4)
		    fread_string( fp );
		pexit->keyword		= fread_string( fp );
		pexit->exit_info	= 0;
                pexit->rs_flags         = 0;                    /* OLC */
		locks			= fread_number( fp );
		pexit->key		= fread_number( fp );
		pexit->u1.vnum		= fread_number( fp );
		pexit->orig_door	= door;			/* OLC */

		switch ( locks )
		{
		case 1: pexit->exit_info = ExitFlag::ExitIsDoor;               
			pexit->rs_flags  = ExitFlag::ExitIsDoor;		     break;
		case 2: pexit->exit_info = ExitFlag::ExitIsDoor | ExitFlag::ExitPickProof;
			pexit->rs_flags  = ExitFlag::ExitIsDoor | ExitFlag::ExitPickProof; break;
		case 3: pexit->exit_info = ExitFlag::ExitIsDoor | ExitFlag::ExitNoPass;    
			pexit->rs_flags  = ExitFlag::ExitIsDoor | ExitFlag::ExitNoPass;    break;
		case 4: pexit->exit_info = ExitFlag::ExitIsDoor|ExitFlag::ExitNoPass|ExitFlag::ExitPickProof;
			pexit->rs_flags  = ExitFlag::ExitIsDoor|ExitFlag::ExitNoPass|ExitFlag::ExitPickProof;
			break;
		}

		pRoomIndex->exit[door]	= pexit;
		top_exit++;
	    }
	    else if ( letter == 'E' )
	    {
            ExtraDescription *ed = new ExtraDescription();
            ed->setKeyword(fread_string( fp ) );
            ed->setDescription(fread_string( fp ));
            pRoomIndex->extra_descr.push_back(ed);
            top_ed++;
	    }

	    else if (letter == 'O')
	    {
		if (pRoomIndex->owner[0] != '\0')
		{
		    bug("Load_rooms: duplicate owner.",0);
		    exit(1);
		}

		pRoomIndex->owner = fread_string(fp);
	    }

	    else
	    {
		bug( "Load_rooms: vnum %d has flag not 'DES'.", vnum );
		exit( 1 );
	    }
	}

	iHash			= vnum % MAX_KEY_HASH;
	pRoomIndex->next	= room_index_hash[iHash];
	room_index_hash[iHash]	= pRoomIndex;
	top_room++;
        top_vnum_room = top_vnum_room < vnum ? vnum : top_vnum_room; /* OLC */
        assign_area_vnum( vnum );                                    /* OLC */
    }

    return;
}



/*
 * Snarf a shop section.
 */
void load_shops( FILE *fp )
{
    SHOP_DATA *pShop;

    for ( ; ; )
    {
	MOB_INDEX_DATA *pMobIndex;
	int iTrade;

	pShop			= (SHOP_DATA*)alloc_perm( sizeof(*pShop) );
	pShop->keeper		= fread_number( fp );
	if ( pShop->keeper == 0 )
	    break;
	for ( iTrade = 0; iTrade < MAX_TRADE; iTrade++ )
	    pShop->buy_type[iTrade]	= fread_number( fp );
	pShop->profit_buy	= fread_number( fp );
	pShop->profit_sell	= fread_number( fp );
	pShop->open_hour	= fread_number( fp );
	pShop->close_hour	= fread_number( fp );
				  fread_to_eol( fp );
	pMobIndex		= get_mob_index( pShop->keeper );
	pMobIndex->pShop	= pShop;

	if ( shop_first == NULL )
	    shop_first = pShop;
	if ( shop_last  != NULL )
	    shop_last->next = pShop;

	shop_last	= pShop;
	pShop->next	= NULL;
	top_shop++;
    }

    return;
}


/*
 * Snarf spec proc declarations.
 */
void load_specials( FILE *fp )
{
    for ( ; ; )
    {
	MOB_INDEX_DATA *pMobIndex;
	char letter;

	switch ( letter = fread_letter( fp ) )
	{
	default:
	    bug( "Load_specials: letter '%c' not *MS.", letter );
	    exit( 1 );

	case 'S':
	    return;

	case '*':
	    break;

	case 'M':
	    pMobIndex		= get_mob_index	( fread_number ( fp ) );
	    pMobIndex->spec_fun	= spec_lookup	( fread_word   ( fp ) );
	    if ( pMobIndex->spec_fun == 0 )
	    {
		bug( "Load_specials: 'M': vnum %d.", pMobIndex->vnum );
		exit( 1 );
	    }
	    break;
	}

	fread_to_eol( fp );
    }
}


/*
 * Translate all room exits from virtual to real.
 * Has to be done after all rooms are read in.
 * Check for bad reverse exits.
 */
void fix_exits( void )
{
    extern const sh_int rev_dir [];
    char buf[MAX_STRING_LENGTH];
    ROOM_INDEX_DATA *pRoomIndex;
    ROOM_INDEX_DATA *to_room;
    EXIT_DATA *pexit;
    EXIT_DATA *pexit_rev;
    int iHash;
    int door;

    for ( iHash = 0; iHash < MAX_KEY_HASH; iHash++ )
    {
	for ( pRoomIndex  = room_index_hash[iHash];
	      pRoomIndex != NULL;
	      pRoomIndex  = pRoomIndex->next )
	{
	    bool fexit;

	    fexit = FALSE;
	    for ( door = 0; door <= 5; door++ )
	    {
		if ( ( pexit = pRoomIndex->exit[door] ) != NULL )
		{
		    if ( pexit->u1.vnum <= 0 
		    || get_room_index(pexit->u1.vnum) == NULL)
			pexit->u1.to_room = NULL;
		    else
		    {
		   	fexit = TRUE; 
			pexit->u1.to_room = get_room_index( pexit->u1.vnum );
		    }
		}
	    }
	    if (!fexit)
		SET_BIT(pRoomIndex->room_flags,ROOM_NO_MOB);
	}
    }

    for ( iHash = 0; iHash < MAX_KEY_HASH; iHash++ )
    {
	for ( pRoomIndex  = room_index_hash[iHash];
	      pRoomIndex != NULL;
	      pRoomIndex  = pRoomIndex->next )
	{
	    for ( door = 0; door <= 5; door++ )
	    {
		if ( ( pexit     = pRoomIndex->exit[door]       ) != NULL
		&&   ( to_room   = pexit->u1.to_room            ) != NULL
		&&   ( pexit_rev = to_room->exit[rev_dir[door]] ) != NULL
		&&   pexit_rev->u1.to_room != pRoomIndex 
		&&   (pRoomIndex->vnum < 1200 || pRoomIndex->vnum > 1299))
		{
		    snprintf(buf, sizeof(buf), "Fix_exits: %d:%d -> %d:%d -> %d.",
			pRoomIndex->vnum, door,
			to_room->vnum,    rev_dir[door],
			(pexit_rev->u1.to_room == NULL)
			    ? 0 : pexit_rev->u1.to_room->vnum );
		    bug( buf, 0 );
		}
	    }
	}
    }

    return;
}

/*
 * Load mobprogs section
 */
void load_mobprogs( FILE *fp )
{
    MPROG_CODE *pMprog;

    if ( area_last == NULL )
    {
	bug( "Load_mobprogs: no #AREA seen yet.", 0 );
	exit( 1 );
    }

    for ( ; ; )
    {
	sh_int vnum;
	char letter;

	letter		  = fread_letter( fp );
	if ( letter != '#' )
	{
	    bug( "Load_mobprogs: # not found.", 0 );
	    exit( 1 );
	}

	vnum		 = fread_number( fp );
	if ( vnum == 0 )
	    break;

	fBootDb = FALSE;
	if ( get_mprog_index( vnum ) != NULL )
	{
	    bug( "Load_mobprogs: vnum %d duplicated.", vnum );
	    exit( 1 );
	}
	fBootDb = TRUE;

	pMprog		= (MPROG_CODE*)alloc_perm( sizeof(*pMprog) );
	pMprog->vnum  	= vnum;
	pMprog->code  	= fread_string( fp );
	if ( mprog_list == NULL )
	    mprog_list = pMprog;
	else
	{
	    pMprog->next = mprog_list;
	    mprog_list 	= pMprog;
	}
	top_mprog_index++;
    }
    return;
}

/*
 *  Translate mobprog vnums pointers to real code
 */
void fix_mobprogs( void )
{
    MOB_INDEX_DATA *pMobIndex;
    MPROG_LIST        *list;
    MPROG_CODE        *prog;
    int iHash;

    for ( iHash = 0; iHash < MAX_KEY_HASH; iHash++ )
    {
	for ( pMobIndex   = mob_index_hash[iHash];
	      pMobIndex   != NULL;
	      pMobIndex   = pMobIndex->next )
	{
	    for( list = pMobIndex->mprogs; list != NULL; list = list->next )
	    {
		if ( ( prog = get_mprog_index( list->vnum ) ) != NULL )
		    list->code = prog->code;
		else
		{
		    bug( "Fix_mobprogs: code vnum %d not found.", list->vnum );
		    exit( 1 );
		}
	    }
	}
    }
}


/*
 * Repopulate areas periodically.
 */
void area_update( void )
{
    AREA_DATA *pArea;
    char buf[MAX_STRING_LENGTH];

    for ( pArea = area_first; pArea != NULL; pArea = pArea->next )
    {

	if ( ++pArea->age < 3 )
	    continue;

	/*
	 * Check age and reset.
	 * Note: Mud School resets every 3 minutes (not 15).
	 */
	if ( (!pArea->empty && (pArea->nplayer == 0 || pArea->age >= 15))
	||    pArea->age >= 31)
	{
	    ROOM_INDEX_DATA *pRoomIndex;

	    reset_area( pArea );
	    snprintf(buf, sizeof(buf),"%s has just been reset.",pArea->name);
	    Wiznet::instance()->report(buf,NULL,NULL,WIZ_RESETS,0,0);
	
	    pArea->age = number_range( 0, 3 );
	    pRoomIndex = get_room_index( ROOM_VNUM_SCHOOL );
	    if ( pRoomIndex != NULL && pArea == pRoomIndex->area )
		pArea->age = 15 - 2;
	    else if (pArea->nplayer == 0) 
		pArea->empty = TRUE;
	}
    }

    return;
}

/* OLC
 * Reset one room.  Called by reset_area and olc.
 */
void reset_room( ROOM_INDEX_DATA *pRoom )
{
    RESET_DATA  *pReset;
    Character   *pMob;
    Character	*mob;
    Object    *pObj;
    Character   *LastMob = NULL;
    Object    *LastObj = NULL;
    int iExit;
    int level = 0;
    bool last;

    if ( !pRoom )
        return;

    pMob        = NULL;
    last        = FALSE;
    
    for ( iExit = 0;  iExit < MAX_DIR;  iExit++ )
    {
        EXIT_DATA *pExit;
        if ( ( pExit = pRoom->exit[iExit] )
	  /*  && !IS_SET( pExit->exit_info, EX_BASHED )   ROM OLC */ )  
        {
            pExit->exit_info = pExit->rs_flags;
            if ( ( pExit->u1.to_room != NULL )
              && ( ( pExit = pExit->u1.to_room->exit[rev_dir[iExit]] ) ) )
            {
                /* nail the other side */
                pExit->exit_info = pExit->rs_flags;
            }
        }
    }

    for ( pReset = pRoom->reset_first; pReset != NULL; pReset = pReset->next )
    {
        MOB_INDEX_DATA  *pMobIndex;
        OBJ_INDEX_DATA  *pObjIndex;
        OBJ_INDEX_DATA  *pObjToIndex;
        ROOM_INDEX_DATA *pRoomIndex;
	char buf[MAX_STRING_LENGTH];
	int count,limit=0;

        switch ( pReset->command )
        {
        default:
                bug( "Reset_room: bad command %c.", pReset->command );
                break;

        case 'M':
            if ( !( pMobIndex = get_mob_index( pReset->arg1 ) ) )
            {
                bug( "Reset_room: 'M': bad vnum %d.", pReset->arg1 );
                continue;
            }

	    if ( ( pRoomIndex = get_room_index( pReset->arg3 ) ) == NULL )
	    {
		bug( "Reset_area: 'R': bad vnum %d.", pReset->arg3 );
		continue;
	    }
            if ( pMobIndex->count >= pReset->arg2 )
            {
                last = FALSE;
                break;
            }
/* */
	    count = 0;
	    for (mob = pRoomIndex->people; mob != NULL; mob = mob->next_in_room)
		if (mob->pIndexData == pMobIndex)
		{
		    count++;
		    if (count >= pReset->arg4)
		    {
		    	last = FALSE;
		    	break;
		    }
		}

	    if (count >= pReset->arg4)
		break;

/* */

            pMob = new NonPlayerCharacter( pMobIndex );

            /*
             * Pet shop mobiles get ACT_PET set.
             */
            {
                ROOM_INDEX_DATA *pRoomIndexPrev;

                pRoomIndexPrev = get_room_index( pRoom->vnum - 1 );
                if ( pRoomIndexPrev
                    && IS_SET( pRoomIndexPrev->room_flags, ROOM_PET_SHOP ) )
                    SET_BIT( pMob->act, ACT_PET);
            }

            char_to_room( pMob, pRoom );

            LastMob = pMob;
            level  = URANGE( 0, pMob->level - 2, LEVEL_HERO - 1 ); /* -1 ROM */
            last = TRUE;
            break;

        case 'O':
            if ( !( pObjIndex = get_obj_index( pReset->arg1 ) ) )
            {
                bug( "Reset_room: 'O' 1 : bad vnum %d", pReset->arg1 );
                snprintf(buf, sizeof(buf),"%d %d %d %d",pReset->arg1, pReset->arg2, pReset->arg3,
                pReset->arg4 );
		bug(buf,1);
                continue;
            }

            if ( !( pRoomIndex = get_room_index( pReset->arg3 ) ) )
            {
                bug( "Reset_room: 'O' 2 : bad vnum %d.", pReset->arg3 );
                snprintf(buf, sizeof(buf),"%d %d %d %d",pReset->arg1, pReset->arg2, pReset->arg3,
                pReset->arg4 );
		bug(buf,1);
                continue;
            }

            if ( pRoom->area->nplayer > 0
              || ObjectHelper::countInList( pObjIndex, pRoom->contents ) > 0 )
	    {
		last = FALSE;
		break;
	    }

            pObj = ObjectHelper::createFromIndex( pObjIndex,              /* UMIN - ROM OLC */
				  UMIN(number_fuzzy( level ), LEVEL_HERO -1) );
            pObj->setCost(0);
            obj_to_room( pObj, pRoom );
	    last = TRUE;
            break;

        case 'P':
            if ( !( pObjIndex = get_obj_index( pReset->arg1 ) ) )
            {
                bug( "Reset_room: 'P': bad vnum %d.", pReset->arg1 );
                continue;
            }

            if ( !( pObjToIndex = get_obj_index( pReset->arg3 ) ) )
            {
                bug( "Reset_room: 'P': bad vnum %d.", pReset->arg3 );
                continue;
            }

            if (pReset->arg2 > 50) /* old format */
                limit = 6;
            else if (pReset->arg2 == -1) /* no limit */
                limit = 999;
            else
                limit = pReset->arg2;

            if ( pRoom->area->nplayer > 0
              || ( LastObj = get_obj_type( pObjToIndex ) ) == NULL
              || ( LastObj->getInRoom() == NULL && !last)
              || ( pObjIndex->count >= limit /* && number_range(0,4) != 0 */ )
              || ( count = ObjectHelper::countInList( pObjIndex, LastObj->getContents() ) ) > pReset->arg4  )
	    {
		last = FALSE;
		break;
	    }
				                /* lastObj->level  -  ROM */

	    while (count < pReset->arg4)
	    {
            pObj = ObjectHelper::createFromIndex( pObjIndex, number_fuzzy( LastObj->getLevel() ) );
            LastObj->addObject( pObj );
            count++;
            if (pObjIndex->count >= limit)
                break;
	    }

	    /* fix object lock state! */
	    LastObj->getValues().at(1) = LastObj->getObjectIndexData()->value[1];
	    last = TRUE;
            break;

        case 'G':
        case 'E':
            if ( !( pObjIndex = get_obj_index( pReset->arg1 ) ) )
            {
                bug( "Reset_room: 'E' or 'G': bad vnum %d.", pReset->arg1 );
                continue;
            }

            if ( !last )
                break;

            if ( !LastMob )
            {
                bug( "Reset_room: 'E' or 'G': null mob for vnum %d.",
                    pReset->arg1 );
                last = FALSE;
                break;
            }

            if ( LastMob->pIndexData->pShop )   /* Shop-keeper? */
            {
                int olevel=0,i,j;

		if (!pObjIndex->new_format)
                 switch ( pObjIndex->item_type )
                {
                default:                olevel = 0;                      break;
                case ITEM_PILL:
                case ITEM_POTION:
                case ITEM_SCROLL:
 		    olevel = 53;
		    for (i = 1; i < 5; i++)
		    {
			if (pObjIndex->value[i] > 0)
			{
		    	    for (j = 0; j < MAX_CLASS; j++)
			    {
				olevel = UMIN(olevel,
				         skill_table[pObjIndex->value[i]].
						     skill_level[j]);
			    }
			}
		    }
		   
		    olevel = UMAX(0,(olevel * 3 / 4) - 2);
		    break;
		    
               case ITEM_WAND:         olevel = number_range( 10, 20 ); break;
                case ITEM_STAFF:        olevel = number_range( 15, 25 ); break;
                case ITEM_ARMOR:        olevel = number_range(  5, 15 ); break;
                /* ROM patch weapon, treasure */
                case ITEM_WEAPON:       olevel = number_range(  5, 15 ); break;
                case ITEM_TREASURE:     olevel = number_range( 10, 20 ); break;
                  break;
                }

                pObj = ObjectHelper::createFromIndex( pObjIndex, olevel );
                pObj->addExtraFlag( ITEM_INVENTORY );  /* ROM OLC */
            }
	    else   /* ROM OLC else version */
	    {
		int limit;
		if (pReset->arg2 > 50 )  /* old format */
		    limit = 6;
		else if ( pReset->arg2 == -1 || pReset->arg2 == 0 )  /* no limit */
		    limit = 999;
		else
		    limit = pReset->arg2;

		if ( pObjIndex->count < limit || number_range(0,4) == 0 )
		{
		    pObj = ObjectHelper::createFromIndex( pObjIndex, 
			   UMIN( number_fuzzy( level ), LEVEL_HERO - 1 ) );
		    /* error message if it is too high */
		    if (pObj->getLevel() > LastMob->level + 3) {
				fprintf(stderr,
						"Err: obj %s (%d) -- %d, mob %s (%d) -- %d (obj level > mob + 3 )\n",
						pObj->getShortDescription().c_str(),pObj->getObjectIndexData()->vnum,pObj->getLevel(),
						LastMob->short_descr,LastMob->pIndexData->vnum,LastMob->level);
			} else if (pObj->getItemType() == ITEM_WEAPON
		    &&   pReset->command == 'E' 
		    &&   pObj->getLevel() < LastMob->level -5 && pObj->getLevel() < 45) {
				fprintf(stderr,
						"Err: obj %s (%d) -- %d, mob %s (%d) -- %d (weapon level 5 or less than mob)\n",
						pObj->getShortDescription().c_str(),pObj->getObjectIndexData()->vnum,pObj->getLevel(),
						LastMob->short_descr,LastMob->pIndexData->vnum,LastMob->level);
			}
		}
		else
		    break;
	    }
									 
#if 0 /* envy else version */
            else
            {
                pObj = ObjectHelper::createFromIndex( pObjIndex, number_fuzzy( level ) );
            }
#endif /* envy else version */

            LastMob->addObject( pObj );
            if ( pReset->command == 'E' )
                equip_char( LastMob, pObj, pReset->arg3 );
            last = TRUE;
            break;

        case 'D':
            break;

        case 'R':
            if ( !( pRoomIndex = get_room_index( pReset->arg1 ) ) )
            {
                bug( "Reset_room: 'R': bad vnum %d.", pReset->arg1 );
                continue;
            }

            {
                EXIT_DATA *pExit;
                int d0;
                int d1;

                for ( d0 = 0; d0 < pReset->arg2 - 1; d0++ )
                {
                    d1                   = number_range( d0, pReset->arg2-1 );
                    pExit                = pRoomIndex->exit[d0];
                    pRoomIndex->exit[d0] = pRoomIndex->exit[d1];
                    pRoomIndex->exit[d1] = pExit;
                }
            }
            break;
        }
    }

    return;
}

/* OLC
 * Reset one area.
 */
void reset_area( AREA_DATA *pArea )
{
    ROOM_INDEX_DATA *pRoom;
    int  vnum;

    for ( vnum = pArea->min_vnum; vnum <= pArea->max_vnum; vnum++ )
    {
        if ( ( pRoom = get_room_index(vnum) ) )
            reset_room(pRoom);
    }

    return;
}

/* duplicate a mobile exactly -- except inventory */
void clone_mobile(Character *parent, Character *clone)
{
    int i;

    if ( parent == NULL || clone == NULL || !IS_NPC(parent))
	return;
    
    /* start fixing values */ 
    clone->setName(parent->getName());
    clone->version	= parent->version;
    clone->short_descr	= str_dup(parent->short_descr);
    clone->long_descr	= str_dup(parent->long_descr);
    clone->setDescription(parent->getDescription());
    clone->group	= parent->group;
    clone->sex		= parent->sex;
    clone->class_num= parent->class_num;
    clone->setRace(parent->getRace());
    clone->level	= parent->level;
    clone->trust	= 0;
    clone->timer	= parent->timer;
    clone->wait		= parent->wait;
    clone->hit		= parent->hit;
    clone->max_hit	= parent->max_hit;
    clone->mana		= parent->mana;
    clone->max_mana	= parent->max_mana;
    clone->move		= parent->move;
    clone->max_move	= parent->max_move;
    clone->gold		= parent->gold;
    clone->silver	= parent->silver;
    clone->exp		= parent->exp;
    clone->act		= parent->act;
    clone->comm		= parent->comm;
    clone->imm_flags	= parent->imm_flags;
    clone->res_flags	= parent->res_flags;
    clone->vuln_flags	= parent->vuln_flags;
    clone->invis_level	= parent->invis_level;
    clone->affected_by	= parent->affected_by;
    clone->position	= parent->position;
    clone->practice	= parent->practice;
    clone->train	= parent->train;
    clone->saving_throw	= parent->saving_throw;
    clone->alignment	= parent->alignment;
    clone->hitroll	= parent->hitroll;
    clone->damroll	= parent->damroll;
    clone->wimpy	= parent->wimpy;
    clone->form		= parent->form;
    clone->parts	= parent->parts;
    clone->size		= parent->size;
    clone->material	= str_dup(parent->material);
    clone->off_flags	= parent->off_flags;
    clone->dam_type	= parent->dam_type;
    clone->start_pos	= parent->start_pos;
    clone->default_pos	= parent->default_pos;
    clone->spec_fun	= parent->spec_fun;
    
    for (i = 0; i < 4; i++)
    	clone->armor[i]	= parent->armor[i];

    for (i = 0; i < MAX_STATS; i++)
    {
	clone->perm_stat[i]	= parent->perm_stat[i];
	clone->mod_stat[i]	= parent->mod_stat[i];
    }

    for (i = 0; i < 3; i++)
	clone->damage[i]	= parent->damage[i];

    /* now add the affects */
    for (auto paf : parent->affected) {
        clone->giveAffect(paf);
    }
}



/*
 * Translates mob virtual number to its mob index struct.
 * Hash table lookup.
 */
MOB_INDEX_DATA *get_mob_index( int vnum )
{
    MOB_INDEX_DATA *pMobIndex;

    for ( pMobIndex  = mob_index_hash[vnum % MAX_KEY_HASH];
	  pMobIndex != NULL;
	  pMobIndex  = pMobIndex->next )
    {
	if ( pMobIndex->vnum == vnum )
	    return pMobIndex;
    }

    if ( fBootDb )
    {
	bug( "Get_mob_index: bad vnum %d.", vnum );
	exit( 1 );
    }

    return NULL;
}



/*
 * Translates mob virtual number to its obj index struct.
 * Hash table lookup.
 */
OBJ_INDEX_DATA *get_obj_index( int vnum )
{
    OBJ_INDEX_DATA *pObjIndex;

    for ( pObjIndex  = obj_index_hash[vnum % MAX_KEY_HASH];
	  pObjIndex != NULL;
	  pObjIndex  = pObjIndex->next )
    {
	if ( pObjIndex->vnum == vnum )
	    return pObjIndex;
    }

    if ( fBootDb )
    {
	bug( "Get_obj_index: bad vnum %d.", vnum );
	exit( 1 );
    }

    return NULL;
}

/*
 * Translates mob virtual number to its room index struct.
 * Hash table lookup.
 */
ROOM_INDEX_DATA *get_room_index( int vnum )
{
    ROOM_INDEX_DATA *pRoomIndex;

    for ( pRoomIndex  = room_index_hash[vnum % MAX_KEY_HASH];
	  pRoomIndex != NULL;
	  pRoomIndex  = pRoomIndex->next )
    {
	if ( pRoomIndex->vnum == vnum )
	    return pRoomIndex;
    }

    if ( fBootDb )
    {
	bug( "Get_room_index: bad vnum %d.", vnum );
	exit( 1 );
    }

    return NULL;
}

MPROG_CODE *get_mprog_index( int vnum )
{
    MPROG_CODE *prg;
    for( prg = mprog_list; prg; prg = prg->next )
    {
    	if ( prg->vnum == vnum )
            return( prg );
    }
    return NULL;
}    



/*
 * Read a letter from a file.
 */
char fread_letter( FILE *fp )
{
    char c;

    do
    {
	c = getc( fp );
    }
    while ( isspace(c) );

    return c;
}



/*
 * Read a number from a file.
 */
int fread_number( FILE *fp )
{
    int number;
    bool sign;
    char c;

    do
    {
	c = getc( fp );
    }
    while ( isspace(c) );

    number = 0;

    sign   = FALSE;
    if ( c == '+' )
    {
	c = getc( fp );
    }
    else if ( c == '-' )
    {
	sign = TRUE;
	c = getc( fp );
    }

    if ( !isdigit(c) )
    {
	bug( "Fread_number: bad format.", 0 );
	exit( 1 );
    }

    while ( isdigit(c) )
    {
	number = number * 10 + c - '0';
	c      = getc( fp );
    }

    if ( sign )
	number = 0 - number;

    if ( c == '|' )
	number += fread_number( fp );
    else if ( c != ' ' )
	ungetc( c, fp );

    return number;
}

long fread_flag( FILE *fp)
{
    int number;
    char c;
    bool negative = FALSE;

    do
    {
	c = getc(fp);
    }
    while ( isspace(c));

    if (c == '-')
    {
	negative = TRUE;
	c = getc(fp);
    }

    number = 0;

    if (!isdigit(c))
    {
	while (('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z'))
	{
	    number += flag_convert(c);
	    c = getc(fp);
	}
    }

    while (isdigit(c))
    {
	number = number * 10 + c - '0';
	c = getc(fp);
    }

    if (c == '|')
	number += fread_flag(fp);

    else if  ( c != ' ')
	ungetc(c,fp);

    if (negative)
	return -1 * number;

    return number;
}

long flag_convert(char letter )
{
    long bitsum = 0;
    char i;

    if ('A' <= letter && letter <= 'Z') 
    {
	bitsum = 1;
	for (i = letter; i > 'A'; i--)
	    bitsum *= 2;
    }
    else if ('a' <= letter && letter <= 'z')
    {
	bitsum = 67108864; /* 2^26 */
	for (i = letter; i > 'a'; i --)
	    bitsum *= 2;
    }

    return bitsum;
}




/*
 * Read and allocate space for a string from a file.
 * These strings are read-only and shared.
 * Strings are hashed:
 *   each string prepended with hash pointer to prev string,
 *   hash code is simply the string length.
 *   this function takes 40% to 50% of boot-up time.
 */
char *fread_string( FILE *fp )
{
    char *plast;
    char c;

    plast = top_string + sizeof(char *);
    if ( plast > &string_space[MAX_STRING - MAX_STRING_LENGTH] )
    {
	bug( "Fread_string: MAX_STRING %d exceeded.", MAX_STRING );
	exit( 1 );
    }

    /*
     * Skip blanks.
     * Read first char.
     */
    do
    {
	c = getc( fp );
    }
    while ( isspace(c) );

    if ( ( *plast++ = c ) == '~' )
	return &str_empty[0];

    for ( ;; )
    {
        /*
         * Back off the char type lookup,
         *   it was too dirty for portability.
         *   -- Furey
         */

	switch ( *plast = getc(fp) )
	{
        default:
            plast++;
            break;
 
        case EOF:
	/* temp fix */
            bug( "Fread_string: EOF", 0 );
	    return NULL;
            /* exit( 1 ); */
            break;
 
        case '\n':
            plast++;
            *plast++ = '\r';
            break;
 
        case '\r':
            break;
 
        case '~':
            plast++;
	    {
		union
		{
		    char *	pc;
		    char	rgc[sizeof(char *)];
		} u1;
		unsigned int ic;
		int iHash;
		char *pHash;
		char *pHashPrev;
		char *pString;

		plast[-1] = '\0';
		iHash     = UMIN( MAX_KEY_HASH - 1, plast - 1 - top_string );
		for ( pHash = string_hash[iHash]; pHash; pHash = pHashPrev )
		{
		    for ( ic = 0; ic < sizeof(char *); ic++ )
			u1.rgc[ic] = pHash[ic];
		    pHashPrev = u1.pc;
		    pHash    += sizeof(char *);

		    if ( top_string[sizeof(char *)] == pHash[0]
		    &&   !strcmp( top_string+sizeof(char *)+1, pHash+1 ) )
			return pHash;
		}

		if ( fBootDb )
		{
		    pString		= top_string;
		    top_string		= plast;
		    u1.pc		= string_hash[iHash];
		    for ( ic = 0; ic < sizeof(char *); ic++ )
			pString[ic] = u1.rgc[ic];
		    string_hash[iHash]	= pString;

		    nAllocString += 1;
		    sAllocString += top_string - pString;
		    return pString + sizeof(char *);
		}
		else
		{
		    return str_dup( top_string + sizeof(char *) );
		}
	    }
	}
    }
}

char *fread_string_eol( FILE *fp )
{
    static bool char_special[256-EOF];
    char *plast;
    char c;
 
    if ( char_special[EOF-EOF] != TRUE )
    {
        char_special[EOF -  EOF] = TRUE;
        char_special['\n' - EOF] = TRUE;
        char_special['\r' - EOF] = TRUE;
    }
 
    plast = top_string + sizeof(char *);
    if ( plast > &string_space[MAX_STRING - MAX_STRING_LENGTH] )
    {
        bug( "Fread_string: MAX_STRING %d exceeded.", MAX_STRING );
        exit( 1 );
    }
 
    /*
     * Skip blanks.
     * Read first char.
     */
    do
    {
        c = getc( fp );
    }
    while ( isspace(c) );
 
    if ( ( *plast++ = c ) == '\n')
        return &str_empty[0];
 
    for ( ;; )
    {
        if ( !char_special[ ( *plast++ = getc( fp ) ) - EOF ] )
            continue;
 
        switch ( plast[-1] )
        {
        default:
            break;
 
        case EOF:
            bug( "Fread_string_eol  EOF", 0 );
            exit( 1 );
            break;
 
        case '\n':  case '\r':
            {
                union
                {
                    char *      pc;
                    char        rgc[sizeof(char *)];
                } u1;
                unsigned int ic;
                int iHash;
                char *pHash;
                char *pHashPrev;
                char *pString;
 
                plast[-1] = '\0';
                iHash     = UMIN( MAX_KEY_HASH - 1, plast - 1 - top_string );
                for ( pHash = string_hash[iHash]; pHash; pHash = pHashPrev )
                {
                    for ( ic = 0; ic < sizeof(char *); ic++ )
                        u1.rgc[ic] = pHash[ic];
                    pHashPrev = u1.pc;
                    pHash    += sizeof(char *);
 
                    if ( top_string[sizeof(char *)] == pHash[0]
                    &&   !strcmp( top_string+sizeof(char *)+1, pHash+1 ) )
                        return pHash;
                }
 
                if ( fBootDb )
                {
                    pString             = top_string;
                    top_string          = plast;
                    u1.pc               = string_hash[iHash];
                    for ( ic = 0; ic < sizeof(char *); ic++ )
                        pString[ic] = u1.rgc[ic];
                    string_hash[iHash]  = pString;
 
                    nAllocString += 1;
                    sAllocString += top_string - pString;
                    return pString + sizeof(char *);
                }
                else
                {
                    return str_dup( top_string + sizeof(char *) );
                }
            }
        }
    }
}



/*
 * Read to end of line (for comments).
 */
void fread_to_eol( FILE *fp )
{
    char c;

    do
    {
	c = getc( fp );
    }
    while ( c != '\n' && c != '\r' );

    do
    {
	c = getc( fp );
    }
    while ( c == '\n' || c == '\r' );

    ungetc( c, fp );
    return;
}



/*
 * Read one word (into static buffer).
 */
char *fread_word( FILE *fp )
{
    static char word[MAX_INPUT_LENGTH];
    char *pword;
    char cEnd;

    do
    {
	cEnd = getc( fp );
    }
    while ( isspace( cEnd ) );

    if ( cEnd == '\'' || cEnd == '"' )
    {
	pword   = word;
    }
    else
    {
	word[0] = cEnd;
	pword   = word+1;
	cEnd    = ' ';
    }

    for ( ; pword < word + MAX_INPUT_LENGTH; pword++ )
    {
	*pword = getc( fp );
	if ( cEnd == ' ' ? isspace(*pword) : *pword == cEnd )
	{
	    if ( cEnd == ' ' )
		ungetc( *pword, fp );
	    *pword = '\0';
	    return word;
	}
    }

    bug( "Fread_word: word too long.", 0 );
    exit( 1 );
    return NULL;
}


/*
 * Allocate some permanent memory.
 * Permanent memory is never freed,
 *   pointers into it may be copied safely.
 */
void *alloc_perm( int sMem )
{
    static char *pMemPerm;
    static int iMemPerm;
    void *pMem;

    while ( sMem % sizeof(long) != 0 )
	sMem++;
    if ( sMem > MAX_PERM_BLOCK )
    {
	bug( "Alloc_perm: %d too large.", sMem );
	exit( 1 );
    }

    if ( pMemPerm == NULL || iMemPerm + sMem > MAX_PERM_BLOCK )
    {
	iMemPerm = 0;
	if ( ( pMemPerm = (char*)calloc( 1, MAX_PERM_BLOCK ) ) == NULL )
	{
	    perror( "Alloc_perm" );
	    exit( 1 );
	}
    }

    pMem        = pMemPerm + iMemPerm;
    iMemPerm   += sMem;
    nAllocPerm += 1;
    sAllocPerm += sMem;
    return pMem;
}



/*
 * Duplicate a string into dynamic memory.
 * Fread_strings are read-only and shared.
 */
char *str_dup( const char *str )
{
    char *str_new;

    if ( str[0] == '\0' )
	return &str_empty[0];

    if ( str >= string_space && str < top_string )
	return (char *) str;

    str_new = new char[strlen(str) + 1];
    strcpy( str_new, str );
    return str_new;
}



/*
 * Free a string.
 * Null is legal here to simplify callers.
 * Read-only shared strings are not touched.
 */
void free_string( char *pstr )
{
    if ( pstr == NULL
    ||   pstr == &str_empty[0]
    || ( pstr >= string_space && pstr < top_string ) )
	return;

    delete [] pstr;
    return;
}



void do_areas( Character *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    AREA_DATA *pArea1;
    AREA_DATA *pArea2;
    int iArea;
    int iAreaHalf;

    if (argument[0] != '\0')
    {
        send_to_char_bw("No argument is used with this command.\n\r",ch);
	return;
    }

    iAreaHalf = (top_area + 1) / 2;
    pArea1    = area_first;
    pArea2    = area_first;
    for ( iArea = 0; iArea < iAreaHalf; iArea++ )
	pArea2 = pArea2->next;

    for ( iArea = 0; iArea < iAreaHalf; iArea++ )
    {
	snprintf(buf, sizeof(buf), "%-39s%-39s\n\r",
	    pArea1->credits, (pArea2 != NULL) ? pArea2->credits : "" );
        send_to_char_bw( buf, ch );
	pArea1 = pArea1->next;
	if ( pArea2 != NULL )
	    pArea2 = pArea2->next;
    }

    return;
}



void do_memory( Character *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];

    snprintf(buf, sizeof(buf), "Affects %5d\n\r", top_affect    ); send_to_char( buf, ch );
    snprintf(buf, sizeof(buf), "Areas   %5d\n\r", top_area      ); send_to_char( buf, ch );
    snprintf(buf, sizeof(buf), "ExDes   %5d\n\r", top_ed        ); send_to_char( buf, ch );
    snprintf(buf, sizeof(buf), "Exits   %5d\n\r", top_exit      ); send_to_char( buf, ch );
    snprintf(buf, sizeof(buf), "Helps   %5d\n\r", top_help      ); send_to_char( buf, ch );
    snprintf(buf, sizeof(buf), "Socials %5d\n\r", social_count  ); send_to_char( buf, ch );
    snprintf(buf, sizeof(buf), "Mobs    %5d(%d new format)\n\r", top_mob_index,newmobs ); 
    send_to_char( buf, ch );
    snprintf(buf, sizeof(buf), "(in use)%5d\n\r", mobile_count  ); send_to_char( buf, ch );
    snprintf(buf, sizeof(buf), "Objs    %5d(%d new format)\n\r", top_obj_index,newobjs ); 
    send_to_char( buf, ch );
    snprintf(buf, sizeof(buf), "Resets  %5d\n\r", top_reset     ); send_to_char( buf, ch );
    snprintf(buf, sizeof(buf), "Rooms   %5d\n\r", top_room      ); send_to_char( buf, ch );
    snprintf(buf, sizeof(buf), "Shops   %5d\n\r", top_shop      ); send_to_char( buf, ch );

    snprintf(buf, sizeof(buf), "Strings %5d strings of %7d bytes (max %d).\n\r",
	nAllocString, sAllocString, MAX_STRING );
    send_to_char( buf, ch );

    snprintf(buf, sizeof(buf), "Perms   %5d blocks  of %7d bytes.\n\r",
	nAllocPerm, sAllocPerm );
    send_to_char( buf, ch );

    return;
}

void do_dump( Character *ch, char *argument )
{
    int count,count2,aff_count;
    Character *fch;
    MOB_INDEX_DATA *pMobIndex;
    OBJ_INDEX_DATA *pObjIndex;
    ROOM_INDEX_DATA *room;
    EXIT_DATA *exit;
    DESCRIPTOR_DATA *d;
    FILE *fp;
    int vnum,nMatch = 0;

    /* open file */
    fclose(fpReserve);
    fp = fopen("mem.dmp","w");

    /* report use of data structures */
    aff_count = 0;

    /* mobile prototypes */
    fprintf(fp,"MobProt	%4d (%8ld bytes)\n",
	top_mob_index, top_mob_index * (sizeof(*pMobIndex))); 

    /* mobs */
    count = 0;  count2 = 0;
    for (std::list<Character*>::iterator it = char_list.begin(); it != char_list.end(); it++)
    {
    	fch = *it;
        count++;
        for (auto af : fch->affected) {
            aff_count++;
        }
    }

    fprintf(fp,"Mobs	%4d (%8ld bytes)\n",
	count, count * (sizeof(*fch)));

    /* descriptors */
    count = 0; count2 = 0;
    for (d = descriptor_list; d != NULL; d = d->next)
	count++;
    for (d= descriptor_free; d != NULL; d = d->next)
	count2++;

    fprintf(fp, "Descs	%4d (%8ld bytes), %2d free (%ld bytes)\n",
	count, count * (sizeof(*d)), count2, count2 * (sizeof(*d)));

    /* object prototypes */
    for ( vnum = 0; nMatch < top_obj_index; vnum++ )
        if ( ( pObjIndex = get_obj_index( vnum ) ) != NULL )
        {
            aff_count += pObjIndex->affected.size();
            nMatch++;
        }

    fprintf(fp,"ObjProt	%4d (%8ld bytes)\n",
	top_obj_index, top_obj_index * (sizeof(*pObjIndex)));


    /* objects */
    count = 0;  count2 = 0;
    for (auto obj : object_list)
    {
        count++;
        aff_count += obj->getAffectedBy().size();
    }

    fprintf(fp,"Objs	%4d (%8ld bytes)\n",
	count, count * (sizeof(object_list.front())));

    /* affects */
    count = 0;
    AFFECT_DATA * af;
    for (af = affect_free; af != NULL; af = af->next) {
    	count++;
    }

    fprintf(fp,"Affects	%4d (%8ld bytes), %2d free (%ld bytes)\n",
	aff_count, aff_count * (sizeof(*af)), count, count * (sizeof(*af)));

    /* rooms */
    fprintf(fp,"Rooms	%4d (%8ld bytes)\n",
	top_room, top_room * (sizeof(*room)));

     /* exits */
    fprintf(fp,"Exits	%4d (%8ld bytes)\n",
	top_exit, top_exit * (sizeof(*exit)));

    fclose(fp);
    system("cp mem.dmp /home/blackavar/public_html/");

    /* start printing out mobile data */
    fp = fopen("mob.dmp","w");

    fprintf(fp,"\nMobile Analysis\n");
    fprintf(fp,"Time: %s\n", (char *) ctime( &current_time ));
    fprintf(fp,  "---------------\n");
    nMatch = 0;
    for (vnum = 0; nMatch < top_mob_index; vnum++)
	if ((pMobIndex = get_mob_index(vnum)) != NULL)
	{
	    nMatch++;
	    fprintf(fp,"#%-4d %3d active %3d killed     %s\n",
		pMobIndex->vnum,pMobIndex->count,
		pMobIndex->killed,pMobIndex->short_descr);
	}
    fclose(fp);
    system("cp mob.dmp /home/blackavar/public_html/");

    /* start printing out object data */
    fp = fopen("obj.dmp","w");

    fprintf(fp,"\nObject Analysis\n");
    fprintf(fp,  "---------------\n");
    nMatch = 0;
    for (vnum = 0; nMatch < top_obj_index; vnum++)
	if ((pObjIndex = get_obj_index(vnum)) != NULL)
	{
	    nMatch++;
	    fprintf(fp,"#%-4d %3d active %3d reset      %s\n",
		pObjIndex->vnum,pObjIndex->count,
		pObjIndex->reset_num,pObjIndex->short_descr);
	}

    /* close file */
    fclose(fp);
    fpReserve = fopen( NULL_FILE, "r" );
}



/*
 * Stick a little fuzz on a number.
 */
int number_fuzzy( int number )
{
    switch ( number_bits( 2 ) )
    {
    case 0:  number -= 1; break;
    case 3:  number += 1; break;
    }

    return UMAX( 1, number );
}



/*
 * Generate a random number.
 */
int number_range( int from, int to )
{
    int power;
    int number;

    if (from == 0 && to == 0)
	return 0;

    if ( ( to = to - from + 1 ) <= 1 )
	return from;

    for ( power = 2; power < to; power <<= 1 )
	;

    while ( ( number = number_mm() & (power -1 ) ) >= to )
	;

    return from + number;
}



/*
 * Generate a percentile roll.
 */
int number_percent( void )
{
    int percent;

    while ( (percent = number_mm() & (128-1) ) > 99 )
	;

    return 1 + percent;
}



/*
 * Generate a random door.
 */
int number_door( void )
{
    int door;

    while ( ( door = number_mm() & (8-1) ) > 5)
	;

    return door;
}

int number_bits( int width )
{
    return number_mm( ) & ( ( 1 << width ) - 1 );
}




/*
 * I've gotten too many bad reports on OS-supplied random number generators.
 * This is the Mitchell-Moore algorithm from Knuth Volume II.
 * Best to leave the constants alone unless you've read Knuth.
 * -- Furey
 */

/* I noticed streaking with this random number generator, so I switched
   back to the system srandom call.  If this doesn't work for you, 
   define OLD_RAND to use the old system -- Alander */

#if defined (OLD_RAND)
static  int     rgiState[2+55];
#endif
 
void init_mm( )
{
#if defined (OLD_RAND)
    int *piState;
    int iState;
 
    piState     = &rgiState[2];
 
    piState[-2] = 55 - 55;
    piState[-1] = 55 - 24;
 
    piState[0]  = ((int) current_time) & ((1 << 30) - 1);
    piState[1]  = 1;
    for ( iState = 2; iState < 55; iState++ )
    {
        piState[iState] = (piState[iState-1] + piState[iState-2])
                        & ((1 << 30) - 1);
    }
#else
    srandom(time(NULL)^getpid());
#endif
    return;
}
 
 
 
long number_mm( void )
{
#if defined (OLD_RAND)
    int *piState;
    int iState1;
    int iState2;
    int iRand;
 
    piState             = &rgiState[2];
    iState1             = piState[-2];
    iState2             = piState[-1];
    iRand               = (piState[iState1] + piState[iState2])
                        & ((1 << 30) - 1);
    piState[iState1]    = iRand;
    if ( ++iState1 == 55 )
        iState1 = 0;
    if ( ++iState2 == 55 )
        iState2 = 0;
    piState[-2]         = iState1;
    piState[-1]         = iState2;
    return iRand >> 6;
#else
    return random() >> 6;
#endif
}


/*
 * Roll some dice.
 */
int dice( int number, int size )
{
    int idice;
    int sum;

    switch ( size )
    {
    case 0: return 0;
    case 1: return number;
    }

    for ( idice = 0, sum = 0; idice < number; idice++ )
	sum += number_range( 1, size );

    return sum;
}



/*
 * Simple linear interpolation.
 */
int interpolate( int level, int value_00, int value_32 )
{
    return value_00 + level * (value_32 - value_00) / 32;
}



/*
 * Removes the tildes from a string.
 * Used for player-entered strings that go into disk files.
 */
void smash_tilde( char *str )
{
    for ( ; *str != '\0'; str++ )
    {
	if ( *str == '~' )
	    *str = '-';
    }

    return;
}



/*
 * Compare strings, case insensitive.
 * Return TRUE if different
 *   (compatibility with historical functions).
 */
bool str_cmp( const char *astr, const char *bstr )
{
    if ( astr == NULL )
    {
	bug( "Str_cmp: null astr.", 0 );
	return TRUE;
    }

    if ( bstr == NULL )
    {
	bug( "Str_cmp: null bstr.", 0 );
	return TRUE;
    }

    for ( ; *astr || *bstr; astr++, bstr++ )
    {
	if ( LOWER(*astr) != LOWER(*bstr) )
	    return TRUE;
    }

    return FALSE;
}



/*
 * Compare strings, case insensitive, for prefix matching.
 * Return TRUE if astr not a prefix of bstr
 *   (compatibility with historical functions).
 */
bool str_prefix( const char *astr, const char *bstr )
{
    if ( astr == NULL )
    {
	bug( "Strn_cmp: null astr.", 0 );
	return TRUE;
    }

    if ( bstr == NULL )
    {
	bug( "Strn_cmp: null bstr.", 0 );
	return TRUE;
    }

    for ( ; *astr; astr++, bstr++ )
    {
	if ( LOWER(*astr) != LOWER(*bstr) )
	    return TRUE;
    }

    return FALSE;
}



/*
 * Compare strings, case insensitive, for match anywhere.
 * Returns TRUE is astr not part of bstr.
 *   (compatibility with historical functions).
 */
bool str_infix( const char *astr, const char *bstr )
{
    int sstr1;
    int sstr2;
    int ichar;
    char c0;

    if ( ( c0 = LOWER(astr[0]) ) == '\0' )
	return FALSE;

    sstr1 = strlen(astr);
    sstr2 = strlen(bstr);

    for ( ichar = 0; ichar <= sstr2 - sstr1; ichar++ )
    {
	if ( c0 == LOWER(bstr[ichar]) && !str_prefix( astr, bstr + ichar ) )
	    return FALSE;
    }

    return TRUE;
}



/*
 * Compare strings, case insensitive, for suffix matching.
 * Return TRUE if astr not a suffix of bstr
 *   (compatibility with historical functions).
 */
bool str_suffix( const char *astr, const char *bstr )
{
    int sstr1;
    int sstr2;

    sstr1 = strlen(astr);
    sstr2 = strlen(bstr);
    if ( sstr1 <= sstr2 && !str_cmp( astr, bstr + sstr2 - sstr1 ) )
	return FALSE;
    else
	return TRUE;
}



/*
 * Returns an initial-capped string.
 */
char *capitalize( const char *str )
{
    static char strcap[MAX_STRING_LENGTH];
    int i;

    for ( i = 0; str[i] != '\0'; i++ )
	strcap[i] = LOWER(str[i]);
    strcap[i] = '\0';
    strcap[0] = UPPER(strcap[0]);
    return strcap;
}


/*
 * Append a string to a file.
 */
void append_file( Character *ch, const char *file, const char *str )
{
    FILE *fp;

    if ( ch->isNPC() || str[0] == '\0' )
	return;

    fclose( fpReserve );
    if ( ( fp = fopen( file, "a" ) ) == NULL )
    {
	perror( file );
	send_to_char( "Could not open the file!\n\r", ch );
    }
    else
    {
	fprintf( fp, "[%5d] %s: %s\n",
	    ch->in_room ? ch->in_room->vnum : 0, ch->getName().c_str(), str );
	fclose( fp );
    }

    fpReserve = fopen( NULL_FILE, "r" );
    return;
}



/*
 * Reports a bug.
 */
void bug( const char *str, int param )
{
    char buf[MAX_STRING_LENGTH];

    if ( fpArea != NULL )
    {
	int iLine;
	int iChar;

	if ( fpArea == stdin )
	{
	    iLine = 0;
	}
	else
	{
	    iChar = ftell( fpArea );
	    fseek( fpArea, 0, 0 );
	    for ( iLine = 0; ftell( fpArea ) < iChar; iLine++ )
	    {
		while ( getc( fpArea ) != '\n' )
		    ;
	    }
	    fseek( fpArea, iChar, 0 );
	}

	snprintf(buf, sizeof(buf), "[*****] FILE: %s LINE: %d", strArea, iLine );
	logger->log_string( buf );
/* RT removed because we don't want bugs shutting the mud 
	if ( ( fp = fopen( "shutdown.txt", "a" ) ) != NULL )
	{
	    fprintf( fp, "[*****] %s\n", buf );
	    fclose( fp );
	}
*/
    }

    strcpy( buf, "[*****] BUG: " );
    snprintf(buf + strlen(buf), sizeof(buf) + strlen(buf), str, param );
    logger->log_string( buf );
/* RT removed due to bug-file spamming 
    fclose( fpReserve );
    if ( ( fp = fopen( BUG_FILE, "a" ) ) != NULL )
    {
	fprintf( fp, "%s\n", buf );
	fclose( fp );
    }
    fpReserve = fopen( NULL_FILE, "r" );
*/

    return;
}



/*
 * This function is here to aid in debugging.
 * If the last expression in a function is another function call,
 *   gcc likes to generate a JMP instead of a CALL.
 * This is called "tail chaining."
 * It hoses the debugger call stack for that call.
 * So I make this the last call in certain critical functions,
 *   where I really need the call stack to be right for debugging!
 *
 * If you don't understand this, then LEAVE IT ALONE.
 * Don't remove any calls to tail_chain anywhere.
 *
 * -- Furey
 */
void tail_chain( void )
{
    return;
}

/*
 * Read a string from file fp using str_dup (ie: no string hashing)
 */
char *fread_string_nohash( FILE *fp )
{
    char buf[MAX_STRING_LENGTH];
    char *plast;
    char c;
    int ln;

    plast = buf;
    buf[0] = '\0';
    ln = 0;

    /*
     * Skip blanks.
     * Read first char.
     */
    do
    {
        if ( feof(fp) )
        {
            bug("fread_string_no_hash: EOF encountered on read.", 0);
            if ( fBootDb )
                exit(1);
            return str_dup("");
        }
        c = getc( fp );
    }
    while ( isspace(c) );

    if ( ( *plast++ = c ) == '~' )
        return str_dup( "" );

    for ( ;; )
    {
        if ( ln >= (MAX_STRING_LENGTH - 1) )
        {
           bug( "fread_string_no_hash: string too long", 0 );
           *plast = '\0';
           return str_dup( buf );
        }
        switch ( *plast = getc( fp ) )
        {
        default:
            plast++; ln++;
            break;

        case EOF:
            bug( "Fread_string_no_hash: EOF", 0 );
            if ( fBootDb )
              exit( 1 );
            *plast = '\0';
            return str_dup(buf);
            break;

        case '\n':
            plast++;  ln++;
            *plast++ = '\r';  ln++;
            break;

        case '\r':
            break;

        case '~':
            *plast = '\0';
            return str_dup( buf );
        }
    }
}

