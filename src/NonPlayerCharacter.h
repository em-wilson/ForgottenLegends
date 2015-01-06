#include "Character.h"

class NonPlayerCharacter : public Character {
public:
    NonPlayerCharacter(MOB_INDEX_DATA *pMobIndex);
    ~NonPlayerCharacter();
    bool isNPC();
};