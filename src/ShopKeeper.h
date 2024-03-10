#ifndef __SHOPKEEPER_H__
#define __SHOPKEEPER_H__

#include <string>

class Character;

class ShopKeeper {
    public:
        ShopKeeper(Character *ch);
        virtual ~ShopKeeper();
        static ShopKeeper * find(Character *ch);
        Character * getCharacter();
        void buy(Character *ch, std::string argument);
        void sell(Character *ch, std::string argument);
        void list(Character *ch, std::string arg);

        // TODO: Refactor
        int get_cost( Object *obj, bool fBuy );
        Object *get_obj_keeper( Character *ch, Character *keeper, char *argument );
        void obj_to_keeper( Object *obj, Character *ch );

    private:
        Character *_keeper;
};

#endif