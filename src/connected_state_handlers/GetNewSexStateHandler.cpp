#include "merc.h"
#include "GetNewSexStateHandler.h"
#include "PlayerCharacter.h"
#include "SocketHelper.h"

GetNewSexStateHandler::GetNewSexStateHandler() : AbstractStateHandler(ConnectedState::GetNewSex) {}

void GetNewSexStateHandler::handle(DESCRIPTOR_DATA *d, char *argument)
{
	char buf[MAX_STRING_LENGTH];
    PlayerCharacter *ch = (PlayerCharacter*) d->character;
    switch (argument[0])
    {
    case 'm':
    case 'M':
        ch->sex = SEX_MALE;
        ch->true_sex = SEX_MALE;
        break;
    case 'f':
    case 'F':
        ch->sex = SEX_FEMALE;
        ch->true_sex = SEX_FEMALE;
        break;
    default:
        SocketHelper::write_to_buffer(d, "That's not a sex.\n\rWhat IS your sex? ", 0);
        return;
    }

    strcpy(buf, "Select a class [");
    for (auto iClass = 0; iClass < MAX_CLASS; iClass++)
    {
        if (!can_be_class(ch, iClass))
            continue;

        if (iClass > 0)
            strcat(buf, " ");
        strcat(buf, class_table[iClass].name);
    }
    strcat(buf, "]: ");
    SocketHelper::write_to_buffer(d, buf, 0);
    d->connected = ConnectedState::GetNewClass;
}