#ifndef __GAME_H__
#define __GAME_H__

#include "clans/ClanManager.h"
#include "ConnectedStateManager.h"

class Game {
    public:
        ClanManager * getClanManager();
        ConnectedStateManager * getConnectedStateManager();
        void setConnectedStateManager(ConnectedStateManager *manager);
    
    private:
        ConnectedStateManager *_connectedStateManager;
};

#endif