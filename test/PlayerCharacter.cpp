#include <catch2/catch_test_macros.hpp>
#include "merc.h"
#include "PlayerCharacter.h"

SCENARIO( "Player creation" ) {
    GIVEN( "A newly created character should be level 0" ) {
        PlayerCharacter target = PlayerCharacter();

        // Creating at level 0 triggers the new player assistance after MOTD state in comm.c
        REQUIRE( target.level == 0 );
    }
}