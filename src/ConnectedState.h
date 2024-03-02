#ifndef __CONNECTED_STATE_H__
#define __CONNECTED_STATE_H__

enum ConnectedState {
    Playing                 = 0,
    GetName                 = 1,
    GetOldPassword          = 2,
    ConfirmNewName          = 3,
    GetNewPassword          = 4,
    ConfirmNewPassword      = 5,
    GetNewRace              = 6,
    GetMorph                = 7,
    GetMorphOrig            = 8,
    GetNewSex               = 9,
    GetNewClass             = 10,
    GetAlignmen             = 11,
    DefaultChoice           = 12,
    GenGroups               = 13,
    PickWeapon              = 14,
    ReadImotd               = 15,
    ReadMotd                = 16,
    BreakConnect            = 17,
    NoteTo                  = 18,
    NoteSubject             = 19,
    NoteExpire              = 20,
    NoteText                = 21,
    NoteFinish              = 22,
    CopyoverRecover         = 23,
    GetReclass              = 24,
    ReclassCust             = 25,
    ClanCreate              = 26
};

#endif