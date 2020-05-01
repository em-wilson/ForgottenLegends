CC      = g++
PROF    = -O -ggdb -I../src/
C_FLAGS =  -Wall -Wno-c++11-extensions $(PROF)
L_FLAGS =  $(PROF)

CPP_FILES := $(wildcard *.cpp)
OBJ_FILES := $(addprefix ./,$(notdir $(CPP_FILES:.cpp=.o)))

tests: $(OBJ_FILES)
	$(CC) $(L_FLAGS) -L../src/ -lm ../src/librom.a -o $@ $^

test: tests
	./tests

%.o: %.cpp
	$(CC) $(C_FLAGS) -c -o $@ $<

clean:
	rm -f *.o tests ../src/*.o

rebuild: clean tests
