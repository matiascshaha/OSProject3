all:
	@gcc -std=c99 -w -o util proj3.c
	@echo Run utility as: util fat32.img
clean:
	rm util
