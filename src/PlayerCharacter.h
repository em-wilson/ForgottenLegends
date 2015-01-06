#include "Character.h"

class PlayerCharacter : public Character {
public:
    PlayerCharacter();
    ~PlayerCharacter();

    void gain_exp(int gain);
    ROOM_INDEX_DATA * getWasNoteRoom();
    void setWasNoteRoom( ROOM_INDEX_DATA *room );

private:
    ROOM_INDEX_DATA *	was_note_room;
};