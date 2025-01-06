CC = gcc
IN = main.c src/main_state.c src/glad/glad.c src/water/frame_buffers.c src/utility.c
OUT = main.out
CFLAGS = -Wall -DGLFW_INCLUDE_NONE
LFLAGS = -lglfw -ldl -lm
IFLAGS = -I. -I./include

.SILENT all: clean build run

clean:
	rm -f $(OUT)

build: $(IN) include/main_state.h include/stb_image.h include/frame_buffers.h include/utility.h
	$(CC) $(IN) -o $(OUT) $(CFLAGS) $(LFLAGS) $(IFLAGS)

run: $(OUT)
	./$(OUT)
