#include "merc.h"
#include "GetNewClassStateHandler.h"
#include "ILogger.h"
#include "IRaceManager.h"
#include "PlayerCharacter.h"
#include "recycle.h"
#include "SocketHelper.h"
#include "Wiznet.h"

/* global functions */
void do_help(Character *ch, char *argument);
void do_skills(Character *ch, char *argument);

GetNewClassStateHandler::GetNewClassStateHandler(ILogger * logger, IRaceManager * race_manager) {
    _logger = logger;
    _raceManager = race_manager;
}
GetNewClassStateHandler::~GetNewClassStateHandler() { }
bool GetNewClassStateHandler::canHandleState(ConnectedState state) {
    return state == ConnectedState::DefaultChoice
        || state == ConnectedState::GenGroups
        || state == ConnectedState::GetNewClass
        || state == ConnectedState::PickWeapon;
}

void GetNewClassStateHandler::handle(DESCRIPTOR_DATA *d, char *argument) {
    if (d->connected == ConnectedState::DefaultChoice) {
        handleDefaultChoice(d, argument);
    }
    if (d->connected == ConnectedState::GenGroups) {
        handleGenGroups(d, argument);
    }
    if (d->connected == ConnectedState::GetNewClass) {
        handleNewClass(d, argument);
    }
    if (d->connected == ConnectedState::PickWeapon) {
        handlePickWeapon(d, argument);
    }
}

void GetNewClassStateHandler::handleGenGroups(DESCRIPTOR_DATA *d, char *argument) {
    char buf[MAX_STRING_LENGTH];
    PlayerCharacter * ch = (PlayerCharacter*)d->character;
    send_to_char("\n\r", ch);
    if (!str_cmp(argument, "done"))
    {
        snprintf(buf, sizeof(buf), "Creation points: %d\n\r", ch->points);
        send_to_char(buf, ch);
        snprintf(buf, sizeof(buf), "Experience per level: %d\n\r",
                    exp_per_level(ch, ch->gen_data->points_chosen));
        if (ch->points < 40)
            ch->train = (40 - ch->points + 1) / 2;
        free_gen_data(ch->gen_data);
        ch->gen_data = NULL;
        send_to_char(buf, ch);
        SocketHelper::write_to_buffer(d, "\n\r", 2);
        SocketHelper::write_to_buffer(d,
                        "Please pick a weapon from the following choices:\n\r", 0);
        buf[0] = '\0';
        for (auto i = 0; weapon_table[i].name != NULL; i++)
            if (ch->learned[*weapon_table[i].gsn] > 0)
            {
                strcat(buf, weapon_table[i].name);
                strcat(buf, " ");
            }
        strcat(buf, "\n\rYour choice? ");
        SocketHelper::write_to_buffer(d, buf, 0);
        d->connected = ConnectedState::PickWeapon;
        return;
    }

    if (!parse_gen_groups(ch, argument))
        send_to_char(
            "Choices are: list,learned,premise,add,drop,info,help, and done.\n\r", ch);

    do_help(ch, (char *)"menu choice");
}

void GetNewClassStateHandler::handlePickWeapon(DESCRIPTOR_DATA *d, char *argument) {
    char buf[MAX_STRING_LENGTH];
    PlayerCharacter * ch = (PlayerCharacter*)d->character;

    SocketHelper::write_to_buffer(d, "\n\r", 2);
    auto weapon = weapon_lookup(argument);
    if (weapon == -1 || ch->learned[*weapon_table[weapon].gsn] <= 0)
    {
        SocketHelper::write_to_buffer(d,
                        "That's not a valid selection. Choices are:\n\r", 0);
        buf[0] = '\0';
        for (auto i = 0; weapon_table[i].name != NULL; i++)
            if (ch->learned[*weapon_table[i].gsn] > 0)
            {
                strcat(buf, weapon_table[i].name);
                strcat(buf, " ");
            }
        strcat(buf, "\n\rYour choice? ");
        SocketHelper::write_to_buffer(d, buf, 0);
        return;
    }

    ch->learned[*weapon_table[weapon].gsn] = 40;
    SocketHelper::write_to_buffer(d, "\n\r", 2);
    do_help(ch, (char *)"motd");
    d->connected = ConnectedState::ReadMotd;
}

void GetNewClassStateHandler::handleDefaultChoice(DESCRIPTOR_DATA *d, char *argument) {
    char buf[MAX_STRING_LENGTH];
    PlayerCharacter * ch = (PlayerCharacter*)d->character;
    SocketHelper::write_to_buffer(d, "\n\r", 2);
    switch (argument[0])
    {
    case 'y':
    case 'Y':
        ch->gen_data = new_gen_data();
        ch->gen_data->points_chosen = ch->points;
        do_help(ch, (char *)"group header");
        list_group_costs(ch);
        SocketHelper::write_to_buffer(d, (char *)"You already have the following skills:\n\r", 0);
        do_skills(ch, (char *)"all");
        do_help(ch, (char *)"menu choice");
        d->connected = ConnectedState::GenGroups;
        break;
    case 'n':
    case 'N':
        group_add(ch, class_table[ch->class_num].default_group, TRUE);
        SocketHelper::write_to_buffer(d, "\n\r", 2);
        SocketHelper::write_to_buffer(d,
                        "Please pick a weapon from the following choices:\n\r", 0);
        buf[0] = '\0';
        for (auto i = 0; weapon_table[i].name != NULL; i++)
            if (ch->learned[*weapon_table[i].gsn] > 0)
            {
                strcat(buf, weapon_table[i].name);
                strcat(buf, " ");
            }
        strcat(buf, "\n\rYour choice? ");
        SocketHelper::write_to_buffer(d, buf, 0);
        d->connected = ConnectedState::PickWeapon;
        break;
    default:
        SocketHelper::write_to_buffer(d, "Please answer (Y/N)? ", 0);
        return;
    }
}

void GetNewClassStateHandler::handleNewClass(DESCRIPTOR_DATA *d, char *argument) {
    auto ch = (PlayerCharacter *)d->character;
    auto iClass = class_lookup(argument);

    if (iClass == -1 || !can_be_class(ch, iClass))
    {
        SocketHelper::write_to_buffer(d,
                        "That's not a class.\n\rWhat IS your class? ", 0);
        return;
    }

    ch->class_num = iClass;
    SET_BIT(ch->done, class_table[ch->class_num].flag);

    snprintf(log_buf, 2 * MAX_INPUT_LENGTH, "%s@%s new player.", ch->getName().c_str(), d->host);
    _logger->log_string(log_buf);
    Wiznet::instance()->report((char *)"Newbie alert!  $N sighted.", ch, NULL, WIZ_NEWBIE, 0, 0);
    Wiznet::instance()->report(log_buf, NULL, NULL, WIZ_SITES, 0, ch->getTrust());

    SocketHelper::write_to_buffer(d, "\n\r", 0);

    group_add(ch, "rom basics", FALSE);
    group_add(ch, class_table[ch->class_num].base_group, FALSE);
    ch->learned[gsn_recall] = 50;

    /*
    ** The werefolk have to know what they're doing.
    ** The check looks between 10 and 70 for skill,
    ** so we'll start these guys off at 25
    */
    if (ch->getRace() == _raceManager->getRaceByName("werefolk"))
        ch->learned[gsn_morph] = 25;

    SocketHelper::write_to_buffer(d, "Do you wish to customize this character?\n\r", 0);
    SocketHelper::write_to_buffer(d, "Customization takes time, but allows a wider range of skills and abilities.\n\r", 0);
    SocketHelper::write_to_buffer(d, "Customize (Y/N)? ", 0);
    d->connected = ConnectedState::DefaultChoice;
}
