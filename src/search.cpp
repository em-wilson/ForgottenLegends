#include <time.h>
#include "merc.h"
#include "ConnectedState.h"
#include "PlayerRace.h"
#include "RaceManager.h"
#include "tables.h"

char *flag_string               args ( ( const struct flag_type *flag_table,
                                         int bits ) );
void html_colour( char * in, char * out );
char *fix_string args( ( const char *str ) );

extern RaceManager * race_manager;

void who_html_update (void)
{
  FILE *fp; 
  DESCRIPTOR_DATA *d;
  char *buf2;
  char s[MAX_STRING_LENGTH];
  int count = 0;

  fclose(fpReserve);
  if ( (fp = fopen(ONLINE_FILE, "w") ) == NULL)
  {
      perror( ONLINE_FILE );
      exit( 1 );
  } else {
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
  
  for ( d = descriptor_list; d != NULL ; d = d->next )
  {
    Character *wch;
    char const *class_name;
    string race;
    
    if ( d->connected != ConnectedState::Playing)
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

	    race = wch->getRace()->getPlayerRace()->getWhoName();
	    if (wch->getRace() == race_manager->getRaceByName("werefolk"))
	    {
		if (IS_SET(wch->act, PLR_IS_MORPHED))
		    race = str_dup(morph_table[wch->morph_form].who_name);
		else
		    race = race_manager->getRaceByLegacyId(wch->orig_form)->getPlayerRace()->getWhoName();
	    }

	count++;

	fprintf(fp, "<FONT COLOR=""#C0C0C0"">[<FONT_COLOR=""#FF0000"">");
	fprintf(fp, "%2d ", wch->level );
	fprintf(fp, "<FONT COLOR=""#808000"">%6s ", race.c_str() );
	fprintf(fp, "<FONT COLOR=""#FFFFFF"">%s<FONT COLOR=""#C0C0C0"">] ", class_name );
	fprintf(fp, "%s", wch->getName().c_str());
	buf[0] = '\0';
	buf2 = str_dup(wch->pcdata->title);
//      snprintf(buf2, sizeof(buf2), "%s", wch->pcdata->title);

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
  snprintf(buf, sizeof(buf), "This file last updated at %s Central Time.\n", s);
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
	    snprintf(buf2, sizeof(buf2), "%c", in[i]);
	    strcat(out, buf2);
	}
    }
}
