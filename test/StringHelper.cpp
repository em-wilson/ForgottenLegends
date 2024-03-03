#include <catch2/catch_test_macros.hpp>
#include "StringHelper.h"

SCENARIO( "str_prefix" ) {
    GIVEN( "A string with matching prefix" ) {
        REQUIRE( !StringHelper::str_prefix("hum", "human") == true );
    }

    GIVEN( "A string without a matching prefix" ) {
        REQUIRE( !StringHelper::str_prefix("rom", "human") == false );
    }
}
