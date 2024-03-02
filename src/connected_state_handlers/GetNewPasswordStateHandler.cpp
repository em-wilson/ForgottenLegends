#include <string.h>
#include <unistd.h>
#include "Character.h"
#include "ConnectedState.h"
#include "Descriptor.h"
#include "GetNewPasswordStateHandler.h"

void free_string( char *pstr );
char *str_dup( const char *str );
void write_to_buffer(DESCRIPTOR_DATA *d, const char *txt, int length);

GetNewPasswordStateHandler::GetNewPasswordStateHandler() : AbstractStateHandler(ConnectedState::GetNewPassword) { }

GetNewPasswordStateHandler::~GetNewPasswordStateHandler() { }

void GetNewPasswordStateHandler::handle(DESCRIPTOR_DATA *d, char *argument) {
    Character *ch = d->character;
    write_to_buffer(d, "\n\r", 2);

    if (strlen(argument) < 5)
    {
        write_to_buffer(d,
                        "Password must be at least five characters long.\n\rPassword: ",
                        0);
        return;
    }

	char *pwdnew = crypt(argument, ch->getName());
    for (char *p = pwdnew; *p != '\0'; p++)
    {
        if (*p == '~')
        {
            write_to_buffer(d,
                            "New password not acceptable, try again.\n\rPassword: ",
                            0);
            return;
        }
    }

    free_string(ch->pcdata->pwd);
    ch->pcdata->pwd = str_dup(pwdnew);
    write_to_buffer(d, "Please retype password: ", 0);
    d->connected = ConnectedState::ConfirmNewPassword;
}