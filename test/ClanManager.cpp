#include <catch2/catch_test_macros.hpp>
#include "merc.h"
#include "ClanManager.h"

SCENARIO( "Clan Management" ) {
    GIVEN( "A clan exists" ) {
        ClanManager *target = new ClanManager();
        CREATE( first_clan, CLAN_DATA, 1 );
        first_clan->name = str_dup("bar");

        WHEN("Looking for a non-existing clan") {
            char * argument = str_dup("foo");
            CLAN_DATA *result = target->get_clan(argument);

            THEN("The result should be NULL") {
                REQUIRE( result == NULL );
            }

            free_string(argument);
        }

        WHEN("Looking for an existing clan") {
            char * argument = str_dup("bar");
            CLAN_DATA *result = target->get_clan(argument);

            THEN("The result should not be NULL") {
                REQUIRE( result != NULL );
            }

            free_string(argument);
        }

        // Clean up
        free_string( first_clan->name );
        DISPOSE( first_clan );
        first_clan = NULL;
    }
}
