#include "AbstractStateHandler.h"

class GetOldPasswordStateHandler : public AbstractStateHandler {
    public:
        GetOldPasswordStateHandler();
        void handle(DESCRIPTOR_DATA *_descriptor, char *argument);

    private:
        bool check_playing(DESCRIPTOR_DATA *d, std::string name);
};