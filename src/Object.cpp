#include "merc.h"
#include "ExtraDescription.h"
#include "Object.h"
#include "ObjectHelper.h"
#include "Room.h"

using std::string;
using std::vector;

void free_affect(AFFECT_DATA *af);
AFFECT_DATA *new_affect(void);

Object::Object() {
    _inRoom = nullptr;
    _enchanted = false;
    _values = vector<int>{0, 0, 0, 0, 0};
    _wearLocation = -1;
}

Object::~Object()
{
    AFFECT_DATA *paf, *paf_next;
    ExtraDescription *ed, *ed_next;

    for (auto paf : _affected)
    {
        _affected.remove(paf);
        free_affect(paf);
    }

    for (auto ed : _extraDescriptions)
    {
        _extraDescriptions.remove(ed);
        delete ed;
    }
}

bool Object::isPortal() { return false; }

/* duplicate an object exactly -- except contents */
Object *Object::clone()
{
    int i;
    AFFECT_DATA *paf;
    ExtraDescription *ed, *ed_new;

    /* start fixing the object */
    Object *clone = ObjectHelper::createFromIndex(this->getObjectIndexData(), 0);
    clone->_name = _name;
    clone->_shortDescription = _shortDescription;
    clone->_description = _description;
    clone->_itemType = _itemType;
    clone->_extraFlags = _extraFlags;
    clone->_wearFlags = _wearFlags;
    clone->_weight = _weight;
    clone->_cost = _cost;
    clone->_level = _level;
    clone->_condition = _condition;
    clone->_material = _material;
    clone->_timer = _timer;

    for (i = 0; i < 5; i++)
        clone->_values.at(i) = _values.at(i);

    /* affects */
    clone->_enchanted = _enchanted;

    for (auto paf : _affected) {
        clone->addAffect(paf);
    }

    /* extended desc */
    for (auto ed : _extraDescriptions)
    {
        clone->_extraDescriptions.push_back(ed->clone());
    }

    return clone;
}

std::vector<AFFECT_DATA *> Object::getAffectedBy() { return vector<AFFECT_DATA *>(_affected.begin(), _affected.end()); }
void Object::addAffect(AFFECT_DATA *af) { _affected.push_back(af); }

Character *Object::getCarriedBy() { return _carriedBy; }
void Object::setCarriedBy(Character *ch) { _carriedBy = ch; }

short int Object::getCondition() { return _condition; }
void Object::setCondition(short int value) { _condition = value; }

std::vector<Object *> Object::getContents() { return vector<Object *>(_contains.begin(), _contains.end() ); }

int Object::getCost() { return _cost; }
void Object::setCost(int value) { _cost = value; }

std::vector<ExtraDescription *> Object::getExtraDescriptions() { return vector<ExtraDescription *>(_extraDescriptions.begin(), _extraDescriptions.end()); }
void Object::addExtraDescription(ExtraDescription *ed) { _extraDescriptions.push_back(ed); }

string Object::getDescription() { return _description; }
void Object::setDescription(std::string value) { _description = value; }

bool Object::hasStat(long stat)
{
    return this->_extraFlags & stat;
}
int Object::getExtraFlags() { return _extraFlags; }
void Object::setExtraFlags(int value) { _extraFlags = value; }
void Object::removeExtraFlag(int bit) { _extraFlags &= ~bit; }
void Object::addExtraFlag(int bit) { _extraFlags |= bit; }

ROOM_INDEX_DATA *Object::getInRoom() { return _inRoom; }
void Object::setInRoom(ROOM_INDEX_DATA *room) { _inRoom = room; }

Object *Object::getInObject() { return _inObject; }
void Object::setInObject(Object *obj) { _inObject = obj; }

short int Object::getItemType() { return _itemType; }
void Object::setItemType(short int value) { _itemType = value; }

short int Object::getLevel() { return _level; }
void Object::setLevel(short int value) { _level = value; }

string Object::getName() { return _name; }
void Object::setName(string value) { _name = value; }

string Object::getShortDescription() { return _shortDescription; }
void Object::setShortDescription(string value) { _shortDescription = value; }

OBJ_INDEX_DATA * Object::getObjectIndexData() { return _pIndexData; }
OBJ_INDEX_DATA * Object::setObjectIndexData(OBJ_INDEX_DATA *pIndexData) { _pIndexData = pIndexData; return _pIndexData; }

vector<int> Object::getValues() { return _values; }

int Object::getWearFlags() { return _wearFlags; }
void Object::setWearFlags(int value) { _wearFlags = value; }

short int Object::getWearLocation() { return _wearLocation; }
void Object::setWearLocation(short int value) { _wearLocation = value; }

short int Object::getWeight() { return _weight; }
void Object::setWeight(short int value) { _weight = value; }

bool Object::hasOwner()
{
    return !_owner.empty();
}

string Object::getOwner() { return _owner; }
void Object::setOwner(std::string name) { _owner = name; }

bool Object::canWear(long position)
{
    return this->_wearFlags & position;
}

short int Object::getTimer() { return _timer; }
void Object::setTimer(short int value) { _timer = value; }
short int Object::decrementTimer(short int value) {
    _timer -= value;
    return _timer;
}

bool Object::isEnchanted() { return _enchanted; }
void Object::setIsEnchanted(bool value) { _enchanted = true; }

/* find an effect in an affect list */
AFFECT_DATA *Object::findAffectBySn(int sn)
{
    for (auto paf_find : _affected)
    {
        if (paf_find->type == sn)
        {
            return paf_find;
        }
    }

    return nullptr;
}

/* give an affect to an object */
void Object::giveAffect(AFFECT_DATA *paf)
{
    AFFECT_DATA *paf_new;

    paf_new = new_affect();

    *paf_new = *paf;
    this->_affected.push_back(paf_new);

    /* apply any affect vectors to the object's extra_flags */
    if (paf->bitvector)
        switch (paf->where)
        {
        case TO_OBJECT:
            this->addExtraFlag(paf->bitvector);
            break;
        case TO_WEAPON:
            if (this->getItemType() == ITEM_WEAPON || this->getItemType() == ITEM_BOW)
                SET_BIT(this->getValues().at(4), paf->bitvector);
            break;
        }

    return;
}

void Object::modifyAffect(AFFECT_DATA *paf, bool fAdd) {
    throw std::logic_error("Object::modifyAffect :: not implemented");
}

void Object::removeAffect(AFFECT_DATA *paf)
{
    int where, vector;
    if (this->_affected.empty())
    {
        bug("Affect_remove_object: no affect.", 0);
        return;
    }

    if (this->getCarriedBy() != NULL && this->getWearLocation() != -1) {
        this->getCarriedBy()->modifyAffect(paf, FALSE);
    }

    where = paf->where;
    vector = paf->bitvector;

    /* remove flags from the object if needed */
    if (paf->bitvector)
        switch (paf->where)
        {
        case TO_OBJECT:
            this->removeExtraFlag(paf->bitvector);
            break;
        case TO_WEAPON:
            if (this->getItemType() == ITEM_WEAPON || this->getItemType() == ITEM_BOW)
                REMOVE_BIT(this->getValues().at(4), paf->bitvector);
            break;
        }

    _affected.remove(paf);
    free_affect(paf);

    if (this->getCarriedBy() != NULL && this->getWearLocation() != -1)
        this->getCarriedBy()->affect_check(where, vector);
    return;
}

/*
 * Move an object into an object.
 */
void Object::addObject(Object *obj)
{
    _contains.push_back(obj);
    obj->_inObject = this;
    obj->_inRoom = nullptr;
    obj->_carriedBy = nullptr;
    if (this->_pIndexData->vnum == OBJ_VNUM_PIT)
        obj->setCost(0);

    for (auto obj_to = this; obj_to != NULL; obj_to = obj_to->_inObject)
    {
        if (obj_to->_carriedBy != NULL)
        {
            obj_to->_carriedBy->carry_number += get_obj_number(obj);
            obj_to->_carriedBy->carry_weight += get_obj_weight(obj) * WEIGHT_MULT(obj_to) / 100;
        }
    }

    return;
}

/*
 * Move an object out of an object.
 */
void Object::removeObject(Object *obj)
{
    this->_contains.remove(obj);
    obj->_inObject = nullptr;

    for (auto obj_from = this; obj_from != NULL; obj_from = obj_from->_inObject)
    {
        if (obj_from->_carriedBy != NULL)
        {
            obj_from->_carriedBy->carry_number -= get_obj_number(obj);
            obj_from->_carriedBy->carry_weight -= get_obj_weight(obj) * WEIGHT_MULT(obj_from) / 100;
        }
    }

    return;
}