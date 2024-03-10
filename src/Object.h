#ifndef __OBJECT_H__
#define __OBJECT_H__

#include <string>
#include <vector>

#include "IAffectableEntity.h"
#include "IObjectContainer.h"

class ExtraDescription;
class ROOM_INDEX_DATA;

typedef struct affect_data AFFECT_DATA;
typedef struct obj_index_data OBJ_INDEX_DATA;

class Object : public IAffectableEntity, IObjectContainer {
    public:
        Object();
        virtual ~Object();

        // IObjectContainer
        void addObject(Object *obj);
        void removeObject(Object *obj);

        // IAffectableEntity
        AFFECT_DATA * findAffectBySn(int sn);
        void removeAffect(AFFECT_DATA *af);
        void modifyAffect(AFFECT_DATA *paf, bool fAdd);
        virtual void giveAffect( AFFECT_DATA *paf ); // replaces affect_to_obj

        Object * clone();

        bool canWear(long position);
        std::vector<AFFECT_DATA *> getAffectedBy();
        void addAffect(AFFECT_DATA *af);
        Character * getCarriedBy();
        void setCarriedBy(Character *ch);
        short int getCondition();
        void setCondition(short int value);
        std::vector<Object *> getContents();
        int getCost();
        void setCost(int value);
        std::string getDescription();
        void setDescription( std::string value );
        std::vector<ExtraDescription *> getExtraDescriptions();
        void addExtraDescription(ExtraDescription *ed);
        int getExtraFlags();
        void setExtraFlags(int flags);
        void addExtraFlag(int bit);
        void removeExtraFlag(int bit);
        std::string getShortDescription();
        void setShortDescription(std::string value);
        Object * getInObject();
        void setInObject(Object *obj);
        ROOM_INDEX_DATA * getInRoom();
        void setInRoom(ROOM_INDEX_DATA *room);
        short int getItemType();
        void setItemType(short int value);
        short int getLevel();
        void setLevel(short int level);
        std::string getName();
        void setName(std::string value);
        OBJ_INDEX_DATA * getObjectIndexData();
        OBJ_INDEX_DATA * setObjectIndexData(OBJ_INDEX_DATA *pIndexData);
        std::vector<int> getValues();
        bool isEnchanted();
        void setIsEnchanted(bool value);
        virtual bool isPortal();
        int getWearFlags();
        void setWearFlags(int value);
        short int getWearLocation();
        void setWearLocation(short int value);
        short int getWeight();
        void setWeight(short int value);
        bool hasOwner();
        std::string getOwner();
        void setOwner(std::string name);
        bool hasStat(long stat);
        void setTimer(short int value);
        short int decrementTimer(short int value = 1);
        short int getTimer();

    private:
        std::list<AFFECT_DATA *> _affected;
        std::list<Object *> _contains;
        Character * _carriedBy;
        short int _condition;
        int _cost;
        std::string _description;
        bool _enchanted;
        std::list<ExtraDescription *> _extraDescriptions;
        int _extraFlags;
        Object * _inObject;
        ROOM_INDEX_DATA * _inRoom;
        short int _level;
        std::string _name;
        short int _itemType;
        std::string _owner;
        OBJ_INDEX_DATA * _pIndexData;
        std::string _shortDescription;
        short int _timer;
        std::vector<int> _values;
        int _wearFlags;
        short int _wearLocation;
        short int _weight;
        std::string _material;
};

// /*
//  * One object.
//  */
// struct	obj_data
// {
//     Object *		next;
//     Object *		next_content;
//     Object *		contains;
//     Object *		in_obj;
//     Object *		on;
//     Character *		carried_by;
//     ExtraDescription *	extra_descr;
//     AFFECT_DATA *	affected;
//     OBJ_INDEX_DATA *	pIndexData;
//     ROOM_INDEX_DATA *	in_room;
//     bool		valid;
//     bool		enchanted;
//     char *	        owner;
//     char *		name;
//     char *		short_descr;
//     char *		description;
//     sh_int		item_type;
//     int			extra_flags;
//     int			wear_flags;
//     sh_int		wear_loc;
//     sh_int		weight;
//     int			cost;
//     sh_int		level;
//     sh_int 		condition;
//     char *		material;
//     sh_int		timer;
//     sh_int		range;
//     int			value	[5];
// };

#endif