#include <sys/time.h>
#include "merc.h"
#include "BanManager.h"

bool BanManager::isSiteBanned(DESCRIPTOR_DATA *d) {
    Character *ch = d->character;
    return check_ban(d->host, BAN_PERMIT) && !IS_SET(ch->act, PLR_PERMIT);
}

bool BanManager::areNewPlayersAllowed(DESCRIPTOR_DATA *d) {
    if (check_ban(d->host, BAN_NEWBIES)) {
        return false;
    }

    return true;
}