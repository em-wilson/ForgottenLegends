#ifndef __BREAKCONNECTSTATEHANDLER_H__
#define __BREAKCONNECTSTATEHANDLER_H__

#include "AbstractStateHandler.h"

class BreakConnectStateHandler : public AbstractStateHandler {
    public:
        BreakConnectStateHandler();
        virtual ~BreakConnectStateHandler();
        void handle(DESCRIPTOR_DATA *d, char *argument);
};

#endif