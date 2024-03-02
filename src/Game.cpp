#include "Game.h"

extern ClanManager * clan_manager;

ClanManager * Game::getClanManager() {
    return clan_manager;
}

ConnectedStateManager * Game::getConnectedStateManager() {
    return _connectedStateManager;
}

void Game::setConnectedStateManager(ConnectedStateManager *manager) {
    _connectedStateManager = manager;
}
