
CC = gcc
CFLAGS = -Wall
LDFLAGS = -lgpiod -lmicrohttpd -lcjson -lcurl
SRCS = Main.c GlueHandlers.c Server.c Config.c
TARGET = out

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET) $(LDFLAGS)
clean:
	rm -f $(TARGET)
