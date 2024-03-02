#ifndef __GETNAMESTATEHANDLER_H__
#define __GETNAMESTATEHANDLER_H__

#include "../clans/ClanManager.h"
#include "AbstractStateHandler.h"

class GetNameStateHandler : public AbstractStateHandler {
    public:
        GetNameStateHandler(ClanManager *clan_manager);
        ~GetNameStateHandler();
        void handle(DESCRIPTOR_DATA *_descriptor, char *argument);

    private:
        bool check_parse_name(char *name);
        ClanManager *clan_manager;
};

#endif