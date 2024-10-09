
CC = gcc
CFLAGS = -Wall
LDFLAGS = -lgpiod -lmicrohttpd -lcjson
SRCS = Main.c GlueHandlers.c Server.c
TARGET = out

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET) $(LDFLAGS)
clean:
	rm -f $(TARGET)
