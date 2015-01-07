#include "Character.h"

class NonPlayerCharacter : public Character {
public:
    NonPlayerCharacter(MOB_INDEX_DATA *pMobIndex);
    ~NonPlayerCharacter();
    bool isNPC();

    virtual void update() override;
    virtual int hit_gain() override;
    virtual int mana_gain( ) override;
    virtual int move_gain( ) override;
};