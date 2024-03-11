#ifndef __EXITFLAG_H__
#define __EXITFLAG_H__

#include "FlagConversions.h"

/*
 * Exit flags.
 * Used in #ROOMS.
 */
enum ExitFlag {
    ExitIsDoor      = A,
    ExitClosed      = B,
    ExitLocked      = C,
    ExitPickProof   = F,
    ExitNoPass      = G,
    ExitEasy        = H,
    ExitHard        = I,
    ExitInfuriating = J,
    ExitNoClose     = K,
    ExitNoLock      = L
};

#endif