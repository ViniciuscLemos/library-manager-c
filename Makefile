# make, make test, make run, make clean

CC      = gcc
CFLAGS  = -Wall -Wextra -std=c17 -g
SRC     = src/main.c src/library.c
HEADERS = src/library.h
TARGET  = library
TEST    = test_library

all: $(TARGET)

$(TARGET): $(SRC) $(HEADERS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)
	@echo "Build done! Run it with: ./$(TARGET)"

# in CI I pass SANITIZE="-fsanitize=address,undefined" to catch memory errors
test: tests/test_library.c src/library.c $(HEADERS)
	$(CC) $(CFLAGS) $(SANITIZE) -o $(TEST) tests/test_library.c src/library.c
	./$(TEST)

clean:
	rm -f $(TARGET) $(TARGET).exe $(TEST) $(TEST).exe library.dat library.dat.bak library.csv

run: $(TARGET)
	./$(TARGET)

.PHONY: all test clean run
