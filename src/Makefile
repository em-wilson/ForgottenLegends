CC      = g++
PROF    = -O -ggdb
NOCRYPT = -DNOCRYPT
C_FLAGS =  -Wall $(PROF) $(NOCRYPT)
L_FLAGS =  $(PROF)

O_FILES = act_clan.o act_comm.o act_enter.o act_info.o act_move.o act_obj.o \
          act_wiz.o alias.o ban.o bank.o bit.o board.o bow.o char_data.o comm.o const.o db.o \
          db2.o effects.o fight.o flags.o handler.o healer.o interp.o \
          lookup.o magic.o map.o magic2.o mem.o mob_cmds.o mob_prog.o music.o \
          olc.o olc_act.o olc_mpcode.o olc_save.o pc_data.o recycle.o save.o scan.o \
          search.o skills.o smclan.o special.o string.o tables.o update.o

rom: $(O_FILES)
	rm -f rom
	$(CC) $(L_FLAGS) -o rom $(O_FILES)

.c.o: merc.h
	$(CC) -c $(C_FLAGS) $<
