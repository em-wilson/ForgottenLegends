#ifndef __CONNECTED_STATE_MANAGER_H__
#define __CONNECTED_STATE_MANAGER_H__

#include <list>

using std::list;
class Game;
class IConnectedStateHandler;
typedef struct descriptor_data DESCRIPTOR_DATA;

class ConnectedStateManager {
    public:
        ConnectedStateManager(Game *game);
        ~ConnectedStateManager();
        IConnectedStateHandler * createHandler(DESCRIPTOR_DATA *d);
        void addHandler(IConnectedStateHandler *handler);
    private:
        Game * _game;
        list<IConnectedStateHandler *> _handlers;
};

#endif