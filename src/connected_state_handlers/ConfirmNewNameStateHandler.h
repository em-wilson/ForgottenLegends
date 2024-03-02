#include "AbstractStateHandler.h"

class ConfirmNewNameStateHandler : public AbstractStateHandler {
    public:
        ConfirmNewNameStateHandler();
        virtual ~ConfirmNewNameStateHandler();
        void handle(DESCRIPTOR_DATA *_descriptor, char *argument);
};