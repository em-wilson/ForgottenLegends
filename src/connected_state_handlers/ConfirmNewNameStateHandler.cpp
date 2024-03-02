#include <stdio.h>
#include "Character.h"
#include "ConfirmNewNameStateHandler.h"
#include "Descriptor.h"
#include "telnet.h"

#ifndef MAX_STRING_LENGTH
#define MAX_STRING_LENGTH	 4608
#endif

void write_to_buffer(DESCRIPTOR_DATA *d, const char *txt, int length);

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
        snprintf(buf, sizeof(buf), "New character.\n\rGive me a password for %s: %s", ch->getName(), echo_off_str);
        write_to_buffer(d, buf, 0);
        d->connected = ConnectedState::GetNewPassword;
        break;

    case 'n':
    case 'N':
        write_to_buffer(d, "Ok, what IS it, then? ", 0);
        delete d->character;
        d->character = NULL;
        d->connected = ConnectedState::GetName;
        break;

    default:
        write_to_buffer(d, "Please type Yes or No? ", 0);
        break;
    }
}