#include <catch2/catch_test_macros.hpp>
#include "merc.h"
#include "IRaceManager.h"
#include "PlayerCharacter.h"

class TestRaceManager : public IRaceManager {
    public:
        Race * getRaceByName(std::string argument) {
            return nullptr;
        }
};

SCENARIO( "Player creation" ) {
    GIVEN( "A newly created character should be level 0" ) {
        PlayerCharacter target = PlayerCharacter(new TestRaceManager());

        // Creating at level 0 triggers the new player assistance after MOTD state in comm.c
        REQUIRE( target.level == 0 );
    }
}