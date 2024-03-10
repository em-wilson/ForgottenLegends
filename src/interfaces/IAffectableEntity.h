#ifndef __IAFFECTABLEENTITY_H__
#define __IAFFECTABLEENTITY_H__

typedef struct	affect_data		    AFFECT_DATA;

class IAffectableEntity {
    public:
        virtual AFFECT_DATA * findAffectBySn(int sn) = 0;
        virtual void removeAffect(AFFECT_DATA *af) = 0;
        virtual void giveAffect( AFFECT_DATA *paf ) = 0; // replaces affect_to_char, affect_to_obj
        virtual void modifyAffect(AFFECT_DATA *paf, bool fAdd) = 0; // replaces affect_modify
};

#endif