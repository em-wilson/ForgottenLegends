#ifndef __BANMANAGER_H__
#define __BANMANAGER_H__

typedef struct descriptor_data DESCRIPTOR_DATA;

class BanManager {
    public:
        bool isSiteBanned(DESCRIPTOR_DATA *d);
        bool areNewPlayersAllowed(DESCRIPTOR_DATA *d);
};

#endif