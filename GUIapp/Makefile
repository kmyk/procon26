DEBUG		=  "ON"
OBJS		=	 Main.o GUIapp.o State.o MainWindow.o SubWindow.o Mouse.o Map.o Cell.o Stone.o Keyboard.o

CURPATH = 	$(shell pwd)
INCLUDE =		$(CURPATH)/include
LIB			=		$(CURPATH)/lib
EXE_FILE			=		$(CURPATH)/GUIapp.elf

CXX 		= 	g++ -fstack-protector-all -D_FORTIFY_SOURCE=2
LIBSDL_CXXFLAGS = $(shell sdl2-config --cflags)
LIBSDL_LDFLAGS  = $(shell sdl2-config --libs)

ifeq ($(DEBUG),"ON")
CXXFLAG =		-Wall -I$(INCLUDE) -std=c++11 $(LIBSDL_CXXFLAGS) -Wdelete-incomplete -g -o
else
CXXFLAG =		-Wall -I$(INCLUDE) -std=c++11 $(LIBSDL_CXXFLAGS) -Wdelete-incomplete -o
endif

LDFLAG	=		-Wl,-z,relro,-z,now -lSDL2_image $(LIBSDL_LDFLAGS) -lSDL2_gfx -lSDL2_ttf -L$(LIB) -lsdl-widgets

CHKSEC	=		$(shell which checksec.sh)

$(EXE_FILE) : $(OBJS) $(LIB)/libsdl-widgets.a
	$(CXX) $(CXXFLAG) $@ $^ $(LDFLAG)

security:
ifneq ("$(CHKSEC)","")
	$(CHKSEC) --file $(EXE_FILE)
else
	echo '[**ERROR**] checksec.sh not found'
endif

clean :
	rm -rf $(EXE_FILE)
	find . -name "*.o" -exec rm -rf {} \;

%.o : %.cpp
	$(CXX) $(CXXFLAG) $*.o -c $*.cpp
