#Makefile for MacBot
CC          = g++
CFLAGS      = -I/usr/include/opencv/ -I/usr/include/flite/ -Wall -rdynamic -O3 -fPIC -g -I/usr/include/SDL -D_GNU_SOURCE=1 -D_REENTRANT -D_LINUX 
LDFLAGS     = -lpthread -ldl -lm -lcv -lhighgui -lcvaux  -lcxcore -L/usr/lib -lSDL -lSDL_ttf  -lSDL_gfx -lflite_cmu_us_kal -lflite_usenglish -lflite_cmulex -lflite -lz 
TARGET      = macbot
BIN_DIR     = /home/noth/dev/macbot/bin/
INCLUDE_DIR = /home/noth/dev/macbot/include/
EXTRA_OPT   = 
SOURCES = CamMgr.cpp IhmMgr.cpp ServoMgr.cpp TargetMgr.cpp VoiceMgr.cpp macbot.cpp Exception.cpp MotionDetect.cpp
OBJ= $(SOURCES:.cpp=.o)

%.o: %.cpp
	$(CC) -o $@ -c $< $(CFLAGS) $(EXTRA_OPT)

all: $(OBJ)
	@echo "######################################"
	@echo "# Building CORE...                   #"
	@echo "######################################"
	$(CC) $(CFLAGS) $(LDFLAGS) -I$(INCLUDE_DIR)  $^ -o $(BIN_DIR)/$(TARGET)
	cp *.hpp $(INCLUDE_DIR)

clean:
	rm -f *.o $(INCLUDE_DIR)/*.hpp $(BIN_DIR)/$(TARGET) *~
