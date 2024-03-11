#ifndef __READMOTDSTATEHANDLER_H__
#define __READMOTDSTATEHANDLER_H__

#include "AbstractStateHandler.h"

class ReadMotdStateHandler : public AbstractStateHandler {
    public:
        ReadMotdStateHandler();
        virtual ~ReadMotdStateHandler();
        void handle(DESCRIPTOR_DATA *_descriptor, char *argument);
};

#endif