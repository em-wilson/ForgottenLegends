#include "FlagHelper.h"

bool FlagHelper::isSet(long flags, long bit) {
    return flags & bit;
}