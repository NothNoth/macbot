#Makefile for MacBot
CC          = g++
CFLAGS      = -I/usr/include/opencv/ -I/usr/include/flite/ -Wall -rdynamic -O3 -fPIC -g -I/usr/include/SDL -D_GNU_SOURCE=1 -D_REENTRANT -D_LINUX -shared -rdynamic -fPIC
TARGET      = parallax.lib
LIB_DIR     = /home/noth/dev/macbot/lib/
INCLUDE_DIR = /home/noth/dev/macbot/include/

SOURCES = driver_parallax.cpp  SerialStreamBuf.cpp  SerialStream.cpp
OBJ= $(SOURCES:.cpp=.o)

%.o: %.cpp
	$(CC) -o $@ -c $< $(CFLAGS) -I./ -I$(INCLUDE_DIR)


all: $(OBJ)
	$(CC) $(CFLAGS)  *.o -o $(LIB_DIR)/$(TARGET)

clean:
	rm -f *.o *~ $(LIB_DIR)/$(TARGET) 
