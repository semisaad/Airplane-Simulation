CC = gcc
CFLAGS = -I. -Wall
LDFLAGS = -lraylib -lm -ldl -pthread -lGL -lX11

SOURCES = main.c 
TARGET = my_project

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCES) $(LDFLAGS)

clean:
	rm -f $(TARGET)
