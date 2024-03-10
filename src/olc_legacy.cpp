// Maintains some camptabibility with most useful legacy info commands

#include "merc.h"
#include "tables.h"

/*****************************************************************************
 Name:		show_flag_cmds
 Purpose:	Displays settable flags and stats.
 Called by:	show_help(olc_act.c).
 ****************************************************************************/
void show_flag_cmds(Character *ch, const struct flag_type *flag_table)
{
	char buf[MAX_STRING_LENGTH];
	char buf1[MAX_STRING_LENGTH];
	int flag;
	int col;

	buf1[0] = '\0';
	col = 0;
	for (flag = 0; flag_table[flag].name != NULL; flag++)
	{
		if (flag_table[flag].settable)
		{
			snprintf(buf, sizeof(buf), "%-19.18s", flag_table[flag].name);
			strcat(buf1, buf);
			if (++col % 4 == 0)
				strcat(buf1, "\n\r");
		}
	}

	if (col % 4 != 0)
		strcat(buf1, "\n\r");

	send_to_char(buf1, ch);
}