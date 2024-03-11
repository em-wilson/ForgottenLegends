#include "AbstractStateHandler.h"

class GetNewSexStateHandler : public AbstractStateHandler {
    public:
        GetNewSexStateHandler();
        void handle(DESCRIPTOR_DATA *_descriptor, char *argument);
};