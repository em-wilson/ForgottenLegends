#ifndef __ABSTRACTSTATEHANDLER_H__
#define __ABSTRACTSTATEHANDLER_H__

#include "IConnectedStateHandler.h"

class AbstractStateHandler : public IConnectedStateHandler {
    public:
        AbstractStateHandler(ConnectedState state);
        bool canHandleState(ConnectedState state);

    private:
        ConnectedState _state;
};

#endif