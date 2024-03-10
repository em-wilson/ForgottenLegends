#ifndef __ROOM_MANAGER_H__
#define __ROOM_MANAGER_H__

class ROOM_INDEX_DATA;

class RoomManager {
    public:
        void char_from_room( Character *ch );
        void char_to_room( Character *ch, ROOM_INDEX_DATA *pRoomIndex );
        ROOM_INDEX_DATA * get_limbo_room();
};

#endif