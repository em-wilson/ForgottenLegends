#include "IConnectedStateHandler.h"

class ILogger;
class IRaceManager;

class GetNewClassStateHandler : public IConnectedStateHandler {
    public:
        GetNewClassStateHandler(ILogger *logger, IRaceManager *race_manager);
        virtual ~GetNewClassStateHandler();
        void handle(DESCRIPTOR_DATA *_descriptor, char *argument) override;
        virtual bool canHandleState(ConnectedState state) override;
    private:
        void handleDefaultChoice(DESCRIPTOR_DATA *_descriptor, char *argument);
        void handleGenGroups(DESCRIPTOR_DATA *_descriptor, char *argument);
        void handleNewClass(DESCRIPTOR_DATA *_descriptor, char *argument);
        void handlePickWeapon(DESCRIPTOR_DATA *_descriptor, char *argument);
        ILogger * _logger;
        IRaceManager *_raceManager;
};
