#ifndef __IOBJECTCONTAINER_H__
#define __IOBJECTCONTAINER_H__

class Object;

class IObjectContainer {
    virtual void addObject(Object *obj) = 0;
    virtual void removeObject(Object *obj) = 0;
};

#endif