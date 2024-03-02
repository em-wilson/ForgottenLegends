#ifndef __GETNEWPASSWORDSTATEHANDLER_H__
#define __GETNEWPASSWORDSTATEHANDLER_H__

#include "AbstractStateHandler.h"

class GetNewPasswordStateHandler : public AbstractStateHandler {
    public:
        GetNewPasswordStateHandler();
        virtual ~GetNewPasswordStateHandler();
        void handle(DESCRIPTOR_DATA *d, char *argument);
};

#endif