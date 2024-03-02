#ifndef __ICONNECTED_STATE_HANDLER_H__
#define __ICONNECTED_STATE_HANDLER_H__

#include "ConnectedState.h"

class Character;
typedef struct descriptor_data DESCRIPTOR_DATA;

class IConnectedStateHandler {
    public:
        virtual void handle(DESCRIPTOR_DATA *_descriptor, char *argument) = 0;
        virtual bool canHandleState(ConnectedState state) = 0;
};

#endif