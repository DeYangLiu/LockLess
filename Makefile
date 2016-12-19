
CFLAGS = -Wall
ifdef DEBUG
CFLAGS+=	-g -O0 -DDEBUG -fno-omit-frame-pointer
CFLAGS+=	-fprofile-arcs -ftest-coverage
else
CFLAGS += -O3 -fno-omit-frame-pointer
endif

LDFLAGS += -pthread

SRC = TestHarness.c 

lazy: $(SRC) LinkedListLazy.c febr.c
	gcc -o $@ $^ $(CFLAGS) $(LDFLAGS) 

all: lazy fine opti 

fine: $(SRC) LinkedListFine.c
	gcc -o $@ $^ $(CFLAGS) $(LDFLAGS)

opti: $(SRC) LinkedListOptimistic.c
	gcc -o $@ $^ $(CFLAGS) $(LDFLAGS)


clean:
	rm -f *.o *.gcno *.gcda *.gcov  fine opti lazy *.exe a.out *.log
