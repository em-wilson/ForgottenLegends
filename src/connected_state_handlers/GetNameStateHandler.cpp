#include <sys/time.h>
#include "../merc.h"
#include "../telnet.h"
#include "../ConnectedState.h"
#include "AbstractStateHandler.h"
#include "GetNameStateHandler.h"

extern bool newlock;
extern bool wizlock;

const unsigned char echo_off_str[] = {IAC, WILL, TELOPT_ECHO, '\0'};


GetNameStateHandler::GetNameStateHandler(ClanManager *clan_manager) : AbstractStateHandler(ConnectedState::GetName) {
    this->clan_manager = clan_manager;
}

GetNameStateHandler::~GetNameStateHandler() {
    this->clan_manager = nullptr;
}

/*
 * Parse a name for acceptability.
 */
bool GetNameStateHandler::check_parse_name(char *name)
{
	/*
	 * Reserved words.
	 */
	if (is_name(name,
				(char *)"all auto immortal self someone something the you demise balance circle loner honor none"))
		return FALSE;

	if (clan_manager->get_clan(name))
		return FALSE;

	if (str_cmp(capitalize(name), "Alander") && (!str_prefix("Alan", name) || !str_suffix("Alander", name)))
		return FALSE;

	/*
	 * Length restrictions.
	 */

	if (strlen(name) < 2)
		return FALSE;

	if (strlen(name) > 12)
		return FALSE;

	/*
	 * Alphanumerics only.
	 * Lock out IllIll twits.
	 */
	{
		char *pc;
		bool fIll, adjcaps = FALSE, cleancaps = FALSE;
		unsigned int total_caps = 0;

		fIll = TRUE;
		for (pc = name; *pc != '\0'; pc++)
		{
			if (!isalpha(*pc))
				return FALSE;

			if (isupper(*pc)) /* ugly anti-caps hack */
			{
				if (adjcaps)
					cleancaps = TRUE;
				total_caps++;
				adjcaps = TRUE;
			}
			else
				adjcaps = FALSE;

			if (LOWER(*pc) != 'i' && LOWER(*pc) != 'l')
				fIll = FALSE;
		}

		if (fIll)
			return FALSE;

		if (cleancaps || (total_caps > (strlen(name)) / 2 && strlen(name) < 3))
			return FALSE;
	}

	/*
	 * Prevent players from naming themselves after mobs.
	 */
	{
		extern MOB_INDEX_DATA *mob_index_hash[MAX_KEY_HASH];
		MOB_INDEX_DATA *pMobIndex;
		int iHash;

		for (iHash = 0; iHash < MAX_KEY_HASH; iHash++)
		{
			for (pMobIndex = mob_index_hash[iHash];
				 pMobIndex != NULL;
				 pMobIndex = pMobIndex->next)
			{
				if (is_name(name, pMobIndex->player_name))
					return FALSE;
			}
		}
	}

	return TRUE;
}

void GetNameStateHandler::handle(DESCRIPTOR_DATA *d, char *argument) {
	char buf[MAX_STRING_LENGTH];

	Character *ch = d->character;

    if (argument[0] == '\0')
		{
			close_socket(d);
			return;
		}

		argument[0] = UPPER(argument[0]);
		if (!check_parse_name(argument))
		{
			write_to_buffer(d, "Illegal name, try another.\n\rName: ", 0);
			return;
		}

		bool fOld = load_char_obj(d, argument);
		ch = d->character;

		if (IS_SET(ch->act, PLR_DENY))
		{
			snprintf(log_buf, 2 * MAX_INPUT_LENGTH, "Denying access to %s@%s.", argument, d->host);
			log_string(log_buf);
			write_to_buffer(d, "You are denied access.\n\r", 0);
			close_socket(d);
			return;
		}

		if (check_ban(d->host, BAN_PERMIT) && !IS_SET(ch->act, PLR_PERMIT))
		{
			write_to_buffer(d, "Your site has been banned from this mud.\n\r", 0);
			close_socket(d);
			return;
		}

		if (check_reconnect(d, argument, FALSE))
		{
			fOld = TRUE;
		}
		else
		{
			if (wizlock && !IS_IMMORTAL(ch))
			{
				write_to_buffer(d, "The game is wizlocked.\n\r", 0);
				close_socket(d);
				return;
			}
		}

		if (fOld)
		{
			/* Old player */
			write_to_buffer(d, "Password: ", 0);
			write_to_buffer(d, (const char *)echo_off_str, 0);
			d->connected = CON_GET_OLD_PASSWORD;
			return;
		}
		else
		{
			/* New player */
			if (newlock)
			{
				write_to_buffer(d, "The game is newlocked.\n\r", 0);
				close_socket(d);
				return;
			}

			if (check_ban(d->host, BAN_NEWBIES))
			{
				write_to_buffer(d,
								"New players are not allowed from your site.\n\r", 0);
				close_socket(d);
				return;
			}

			snprintf(buf, sizeof(buf), "Did I get that right, %s (Y/N)? ", argument);
			write_to_buffer(d, buf, 0);
			d->connected = CON_CONFIRM_NEW_NAME;
			return;
		}
}