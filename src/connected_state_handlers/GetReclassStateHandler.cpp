#include "merc.h"
#include "GetReclassStateHandler.h"
#include "ILogger.h"
#include "PlayerCharacter.h"
#include "recycle.h"
#include "SocketHelper.h"
#include "Wiznet.h"

/* global functions */
void do_help(Character *ch, char *argument);
void do_skills(Character *ch, char *argument);

GetReclassStateHandler::GetReclassStateHandler(ILogger *logger) { _logger = logger; }
GetReclassStateHandler::~GetReclassStateHandler() { }
bool GetReclassStateHandler::canHandleState(ConnectedState state) {
    return state == ConnectedState::GetReclass || state == ConnectedState::ReclassCust;
}

void GetReclassStateHandler::handle(DESCRIPTOR_DATA *d, char *argument) {
    if (d->connected == ConnectedState::GetReclass) {
        handleReclassSelection(d, argument);
    }

    if (d->connected == ConnectedState::ReclassCust) {
        handleReclassCustomization(d, argument);
    }
}

void GetReclassStateHandler::handleReclassCustomization(DESCRIPTOR_DATA *d, char *argument) {
    char buf[MAX_STRING_LENGTH];
    PlayerCharacter *ch = (PlayerCharacter *)d->character;
    send_to_char("\n\r", ch);
    if (!str_cmp(argument, "done"))
    {
        if (ch->points < ch->min_reclass_points)
            ch->points = ch->min_reclass_points;
        snprintf(buf, sizeof(buf), "Creation points: %d\n\r", ch->points);
        send_to_char(buf, ch);
        snprintf(buf, sizeof(buf), "Experience per level: %d\n\r",
                    exp_per_level(ch, ch->gen_data->points_chosen));
        ch->exp = exp_per_level(ch, ch->gen_data->points_chosen) * ch->level;
        free_gen_data(ch->gen_data);
        ch->gen_data = NULL;
        send_to_char(buf, ch);
        char_from_room(ch);
        char_to_room(ch, ((PlayerCharacter *)ch)->getWasNoteRoom());
        d->connected = ConnectedState::Playing;
        return;
    }
    if (!parse_gen_groups(ch, argument))
        send_to_char(
            "Choices are: list,learned,premise,add,drop,info,help, and done.\n\r", ch);

    do_help(ch, (char *)"menu choice");
}

void GetReclassStateHandler::handleReclassSelection(DESCRIPTOR_DATA *d, char *argument) {
    auto ch = (PlayerCharacter *)d->character;
    auto iClass = class_lookup(argument);

    if (iClass == -1 || IS_SET(ch->done, class_table[iClass].flag) || !can_be_class(ch, iClass))
    {
        SocketHelper::write_to_buffer(d,
                        "That class is not an option.\n\rWhat IS your class? ", 0);
        return;
    }

    ch->class_num = iClass;
    SET_BIT(ch->done, class_table[ch->class_num].flag);

    snprintf(log_buf, 2 * MAX_INPUT_LENGTH, "%s@%s reclasses as the %s class.", ch->getName().c_str(), d->host, class_table[ch->class_num].name);
    _logger->log_string(log_buf);
    Wiznet::instance()->report((char *)"Reclass alert!  $N sighted.", ch, NULL, WIZ_NEWBIE, 0, 0);
    Wiznet::instance()->report(log_buf, NULL, NULL, WIZ_SITES, 0, ch->getTrust());

    group_add(ch, class_table[ch->class_num].base_group, FALSE);
    ch->gen_data = new_gen_data();
    ch->gen_data->points_chosen = ch->points;
    do_help(ch, (char *)"group header");
    list_group_costs(ch);
    SocketHelper::write_to_buffer(d, "You already have the following skills:\n\r", 0);
    ch->min_reclass_points = ch->points + 15;
    do_skills(ch, (char *)"all");
    do_help(ch, (char *)"menu choice");
    d->connected = ConnectedState::ReclassCust;
}
