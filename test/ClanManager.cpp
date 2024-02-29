#include <catch2/catch_test_macros.hpp>
#include "merc.h"
#include "ClanManager.h"

SCENARIO( "Clan Management" ) {
    GIVEN( "A clan exists" ) {
        ClanManager *target = new ClanManager();
        CREATE( first_clan, CLAN_DATA, 1 );
        first_clan->name = str_dup("bar");

        WHEN("Looking for a non-existing clan") {
            CLAN_DATA *result = target->get_clan("foo");

            THEN("The result should be NULL") {
                REQUIRE( result == NULL );
            }
        }

        WHEN("Looking for an existing clan") {
            CLAN_DATA *result = target->get_clan("bar");

            THEN("The result should not be NULL") {
                REQUIRE( result != NULL );
            }
        }

        // Clean up
        free_string( first_clan->name );
        DISPOSE( first_clan );
        first_clan = NULL;
    }
}
