#include <string.h>
#include <sys/types.h>
#include "merc.h"
#include "PlayerCharacter.h"
#include "Wiznet.h"

PlayerCharacter::PlayerCharacter()
        : Character() {
        this->was_note_room = NULL;
}

PlayerCharacter::~PlayerCharacter() {

}

void PlayerCharacter::gain_exp( int gain ) {
        char buf[MAX_STRING_LENGTH];

        if ( IS_NPC(this) || this->level >= LEVEL_HERO )
                return;

        this->exp = UMAX( exp_per_level(this,this->pcdata->points), this->exp + gain );
        while ( this->level < LEVEL_HERO && this->exp >= exp_per_level(this,this->pcdata->points) * (this->level+1) )
        {
                send_to_char( "You raise a level!!  ", this );
                this->level += 1;
                sprintf(buf,"%s gained level %d",this->getName(),this->level);
                log_string(buf);
                sprintf(buf,"$N has attained level %d!",this->level);
                this->advance_level(FALSE);
                save_char_obj(this);
                Wiznet::instance()->report( buf, this, NULL, WIZ_LEVELS, 0, 0 );
        }

        return;
}

ROOM_INDEX_DATA * PlayerCharacter::getWasNoteRoom() {
        return this->was_note_room;
}

void PlayerCharacter::setWasNoteRoom( ROOM_INDEX_DATA *room ) {
        this->was_note_room = room;
        return;
}