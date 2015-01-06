#include "Character.h"

class PlayerCharacter : public Character {
public:
    PlayerCharacter();
    ~PlayerCharacter();

    void gain_exp(int gain);
};