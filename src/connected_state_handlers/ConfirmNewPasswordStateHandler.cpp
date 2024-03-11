#include <string>
#include <unistd.h>
#include "Character.h"
#include "ConfirmNewPasswordStateHandler.h"
#include "ConnectedState.h"
#include "Descriptor.h"
#include "PlayerCharacter.h"
#include "Race.h"
#include "RaceManager.h"
#include "SocketHelper.h"
#include "telnet.h"

using std::string;

const unsigned char echo_on_str[] = {IAC, WONT, TELOPT_ECHO, '\0'};

ConfirmNewPasswordStateHandler::ConfirmNewPasswordStateHandler(RaceManager *raceManager) : AbstractStateHandler(ConnectedState::ConfirmNewPassword) { _raceManager = raceManager; }

ConfirmNewPasswordStateHandler::~ConfirmNewPasswordStateHandler() { }


void ConfirmNewPasswordStateHandler::handle(DESCRIPTOR_DATA *d, char *argument) {
    PlayerCharacter *ch = (PlayerCharacter*) d->character;
    SocketHelper::write_to_buffer(d, "\n\r", 2);

    if (string(crypt(argument, ch->getPassword().c_str())) != ch->getPassword())
    {
        SocketHelper::write_to_buffer(d, "Passwords don't match.\n\rRetype password: ",
                        0);
        d->connected = ConnectedState::GetNewPassword;
        return;
    }

    SocketHelper::write_to_buffer(d, (const char *)echo_on_str, 0);
    SocketHelper::write_to_buffer(d, "The following races are available:\n\r  ", 0);
    for (auto race : _raceManager->getAllRaces() ) {
        if (race->isPlayerRace()) {
            SocketHelper::write_to_buffer(d, race->getName().c_str(), 0);
            SocketHelper::write_to_buffer(d, " ", 1);
        }
    }
    SocketHelper::write_to_buffer(d, "\n\r", 0);
    SocketHelper::write_to_buffer(d, "What is your race (help for more information)? ", 0);
    d->connected = ConnectedState::GetNewRace;
}