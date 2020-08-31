all: hwloc_ex.c Makefile
	gcc -Wall -Werror hwloc_ex.c -o x -lhwloc

