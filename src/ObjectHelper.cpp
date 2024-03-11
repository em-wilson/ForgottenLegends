#include "merc.h"
#include "Character.h"
#include "Object.h"
#include "ObjectHelper.h"
#include "Portal.h"

using std::list;
using std::string;
using std::vector;

int ObjectHelper::countInList(OBJ_INDEX_DATA *pObjIndex, list<Object *> list) {
    if (list.begin() == list.end()) return 0;
    return countInList(pObjIndex, std::vector<Object *>(list.begin(), list.end()));
}

int ObjectHelper::countInList(OBJ_INDEX_DATA *pObjIndex, vector<Object *> list) {
    if (list.size() == 0) return 0;

    int nMatch = 0;

    for ( auto obj : list )
    {
	    if ( obj->getObjectIndexData() == pObjIndex ) {
    	    nMatch++;
        }
    }

    return nMatch;
}

Object * ObjectHelper::findInList(Character *ch, string argument, list<Object *> list) {
    return findInList(ch, argument, std::vector<Object *>(list.begin(), list.end()));
}

Object * ObjectHelper::findInList(Character *ch, string argument, vector<Object *> list) {
    char arg[MAX_INPUT_LENGTH];
    Object *obj;
    int number;
    int count;

    number = number_argument( argument.data(), arg );
    count  = 0;
    for ( auto obj : list )
    {
	if ( ch->can_see( obj ) && is_name( arg, obj->getName().c_str() ) )
	{
	    if ( ++count == number )
		return obj;
	}
    }

    return nullptr;
}

bool ObjectHelper::isObjectOwnedBy(Object *obj, Character *ch) {
    return obj->getOwner() == ch->getName();
}

Object * ObjectHelper::getFountainFromList(vector<Object *> list) {
    for ( auto fountain : list )
    {
		if ( fountain->getItemType() == ITEM_FOUNTAIN )
		{
			return fountain;
		}
    }

    return nullptr;
}

/*
 * Create an instance of an object.
 */
Object * ObjectHelper::createFromIndex(OBJ_INDEX_DATA *pObjIndex, int level)
{
    AFFECT_DATA *paf;
    Object *obj;
    int i;

    if (pObjIndex == NULL)
    {
        bug("ObjectHelper::createFromIndex: NULL pObjIndex.", 0);
        exit(1);
    }

    switch (pObjIndex->item_type) {
        case ITEM_PORTAL:
            obj = new Portal();
            break;

        default:
            obj = new Object();
            break;
    }
    obj->setObjectIndexData(pObjIndex);

    obj->setLevel(pObjIndex->new_format ? pObjIndex->level : UMAX(0, level));

    obj->setName(pObjIndex->name);
    obj->setShortDescription(pObjIndex->short_descr);
    obj->setDescription(pObjIndex->description);
    obj->setItemType(pObjIndex->item_type);
    obj->setExtraFlags(pObjIndex->extra_flags);
    obj->setWearFlags(pObjIndex->wear_flags);
    obj->setWeight(pObjIndex->weight);
    obj->setCondition(pObjIndex->condition);
    obj->getValues().at(0) = pObjIndex->value[0];
    obj->getValues().at(1) = pObjIndex->value[1];
    obj->getValues().at(2) = pObjIndex->value[2];
    obj->getValues().at(3) = pObjIndex->value[3];
    obj->getValues().at(4) = pObjIndex->value[4];

    if (level == -1 || pObjIndex->new_format)
        obj->setCost(pObjIndex->cost);
    else
        obj->setCost(number_fuzzy(10) * number_fuzzy(level) * number_fuzzy(level));

    /*
     * Mess with object properties.
     */
    switch (obj->getItemType())
    {
    default:
        bug("Read_object: vnum %d bad type.", pObjIndex->vnum);
        break;

    case ITEM_LIGHT:
        if (obj->getValues().at(2) == 999)
            obj->getValues().at(2) = -1;
        break;

    case ITEM_FURNITURE:
    case ITEM_TRASH:
    case ITEM_CONTAINER:
    case ITEM_DRINK_CON:
    case ITEM_KEY:
    case ITEM_FOOD:
    case ITEM_BOAT:
    case ITEM_CORPSE_NPC:
    case ITEM_CORPSE_PC:
    case ITEM_FOUNTAIN:
    case ITEM_MAP:
    case ITEM_CLOTHING:
    case ITEM_PORTAL:
        if (!pObjIndex->new_format)
            obj->setCost(obj->getCost() / 5);
        break;

    case ITEM_NUKE:
    case ITEM_ARROW:
    case ITEM_BOW:
    case ITEM_BANKNOTE:
    case ITEM_TREASURE:
    case ITEM_WARP_STONE:
    case ITEM_ROOM_KEY:
    case ITEM_GEM:
    case ITEM_JEWELRY:
    case ITEM_WINDOW:
        break;

    case ITEM_JUKEBOX:
        for (i = 0; i < 5; i++)
            obj->getValues().at(i) = -1;
        break;

    case ITEM_SCROLL:
        if (level != -1 && !pObjIndex->new_format)
            obj->getValues().at(0) = number_fuzzy(obj->getValues().at(0));
        break;

    case ITEM_WAND:
    case ITEM_STAFF:
        if (level != -1 && !pObjIndex->new_format)
        {
            obj->getValues().at(0) = number_fuzzy(obj->getValues().at(0));
            obj->getValues().at(1) = number_fuzzy(obj->getValues().at(1));
            obj->getValues().at(2) = obj->getValues().at(1);
        }
        if (!pObjIndex->new_format)
            obj->setCost(obj->getCost() * 2);
        break;

    case ITEM_WEAPON:
        if (level != -1 && !pObjIndex->new_format)
        {
            obj->getValues().at(1) = number_fuzzy(number_fuzzy(1 * level / 4 + 2));
            obj->getValues().at(2) = number_fuzzy(number_fuzzy(3 * level / 4 + 6));
        }
        break;

    case ITEM_ARMOR:
        if (level != -1 && !pObjIndex->new_format)
        {
            obj->getValues().at(0) = number_fuzzy(level / 5 + 3);
            obj->getValues().at(1) = number_fuzzy(level / 5 + 3);
            obj->getValues().at(2) = number_fuzzy(level / 5 + 3);
        }
        break;

    case ITEM_POTION:
    case ITEM_PILL:
        if (level != -1 && !pObjIndex->new_format)
            obj->getValues().at(0) = number_fuzzy(number_fuzzy(obj->getValues().at(0)));
        break;

    case ITEM_MONEY:
        if (!pObjIndex->new_format)
            obj->getValues().at(0) = obj->getCost();
        break;
    }

    for (auto paf : pObjIndex->affected) {
        if (paf->location == APPLY_SPELL_AFFECT) {
            obj->giveAffect(paf);
        }
    }

    object_list.push_back(obj);
    pObjIndex->count++;

    return obj;
}