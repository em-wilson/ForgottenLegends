#include <sys/time.h>
#include "merc.h"
#include "telnet.h"
#include "ConfirmNewNameStateHandler.h"

const unsigned char echo_off_str[] = {IAC, WILL, TELOPT_ECHO, '\0'};

ConfirmNewNameStateHandler::ConfirmNewNameStateHandler() : AbstractStateHandler(ConnectedState::ConfirmNewName) {

}

ConfirmNewNameStateHandler::~ConfirmNewNameStateHandler() { }

void ConfirmNewNameStateHandler::handle(DESCRIPTOR_DATA *d, char *argument) {
    char buf[MAX_STRING_LENGTH];
    Character *ch = d->character;

    switch (*argument)
    {
    case 'y':
    case 'Y':
        snprintf(buf, sizeof(buf), "New character.\n\rGive me a password for %s: %s",
                    ch->getName(), echo_off_str);
        write_to_buffer(d, buf, 0);
        d->connected = CON_GET_NEW_PASSWORD;
        break;

    case 'n':
    case 'N':
        write_to_buffer(d, "Ok, what IS it, then? ", 0);
        delete d->character;
        d->character = NULL;
        d->connected = CON_GET_NAME;
        break;

    default:
        write_to_buffer(d, "Please type Yes or No? ", 0);
        break;
    }
}