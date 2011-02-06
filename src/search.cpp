#if defined(macintosh)
#include <types.h>
#else
#include <sys/types.h>
#endif
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "merc.h"
#include "tables.h"

char *flag_string               args ( ( const struct flag_type *flag_table,
                                         int bits ) );
void save_object_stats args( ( FILE *fp, OBJ_INDEX_DATA *pObjIndex ) );
void save_object_stats_html args( ( FILE *fp, OBJ_INDEX_DATA *pObjIndex ) );
void html_colour( char * in, char * out );
void save_area_objects args( ( AREA_DATA *pArea ) );
char *fix_string args( ( const char *str ) );
void save_item_level args( ( const char *title, int type, long int wear ) );

void do_wdump( Character *ch, char *argument )
{
    FILE *fp;
    ROOM_INDEX_DATA *room;
    char buf[MAX_STRING_LENGTH];
    EXIT_DATA *pexit;
    int vnum, door;

    log_string("Starting dump of world:");

    for (vnum = 0; vnum < 32000; vnum++)
    {
	if ((room = get_room_index(vnum)) == NULL)
	    continue;

    sprintf(buf, "/home/blackavar/public_html/rooms/r%d.html", vnum );

    fclose( fpReserve );
    if ( ( fp = fopen( buf, "w" ) ) == NULL )
    {
        bug( "Open_html: fopen", 0 );
        perror( buf );
    }

    fprintf(fp, "<html>\n");
    fprintf(fp, "<head>\n");
    fprintf(fp, "<meta http-equiv=""Content-Type"" content=""text/html; charset=windows-1252"">\n");
    fprintf(fp, "<meta name=""GENERATOR"" content=""Forgotten Legends Eq Generator"">\n");
    html_colour(room->name,buf);
    fprintf(fp, "<title>%s</title>\n", buf);
    fprintf(fp, "</head>\n");
    fprintf(fp, "<body stylesrc=""main.htm"" bgcolor=""#000000"" text=""#C0C0C0"">\n");

    fprintf(fp, "<H2>%s</H2>\n<p>", buf);
    html_colour(room->description,buf);
    fprintf(fp, "%s", buf);
    fprintf(fp, "</p>" );

    fprintf(fp, "        <table border=\"1\" width=\"100%%\">\n" );
    fprintf(fp, "          <tr>" );
    fprintf(fp, "            <td>Rooms</td>" );
    fprintf(fp, "            <td>Objects</td>" );
    fprintf(fp, "            <td>Mobiles</td>" );
    fprintf(fp, "          </tr>" );
    fprintf(fp, "          <tr>" );
    fprintf(fp, "	     <td>" );

    for ( door = 0; door <= 5; door++ )
    {
        if ( ( pexit = room->exit[door] ) != NULL
        &&   pexit->u1.to_room != NULL )
        {
            fprintf( fp, "          <a href=\"r%d.html\">%s</a><br>\n", 
		    pexit->u1.to_room->vnum,
                    capitalize( dir_name[door] ) );
        }
    }

fprintf(fp, "</td>\n                <td>&nbsp;</td>\
                <td>&nbsp;</td>\
            </tr>\
        </table>" );
    fprintf(fp, "</body>\n");
    fprintf(fp, "</html>\n");
    fclose( fp );
    fpReserve = fopen( NULL_FILE, "r" );
    }
    log_string("World dump completed.");
    send_to_char("World dump completed.\n\r",ch);
}

void save_item_level( const char *title, int type, long int wear )
{
    char buf[MAX_STRING_LENGTH];
    char fixer[MAX_STRING_LENGTH];
    FILE *fp;
    int i, level;
    OBJ_INDEX_DATA *pObj;

    sprintf(fixer, "%s", title);
    fixer[0] = LOWER(fixer[0]);
    sprintf(buf, "/home/blackavar/public_html/%s.html", fixer );

    fclose( fpReserve );
    if ( !( fp = fopen( buf, "w" ) ) )
    {
        bug( "Open_area: fopen", 0 );
        perror( buf );
    }

    fprintf(fp, "<html>\n\r");
    fprintf(fp, "<head>\n\r");
    fprintf(fp, "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=windows-1252\">\n\r");
    fprintf(fp, "<meta name=\"GENERATOR\" content=\"Forgotten Legends Eq Generator\">\n\r");
    fprintf(fp, "<title>%s</title>\n\r", title);
    fprintf(fp, "</head>\n\r");
    fprintf(fp, "\n\r");
    fprintf(fp, "<body stylesrc=\"main.htm\" bgcolor=\"#000000\" text=\"#C0C0C0\">\n\r");
    fprintf(fp, "\n\r");
    fprintf(fp, "<h1><font color=\"#ff0000\"><u>%s</u></font></h1>\n\r", title);
    fprintf(fp, "\n\r");
 
    fprintf(fp, " <table border=\"1\">");

    /* we start at level 0, move up to level 60, and for each level check
       all 32k objects to see if they match level.  If they do, they pass
       into the next phase of the html printout... this is extremely
       SLOW!!!!

       -Blizzard
    */
    for ( level = 0; level < MAX_LEVEL; level++)
    {
	for ( i = 0; i < 32000; i++ )
	{
		/* Item must exist, must be proper level, and must not have a noshow flag */
	    if ((pObj = get_obj_index( i )) == NULL || pObj->level != level
			|| IS_SET(pObj->extra_flags, ITEM_NOSHOW))
		continue;

            if (pObj->item_type == ITEM_WEAPON && pObj->value[0] == wear && pObj->item_type == type
		&& str_cmp(pObj->name, "super weapon"))
	    {
		save_object_stats_html( fp, pObj );
		continue;
	    }
	    else if (pObj->item_type == ITEM_BOW && pObj->item_type == type)
	    {
		save_object_stats_html( fp, pObj );
		continue;
	    }
	    else if (((pObj->item_type == ITEM_GEM)
			|| (pObj->item_type == ITEM_JEWELRY)
			|| (pObj->item_type == ITEM_CONTAINER)
			|| (pObj->item_type == ITEM_TREASURE)
			|| (pObj->item_type == ITEM_ARMOR))
		&& type != ITEM_WEAPON
		&& IS_SET(pObj->wear_flags, wear))
	    {
		save_object_stats_html( fp, pObj );
		continue;
	    }
	    else continue;
	}
    }

    fprintf(fp, "</table>\n");
    fprintf(fp, "\n\r");
    fprintf(fp, "</body>\n\r");
    fprintf(fp, "\n\r");
    fprintf(fp, "</html>\n\r");
    fclose(fp);
    fpReserve = fopen( NULL_FILE, "r" );
    return;
}

void do_odump( Character *ch, char *argument )
{
    /* Save html files */
    save_item_level( "Exotic", ITEM_WEAPON, WEAPON_EXOTIC );    
    save_item_level( "Swords", ITEM_WEAPON, WEAPON_SWORD );
    save_item_level( "Daggers", ITEM_WEAPON, WEAPON_DAGGER );
    save_item_level( "Spears", ITEM_WEAPON, WEAPON_SPEAR );    
    save_item_level( "Maces", ITEM_WEAPON, WEAPON_MACE );    
    save_item_level( "Axes", ITEM_WEAPON, WEAPON_AXE );    
    save_item_level( "Flails", ITEM_WEAPON, WEAPON_FLAIL );    
    save_item_level( "Whips", ITEM_WEAPON, WEAPON_WHIP );    
    save_item_level( "Polearms", ITEM_WEAPON, WEAPON_POLEARM );    

    save_item_level( "Bow",	ITEM_BOW, 0 );
    save_item_level( "Finger", ITEM_ARMOR, ITEM_WEAR_FINGER );
    save_item_level( "Neck", ITEM_ARMOR, ITEM_WEAR_NECK );
    save_item_level( "Back", ITEM_ARMOR, ITEM_WEAR_BACK );
    save_item_level( "Body", ITEM_ARMOR, ITEM_WEAR_BODY );
    save_item_level( "Head", ITEM_ARMOR, ITEM_WEAR_HEAD );
    save_item_level( "Legs", ITEM_ARMOR, ITEM_WEAR_LEGS );
    save_item_level( "Feet", ITEM_ARMOR, ITEM_WEAR_FEET );
    save_item_level( "Hands", ITEM_ARMOR, ITEM_WEAR_HANDS );
    save_item_level( "Arms", ITEM_ARMOR, ITEM_WEAR_ARMS );
    save_item_level( "Shield", ITEM_ARMOR, ITEM_WEAR_SHIELD );
    save_item_level( "About", ITEM_ARMOR, ITEM_WEAR_ABOUT );
    save_item_level( "Waist", ITEM_ARMOR, ITEM_WEAR_WAIST );
    save_item_level( "Wrist", ITEM_ARMOR, ITEM_WEAR_WRIST );
    send_to_char("All objects in area have been saved.\n\r", ch);
    return;
}

void save_object_stats_html( FILE *fp, OBJ_INDEX_DATA *pObjIndex )
{
    int cnt;
    AFFECT_DATA *paf;
    char buf[MAX_STRING_LENGTH];
    
    fprintf( fp, "<tr>\n" );
    fprintf( fp, "  <td>%d</td>\n", pObjIndex->level );
    fprintf( fp, "  <td>%s</td>\n", pObjIndex->name );
    fprintf( fp, "  <td>%s</td>\n", pObjIndex->area->name );
 
/*
 *  Using fwrite_flag to write most values gives a strange
 *  looking area file, consider making a case for each
 *  item type later.
 */ 
    

    switch ( pObjIndex->item_type )
    {
        default:
	     sprintf( buf, "%d/%d/%d/%d", 
				pObjIndex->value[0],
				pObjIndex->value[1],
				pObjIndex->value[2],
				pObjIndex->value[3] );

	    if (!str_cmp(buf, "0/0/0/0" ))
		sprintf(buf, " ");

	    fprintf( fp, "<td>%s ", buf );
            break;

        case ITEM_CONTAINER:
            fprintf( fp, "<td>Capacity: %d Weight Mult: %d",
                     pObjIndex->value[3],
                     pObjIndex->value[4]);
            break;

        case ITEM_WEAPON:
            sprintf(buf," %s",weapon_bit_name(pObjIndex->value[4]));

            fprintf( fp,
		"<td>%dd%d(%d)%s",
                pObjIndex->value[1],pObjIndex->value[2],
                (1 + pObjIndex->value[2]) * pObjIndex->value[1] / 2,
		pObjIndex->value[4] ? buf : "");
            break;

        case ITEM_BOW:
            fprintf( fp,
		"<td>[Range: %d] %dd%d(%d)",
		pObjIndex->value[0], pObjIndex->value[1],pObjIndex->value[2],
                (1 + pObjIndex->value[2]) * pObjIndex->value[1] / 2);
            break;

        case ITEM_PILL:
        case ITEM_POTION:
        case ITEM_SCROLL:
            fprintf( fp, "<td>Level: %d Spell: '%s' Spell: '%s' Spell: '%s' Spell: '%s' ",
                     pObjIndex->value[0] > 0 ? /* no negative numbers */
                     pObjIndex->value[0]
                     : 0,
                     pObjIndex->value[1] != -1 ?
                     skill_table[pObjIndex->value[1]].name
                     : "none",
                     pObjIndex->value[2] != -1 ?
                     skill_table[pObjIndex->value[2]].name
                     : "none",
                     pObjIndex->value[3] != -1 ?
                     skill_table[pObjIndex->value[3]].name
                     : "none",
                     pObjIndex->value[4] != -1 ?
                     skill_table[pObjIndex->value[4]].name
                     : "none");
            break;

        case ITEM_STAFF:
        case ITEM_WAND:
            fprintf( fp, "<td>Level: %d Total Charges: %d Charges Left: %d Spell: '%s'</td>\n",
                                pObjIndex->value[0],
                                pObjIndex->value[1],
                                pObjIndex->value[2],
                                pObjIndex->value[3] != -1 ? skill_table[pObjIndex->value[3]].name :
                                        "none" );
            break;
    }

    for ( cnt = 0, paf = pObjIndex->affected; paf; paf = paf->next )
    {
        sprintf( buf, " %s%-8d%s ", paf->modifier > 0 ? "+" : "",
            paf->modifier, flag_string( apply_flags, paf->location ) );
        fprintf( fp, "%s", buf );
        cnt++;
    }

    sprintf(buf, " (%s) ", extra_bit_name( pObjIndex->extra_flags ) );
    if (str_cmp(buf, " (none) ") )
	fprintf(fp, "%s", buf);

    fprintf( fp, "  </td>\n</tr>\n" );

    return;
}

void who_html_update (void)
{
  FILE *fp; 
  DESCRIPTOR_DATA *d;
//  char buf[MAX_STRING_LENGTH];
//  char buf2[MAX_STRING_LENGTH];
  char *buf2;
  char s[MAX_STRING_LENGTH];
  int count = 0;

  fclose(fpReserve);
  if ( (fp = fopen("/home/blackavar/public_html/online.htm", "w") ) == NULL)
  {
     bug( "online.htm: fopen", 0 );
     perror( "online.htm" );
  }
  else
  {
  char buf[MAX_STRING_LENGTH];
  fprintf(fp, "<html>\n");
  fprintf(fp, "<head>\n");
  fprintf(fp, "<title>\n");
  fprintf(fp, "Players currently on Forgotten Legends\n");
  fprintf(fp, "</title>\n");
  fprintf(fp, "<body stylesrc=""main.htm"" bgcolor=""#000000"" text=""#C0C0C0"">\n");
  fprintf(fp, "<h1><font color=""#FF2222"">\n");
  fprintf(fp, "Who's on Forgotten Legends?\n");  
  fprintf(fp, "</font></h1>\n");
  
  fprintf(fp, "<pre>\n");
  fprintf(fp, "[<font color=""#FF0000"">Lev  <font color=""#808000"">Race <font color=""#FFFFFF"">Cla<FONT COLOR=""#C0C0C0"">] Name\n");
  fprintf(fp, "<font color=""#0000FF"">_________________________________________________________________\n");
  
  log_string("First part ok");
  for ( d = descriptor_list; d != NULL ; d = d->next )
  {
    Character *wch;
    char const *class_name;
    char *race;
    
    if ( d->connected != CON_PLAYING)
        continue;

        wch   = ( d->original != NULL ) ? d->original : d->character;

    if ( wch->invis_level > 1 || wch->incog_level > 1)
	continue;

	class_name = class_table[wch->class_num].who_name;
	switch ( wch->level )
	{
	default: break;
            {
                case MAX_LEVEL - 0 : class_name = "IMP";     break;
                case MAX_LEVEL - 1 : class_name = "CRE";     break;
                case MAX_LEVEL - 2 : class_name = "SUP";     break;
                case MAX_LEVEL - 3 : class_name = "DEI";     break;
                case MAX_LEVEL - 4 : class_name = "GOD";     break;
                case MAX_LEVEL - 5 : class_name = "IMM";     break;
                case MAX_LEVEL - 6 : class_name = "DEM";     break;
                case MAX_LEVEL - 7 : class_name = "ANG";     break;
                case MAX_LEVEL - 8 : class_name = "AVA";     break;
            }
	}

	    race = str_dup(pc_race_table[wch->race].who_name);
	    if (wch->race == race_lookup("werefolk"))
	    {
		if (IS_SET(wch->act, PLR_IS_MORPHED))
		    race = str_dup(morph_table[wch->morph_form].who_name);
		else
		    race = str_dup(pc_race_table[wch->orig_form].who_name);
	    }

	count++;

	fprintf(fp, "<FONT COLOR=""#C0C0C0"">[<FONT_COLOR=""#FF0000"">");
	fprintf(fp, "%2d ", wch->level );
	fprintf(fp, "<FONT COLOR=""#808000"">%6s ", race );
	fprintf(fp, "<FONT COLOR=""#FFFFFF"">%s<FONT COLOR=""#C0C0C0"">] ", class_name );
	fprintf(fp, "%s", wch->getName());
	buf[0] = '\0';
	buf2 = str_dup(wch->pcdata->title);
//      sprintf(buf2, "%s", wch->pcdata->title);

      html_colour(buf2,buf);
      fprintf(fp, "%s", buf);
     	fprintf(fp, "\n");

  }    /*end for */
  
  fprintf(fp, "\n<FONT COLOR=""#C0C0C0"">Players found: %d.\n", count);
  fprintf(fp, "\n");
 
  fprintf(fp, "</font>\n");
  fprintf(fp, "</pre><br>\n");
  fprintf(fp, "<p>\n"); 
  fprintf(fp, "<br>\n");
  fprintf(fp, "<font color=""#FFFFFF"" face=""Times New Roman"">\n");
  strftime( s, 100, "%I:%M%p", localtime( &current_time ) );
  sprintf(buf, "This file last updated at %s Central Time.\n", s);
  fprintf(fp, "%s", buf);
  fprintf(fp, "<br>\n");
  fprintf(fp, "If this is more than a few moments old, the mud is likely to be down.\n");
  fprintf(fp, "<br>\n");
  fprintf(fp, "<a href=""main.htm"">Back to main page.</a>\n");
  fprintf(fp, "<br>\n");
  fprintf(fp, "<br>\n");
  fprintf(fp, "<hr>\n");

  fprintf(fp, "</p>\n");
  fprintf(fp, "</body>\n");
  fprintf(fp, "</html>\n");
	fclose( fp ); 
   	fpReserve = fopen( NULL_FILE, "r" );
    } /*end if */ 
  
  return;
}/* end function */

void get_who_data (char buf[MAX_INPUT_LENGTH], char arg[MAX_INPUT_LENGTH], int len)
{
  char buf2[22];
  char buf3[1];
  
  int i;
  
  
for (i=0; i < MAX_INPUT_LENGTH; ++i)
{
   buf[i]= 'G';
}

for (i=0; i < 22; ++i)
{
  buf2[i] = 'G';
}

buf[0] = '\0';
buf2[0] = '\0';

for (i=0; i <= len; ++i) 
{
  
  switch (arg[i])
  {
    default: buf3[0] = arg[i]; strcat(buf, buf3); break;
      {
        case '{':
          buf2[0] = '\0';
          i = i + 1;
       switch (arg[i])
       {
         
           {
           case 'm':           
             strcat(buf2,"<font color=""#8B008B"">");
             break;
           case 'M':
             strcat(buf2, "<font color=""#FF00FF"">");
             break;
           case 'r':
             strcat(buf2, "<font color=""#8B0000"">");
             break;
           case 'R':
             strcat(buf2, "<font color=""#FF0000"">");
             break;
           case 'g':
             strcat(buf2, "<font color=""#006400"">");
             break;
           case 'G':
             strcat(buf2, "<font color=""#00FF00"">");
             break;
           case 'c':
             strcat(buf2, "<font color=""#008B8B"">");
             break;
           case 'C':
             strcat(buf2, "<font color=""#00FFFF"">");
             break;
           case 'y':
             strcat(buf2, "<font color=""#808000"">");
             break;
           case 'Y':
             strcat(buf2, "<font color=""#FFFF00"">");
             break;
           case 'w':
             strcat(buf2, "<font color=""#808080"">");
             break;
           case 'W':
             strcat(buf2, "<font color=""#FFFFFF"">");
             break;
           case 'D':
             strcat(buf2, "<font color=""#636363"">");
             break;
           case 'b':
             strcat(buf2, "<font color=""#00008B"">");
             break;
           case 'B':
             strcat(buf2, "<font color=""#0000FF"">");
             break;
           case '{':
             strcat(buf2, "{");
             break;
           case 'x':
	     strcat(buf2, "<font color=""#C0C0C0"">");
//             strcat(buf2 ,"<font color=""#006400"">");
             break;
          }
          default: strcat(buf2, "");break;
        }      
        strcat(buf, buf2);
        break;
      case '\0':
          buf3[0] = arg[i];
          strcat(buf, buf3);
          break;
      } 
     }
}
 
 return;
 }

void html_colour( char * in, char *out)
{
    bool continue1 = FALSE;
    int i;

    for (i = 0; in[i] != '\0'; i++)
    {
	char buf2[MAX_STRING_LENGTH];

	if (in[i] == '{')
	{
	    continue1 = TRUE;
	    continue;
	}
	else if (continue1 == TRUE)
	{
	    continue1 = FALSE;

       switch (in[i])
       {
           {
           case 'm':           
             strcat(out,"<font color=""#8B008B"">");
             break;
           case 'M':
             strcat(out, "<font color=""#FF00FF"">");
             break;
           case 'r':
             strcat(out, "<font color=""#8B0000"">");
             break;
           case 'R':
             strcat(out, "<font color=""#FF0000"">");
             break;
           case 'g':
             strcat(out, "<font color=""#006400"">");
             break;
           case 'G':
             strcat(out, "<font color=""#00FF00"">");
             break;
           case 'c':
             strcat(out, "<font color=""#008B8B"">");
             break;
           case 'C':
             strcat(out, "<font color=""#00FFFF"">");
             break;
           case 'y':
             strcat(out, "<font color=""#808000"">");
             break;
           case 'Y':
             strcat(out, "<font color=""#FFFF00"">");
             break;
           case 'w':
             strcat(out, "<font color=""#808080"">");
             break;
           case 'W':
             strcat(out, "<font color=""#FFFFFF"">");
             break;
           case 'D':
             strcat(out, "<font color=""#636363"">");
             break;
           case 'b':
             strcat(out, "<font color=""#00008B"">");
             break;
           case 'B':
             strcat(out, "<font color=""#0000FF"">");
             break;
           case '{':
             strcat(out, "{");
             break;
           case 'x':
	     strcat(out, "<font color=""#C0C0C0"">");
             break;
          }
          default: strcat(out, "");break;
        	}  //{end case}
	} //{end else if}
	else
	{
	    sprintf(buf2, "%c", in[i]);
	    strcat(out, buf2);
	}
    }
}
