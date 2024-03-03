#include <string.h>
#include "Character.h"
#include "ConnectedState.h"
#include "Descriptor.h"
#include "GetNewRaceStateHandler.h"
#include "Race.h"
#include "RaceManager.h"

#define MAX_INPUT_LENGTH	  256

extern	const	struct	morph_race_type	morph_table	[];

void do_help( Character *ch, char *argument );
void group_add( Character *ch, const char *name, bool deduct);
char *one_argument( char *argument, char *arg_first );
int race_lookup (const char *name);
void write_to_buffer(DESCRIPTOR_DATA *d, const char *txt, int length);

GetNewRaceStateHandler::GetNewRaceStateHandler(RaceManager *race_manager) : AbstractStateHandler(ConnectedState::GetNewRace) {
    this->race_manager = race_manager;
}

GetNewRaceStateHandler::~GetNewRaceStateHandler() {
    this->race_manager = nullptr;
}

void GetNewRaceStateHandler::handle(DESCRIPTOR_DATA *d, char *argument) {
	char arg[MAX_INPUT_LENGTH];

    one_argument(argument, arg);

    Character *ch = d->character;

    if (!strcmp(arg, "help"))
    {
        argument = one_argument(argument, arg);
        if (argument[0] == '\0')
            do_help(ch, (char *)"race help");
        else
            do_help(ch, argument);
        write_to_buffer(d, "What is your race (help for more information)? ", 0);
        return;
    }

    Race race;
    
    try {
        race = race_manager->getRaceByName(argument);
        if (!race.isPlayerRace()) {
            throw new InvalidRaceException();
        }
    } catch (InvalidRaceException) {
        write_to_buffer(d, "That is not a valid race.\n\r", 0);
        write_to_buffer(d, "The following races are available:\n\r  ", 0);
        for (auto race : race_manager->getAllRaces()) {
            if (race.isPlayerRace()) {
                write_to_buffer(d, race.getName().c_str(), 0);
                write_to_buffer(d, " ", 1);
            }
        }
        write_to_buffer(d, "\n\r", 0);
        write_to_buffer(d, "What is your race? (help for more information) ", 0);
        return;
    }

    ch->race = race.getLegacyId();
    /* initialize stats */
    for (int i = 0; i < MAX_STATS; i++)
        ch->perm_stat[i] = pc_race_table[race.getLegacyId()].stats[i];
    ch->affected_by = ch->affected_by | race.getAffectFlags();
    ch->imm_flags = ch->imm_flags | race.getImmunityFlags();
    ch->res_flags = ch->res_flags | race.getResistanceFlags();
    ch->vuln_flags = ch->vuln_flags | race.getVulnerabilityFlags();
    ch->form = race.getForm();
    ch->parts = race.getParts();

    /* add skills */
    for (int i = 0; i < 5; i++)
    {
        if (pc_race_table[race.getLegacyId()].skills[i] == NULL)
            break;
        group_add(ch, pc_race_table[race.getLegacyId()].skills[i], false);
    }
    /* add cost */
    ch->pcdata->points = pc_race_table[race.getLegacyId()].points;
    ch->size = pc_race_table[race.getLegacyId()].size;

    // Werefolk not present currently
    // if (race_lookup(race_table[race.getLegacyId()].name) == race_lookup("werefolk"))
    // {
    //     int morph;
    //     write_to_buffer(d, "As a werefolk, you may morph into the following forms:\n\r ", 0);
    //     for (morph = 0; morph_table[morph].name[0] != '\0'; morph++)
    //     {
    //         write_to_buffer(d, morph_table[morph].name, 0);
    //         write_to_buffer(d, " ", 1);
    //     }
    //     write_to_buffer(d, "\n\r", 0);
    //     write_to_buffer(d, "What is your morphing race (help for more information)? ", 0);
    //     d->connected = ConnectedState::GetMorph;
    //     return;
    // }

    write_to_buffer(d, "What is your sex (M/F)? ", 0);
    d->connected = ConnectedState::GetNewSex;
}