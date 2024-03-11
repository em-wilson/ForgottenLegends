#include "IConnectedStateHandler.h"

class ILogger;

class GetReclassStateHandler : public IConnectedStateHandler {
    public:
        GetReclassStateHandler(ILogger *logger);
        virtual ~GetReclassStateHandler();
        void handle(DESCRIPTOR_DATA *_descriptor, char *argument) override;
        virtual bool canHandleState(ConnectedState state) override;

    private:
        void handleReclassSelection(DESCRIPTOR_DATA *_descriptor, char *argument);
        void handleReclassCustomization(DESCRIPTOR_DATA *_descriptor, char *argument);
        ILogger * _logger;
};