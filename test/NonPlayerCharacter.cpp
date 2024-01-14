#include <catch2/catch_test_macros.hpp>
#include "merc.h"
#include "NonPlayerCharacter.h"

SCENARIO( "Generic NPC stuff" ) {
    GIVEN( "A default character" ) {
	NonPlayerCharacter target = NonPlayerCharacter();

	REQUIRE( target.isNPC() == true );
    }
}
