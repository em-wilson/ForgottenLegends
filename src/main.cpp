#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include "merc.h"
#include "BanManager.h"
#include "FileLogger.h"
#include "Game.h"
#include "RaceManager.h"
#include "RaceReader.h"
#include "RaceWriter.h"
#include "RoomManager.h"

// Clan helpers
#include "clans/ClanReader.h"
#include "clans/ClanWriter.h"

// Connected State Handlers
#include "connected_state_handlers/BreakConnectStateHandler.h"
#include "connected_state_handlers/ConfirmNewNameStateHandler.h"
#include "connected_state_handlers/GetNameStateHandler.h"
#include "connected_state_handlers/GetNewPasswordStateHandler.h"
#include "connected_state_handlers/GetNewRaceStateHandler.h"
#include "connected_state_handlers/GetOldPasswordStateHandler.h"


/* Needs to be global because of do_copyover */
int port, control;

char                str_boot_time[MAX_INPUT_LENGTH];
void    game_loop_unix          args( ( Game *game, int control ) );
int     init_socket             args( ( int port ) );

extern ClanManager * clan_manager;
extern RaceManager * race_manager;
extern RoomManager * room_manager;


//int     main                    args( ( int argc, char **argv ) );
int main( int argc, char **argv )
{
    struct timeval now_time;
    bool fCopyOver = FALSE;

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

    Game game = Game();

    /*
     * Run the game.
     */
    if (!fCopyOver)
         control = init_socket( port );

    BanManager ban_manager = BanManager();
    clan_manager = new ClanManager(new ClanReader(), new ClanWriter(CLAN_DIR, CLAN_LIST));
    race_manager = new RaceManager(new RaceReader(RACE_DIR), new RaceWriter(RACE_DIR));
    room_manager = new RoomManager();
    logger = new FileLogger();

    try {
        race_manager->loadRaces();
    } catch (RaceNotFoundInFileException rfx) {
        logger->log_stringf("Error reading race from: %s", rfx.what());
        return 1;
    }
    
    ConnectedStateManager csm = ConnectedStateManager(&game);
    csm.addHandler(new BreakConnectStateHandler());
    csm.addHandler(new ConfirmNewNameStateHandler());
    csm.addHandler(new GetNameStateHandler(&ban_manager, clan_manager));
    csm.addHandler(new GetOldPasswordStateHandler());
    csm.addHandler(new GetNewPasswordStateHandler());
    csm.addHandler(new GetNewRaceStateHandler(race_manager));
    game.setConnectedStateManager(&csm);

    try {
        boot_db();
    } catch (std::runtime_error rte) {
        logger->log_stringf("Error booting database: %s", rte.what());
        return 1;
    }
    snprintf(log_buf, 2*MAX_INPUT_LENGTH, "ROM is ready to rock on port %d.", port );
    logger->log_string( log_buf );
    if (fCopyOver)
       copyover_recover();
    game_loop_unix( &game, control );
    close (control);

    /*
     * That's all, folks.
     */
    logger->log_string( "Normal termination of game." );

    delete clan_manager;
    delete race_manager;
    clan_manager = nullptr;
    race_manager = nullptr;
    exit( 0 );
    return 0;
}

