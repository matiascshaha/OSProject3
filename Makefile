all:
	@gcc -std=c99 -w -o project3 proj3.c
	@echo Run project3 as: project3 fat32.img
clean:
	rm project3
