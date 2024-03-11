#include "merc.h"
#include "Character.h"
#include "ExitFlag.h"
#include "FlagHelper.h"
#include "NonPlayerCharacter.h"
#include "Portal.h"
#include "Room.h"

/* global functions */
void do_look(Character *ch, char *argument);
void do_stand(Character *ch, char *argument);

Portal::Portal() { }
Portal::~Portal() {}

bool Portal::isPortal() { return true; }

bool Portal::isTraversableByCharacter(Character *ch)
{
	return FlagHelper::isSet(this->getValues().at(1), ExitFlag::ExitClosed) && !IS_TRUSTED(ch, ANGEL);
}

void Portal::enter(Character *ch)
{
	ROOM_INDEX_DATA *old_room = ch->in_room;
	ROOM_INDEX_DATA *location = nullptr;
	if (!this->isTraversableByCharacter(ch))
	{
		throw PortalNotTraversableException();
	}

	if (!IS_TRUSTED(ch, ANGEL) && !FlagHelper::isSet(this->getValues().at(2), GATE_NOCURSE) && (IS_AFFECTED(ch, AFF_CURSE) || FlagHelper::isSet(ch->in_room->room_flags, ROOM_NO_RECALL)))
	{
		throw PortalUeePreventedByCurseException();
	}

	if (FlagHelper::isSet(this->getValues().at(2), GATE_RANDOM) || this->getValues().at(3) == -1)
	{
		location = get_random_room(ch);
		this->getValues().at(3) = location->vnum; /* for record keeping :) */
	}
	else if (FlagHelper::isSet(this->getValues().at(2), GATE_BUGGY) && (number_percent() < 5))
		location = get_random_room(ch);
	else
		location = get_room_index(this->getValues().at(3));

	if (location == NULL || location == old_room || !can_see_room(ch, location) || (room_is_private(location) && !IS_TRUSTED(ch, IMPLEMENTOR)))
	{
		act("$p doesn't seem to go anywhere.", ch, this, NULL, TO_CHAR, POS_RESTING);
		return;
	}

	if (ch->isNPC() && FlagHelper::isSet(ch->act, ACT_AGGRESSIVE) && FlagHelper::isSet(location->room_flags, ROOM_LAW))
	{
		send_to_char("Something prevents you from leaving...\n\r", ch);
		return;
	}

	act("$n steps into $p.", ch, this, NULL, TO_ROOM, POS_RESTING);

	if (FlagHelper::isSet(this->getValues().at(2), GATE_NORMAL_EXIT))
		act("You enter $p.", ch, this, NULL, TO_CHAR, POS_RESTING);
	else
		act("You walk through $p and find yourself somewhere else...", ch, this, NULL, TO_CHAR, POS_RESTING);

	char_from_room(ch);
	char_to_room(ch, location);

	if (FlagHelper::isSet(this->getValues().at(2), GATE_GOWITH)) /* take the gate along */
	{
		obj_from_room(this);
		obj_to_room(this, location);
	}

	if (FlagHelper::isSet(this->getValues().at(2), GATE_NORMAL_EXIT))
		act("$n has arrived.", ch, this, NULL, TO_ROOM, POS_RESTING);
	else
		act("$n has arrived through $p.", ch, this, NULL, TO_ROOM, POS_RESTING);

	do_look(ch, (char *)"auto");

	/* charges */
	if (this->getValues().at(0) > 0)
	{
		this->getValues().at(0)--;
		if (this->getValues().at(0) == 0)
			this->getValues().at(0) = -1;
	}

	/* protect against circular follows */
	if (old_room == location)
		return;

	for (Character *fch = old_room->people; fch != NULL; fch = fch->next_in_room)
	{
		if (this->getValues().at(0) == -1)
		{
			/* no following through dead portals */
			continue;
		}

		if (fch->master == ch && IS_AFFECTED(fch, AFF_CHARM) && fch->position < POS_STANDING)
			do_stand(fch, (char *)"");

		if (fch->master == ch && fch->position == POS_STANDING)
		{

			if (FlagHelper::isSet(ch->in_room->room_flags, ROOM_LAW) && (IS_NPC(fch) && FlagHelper::isSet(fch->act, ACT_AGGRESSIVE)))
			{
				act("You can't bring $N into the city.", ch, NULL, fch, TO_CHAR, POS_RESTING);
				act("You aren't allowed in the city.", fch, NULL, NULL, TO_CHAR, POS_RESTING);
				continue;
			}

			act("You follow $N.", fch, NULL, ch, TO_CHAR, POS_RESTING);
			this->enter(fch);
		}
	}

	if (this->getValues().at(0) == -1)
	{
		act("$p fades out of existence.", ch, this, NULL, TO_CHAR, POS_RESTING);
		if (ch->in_room == old_room)
			act("$p fades out of existence.", ch, this, NULL, TO_ROOM, POS_RESTING);
		else if (old_room->people != NULL)
		{
			act("$p fades out of existence.", old_room->people, this, NULL, TO_CHAR, POS_RESTING);
			act("$p fades out of existence.", old_room->people, this, NULL, TO_ROOM, POS_RESTING);
		}
		extract_obj(this);
	}

	/*
	 * If someone is following the char, these triggers get activated
	 * for the followers before the char, but it's safer this way...
	 */
	if (ch->isNPC() && HAS_TRIGGER(ch, TRIG_ENTRY))
		mp_percent_trigger(ch, NULL, NULL, NULL, TRIG_ENTRY);
	if (!ch->isNPC())
		mp_greet_trigger(ch);

	return;
}