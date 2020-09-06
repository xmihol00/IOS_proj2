#================================================================================================
# File:        Makefile
# Case:        IOS proj2, 15. 4. 2020
# Author:      David Mihola, FIT, xmihol00@stud.fit.vutbr.cz
# Description: compilation of program proj2
#================================================================================================

CC = gcc
CFLAGS = -std=gnu99 -Wall -Wextra -Werror -pedantic
LDFLAGS = -pthread -lrt
OBJ = immigrant.o judge.o proj2-func.o proj2-main.o
EXE = proj2
DEF =
### preprocessor defines for conditional compilation (for testing and debugging)
#DEF += -DDEBUG
#To activate noise testing, debugging has to be also activated. All the testing macrose were removed from the source files.
#They can be only added, when DEBUG is defined
#DEF += -DNOISE__TEST1
#DEF += -DNOISE__TEST2
#DEF += -DNOISE__TEST3
#DEF += -DNOISE__TEST4
#DEF += -DNOISE__TEST5

.PHONY: clean pack all

all: $(EXE)

immigrant.o: immigrant.c immigrant.h proj2.h
	$(CC) immigrant.c $(CFLAGS) $(DEF) -c -o immigrant.o

judge.o: judge.c judge.h proj2.h
	$(CC) judge.c $(CFLAGS) $(DEF) -c -o judge.o

proj2-func.o: proj2-func.c proj2.h immigrant.h judge.h
	$(CC) proj2-func.c $(CFLAGS) $(DEF) -c -o proj2-func.o

proj2-main.o: proj2-main.c immigrant.h judge.h proj2.h
	$(CC) proj2-main.c $(CFLAGS) $(DEF) -c -o proj2-main.o

proj2: $(OBJ)
	$(CC) $(OBJ) $(CFLAGS) $(LDFLAGS) -o proj2


clean:
	rm *.o $(EXE)

pack:
	zip bonus.zip *.c *.h Makefile
