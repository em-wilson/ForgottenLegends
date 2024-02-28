CC      = g++
PROF    = -O -ggdb
DEBUG	= -DNDEBUG
NOCRYPT = -DCRYPT
#C_FLAGS =  -Wall -Wno-c++11-extensions $(PROF) $(NOCRYPT)
C_FLAGS =  -Wall $(PROF) $(DEBUG) $(NOCRYPT) -std=c++11
L_FLAGS =  $(PROF)

O_FILES = act_clan.o act_comm.o act_enter.o act_info.o act_move.o act_obj.o \
          act_wiz.o alias.o ban.o bank.o bit.o board.o bow.o Character.o ClanManager.o \
          comm.o const.o db.o db2.o effects.o EquipmentListGenerator.o fight.o flags.o \
          handler.o healer.o interp.o lookup.o magic.o map.o magic2.o mem.o mob_cmds.o \
          mob_prog.o music.o NonPlayerCharacter.o Note.o olc.o olc_act.o olc_mpcode.o \
          olc_save.o pc_data.o PlayerCharacter.o recycle.o save.o scan.o search.o \
          skills.o smclan.o special.o string.o tables.o update.o Wiznet.o

all: rom

rom:main.o librom.a
	$(CC) -lm -o rom librom.a main.o

#rom: $(O_FILES)
#	rm -f rom
#	$(CC) $(L_FLAGS) -o rom $(O_FILES)

#main.o: main.cpp
#	$(CC) $(L_FLAGS) -c main.cpp


librom.a: $(O_FILES)
	ar rcu librom.a $(O_FILES)

.cpp.o: merc.h
	$(CC) -c $(C_FLAGS) $<

clean: merc.h
	rm -f *.o *.a rom
