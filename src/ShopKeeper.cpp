#include "merc.h"
#include "NonPlayerCharacter.h"
#include "Object.h"
#include "ObjectHelper.h"
#include "Room.h"
#include "ShopKeeper.h"

using std::string;

void do_say(Character *ch, char *argument);

ShopKeeper::ShopKeeper(Character *ch)
{
	_keeper = ch;
}

ShopKeeper::~ShopKeeper()
{
	_keeper = nullptr;
}

Character *ShopKeeper::getCharacter() { return _keeper; }

/*
 * Shopping commands.
 */
ShopKeeper * ShopKeeper::find(Character *ch)
{
	/*char buf[MAX_STRING_LENGTH];*/
	Character *keeper;
	SHOP_DATA *pShop;

	pShop = NULL;
	for (keeper = ch->in_room->people; keeper; keeper = keeper->next_in_room)
	{
		if (IS_NPC(keeper) && (pShop = keeper->pIndexData->pShop) != NULL)
			break;
	}

	if (pShop == NULL)
	{
		send_to_char("You can't do that here.\n\r", ch);
		return NULL;
	}

	/*
	 * Shop hours.
	 */
	if (time_info.hour < pShop->open_hour)
	{
		do_say(keeper, (char *)"Sorry, I am closed. Come back later.");
		return NULL;
	}

	if (time_info.hour > pShop->close_hour)
	{
		do_say(keeper, (char *)"Sorry, I am closed. Come back tomorrow.");
		return NULL;
	}

	/*
	 * Invisible or hidden people.
	 */
	if (!keeper->can_see(ch))
	{
		do_say(keeper, (char *)"I don't trade with folks I can't see.");
		return NULL;
	}

	return new ShopKeeper(keeper);
}

void ShopKeeper::buy(Character *ch, string argument)
{
    char buf[MAX_STRING_LENGTH];
	Object *obj;

	if ((obj = get_obj_carry(ch, argument.data(), ch)) == NULL)
	{
		act("$n tells you 'You don't have that item'.", _keeper, NULL, ch, TO_VICT, POS_RESTING);
		ch->reply = _keeper;
		return;
	}

	if (!can_drop_obj(ch, obj))
	{
		send_to_char("You can't let go of it.\n\r", ch);
		return;
	}

	if (!_keeper->can_see(obj))
	{
		act("$n doesn't see what you are offering.", _keeper, NULL, ch, TO_VICT, POS_RESTING);
		return;
	}

	int cost;
	if ((cost = this->get_cost(obj, false)) <= 0)
	{
		act("$n looks uninterested in $p.", _keeper, obj, ch, TO_VICT, POS_RESTING);
		return;
	}

	if (cost > (_keeper->silver + 100 * _keeper->gold))
		_keeper->silver = cost;

	act("$n sells $p.", ch, obj, NULL, TO_ROOM, POS_RESTING);
	/* haggle */
	int roll = number_percent();
	if (!obj->hasStat(ITEM_SELL_EXTRACT) && roll < get_skill(ch, gsn_haggle) && number_range(1, 25) < get_curr_stat(ch, STAT_INT))
	{
		send_to_char("You haggle with the shopkeeper.\n\r", ch);
		cost += obj->getCost() / 2 * roll / 100;
		cost = UMIN(cost, 95 * get_cost(obj, TRUE) / 100);
		cost = UMIN(cost, (_keeper->silver + 100 * _keeper->gold));
		check_improve(ch, gsn_haggle, TRUE, 4);
	}
	snprintf(buf, sizeof(buf), "You sell $p for %d silver and %d gold piece%s.",
			 cost - (cost / 100) * 100, cost / 100, cost == 1 ? "" : "s");
	act(buf, ch, obj, NULL, TO_CHAR, POS_RESTING);
	ch->gold += cost / 100;
	ch->silver += cost - (cost / 100) * 100;
	deduct_cost(_keeper, cost);
	if (_keeper->gold < 0)
		_keeper->gold = 0;
	if (_keeper->silver < 0)
		_keeper->silver = 0;

	if (obj->getItemType() == ITEM_TRASH || obj->hasStat(ITEM_SELL_EXTRACT))
	{
		extract_obj(obj);
	}
	else
	{
		obj_from_char(obj);
		if (obj->getTimer())
			obj->addExtraFlag(ITEM_HAD_TIMER);
		else
			obj->setTimer(number_range(50, 100));
		obj_to_keeper(obj, _keeper);
	}
}

void ShopKeeper::sell(Character *ch, string argument)
{
    char buf[MAX_STRING_LENGTH];
	Object *obj, *t_obj;
	char arg[MAX_INPUT_LENGTH];
	int number, count = 1;

	number = mult_argument(argument.data(), arg);
	obj = get_obj_keeper(ch, _keeper, arg);
	int cost = this->get_cost(obj, true);

	if (number < 1 || number > 200)
	{
		act("$n tells you 'Get real!", _keeper, NULL, ch, TO_VICT, POS_RESTING);
		return;
	}

	if (cost <= 0 || !ch->can_see(obj))
	{
		act("$n tells you 'I don't sell that -- try 'list''.", _keeper, NULL, ch, TO_VICT, POS_RESTING);
		ch->reply = _keeper;
		return;
	}

	if (!obj->hasStat(ITEM_INVENTORY))
	{
		auto contents = obj->getContents();
		for (auto it = contents.begin();
			 count < number && it != contents.end();
			 it++)
		{
			auto t_obj = *it;
			if (t_obj->getObjectIndexData() == obj->getObjectIndexData() && obj->getShortDescription()== t_obj->getShortDescription())
				count++;
			else
				break;
		}

		if (count < number)
		{
			act("$n tells you 'I don't have that many in stock.", _keeper, NULL, ch, TO_VICT, POS_RESTING);
			ch->reply = _keeper;
			return;
		}
	}

	if ((ch->silver + ch->gold * 100) < cost * number)
	{
		if (number > 1)
			act("$n tells you 'You can't afford to buy that many.", _keeper, obj, ch, TO_VICT, POS_RESTING);
		else
			act("$n tells you 'You can't afford to buy $p'.", _keeper, obj, ch, TO_VICT, POS_RESTING);
		ch->reply = _keeper;
		return;
	}

	if (obj->getLevel() > ch->level)
	{
		act("$n tells you 'You can't use $p yet'.", _keeper, obj, ch, TO_VICT, POS_RESTING);
		ch->reply = _keeper;
		return;
	}

	if (ch->carry_number + number * get_obj_number(obj) > can_carry_n(ch))
	{
		send_to_char("You can't carry that many items.\n\r", ch);
		return;
	}

	if (ch->carry_weight + number * get_obj_weight(obj) > can_carry_w(ch))
	{
		send_to_char("You can't carry that much weight.\n\r", ch);
		return;
	}

	/* haggle */
	int roll = number_percent();
	if (!obj->hasStat(ITEM_SELL_EXTRACT) && roll < get_skill(ch, gsn_haggle) && number_range(1, 25) < get_curr_stat(ch, STAT_INT))
	{
		cost -= obj->getCost() / 2 * roll / 100;
		act("You haggle with $N.", ch, NULL, _keeper, TO_CHAR, POS_RESTING);
		check_improve(ch, gsn_haggle, TRUE, 4);
	}

	if (number > 1)
	{
		snprintf(buf, sizeof(buf), "$n buys $p[%d].", number);
		act(buf, ch, obj, NULL, TO_ROOM, POS_RESTING);
		snprintf(buf, sizeof(buf), "You buy $p[%d] for %d silver.", number, cost * number);
		act(buf, ch, obj, NULL, TO_CHAR, POS_RESTING);
	}
	else
	{
		act("$n buys $p.", ch, obj, NULL, TO_ROOM, POS_RESTING);
		snprintf(buf, sizeof(buf), "You buy $p for %d silver.", cost);
		act(buf, ch, obj, NULL, TO_CHAR, POS_RESTING);
	}
	deduct_cost(ch, cost * number);
	_keeper->gold += cost * number / 100;
	_keeper->silver += cost * number - (cost * number / 100) * 100;

	for (count = 0; count < number; count++)
	{
		if (IS_SET(obj->getExtraFlags(), ITEM_INVENTORY))
			t_obj = ObjectHelper::createFromIndex(obj->getObjectIndexData(), obj->getLevel());
		else
		{
			t_obj = obj;
			obj->addObject(obj);
			obj_from_char(t_obj);
		}

		if (t_obj->getTimer() > 0 && !t_obj->hasStat(ITEM_HAD_TIMER))
			t_obj->setTimer(0);
		t_obj->removeExtraFlag(ITEM_HAD_TIMER);
		ch->addObject(t_obj);
		if (cost < t_obj->getCost())
			t_obj->setCost(cost);
	}
}

void ShopKeeper::list(Character *ch, string arg)
{
	char buf[MAX_STRING_LENGTH];
	int cost, count;
	bool found = false;

	for (auto obj : _keeper->getCarrying())
	{
		if (obj->getWearLocation() == WEAR_NONE && ch->can_see(obj) && (cost = this->get_cost(obj, TRUE)) > 0 && (arg[0] == '\0' || is_name(arg.c_str(), obj->getName().c_str())))
		{
			if (!found)
			{
				found = true;
				send_to_char("[Lv Price Qty] Item\n\r", ch);
			}

			if (obj->hasStat(ITEM_INVENTORY))
				snprintf(buf, sizeof(buf), "[%2d %5d -- ] %s\n\r",
						 obj->getLevel(), cost, obj->getShortDescription().c_str());
			else
			{
				count = 1;

				// TODO: compress list
				// while (obj->next_content != NULL && obj->getObjectIndexData() == obj->next_content->pIndexData && !str_cmp(obj->short_descr, obj->next_content->short_descr))
				// {
				// 	obj = obj->next_content;
				// 	count++;
				// }
				snprintf(buf, sizeof(buf), "[%2d %5d %2d ] %s\n\r",
						 obj->getLevel(), cost, count, obj->getShortDescription().c_str());
			}
			send_to_char(buf, ch);
		}
	}

	if (!found)
	{
		send_to_char("You can't buy anything here.\n\r", ch);
	}
}

/* insert an object at the right spot for the keeper */
void ShopKeeper::obj_to_keeper(Object *obj, Character *ch)
{
	Object *t_obj, *t_obj_next;

	/* see if any duplicates are found */
	for (auto o : ch->getCarrying())
	{
		t_obj = o;
		if (obj->getObjectIndexData() == o->getObjectIndexData() && obj->getShortDescription() == o->getShortDescription())
		{
			/* if this is an unlimited item, destroy the new one */
			if (t_obj->hasStat(ITEM_INVENTORY))
			{
				extract_obj(obj);
				return;
			}
			obj->setCost(o->getCost()); /* keep it standard */
			break;
		}
	}

	ch->addObject(obj);
	ch->carry_number += get_obj_number(obj);
	ch->carry_weight += get_obj_weight(obj);
}

/* get an object from a shopkeeper's list */
Object *ShopKeeper::get_obj_keeper(Character *ch, Character *keeper, char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	int number;
	int count;

	number = number_argument(argument, arg);
	count = 0;
	for (auto obj : _keeper->getCarrying())
	{
		if (obj->getWearLocation() == WEAR_NONE && _keeper->can_see(obj) && ch->can_see(obj) && is_name(arg, obj->getName().c_str()))
		{
			if (++count == number)
				return obj;

			// /* skip other objects of the same name */
			// while (obj->next_content != NULL && obj->getObjectIndexData() == obj->next_content->pIndexData && !str_cmp(obj->short_descr, obj->next_content->short_descr))
			// 	obj = obj->next_content;
		}
	}

	return NULL;
}

int ShopKeeper::get_cost(Object *obj, bool fBuy)
{
	SHOP_DATA *pShop;
	int cost;

	if (obj == NULL || (pShop = _keeper->pIndexData->pShop) == NULL)
		return 0;

	if (fBuy)
	{
		cost = obj->getCost() * pShop->profit_buy / 100;
	}
	else
	{
		//	Object *obj2;
		int itype;

		cost = 0;
		for (itype = 0; itype < MAX_TRADE; itype++)
		{
			if (obj->getItemType() == pShop->buy_type[itype])
			{
				cost = obj->getCost() * pShop->profit_sell / 100;
				break;
			}
		}

		/*
		 * This makes keepers buy for less, no good -Blizzard
		 *
		if (!obj->hasStat(ITEM_SELL_EXTRACT))
			for ( obj2 = keeper->carrying; obj2; obj2 = obj2->next_content )
			{
				if ( obj->getObjectIndexData() == obj2->pIndexData
			&&   !str_cmp(obj->short_descr,obj2->short_descr) )
			{
				if (obj2->hasStat(ITEM_INVENTORY))
				cost /= 2;
				else
							cost = cost * 3 / 4;
			}
			}
		*/
	}

	/*
	 * This adjusts the prices of staves/wands that have their spells
	 * in charges accordingly
	 */
	if (obj->getItemType() == ITEM_STAFF || obj->getItemType() == ITEM_WAND)
	{
		if (obj->getValues().at(1) == 0)
			cost /= 4;
		else
			cost = cost * obj->getValues().at(2) / obj->getValues().at(1);
	}

	return cost;
}