#Makefile for MacBot
CC          = g++
CFLAGS      = -I/usr/include/opencv/ -I/usr/include/flite/ -Wall -rdynamic -O3 -fPIC -g -I/usr/include/SDL -D_GNU_SOURCE=1 -D_REENTRANT -D_LINUX 
LDFLAGS_CLIENT  = -lpthread -ldl -L/opt/local/lib/ -lm -L/usr/lib -lSDL -lSDL_gfx -lz 
TARGET      = client
BIN_DIR     = /home/noth/dev/macbot/bin/
INCLUDE_DIR = /home/noth/dev/macbot/include/

SOURCES = client_macbot.cpp
OBJ= $(SOURCES:.cpp=.o)

%.o: %.cpp
	$(CC) -o $@ -c $< $(CFLAGS)

all: $(OBJ)
	@echo "######################################"
	@echo "# Building Client...                 #"
	@echo "######################################"
	$(CC) $(CFLAGS) $(LDFLAGS_CLIENT) -I$(INCLUDE_DIR)  $^ -o $(BIN_DIR)/$(TARGET)

clean:
	rm -f *.o $(INCLUDE_DIR)/*.hpp $(BIN_DIR)/$(TARGET) *~
