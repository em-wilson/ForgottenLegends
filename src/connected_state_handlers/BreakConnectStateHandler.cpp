#include "BreakConnectStateHandler.h"
#include "Character.h"
#include "ConnectedState.h"
#include "Descriptor.h"

extern DESCRIPTOR_DATA * descriptor_list;
extern std::list<Character *> char_list;
bool str_cmp( const char *astr, const char *bstr );
void close_socket(DESCRIPTOR_DATA *dclose);
void write_to_buffer(DESCRIPTOR_DATA *d, const char *txt, int length);

BreakConnectStateHandler::BreakConnectStateHandler() : AbstractStateHandler(ConnectedState::BreakConnect) {
    
}

BreakConnectStateHandler::~BreakConnectStateHandler() { }

// RT code for breaking link
void BreakConnectStateHandler::handle(DESCRIPTOR_DATA *d, char *argument) {
    Character *ch = d->character;
    switch (*argument)
    {
    case 'y':
    case 'Y':
        DESCRIPTOR_DATA *d_next;
        for (DESCRIPTOR_DATA *d_old = descriptor_list; d_old != NULL; d_old = d_next)
        {
            d_next = d_old->next;
            if (d_old == d || d_old->character == NULL)
                continue;

            if (str_cmp(ch->getName(), d_old->original ? d_old->original->getName() : d_old->character->getName()))
                continue;

            close_socket(d_old);
        }
        if (check_reconnect(d, ch->getName(), true))
            return;
        write_to_buffer(d, "Reconnect attempt failed.\n\rName: ", 0);
        if (d->character != NULL)
        {
            char_list.remove(d->character);
            delete d->character;
            d->character = NULL;
        }
        d->connected = ConnectedState::GetName;
        break;

    case 'n':
    case 'N':
        write_to_buffer(d, "Name: ", 0);
        if (d->character != NULL)
        {
            delete d->character;
            d->character = NULL;
        }
        d->connected = ConnectedState::GetName;
        break;

    default:
        write_to_buffer(d, "Please type Y or N? ", 0);
        break;
    }
}