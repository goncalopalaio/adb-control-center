# Flags
CFLAGS = -I/usr/local/include -pedantic -O2  -Wno-deprecated -Wno-vla-extension
CC= gcc
#LIBS := -lglfw3 -lopengl32 -lm -lGLU32 -lGLEW32
#LIBS := -lglfw -lGL -lm -lGLU -lGLEW
LIBS := -lglfw3 -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo -lm -lGLEW -L/usr/local/lib
EXT_LIBS :=
OBJS = connection.o

all: main

clean:
	@rm -rf $(OBJS)
test_con: ${OBJS}
	$(CC) $(CFLAGS) $(OBJS) $(EXT_LIBS) -o test_con test_con.c $(LIBS)
	

main: ${OBJS} 
	$(CC) $(CFLAGS) $(OBJS) $(EXT_LIBS) -o main main.c $(LIBS)

connection.o: connection.c connection.h
