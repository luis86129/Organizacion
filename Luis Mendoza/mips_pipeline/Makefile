CC = gcc

CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -O2


TARGET = mips_sim

TEST = test_mips


all: $(TARGET) $(TEST)


$(TARGET): main.o mips.o

	$(CC) $(CFLAGS) -o $@ $^


$(TEST): tests.o mips.o

	$(CC) $(CFLAGS) -o $@ $^


main.o: main.c mips.h

	$(CC) $(CFLAGS) -c main.c


mips.o: mips.c mips.h

	$(CC) $(CFLAGS) -c mips.c


tests.o: tests.c mips.h

	$(CC) $(CFLAGS) -c tests.c


test: $(TEST)

	./$(TEST)


clean:

	rm -f *.o $(TARGET) $(TEST)


.PHONY: all test clean