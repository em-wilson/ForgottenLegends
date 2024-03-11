#include "merc.h"
#include "ObjectHelper.h"
#include "PlayerCharacter.h"
#include "ReadMotdStateHandler.h"
#include "SocketHelper.h"
#include "Wiznet.h"

/* global functions */
void do_board (Character *ch, char *argument);
void do_help(Character *ch, char *argument);
void do_look(Character *ch, char *argument);
void do_outfit(Character *caller, char *argument);

ReadMotdStateHandler::ReadMotdStateHandler() : AbstractStateHandler(ConnectedState::ReadMotd) {}
ReadMotdStateHandler::~ReadMotdStateHandler() {}

void ReadMotdStateHandler::handle(DESCRIPTOR_DATA *d, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    PlayerCharacter * ch = (PlayerCharacter*) d->character;
    if (ch->getPassword().empty())
    {
        SocketHelper::write_to_buffer(d, "Warning! Null password!\n\r", 0);
        SocketHelper::write_to_buffer(d, "Please report old password with bug.\n\r", 0);
        SocketHelper::write_to_buffer(d,
                                      "Type 'password null <new password>' to fix.\n\r", 0);
    }

    char_list.push_back(ch);
    d->connected = ConnectedState::Playing;
    send_to_char("The early bird may get the worm, but it's the second mouse that gets\n\r", ch);
    send_to_char("the cheese.\n\r\n\r\n\r", ch);
    reset_char(ch);

    if (ch->level == 0)
    {

        ch->perm_stat[class_table[ch->class_num].attr_prime] += 3;

        ch->level = 1;
        ch->exp = exp_per_level(ch, ch->points);
        ch->hit = ch->max_hit;
        ch->mana = ch->max_mana;
        ch->move = ch->max_move;
        ch->train = 5;
        ch->practice = 5;
        snprintf(buf, sizeof(buf), "the %s", capitalize(class_table[ch->class_num].name));
        set_title(ch, buf);

        do_outfit(ch, (char *)"");
        ch->addObject(ObjectHelper::createFromIndex(get_obj_index(OBJ_VNUM_MAP), 0));

        char_to_room(ch, get_room_index(ROOM_VNUM_SCHOOL));
        send_to_char("\n\r", ch);
        do_help(ch, (char *)"NEWBIE INFO");
        SET_BIT(ch->act, PLR_COLOUR);
        SET_BIT(ch->act, PLR_AUTOASSIST);
        SET_BIT(ch->act, PLR_AUTOLOOT);
        SET_BIT(ch->act, PLR_AUTOSAC);
        SET_BIT(ch->act, PLR_AUTOGOLD);
        SET_BIT(ch->act, PLR_AUTOEXIT);
        send_to_char("\n\r", ch);
    }
    else if (ch->in_room != NULL)
    {
        char_to_room(ch, ch->in_room);
    }
    else if (IS_IMMORTAL(ch))
    {
        char_to_room(ch, get_room_index(ROOM_VNUM_CHAT));
    }
    else
    {
        char_to_room(ch, get_room_index(ROOM_VNUM_TEMPLE));
    }

    /* We don't want them showing up in the note room */
    if (ch->in_room == get_room_index(ROOM_VNUM_NOTE))
    {
        char_from_room(ch);
        char_to_room(ch, get_room_index(ROOM_VNUM_LIMBO));
    }

    act("$n has entered the game.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
    do_look(ch, (char *)"auto");

    Wiznet::instance()->report((char *)"$N has left real life behind.", ch, NULL,
                               WIZ_LOGINS, WIZ_SITES, ch->getTrust());

    if (ch->pet != NULL)
    {
        char_to_room(ch->pet, ch->in_room);
        act("$n has entered the game.", ch->pet, NULL, NULL, TO_ROOM, POS_RESTING);
    }

    send_to_char("\n", ch);
    do_board(ch, (char *)""); /* Show board status */
}