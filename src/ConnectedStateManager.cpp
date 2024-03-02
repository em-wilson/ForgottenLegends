#include "ConnectedState.h"
#include "ConnectedStateManager.h"
#include "connected_state_handlers/GetNameStateHandler.h"
#include "Descriptor.h"

ConnectedStateManager::ConnectedStateManager(Game *game) {
    _game = game;
}

ConnectedStateManager::~ConnectedStateManager() {
    _game = nullptr;
}

IConnectedStateHandler * ConnectedStateManager::createHandler(DESCRIPTOR_DATA *d) {
    auto state = ConnectedState(d->connected);
    for ( auto handler : _handlers ) {
        if ( handler->canHandleState(state)) {
            return handler;
        }
    }
    return nullptr;
}

void ConnectedStateManager::addHandler(IConnectedStateHandler *handler) {
    _handlers.push_back(handler);
}