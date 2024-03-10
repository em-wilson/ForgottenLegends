#ifndef __PORTAL_H__
#define __PORTAL_H__

#include "Object.h"

class Character;

class PortalException : public std::exception { };

class PortalUeePreventedByCurseException : public PortalException { };

class PortalNotTraversableException : public PortalException { };

class Portal : public Object {
    public:
        Portal(Object *obj);
        virtual ~Portal();
        void enter(Character *ch);
        bool isPortal() override;
        bool isTraversableByCharacter(Character *ch);
};

#endif