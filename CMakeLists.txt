cmake_minimum_required(VERSION 2.8.4)
project(ForgottenLegends)

set(LIB_LIST cryptlib.lib
            cryptlibd.lib)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES
    src/act_clan.cpp
    src/act_comm.cpp
    src/act_enter.cpp
    src/act_info.cpp
    src/act_move.cpp
    src/act_obj.cpp
    src/act_wiz.cpp
    src/alias.cpp
    src/ban.cpp
    src/bank.cpp
    src/bit.cpp
    src/board.cpp
    src/board.h
    src/bow.cpp
    src/Character.cpp
    src/Character.h
    src/clan.h
    src/colordef.h
    src/comm.cpp
    src/const.cpp
    src/db.cpp
    src/db.h
    src/db2.cpp
    src/effects.cpp
    src/fight.cpp
    src/flags.cpp
    src/handler.cpp
    src/healer.cpp
    src/interp.cpp
    src/interp.h
    src/lookup.cpp
    src/lookup.h
    src/magic.cpp
    src/magic.h
    src/magic2.cpp
    src/map.cpp
    src/mem.cpp
    src/merc.h
    src/mob_cmds.cpp
    src/mob_cmds.h
    src/mob_prog.cpp
    src/music.cpp
    src/music.h
    src/Note.cpp
    src/Note.h
    src/olc.cpp
    src/olc.h
    src/olc_act.cpp
    src/olc_mpcode.cpp
    src/olc_save.cpp
    src/pc_data.cpp
    src/pc_data.h
    src/recycle.cpp
    src/recycle.h
    src/save.cpp
    src/scan.cpp
    src/search.cpp
    src/skills.cpp
    src/smclan.cpp
    src/special.cpp
    src/string.cpp
    src/tables.cpp
    src/tables.h
    src/telnet.h
    src/update.cpp
    src/Wiznet.cpp
    src/Wiznet.h)

add_executable(ForgottenLegends ${SOURCE_FILES})