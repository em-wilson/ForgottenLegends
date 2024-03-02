#include <sys/time.h>
#include <unistd.h> // crypt
#include "GetOldPasswordStateHandler.h"
#include "Character.h"
#include "ConnectedState.h"
#include "Descriptor.h"
#include "telnet.h"
#include "Wiznet.h"

extern DESCRIPTOR_DATA *descriptor_list;
const unsigned char echo_on_str[] = {IAC, WONT, TELOPT_ECHO, '\0'};
#define args( list )			list
#define DECLARE_DO_FUN( fun )		DO_FUN    fun
typedef	void DO_FUN	args( ( Character *ch, char *argument ) );

DECLARE_DO_FUN(do_help);

bool str_cmp( const char *astr, const char *bstr );

void close_socket(DESCRIPTOR_DATA *dclose);
void log_string( const char *str );
void write_to_buffer(DESCRIPTOR_DATA *d, const char *txt, int length);

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
		if (dold != d && dold->character != NULL && dold->connected != ConnectedState::GetName && dold->connected != ConnectedState::GetOldPassword && !str_cmp(name, dold->original ? dold->original->getName() : dold->character->getName()))
		{
			write_to_buffer(d, "That character is already playing.\n\r", 0);
			write_to_buffer(d, "Do you wish to connect anyway (Y/N)?", 0);
			d->connected = ConnectedState::BreakConnect;
			return true;
		}
	}

	return false;
}

void GetOldPasswordStateHandler::handle(DESCRIPTOR_DATA *d, char *argument) {
	char log_buf[2*MAX_INPUT_LENGTH];
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

    if (check_reconnect(d, ch->getName(), true))
        return;

    snprintf(log_buf, 2 * MAX_INPUT_LENGTH, "%s@%s has connected.", ch->getName(), d->host);
    log_string(log_buf);
    Wiznet::instance()->report(log_buf, NULL, NULL, WIZ_SITES, 0, ch->getTrust());

    if (ch->isImmortal())
    {
        do_help(ch, (char *)"imotd");
        d->connected = ConnectedState::ReadImotd;
    }
    else
    {
        do_help(ch, (char *)"motd");
        d->connected = ConnectedState::ReadMotd;
    }
}