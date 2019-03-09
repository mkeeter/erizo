hedgehog: main.c
	$(CC) -O3 -o hedgehog -lglfw -lglew -framework OpenGL -std=c89 -Wall -g main.c
