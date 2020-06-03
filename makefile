#makefile structure and design obtained from
#http://www.cs.colby.edu/maxwell/courses/tutorials/maketutor/
CFLAGS = -I.
DEPS = config.h process.h queue.h sorted_pages.h memory_manager.h schedulers.h stats.h
CC = gcc

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

main: stats.o queue.o sorted_pages.o memory_manager.o schedulers.o runner.o
	$(CC) -o scheduler stats.o queue.o sorted_pages.o memory_manager.o schedulers.o runner.o
#make clean structure obtained from
#https://www.gnu.org/software/make/manual/html_node/Cleanup.html
.PHONY : clean
clean :
	-rm *.o $(objects)

