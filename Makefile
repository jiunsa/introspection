CC = gcc
CFLAGS = -Wall -Wextra -pthread -O2
LDFLAGS = -lcurl -lpthread

TARGET = introspection
SRC = introspection.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)

clean:
	rm -f $(TARGET)

install:
	cp $(TARGET) /usr/local/bin/

uninstall:
	rm -f /usr/local/bin/$(TARGET)

.PHONY: all clean install uninstall
