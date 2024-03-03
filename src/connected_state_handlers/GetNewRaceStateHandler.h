#ifndef __GETNEWRACESTATEHANDLER_H__
#define __GETNEWRACESTATEHANDLER_H__

#include "AbstractStateHandler.h"

class RaceManager;

class GetNewRaceStateHandler : public AbstractStateHandler {
    public:
        GetNewRaceStateHandler(RaceManager *race_manager);
        virtual ~GetNewRaceStateHandler();
        void handle(DESCRIPTOR_DATA *_descriptor, char *argument);

    private:
        RaceManager *race_manager;
};

#endif