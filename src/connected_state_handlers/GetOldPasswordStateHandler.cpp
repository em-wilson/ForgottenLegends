#include <sys/time.h>
#include <unistd.h> // crypt
#include "merc.h"
#include "telnet.h"
#include "Wiznet.h"
#include "GetOldPasswordStateHandler.h"
#include "ConnectedState.h"

extern DESCRIPTOR_DATA *descriptor_list;
const unsigned char echo_on_str[] = {IAC, WONT, TELOPT_ECHO, '\0'};
DECLARE_DO_FUN(do_help);

GetOldPasswordStateHandler::GetOldPasswordStateHandler() : AbstractStateHandler(ConnectedState::GetOldPassword) {

}

/*
 * Check if already playing.
 */
bool GetOldPasswordStateHandler::check_playing(DESCRIPTOR_DATA *d, char *name)
{
	DESCRIPTOR_DATA *dold;

	for (dold = descriptor_list; dold; dold = dold->next)
	{
		if (dold != d && dold->character != NULL && dold->connected != CON_GET_NAME && dold->connected != CON_GET_OLD_PASSWORD && !str_cmp(name, dold->original ? dold->original->getName() : dold->character->getName()))
		{
			write_to_buffer(d, "That character is already playing.\n\r", 0);
			write_to_buffer(d, "Do you wish to connect anyway (Y/N)?", 0);
			d->connected = CON_BREAK_CONNECT;
			return TRUE;
		}
	}

	return FALSE;
}

void GetOldPasswordStateHandler::handle(DESCRIPTOR_DATA *d, char *argument) {
    Character *ch = d->character;

    write_to_buffer(d, "\n\r", 2);

    if (strcmp(crypt(argument, ch->pcdata->pwd), ch->pcdata->pwd))
    {
        write_to_buffer(d, "Wrong password.\n\r", 0);
        close_socket(d);
        return;
    }

    write_to_buffer(d, (const char *)echo_on_str, 0);

    if (check_playing(d, ch->getName()))
        return;

    if (check_reconnect(d, ch->getName(), TRUE))
        return;

    snprintf(log_buf, 2 * MAX_INPUT_LENGTH, "%s@%s has connected.", ch->getName(), d->host);
    log_string(log_buf);
    Wiznet::instance()->report(log_buf, NULL, NULL, WIZ_SITES, 0, get_trust(ch));

    if (IS_IMMORTAL(ch))
    {
        do_help(ch, (char *)"imotd");
        d->connected = CON_READ_IMOTD;
    }
    else
    {
        do_help(ch, (char *)"motd");
        d->connected = CON_READ_MOTD;
    }
}