#include <sys/time.h>
#include "telnet.h"
#include "BanManager.h"
#include "clans/ClanManager.h"
#include "ConnectedState.h"
#include "Descriptor.h"
#include "AbstractStateHandler.h"
#include "GetNameStateHandler.h"
#include "ILogger.h"
#include "NonPlayerCharacter.h" // for MOB_INDEX_DATA
#include "SocketHelper.h"

extern bool newlock;
extern bool wizlock;

extern ILogger *logger;

#ifndef MAX_STRING_LENGTH
#define MAX_STRING_LENGTH	 4608
#endif

#ifndef MAX_KEY_HASH
#define	MAX_KEY_HASH		 1024
#endif

#ifndef LOWER
#define LOWER(c)		((c) >= 'A' && (c) <= 'Z' ? (c)+'a'-'A' : (c))
#endif

#ifndef UPPER
#define UPPER(c)		((c) >= 'a' && (c) <= 'z' ? (c)+'A'-'a' : (c))
#endif


const unsigned char echo_off_str[] = {IAC, WILL, TELOPT_ECHO, '\0'};

bool is_name ( const char *str, const char *namelist );
char *capitalize( const char *str );
bool load_char_obj( DESCRIPTOR_DATA *d, char *name );
bool str_cmp( const char *astr, const char *bstr );
bool str_prefix( const char *astr, const char *bstr );
bool str_suffix( const char *astr, const char *bstr );

GetNameStateHandler::GetNameStateHandler(BanManager *ban_manager, ClanManager *clan_manager) : AbstractStateHandler(ConnectedState::GetName) {
	this->ban_manager = ban_manager;
    this->clan_manager = clan_manager;
}

GetNameStateHandler::~GetNameStateHandler() {
	this->ban_manager = nullptr;
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
		return false;

	if (clan_manager->get_clan(name))
		return false;

	if (str_cmp(capitalize(name), "Alander") && (!str_prefix("Alan", name) || !str_suffix("Alander", name)))
		return false;

	/*
	 * Length restrictions.
	 */

	if (strlen(name) < 2)
		return false;

	if (strlen(name) > 12)
		return false;

	/*
	 * Alphanumerics only.
	 * Lock out IllIll twits.
	 */
	{
		char *pc;
		bool fIll, adjcaps = false, cleancaps = false;
		unsigned int total_caps = 0;

		fIll = true;
		for (pc = name; *pc != '\0'; pc++)
		{
			if (!isalpha(*pc))
				return false;

			if (isupper(*pc)) /* ugly anti-caps hack */
			{
				if (adjcaps)
					cleancaps = true;
				total_caps++;
				adjcaps = true;
			}
			else
				adjcaps = false;

			if (LOWER(*pc) != 'i' && LOWER(*pc) != 'l')
				fIll = false;
		}

		if (fIll)
			return false;

		if (cleancaps || (total_caps > (strlen(name)) / 2 && strlen(name) < 3))
			return false;
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
					return false;
			}
		}
	}

	return true;
}

void GetNameStateHandler::handle(DESCRIPTOR_DATA *d, char *argument) {
	char buf[MAX_STRING_LENGTH];
	char log_buf[2*MAX_INPUT_LENGTH];

	Character *ch = d->character;

    if (argument[0] == '\0')
		{
			SocketHelper::close_socket(d);
			return;
		}

		argument[0] = UPPER(argument[0]);
		if (!check_parse_name(argument))
		{
			SocketHelper::write_to_buffer(d, "Illegal name, try another.\n\rName: ", 0);
			return;
		}

		bool fOld = load_char_obj(d, argument);
		ch = d->character;

		if (ch->isDenied())
		{
			snprintf(log_buf, 2 * MAX_INPUT_LENGTH, "Denying access to %s@%s.", argument, d->host);
			logger->log_string(log_buf);
			SocketHelper::write_to_buffer(d, "You are denied access.\n\r", 0);
			SocketHelper::close_socket(d);
			return;
		}

		if (ban_manager->isSiteBanned(d))
		{
			SocketHelper::write_to_buffer(d, "Your site has been banned from this mud.\n\r", 0);
			SocketHelper::close_socket(d);
			return;
		}

		if (SocketHelper::check_reconnect(d, false))
		{
			fOld = true;
		}
		else
		{
			if (wizlock && !ch->isImmortal())
			{
				SocketHelper::write_to_buffer(d, "The game is wizlocked.\n\r", 0);
				SocketHelper::close_socket(d);
				return;
			}
		}

		if (fOld)
		{
			/* Old player */
			SocketHelper::write_to_buffer(d, "Password: ", 0);
			SocketHelper::write_to_buffer(d, (const char *)echo_off_str, 0);
			d->connected = ConnectedState::GetOldPassword;
			return;
		}
		else
		{
			/* New player */
			if (newlock)
			{
				SocketHelper::write_to_buffer(d, "The game is newlocked.\n\r", 0);
				SocketHelper::close_socket(d);
				return;
			}

			if (!ban_manager->areNewPlayersAllowed(d))
			{
				SocketHelper::write_to_buffer(d,
								"New players are not allowed from your site.\n\r", 0);
				SocketHelper::close_socket(d);
				return;
			}

			snprintf(buf, sizeof(buf), "Did I get that right, %s (Y/N)? ", argument);
			SocketHelper::write_to_buffer(d, buf, 0);
			d->connected = ConnectedState::ConfirmNewName;
			return;
		}
}