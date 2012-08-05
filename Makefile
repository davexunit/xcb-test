all:
	gcc -o winman main.c `pkg-config --cflags --libs xcb xcb-aux xcb-atom xcb-ewmh` -Wall -Wextra -ansi -pedantic -std=c99
