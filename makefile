all: scheduler
scheduler: mem.c
	gcc mem.c -o scheduler
