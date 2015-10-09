CC := g++
CFLAGS := -g -O $$(sdl2-config --cflags) -I.. -std=c++11 \
  -Wuninitialized -Wshadow -Wall -Wno-multichar -Wno-delete-non-virtual-dtor \
  -Wno-delete-non-virtual-dtor -Wno-sequence-point
LDFLAGS := $$(sdl2-config --libs) -lSDL2_ttf
OBJ := make-waves.o dump-wave.o ../sdl-widgets.a
# sdl2-config --cflags = -I/usr/local/include/SDL2 -D_REENTRANT
# sdl2-config --libs   = -L/usr/local/lib -Wl,-rpath,/usr/local/lib -lSDL2 -lpthread

.SUFFIXES=

make-waves: $(OBJ)
	make -C .. archive
	$(CC) $(OBJ) -o $@ $(LDFLAGS)

%.o: %.cpp
	$(CC) -c $< $(CFLAGS)

make-waves.o: ../sdl-widgets.h dump-wave.h filter-test.cpp 
dump-wave.o: dump-wave.h
