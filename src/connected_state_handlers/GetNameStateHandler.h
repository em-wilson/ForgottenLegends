#ifndef __GETNAMESTATEHANDLER_H__
#define __GETNAMESTATEHANDLER_H__

#include "AbstractStateHandler.h"

class BanManager;
class ClanManager;

class GetNameStateHandler : public AbstractStateHandler {
    public:
        GetNameStateHandler(BanManager *ban_manager, ClanManager *clan_manager);
        ~GetNameStateHandler();
        void handle(DESCRIPTOR_DATA *_descriptor, char *argument);

    private:
        bool check_parse_name(char *name);
        BanManager *ban_manager;
        ClanManager *clan_manager;
};

#endif