#include "AbstractStateHandler.h"

AbstractStateHandler::AbstractStateHandler(ConnectedState state) {
    _state = state;
}

bool AbstractStateHandler::canHandleState(ConnectedState state) {
    return state == _state;
}