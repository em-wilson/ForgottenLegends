#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <limits.h>
#include <time.h>
#include "merc.h"

void    fread_stock      args( ( STOCK_DATA *stock, FILE *fp ) );
bool    load_stock_file  args( ( char *stockfile ) );
void    write_stock_list args( ( void ) );

STOCK_DATA * first_stock;
STOCK_DATA * last_stock;

/*
 * Load in all the stock files.
 */
void load_stocks( )
{
    FILE *fpList;
    char *filename;
    char stocklist[256];
    char buf[MAX_STRING_LENGTH];


    first_stock  = NULL;
    last_stock   = NULL;

    log_string( "Loading stocks..." );

    sprintf( stocklist, "%s%s%s", DATA_DIR, STOCK_DIR, STOCK_LIST );
    fclose( fpReserve );
    if ( ( fpList = fopen( stocklist, "r" ) ) == NULL )
    {
        perror( stocklist );
        exit( 1 );
    }

    for ( ; ; )
    {
        filename = feof( fpList ) ? "$" : fread_word( fpList );
        log_string( filename );
        if ( filename[0] == '$' )
          break;

        if ( !load_stock_file( filename ) )
        {
          sprintf( buf, "Cannot load stock file: %s", filename );
          bug( buf, 0 );
        }
    }
    fclose( fpList );
    log_string(" Done stocks. " );
    fpReserve = fopen( NULL_FILE, "r" );
    return;
}

/*
 * Load a stock file
 */
bool load_stock_file( char *stockfile )
{
    char filename[256];
    STOCK_DATA *stock;
    FILE *fp;
    bool found = FALSE;

    CREATE( stock, STOCK_DATA, 1 );

    sprintf( filename, "%s%s%s", DATA_DIR, STOCK_DIR, stockfile );

    if ( ( fp = fopen( filename, "r" ) ) != NULL )
    {
        found = TRUE;
        for ( ; ; )
        {
            char letter;
            char *word;

            letter = fread_letter( fp );
            if ( letter == '*' )
            {
                fread_to_eol( fp );
                continue;
            }

            if ( letter != '#' )
            {
                bug( "Load_stock_file: # not found.", 0 );
                break;
            }

            word = fread_word( fp );
            if ( !str_cmp( word, "STOCK" ) )
            {
                fread_stock( stock, fp );
                break;
            }
            else
            if ( !str_cmp( word, "END"  ) )
                break;
            else
            {
                char buf[MAX_STRING_LENGTH];

                sprintf( buf, "Load_stock_file: bad section: %s.", word );
                bug( buf, 0 );
                break;
            }
        }
        fclose( fp );
    }

    if ( found )
        LINK( stock, first_stock, last_stock, next, prev );
    else
      DISPOSE( stock );

    return found;
}

/*
 * Get pointer to stock structure from stock name.
 */
STOCK_DATA *get_stock( char *name )
{
    STOCK_DATA *stock;

    for ( stock = first_stock; stock; stock = stock->next )
       if ( !str_cmp( name, stock->name ) )
         return stock;

    return NULL;
}

void write_stock_list( )
{
    STOCK_DATA *tstock;
    FILE *fpout;
    char filename[256];

    sprintf( filename, "%s%s%s", DATA_DIR, STOCK_DIR, STOCK_LIST );
    fpout = fopen( filename, "w" );
    if ( !fpout )
    {
        bug( "FATAL: cannot open stock.lst for writing!\n\r", 0 );
        return;
    }
    for ( tstock = first_stock; tstock; tstock = tstock->next )
        fprintf( fpout, "%s\n", tstock->filename );
    fprintf( fpout, "$\n" );
    fclose( fpout );
}

/*
 * Save a stock's data to its data file
 */
void save_stock( STOCK_DATA *stock )
{
    FILE *fp;
    char filename[256];
    char buf[MAX_STRING_LENGTH];

    if ( !stock )
    {
        bug( "save_stock: null stock pointer!", 0 );
        return;
    }

    if ( !stock->filename || stock->filename[0] == '\0' )
    {
        sprintf( buf, "save_stock: %s has no filename", stock->filename );
        bug( buf, 0 );
        return;
    }

    sprintf( filename, "%s%s%s", DATA_DIR, STOCK_DIR, stock->filename );

    fclose( fpReserve );
    if ( ( fp = fopen( filename, "w" ) ) == NULL )
    {
        bug( "save_stock: fopen", 0 );
        perror( filename );
    }
    else
    {
        fprintf( fp, "#STOCK\n" );
        fprintf( fp, "Name         %s~\n",      stock->name             );
        fprintf( fp, "Filename     %s~\n",      stock->filename         );
        fprintf( fp, "Description  %s~\n",      stock->description      );
        fprintf( fp, "Max          %d\n",       stock->max              );
        fprintf( fp, "Owners       %d\n",       stock->owners           );
	fprintf( fp, "Cost         %d\n",	stock->cost		);
	fprintf( fp, "Free         %d\n",	stock->free		);
	fprintf( fp, "Min_Change   %d\n",	stock->min_change	);
	fprintf( fp, "Max_Change   %d\n",	stock->max_change	);
	fprintf( fp, "Strength     %d\n",	stock->strength		);
        fprintf( fp, "#END\n"                                           );
    }
    fclose( fp );
    fpReserve = fopen( NULL_FILE, "r" );
    return;
}

void do_stock( CHAR_DATA *ch, char *argument )
{
    STOCK_DATA *stock;
    char arg1[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];

    argument = one_argument( argument, arg1 );

    if (!str_prefix( arg1, "list" ))
    {
	sprintf(buf, "%-20s %-20s %-20s\n\r", 	"Stock Name", "Stock Cost",
						"Stocks Free" );
	send_to_char(buf,ch);

	for (stock = first_stock; stock; stock = stock->next )
	{
	    sprintf(buf, "%-20s %-20d %-20d\n\r",	stock->name, stock->cost,
							stock->free );
	    send_to_char(buf,ch);
	}
    }

    if (!str_prefix(arg1, "buy"))
    {
	if (!(stock = get_stock(argument)))
	{
	    send_to_char("That stock does not exist.\n\r",ch);
	    return;
	}

	if (stock->free < 1)
	{
	    send_to_char("That stock is sold out at this time, sorry.\n\r",ch);
	    return;
	}

	if (ch->gold < stock->cost)
	{
	    send_to_char("You cannot afford to buy this stock right now.\n\r",ch);
	    return;
	}
    }

    /*
     * Immortals only -- Create stocks
     */
    if (!str_prefix(arg1, "make") && IS_IMMORTAL(ch))
    {
	char arg2[MAX_INPUT_LENGTH];
	char arg3[MAX_INPUT_LENGTH];

	argument = one_argument(argument, arg2);
	argument = one_argument(argument, arg3);

	if (arg2[0] == '\0' || arg3[0] == '\0' || argument[0] == '\0')
	{
	    send_to_char("Usage: stock make <filename> <cost> <stockname>\n\r",ch);
	    return;
	}

	if (get_stock(argument) != NULL)
	{
	    send_to_char("Stock already exists.\n\r",ch);
	    send_to_char("Use 'stock split' to increase the number.\n\r",ch);
	    return;
	}

	CREATE( stock, STOCK_DATA, 1 );
	LINK( stock, first_stock, last_stock, next, prev );

	stock->name          = str_dup( argument );
	stock->filename      = str_dup( arg2 );
	stock->cost	     = atoi( arg3 );
	stock->max	     = 100;
	stock->free	     = 100;
	stock->strength	     = 0;
	save_stock( stock );
	write_stock_list( );
	send_to_char("Stock created.\n\r",ch);
    }
}
