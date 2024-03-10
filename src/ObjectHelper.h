#ifndef __OBJECTHELPER_H__
#define __OBJECTHELPER_H__

#include <list>
#include <string>
#include <vector>

class Character;
class Object;
typedef obj_index_data OBJ_INDEX_DATA;

class ObjectHelper {
    public:
        static int countInList(OBJ_INDEX_DATA *pObjIndex, std::list<Object *> list);
        static int countInList(OBJ_INDEX_DATA *pObjIndex, std::vector<Object *> list);
        static Object * createFromIndex( OBJ_INDEX_DATA *pObjIndex, int level );
        static Object * findInList(Character *ch, std::string name, std::list<Object *> list);
        static Object * findInList(Character *ch, std::string name, std::vector<Object *> list);
        static Object * getFountainFromList(std::vector<Object *> list);
        static bool isObjectOwnedBy(Object *obj, Character *ch);
};

#endif