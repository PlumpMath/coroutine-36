EXAMPLES = main

INCLUDEDIR = ../
COLIB 	= ../libcoroutine.a
HEADER 	= ../coroutine.h

CFLAGS = -Wall -g -DNDEBUG

all : $(EXAMPLES)

main : main.c $(COLIB) $(HEADER)
	gcc $(CFLAGS) -I$(INCLUDEDIR) main.c $(COLIB) -o $@ 

clean :
	rm -rf $(EXAMPLES)
