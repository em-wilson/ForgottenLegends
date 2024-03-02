#ifndef __ABSTRACTSTATEHANDLER_H__
#define __ABSTRACTSTATEHANDLER_H__

#include "IConnectedStateHandler.h"

class AbstractStateHandler : public IConnectedStateHandler {
    public:
        AbstractStateHandler(ConnectedState state);
        bool canHandleState(ConnectedState state);

    protected:
        bool check_reconnect(DESCRIPTOR_DATA *d, char *name, bool fConn);

    private:
        ConnectedState _state;
};

#endif