#include "AbstractStateHandler.h"

class RaceManager;

class ConfirmNewPasswordStateHandler : public AbstractStateHandler {
    public:
        ConfirmNewPasswordStateHandler(RaceManager *raceManager);
        virtual ~ConfirmNewPasswordStateHandler();
        void handle(DESCRIPTOR_DATA *_descriptor, char *argument);

    private:
        RaceManager *_raceManager;
};