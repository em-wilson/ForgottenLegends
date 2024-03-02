#ifndef __ACTIONTARGET_H__
#define __ACTIONTARGET_H__

enum ActionTarget {
    ToRoom                = 0,
    ToNotVictim           = 1,
    ToVictim              = 2,
    ToCharacter           = 3,
    ToAll                 = 4,
    ToArea                = 5,
    ToAreaOutsideRoom     = 6
};

#endif