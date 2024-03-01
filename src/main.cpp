#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include "merc.h"

/* Needs to be global because of do_copyover */
int port, control;

char                str_boot_time[MAX_INPUT_LENGTH];
void    game_loop_unix          args( ( int control ) );
int     init_socket             args( ( int port ) );




//int     main                    args( ( int argc, char **argv ) );
int main( int argc, char **argv )
{
    struct timeval now_time;
    bool fCopyOver = FALSE;

    /*
     * Memory debugging if needed.
     */
#if defined(MALLOC_DEBUG)
    malloc_debug( 2 );
#endif

    /*
     * Init time.
     */
    gettimeofday( &now_time, NULL );
    current_time        = (time_t) now_time.tv_sec;
    strcpy( str_boot_time, ctime( &current_time ) );

    /*
     * Reserve one channel for our use.
     */
    if ( ( fpReserve = fopen( NULL_FILE, "r" ) ) == NULL )
    {
        perror( NULL_FILE );
        exit( 1 );
    }

    /*
     * Get the port number.
     */
    port = 4000;
    if ( argc > 1 )
    {
        if ( !is_number( argv[1] ) )
        {
            fprintf( stderr, "Usage: %s [port #]\n", argv[0] );
            exit( 1 );
        }
        else if ( ( port = atoi( argv[1] ) ) <= 1024 )
        {
            fprintf( stderr, "Port number must be above 1024.\n" );
            exit( 1 );
        }

       /* Are we recovering from a copyover? */
       if (argv[2] && argv[2][0])
       {
               fCopyOver = TRUE;
               control = atoi(argv[3]);
       }
       else
               fCopyOver = FALSE;
    }

    /*
     * Run the game.
     */
    if (!fCopyOver)
         control = init_socket( port );

    boot_db();
    snprintf(log_buf, 2*MAX_INPUT_LENGTH, "ROM is ready to rock on port %d.", port );
    log_string( log_buf );
    if (fCopyOver)
       copyover_recover();
    game_loop_unix( control );
    close (control);

    /*
     * That's all, folks.
     */
    log_string( "Normal termination of game." );
    exit( 0 );
    return 0;
}

