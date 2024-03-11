#include <string.h>
#include "ConnectedState.h"
#include "Descriptor.h"
#include "GetNewRaceStateHandler.h"
#include "PlayerCharacter.h"
#include "PlayerRace.h"
#include "Race.h"
#include "RaceManager.h"
#include "SocketHelper.h"

#define MAX_INPUT_LENGTH	  256

extern	const	struct	morph_race_type	morph_table	[];

void do_help( Character *ch, char *argument );
void group_add( Character *ch, const char *name, bool deduct);
char *one_argument( char *argument, char *arg_first );
int race_lookup (const char *name);

GetNewRaceStateHandler::GetNewRaceStateHandler(RaceManager *race_manager) : AbstractStateHandler(ConnectedState::GetNewRace) {
    this->race_manager = race_manager;
}

GetNewRaceStateHandler::~GetNewRaceStateHandler() {
    this->race_manager = nullptr;
}

void GetNewRaceStateHandler::handle(DESCRIPTOR_DATA *d, char *argument) {
	char arg[MAX_INPUT_LENGTH];

    one_argument(argument, arg);

    PlayerCharacter *ch = (PlayerCharacter*) d->character;

    if (!strcmp(arg, "help"))
    {
        argument = one_argument(argument, arg);
        if (argument[0] == '\0')
            do_help(ch, (char *)"race help");
        else
            do_help(ch, argument);
        SocketHelper::write_to_buffer(d, "What is your race (help for more information)? ", 0);
        return;
    }

    Race * race;
    
    try {
        race = race_manager->getRaceByName(argument);
        if (!race->isPlayerRace()) {
            throw InvalidRaceException(argument);
        }
    } catch (InvalidRaceException) {
        SocketHelper::write_to_buffer(d, "That is not a valid race->\n\r", 0);
        SocketHelper::write_to_buffer(d, "The following races are available:\n\r  ", 0);
        for (auto race : race_manager->getAllRaces()) {
            if (race->isPlayerRace()) {
                SocketHelper::write_to_buffer(d, race->getName().c_str(), 0);
                SocketHelper::write_to_buffer(d, " ", 1);
            }
        }
        SocketHelper::write_to_buffer(d, "\n\r", 0);
        SocketHelper::write_to_buffer(d, "What is your race? (help for more information) ", 0);
        return;
    }

    ch->setRace(race);
    /* initialize stats */
    for (int i = 0; i < MAX_STATS; i++)
        ch->perm_stat[i] = race->getPlayerRace()->getStats().at(i);
    ch->affected_by = ch->affected_by | race->getAffectFlags();
    ch->imm_flags = ch->imm_flags | race->getImmunityFlags();
    ch->res_flags = ch->res_flags | race->getResistanceFlags();
    ch->vuln_flags = ch->vuln_flags | race->getVulnerabilityFlags();
    ch->form = race->getForm();
    ch->parts = race->getParts();

    /* add skills */
    for (auto skill : race->getPlayerRace()->getSkills()) {
        group_add(ch, skill.c_str(), false);
    }
    /* add cost */
    ch->points = race->getPlayerRace()->getPoints();
    ch->size = race->getPlayerRace()->getSize();

    // Werefolk not present currently
    // if (race_lookup(race_table[race->getLegacyId()].name) == race_lookup("werefolk"))
    // {
    //     int morph;
    //     SocketHelper::write_to_buffer(d, "As a werefolk, you may morph into the following forms:\n\r ", 0);
    //     for (morph = 0; morph_table[morph].name[0] != '\0'; morph++)
    //     {
    //         SocketHelper::write_to_buffer(d, morph_table[morph].name, 0);
    //         SocketHelper::write_to_buffer(d, " ", 1);
    //     }
    //     SocketHelper::write_to_buffer(d, "\n\r", 0);
    //     SocketHelper::write_to_buffer(d, "What is your morphing race (help for more information)? ", 0);
    //     d->connected = ConnectedState::GetMorph;
    //     return;
    // }

    SocketHelper::write_to_buffer(d, "What is your sex (M/F)? ", 0);
    d->connected = ConnectedState::GetNewSex;
}