#include "catch.hpp"
#include "merc.h"
#include "PlayerCharacter.h"

SCENARIO( "Determining whether someone may be a certain class" ) {
    GIVEN( "A default character" ) {
        auto target = new PlayerCharacter();
        REQUIRE( can_be_class(target, class_lookup("mage")) == true );
        REQUIRE( can_be_class(target, class_lookup("cleric")) == true );
        REQUIRE( can_be_class(target, class_lookup("warrior")) == true );
        REQUIRE( can_be_class(target, class_lookup("thief")) == true );
        REQUIRE( can_be_class(target, class_lookup("ranger")) == false );
        REQUIRE( can_be_class(target, class_lookup("druid")) == false );
        REQUIRE( can_be_class(target, class_lookup("paladin")) == false );
        REQUIRE( can_be_class(target, class_lookup("invoker")) == false );
        REQUIRE( can_be_class(target, class_lookup("psioniscist")) == false );
        REQUIRE( can_be_class(target, class_lookup("rogue")) == false );
        REQUIRE( can_be_class(target, class_lookup("illusionist")) == false );
    }
    GIVEN( "A previous ranger" ) {
        auto target = new PlayerCharacter();
        SET_BIT(target->done, DONE_RANGER);
        REQUIRE( can_be_class(target, class_lookup("mage")) == true );
        REQUIRE( can_be_class(target, class_lookup("cleric")) == true );
        REQUIRE( can_be_class(target, class_lookup("warrior")) == true );
        REQUIRE( can_be_class(target, class_lookup("thief")) == true );
        REQUIRE( can_be_class(target, class_lookup("ranger")) == false );
        REQUIRE( can_be_class(target, class_lookup("druid")) == false );
        REQUIRE( can_be_class(target, class_lookup("paladin")) == false );
        REQUIRE( can_be_class(target, class_lookup("invoker")) == false );
        REQUIRE( can_be_class(target, class_lookup("psioniscist")) == false );
        REQUIRE( can_be_class(target, class_lookup("rogue")) == false );
        REQUIRE( can_be_class(target, class_lookup("illusionist")) == false );
    }
    GIVEN( "A previous mage" ) {
        auto target = new PlayerCharacter();
        SET_BIT(target->done, DONE_MAGE);
        long druidRequirements = class_table[class_lookup("druid")].requirements;
        REQUIRE( can_be_class(target, class_lookup("mage")) == true );
        REQUIRE( can_be_class(target, class_lookup("cleric")) == true );
        REQUIRE( can_be_class(target, class_lookup("warrior")) == true );
        REQUIRE( can_be_class(target, class_lookup("thief")) == true );
        REQUIRE( can_be_class(target, class_lookup("ranger")) == false );
        REQUIRE( can_be_class(target, class_lookup("druid")) == false );
        REQUIRE( can_be_class(target, class_lookup("paladin")) == false );
        REQUIRE( can_be_class(target, class_lookup("invoker")) == false );
        REQUIRE( can_be_class(target, class_lookup("psioniscist")) == false );
        REQUIRE( can_be_class(target, class_lookup("rogue")) == false );
        REQUIRE( can_be_class(target, class_lookup("illusionist")) == false );
    }
    // This combo can become a druid
    GIVEN( "A previous ranger/mage combo" ) {
        auto target = new PlayerCharacter();
        SET_BIT(target->done, DONE_RANGER);
        SET_BIT(target->done, DONE_MAGE);
        REQUIRE( can_be_class(target, class_lookup("mage")) == true );
        REQUIRE( can_be_class(target, class_lookup("cleric")) == true );
        REQUIRE( can_be_class(target, class_lookup("warrior")) == true );
        REQUIRE( can_be_class(target, class_lookup("thief")) == true );
        REQUIRE( can_be_class(target, class_lookup("ranger")) == false );
        REQUIRE( can_be_class(target, class_lookup("druid")) == true );
        REQUIRE( can_be_class(target, class_lookup("paladin")) == false );
        REQUIRE( can_be_class(target, class_lookup("invoker")) == false );
        REQUIRE( can_be_class(target, class_lookup("psioniscist")) == false );
        REQUIRE( can_be_class(target, class_lookup("rogue")) == false );
        REQUIRE( can_be_class(target, class_lookup("illusionist")) == false );
    }
    // This combo can become a ranger
    GIVEN( "A previous warrior/archer combo" ) {
        auto target = new PlayerCharacter();
        SET_BIT(target->done, DONE_WARRIOR);
        SET_BIT(target->done, DONE_ARCHER);
        INFO("The target->done bit is " << target->done);
        REQUIRE( can_be_class(target, class_lookup("mage")) == true );
        REQUIRE( can_be_class(target, class_lookup("cleric")) == true );
        REQUIRE( can_be_class(target, class_lookup("warrior")) == true );
        REQUIRE( can_be_class(target, class_lookup("thief")) == true );
        REQUIRE( can_be_class(target, class_lookup("ranger")) == true );
        REQUIRE( can_be_class(target, class_lookup("druid")) == false );
        REQUIRE( can_be_class(target, class_lookup("paladin")) == false );
        REQUIRE( can_be_class(target, class_lookup("invoker")) == false );
        REQUIRE( can_be_class(target, class_lookup("psioniscist")) == false );
        REQUIRE( can_be_class(target, class_lookup("rogue")) == false );
        REQUIRE( can_be_class(target, class_lookup("illusionist")) == false );
    }
}
